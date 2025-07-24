// /src/executor/executor.cpp

#include "executor/executor.h"
#include "table/table_heap.h"
#include <iostream>
#include <sstream>

namespace venus {
namespace executor {
	void ExecutionEngine::ExecuteQuery(const std::string& query) {
        // Query path
        // parser -> planner -> optimizer -> executor
        std::cout << "Executing query: " << query << std::endl;
	}
}
}
