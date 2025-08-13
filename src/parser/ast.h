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

	struct BoundASTNode {
		ASTNodeType type;
		virtual ~BoundASTNode() = default;
 	};

	struct BoundTableRef {
		table_id_t table_id;
		std::string table_name;
		Schema schema;

		BoundTableRef(table_id_t id_, const std::string& name_, const Schema &schema_)
		    : table_id(id_)
		    , table_name(name_)
		    , schema(schema_) { }

		const Column& GetColumnByName(const std::string& name) const {
			return schema.GetColumn(name);
		}

		const Column& GetColumnByIndex(size_t index) const {
			return schema.GetColumn(index);
		}
	};

	struct BoundColumnRef {
		column_id_t col_id;
		Column* column_entry_;
	};

	struct BoundConstant {
		std::string value;
		ColumnType type;
	};

	struct BoundExpression {
		// for now this struct only supports simple equality (=, <, >, <=, >=)
		BoundColumnRef left;
		std::string op;
		BoundConstant right;
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
		BoundTableRef table_ref;
		std::vector<BoundColumnRef> projections;
		std::unique_ptr<BoundExpression> where_clause;
		int limit;

		BoundSelectNode(
		    BoundTableRef table,
		    std::vector<BoundColumnRef> cols,
		    std::unique_ptr<BoundExpression> where = nullptr,
		    int limit_value = -1)
		    : table_ref(table)
		    , projections(std::move(cols))
		    , where_clause(std::move(where))
		    , limit(limit_value) {
			type = ASTNodeType::SELECT;
		}
	};

	struct BoundInsertNode : BoundASTNode {
		BoundTableRef table_ref;
		std::vector<BoundColumnRef> target_cols;
		std::vector<BoundConstant> values;

		BoundInsertNode(
		    BoundTableRef table,
		    std::vector<BoundColumnRef> cols,
		    std::vector<BoundConstant> vals)
		    : table_ref(table)
		    , target_cols(std::move(cols))
		    , values(std::move(vals)) {
			type = ASTNodeType::INSERT;
		}
	};

	struct BoundCreateTableNode : BoundASTNode {
		std::string table_name;
		std::vector<Column> columns;

		BoundCreateTableNode(const std::string& table_name)
		    : table_name(table_name) {
			type = ASTNodeType::CREATE_TABLE;
		}

		void add_column(const Column& column) {
			columns.push_back(column);
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
