#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "catalog/schema.h"
#include "common/config.h"
#include "common/types.h"

namespace venus {
namespace parser {

	struct ASTNode {
		ASTNodeType type;
		std::string value; // db/table/col/literal
		std::vector<std::shared_ptr<ASTNode>> children;

		ASTNode(ASTNodeType t, const std::string& v = "")
		    : type(t)
		    , value(v) { }

		void add_child(std::shared_ptr<ASTNode> child) {
			children.push_back(child);
		}

		static std::string typeToString(ASTNodeType t) {
			switch (t) {
			case ASTNodeType::SHOW_DATABASES:
				return "SHOW_DATABASES";
			case ASTNodeType::CREATE_DATABASE:
				return "CREATE_DATABASE";
			case ASTNodeType::DROP_DATABASE:
				return "DROP_DATABASE";
			case ASTNodeType::USE_DATABASE:
				return "USE_DATABASE";
			case ASTNodeType::CREATE_TABLE:
				return "CREATE_TABLE";
			case ASTNodeType::DROP_TABLE:
				return "DROP_TABLE";
			case ASTNodeType::SHOW_TABLES:
				return "SHOW_TABLES";
			case ASTNodeType::SELECT:
				return "SELECT";
			case ASTNodeType::INSERT:
				return "INSERT";
			case ASTNodeType::TABLE_REF:
				return "TABLE_REF";
			case ASTNodeType::COLUMN_REF:
				return "COLUMN_REF";
			case ASTNodeType::COLUMN_DEF:
				return "COLUMN_DEF";
			case ASTNodeType::CONST_VALUE:
				return "CONST_VALUE";
			case ASTNodeType::PROJECTION_LIST:
				return "PROJECTION_LIST";
			case ASTNodeType::CONDITION:
				return "CONDITION";
			case ASTNodeType::WHERE_CLAUSE:
				return "WHERE_CLAUSE";
			default:
				return "UNKNOWN_NODE";
			}
		}

		void Print(int depth = 0) const {
			for (int i = 0; i < depth; i++) {
				std::cout << "  ";
			}

			std::cout << typeToString(type);
			if (!value.empty()) {
				std::cout << " : " << value;
			}
			std::cout << "\n";

			for (const auto& child : children) {
				if (child) {
					child->Print(depth + 1);
				}
			}
		}
	};

	struct BoundASTNode {
		ASTNodeType type;
		virtual ~BoundASTNode() = default;

		ASTNodeType GetType() const { return type; }
	};

	struct BoundDatabaseNode : BoundASTNode {
		// Bound node for CREATE/DROP/SHOW/USE DATABASE commands
		std::string database_name;

		BoundDatabaseNode(ASTNodeType node_type, const std::string& db_name = "")
		    : database_name(db_name) {
			type = node_type;
		}
	};

	struct BoundSelectNode : BoundASTNode {
		TableRef table_ref;
		std::vector<ColumnRef> projections;
		std::unique_ptr<Expression> where_clause;
		int limit;

		BoundSelectNode(
		    TableRef table,
		    std::vector<ColumnRef> cols,
		    std::unique_ptr<Expression> where = nullptr,
		    int limit_value = -1)
		    : table_ref(table)
		    , projections(std::move(cols))
		    , where_clause(std::move(where))
		    , limit(limit_value) {
			type = ASTNodeType::SELECT;
		}
	};

	struct BoundInsertNode : BoundASTNode {
		TableRef table_ref;
		std::vector<ColumnRef> target_cols;
		std::vector<ConstantType> values;

		BoundInsertNode(
		    TableRef table,
		    std::vector<ColumnRef> cols,
		    std::vector<ConstantType> vals)
		    : table_ref(table)
		    , target_cols(std::move(cols))
		    , values(std::move(vals)) {
			type = ASTNodeType::INSERT;
		}
	};

	struct BoundCreateTableNode : BoundASTNode {
		std::string table_name;
		Schema schema;

		BoundCreateTableNode(const std::string& table_name)
		    : table_name(table_name) {
			type = ASTNodeType::CREATE_TABLE;
		}
	};

	struct BoundDropTableNode : BoundASTNode {
		std::string table_name;

		BoundDropTableNode(const std::string& table_name)
		    : table_name(table_name) {
			type = ASTNodeType::DROP_TABLE;
		}
	};

	struct BoundShowTablesNode : BoundASTNode {
		BoundShowTablesNode() {
			type = ASTNodeType::SHOW_TABLES;
		}
	};

	struct BoundExecNode : BoundASTNode {
		std::string query;

		BoundExecNode(const std::string& query)
		    : query(query) {
			type = ASTNodeType::EXEC;
		}
	};

} // namespace parser
} // namespace venus
