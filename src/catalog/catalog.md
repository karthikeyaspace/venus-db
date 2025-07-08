The initial implementation thought for catalog was to have a catalog manager 
that will access a catalog database and its tables to retrieve list of databases, tables, schemas, indexes etc

That sounds complicated, the common and best approach i can think of is master table implementation by sqlite

Every database will have a few master tables that will have all the metadata of the database

### <dbname>_tables
| Column       | Data Type |
|--------------|-----------|
| table_id     |  INT      |
| table_name   |  CHAR     |
| num_columns  |  INT      |
| num_rows     |  INT      |
| primary_key  |  INT      |
| root_page_id |  INT      |

### <dbname>_columns
| Column           | Data Type |
|------------------|-----------|
| col_id           |  INT      |
| table_id         |  INT      |
| col_name         |  CHAR     |
| col_type         |  CHAR     |
| col_size         |  INT      |
| ordinal_position |  INT      |
| is_primary_key   |  INT      |
| is_nullable      |  INT      |

### <dbname>_indexes
| Column       | Data Type |
|--------------|-----------|
| index_id     |  INT      |
| table_id     |  INT      |
| index_name   |  CHAR     |
| col_id       |  INT      |
| is_unique    |  INT      |
| root_page_id |  INT      |


Just like sqlite, these tables will be accessed using predefined SQL statements from within
eg, when creating a table in db, we do `SELECT * FROM <dbname>_tables WHERE table_name = 'my_table'`
to check if the table already exists, and if not, we insert a new row into the `<dbname>_tables` table with the new table's metadata.


This hence clears that for using this, we need entire db build first, incl parser, executor, buffer pool, table heap, disk manager.