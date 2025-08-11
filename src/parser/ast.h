#pragma once

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <common/types.h>

namespace venus {
namespace parser {

	struct ASTNode {
		ASTNodeType type;
		std::string value; // db/table/col/literal
		std::vector<std::shared_ptr<ASTNode>> children;

		ASTNode(ASTNodeType t, const std::string& v = "")
		    : type(t), value(v) { }

		void add_child(std::shared_ptr<ASTNode> child) {
			children.push_back(child);
		}

		// Helper: convert ASTNodeType to readable string
		static std::string typeToString(ASTNodeType t) {
			switch (t) {
			case ASTNodeType::SHOW_DATABASES: return "SHOW_DATABASES";
			case ASTNodeType::CREATE_DATABASE: return "CREATE_DATABASE";
			case ASTNodeType::DROP_DATABASE: return "DROP_DATABASE";
			case ASTNodeType::USE_DATABASE: return "USE_DATABASE";
			case ASTNodeType::CREATE_TABLE: return "CREATE_TABLE";
			case ASTNodeType::DROP_TABLE: return "DROP_TABLE";
			case ASTNodeType::SHOW_TABLES: return "SHOW_TABLES";
			case ASTNodeType::SELECT: return "SELECT";
			case ASTNodeType::INSERT: return "INSERT";
			case ASTNodeType::TABLE_REF: return "TABLE_REF";
			case ASTNodeType::COLUMN_REF: return "COLUMN_REF";
			case ASTNodeType::COLUMN_DEF: return "COLUMN_DEF";
			case ASTNodeType::CONST_VALUE: return "CONST_VALUE";
			case ASTNodeType::PROJECTION_LIST: return "PROJECTION_LIST";
			case ASTNodeType::CONDITION: return "CONDITION";
			case ASTNodeType::WHERE_CLAUSE: return "WHERE_CLAUSE";
			default: return "UNKNOWN_NODE";
			}
		}

		// Pretty-print AST tree
		void print(int depth = 0) const {
			// Indentation
			for (int i = 0; i < depth; i++) {
				std::cout << "  ";
			}

			// Node type and optional value
			std::cout << typeToString(type);
			if (!value.empty()) {
				std::cout << " : " << value;
			}
			std::cout << "\n";

			// Recurse
			for (const auto& child : children) {
				if (child) {
					child->print(depth + 1);
				}
			}
		}
	};

} // namespace parser
} // namespace venus
