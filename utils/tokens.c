#include "../scanner.h"
#include "../token.h"
#include <stdio.h>
#include <stdlib.h>

static void
print_token(token_t *tok)
{
	if (tok->meta != 0) {
		printf("%s(%s)\n", tokentype_string(tok->type), tok->meta);
	} else {
		puts(tokentype_string(tok->type));
	}
}

static void
openerror()
{
	perror("cannot open input file");
	exit(1);
}

int
main(int argc, char **argv)
{
	scanner_t *scanner;
	token_t *tok;
	int eof = 0;
	FILE *fp;
	char *buffer;
	size_t len;

	if (argc == 1) {
		fprintf(stderr, "Please provide the file name\n");
		exit(1);
	}

	if ((fp = fopen(argv[1], "r")) == NULL) {
		openerror();
	}
	if (fseek(fp, 0, SEEK_END) == -1) {
		openerror();
	}
	if ((len = ftell(fp)) == -1) {
		openerror();
	}
	if (fseek(fp, 0, SEEK_SET) == -1) {
		openerror();
	}
	if ((buffer = malloc(sizeof(char) * len)) == NULL) {
		openerror();
	}
	if (fread(buffer, len, sizeof(char), fp) == 0) {
		openerror();
	}

	if ((scanner = scanner_init(buffer, len)) == 0) {
		puts("error: scanner_init");
		return 1;
	}

	do {
		tok = scanner_next(scanner);
		if (tok && tok->type != TOK_EOF) {
			print_token(tok);
		}
		if (tok->type == TOK_EOF) {
			eof = 1;
		}
		token_free(tok);
		free(tok);
	} while (!eof);

	scanner_free(scanner);
	return 0;
}
