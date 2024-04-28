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
	case EXP_UNSIGNED_IDENTIFIER:
		type = "UnsignedIdentifier";
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
	default:
		type = NULL;
		detail_func = NULL;
	}

	if (type != NULL) {
		printf("{\"type\": \"%s\", \"data\": ", type);
		if (detail_func) {
			detail_func(exp);
		} else {
			printf("{}");
		}
		printf("}\n");
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

	expr2_t *ident = parser2_variable(parser);
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
