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
#include "parser.h"
#include <stdlib.h>

static void
print_token(token_t *tok)
{
	if (tok == 0) {
		return;
	}
	if (tok->meta != 0) {
		printf("%s(%s)\n", tokentype_string(tok->type), tok->meta);
	} else {
		puts(tokentype_string(tok->type));
	}
}

static void
dump_expr_impl(expr_t *expr, int indent)
{
	int i;

	if (!expr) {
		return;
	}

	for (i = 0; i < indent; i++) {
		printf("|");
		if (i == indent - 1) {
			printf("- ");
		} else {
			printf("  ");
		}
	}

	switch (expr->type) {
	case BINARY:
		printf("BINARY ");
		break;
	case UNARY:
		printf("UNARY ");
		break;
	case GROUPING:
		printf("GROUPING ");
		break;
	case LITERAL:
		printf("LITERAL ");
		break;
	}
	print_token(expr->token);
	dump_expr_impl(expr->exp_left, indent + 1);
	dump_expr_impl(expr->exp_right, indent + 1);
}

/* TODO: This function should be moved to repl.c, but it is useful for
 * debugging. */
void
dump_expr(expr_t *expr)
{
	dump_expr_impl(expr, 0);
}

void
expr_free(expr_t *expr)
{
	free(expr);
}

expr_t *
new_unary(token_t *t, expr_t *expr)
{
	expr_t *exp = (expr_t *) calloc(sizeof(expr_t), 1);
	exp->type = UNARY;
	exp->exp_left = expr;
	exp->token = t;
	return exp;
}

expr_t *
new_binary(token_t *t, expr_t *left, expr_t *right)
{
	expr_t *exp = (expr_t *) calloc(sizeof(expr_t), 1);
	exp->type = BINARY;
	exp->exp_left = left;
	exp->exp_right = right;
	exp->token = t;
	return exp;
}

expr_t *
new_grouping(expr_t *wrap)
{
	expr_t *exp = (expr_t *) calloc(sizeof(expr_t), 1);
	exp->type = GROUPING;
	exp->exp_left = wrap;
	return exp;
}

expr_t *
new_literal(token_t *tok)
{
	expr_t *exp = (expr_t *) calloc(sizeof(expr_t), 1);
	exp->type = LITERAL;
	exp->token = tok;
	return exp;
}

////

parser_t *
parser_new()
{
	parser_t *par = (parser_t *) malloc(sizeof(parser_t));
	par->tokens = NULL;
	par->len = 0;
	par->pos = 0;
	return par;
}

#define TOKEN_LOAD_BUFSIZ 64

static int
parser_append(parser_t *parser, token_t **tlist, int len)
{
	unsigned i;
	token_t **next;

	next = realloc(parser->tokens, sizeof(token_t *) * (parser->len + len));
	if (next == NULL) {
		return 0;
	}

	for (i = 0; i < len; i++) {
		next[i + parser->len] = tlist[i];
	}
	parser->tokens = next;
	parser->len += len;
	return 1;
}

void
parser_load_tokens(parser_t *parser, scanner_t *scanner)
{
	int bufsiz = 0;
	token_t *last_token = NULL;
	token_t *tokens[TOKEN_LOAD_BUFSIZ];

	do {
		last_token = scanner_next(scanner);
		tokens[bufsiz++] = last_token;

		// If the buffer is full, concat to the parser.
		if (bufsiz == TOKEN_LOAD_BUFSIZ) {
			if (parser_append(parser, tokens, TOKEN_LOAD_BUFSIZ)
			    == 0) {
				printf("Algo ha salido mal"); // TODO: borrar
				exit(1);
			}
			bufsiz = 0;
		}
	} while (last_token && last_token->type != TOK_EOF);

	// Add the remaining tokens that did not fill the buffer.
	parser_append(parser, tokens, bufsiz);
}

void __attribute__((noreturn))
parser_error(parser_t *parser, token_t *token, char *error)
{
	printf("Error: %s. ", error);
	print_token(token);
	printf("\n");
	printf(" Line: %d, Col: %d\n", token->line, token->col);
	exit(1);
}

token_t *
parser_peek(parser_t *parser)
{
	return parser->tokens[parser->pos];
}

token_t *
parser_peek_far(parser_t *parser, unsigned int offset)
{
	if (parser->pos + offset < parser->len) {
		return parser->tokens[parser->pos + offset];
	}
	// TODO: devolver EOF
	parser_error(parser, parser->tokens[parser->pos], "EOF");
}

token_t *
parser_token(parser_t *parser)
{
	return parser->tokens[parser->pos++];
}
