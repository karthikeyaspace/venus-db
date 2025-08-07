// /src/parser/parser.h

/**
 * Parser for Venus DB
 *
 * A SQL parser handles the input of SQL query string, tokenizes it, and generates a parse tree
 * for further processing by the query planner and executor.
 *
 * A lexer is responsible for breaking down the input string into tokens, while the parser
 * takes these tokens and constructs a parse tree representing the syntactic structure of the query.
 * Then, binder will take the parse tree and resolve identifiers, types, and other semantic elements - ensuring consistency and integrity of db.
 * The ast is then used by the query planner to generate an execution plan.
 *
 * For venus db, Lexer, Parser, and Binder are implemented in Parser class.
 *
 * Some commands supported by venus db:
 * - CREATE/USE/DROP DATABASE
 * - CREATE TABLE
 * - INSERT INTO
 * - SELECT
 */

#pragma once

#include "common/config.h"
#include "common/types.h"
#include "parser/ast.h"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "ast.h"

namespace venus {
namespace parser {
	struct Token {
		TokenType type;
		std::string value;

		Token(TokenType t, const std::string& v)
		    : type(t)
		    , value(v) { }
	};

	class Parser {
	public:
		// Constructor
		Parser() = default;

		~Parser() {
			tokens.clear();
		}

		// Main parse method - returns AST
		std::unique_ptr<ASTNode> parse(const std::string& query);

	private:
		std::vector<Token> tokens;
		size_t current_token = 0;

		// Lexer
		std::vector<Token> tokenize(const std::string& query);
		bool isAlpha(char c);
		bool isDigit(char c);
		bool isAlphaNumeric(char c);

		Token& currentToken();
		Token& peekToken(size_t offset = 1);
		bool isAtEnd();
		bool check(TokenType type);
		bool match(TokenType type);
		Token advance();
		void consume(TokenType type, const std::string& message);

		DISALLOW_COPY_AND_MOVE(Parser);
	};
}
}