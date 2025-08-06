#pragma once

#include <memory>
#include <string>
#include <vector>

#include <common/types.h>

namespace venus {
namespace parser {

	struct ASTNode {
		ASTNodeType type;

		ASTNode(ASTNodeType t)
		    : type(t) { }
		virtual ~ASTNode() = default;
	};

	struct CreateDatabaseNode : public ASTNode {
		std::string db_name;
		CreateDatabaseNode(const std::string& name)
		    : ASTNode(ASTNodeType::CREATE_DATABASE)
		    , db_name(name) {
		}
	};

	struct UseDatabaseNode : public ASTNode {
		std::string db_name;
		UseDatabaseNode(const std::string& name)
		    : ASTNode(ASTNodeType::USE_DATABASE)
		    , db_name(name) {
		}
	};

	struct ShowDatabasesNode : public ASTNode {
		ShowDatabasesNode()
		    : ASTNode(ASTNodeType::SHOW_DATABASES) {
		}
	};

	struct DropDatabaseNode : public ASTNode {
		std::string db_name;
		DropDatabaseNode(const std::string& name)
		    : ASTNode(ASTNodeType::DROP_DATABASE)
		    , db_name(name) {
		}
	};

	struct SelectNode : public ASTNode {
		std::string table_name;
		SelectNode(const std::string& name)
		    : ASTNode(ASTNodeType::SELECT)
		    , table_name(name) {
		}
	};

} // namespace parser
} // namespace venus
