// /src/planner/planner.h

/**
 * Planner for Venus DB
 *
 * Planner takes in Bounded AST emitted by Binder, and generates a Plan
 *
 * The generated Plan is a tree-like structure that represents the
 * execution strategy for the query.
 *
 * Generally Plan is of 2 types - Logical and Physical Plan
 *
 * Logical Plan: Represents the high-level query structure and semantics.
 * Physical Plan: Represents the low-level execution strategy, including
 *                operator implementations and execution order.
 *
 * Plan shall be relevent to relational model
 *
 * For Venus DB, We will consider Logical and Physical plan as a single entity `Plan`
 * The generator Plan goes through executor as Iterator model (Open, Close, Next)
 *
 *
 *
 * Relational Algebra Operators:
 *   SEQ_SCAN       — Full table scan (default SELECT input)
 *   INDEX_SCAN     — Index-based scan (future optimization)
 *   PROJECTION     — Column selection (SELECT col1, col2)
 *   FILTER         — Row filtering (WHERE clauses)
 *   JOIN           — Row combination from multiple tables (JOIN ... ON ...)
 *   AGGREGATION    — Grouping and aggregation (GROUP BY, aggregates)
 *   SORT           — Ordering rows (ORDER BY)
 *   LIMIT          — Restricting row count (LIMIT n)
 *   INSERT         — Insert rows into a table
 *   UPDATE         — Update existing rows
 *   DELETE         — Delete rows
 *
 * Data Definition (DDL):
 *   CREATE_DATABASE — Create a new database
 *   DROP_DATABASE   — Remove an existing database
 *   USE_DATABASE    — Switch current database
 *   CREATE_TABLE    — Create a new table
 *   DROP_TABLE      — Remove a table
 *   CREATE_INDEX    — Create an index on table columns
 *   DROP_INDEX      — Remove an index
 *
 * Utility / Meta Commands:
 *   SHOW_DATABASES — List available databases
 *   SHOW_TABLES    — List tables in current database
 *   HELP           — Display help message
 *   EXIT           — Exit the shell
 *   EXEC_FILE      — Execute SQL from a file
 *
 *
 * eg
 * SELECT name FROM employees WHERE age > 30 ORDER BY name LIMIT 10
 *   → LIMIT
 *        → SORT(columns=[name])
 *             → PROJECTION(columns=[name])
 *                  → FILTER(condition="age > 30")
 *                       → SEQ_SCAN(table=employees)
 *
 * INSERT INTO employees (name, age) VALUES ('Alice', 30)
 *   → INSERT(table=employees, columns=[name, age], values=[["Alice", "30"]])
 *
 * UPDATE employees SET age=31 WHERE name='Alice'
 *   → UPDATE(table=employees, set_clauses={age=31}, condition="name='Alice'")
 */