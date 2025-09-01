// /src/executor/executor.h

/**
 * Execution Engine
 *
 * Query life cycle
 *
 * SQL Query ---> Lexer ---> Parser ---> Binder ---> Planner ---> Optimizer ---> Executor
 *
 * ExecutionEngine owns the REPL interface and coordinates query execution among parser, planner, and various executors
 * Will integrate with parser and planner once they are implemented
 *
 * Ownership
 * ExecutionEngine
 * ├── Parser (contains Lexer)
 * ├── Binder (resolves identifiers, types, etc.)
 * ├── Planner (query optimization)
 * ├── Optimizer (query optimization)
 * └── Executor (query execution)
 *   ├── QueryExecutor (handles query execution)
 *   └── ResultSet (stores query results)
 *
 */

#pragma once

#include "binder/binder.h"
#include "buffer/buffer_pool.h"
#include "catalog/catalog.h"
#include "catalog/schema.h"
#include "common/config.h"
#include "parser/parser.h"
#include "planner/planner.h"
#include "executor/executor.h"
#include "storage/tuple.h"
#include "table/table_heap.h"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace venus {
namespace executor {
	class ExecutionEngine {
	public:
		~ExecutionEngine() = default;

		void InitializeCallback(std::function<void(const std::string&)> cb) {
			init_callback_ = std::move(cb);
		}

		void SetLocalContext(buffer::BufferPoolManager* bpm,
		    catalog::CatalogManager* catalog) {
			bpm_ = bpm;
			catalog_ = catalog;
			binder_.SetContext(catalog);
			executor_.SetContext(bpm, catalog);
		}

		void SetStopDBCallback(std::function<void()> cb) {
			stop_db_callback_ = std::move(cb);
		}

		bool Execute(const std::string& query);
		void StartRepl();

	private:
		buffer::BufferPoolManager* bpm_ = nullptr;
		catalog::CatalogManager* catalog_ = nullptr;

		parser::Parser parser_;
		binder::Binder binder_;
		planner::Planner planner_;
		executor::Executor executor_;

		std::function<void(const std::string&)> init_callback_;
		std::function<void()> stop_db_callback_;
	};
} // namespace executor
} // namespace venus
