// /src/binder/binder/h

/**
 * Binder
 *
 * Binder takes in the parsed AST from parser and binds the identifiers to their corresponding
 * symbols in the database schema.
 *
 * Resolves names and types
 * Adds metadata to AST to create BoundAstNode,
 * 		so further steps need not refetch from catalog
 * Turns tables names to reference to an actual table object in the catalog
 * Turns column names like "rad" to "column #3 in planets, type=FLOAT"
 * Resolves permissions (if any, not for venus)
 * Makes the AST ready for logical planning
 * Has complete access to the catalog
 */

#pragma once

#include "catalog/catalog.h"
#include "parser/ast.h"
#include "common/types.h"

namespace venus {
namespace binder {
	class Binder {
	public:
		void SetContext(catalog::CatalogManager* catalog) {
			catalog_ = catalog;
		}

		std::unique_ptr<parser::BoundASTNode> Bind(std::unique_ptr<parser::ASTNode> ast);

	private:
		catalog::CatalogManager* catalog_ = nullptr;
	};
}
}