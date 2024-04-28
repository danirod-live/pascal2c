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

#include "scanner.h"
#include "token.h"

typedef enum expr_type {
	UNARY, // -5
	BINARY, // 2+3
	GROUPING, // wrapper
	LITERAL, // 4
} expr_type_t;

typedef struct expr {
	expr_type_t type;
	struct expr *exp_left, *exp_right;
	token_t *token;
	void *literal;
} expr_t;

expr_t *new_unary(token_t *t, expr_t *expr);
expr_t *new_binary(token_t *t, expr_t *left, expr_t *right);
expr_t *new_grouping(expr_t *exp);
expr_t *new_literal(token_t *lit);
void expr_free(expr_t *expr);

typedef struct parser {
	token_t **tokens;
	unsigned int len;
	unsigned int pos;
} parser_t;

parser_t *parser_new();
void parser_free(parser_t *parser);
void parser_load_tokens(parser_t *parser, scanner_t *scanner);
token_t *parser_peek(parser_t *parser);
token_t *parser_peek_far(parser_t *parser, unsigned int offt);
token_t *parser_token(parser_t *parser);
token_t *parser_token_expect(parser_t *, tokentype_t);
void __attribute__((noreturn))
parser_error(parser_t *parser, token_t *token, char *error);

expr_t *parser_identifier_list(parser_t *parser);

expr_t *parser_identifier(parser_t *parser);
expr_t *parser_unsigned_integer(parser_t *parser);
expr_t *parser_unsigned_number(parser_t *parser);
expr_t *parser_unsigned_constant(parser_t *parser);
expr_t *parser_constant(parser_t *parser);
expr_t *parser_simple_type(parser_t *parser);
expr_t *parser_type(parser_t *parser);
expr_t *parser_field_list(parser_t *parser);
expr_t *parser_variable(parser_t *parser);
expr_t *parser_expression(parser_t *parser);
expr_t *parser_simple_expression(parser_t *parser);
expr_t *parser_term(parser_t *parser);
expr_t *parser_factor(parser_t *parser);
expr_t *parser_parameter_list(parser_t *parser);
expr_t *parser_statement(parser_t *parser);
expr_t *parser_block(parser_t *parser);
expr_t *parser_program(parser_t *parser);
void dump_expr(expr_t *expr);

// Everything is wrong with this.

typedef enum expr2_type {
	EXP_IDENTIFIER,
	EXP_UNSIGNED_NUMBER,
	EXP_UNSIGNED_IDENTIFIER,
	EXP_VARIABLE,
	EXP_VARIABLE_PATH_CARET,
	EXP_VARIABLE_PATH_DOT,
	EXP_VARIABLE_PATH_ARRAY,
	EXP_UNSIGNED_CONSTANT,
	EXP_NIL,
	EXP_STRING,
	EXP_EXPRESSION,
} expr2_type_t;

struct expr2;

typedef struct expr_identifier {
	token_t *token;
} expr_identifier_t;

typedef struct expr_unsigned_number {
	token_t *token;
} expr_unsigned_number_t;

typedef struct expr_unsigned_integer {
	token_t *token;
} expr_unsigned_integer_t;

typedef struct expr_variable_path_dot {
	struct expr2 *identifier;
} expr_variable_path_dot_t;

typedef struct expr_variable {
	struct expr2 *identifier;
	struct expr2 **paths;
	unsigned int path_count;
} expr_variable_t;

typedef struct expr_unsigned_constant {
	struct expr2 *inner;
} expr_unsigned_constant_t;

typedef struct expr_string {
	token_t *token;
} expr_string_t;

typedef struct expr_expression {
	expr_t *exp;
} expr_expression_t;

typedef struct expr2 {
	expr2_type_t type;
	union {
		expr_identifier_t identifier;
		expr_unsigned_number_t unsigned_number;
		expr_unsigned_integer_t unsigned_integer;
		expr_variable_t variable;
		expr_variable_path_dot_t variable_path_dot;
		expr_unsigned_constant_t unsigned_constant;
		expr_string_t string;
		expr_expression_t expression;
	} payload;
} expr2_t;

expr2_t *expression_new(expr2_type_t type);

expr2_t *parser2_identifier(parser_t *parser);
expr2_t *parser2_unsigned_number(parser_t *parser);
expr2_t *parser2_unsigned_integer(parser_t *parser);
expr2_t *parser2_variable(parser_t *parser);
expr2_t *parser2_unsigned_constant(parser_t *parser);
expr2_t *parser2_expression(parser_t *parser);

#define E_IDENTIFIER(e) ((e).payload.identifier)
#define E_UNSIGNED_NUMBER(e) ((e).payload.unsigned_number)
#define E_UNSIGNED_INTEGER(e) ((e).payload.unsigned_integer)
#define E_VARIABLE(e) ((e).payload.variable)
#define E_VARIABLE_PATH_DOT(e) ((e).payload.variable_path_dot)
#define E_UNSIGNED_CONSTANT(e) ((e).payload.unsigned_constant)
#define E_STRING(e) ((e).payload.string)
#define E_EXPRESSION(e) ((e).payload.expression)
