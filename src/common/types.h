// src/common/types.h

#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace venus {
enum class PageType : uint8_t {
	INVALID_PAGE = 0,
	TABLE_PAGE, // regular data page
	INDEX_LEAF_PAGE,
	INDEX_INTERNAL_PAGE,
};

enum class ColumnType : uint8_t {
	INVALID_COLUMN = 0,
	INT,
	FLOAT,
	CHAR
};

// Lexer token types
enum class TokenType : uint8_t {
	INVALID_TOKEN = 0,

	// keywords
	SHOW,
	CREATE,
	DROP,
	USE,
	DATABASE,

	TABLE,

	SELECT,
	INSERT,
	UPDATE,
	DELETE,

	INTO,
	VALUES,
	FROM,
	PK,

	IDENTIFIER, // table or column name

	WHERE,
	STAR,
	JOIN,
	GROUP_BY,
	HAVING,
	ORDER_BY,
	AS,
	ON,
	LIMIT,
	OFFSET,
	SET,
	INDEX,

	// operators
	PLUS,
	MINUS,
	MULTIPLY,
	DIVIDE,
	EQUALS,
	NOT_EQUALS,
	GREATER_THAN,
	LESS_THAN,

	// types
	INT_TYPE,
	FLOAT_TYPE,
	CHAR_TYPE,

	LITERAL, // string or number values

	// delimiters
	COMMA,
	SEMICOLON,
	LPAREN,
	RPAREN,
	DOT,
	END,

	HELP,
	EXIT,
	EXEC
};

// AST types
enum class ASTNodeType : uint8_t {
	INVALID_NODE = 0,

	SHOW_DATABASES,
	CREATE_DATABASE,
	DROP_DATABASE,
	USE_DATABASE,

	CREATE_TABLE,
	DROP_TABLE,

	SELECT,
	INSERT,
	UPDATE,
	DELETE,

	WHERE_CLAUSE,
	JOIN_CLAUSE,
	GROUP_BY_CLAUSE,
	HAVING_CLAUSE,
	ORDER_BY_CLAUSE
};

}