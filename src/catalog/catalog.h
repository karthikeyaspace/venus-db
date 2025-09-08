// /src/catalog/catalog.h

/**
 * The initial implementation thought for catalog was to have a catalog manager
 * that will access a catalog database and its tables to retrieve list of databases, tables, schemas, indexes etc
 *
 * That sounds complicated, the common and best approach i can think of is master table implementation by sqlite
 *
 * Every database will have a few master tables that will have all the metadata of the database
 *
 * ### <dbname>_tables
 * | Column        | Data Type |
 * |---------------|-----------|
 * | table_id      |  INT      |
 * | table_name    |  CHAR     |
 * | num_columns   |  INT      |
 * | first_page_id |  INT      |
 * | primary_key   |  INT      |
 *
 * ### <dbname>_columns
 * | Column           | Data Type |
 * |------------------|-----------|
 * | col_id           |  INT      |
 * | table_id         |  INT      |
 * | col_name         |  CHAR     |
 * | col_type         |  CHAR     |
 * | col_size         |  INT      |
 * | ordinal_position |  INT      |
 * | is_primary_key   |  INT      |
 * | is_nullable      |  INT      |
 *
 * ### <dbname>_indexes
 * | Column       | Data Type |
 * |--------------|-----------|
 * | index_id     |  INT      |
 * | table_id     |  INT      |
 * | index_name   |  CHAR     |
 * | col_id       |  INT      |
 * | is_unique    |  INT      |
 * | root_page_id |  INT      |
 *
 * Just like sqlite, these tables will be accessed using predefined SQL statements from within
 * eg, when creating a table in db, we do `SELECT * FROM <dbname>_tables WHERE table_name = 'my_table'`
 * to check if the table already exists, and if not, we insert a new row into the `<dbname>_tables` table with the new table's metadata.
 *
 * This hence clears that for using this, we need entire db build first, incl parser, executor, buffer pool, table heap, disk manager.
 */

#pragma once

#include <string>
#include <vector>

#include "buffer/buffer_pool.h"
#include "catalog/schema.h"
#include "common/config.h"
#include "table/table_heap.h"
#include "storage/tuple.h"

namespace venus {
namespace catalog {
	class CatalogManager {
	public:
		CatalogManager(buffer::BufferPoolManager* bpm);
		~CatalogManager();

		void CreateTable(const std::string table_name, const Schema* schema);
		bool DropTable(const std::string& table_name);

		TableRef* GetTableRef(const std::string& table_name);

	private:
		buffer::BufferPoolManager* bpm_;
		table::TableHeap* tables_table_;
		table::TableHeap* columns_table_;

		Schema* master_tables_schema_;
		Schema* master_columns_schema_;

		table_id_t next_table_id_;
		column_id_t next_column_id_;

		void InitializeSystemTables();
		void CreateNewSystemTables();
		void InsertSystemTableColumns();
		void LoadMaxIds(); // Load max IDs from catalog

		table_id_t GetNextTableId();
		column_id_t GetNextColumnId();
		
		std::string GetValueAsString(const Tuple& tuple, uint32_t column_idx, const Schema* schema) const;
	};
}
}