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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "parser.h"
#include "scanner.h"
#include "token.h"

#define BUFFER_SIZE 4096
#define FGETS_SIZE 80

#define DEFAULT_EXPRESSION_NODE "statement"

struct expfunc_type {
	const char *type;
	expr_t *(*callback)(parser_t *);
	const char *desc;
};

static struct expfunc_type expfunc_list[] = {
    {"identifier", parser_identifier, "Identifiers"},
    {"unsigned_integer", parser_unsigned_integer, "Unsigned integers"},
    {"unsigned_number", parser_unsigned_number, "Unsigned numbers"},
    {"unsigned_constant", parser_unsigned_constant, "Unsigned constant"},
    {"constant", parser_constant, "Constant"},
    {"simple_type", parser_simple_type, "Simple type"},
    {"type", parser_type, "Type"},
    {"field_list", parser_field_list, "Field list"},
    {"variable", parser_variable, "Variable"},
    {"expression", parser_expression, "Expression"},
    {"simple_expression", parser_simple_expression, "Simple expression"},
    {"term", parser_term, "Term"},
    {"factor", parser_factor, "Factor"},
    {"parameter_list", parser_parameter_list, "Parameter list"},
    {"statement", parser_statement, "Statement"},
    {0, 0, 0},
};

#define MODE_UNKNOWN 0
#define MODE_TOKENS 1
#define MODE_EXPRS 2

static int func_mode = MODE_UNKNOWN;
static char *func_expr_type = NULL;
static expr_t *(*func_expr_cb)(parser_t *);
static int func_quiet = 0;

static struct expfunc_type *
get_desired_expfunc(char *type)
{
	struct expfunc_type *expr = expfunc_list;
	while (expr->type != 0) {
		if (!strcmp(type, expr->type)) {
			return expr;
		}
		expr++;
	}
	return NULL;
}

static char buffer[BUFFER_SIZE];

static void
print_token(token_t *tok)
{
	if (tok->meta != 0) {
		printf("%s(%s)\n", tokentype_string(tok->type), tok->meta);
	} else {
		puts(tokentype_string(tok->type));
	}
}

static int
readkeyb()
{
	int offt = 0;
	char partial_buffer[FGETS_SIZE];

	// mark the start of the string with a zero to detect empty string
	buffer[0] = 0;

	while (fgets(partial_buffer, FGETS_SIZE, stdin) != NULL) {
		char *pbuf_start = partial_buffer;

		// skip whitespace at the beginning of the read line and treat
		// empty strings as if no characters had been read
		while (*pbuf_start == '\n' || *pbuf_start == '\r'
		       || *pbuf_start == '\t' || *pbuf_start == ' ')
			pbuf_start++;
		if (*pbuf_start == 0) {
			break;
		}

		// append whatever we have read into the buffer
		strncpy(buffer + offt, pbuf_start, FGETS_SIZE);
		offt += strnlen(pbuf_start, FGETS_SIZE);
	}
	return offt;
}

static int
evaltoken()
{
	scanner_t *scanner;
	token_t *token;
	int eof = 0;
	int length = strnlen((const char *) buffer, BUFFER_SIZE);

	if ((scanner = scanner_init(buffer, length)) != NULL) {
		do {
			token = scanner_next(scanner);
			if (token)
				print_token(token);
			if (token->type == TOK_EOF) {
				eof = 1;
			}
			token_free(token);
			free(token);
		} while (!eof);

		scanner_free(scanner);
		return 0;
	}
	return -1;
}

static int
evalexpr()
{
	scanner_t *scanner;
	parser_t *parser;
	token_t *token;
	int eof = 0;
	int length = strnlen((const char *) buffer, BUFFER_SIZE);

	if ((scanner = scanner_init(buffer, length)) != NULL) {
		parser = parser_new();
		parser_load_tokens(parser, scanner);
		dump_expr(func_expr_cb(parser));
		scanner_free(scanner);
		return 0;
	}
	return -1;
}

static int
readtokenloop()
{
	if (isatty(0))
		printf("> ");
	if (readkeyb() == 0) {
		return 1;
	}

	evaltoken();

	return 0;
}

static int
readexprloop()
{
	if (isatty(0))
		printf("> ");
	if (readkeyb() == 0) {
		return 1;
	}

	evalexpr();

	return 0;
}

void
usage()
{
	puts("Flags:");
	puts(" -t: read in tokens mode");
	puts(" -e=<node>: read in expressions mode of type <node>");
}

int
main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "te::hq")) != -1) {
		switch (c) {
		case 't':
			if (func_mode != MODE_UNKNOWN) {
				puts("Provide a single -t or -e");
				return 1;
			}
			func_mode = MODE_TOKENS;
			break;
		case 'e':
			if (func_mode != MODE_UNKNOWN) {
				puts("Provide a single -t or -e");
				return 1;
			}
			func_mode = MODE_EXPRS;
			func_expr_type = optarg;
			break;
		case 'h':
			usage();
			break;
		case 'q':
			func_quiet = 1;
			break;
		case '?':
			printf("tenemos un problema. c = %d\n", c);
			return 1;
			break;
		}
	}

	if (func_mode == MODE_UNKNOWN) {
		usage();
	} else if (func_mode == MODE_TOKENS) {
		while (!readtokenloop())
			;
	} else if (func_mode == MODE_EXPRS) {
		if (func_expr_type == NULL) {
			func_expr_type = DEFAULT_EXPRESSION_NODE;
		}
		struct expfunc_type *type = get_desired_expfunc(func_expr_type);
		if (type == NULL) {
			printf("Unrecognised type: %s\n", func_expr_type);
			return 1;
		}
		func_expr_cb = type->callback;

		if (!func_quiet) {
			puts("Entering expression mode. Type Pascal code to be "
			     "evaluated.\n"
			     "End your expression with an empty line to submit "
			     "to the "
			     "parser.");
			printf("Expression mode: %s\n", type->desc);
		}
		while (!readexprloop())
			;
	}
}
