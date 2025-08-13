// /src/binder/binder.cpp

#include "binder/binder.h"

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
			// In general, data about databases is also stored in a master table, but venus does not do that
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

			table_id_t table_id = catalog_->TableExists(table_name);
			if (table_id == -1) {
				throw std::runtime_error("Binder error: Table '" + table_name + "' does not exist");
			}

			// Get the table schema
			Schema* table_schema = catalog_->GetTableSchema(table_id);
			if (table_schema == nullptr) {
				throw std::runtime_error("Binder error: Could not retrieve schema for table '" + table_name + "'");
			}

			BoundTableRef bound_table(table_id, table_name, *table_schema);

			std::vector<BoundColumnRef> bound_columns;
			auto projection_list = ast->children[0];

			for (const auto& projection : projection_list->children) {
				if (projection->value == "*") {
					// SELECT * - add all columns from the table
					for (size_t i = 0; i < table_schema->GetColumnCount(); i++) {
						const Column& column = table_schema->GetColumn(i);
						BoundColumnRef bound_col;
						bound_col.col_id = static_cast<column_id_t>(i);
						bound_col.column_entry_ = const_cast<Column*>(&column);
						bound_columns.push_back(bound_col);
					}
				} else {
					const std::string& col_name = projection->value;
					if (!table_schema->HasColumn(col_name)) {
						throw std::runtime_error("Binder error: Column '" + col_name + "' does not exist in table '" + table_name + "'");
					}

					const Column& column = table_schema->GetColumn(col_name);
					BoundColumnRef bound_col;
					bound_col.col_id = static_cast<column_id_t>(column.GetOrdinalPosition());
					bound_col.column_entry_ = const_cast<Column*>(&column);
					bound_columns.push_back(bound_col);
				}
			}

			for (const auto& col : bound_columns) {
				LOG("Selected column: " + col.column_entry_->GetName());
			}

			return std::make_unique<BoundSelectNode>(bound_table, std::move(bound_columns));
		}

		case ASTNodeType::CREATE_TABLE: {
			return nullptr;
		}

		case ASTNodeType::INSERT: {
			return nullptr;
		}

		case ASTNodeType::DROP_TABLE: {
			const std::string& table_name = ast->value;
			if (catalog_->TableExists(table_name) == -1) {
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
