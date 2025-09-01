// /src/executor/executor.cpp

#include <iostream>
#include <sstream>

#include "engine/execution_engine.h"

using namespace venus::parser;
using namespace venus::binder;
using namespace venus::planner;
using namespace venus::executor;

namespace venus {
namespace executor {

	bool ExecutionEngine::Execute(const std::string& query) {
		try {
			auto ast = parser_.Parse(query);

			if (ast->type == ASTNodeType::USE_DATABASE) {
				std::string db_name = ast->value;
				init_callback_(db_name);
				std::cout << "Database initialized: " << db_name << std::endl;
				return true;
			}

			if (ast->type == ASTNodeType::EXIT) {
				stop_db_callback_();
				return false;
			}

			auto bounded_ast = binder_.Bind(std::move(ast));

			auto plan = planner_.Plan(std::move(bounded_ast));
			planner_.PrintPlan(plan);

			auto result_set = executor_.ExecutePlan(plan.get());
			executor_.PrintResultSet(result_set);

		} catch (const std::exception& e) {
			std::cout << e.what() << std::endl;
		}
		return true;
	}

	void ExecutionEngine::StartRepl() {
		std::cout << "===== Venus DB =====" << std::endl;

		std::string query;
		while (true) {
			std::cout << std::endl;
			std::cout << "venus> ";
			std::getline(std::cin, query);

			std::cout << std::endl;

			if (query.empty())
				continue;

			if (!this->Execute(query)) {
				break;
			}
		}
	}
} // namespace executor
} // namespace venus
