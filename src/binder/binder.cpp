// /src/binder/binder.cpp

#include "binder/binder.h"

namespace venus {
namespace binder {

	std::unique_ptr<parser::ASTNode> Binder::Bind(std::unique_ptr<parser::ASTNode> ast) {
		if (catalog_ == nullptr) {
			throw std::runtime_error("Database is not initialized");
		}

		return ast;
	}

} // namespace binder
} // namespace venus
