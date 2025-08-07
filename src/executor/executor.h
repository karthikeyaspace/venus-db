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
 * ExecutionEngine owns the REPL interface and coordinates query execution among parser, planner, and various executors
 * Will integrate with parser and planner once they are implemented
 *
 */

#pragma once

#include "buffer/buffer_pool.h"
#include "catalog/catalog.h"
#include "catalog/schema.h"
#include "common/config.h"
#include "table/table_heap.h"
#include "parser/parser.h"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

using namespace venus::catalog;
using namespace venus::buffer;
using namespace venus::table;
using namespace venus::parser;

namespace venus {
namespace executor {
	class ExecutionEngine {
	public:
		~ExecutionEngine();

		void InitializeCallback(std::function<void(const std::string&)> cb) {
			init_callback_ = std::move(cb);
		}

		void SetLocalContext(BufferPoolManager* bpm,
		    CatalogManager* catalog) {
			bpm_ = bpm;
			catalog_ = catalog;
		}

		void SetStopDBCallback(std::function<void()> cb) {
			stop_db_callback_ = std::move(cb);
		}

		void execute(const std::string& query);
		void StartRepl();

	private:
		BufferPoolManager* bpm_ = nullptr;
		CatalogManager* catalog_ = nullptr;

		Parser parser_;

		std::function<void(const std::string&)> init_callback_;
		std::function<void()> stop_db_callback_;
	};
} // namespace executor
} // namespace venus
