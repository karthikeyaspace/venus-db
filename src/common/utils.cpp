// src/common/utils.cpp

#include "common/utils.h"

namespace venus {
namespace utils {

	void PrintPlan(const std::unique_ptr<planner::PlanNode>& plan, int depth) {
		if (!plan) {
			return;
		}

		for (int i = 0; i < depth; i++) {
			std::cout << "  ";
		}

		switch (plan->GetType()) {
		case PlanNodeType::SEQ_SCAN: {
			auto* seq_scan = static_cast<const planner::SeqScanPlanNode*>(plan.get());
			std::cout << "SeqScan(table=" << seq_scan->table_ref_->table_name
			          << ", id=" << seq_scan->table_ref_->table_id << ")\n";
			break;
		}
		case PlanNodeType::PROJECTION: {
			auto* projection = static_cast<const planner::ProjectionPlanNode*>(plan.get());
			std::cout << "Projection(columns=[";
			for (size_t i = 0; i < projection->column_refs_.size(); i++) {
				std::cout << projection->column_refs_[i].GetName();
				if (i < projection->column_refs_.size() - 1) {
					std::cout << ", ";
				}
			}
			std::cout << "])\n";
			break;
		}
		case PlanNodeType::INSERT: {
			auto* insert_plan = static_cast<const planner::InsertPlanNode*>(plan.get());
			std::cout << "Insert(table=" << insert_plan->table_ref->table_name << ", values=[";
			for (size_t i = 0; i < insert_plan->values.size(); i++) {
				std::cout << insert_plan->values[i].value.c_str();
				if (i < insert_plan->values.size() - 1) {
					std::cout << ", ";
				}
			}
			std::cout << "])\n";
			break;
		}
		case PlanNodeType::INSERT_BULK: {
			auto* bulk_insert_plan = static_cast<const planner::BulkInsertPlanNode*>(plan.get());
			std::cout << "BulkInsert(table=" << bulk_insert_plan->table_ref->table_name
			          << ", value_sets=" << bulk_insert_plan->value_sets.size() << ", values=[";
			for (size_t set_idx = 0; set_idx < bulk_insert_plan->value_sets.size() && set_idx < 3; set_idx++) {
				std::cout << "(";
				const auto& value_set = bulk_insert_plan->value_sets[set_idx];
				for (size_t i = 0; i < value_set.size(); i++) {
					std::cout << value_set[i].value.c_str();
					if (i < value_set.size() - 1) {
						std::cout << ", ";
					}
				}
				std::cout << ")";
				if (set_idx < bulk_insert_plan->value_sets.size() - 1 && set_idx < 2) {
					std::cout << ", ";
				}
			}
			if (bulk_insert_plan->value_sets.size() > 3) {
				std::cout << ", ...";
			}
			std::cout << "])\n";
			break;
		}
		case PlanNodeType::CREATE_TABLE: {
			auto* create_table = static_cast<const planner::CreateTablePlanNode*>(plan.get());
			std::cout << "CreateTable(table=" << create_table->table_name_ << ", columns=[";
			for (size_t i = 0; i < create_table->schema_.GetColumnCount(); i++) {
				std::cout << create_table->schema_.GetColumn(i).GetName();
				if (i < create_table->schema_.GetColumnCount() - 1) {
					std::cout << ", ";
				}
			}
			std::cout << "])\n";
			break;
		}
		case PlanNodeType::CREATE_DATABASE:
		case PlanNodeType::DROP_DATABASE:
		case PlanNodeType::USE_DATABASE:
		case PlanNodeType::SHOW_DATABASES: {
			auto* db_op = static_cast<const planner::DatabaseOpPlanNode*>(plan.get());
			std::cout << db_op->GetOperation() << "(";
			if (!db_op->database_name_.empty()) {
				std::cout << "database=" << db_op->database_name_;
			}
			std::cout << ")\n";
			break;
		}
		case PlanNodeType::SHOW_TABLES: {
			std::cout << "ShowTables()\n";
			break;
		}
		case PlanNodeType::DROP_TABLE: {
			auto* drop_table = static_cast<const planner::DropTablePlanNode*>(plan.get());
			std::cout << "DropTable(table=" << drop_table->table_name_ << ")\n";
			break;
		}
		default:
			std::cout << "UnknownPlan(type=" << static_cast<int>(plan->GetType()) << ")\n";
			break;
		}

		for (const auto& child : plan->GetChildren()) {
			PrintPlan(child, depth + 1);
		}
	}

	void PrintResultSet(const executor::ResultSet& rs) {
		if (rs.success_) {
			std::cout << std::endl;
			if (rs.data_) {
				const executor::TupleSet& tuple_set = *rs.data_;
				const Schema& schema = tuple_set.GetSchema();
				const auto& tuples = tuple_set.GetTuples();

				if (tuples.empty()) {
					std::cout << "No data found." << std::endl;
					return;
				}

				for (size_t i = 0; i < schema.GetColumnCount(); i++) {
					const Column& col = schema.GetColumn(i);
					std::cout << std::setw(18) << std::left << col.GetName();
					if (i < schema.GetColumnCount() - 1) {
						std::cout << " | ";
					}
				}
				std::cout << std::endl;

				for (size_t i = 0; i < schema.GetColumnCount(); i++) {
					std::cout << std::string(18, '-');
					if (i < schema.GetColumnCount() - 1) {
						std::cout << "-+-";
					}
				}
				std::cout << std::endl;

				for (const auto& tuple : tuples) {
					for (size_t i = 0; i < schema.GetColumnCount(); i++) {
						const Column& col = schema.GetColumn(i);

						const char* raw_value = tuple.GetValue(i, &schema);
						std::string value;

						if (raw_value != nullptr) {
							switch (col.GetType()) {
							case ColumnType::INT:
								value = std::to_string(*reinterpret_cast<const int*>(raw_value));
								break;
							case ColumnType::FLOAT:
								value = std::to_string(*reinterpret_cast<const float*>(raw_value));
								break;
							case ColumnType::CHAR:
								value = std::string(raw_value);
								break;
							default:
								value = "UNK";
								break;
							}
						} else {
							value = "NULL";
						}

						std::cout << std::setw(18) << std::left << value;
						if (i < schema.GetColumnCount() - 1) {
							std::cout << " | ";
						}
					}
					std::cout << std::endl;
				}
				std::cout << std::endl;
			}
			std::cout << rs.message_ << std::endl;
			std::cout << std::endl;
		} else {
			std::cout << "Failed to execute query: " << rs.message_ << std::endl;
		}
	}

	void PrintHelp() {
		std::cout << "\n\n  CREATE DATABASE <name>\n";
		std::cout << "  DROP DATABASE <name>\n";
		std::cout << "  USE <database>\n";
		std::cout << "  SHOW DATABASES\n\n";
		
		std::cout << "  CREATE TABLE <name> (\n";
		std::cout << "    <col> <type> [PRIMARY KEY],\n";
		std::cout << "    <col> <type>,\n";
		std::cout << "    ...\n";
		std::cout << "  )\n";
		std::cout << "  DROP TABLE <name>\n";
		std::cout << "  SHOW TABLES\n\n";
		
		std::cout << "  INSERT INTO <table> VALUES (<val1>, <val2>, ...);\n";
		std::cout << "  INSERT INTO <table> VALUES  \n";
		std::cout << "    (<val1>, <val2>, ...),\n";
		std::cout << "    (<val3>, <val4>, ...),\n";
		std::cout << "    ...;\n";
		std::cout << "  SELECT * FROM <table>     \n";
		std::cout << "  SELECT <col1>, <col2> FROM <table> \n\n";
		
		std::cout << "  EXAMPLE:\n";
		std::cout << "  CREATE DATABASE my_db;\n";
		std::cout << "  USE my_db;\n";
		std::cout << "  CREATE TABLE users (id INT, name CHAR, score FLOAT);\n";
		std::cout << "  INSERT INTO users VALUES (1, 'Alice', 95.5);\n";
		std::cout << "  INSERT INTO users VALUES (2, 'Bob', 87.2), (3, 'Charlie', 92.1);\n";
		std::cout << "  SELECT * FROM users;\n\n";		
	}

} // namespace utils
} // namespace venus