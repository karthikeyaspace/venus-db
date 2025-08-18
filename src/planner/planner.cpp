// /src/planner/planner.cpp

#include "planner.h"
#include <iostream>

namespace venus {
namespace planner {

	std::unique_ptr<PlanNode> Planner::Plan(std::unique_ptr<BoundASTNode> bound_ast) {
		if (!bound_ast) {
			throw std::runtime_error("Planner error: Null bound AST provided");
		}

		switch (bound_ast->GetType()) {
		case ASTNodeType::SELECT:
			return CreateSelectPlan(static_cast<BoundSelectNode*>(bound_ast.get()));

		case ASTNodeType::INSERT:
			return CreateInsertPlan(static_cast<BoundInsertNode*>(bound_ast.get()));

		case ASTNodeType::CREATE_TABLE:
			return CreateCreateTablePlan(static_cast<BoundCreateTableNode*>(bound_ast.get()));

		case ASTNodeType::CREATE_DATABASE:
		case ASTNodeType::DROP_DATABASE:
		case ASTNodeType::USE_DATABASE:
		case ASTNodeType::SHOW_DATABASES:
			return CreateDatabaseOpPlan(static_cast<BoundDatabaseNode*>(bound_ast.get()));

		case ASTNodeType::SHOW_TABLES:
			return CreateShowTablesPlan(static_cast<BoundShowTablesNode*>(bound_ast.get()));

		case ASTNodeType::DROP_TABLE:
			return CreateDropTablePlan(static_cast<BoundDropTableNode*>(bound_ast.get()));

		default:
			throw std::runtime_error("Planner error: Unsupported AST node type");
		}
	}

	std::unique_ptr<PlanNode> Planner::CreateSelectPlan(BoundSelectNode* select_node) {
		auto scan_plan = std::make_unique<SeqScanPlanNode>(
		    select_node->table_ref.table_id,
		    select_node->table_ref.table_name);

		std::vector<column_id_t> column_ids;
		std::vector<std::string> column_names;

		for (const auto& proj : select_node->projections) {
			column_ids.push_back(proj.col_id);
			if (proj.column_entry_) {
				column_names.push_back(proj.column_entry_->GetName());
			} else {
				column_names.push_back("col_" + std::to_string(proj.col_id));
			}
		}

		auto projection_plan = std::make_unique<ProjectionPlanNode>(
		    column_ids,
		    column_names);

		projection_plan->AddChild(std::move(scan_plan));

		return std::move(projection_plan);
	}

	std::unique_ptr<PlanNode> Planner::CreateInsertPlan(BoundInsertNode* insert_node) {
		if (!insert_node) {
			throw std::runtime_error("Planner error: Null insert node");
		}

		std::vector<std::string> values;
		for (const auto& val : insert_node->values) {
			values.push_back(val.value);
		}

		return std::make_unique<InsertPlanNode>(
		    insert_node->table_ref.table_id,
		    insert_node->table_ref.table_name,
		    values);
	}

	std::unique_ptr<PlanNode> Planner::CreateCreateTablePlan(BoundCreateTableNode* create_table_node) {
		if (!create_table_node) {
			throw std::runtime_error("Planner error: Null create table node");
		}

		Schema schema;
		for (size_t i = 0; i < create_table_node->columns.size(); i++) {
			const auto& col = create_table_node->columns[i];
			schema.AddColumn(col.GetName(), col.GetType(), col.IsPrimary(), i);
		}

		return std::make_unique<CreateTablePlanNode>(
		    create_table_node->table_name,
		    schema);
	}

	std::unique_ptr<PlanNode> Planner::CreateDatabaseOpPlan(BoundDatabaseNode* database_node) {
		if (!database_node) {
			throw std::runtime_error("Planner error: Null database node");
		}

		PlanNodeType plan_type;
		switch (database_node->GetType()) {
		case ASTNodeType::CREATE_DATABASE:
			plan_type = PlanNodeType::CREATE_DATABASE;
			break;
		case ASTNodeType::DROP_DATABASE:
			plan_type = PlanNodeType::DROP_DATABASE;
			break;
		case ASTNodeType::USE_DATABASE:
			plan_type = PlanNodeType::USE_DATABASE;
			break;
		case ASTNodeType::SHOW_DATABASES:
			plan_type = PlanNodeType::SHOW_DATABASES;
			break;
		default:
			throw std::runtime_error("Planner error: Invalid database operation type");
		}

		return std::make_unique<DatabaseOpPlanNode>(plan_type, database_node->database_name);
	}

	std::unique_ptr<PlanNode> Planner::CreateShowTablesPlan(BoundShowTablesNode* show_tables_node) {
		if (!show_tables_node) {
			throw std::runtime_error("Planner error: Null show tables node");
		}

		return std::make_unique<DatabaseOpPlanNode>(PlanNodeType::SHOW_TABLES);
	}

	std::unique_ptr<PlanNode> Planner::CreateDropTablePlan(BoundDropTableNode* drop_table_node) {
		if (!drop_table_node) {
			throw std::runtime_error("Planner error: Null drop table node");
		}

		return std::make_unique<DatabaseOpPlanNode>(PlanNodeType::DROP_TABLE, drop_table_node->table_name);
	}

} // namespace planner
} // namespace venus