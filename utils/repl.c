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

#include "parser.h"
#include "scanner.h"
#include "token.h"

#define BUFFER_SIZE 4096
#define FGETS_SIZE 80

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
		dump_expr(parser_statement(parser));
		scanner_free(scanner);
		return 0;
	}
	return -1;
}

static int
readtokenloop()
{
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
	puts(" --tokens: read in tokens mode");
	puts(" --exprs: read in expressions mode");
	puts(" --multiline: don't stop after a line break");
}

#define MODE_TOKENS 1
#define MODE_EXPRS 2

int
main(int argc, char **argv)
{
	int mode = MODE_TOKENS;
	if (argc >= 2) {
		if (!strncmp(argv[1], "--tokens", 10)) {
			mode = MODE_TOKENS;
		} else if (!strncmp(argv[1], "--exprs", 10)) {
			mode = MODE_EXPRS;
		} else {
			printf("Invalid argument: %s", argv[1]);
			usage();
			return 1;
		}
	}
	if (mode == MODE_TOKENS) {
		while (!readtokenloop())
			;
	} else if (mode == MODE_EXPRS) {
		puts("Entering expression mode. Type Pascal code to be "
		     "evaluated.\n"
		     "End your expression with an empty line to submit to the "
		     "parser.");
		while (!readexprloop())
			;
	}
}
