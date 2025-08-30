// /src/planner/planner.h

/**
 * Planner for Venus DB
 *
 * Planner takes in Bounded AST emitted by Binder, and generates a Plan
 *
 * The generated Plan is a tree-like structure that represents the
 * execution strategy for the query.
 *
 * Generally Plan is of 2 types - Logical and Physical Plan
 *
 * Logical Plan: Represents the high-level query structure and semantics.
 * Physical Plan: Represents the low-level execution strategy, including
 *                operator implementations, execution order, using of indexes etc.
 *
 * Plan shall be relevent to relational model
 *
 * For Venus DB, We will consider Logical and Physical plan as a single entity `Plan`
 * The generator Plan goes through executor as Iterator model (Open, Close, Next)
 *
 * Imp: Binder creates important references like TableRef, ColumnRef, Schema
 * which are needed to be reused by planner and by executor
 *
 *
 * Relational Algebra Operators:
 *   SEQ_SCAN       — Full table scan (default SELECT input)
 *   INDEX_SCAN     — Index-based scan (future optimization)
 *   PROJECTION     — Column selection (SELECT col1, col2)
 *   FILTER         — Row filtering (WHERE clauses)
 *   JOIN           — Row combination from multiple tables (JOIN ... ON ...)
 *   AGGREGATION    — Grouping and aggregation (GROUP BY, aggregates)
 *   SORT           — Ordering rows (ORDER BY)
 *   LIMIT          — Restricting row count (LIMIT n)
 *   INSERT         — Insert rows into a table
 *   UPDATE         — Update existing rows
 *   DELETE         — Delete rows
 *
 * Data Definition (DDL):
 *   CREATE_DATABASE — Create a new database
 *   DROP_DATABASE   — Remove an existing database
 *   USE_DATABASE    — Switch current database
 *   CREATE_TABLE    — Create a new table
 *   DROP_TABLE      — Remove a table
 *   CREATE_INDEX    — Create an index on table columns
 *   DROP_INDEX      — Remove an index
 *
 * Utility / Meta Commands:
 *   SHOW_DATABASES — List available databases
 *   SHOW_TABLES    — List tables in current database
 *   HELP           — Display help message
 *   EXIT           — Exit the shell
 *   EXEC_FILE      — Execute SQL from a file
 *
 *
 * eg
 * SELECT name FROM employees WHERE age > 30 ORDER BY name LIMIT 10
 *   → LIMIT
 *        → SORT(columns=[name])
 *             → PROJECTION(columns=[name])
 *                  → FILTER(condition="age > 30")
 *                       → SEQ_SCAN(table=employees)
 *
 * SELECT e.name, d.name FROM employees e JOIN departments d ON e.department_id = d.id
 *   → PROJECTION(columns=[e.name, d.name])
 *        → JOIN(condition="e.department_id = d.id")
 *             → SEQ_SCAN(table=employees, alias=e)
 *             → SEQ_SCAN(table=departments, alias=d)
 *
 * INSERT INTO employees (name, age) VALUES ('Alice', 30)
 *   → INSERT(table=employees, columns=[name, age], values=[["Alice", "30"]])
 *
 * UPDATE employees SET age=31 WHERE name='Alice'
 *   → UPDATE(table=employees, set_clauses={age=31}, condition="name='Alice'")
 */

#pragma once

#include "buffer/buffer_pool.h"
#include "catalog/catalog.h"
#include "common/types.h"
#include "parser/ast.h"
#include "storage/tuple.h"
#include "table/table_heap.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

using namespace venus::parser;

namespace venus {
namespace planner {

	class PlanNode {
	public:
		PlanNodeType type_;
		std::vector<std::unique_ptr<PlanNode>> children_;

		explicit PlanNode(PlanNodeType type)
		    : type_(type) { }

		virtual ~PlanNode() = default;

		void AddChild(std::unique_ptr<PlanNode> child) {
			children_.push_back(std::move(child));
		}

		PlanNodeType GetType() const {
			return type_;
		}

		const std::vector<std::unique_ptr<PlanNode>>& GetChildren() const {
			return children_;
		}

		virtual void Print(int depth = 0) const = 0;
	};

	class SeqScanPlanNode : public PlanNode {
	public:
		TableRef* table_ref_;

		table::TableHeap* table_heap_;
		std::unique_ptr<table::TableHeap::Iterator> iterator_;
		std::unique_ptr<table::TableHeap::Iterator> end_iterator_;
		bool is_open_;

		explicit SeqScanPlanNode(TableRef* table_ref)
		    : PlanNode(PlanNodeType::SEQ_SCAN)
		    , table_ref_(table_ref)
		    , table_heap_(nullptr)
		    , is_open_(false) { }

		void Print(int depth = 0) const override {
			for (int i = 0; i < depth; i++)
				std::cout << "  ";
			std::cout << "SeqScan(table=" << table_ref_->table_name << ", id=" << table_ref_->table_id << ")\n";
		}
	};

	class ProjectionPlanNode : public PlanNode {
	public:
		std::vector<ColumnRef> column_refs_;
		Schema output_schema_;

		ProjectionPlanNode(const std::vector<ColumnRef>& column_refs)
		    : PlanNode(PlanNodeType::PROJECTION)
		    , column_refs_(column_refs) {
			for (const auto& col_ref : column_refs_) {
				output_schema_.AddColumn(
				    col_ref.GetName(),
				    col_ref.column_entry_->GetType(),
				    col_ref.column_entry_->IsPrimary(),
				    output_schema_.GetColumnCount());
			}
		}

		void Print(int depth = 0) const override {
			for (int i = 0; i < depth; i++)
				std::cout << "  ";
			std::cout << "Projection(columns=[";
			for (size_t i = 0; i < column_refs_.size(); i++) {
				std::cout << column_refs_[i].GetName();
				if (i < column_refs_.size() - 1)
					std::cout << ", ";
			}
			std::cout << "])\n";
			for (const auto& child : children_) {
				child->Print(depth + 1);
			}
		}
	};

	class InsertPlanNode : public PlanNode {
	public:
		TableRef* table_ref;
		std::vector<ColumnRef> target_cols;
		std::vector<ConstantType> values;
		bool executed_;

		InsertPlanNode(TableRef *table_ref, const std::vector<ColumnRef>& target_cols,
		    const std::vector<ConstantType>& values)
		    : PlanNode(PlanNodeType::INSERT)
		    , table_ref(table_ref)
		    , target_cols(target_cols)
		    , values(values)
		    , executed_(false) { }

		void Print(int depth = 0) const override {
			for (int i = 0; i < depth; i++)
				std::cout << "  ";
			std::cout << "Insert(table=" << table_ref->table_name << ", values=[";
			for (size_t i = 0; i < values.size(); i++) {
				std::cout << values[i].value.c_str();
				if (i < values.size() - 1)
					std::cout << ", ";
			}
			std::cout << "])\n";
		}
	};

	class CreateTablePlanNode : public PlanNode {
	public:
		std::string table_name_;
		Schema schema_;
		bool executed_;

		CreateTablePlanNode(const std::string& table_name, const Schema& schema)
		    : PlanNode(PlanNodeType::CREATE_TABLE)
		    , table_name_(table_name)
		    , schema_(schema)
		    , executed_(false) { }

		void Print(int depth = 0) const override {
			for (int i = 0; i < depth; i++)
				std::cout << "  ";
			std::cout << "CreateTable(table=" << table_name_ << ", columns=[";
			for (size_t i = 0; i < schema_.GetColumnCount(); i++) {
				std::cout << schema_.GetColumn(i).GetName();
				if (i < schema_.GetColumnCount() - 1)
					std::cout << ", ";
			}
			std::cout << "])\n";
		}
	};

	class DatabaseOpPlanNode : public PlanNode {
	public:
		std::string database_name_;
		bool executed_;

		DatabaseOpPlanNode(PlanNodeType type, const std::string& database_name = "")
		    : PlanNode(type)
		    , database_name_(database_name)
		    , executed_(false) { }

		void Print(int depth = 0) const override {
			for (int i = 0; i < depth; i++)
				std::cout << "  ";
			std::cout << GetOperation() << "(";
			if (!database_name_.empty()) {
				std::cout << "database=" << database_name_;
			}
			std::cout << ")\n";
		}

	private:
		std::string GetOperation() const {
			switch (type_) {
			case PlanNodeType::CREATE_DATABASE:
				return "CreateDatabase";
			case PlanNodeType::DROP_DATABASE:
				return "DropDatabase";
			case PlanNodeType::USE_DATABASE:
				return "UseDatabase";
			case PlanNodeType::SHOW_DATABASES:
				return "ShowDatabases";
			default:
				throw std::runtime_error("Planner error: Unknown Database Operation");
			}
		}
	};

	class ShowTablesPlanNode : public PlanNode {
	public:
		bool executed_;

		ShowTablesPlanNode()
		    : PlanNode(PlanNodeType::SHOW_TABLES)
		    , executed_(false) { }

		void Print(int depth = 0) const override {
			for (int i = 0; i < depth; i++)
				std::cout << "  ";
			std::cout << "ShowTables()\n";
		}
	};

	class DropTablePlanNode : public PlanNode {
	public:
		std::string table_name_;
		bool executed_;

		DropTablePlanNode(const std::string& table_name)
		    : PlanNode(PlanNodeType::DROP_TABLE)
		    , table_name_(table_name)
		    , executed_(false) { }

		void Print(int depth = 0) const override {
			for (int i = 0; i < depth; i++)
				std::cout << "  ";
			std::cout << "DropTable(table=" << table_name_ << ")\n";
		}
	};

	class Planner {
	public:
		Planner() = default;
		~Planner() = default;

		std::unique_ptr<PlanNode> Plan(std::unique_ptr<BoundASTNode> bound_ast);

		void PrintPlan(const std::unique_ptr<PlanNode>& plan) {
			if (plan) {
				plan->Print();
			}
		}
	};

} // namespace planner
} // namespace venus