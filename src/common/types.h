// src/common/types.h

#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "catalog/schema.h"
#include "common/config.h"

namespace venus {

enum class PageType : uint8_t {
	INVALID_PAGE = 0,
	TABLE_PAGE, // regular data page
	INDEX_LEAF_PAGE,
	INDEX_INTERNAL_PAGE,
};

struct TableRef {
	table_id_t table_id;
	page_id_t first_page_id;
	std::string table_name;
	Schema* schema;

	TableRef(table_id_t id_, page_id_t first_page_id_, const std::string& name_, Schema* schema_)
	    : table_id(id_)
	    , first_page_id(first_page_id_)
	    , table_name(name_)
	    , schema(schema_) { }

	const Column& GetColumnByName(const std::string& name) const {
		return schema->GetColumn(name);
	}

	const Column& GetColumnByIndex(size_t index) const {
		return schema->GetColumn(index);
	}

	const table_id_t GetTableId() const {
		return table_id;
	}

	const page_id_t GetFirstPageId() const {
		return first_page_id;
	}

	const Schema* GetSchema() const {
		return schema;
	}
};

struct ColumnRef {
	column_id_t col_id;
	Column* column_entry_;

	const std::string& GetName() const {
		return column_entry_->GetName();
	}
};

struct ConstantType {
	std::string value;
	ColumnType type;
};

struct Expression {
	// for now this struct only supports simple equality (=, <, >, <=, >=)
	ColumnRef left;
	std::string op;
	ConstantType right;
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
	DATABASES,

	TABLE,
	TABLES,

	SELECT,
	INSERT,
	UPDATE,
	DELETE,

	INTO,
	VALUES,
	FROM,
	PRIMARY,
	KEY,

	IDENTIFIER, // table or column name

	WHERE,
	ASTERISK,
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
	SHOW_TABLES,

	SELECT,
	INSERT,
	INSERT_MANY,
	UPDATE,
	DELETE,

	TABLE_REF, // planets
	COLUMN_REF, // planets.id
	COLUMN_DEF, // id INT PRIMARY_KEY
	CONST_VALUE, // 420, '69', 420.69

	PROJECTION_LIST, // planets.id, planets.name -> pi in relational model
	CONDITION, // planets.id = 420
	WHERE_CLAUSE,
	FROM_CLAUSE,
	JOIN_CLAUSE,
	GROUP_BY_CLAUSE,
	HAVING_CLAUSE,
	ORDER_BY_CLAUSE,
	LIMIT_CLAUSE,
	ASSIGNMENT,

	EXIT,
	EXEC
};

enum class PlanNodeType : uint8_t {
	INVALID_PLAN = 0,

	SEQ_SCAN,
	INDEX_SCAN,

	PROJECTION,
	FILTER,

	NESTED_LOOP_JOIN,

	AGGREGATION,
	SORT,
	LIMIT,

	// DML
	INSERT,
	UPDATE,
	DELETE,

	// DDL
	CREATE_DATABASE,
	DROP_DATABASE,
	USE_DATABASE,
	CREATE_TABLE,
	DROP_TABLE,
	CREATE_INDEX,
	DROP_INDEX,

	SHOW_DATABASES,
	SHOW_TABLES,
	HELP,
	EXIT,
	EXEC_FILE
};

}