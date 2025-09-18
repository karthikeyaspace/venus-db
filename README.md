<img width="1125" height="295" alt="VENUS DB" src="/docs/banner.png" />

A simple SQL database system built from scratch in C++ to understand the internal workings of database systems. This project is inspired by Sir Arpit Bhayani and Prof. Andy Pavlo's educational content on database internals.

This is the Version v1.0 of Venus DB, more are yet to come.

### Currently Implemented
- **Storage Engine**: Page-based storage with tuple management
- **Buffer Pool Manager**: In-memory page caching with LRU-K replacement
- **Catalog System**: Metadata management with system tables
- **Query Executor**: Volcano-style iterator execution model
- **SQL Parser**: Basic SQL command parsing (CREATE, INSERT, SELECT, etc.)
- **Basic SQL Operations**:
  - `CREATE DATABASE`, `USE DATABASE`, `DROP DATABASE`, `SHOW DATABASES`
  - `CREATE TABLE`, `DROP TABLE`, `SHOW TABLES`
  - `INSERT INTO` with values
  - `SELECT *` and `SELECT columns` with projection
  - Basic table scanning (Sequential Scan)

### Architecture

![Venus DB Architecture](/docs/venus.png)

*Venus DB follows a layered architecture with clear separation between parsing, planning, execution, and storage layers.*

## Technical Details

### Data Types Supported
- `INT` - 32-bit integers
- `FLOAT` - 32-bit floating point
- `CHAR` - Fixed-length strings (max 32 chars)

### Storage Model
- **Row-oriented storage** (N-ary Storage Model)
- **Page-based** storage (4KB pages)
- **Slotted page layout** with slot directory
- **Record ID** addressing (page_id, slot_id)

## Getting Started

### Prerequisites
- C++17 or later
- CMake 3.10+
- Linux/WSL environment (currently)

### Building
```bash
git clone https://github.com/karthikeyaspace/venus-db.git
cd venus-db
make run
```

### Testing
```bash
make test
```

make sure this is performed in linux environment (Ubuntu is preferred)

### Example Usage
```sql
venus> CREATE DATABASE mydb;
venus> USE mydb;
venus> CREATE TABLE planets (id INT PRIMARY KEY, name CHAR, radius FLOAT);
venus> INSERT INTO planets VALUES (1, 'Earth', 6371.0);
venus> INSERT INTO planets VALUES (2, 'Mars', 3389.5);
venus> INSERT INTO planets VALUES (3, 'Venus', 6051.8), (4, 'Jupiter', 69911.0);
venus> SELECT * FROM planets;
venus> SELECT name, radius FROM planets;
venus> SHOW TABLES;
venus> EXIT;
```

## Version Roadmaps
- [v1](/docs/notes.v1.md) - present version
- [v2](/docs/notes.v2.md)
- [v3](/docs/notes.v3.md)

## Contributing

This is an educational project, but contributions are welcome!

## Learning Resources

This project was inspired by:
- [CMU Database Systems Course](https://15445.courses.cs.cmu.edu)
- [Arpit Bhayani](https://www.youtube.com/c/ArpitBhayani)

## Limitations

- There are a lot, the main goal of this project is to learn databases and C++.

## Author

**Karthikeya** - [@karthikeyaspace](https://github.com/karthikeyaspace)

---

*Built with ❤️ for learning database internals*
