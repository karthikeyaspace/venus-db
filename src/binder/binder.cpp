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
			// Its processing is taken care of by executor, this is just a binder

			return std::make_unique<BoundDatabaseNode>(ast->type, ast->value);
		}

		case ASTNodeType::SELECT: {
			return nullptr;
		}

		case ASTNodeType::CREATE_TABLE: {
			return nullptr;
		}

		case ASTNodeType::INSERT: {
			return nullptr;
		}

		case ASTNodeType::DROP_TABLE: {
			const std::string& table_name = ast->children[0]->value;
			if (!catalog_->TableExists(table_name)) {
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
