// /src/catalog/schema.h

/**
 * Schema represents the structure of a table in the database.
 * Every table has a schema that defines its columns, their types, and constraints.
 * It is used to validate tuples and provide metadata for the table - consistency and integrity in the table data.
 *
 * Generally, a schema is stored in catalog as metadata for a table
 * Integrity constraints, Data types, are checked and type casted while inputing/outputing the data
 *
 * VenusDB offers only Int, Float, and Char data types for now.
 * Only primary key supported, no foreign keys or unique constraints.
 *
 * This is a single instance of column and schema
 * For multiple tables, multiple instances of Schema and Column are created and mapped to table's
 */

#pragma once

#include <stdexcept> // for std::invalid_argument
#include <string>
#include <unordered_map>
#include <vector>

#include "common/config.h"

namespace venus {

class Column {
public:
	Column(std::string name, ColumnType type, bool is_primary)
	    : name_(std::move(name))
	    , type_(type)
	    , is_primary_(is_primary) { }

	const std::string& GetName() const { return name_; }
	ColumnType GetType() const { return type_; }
	bool IsPrimary() const { return is_primary_; }
	size_t GetLength() const {
		switch (type_) {
		case ColumnType::INT:
			return sizeof(int);
		case ColumnType::FLOAT:
			return sizeof(float);
		case ColumnType::CHAR:
			return sizeof(char) * MAX_CHAR_LENGTH;
		default:
			return 0;
		}
	}

private:
	std::string name_;
	ColumnType type_;
	bool is_primary_;
};

class Schema {
public:
	Schema() = default;
	~Schema() = default;

	// Columns are added in an order, for tuple serialization and deserialization
	void AddColumn(const std::string& name, ColumnType type, bool is_primary) {
		if (column_name_to_index_.find(name) != column_name_to_index_.end()) {
			throw std::invalid_argument("Column with name '" + name + "' already exists in the schema.");
		}
		columns_.emplace_back(name, type, is_primary);
		column_name_to_index_[name] = columns_.size() - 1;
	}

	const Column& GetColumn(const std::string& name) const {
		auto it = column_name_to_index_.find(name);
		if (it == column_name_to_index_.end()) {
			throw std::invalid_argument("Column with name '" + name + "' does not exist in the schema.");
		}
		return columns_[it->second];
	}

	const Column& GetColumn(size_t index) const {
		if (index >= columns_.size()) {
			throw std::out_of_range("Column index out of range");
		}
		return columns_[index];
	}

	size_t GetColumnCount() const { return columns_.size(); }

private:
	std::vector<Column> columns_;
	std::unordered_map<std::string, size_t> column_name_to_index_;

	// Disallow copy and move semantics
	DISALLOW_COPY_AND_MOVE(Schema);
};
} // namespace venus