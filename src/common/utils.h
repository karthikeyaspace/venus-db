#pragma once

#include "buffer/buffer_pool.h"
#include "catalog/catalog.h"
#include "catalog/schema.h"
#include "common/config.h"
#include "storage/tuple.h"
#include "table/table_heap.h"

namespace venus {
namespace utils {
	void InsertTuple(table::TableHeap* table_heap, const Schema* schema, const std::vector<std::string>& values);

	Schema* GetMasterTableSchema() {
		Schema* schema = new Schema();
		schema->AddColumn("table_id", ColumnType::INT, true, 0);
		schema->AddColumn("table_name", ColumnType::CHAR, false, 1);
		schema->AddColumn("num_columns", ColumnType::INT, false, 2);
		schema->AddColumn("first_page_id", ColumnType::INT, false, 3);
		schema->AddColumn("primary_key", ColumnType::INT, false, 4); // column_id for primary key column
		return schema;
	}

	Schema* GetMasterColumnSchema() {
		Schema* schema = new Schema();
		schema->AddColumn("column_id", ColumnType::INT, true, 0);
		schema->AddColumn("table_id", ColumnType::INT, false, 1);
		schema->AddColumn("column_name", ColumnType::CHAR, false, 2);
		schema->AddColumn("column_type", ColumnType::INT, false, 3);
		schema->AddColumn("column_size", ColumnType::INT, false, 4);
		schema->AddColumn("ordinal_position", ColumnType::INT, false, 5);
		schema->AddColumn("is_primary_key", ColumnType::INT, false, 6);
		return schema;
	}
}
}