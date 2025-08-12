/**
 * Executor Framework for Venus DB
 *
 * The Executor is responsible for taking a fully constructed query Plan
 * (produced by the Planner from a Bound AST) and executing it to produce results.
 * 
 * Execution Model:
 *   - Venus DB uses an iterator-based (Volcano-style) model.
 *   - Each Executor implements the standard lifecycle:
 *       1. Open()  — Prepare resources and initialize child executors.
 *       2. Next()  — Produce the next output tuple (pull model).
 *       3. Close() — Release resources and clean up.
 *   - Executors are connected in a pipeline where parent executors
 *     pull tuples from child executors on demand.
 *   - Streaming-oriented: tuples are processed as they are generated
 *     without requiring the entire dataset in memory.
 * 
 * Relationship to Planner:
 *   - The Planner generates a Plan tree composed of PlanNode objects.
 *   - For each PlanNodeType, there is a corresponding Executor subclass.
 *   - Executors use the metadata and parameters from their PlanNode
 *     to perform the specified operation.
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
 * Notes:
 *   - Executors are stateful: internal cursors/iterators track progress.
 *   - Can be extended for push-based execution or parallel operators.
 *   - Designed to support statistics collection for future optimizer integration.
 *   - All executors should be unit-testable in isolation.
 *   - Every executor is inherited from a common base class.
 */
