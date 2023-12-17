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
	int i, offt = 0;
	char *cleanup;

	if (fgets(buffer + offt, FGETS_SIZE, stdin)) {
		// check if it is empty string
		cleanup = buffer;
		while (*cleanup == '\n' || *cleanup == '\t' || *cleanup == ' ')
			cleanup++;
		if (*cleanup == 0)
			return -1;
		return 0;
	}
	return -1;
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
