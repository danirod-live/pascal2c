#include "../scanner.h"
#include "../token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

		// skip whitespace present at the beginning of partial_buffer
		while (*pbuf_start == '\n' || *pbuf_start == '\r'
		       || *pbuf_start == '\t' || *pbuf_start == ' ')
			pbuf_start++;

		if (*pbuf_start == 0) {
			// if after skipping whitespace, you reach end, then
			// you did not read nothing at all.
			break;
		}

		strncpy(buffer + offt, pbuf_start, FGETS_SIZE);
		offt += strnlen(pbuf_start, FGETS_SIZE);

		// limit to a single line -- TODO: remove this in the future
		int line_ended = 0;
		for (char *p = pbuf_start; *p; p++)
			if (*p == '\n')
				line_ended = 1;
		if (line_ended)
			break;
	}
	if (buffer[0] == 0)
		return -1;
	return 0;
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
readloop()
{
	printf("> ");
	if (readkeyb() == -1) {
		return 1;
	}

	evaltoken();

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

int
main(int argc, char **argv)
{
	while (!readloop())
		;
}
