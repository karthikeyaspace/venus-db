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
 * 6. Projection - SELECT clause column selection and expression evaluation
 * 7. Limit - LIMIT and OFFSET operations for result set pagination
 * 8. Insert - INSERT statement execution
 * 9. Update - UPDATE statement execution
 * 10. Delete - DELETE statement execution
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
 */