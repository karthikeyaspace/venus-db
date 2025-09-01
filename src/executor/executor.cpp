// /src/executor/executor.cpp

#include "executor/executor.h"
#include "executor/operators.h"

namespace venus {
namespace executor {

	ResultSet Executor::ExecutePlan(const planner::PlanNode* plan) {
		if (plan == nullptr) {
			return ResultSet::Failure("Executor error: Plan node is null");
		}

		std::unique_ptr<AbstractExecutor> root;

		try {
			root = BuildExecutorTree(plan);
		} catch (const std::exception& e) {
			return ResultSet::Failure(std::string("Executor build failed: ") + e.what());
		}

		if (root == nullptr) {
			return ResultSet::Failure("Executor error: Failed to build executor tree");
		}

		try {
			root->Open();
		} catch (const std::exception& e) {
			return ResultSet::Failure(std::string("Executor open failed: ") + e.what());
		}

		const Schema& out_schema = root->GetOutputSchema();
		const bool has_output = out_schema.GetColumnCount() > 0;

		std::unique_ptr<TupleSet> result_set = nullptr;
		size_t num_rows = 0;

		try {
			if (has_output) {
				result_set = std::make_unique<TupleSet>(out_schema);
				Tuple t;
				while (root->Next(&t)) {
					result_set->AddTuple(t);
					num_rows++;
				}
			} else {
				Tuple t;
				while (root->Next(&t)) {
					num_rows++;
				}
			}
		} catch (const std::exception& e) {
			root->Close();
			return ResultSet::Failure(std::string("Executor Next() failed: ") + e.what());
		}

		try {
			root->Close();
		} catch (const std::exception& e) {
			return ResultSet::Failure(std::string("Executor Close() failed: ") + e.what());
		}

		if (has_output) {
			auto rs = ResultSet::Data(std::move(result_set));
			rs.message_ = std::to_string(num_rows) + " rows returned.";
			return rs;
		}

		return ResultSet::Success(std::to_string(num_rows) + " rows affected.");
	}

	std::unique_ptr<AbstractExecutor> Executor::BuildExecutorTree(const planner::PlanNode* plan) {
		if (!plan)
			return nullptr;

		switch (plan->type_) {
		case PlanNodeType::SEQ_SCAN: {
			auto p = static_cast<const planner::SeqScanPlanNode*>(plan);
			return std::make_unique<SeqScanExecutor>(context_, p);
		}
		case PlanNodeType::PROJECTION: {
			auto p = static_cast<const planner::ProjectionPlanNode*>(plan);
			if (p->children_.size() != 1) {
				throw std::runtime_error("Projection expects exactly one child");
			}
			auto child = BuildExecutorTree(p->children_[0].get());
			return std::make_unique<ProjectionExecutor>(context_, p, std::move(child));
		}

		case PlanNodeType::INSERT: {
			auto p = static_cast<const planner::InsertPlanNode*>(plan);
			return std::make_unique<InsertExecutor>(context_, p);
		}

		case PlanNodeType::CREATE_TABLE: {
			auto p = static_cast<const planner::CreateTablePlanNode*>(plan);
			return std::make_unique<CreateTableExecutor>(context_, p);
		}

		case PlanNodeType::DROP_TABLE: {
			auto p = static_cast<const planner::DropTablePlanNode*>(plan);
			return std::make_unique<DropTableExecutor>(context_, p);
		}

		case PlanNodeType::CREATE_DATABASE:
		case PlanNodeType::DROP_DATABASE:
		case PlanNodeType::USE_DATABASE:
		case PlanNodeType::SHOW_DATABASES: {
			auto p = static_cast<const planner::DatabaseOpPlanNode*>(plan);
			return std::make_unique<DatabaseOpExecutor>(context_, p);
		}

		case PlanNodeType::SHOW_TABLES: {
			auto p = static_cast<const planner::ShowTablesPlanNode*>(plan);
			return std::make_unique<ShowTablesExecutor>(context_, p);
		}

		default:
			throw std::runtime_error("Executor: Unsupported plan node type");
		}
	}

}
}