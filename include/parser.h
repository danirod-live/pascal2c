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
	EXP_UNSIGNED_INTEGER,
	EXP_VARIABLE,
	EXP_VARIABLE_PATH_CARET,
	EXP_VARIABLE_PATH_DOT,
	EXP_VARIABLE_PATH_ARRAY,
	EXP_UNSIGNED_CONSTANT,
	EXP_CONSTANT,
	EXP_NIL,
	EXP_STRING,
	EXP_EXPRESSION,
	EXP_SIMPLE_TYPE,
	EXP_REFERENCE_TYPE,
	EXP_SET_TYPE,
	EXP_ARRAY_TYPE,
	EXP_RECORD_TYPE,
	EXP_FILE_TYPE,
	EXP_PLIST_CHUNK,
	EXP_PLIST,
	EXP_FIELD_LIST,
	EXP_FIELD_LIST_ROW,
	EXP_FIELD_LIST_CASE,
	EXP_FIELD_LIST_CASE_ROW,
	EXP_STMT_LABEL,
	EXP_STMT_ASSIGN,
	EXP_STMT_FUNCTION_CALL,
	EXP_STMT_BEGIN,
	EXP_STMT_IF,
	EXP_STMT_REPEAT,
	EXP_STMT_WHILE,
	EXP_STMT_FOR,
	EXP_STMT_CASE_BRANCH,
	EXP_STMT_CASE,
	EXP_STMT_WITH,
	EXP_STMT_GOTO,
	EXP_STMT_EXIT,
	EXP_BLOCK,
	EXP_BLOCK_LABEL,
	EXP_BLOCK_CONST,
	EXP_BLOCK_CONST_LINE,
	EXP_BLOCK_TYPE,
	EXP_BLOCK_TYPE_LINE,
	EXP_BLOCK_VAR,
	EXP_BLOCK_VAR_LINE,
	EXP_BLOCK_FUNCTION,
	EXP_BLOCK_PROCEDURE,
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

typedef struct expr_variable_path_array {
	struct expr2 **expressions;
	unsigned int expression_count;
} expr_variable_path_array_t;

typedef struct expr_variable {
	struct expr2 *identifier;
	struct expr2 **paths;
	unsigned int path_count;
} expr_variable_t;

typedef struct expr_unsigned_constant {
	struct expr2 *inner;
} expr_unsigned_constant_t;

typedef struct expr_constant {
	struct expr2 *inner;
	char sign;
} expr_constant_t;

typedef struct expr_string {
	token_t *token;
} expr_string_t;

typedef struct expr_expression {
	expr_t *exp;
} expr_expression_t;

typedef enum expr_simple_type_kind {
	SIMPLE_TYPE_SINGLE,
	SIMPLE_TYPE_RANGE,
	SIMPLE_TYPE_LIST,
} expr_simple_type_kind_t;

typedef struct expr_simple_type {
	struct expr2 **components;
	unsigned int component_count;
	expr_simple_type_kind_t type;
} expr_simple_type_t;

typedef struct expr_reference_type {
	struct expr2 *identifier;
} expr_reference_type_t;

typedef struct expr_set_type {
	struct expr2 *simple_type;
	int packed;
} expr_set_type_t;

typedef struct expr_record_type {
	struct expr2 *fields;
	int packed;
} expr_record_type_t;

typedef struct expr_array_type {
	struct expr2 **inner_types;
	int inner_type_count;
	struct expr2 *type;
	int packed;
} expr_array_type_t;

typedef struct expr_file_type {
	struct expr2 *type;
	int packed;
} expr_file_type_t;

typedef struct expr_plist_chunk {
	struct expr2 **identifiers;
	unsigned int identifier_count;
	struct expr2 *data_type;
	int is_reference;
} expr_plist_chunk_t;

typedef struct expr_plist {
	struct expr2 **chunks;
	unsigned int chunks_count;
} expr_plist_t;

typedef struct expr_field_list {
	struct expr2 **rows;
	unsigned int row_count;
	struct expr2 *case_stmt;
} expr_field_list_t;

typedef struct expr_field_list_row {
	struct expr2 **identifiers;
	unsigned int identifier_count;
	struct expr2 *data_type;
} expr_field_list_row_t;

typedef struct expr_field_list_case {
	struct expr2 *identifier;
	struct expr2 *type;
	struct expr2 **rows;
	unsigned int rows_count;
} expr_field_list_case_t;

typedef struct expr_field_list_case_row {
	struct expr2 **constants;
	unsigned int constant_count;
	struct expr2 *field_list;
} expr_field_list_case_row_t;

typedef struct expr_stmt_label {
	struct expr2 *label;
	struct expr2 *nested;
} expr_stmt_label_t;

typedef struct expr_stmt_assign {
	struct expr2 *identifier;
	struct expr2 *expression;
} expr_stmt_assign_t;

typedef struct expr_stmt_function_call {
	struct expr2 *identifier;
	struct expr2 **params;
	unsigned int param_count;
} expr_stmt_function_call_t;

typedef struct expr_stmt_begin {
	struct expr2 **statements;
	unsigned int statement_count;
} expr_stmt_begin_t;

typedef struct expr_stmt_if {
	struct expr2 *condition;
	struct expr2 *then;
	struct expr2 *else_;
} expr_stmt_if_t;

typedef struct expr_stmt_repeat {
	struct expr2 **statements;
	unsigned int statement_count;
	struct expr2 *condition;
} expr_stmt_repeat_t;

typedef struct expr_stmt_while {
	struct expr2 *condition;
	struct expr2 *statement;
} expr_stmt_while_t;

typedef struct expr_stmt_for {
	struct expr2 *identifier;
	struct expr2 *start;
	struct expr2 *end;
	int direction; // 1 for UP, -1 for DOWN
	struct expr2 *statement;
} expr_stmt_for_t;

typedef struct expr_stmt_case_branch {
	struct expr2 **constants;
	int constant_count;
	struct expr2 *statement;
} expr_stmt_case_branch_t;

typedef struct expr_stmt_case {
	struct expr2 *expression;
	struct expr2 **cases;
	int case_count;
} expr_stmt_case_t;

typedef struct expr_stmt_with {
	struct expr2 **variables;
	int variable_count;
	struct expr2 *statement;
} expr_stmt_with_t;

typedef struct expr_stmt_goto {
	struct expr2 *identifier;
} expr_stmt_goto_t;

typedef struct expr_stmt_exit {
	int program; // boolean
	struct expr2 *identifier;
} expr_stmt_exit_t;

typedef struct expr_block {
	struct expr2 **preblocks;
	int preblock_count;
	struct expr2 **statements;
	int statement_count;
} expr_block_t;

typedef struct expr_block_label {
	struct expr2 **labels;
	int label_count;
} expr_block_label_t;

typedef struct expr_block_const {
	struct expr2 **lines;
	int line_count;
} expr_block_const_t;

typedef struct expr_block_const_line {
	struct expr2 *name;
	struct expr2 *value;
} expr_block_const_line_t;

typedef struct expr_block_type {
	struct expr2 **lines;
	int line_count;
} expr_block_type_t;

typedef struct expr_block_type_line {
	struct expr2 *name;
	struct expr2 *value;
} expr_block_type_line_t;

typedef struct expr_block_var {
	struct expr2 **lines;
	int line_count;
} expr_block_var_t;

typedef struct expr_block_var_line {
	struct expr2 **identifiers;
	int identifier_count;
	struct expr2 *type;
} expr_block_var_line_t;

typedef struct expr_block_procedure {
	struct expr2 *identifier;
	struct expr2 *parlist;
	struct expr2 *block;
} expr_block_procedure_t;

typedef struct expr_block_function {
	struct expr2 *identifier;
	struct expr2 *parlist;
	struct expr2 *returntype;
	struct expr2 *block;
} expr_block_function_t;

typedef struct expr2 {
	expr2_type_t type;
	union {
		expr_identifier_t identifier;
		expr_unsigned_number_t unsigned_number;
		expr_unsigned_integer_t unsigned_integer;
		expr_variable_t variable;
		expr_variable_path_dot_t variable_path_dot;
		expr_variable_path_array_t variable_path_array;
		expr_unsigned_constant_t unsigned_constant;
		expr_constant_t constant;
		expr_string_t string;
		expr_expression_t expression;
		expr_simple_type_t simple_type;
		expr_reference_type_t reference_type;
		expr_set_type_t set_type;
		expr_array_type_t array_type;
		expr_record_type_t record_type;
		expr_file_type_t file_type;
		expr_plist_chunk_t plist_chunk;
		expr_plist_t plist;
		expr_field_list_t field_list;
		expr_field_list_row_t field_list_row;
		expr_field_list_case_t field_list_case;
		expr_field_list_case_row_t field_list_case_row;
		expr_stmt_label_t stmt_label;
		expr_stmt_assign_t stmt_assign;
		expr_stmt_function_call_t stmt_function_call;
		expr_stmt_begin_t stmt_begin;
		expr_stmt_if_t stmt_if;
		expr_stmt_repeat_t stmt_repeat;
		expr_stmt_while_t stmt_while;
		expr_stmt_for_t stmt_for;
		expr_stmt_case_t stmt_case;
		expr_stmt_case_branch_t stmt_case_branch;
		expr_stmt_with_t stmt_with;
		expr_stmt_goto_t stmt_goto;
		expr_stmt_exit_t stmt_exit;
		expr_block_t block;
		expr_block_label_t block_label;
		expr_block_const_t block_const;
		expr_block_const_line_t block_const_line;
		expr_block_type_t block_type;
		expr_block_type_line_t block_type_line;
		expr_block_var_t block_var;
		expr_block_var_line_t block_var_line;
		expr_block_procedure_t block_procedure;
		expr_block_function_t block_function;
	} payload;
} expr2_t;

expr2_t *expression_new(expr2_type_t type);

expr2_t *parser2_identifier(parser_t *parser);
expr2_t *parser2_unsigned_number(parser_t *parser);
expr2_t *parser2_unsigned_integer(parser_t *parser);
expr2_t *parser2_variable(parser_t *parser);
expr2_t *parser2_unsigned_constant(parser_t *parser);
expr2_t *parser2_expression(parser_t *parser);
expr2_t *parser2_constant(parser_t *parser);
expr2_t *parser2_simple_type(parser_t *parser);
expr2_t *parser2_type(parser_t *parser);
expr2_t *parser2_field_list(parser_t *parser);
expr2_t *parser2_parameter_list(parser_t *parser);
expr2_t *parser2_statement(parser_t *parser);
expr2_t *parser2_block(parser_t *parser);

#define E_IDENTIFIER(e) ((e).payload.identifier)
#define E_UNSIGNED_NUMBER(e) ((e).payload.unsigned_number)
#define E_UNSIGNED_INTEGER(e) ((e).payload.unsigned_integer)
#define E_VARIABLE(e) ((e).payload.variable)
#define E_VARIABLE_PATH_DOT(e) ((e).payload.variable_path_dot)
#define E_VARIABLE_PATH_ARRAY(e) ((e).payload.variable_path_array)
#define E_UNSIGNED_CONSTANT(e) ((e).payload.unsigned_constant)
#define E_CONSTANT(e) ((e).payload.constant)
#define E_STRING(e) ((e).payload.string)
#define E_EXPRESSION(e) ((e).payload.expression)
#define E_SIMPLE_TYPE(e) ((e).payload.simple_type)
#define E_REFERENCE_TYPE(e) ((e).payload.reference_type)
#define E_SET_TYPE(e) ((e).payload.set_type)
#define E_ARRAY_TYPE(e) ((e).payload.array_type)
#define E_RECORD_TYPE(e) ((e).payload.record_type)
#define E_FILE_TYPE(e) ((e).payload.file_type)
#define E_PLIST_CHUNK(e) ((e).payload.plist_chunk)
#define E_PLIST(e) ((e).payload.plist)
#define E_FIELD_LIST(e) ((e).payload.field_list)
#define E_FIELD_LIST_ROW(e) ((e).payload.field_list_row)
#define E_FIELD_LIST_CASE(e) ((e).payload.field_list_case)
#define E_FIELD_LIST_CASE_ROW(e) ((e).payload.field_list_case_row)
#define E_STMT_LABEL(e) ((e).payload.stmt_label)
#define E_STMT_ASSIGNMENT(e) ((e).payload.stmt_assign)
#define E_STMT_FUNCTION_CALL(e) ((e).payload.stmt_function_call)
#define E_STMT_BEGIN(e) ((e).payload.stmt_begin)
#define E_STMT_IF(e) ((e).payload.stmt_if)
#define E_STMT_REPEAT(e) ((e).payload.stmt_repeat)
#define E_STMT_WHILE(e) ((e).payload.stmt_while)
#define E_STMT_FOR(e) ((e).payload.stmt_for)
#define E_STMT_CASE_BRANCH(e) ((e).payload.stmt_case_branch)
#define E_STMT_CASE(e) ((e).payload.stmt_case)
#define E_STMT_WITH(e) ((e).payload.stmt_with)
#define E_STMT_GOTO(e) ((e).payload.stmt_goto)
#define E_STMT_EXIT(e) ((e).payload.stmt_exit)
#define E_BLOCK(e) ((e).payload.block)
#define E_BLOCK_LABEL(e) ((e).payload.block_label)
#define E_BLOCK_CONST(e) ((e).payload.block_const)
#define E_BLOCK_CONST_LINE(e) ((e).payload.block_const_line)
#define E_BLOCK_TYPE(e) ((e).payload.block_type)
#define E_BLOCK_TYPE_LINE(e) ((e).payload.block_type_line)
#define E_BLOCK_VAR(e) ((e).payload.block_var)
#define E_BLOCK_VAR_LINE(e) ((e).payload.block_var_line)
#define E_BLOCK_PROCEDURE(e) ((e).payload.block_procedure)
#define E_BLOCK_FUNCTION(e) ((e).payload.block_function)
