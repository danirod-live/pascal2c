#include "parser.h"
#include "scanner.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>

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
	parser_t *parser;
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

	if ((parser = parser_new()) == NULL) {
		puts("error: parser_new");
		return 1;
	}

	parser_load_tokens(parser, scanner);
	dump_expr(parser_type(parser));

	scanner_free(scanner);
	return 0;
}
