Venus-DB

A simple SQL database system to achieve a goal of understanding how the internal workings of a database system work.
This is inspired by 2 people: 1. Sir Arpit Bhayani, 2. Prof. Andy Pavlo


## Some ground work:
- C++, WSL implementation - why, i dont know!
- SQL case sensitive - all caps
- N-ary Storage Model - Row storage
- Only 1 primary key - for index, no foreign keys
- Single/simple joins
- Make a simple parser only for supporting commands - extensible
- Supporting data types: INT, FLOAT, CHAR(N)
- No Transactions - if touched, only serial txns
- No Views, concurrency, ACID compliance, db auth


## Components
- Parser, Query Executor, Query Optimizer(Heuristic) - Interpreter
- API 
- Index manager - Access methods(B+ Tree) - Clustered only
- Buffer manager - Disk scheduling, LRU-K
- Storage manager - File system, data structures

- Transaction manager - Serial transactions 
- Catalog manager - Metadata, system tables


## Page layout
+--------------------+  ← offset 0
| Page Header        |  (e.g., 32B)
+--------------------+
| Slot Directory     |  (grows downward from header end)
+--------------------+
| Free Space         |
+--------------------+
| Tuples / Records   |  (grows upward from page end)
+--------------------+  ← offset 4096 (PAGE_SIZE)


Record ID (page_id, slot_id)
          ↓
+────────────────────+
│  Tuple Header      │  (e.g., Null Bitmap, Size of Tuple)
+────────────────────+
│  Tuple Body        │  (Column Values)
+────────────────────+


Header - page id, page type, num slots, free space ptr, dirty bit
Disk -> DB File -> Page -> Tuple
Page in buffer pool is called frame


## Tuple format
- Record ID
  - page_id, slot_id
- Tuple Header
  - null bitmap, size of tuple
- Tuple Body
  - column values (fixed size for INT, FLOAT, CHAR(N))


## Architecture
┌────────────────────────────┐
│        SQL Shell           │  ← User input (REPL or EXEC file)
└────────────┬───────────────┘
             │
             ▼
┌────────────────────────────┐
│        SQL Parser          │  ← Simple grammar
└────────────┬───────────────┘
             │
             ▼
┌────────────────────────────┐
│     Query Planner / Tree   │  ← Logical plan builder
└────────────┬───────────────┘
             │
             ▼
┌────────────────────────────┐
│      Query Executor        │  ← Executes plan nodes (scan, filter, join)
└────┬────────────┬──────────┘
     │            │
     ▼            ▼
┌──────────┐   ┌────────────────────┐
│ Catalog  │   │  Access Methods    │  ← B+ Tree Index, Seq scan
│ Manager  │   │(Index/Table Scan)  │
└────┬─────┘   └──────────┬─────────┘
     │                    ▼
     ▼             ┌──────────────┐
┌──────────────┐   │ Buffer Pool  │ ← Handles in-memory page caching
│ System Tables│   └─────┬────────┘
└────┬─────────┘         │
     ▼                   ▼
┌──────────────────────────────────┐
│        Storage Manager           │  ← Reads/writes pages from disk
│  (File + Page I/O abstraction)   │
└──────────────────────────────────┘


## References
- https://chatgpt.com/c/6854042b-becc-800e-876f-a66f8cf67cb7

- https://www.youtube.com/playlist?list=PLSE8ODhjZXjYDBpQnSymaectKjxCy6BYq
- https://www.youtube.com/playlist?list=PLsdq-3Z1EPT2C-Da7Jscr7NptGcIZgQ2l
- https://www.youtube.com/playlist?list=PLsdq-3Z1EPT1cL2FsCkxLlw-VLqK0GU-F

- https://github.com/nrthyrk/minidb?tab=readme-ov-file
- https://github.com/kw7oe/mini-db?tab=readme-ov-file
- https://www.databass.dev/
- https://github.com/JyotinderSingh/dropdb
- https://github.com/Sahilb315/AtomixDB


## Ownership

NetworkEngine
DatabaseEngine
├── DiskManager
├── BufferPoolManager  
├── CatalogManager
├── IndexManager
└── ExecutionEngine
    ├── Parser
    ├── Planner
    └── Executor


## Various std error
- invalid_argument
- out_of_range
- runtime_error
- logic_error
- system_error
- overflow_error


### Queries to be supported by Venus DB

For sql grammer - https://forcedotcom.github.io/phoenix/

1. SHOW DATABASES
   DROP <db_name>
   USE <db_name>
   CREATE DATABASE <db_name>
   CREATE DATABASE IF NOT EXISTS <db_name>

2. CREATE TABLE <table_name> ( 
    col_name col_type primary_key,
    col_name col_type,
    col_name col_type,
   )

   DROP TABLE <table_name>
   SHOW TABLE;

3. SELECT * FROM <table_name>
   SELECT col_name,.. FROM <table_name>
   SELECT col_name,.. FROM <table_name> WHERE condition;
   SELECT col_name,.. FROM <table_name> JOIN <table_name2> ON condition;
   SELECT col_name,.. FROM <table_name> WHERE condition ORDER BY col_name;
   SELECT col_name,.. FROM <table_name> WHERE condition LIMIT n;
   SELECT col_name,.. FROM <table_name> WHERE condition GROUP BY col_name;

   INSERT INTO <table_name> VALUES (val1, val2,..);
   INSERT INTO <table_name> (col1, col2,..) VALUES (val1, val2,..), (val3, val4,..);

4. UPDATE <table_name> SET col1 = val1, col2 = val2 WHERE condition;
   DELETE FROM <table_name> WHERE condition;

5. CREATE INDEX <index_name> ON <table_name> (col_name);
   DROP INDEX <index_name>;

6. HELP
   EXIT
   EXEC <file_name>



## Roadmap
- more queries - where, limit, order by, joins
- Better constraints handling - unique, not null, check
- Auto increment primary keys
- Need better CHAR data type handling
- further data validations for insert (primary key not negative, not repeated, checking input value types based on schema from catalog, char length etc)
- Better error handling for parser and binder
- Migrate REPL to Network manager, have better cli -> history, better everything

- Add indexes - B+ Tree
- Buffer pool manager - LRU-K, tracking dirty pages(pin/unpin)
- Better deletion for Tuple/Page - reclaim functionality
- Better error handling - custom exceptions

- Network layer - HTTP connection, Req queue, Thread pool
- EXEC file

- WAL
- Performance stats