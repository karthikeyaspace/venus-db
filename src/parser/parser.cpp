// /src/parser/parser.cpp

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

#include "parser/parser.h"

namespace venus {
namespace parser {

	std::unordered_map<std::string, TokenType> keywords = {
		{ "show", TokenType::SHOW },
		{ "create", TokenType::CREATE },
		{ "drop", TokenType::DROP },
		{ "use", TokenType::USE },
		{ "database", TokenType::DATABASE },

		{ "table", TokenType::TABLE },
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
				result.emplace_back(TokenType::STAR, "*");
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
					throw std::runtime_error("Unterminated string literal");
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

			throw std::runtime_error("Unexpected character: " + std::string(1, c));
		}

		result.emplace_back(TokenType::END, "END");
		return result;
	}

	std::unique_ptr<ASTNode> Parser::parse(const std::string& query) {
		tokens = tokenize(query);

		// Debug: console all tokens
		std::cout << "=== TOKENIZATION RESULT ===" << std::endl;
		for (const auto& token : tokens) {
			std::cout << "Token Type: " << static_cast<int>(token.type)
			          << ", Value: '" << token.value << "'" << std::endl;
		}
		std::cout << "===========================" << std::endl;

		current_token = 0;

		if (tokens.empty() || (tokens.size() == 1 && tokens[0].type == TokenType::END)) {
			throw std::runtime_error("Empty query");
		}

		if (match(TokenType::CREATE)) {
			if (match(TokenType::DATABASE)) {
				std::string db_name = currentToken().value;
				consume(TokenType::IDENTIFIER, "Expected database name");
				return std::make_unique<CreateDatabaseNode>(db_name);
			}

			throw std::runtime_error("Only CREATE DATABASE supported currently");
		}

		if (match(TokenType::USE)) {
			std::string db_name = currentToken().value;
			consume(TokenType::IDENTIFIER, "Expected database name after USE");
			return std::make_unique<UseDatabaseNode>(db_name);
		}

		if (match(TokenType::SHOW)) {
			if (match(TokenType::DATABASE)) {
				return std::make_unique<ShowDatabasesNode>();
			}
			throw std::runtime_error("Only SHOW DATABASES supported currently");
		}

		if (match(TokenType::DROP)) {
			if (match(TokenType::DATABASE)) {
				std::string db_name = currentToken().value;
				consume(TokenType::IDENTIFIER, "Expected database name after DROP DATABASE");
				return std::make_unique<DropDatabaseNode>(db_name);
			}
			throw std::runtime_error("Only DROP DATABASE supported currently");
		}

		if (match(TokenType::SELECT)) {
			// For now, just parse simple SELECT queries
			// This will need to be expanded to handle JOINs properly

			// Skip column list for now (assume SELECT *)
			while (!check(TokenType::FROM) && !isAtEnd()) {
				advance();
			}

			consume(TokenType::FROM, "Expected FROM after SELECT");
			std::string table_name = currentToken().value;
			consume(TokenType::IDENTIFIER, "Expected table name after FROM");

			// Skip the rest for now (including JOIN clauses)
			return std::make_unique<SelectNode>(table_name);
		}

		throw std::runtime_error("Unknown statement type");
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
			throw std::runtime_error("Unexpected end of input");
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

	void Parser::consume(TokenType type, const std::string& message) {
		if (check(type)) {
			advance();
			return;
		}
		throw std::runtime_error(message + ". Got: " + currentToken().value);
	}
} // namespace parser
} // namespace venus
