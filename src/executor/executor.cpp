// /src/executor/executor.cpp

#include "executor/executor.h"
#include "parser/parser.h"

#include <iostream>
#include <sstream>

#include "table/table_heap.h"

using namespace venus::parser;

namespace venus {
namespace executor {

	ExecutionEngine::~ExecutionEngine() {
		if (stop_db_callback_) {
			stop_db_callback_();
		}
	}

	void ExecutionEngine::execute(const std::string& query) {
		try {
			auto ast = parser_.parse(query);

			ast->print();

		} catch (const std::exception& e) {
			std::cout << "Error: " << e.what() << std::endl;
		}
	}

	void ExecutionEngine::StartRepl() {
		std::cout << "===== Venus DB =====" << std::endl;

		std::string query;
		while (true) {
			std::cout << std::endl;
			std::cout << "venus> ";
			std::getline(std::cin, query);

			if (query == "exit" || query == "quit") {
				stop_db_callback_();
				break;
			}

			if (query.empty())
				continue;

			execute(query);
		}
	}
} // namespace executor
} // namespace venus
