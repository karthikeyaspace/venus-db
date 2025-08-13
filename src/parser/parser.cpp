// /src/parser/parser.cpp

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

#include "parser/parser.h"

/**
 * Known parser issues to fix
 * - UNARY MINUS eg: -42
 * - float eg: .5
 * - identifier names eg: myVar - currently small casing all of them
 */

namespace venus {
namespace parser {

	std::unordered_map<std::string, TokenType> keywords = {
		{ "show", TokenType::SHOW },
		{ "create", TokenType::CREATE },
		{ "drop", TokenType::DROP },
		{ "use", TokenType::USE },
		{ "database", TokenType::DATABASE },
		{ "databases", TokenType::DATABASES },

		{ "table", TokenType::TABLE },
		{ "tables", TokenType::TABLES },
		{ "select", TokenType::SELECT },
		{ "insert", TokenType::INSERT },
		{ "update", TokenType::UPDATE },
		{ "delete", TokenType::DELETE },

		{ "from", TokenType::FROM },
		{ "into", TokenType::INTO },
		{ "values", TokenType::VALUES },
		{ "where", TokenType::WHERE },

		{ "primary_key", TokenType::PK },
		{ "join", TokenType::JOIN },
		{ "group_by", TokenType::GROUP_BY },
		{ "having", TokenType::HAVING },
		{ "order_by", TokenType::ORDER_BY },
		{ "as", TokenType::AS },
		{ "on", TokenType::ON },
		{ "limit", TokenType::LIMIT },
		{ "offset", TokenType::OFFSET },
		{ "set", TokenType::SET },
		{ "index", TokenType::INDEX },

		{ "int", TokenType::INT_TYPE },
		{ "float", TokenType::FLOAT_TYPE },
		{ "char", TokenType::CHAR_TYPE },

		{ "help", TokenType::HELP },
		{ "exit", TokenType::EXIT },
		{ "exec", TokenType::EXEC }
	};

	std::vector<Token> Parser::tokenize(const std::string& query) {
		std::vector<Token> result;
		size_t i = 0;

		while (i < query.length()) {
			char c = query[i];

			if (std::isspace(c)) {
				i++;
				continue;
			}

			if (c == '(') {
				result.emplace_back(TokenType::LPAREN, "(");
				i++;
				continue;
			}
			if (c == ')') {
				result.emplace_back(TokenType::RPAREN, ")");
				i++;
				continue;
			}
			if (c == ',') {
				result.emplace_back(TokenType::COMMA, ",");
				i++;
				continue;
			}
			if (c == '*') {
				result.emplace_back(TokenType::ASTERISK, "*");
				i++;
				continue;
			}
			if (c == ';') {
				result.emplace_back(TokenType::SEMICOLON, ";");
				i++;
				continue;
			}
			if (c == '=') {
				result.emplace_back(TokenType::EQUALS, "=");
				i++;
				continue;
			}
			if (c == '<') {
				result.emplace_back(TokenType::LESS_THAN, "<");
				i++;
				continue;
			}
			if (c == '>') {
				result.emplace_back(TokenType::GREATER_THAN, ">");
				i++;
				continue;
			}
			if (c == '+') {
				result.emplace_back(TokenType::PLUS, "+");
				i++;
				continue;
			}
			if (c == '-') {
				result.emplace_back(TokenType::MINUS, "-");
				i++;
				continue;
			}
			if (c == '/') {
				result.emplace_back(TokenType::DIVIDE, "/");
				i++;
				continue;
			}
			if (c == '.') {
				result.emplace_back(TokenType::DOT, ".");
				i++;
				continue;
			}

			// String literals (in single quotes - char)
			if (c == '\'') {
				size_t start = ++i;
				while (i < query.length() && query[i] != '\'') {
					i++;
				}
				if (i >= query.length()) {
					throw std::runtime_error("Parser error: Unterminated string literal");
				}
				std::string value = query.substr(start, i - start);
				result.emplace_back(TokenType::LITERAL, value);
				i++;
				continue;
			}

			// Number literals (integers and floats)
			if (isDigit(c)) {
				size_t start = i;
				bool has_dot = false;

				while (i < query.length()) {
					if (isDigit(query[i])) {
						i++;
					} else if (query[i] == '.' && !has_dot) {
						if (i + 1 < query.length() && isDigit(query[i + 1])) {
							has_dot = true;
							i++;
						} else {
							// not float
							break;
						}
					} else {
						break;
					}
				}

				std::string value = query.substr(start, i - start);
				result.emplace_back(TokenType::LITERAL, value);
				continue;
			}

			// Identifiers and keywords
			if (isAlpha(c)) {
				size_t start = i;
				while (i < query.length() && isAlphaNumeric(query[i])) {
					i++;
				}
				std::string value = query.substr(start, i - start);

				// Convert to lowercase for keyword matching
				std::string lower_value = value;
				std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);

				auto keyword_it = keywords.find(lower_value);
				if (keyword_it != keywords.end()) {
					result.emplace_back(keyword_it->second, value);
				} else {
					result.emplace_back(TokenType::IDENTIFIER, value);
				}
				continue;
			}

			throw std::runtime_error("Parser error: Unexpected character: " + std::string(1, c));
		}

		result.emplace_back(TokenType::END, "END");
		return result;
	}

	// Return AST
	std::unique_ptr<ASTNode> Parser::Parse(const std::string& query) {

		// Reference for SQL grammer - https://forcedotcom.github.io/phoenix

		tokens = tokenize(query);

		current_token = 0;

		if (tokens.empty() || (tokens.size() == 1 && tokens[0].type == TokenType::END)) {
			throw std::runtime_error("Parser error: Query is empty!");
		}

		// walk through the tokens, check grammer, create ast
		Token& first_token = currentToken();

		switch (first_token.type) {
		case TokenType::SHOW: {
			advance();
			if (check(TokenType::DATABASES)) {
				advance();
				auto root = std::make_unique<ASTNode>(ASTNodeType::SHOW_DATABASES);
				return root;
			} else if (check(TokenType::TABLES)) {
				advance();
				auto root = std::make_unique<ASTNode>(ASTNodeType::SHOW_TABLES);
				return root;
			} else {
				invalidToken("Expected keywords DATABASE or TABLE");
			}
		}

		case TokenType::USE: {
			advance();
			if (check(TokenType::IDENTIFIER)) {
				std::string db_name = advance().value;
				auto root = std::make_unique<ASTNode>(ASTNodeType::USE_DATABASE, db_name);
				return root;
			} else {
				invalidToken("Expected database name after USE");
			}
		}

		case TokenType::DROP: {
			advance();
			if (check(TokenType::DATABASE)) {
				advance();
				if (check(TokenType::IDENTIFIER)) {
					std::string db_name = advance().value;
					auto root = std::make_unique<ASTNode>(ASTNodeType::DROP_DATABASE, db_name);
					return root;
				} else {
					invalidToken("Expected database name after DROP DATABASE");
				}
			} else if (check(TokenType::TABLE)) {
				advance();
				if (check(TokenType::IDENTIFIER)) {
					std::string table_name = advance().value;
					auto root = std::make_unique<ASTNode>(ASTNodeType::DROP_TABLE, table_name);
					return root;
				} else {
					invalidToken("Expected table name after DROP TABLE");
				}
			} else {
				invalidToken("Expected TABLE or DATABASE after DROP");
			}
		}

		case TokenType::CREATE: {
			// CREATE TABLE <table_name> (col_name col_type constraint)
			advance();
			if (check(TokenType::DATABASE)) {
				advance();
				if (check(TokenType::IDENTIFIER)) {
					std::string db_name = advance().value;
					auto root = std::make_unique<ASTNode>(ASTNodeType::CREATE_DATABASE, db_name);
					return root;
				} else {
					invalidToken("Expected database name after CREATE DATABASE");
				}
			} else if (check(TokenType::TABLE)) {
				advance();
				if (check(TokenType::IDENTIFIER)) {
					std::string table_name = advance().value;
					auto root = std::make_unique<ASTNode>(ASTNodeType::CREATE_TABLE, table_name);

					if (check(TokenType::LPAREN)) {
						advance();
						while (!isAtEnd() && !check(TokenType::RPAREN)) {
							if (check(TokenType::IDENTIFIER)) {
								std::string col_name = advance().value;

								if (check(TokenType::INT_TYPE) || check(TokenType::FLOAT_TYPE) || check(TokenType::CHAR_TYPE)) {
									std::string col_type = advance().value;

									bool is_primary_key = false;
									if (check(TokenType::PK)) {
										advance();
										is_primary_key = true;
									}

									if (is_primary_key) {
										root->children.emplace_back(std::make_unique<ASTNode>(ASTNodeType::COLUMN_DEF, col_name + " " + col_type + " PK"));
									} else {
										root->children.emplace_back(std::make_unique<ASTNode>(ASTNodeType::COLUMN_DEF, col_name + " " + col_type));
									}
								} else {
									invalidToken("Expected column type after column name");
								}
							}
							if (check(TokenType::COMMA)) {
								advance();
							}
						}
						consume(TokenType::RPAREN, "Expected ')' after column definitions");
					} else {
						invalidToken("Expected '(' after table name");
					}

					return root;
				} else {
					invalidToken("Expected table name after CREATE TABLE");
				}
			} else {
				invalidToken("Expected TABLE or DATABASE after CREATE");
			}
		}

		case TokenType::SELECT: {
			advance();
			// SELECT * FROM <table_name>
			// SELECT <column_name>, <column_name> FROM <table_name>

			if (check(TokenType::ASTERISK)) {
				advance();
				if (check(TokenType::FROM)) {
					advance();
					if (check(TokenType::IDENTIFIER)) {
						std::string table_name = advance().value;
						auto root = std::make_unique<ASTNode>(ASTNodeType::SELECT);
						auto projection_list = std::make_shared<ASTNode>(ASTNodeType::PROJECTION_LIST);
						projection_list->add_child(std::make_shared<ASTNode>(ASTNodeType::COLUMN_REF, "*"));
						root->add_child(projection_list);
						root->add_child(std::make_shared<ASTNode>(ASTNodeType::TABLE_REF, table_name));
						return root;
					} else {
						invalidToken("Expected table name after FROM");
					}
				} else {
					invalidToken("Expected FROM after SELECT *");
				}
			} else if (check(TokenType::IDENTIFIER)) {
				std::vector<std::string> columns;
				while (check(TokenType::IDENTIFIER)) {
					columns.push_back(advance().value);
					if (check(TokenType::COMMA)) {
						advance();
					} else {
						break;
					}
				}

				if (check(TokenType::FROM)) {
					advance();
					if (check(TokenType::IDENTIFIER)) {
						std::string table_name = advance().value;
						auto root = std::make_unique<ASTNode>(ASTNodeType::SELECT);
						auto projection_list = std::make_shared<ASTNode>(ASTNodeType::PROJECTION_LIST);
						for (const auto& col : columns) {
							projection_list->add_child(std::make_shared<ASTNode>(ASTNodeType::COLUMN_REF, col));
						}
						root->add_child(projection_list);
						root->add_child(std::make_shared<ASTNode>(ASTNodeType::TABLE_REF, table_name));
						return root;
					} else {
						invalidToken("Expected table exe after FROM");
					}
				} else {
					invalidToken("Expected FROM after column list");
				}
			} else {
				invalidToken("Expected '*' or column names after SELECT");
			}
		}

		case TokenType::INSERT: {
			// INSERT INTO <table_name> VALUES (val1, val2, val3, ...)
			advance();
			if (check(TokenType::INTO)) {
				advance();
				if (check(TokenType::IDENTIFIER)) {
					std::string table_name = advance().value;
					auto root = std::make_unique<ASTNode>(ASTNodeType::INSERT, table_name);

					if (check(TokenType::VALUES)) {
						advance();
						if (check(TokenType::LPAREN)) {
							advance();
							while (!isAtEnd() && !check(TokenType::RPAREN)) {
								if (check(TokenType::LITERAL)) {
									std::string value = advance().value;
									root->add_child(std::make_shared<ASTNode>(ASTNodeType::CONST_VALUE, value));
								} else {
									invalidToken("Expected a literal in VALUES");
								}
								if (check(TokenType::COMMA)) {
									advance();
								}
							}
							consume(TokenType::RPAREN, "Expected ')' after VALUES");
						} else {
							invalidToken("Expected '(' after VALUES");
						}
					} else {
						invalidToken("Expected VALUES after table name");
					}

					return root;
				} else {
					invalidToken("Expected table name after INTO");
				}
			} else {
				invalidToken("Expected INTO after INSERT");
			}
		}

		case TokenType::EXIT: {
			advance();
			if (check(TokenType::SEMICOLON)) {
				advance();
			}
			return std::make_unique<ASTNode>(ASTNodeType::EXIT);
		}

		default: {
			std::cout << "Not implemented: " << currentToken().value << std::endl;
			return std::make_unique<ASTNode>(ASTNodeType::INVALID_NODE);
		}
		}
	}

	bool Parser::isAlpha(char c) {
		return std::isalpha(c) || c == '_';
	}

	bool Parser::isDigit(char c) {
		return std::isdigit(c);
	}

	bool Parser::isAlphaNumeric(char c) {
		return isAlpha(c) || isDigit(c);
	}

	Token& Parser::currentToken() {
		if (current_token >= tokens.size()) {
			throw std::runtime_error("Parser error: Unexpected end of input");
		}
		return tokens[current_token];
	}

	Token& Parser::peekToken(size_t offset) {
		size_t index = current_token + offset;
		if (index >= tokens.size()) {
			return tokens.back();
		}
		return tokens[index];
	}

	bool Parser::isAtEnd() {
		return current_token >= tokens.size() || currentToken().type == TokenType::END;
	}

	bool Parser::check(TokenType type) {
		if (isAtEnd())
			return false;
		return currentToken().type == type;
	}

	bool Parser::match(TokenType type) {
		if (check(type)) {
			advance();
			return true;
		}
		return false;
	}

	Token Parser::advance() {
		if (!isAtEnd())
			current_token++;
		return tokens[current_token - 1];
	}

	// check is current token is same as expected token type
	void Parser::consume(TokenType type, const std::string& message) {
		if (check(type)) {
			advance();
			return;
		}
		throw std::runtime_error("Parser error: " + message + ". Got: " + currentToken().value);
	}

	void Parser::invalidToken(const std::string& msg) {
		throw std::runtime_error("Parser error: Invalid Token '" + currentToken().value + "'\n" + msg);
	}

} // namespace parser
} // namespace venus
