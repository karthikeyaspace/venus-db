## Roadmap for Venus DB v3.0 and further

- SQL Joins
  - Nested Loop Join
  - Index Nested Loop Join
  - Sort-Merge Join
- Aggregate functions
- More data types - varchar, boolean, blob, datetime
- Hash indexes
- Better parser (ANTLR or Flex/Bison) using grammar files
- Transactions (serial transactions)
- Recovery (WAL + checkpointing) using ARIES
- Query optimization (heuristic and cost-based)
- User management and authentication
- Network layer - HTTP connection, Req queue, Thread pool
- EXEC file