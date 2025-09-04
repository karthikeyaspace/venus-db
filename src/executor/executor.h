/**
 * Executor Framework for Venus DB - Simplified Architecture
 *
 * The Executor is responsible for taking a fully constructed query Plan
 * (produced by the Planner from a Bound AST) and executing it to produce results.
 *
 * Execution Model:
 *   iterator-based (Volcano-style) model.
 *   - Each Executor implements the standard lifecycle:
 *       1. Open()  — Prepare resources and initialize child executors.
 *       2. Next()  — Produce the next output tuple (pull model).
 *       3. Close() — Release resources and clean up.
 *   - Executors are connected in a pipeline where parent executors
 *     pull tuples from child executors on demand.
 *   - Streaming-oriented: tuples are processed as they are generated
 *     without requiring the entire dataset in memory
 *
 *
 * Supported Executor Types (aligned with PlanNodeTypes):
 *   - SeqScanExecutor       — Full table scan.
 *   - IndexScanExecutor     — Index-based scan (future extension).
 *   - ProjectionExecutor    — Select specific columns from input.
 *   - FilterExecutor        — Apply a WHERE condition to filter tuples.
 *   - JoinExecutor          — Combine rows from multiple inputs (nested loop join in v1).
 *   - AggregationExecutor   — GROUP BY and aggregate function computation.
 *   - SortExecutor          — ORDER BY implementation.
 *   - LimitExecutor         — Restrict output to N rows.
 *   - InsertExecutor        — Insert new rows into a table.
 *   - UpdateExecutor        — Update existing rows.
 *   - DeleteExecutor        — Delete rows from a table.
 *
 *   DDL and Utility Executors:
 *   - CreateDatabaseExecutor — CREATE DATABASE.
 *   - DropDatabaseExecutor   — DROP DATABASE.
 *   - UseDatabaseExecutor    — USE DATABASE.
 *   - CreateTableExecutor    — CREATE TABLE.
 *   - DropTableExecutor      — DROP TABLE.
 *   - CreateIndexExecutor    — CREATE INDEX.
 *   - DropIndexExecutor      — DROP INDEX.
 *   - ShowDatabasesExecutor  — SHOW DATABASES.
 *   - ShowTablesExecutor     — SHOW TABLES.
 *   - HelpExecutor           — HELP command.
 *   - ExitExecutor           — EXIT command.
 *   - ExecFileExecutor       — EXEC <file>.
 *
 * This is the all-mighty object handling entire execution process of a query
 *
 */

#pragma once

#include "buffer/buffer_pool.h"
#include "catalog/catalog.h"
#include "common/types.h"
#include "planner/planner.h"
#include "storage/tuple.h"
#include "table/table_heap.h"

#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

namespace venus {
namespace executor {

	class ExecutorContext;
	class ExecutionEngine;

	class AbstractExecutor {
	public:
		explicit AbstractExecutor(ExecutorContext* context)
		    : context_(context)
		    , table_heap_(nullptr) { }
		virtual ~AbstractExecutor() = default;

		virtual void Open() = 0;
		virtual bool Next(Tuple* tuple) = 0;
		virtual void Close() = 0;

		virtual const Schema& GetOutputSchema() const = 0;

	protected:
		ExecutorContext* context_;
		table::TableHeap* table_heap_;
	};

	class TupleSet {
	public:
		std::vector<Tuple> tuples_;
		Schema schema_;

		TupleSet(const Schema& schema)
		    : schema_(schema) { }

		void AddTuple(const Tuple& tuple) {
			tuples_.push_back(tuple);
		}

		size_t GetSize() const { return tuples_.size(); }
		bool IsEmpty() const { return tuples_.empty(); }

		const Schema& GetSchema() const { return schema_; }
		const std::vector<Tuple>& GetTuples() const { return tuples_; }
	};

	class ResultSet {
	public:
		bool success_;
		std::string message_;
		std::unique_ptr<TupleSet> data_;

		ResultSet(bool success, const std::string& message = "")
		    : success_(success)
		    , message_(message)
		    , data_(nullptr) { }

		static ResultSet Success(const std::string& message = "") {
			return ResultSet(true, message);
		}

		static ResultSet Failure(const std::string& message) {
			return ResultSet(false, message);
		}

		static ResultSet Data(std::unique_ptr<TupleSet> data) {
			ResultSet result(true);
			result.data_ = std::move(data);
			return result;
		}
	};

	class ExecutorContext {
	public:
		catalog::CatalogManager* catalog_manager_;
		buffer::BufferPoolManager* bpm_;

		ExecutorContext(catalog::CatalogManager* catalog, buffer::BufferPoolManager* bpm)
		    : catalog_manager_(catalog)
		    , bpm_(bpm) { }
	};

	class Executor {
	public:
		Executor() = default;

		// volcano driver
		ResultSet ExecutePlan(const planner::PlanNode* plan);

		void SetContext(buffer::BufferPoolManager* bpm, catalog::CatalogManager* catalog) {
			context_ = new ExecutorContext(catalog, bpm);
		}

		void PrintResultSet(const ResultSet& result_set);

	private:
		ExecutorContext* context_;

		std::unique_ptr<AbstractExecutor> BuildExecutorTree(const planner::PlanNode* plan);
	};

} // namespace executor
} // namespace venus
