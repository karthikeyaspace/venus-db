// /src/binder/binder.cpp

#include "binder/binder.h"
#include <sstream>

using namespace venus::parser;

namespace venus {
namespace binder {

	std::unique_ptr<parser::BoundASTNode> Binder::Bind(std::unique_ptr<parser::ASTNode> ast) {
		if (ast == nullptr) {
			throw std::invalid_argument("Binder error: Parsing failed");
		}

		if (catalog_ == nullptr && ast->type != ASTNodeType::USE_DATABASE) {
			throw std::runtime_error("Binder error: Database is not initialized");
		}

		switch (ast->type) {
		case ASTNodeType::SHOW_DATABASES:
		case ASTNodeType::CREATE_DATABASE:
		case ASTNodeType::DROP_DATABASE:
		case ASTNodeType::USE_DATABASE: {
			// In general, metadata about databases is also stored in a master table, but venus does not do that
			// venus only stores metadata of tables and columns
			// So we dont need catalog here, just Bind and send to next step,
			// Its processing is taken care of by executor, this is just a bindere

			return std::make_unique<BoundDatabaseNode>(ast->type, ast->value);
		}

		case ASTNodeType::SELECT: {
			// ast->children[0] is PROJECTION_LIST, children of it are COLUMN_REF
			// ast->children[1] is TABLE_REF

			if (ast->children.size() < 2) {
				throw std::runtime_error("Binder error: Invalid SELECT AST structure");
			}

			const std::string& table_name = ast->children[1]->value;

			TableRef* bound_table = catalog_->GetTableRef(table_name);
			if (bound_table == nullptr) {
				throw std::runtime_error("Binder error: Table '" + table_name + "' does not exist");
			}

			std::vector<ColumnRef> bound_columns;
			auto projection_list = ast->children[0];

			for (const auto& projection : projection_list->children) {
				if (projection->value == "*") {
					// SELECT * - add all columns from the table
					for (size_t i = 0; i < bound_table->GetSchema()->GetColumnCount(); i++) {
						const Column& column = bound_table->GetSchema()->GetColumn(i);
						ColumnRef bound_col;
						bound_col.col_id = static_cast<column_id_t>(i);
						bound_col.column_entry_ = const_cast<Column*>(&column);
						bound_columns.push_back(bound_col);
					}
				} else {
					const std::string& col_name = projection->value;
					if (!bound_table->GetSchema()->HasColumn(col_name)) {
						throw std::runtime_error("Binder error: Column '" + col_name + "' does not exist in table '" + table_name + "'");
					}

					const Column& column = bound_table->GetSchema()->GetColumn(col_name);
					ColumnRef bound_col;
					bound_col.col_id = static_cast<column_id_t>(column.GetOrdinalPosition());
					bound_col.column_entry_ = const_cast<Column*>(&column);
					bound_columns.push_back(bound_col);
				}
			}

			return std::make_unique<BoundSelectNode>(bound_table, std::move(bound_columns));
		}

		case ASTNodeType::CREATE_TABLE: {
			const std::string& table_name = ast->value;

			TableRef* bound_table = catalog_->GetTableRef(table_name);
			if (bound_table == nullptr) {
				throw std::runtime_error("Binder error: Table '" + table_name + "' does not exist");
			}

			auto bound_create_table = std::make_unique<BoundCreateTableNode>(table_name);

			for (const auto& child : ast->children) {
				if (child->type == ASTNodeType::COLUMN_DEF) {
					std::string col_def = child->value;
					std::vector<std::string> parts;

					std::istringstream iss(col_def);
					std::string part;
					while (iss >> part) {
						parts.push_back(part);
					}

					// atleast col_name, col_type
					if (parts.size() < 2) {
						throw std::runtime_error("Binder error: Invalid column definition: " + col_def);
					}

					std::string col_name = parts[0];
					std::string col_type_str = parts[1];
					bool is_primary = (parts.size() > 2 && parts[2] == "PK");

					ColumnType col_type;
					if (col_type_str == "int") {
						col_type = ColumnType::INT;
					} else if (col_type_str == "float") {
						col_type = ColumnType::FLOAT;
					} else if (col_type_str == "char") {
						col_type = ColumnType::CHAR;
					} else {
						throw std::runtime_error("Binder error: Unsupported column type: " + col_type_str);
					}

					size_t ordinal_position = bound_create_table->schema.GetColumnCount();
					bound_create_table->schema.AddColumn(col_name, col_type, is_primary, ordinal_position);
				}
			}

			if (bound_create_table->schema.GetColumnCount() == 0) {
				throw std::runtime_error("Binder error: CREATE TABLE must have at least one column");
			}

			return std::move(bound_create_table);
		}

		case ASTNodeType::INSERT: {
			const std::string& table_name = ast->value;

			TableRef* bound_table = catalog_->GetTableRef(table_name);
			if (bound_table == nullptr) {
				throw std::runtime_error("Binder error: Table '" + table_name + "' does not exist");
			}

			std::vector<ColumnRef> target_cols;
			for (size_t i = 0; i < bound_table->GetSchema()->GetColumnCount(); i++) {
				const Column& column = bound_table->GetSchema()->GetColumn(i);
				ColumnRef bound_col;
				bound_col.col_id = static_cast<column_id_t>(i);
				bound_col.column_entry_ = const_cast<Column*>(&column);
				target_cols.push_back(bound_col);
			}

			std::vector<ConstantType> bound_values;
			for (const auto& child : ast->children) {
				if (child->type == ASTNodeType::CONST_VALUE) {
					std::string value_str = child->value;

					size_t value_index = bound_values.size();
					if (value_index >= bound_table->GetSchema()->GetColumnCount()) {
						throw std::runtime_error("Binder error: Too many values provided for INSERT");
					}

					const Column& target_column = bound_table->GetSchema()->GetColumn(value_index);
					ColumnType expected_type = target_column.GetType();

					ConstantType bound_const;
					bound_const.value = value_str;
					bound_const.type = expected_type;

					// checking literal validity
					switch (expected_type) {
					case ColumnType::INT: {
						try {
							std::stoi(value_str);
						} catch (const std::exception&) {
							throw std::runtime_error("Binder error: Invalid integer value '" + value_str + "' for column '" + target_column.GetName() + "'");
						}
						break;
					}
					case ColumnType::FLOAT: {
						try {
							std::stof(value_str);
						} catch (const std::exception&) {
							throw std::runtime_error("Binder error: Invalid float value '" + value_str + "' for column '" + target_column.GetName() + "'");
						}
						break;
					}
					case ColumnType::CHAR: {
						break;
					}
					default:
						throw std::runtime_error("Binder error: Unsupported column type for INSERT");
					}

					bound_values.push_back(bound_const);
				}
			}

			if (bound_values.size() != bound_table->GetSchema()->GetColumnCount()) {
				throw std::runtime_error("Binder error: Number of values (" + std::to_string(bound_values.size()) + ") does not match number of columns (" + std::to_string(bound_table->GetSchema()->GetColumnCount()) + ")");
			}

			return std::make_unique<BoundInsertNode>(bound_table, std::move(target_cols), std::move(bound_values));
		}

		case ASTNodeType::DROP_TABLE: {
			const std::string& table_name = ast->value;
			
			TableRef* bound_table = catalog_->GetTableRef(table_name);
			if (bound_table == nullptr) {
				throw std::runtime_error("Binder error: Table '" + table_name + "' does not exist");
			}

			return std::make_unique<BoundDropTableNode>(table_name);
		}

		case ASTNodeType::SHOW_TABLES: {
			return std::make_unique<BoundShowTablesNode>();
		}

		default: {
			throw std::runtime_error("Binder error: Unsupported AST node type: " + ASTNode::typeToString(ast->type));
		}
		}

		return nullptr;
	}

} // namespace binder
} // namespace venus
