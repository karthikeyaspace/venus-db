// /src/catalog/catalog.cpp

#include "catalog/catalog.h"
#include "common/config.h"
#include "common/utils.h"

namespace venus {
namespace catalog {

	CatalogManager::CatalogManager(buffer::BufferPoolManager* bpm)
	    : bpm_(bpm)
	    , tables_table_(nullptr)
	    , columns_table_(nullptr)
	    , next_table_id_(2) // Start after system tables (0, 1)
	    , next_column_id_(14) // Start after system columns (0-13)
	    , master_tables_schema_(utils::GetMasterTableSchema())
	    , master_columns_schema_(utils::GetMasterColumnSchema()) {
		InitializeSystemTables();
		LoadMaxIds();
	}

	CatalogManager::~CatalogManager() {
		delete tables_table_;
		delete columns_table_;
	}

	void CatalogManager::InitializeSystemTables() {
		Page* tables_page = bpm_->FetchPage(MASTER_TABLES_PAGE_ID);

		if (tables_page == nullptr) {
			CreateNewSystemTables();
		} else {

			tables_table_ = new table::TableHeap(bpm_, master_tables_schema_, MASTER_TABLES_PAGE_ID);
			columns_table_ = new table::TableHeap(bpm_, master_columns_schema_, MASTER_COLUMNS_PAGE_ID);

			std::cout << "CatalogManager: Loaded existing system tables" << std::endl;
		}
	}

	void CatalogManager::CreateNewSystemTables() {

		Page* tables_page = bpm_->NewPage(MASTER_TABLES_PAGE_ID);
		if (tables_page == nullptr) {
			throw std::runtime_error("Failed to create master_tables page");
		}

		tables_table_ = new table::TableHeap(bpm_, master_tables_schema_, MASTER_TABLES_PAGE_ID);

		utils::InsertTuple(tables_table_, master_tables_schema_,
		    { "0", MASTER_TABLES_NAME, "5", "0", std::to_string(MASTER_TABLES_PAGE_ID), "0" });

		utils::InsertTuple(tables_table_, master_tables_schema_,
		    { "1", MASTER_COLUMNS_NAME, "7", "0", std::to_string(MASTER_COLUMNS_PAGE_ID), "0" });

		// Create the master_columns page at page 1
		Page* columns_page = bpm_->NewPage(MASTER_COLUMNS_PAGE_ID);
		if (columns_page == nullptr) {
			throw std::runtime_error("Failed to create master_columns page");
		}

		columns_table_ = new table::TableHeap(bpm_, master_columns_schema_, MASTER_COLUMNS_PAGE_ID);

		InsertSystemTableColumns();

		bpm_->FlushPage(MASTER_TABLES_PAGE_ID);
		bpm_->FlushPage(MASTER_COLUMNS_PAGE_ID);

		std::cout << "CatalogManager: Created new system tables" << std::endl;
	}

	void CatalogManager::CreateTable(const std::string& table_name, const Schema* schema) {
		if (TableExists(table_name)) {
			throw std::runtime_error("Table '" + table_name + "' already exists.");
		}

		page_id_t first_page_id = bpm_->NewPage()->GetPageId();
		table_id_t table_id = GetNextTableId();

		column_id_t primary_key_col = 0;
		for (size_t i = 0; i < schema->GetColumnCount(); ++i) {
			if (schema->GetColumn(i).IsPrimary()) {
				primary_key_col = static_cast<column_id_t>(i);
				break;
			}
		}

		utils::InsertTuple(
		    tables_table_,
		    master_tables_schema_,
		    { std::to_string(table_id),
		        table_name,
		        std::to_string(schema->GetColumnCount()),
		        std::to_string(first_page_id),
		        std::to_string(primary_key_col) });

		// insert columns to master_columns
		for (size_t i = 0; i < schema->GetColumnCount(); ++i) {
			const Column& column = schema->GetColumn(i);
			column_id_t column_id = GetNextColumnId();

			utils::InsertTuple(
			    columns_table_,
			    master_columns_schema_,
			    { std::to_string(column_id),
			        std::to_string(table_id),
			        column.GetName(),
			        std::to_string(static_cast<int>(column.GetType())),
			        std::to_string(column.GetLength()),
			        std::to_string(column.GetOrdinalPosition()),
			        std::to_string(column.IsPrimary()) });
		}

		bpm_->FlushPage(MASTER_TABLES_PAGE_ID);
		bpm_->FlushPage(MASTER_COLUMNS_PAGE_ID);
		bpm_->FlushPage(first_page_id);
	}

	// access both tables, and create  a schema object
	Schema* CatalogManager::GetTableSchema(const std::string& table_name) {
		if (!TableExists(table_name)) {
			throw std::runtime_error("Table '" + table_name + "' does not exist.");
		}

		Schema* schema = new Schema();
		for (auto it = columns_table_->begin(); it != columns_table_->end(); ++it) {
			const Tuple& tuple = *it;
			const char* value = tuple.GetValue(1, master_columns_schema_); // Get table_id
			if (std::stoi(std::string(value)) == std::stoi(table_name)) {
				const char* col_name = tuple.GetValue(2, master_columns_schema_);
				ColumnType col_type = static_cast<ColumnType>(std::stoi(std::string(tuple.GetValue(3, master_columns_schema_))));
				size_t ordinal_position = std::stoi(std::string(tuple.GetValue(5, master_columns_schema_)));
				bool is_primary = std::stoi(std::string(tuple.GetValue(6, master_columns_schema_))) == 1;
				schema->AddColumn(col_name, col_type, is_primary, ordinal_position);
			}
		}
		return schema;
	}

	bool CatalogManager::TableExists(const std::string& table_name) {
		for (auto it = tables_table_->begin(); it != tables_table_->end(); ++it) {
			const Tuple& tuple = *it;
			const char* value = tuple.GetValue(1, master_tables_schema_); // Get table_name
			if (std::string(value) == table_name) {
				return true;
			}
		}
		return false;
	}

	table_id_t CatalogManager::GetNextTableId() {
		return next_table_id_++; // Return current and increment
	}

	column_id_t CatalogManager::GetNextColumnId() {
		return next_column_id_++; // Return current and increment
	}

	void CatalogManager::LoadMaxIds() {
		// finding hightest table_id
		for (auto it = tables_table_->begin(); it != tables_table_->end(); ++it) {
			const Tuple& tuple = *it;
			const char* value = tuple.GetValue(0, master_tables_schema_);
			table_id_t current_id = std::stoi(std::string(value));
			if (current_id >= next_table_id_) {
				next_table_id_ = current_id + 1;
			}
		}

		// finding highest column_id
		for (auto it = columns_table_->begin(); it != columns_table_->end(); ++it) {
			const Tuple& tuple = *it;
			const char* value = tuple.GetValue(0, master_columns_schema_);
			column_id_t current_id = std::stoi(std::string(value));
			if (current_id >= next_column_id_) {
				next_column_id_ = current_id + 1;
			}
		}

		std::cout << "CatalogManager: Loaded max IDs - next_table_id: " << next_table_id_
		          << ", next_column_id: " << next_column_id_ << std::endl;
	}

	void CatalogManager::InsertSystemTableColumns() {
		// Insert column definitions for master_tables

		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "0", "0", "table_id", "INT", "4", "1", "1", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "1", "0", "table_name", "CHAR", "32", "2", "0", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "2", "0", "num_columns", "INT", "4", "3", "0", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "3", "0", "num_rows", "INT", "4", "4", "0", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "4", "0", "primary_key", "INT", "4", "5", "0", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "5", "0", "root_page_id", "INT", "4", "6", "0", "0" });

		// Insert column definitions for master_columns
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "6", "1", "col_id", "INT", "4", "1", "1", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "7", "1", "table_id", "INT", "4", "2", "0", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "8", "1", "col_name", "CHAR", "32", "3", "0", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "9", "1", "col_type", "CHAR", "16", "4", "0", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "10", "1", "col_size", "INT", "4", "5", "0", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "11", "1", "ordinal_position", "INT", "4", "6", "0", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "12", "1", "is_primary_key", "INT", "4", "7", "0", "0" });
		utils::InsertTuple(columns_table_, master_columns_schema_,
		    { "13", "1", "is_nullable", "INT", "4", "8", "0", "0" });
	}

}
}