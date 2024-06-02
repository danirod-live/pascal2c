/* repl -- a tool for interactively querying Pascal code
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
#include "scanner.h"
#include "token.h"
#include <parser.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FGETS_SIZE 80
#define BUFFER_SIZE 4096
static char buffer[BUFFER_SIZE];

typedef void (*print_expr2_detail)(expr2_t *exp);
static void print_expr2(expr2_t *exp);

static void
print_expr2_array(expr2_t **exps, unsigned int len)
{
	int i;

	printf("[");
	for (i = 0; i < len; i++) {
		print_expr2(exps[i]);
		if (i + 1 < len)
			printf(",");
	}

	printf("]");
}

static void
print_expr2_identifier(expr2_t *exp)
{
	printf("{\"token\": \"%s\"}", E_IDENTIFIER(*exp).token->meta);
}

static void
print_expr2_unsigned_number(expr2_t *exp)
{
	printf("{\"token\": \"%s\"}", E_UNSIGNED_NUMBER(*exp).token->meta);
}

static void
print_expr2_unsigned_integer(expr2_t *exp)
{
	printf("{\"token\": \"%s\"}", E_UNSIGNED_INTEGER(*exp).token->meta);
}

static void
print_expr2_variable(expr2_t *exp)
{
	printf("{\"identifier\": ");
	print_expr2(E_VARIABLE(*exp).identifier);
	printf(", \"paths\": ");
	print_expr2_array(E_VARIABLE(*exp).paths, E_VARIABLE(*exp).path_count);
	printf("}");
}

static void
print_expr2_variable_path_dot(expr2_t *exp)
{
	printf("{\"identifier\": ");
	print_expr2(E_VARIABLE_PATH_DOT(*exp).identifier);
	printf("}");
}

static void
print_expr2_variable_path_array(expr2_t *exp)
{
	printf("{\"paths\": ");
	print_expr2_array(E_VARIABLE_PATH_ARRAY(*exp).expressions,
	                  E_VARIABLE_PATH_ARRAY(*exp).exp_count);
	printf("}");
}

static void
print_expr2_string(expr2_t *exp)
{
	char *ptr;

	printf("{\"string\": \"");
	ptr = E_STRING(*exp).token->meta;
	while (ptr && *ptr) {
		switch (*ptr) {
		case '\\':
		case '"':
			printf("\\%c", *ptr);
			break;
		default:
			printf("%c", *ptr);
			break;
		}
		ptr++;
	}

	printf("\"}");
}

static void
print_expr2_unsigned_constant(expr2_t *exp)
{
	printf("{\"inner\": ");
	print_expr2(E_UNSIGNED_CONSTANT(*exp).inner);
	printf("}");
}

static void
print_expr2_constant(expr2_t *exp)
{
	printf("{\"sign\": \"%c\", ", E_CONSTANT(*exp).sign);
	printf("\"inner\": ");
	print_expr2(E_CONSTANT(*exp).inner);
	printf("}");
}

static void
print_expr2_expression_node(expr_t *exp)
{
	const char *token_type;

	printf("{");

	switch (exp->type) {
	case UNARY:
		printf("\"type\": \"UNARY\"");
		break;
	case BINARY:
		printf("\"type\": \"BINARY\"");
		break;
	case GROUPING:
		printf("\"type\": \"GROUPING\"");
		break;
	case LITERAL:
		printf("\"type\": \"LITERAL\"");
		break;
	}

	if (exp->token) {
		if (exp->token->type) {
			token_type = tokentype_string(exp->token->type);
			printf(", \"token\": \"%s\"", token_type);
		}
		if (exp->token->meta) {
			printf(", \"meta\": \"%s\"", exp->token->meta);
		}
	}
	if (exp->exp_left) {
		printf(", \"left\": ");
		print_expr2_expression_node(exp->exp_left);
	}
	if (exp->exp_right) {
		printf(", \"right\": ");
		print_expr2_expression_node(exp->exp_right);
	}

	printf("}");
}

static void
print_expr2_expression(expr2_t *exp)
{
	print_expr2_expression_node(E_EXPRESSION(*exp).exp);
}

static void
print_expr2_simple_type(expr2_t *exp)
{
	printf("{\"type\": \"");
	switch (E_SIMPLE_TYPE(*exp).type) {
	case SIMPLE_TYPE_SINGLE:
		printf("single");
		break;
	case SIMPLE_TYPE_RANGE:
		printf("range");
		break;
	case SIMPLE_TYPE_LIST:
		printf("list");
		break;
	}
	printf("\", ");

	printf("\"components\": ");
	print_expr2_array(E_SIMPLE_TYPE(*exp).components,
	                  E_SIMPLE_TYPE(*exp).component_count);
	printf("}");
}

static void
print_expr2_set_type(expr2_t *exp)
{
	expr_set_type_t *set = &(E_SET_TYPE(*exp));

	printf("{\"packed\": %s, \"simple_type\": ",
	       set->packed ? "true" : "false");
	print_expr2_simple_type(E_SET_TYPE(*exp).simple_type);
	printf("}");
}

static void
print_expr2_array_type(expr2_t *exp)
{
	expr_array_type_t *arr = &(E_ARRAY_TYPE(*exp));

	printf("{\"packed\": %s, \"type\": ", arr->packed ? "true" : "false");
	print_expr2(arr->type);
	printf(", \"dimensions\": ");
	print_expr2_array(arr->inner_types, arr->inner_types_count);
	printf("}");
}

static void
print_expr2_ref_type(expr2_t *exp)
{
	print_expr2_identifier(E_REFERENCE_TYPE(*exp).identifier);
}

static void
print_expr2_file_type(expr2_t *exp)
{
	expr_file_type_t *file = &(E_FILE_TYPE(*exp));

	printf("{\"packed\": %s, \"type\": ", file->packed ? "true" : "false");
	if (file->type == NULL) {
		printf("null");
	} else {
		print_expr2(file->type);
	}
	printf("}");
}

static void
print_expr2_plist(expr2_t *exp)
{
	int i;
	expr_plist_t *plist = &(E_PLIST(*exp));

	printf("{\"chunks\": [");
	for (i = 0; i < plist->chunks_count; i++) {
		print_expr2(plist->chunks[i]);
		if (i + 1 < plist->chunks_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_plist_chunk(expr2_t *exp)
{
	int i;
	expr_plist_chunk_t *chunk = &(E_PLIST_CHUNK(*exp));

	printf("{\"data_type\": ");
	print_expr2(chunk->data_type);

	printf(", \"identifiers\": [");
	for (i = 0; i < chunk->identifier_count; i++) {
		print_expr2(chunk->identifiers[i]);
		if (i + 1 < chunk->identifier_count)
			printf(", ");
	}
	printf("], \"reference\": %s", chunk->is_reference ? "true" : "false");

	printf("}");
}

static void
print_expr2(expr2_t *exp)
{
	char *type;
	print_expr2_detail detail_func;

	switch (exp->type) {
	case EXP_IDENTIFIER:
		type = "Identifier";
		detail_func = print_expr2_identifier;
		break;
	case EXP_UNSIGNED_NUMBER:
		type = "UnsignedNumber";
		detail_func = print_expr2_unsigned_number;
		break;
	case EXP_UNSIGNED_INTEGER:
		type = "UnsignedInteger";
		detail_func = print_expr2_unsigned_integer;
		break;
	case EXP_VARIABLE:
		type = "Variable";
		detail_func = print_expr2_variable;
		break;
	case EXP_VARIABLE_PATH_DOT:
		type = "VariableDot";
		detail_func = print_expr2_variable_path_dot;
		break;
	case EXP_VARIABLE_PATH_CARET:
		type = "VariableCaret";
		detail_func = NULL;
		break;
	case EXP_VARIABLE_PATH_ARRAY:
		type = "VariableArray";
		detail_func = print_expr2_variable_path_array;
		break;
	case EXP_NIL:
		type = "NilConstant";
		detail_func = NULL;
		break;
	case EXP_STRING:
		type = "String";
		detail_func = print_expr2_string;
		break;
	case EXP_UNSIGNED_CONSTANT:
		type = "UnsignedConstant";
		detail_func = print_expr2_unsigned_constant;
		break;
	case EXP_CONSTANT:
		type = "Constant";
		detail_func = print_expr2_constant;
		break;
	case EXP_EXPRESSION:
		type = "Expression";
		detail_func = print_expr2_expression;
		break;
	case EXP_SIMPLE_TYPE:
		type = "SimpleType";
		detail_func = print_expr2_simple_type;
		break;
	case EXP_REFERENCE_TYPE:
		type = "ReferenceType";
		detail_func = print_expr2_ref_type;
		break;
	case EXP_SET_TYPE:
		type = "SetType";
		detail_func = print_expr2_set_type;
		break;
	case EXP_ARRAY_TYPE:
		type = "ArrayType";
		detail_func = print_expr2_array_type;
		break;
	case EXP_FILE_TYPE:
		type = "FileType";
		detail_func = print_expr2_file_type;
		break;
	case EXP_PLIST:
		type = "ParameterList";
		detail_func = print_expr2_plist;
		break;
	case EXP_PLIST_CHUNK:
		type = "ParameterListChunk";
		detail_func = print_expr2_plist_chunk;
		break;
	default:
		type = NULL;
		detail_func = NULL;
	}

	if (type != NULL) {
		printf("{\"type\": \"%s\"", type);
		if (detail_func) {
			printf(", \"data\": ");
			detail_func(exp);
		}
		printf("}");
	}
}

static void
read_file()
{
	int offt = 0;
	char partial_buffer[FGETS_SIZE];

	while (fgets(partial_buffer, FGETS_SIZE, stdin) != NULL) {
		// append whatever we have read into the buffer
		strncpy(buffer + offt, partial_buffer, FGETS_SIZE);
		offt += strnlen(partial_buffer, FGETS_SIZE);
	}
}

static int
read_keyboard()
{
	int offt = 0;
	char partial_buffer[FGETS_SIZE];
	int is_new_line;

	// mark the start of the string with a zero to detect empty string
	buffer[0] = 0;

	printf(">>> ");

	while (fgets(partial_buffer, FGETS_SIZE, stdin) != NULL) {
		char *pbuf_start = partial_buffer;
		int partial_buffer_len = strnlen(partial_buffer, FGETS_SIZE);

		// skip whitespace at the beginning of the read line and treat
		// empty strings as if no characters had been read
		while (*pbuf_start == '\n' || *pbuf_start == '\r'
		       || *pbuf_start == '\t')
			pbuf_start++;
		if (*pbuf_start == 0) {
			break;
		}

		// append whatever we have read into the buffer
		strncpy(buffer + offt, pbuf_start, FGETS_SIZE);
		offt += strnlen(pbuf_start, FGETS_SIZE);

		// check if partial_buffer ended with line break
		is_new_line = partial_buffer[partial_buffer_len - 1] == '\n';
		if (is_new_line) {
			printf("... ");
		}
	}
	return offt;
}

int
eval_code()
{
	int successful = 1;
	scanner_t *scanner;
	parser_t *parser;

	scanner = scanner_init(buffer, strnlen(buffer, BUFFER_SIZE));
	if (!scanner) {
		successful = 0;
		goto pre_cleanup_scanner;
	}
	parser = parser_new();
	if (!parser) {
		successful = 0;
		goto pre_cleanup_parser;
	}

	parser_load_tokens(parser, scanner);
	expr2_t *ident = parser2_type(parser);
	print_expr2(ident);

pre_cleanup_parser:
	if (parser)
		parser_free(parser);
pre_cleanup_scanner:
	if (scanner)
		scanner_free(scanner);
	return successful;
}

void
main_loop()
{
	int len;
	do {
		len = read_keyboard();
		if (len > 0) {
			eval_code();
			printf("\n");
		}
	} while (len > 0);
}

void
single_read()
{
	read_file();
	eval_code();
}

int
main(int argc, char **argv)
{
	strcpy(buffer, "");

	if (isatty(0)) {
		puts("Entering REPL. Send empty prompt to exit.");
		main_loop();
	} else {
		single_read();
	}
}
