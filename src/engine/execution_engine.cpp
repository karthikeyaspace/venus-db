// /src/executor/executor.cpp

#include <iostream>
#include <sstream>

#include "common/utils.h"
#include "engine/execution_engine.h"

using namespace venus::parser;
using namespace venus::binder;
using namespace venus::planner;
using namespace venus::executor;

namespace venus {
namespace engine {

	executor::ResultSet ExecutionEngine::Execute(const std::string& query) {
		try {
			auto ast = parser_.Parse(query);

			if (ast->type == ASTNodeType::USE_DATABASE) {
				std::string db_name = ast->value;
				init_callback_(db_name);
				return executor::ResultSet::Success("Database initialized: " + db_name);
			}

			if (ast->type == ASTNodeType::EXIT) {
				stop_db_callback_();
				return executor::ResultSet::Success("");
			}

			auto bounded_ast = binder_.Bind(std::move(ast));

			auto plan = planner_.Plan(std::move(bounded_ast));
			// utils::PrintPlan(plan);

			auto result_set = executor_.ExecutePlan(plan.get());
			// Remove duplicate print - network layer will handle output
			return result_set;

		} catch (const std::exception& e) {
			std::cout << e.what() << std::endl;
			return executor::ResultSet::Failure(e.what());
		}
	}

} // namespace engine
} // namespace venus
