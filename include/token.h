/* libpasta -- an AST parser for Pascal
 * Copyright (C) 2024 Dani Rodríguez <dani@danirod.es>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

typedef enum tokentype {
	TOK_EOF,

	TOK_AND,
	TOK_ARRAY,
	TOK_ASSIGN,
	TOK_ASTERISK,
	TOK_AT,
	TOK_BEGIN,
	TOK_CARET,
	TOK_CASE,
	TOK_COLON,
	TOK_COMMA,
	TOK_CONST,
	TOK_CTRLCODE,
	TOK_DIGIT,
	TOK_DIV,
	TOK_DO,
	TOK_DOLLAR,
	TOK_DOT,
	TOK_DOTDOT,
	TOK_DOWNTO,
	TOK_ELSE,
	TOK_END,
	TOK_EQUAL,
	TOK_EXIT,
	TOK_FILE,
	TOK_FUNCTION,
	TOK_FOR,
	TOK_GOTO,
	TOK_GREATEQL,
	TOK_GREATER,
	TOK_IDENTIFIER,
	TOK_IF,
	TOK_IN,
	TOK_LBRACKET,
	TOK_LESSEQL,
	TOK_LESSER,
	TOK_LPAREN,
	TOK_MINUS,
	TOK_MOD,
	TOK_NEQUAL,
	TOK_NIL,
	TOK_NOT,
	TOK_OF,
	TOK_OR,
	TOK_PACKED,
	TOK_PLUS,
	TOK_PROCEDURE,
	TOK_PROGRAM,
	TOK_RBRACKET,
	TOK_RECORD,
	TOK_REPEAT,
	TOK_RPAREN,
	TOK_SEMICOLON,
	TOK_SET,
	TOK_SLASH,
	TOK_STRING,
	TOK_THEN,
	TOK_TO,
	TOK_TYPE,
	TOK_UNTIL,
	TOK_VAR,
	TOK_WHILE,
	TOK_WITH,
} tokentype_t;

typedef struct token {
	tokentype_t type;
	char *meta;
} token_t;

void token_free(token_t *tok);

const char *tokentype_string(tokentype_t token);

tokentype_t match_identifier(char *input);
