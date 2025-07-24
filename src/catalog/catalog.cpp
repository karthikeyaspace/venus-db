// /src/catalog/catalog.cpp

#include "catalog/catalog.h"
#include "common/config.h"

namespace venus {
namespace catalog {

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

	CatalogManager::CatalogManager(buffer::BufferPoolManager* bpm)
	    : bpm_(bpm)
	    , tables_table_(nullptr)
	    , columns_table_(nullptr)
	    , next_table_id_(2) // Start after system tables (0, 1)
	    , next_column_id_(12) // Start after system columns (0-11)
	    , master_tables_schema_(GetMasterTableSchema())
	    , master_columns_schema_(GetMasterColumnSchema()) {
		InitializeSystemTables();
	}

	CatalogManager::~CatalogManager() {
		delete tables_table_;
		delete columns_table_;
	}

	void CatalogManager::InitializeSystemTables() {
		Page* tables_page = bpm_->FetchPage(MASTER_TABLES_PAGE_ID);

		if (tables_page == nullptr) {
			CreateNewSystemTables();
			// next table and column IDs are initialized in constructor
		} else {
			tables_table_ = new table::TableHeap(bpm_, master_tables_schema_, MASTER_TABLES_PAGE_ID);
			columns_table_ = new table::TableHeap(bpm_, master_columns_schema_, MASTER_COLUMNS_PAGE_ID);
			if (tables_table_ == nullptr || columns_table_ == nullptr) {
				throw std::runtime_error("Failed to initialize system tables.");
			}

			// obtain next table and column IDs from existing tables
			LoadMaxIds();
		}
	}

	void CatalogManager::CreateNewSystemTables() {
		// page 0 reserved for master_tables
		Page* tables_page = bpm_->NewPage(MASTER_TABLES_PAGE_ID);
		if (tables_page == nullptr) {
			throw std::runtime_error("Failed to create master_tables page");
		}

		tables_table_ = new table::TableHeap(bpm_, master_tables_schema_, MASTER_TABLES_PAGE_ID);

		// Insert master_tables entry: table_id, table_name, num_columns, first_page_id, primary_key
		tables_table_->InsertTuple({ "0", MASTER_TABLES_NAME, "5", std::to_string(MASTER_TABLES_PAGE_ID), "0" });
		tables_table_->InsertTuple({ "1", MASTER_COLUMNS_NAME, "7", std::to_string(MASTER_COLUMNS_PAGE_ID), "0" });

		// page 1 reserved for master_columns
		Page* columns_page = bpm_->NewPage(MASTER_COLUMNS_PAGE_ID);
		if (columns_page == nullptr) {
			throw std::runtime_error("Failed to create master_columns page");
		}

		columns_table_ = new table::TableHeap(bpm_, master_columns_schema_, MASTER_COLUMNS_PAGE_ID);

		InsertSystemTableColumns();

		bpm_->FlushPage(MASTER_TABLES_PAGE_ID);
		bpm_->FlushPage(MASTER_COLUMNS_PAGE_ID);
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

		// Insert table metadata into master_tables
		tables_table_->InsertTuple({ std::to_string(table_id),
		    table_name,
		    std::to_string(schema->GetColumnCount()),
		    std::to_string(first_page_id),
		    std::to_string(primary_key_col) });

		// insert columns to master_columns
		for (size_t i = 0; i < schema->GetColumnCount(); ++i) {
			const Column& column = schema->GetColumn(i);
			column_id_t column_id = GetNextColumnId();

			columns_table_->InsertTuple({ std::to_string(column_id),
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
			const char* value = tuple.GetValue(1, master_columns_schema_); // table_id
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
		return next_table_id_++;
	}

	column_id_t CatalogManager::GetNextColumnId() {
		return next_column_id_++;
	}

	void CatalogManager::LoadMaxIds() {
		int table_count = 0;
		for (auto it = tables_table_->begin(); it != tables_table_->end(); ++it) {
			table_count++;
		}

		int column_count = 0;
		for (auto it = columns_table_->begin(); it != columns_table_->end(); ++it) {
			column_count++;
		}
		next_table_id_ = table_count;
		next_column_id_ = column_count;
	}

	void CatalogManager::InsertSystemTableColumns() {
		// Schema: column_id, table_id, column_name, column_type, column_size, ordinal_position
		columns_table_->InsertTuple({ "0", "0", "table_id", "0", "4", "0", "1" });
		columns_table_->InsertTuple({ "1", "0", "table_name", "1", "32", "1", "0" });
		columns_table_->InsertTuple({ "2", "0", "num_columns", "0", "4", "2", "0" });
		columns_table_->InsertTuple({ "3", "0", "first_page_id", "0", "4", "3", "0" });
		columns_table_->InsertTuple({ "4", "0", "primary_key", "0", "4", "4", "0" });

		// Schema: column_id, table_id, column_name, column_type, column_size, ordinal_position, is_primary_key
		columns_table_->InsertTuple({ "5", "1", "column_id", "0", "4", "0", "1" });
		columns_table_->InsertTuple({ "6", "1", "table_id", "0", "4", "1", "0" });
		columns_table_->InsertTuple({ "7", "1", "column_name", "1", "32", "2", "0" });
		columns_table_->InsertTuple({ "8", "1", "column_type", "0", "4", "3", "0" });
		columns_table_->InsertTuple({ "9", "1", "column_size", "0", "4", "4", "0" });
		columns_table_->InsertTuple({ "10", "1", "ordinal_position", "0", "4", "5", "0" });
		columns_table_->InsertTuple({ "11", "1", "is_primary_key", "0", "4", "6", "0" });
	}

	std::string CatalogManager::GetValueAsString(const Tuple& tuple, uint32_t column_idx, const Schema* schema) const {
		const Column& column = schema->GetColumn(column_idx);
		const char* raw_data = tuple.GetValue(column_idx, schema);

		switch (column.GetType()) {
		case ColumnType::INT: {
			int int_value;
			std::memcpy(&int_value, raw_data, sizeof(int));
			return std::to_string(int_value);
		}
		case ColumnType::FLOAT: {
			float float_value;
			std::memcpy(&float_value, raw_data, sizeof(float));
			return std::to_string(float_value);
		}
		case ColumnType::CHAR: {
			return std::string(raw_data);
		}
		default:
			return "UNKNOWN_TYPE";
		}
	}

	void CatalogManager::ShowMasterTables() {
		std::cout << "=== MASTER_TABLES ===" << std::endl;
		std::cout
		    << "table_id | table_name | num_columns | first_page_id | primary_key" << std::endl;
		std::cout << "---------|------------|-------------|---------------|------------" << std::endl;

		int table_count = 0;
		for (auto it = tables_table_->begin(); it != tables_table_->end(); ++it) {
			const Tuple& tuple = *it;
			std::cout << GetValueAsString(tuple, 0, master_tables_schema_) << " | "
			          << GetValueAsString(tuple, 1, master_tables_schema_) << " | "
			          << GetValueAsString(tuple, 2, master_tables_schema_) << " | "
			          << GetValueAsString(tuple, 3, master_tables_schema_) << " | "
			          << GetValueAsString(tuple, 4, master_tables_schema_) << std::endl;
			table_count++;
		}

		std::cout << "=== MASTER_COLUMNS ===" << std::endl;
		std::cout
		    << "col_id | table_id | col_name | col_type | col_size | ordinal_pos | is_primary" << std::endl;
		std::cout << "-------|----------|----------|----------|----------|-------------|------------" << std::endl;

		for (auto it = columns_table_->begin(); it != columns_table_->end(); ++it) {
			const Tuple& tuple = *it;
			std::cout << GetValueAsString(tuple, 0, master_columns_schema_) << " | "
			          << GetValueAsString(tuple, 1, master_columns_schema_) << " | "
			          << GetValueAsString(tuple, 2, master_columns_schema_) << " | "
			          << GetValueAsString(tuple, 3, master_columns_schema_) << " | "
			          << GetValueAsString(tuple, 4, master_columns_schema_) << " | "
			          << GetValueAsString(tuple, 5, master_columns_schema_) << " | "
			          << GetValueAsString(tuple, 6, master_columns_schema_) << std::endl;
		}
		std::cout << std::endl;
	}

}
}