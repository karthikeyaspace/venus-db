// /src/executor/executor.h

/**
 * Executor Framework for Venus DB
 *
 * The executor system is responsible for implementing query execution operators
 * in a modular, pipeline-based architecture. Each executor implements a specific
 * database operation and can be chained together to form complex query plans.
 *
 * Current Executors:
 * 1. Scan - Handles table scanning with WHERE clause filters
 *    - Supports predicate pushdown for efficient filtering
 *    - Does NOT handle GROUP BY, HAVING, or aggregation operations
 *    - Primarily focused on row-level filtering during table access
 *
 * Planned Executors:
 * 2. Filter - Additional WHERE clause filtering for complex predicates
 * 3. Sort - ORDER BY operations and sorting support
 * 4. Aggregate - GROUP BY, HAVING, and aggregation functions (SUM, COUNT, etc.)
 * 5. Join - Various join algorithms (nested loop, hash join, sort-merge)
 * 6. Limit - LIMIT and OFFSET operations for result set pagination
 * 7. Insert - INSERT statement execution
 * 8. Update - UPDATE statement execution
 * 9. Delete - DELETE statement execution
 *
 * Architecture:
 * - Iterator-based execution model (Volcano-style)
 * - Each executor implements Open(), Next(), Close() interface
 * - Supports both pull-based and push-based execution
 * - Memory-efficient streaming processing
 * - Statistics collection for query optimization
 *
 * Threading Model:
 * - Thread-safe executor implementations
 * - Support for parallel execution where applicable
 * - Resource management and cleanup
 *
 * ExecutionEngine owns the REPL interface and coordinates query execution
 * Will integrate with parser and planner once they are implemented
 */

#pragma once

#include "buffer/buffer_pool.h"
#include "catalog/catalog.h"
#include "catalog/schema.h"
#include "common/config.h"
#include "table/table_heap.h"
#include <iostream>
#include <string>
#include <vector>

using namespace venus::catalog;
using namespace venus::buffer;
using namespace venus::table;

namespace venus {
namespace executor {
	class ExecutionEngine {
	public:
		ExecutionEngine(BufferPoolManager* bpm, CatalogManager* catalog)
		    : bpm_(bpm)
		    , catalog_(catalog) { }

		~ExecutionEngine() = default;

		void ExecuteQuery(const std::string& query);

	private:
		BufferPoolManager* bpm_;
		CatalogManager* catalog_;
	};
}
}
