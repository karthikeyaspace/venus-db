// /src/parser/parser.cpp

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

#include "parser/ast.h"
#include "parser/parser.h"

namespace venus {
namespace parser {

	// Initialize keyword mapping
	std::unordered_map<std::string, TokenType> Parser::keywords = {
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
		{ "into", TokenType::INTO },
		{ "values", TokenType::VALUES },
		{ "from", TokenType::FROM },
		{ "where", TokenType::WHERE },
		{ "int", TokenType::INT_TYPE },
		{ "float", TokenType::FLOAT_TYPE },
		{ "char", TokenType::CHAR_TYPE },
		{ "primary_key", TokenType::PK }
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

			// Single character tokens
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

			// String literals (single quotes)
			if (c == '\'') {
				size_t start = ++i; // Skip opening quote
				while (i < query.length() && query[i] != '\'') {
					i++;
				}
				if (i >= query.length()) {
					throw std::runtime_error("Unterminated string literal");
				}
				std::string value = query.substr(start, i - start);
				result.emplace_back(TokenType::LITERAL, value);
				i++; // Skip closing quote
				continue;
			}

			// Numbers
			if (isDigit(c)) {
				size_t start = i;
				while (i < query.length() && (isDigit(query[i]) || query[i] == '.')) {
					i++;
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

		result.emplace_back(TokenType::END, "");
		return result;
	}

	std::unique_ptr<ASTNode> Parser::parse(const std::string& query) {
		tokens = tokenize(query);
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

			throw std::runtime_error("Not implemented error");
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
			consume(TokenType::STAR, "Only SELECT * supported for now");
			consume(TokenType::FROM, "Expected FROM after SELECT *");
			std::string table_name = currentToken().value;
			consume(TokenType::IDENTIFIER, "Expected table name after FROM");
			return std::make_unique<SelectNode>(table_name);
		}

		std::cout << "Not implemented" << std::endl;
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
