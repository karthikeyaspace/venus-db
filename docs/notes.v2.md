## Venus DB v2 roadmap

- Implement strict validations for insert, select (where, etc)..
- Better test suits
- more queries - bulk insert, where, limit, order by, joins
- Better constraints handling - unique, not null, check
- Auto increment primary keys
- Need better CHAR data type handling
- further data validations for insert (primary key not negative, not repeated, checking input value types based on schema from catalog, char length etc)
- Better error handling for parser and binder
- Migrate REPL to Network manager, have better cli -> history, better everything

- Indexing - B+ Tree
- Buffer pool manager - LRU-K, tracking dirty pages(pin/unpin)
- Better deletion for Tuple/Page - reclaim functionality
- Better error handling - custom exceptions

- Network layer - HTTP connection, Req queue, Thread pool
- EXEC file

- WAL
- Performance stats