// src/common/utils.h

#pragma once

#include "executor/executor.h"
#include "planner/planner.h"

#include <iomanip>
#include <iostream>

namespace venus {
namespace utils {

	void PrintPlan(const std::unique_ptr<planner::PlanNode>& plan, int depth = 0);
	void PrintResultSet(const executor::ResultSet& rs);
	void PrintHelp();

} // namespace utils
} // namespace venus