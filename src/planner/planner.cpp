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

		case ASTNodeType::SELECT: {
			auto select_node = static_cast<BoundSelectNode*>(bound_ast.get());
			if (!select_node) {
				throw std::runtime_error("Planner error: Null select node");
			}

			auto scan_plan = std::make_unique<SeqScanPlanNode>(select_node->table_ref);

			auto projection_plan = std::make_unique<ProjectionPlanNode>(select_node->projections);

			projection_plan->AddChild(std::move(scan_plan));

			// where clause and limit will come under filter -> TODO
			// can also add joins, aggregations, sorting

			return std::move(projection_plan);
		}

		case ASTNodeType::INSERT: {
			auto insert_node = static_cast<BoundInsertNode*>(bound_ast.get());
			if (!insert_node) {
				throw std::runtime_error("Planner error: Null insert node");
			}

			return std::make_unique<InsertPlanNode>(
			    insert_node->table_ref,
			    insert_node->target_cols,
			    insert_node->values);
		}

		case ASTNodeType::INSERT_BULK: {
			auto bulk_insert_node = static_cast<BoundBulkInsertNode*>(bound_ast.get());
			if (!bulk_insert_node) {
				throw std::runtime_error("Planner error: Null bulk insert node");
			}

			return std::make_unique<BulkInsertPlanNode>(
			    bulk_insert_node->table_ref,
			    bulk_insert_node->target_cols,
			    bulk_insert_node->values);
		}

		case ASTNodeType::CREATE_TABLE: {
			auto create_table_node = static_cast<BoundCreateTableNode*>(bound_ast.get());
			if (!create_table_node) {
				throw std::runtime_error("Planner error: Null create table node");
			}

			return std::make_unique<CreateTablePlanNode>(
			    create_table_node->table_name,
			    create_table_node->schema);
		}

		case ASTNodeType::CREATE_DATABASE:
		case ASTNodeType::DROP_DATABASE:
		case ASTNodeType::USE_DATABASE:
		case ASTNodeType::SHOW_DATABASES: {
			auto database_node = static_cast<BoundDatabaseNode*>(bound_ast.get());
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

		case ASTNodeType::SHOW_TABLES: {
			auto show_tables_node = static_cast<BoundShowTablesNode*>(bound_ast.get());

			if (!show_tables_node) {
				throw std::runtime_error("Planner error: Null show tables node");
			}

			return std::make_unique<ShowTablesPlanNode>();
		}

		case ASTNodeType::DROP_TABLE: {
			auto drop_table_node = static_cast<BoundDropTableNode*>(bound_ast.get());

			if (!drop_table_node) {
				throw std::runtime_error("Planner error: Null drop table node");
			}

			return std::make_unique<DropTablePlanNode>(drop_table_node->table_name);
		}

		default:
			throw std::runtime_error("Planner error: Unsupported AST node type");
		}
	}

} // namespace planner
} // namespace venus