// /src/executor/executor.cpp

#include "executor/executor.h"

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

		return ResultSet::Success("Query executed successfully. " + std::to_string(num_rows) + " rows affected.");
	}

	std::unique_ptr<AbstractExecutor> Executor::BuildExecutorTree(const planner::PlanNode* plan) {
	return nullptr;
	}

}
}