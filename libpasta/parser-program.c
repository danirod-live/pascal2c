#include <parser.h>

static expr_t *progident(parser_t *parser);
static expr_t *progparam(parser_t *parser);

expr_t *
parser_program(parser_t *parser)
{
	token_t *programkw;
	expr_t *ident, *block;

	programkw = parser_token_expect(parser, TOK_PROGRAM);
	ident = progident(parser);
	parser_token_expect(parser, TOK_SEMICOLON);
	block = parser_block(parser);
	parser_token_expect(parser, TOK_DOT);

	return new_binary(programkw, ident, block);
}

static expr_t *
progident(parser_t *parser)
{
	expr_t *ident;
	token_t *token;

	ident = parser_identifier(parser);
	token = parser_peek(parser);
	if (token->type == TOK_LPAREN) {
		/* elevate the identifier into a unary tree. */
		ident->type = UNARY;
		ident->exp_left = progparam(parser);
	}
	return ident;
}

static expr_t *
progparam(parser_t *parser)
{
	expr_t *root = NULL, *next;
	token_t *peek;

	parser_token_expect(parser, TOK_LPAREN);
	for (;;) {
		if (root == NULL) {
			root = parser_identifier(parser);
			next = root;
		} else {
			next->type = UNARY;
			next->exp_left = parser_identifier(parser);
			next = next->exp_left;
		}

		peek = parser_peek(parser);
		switch (peek->type) {
		/* another iteration. */
		case TOK_COMMA:
			parser_token_expect(parser, TOK_COMMA);
			break;
		case TOK_RPAREN:
			parser_token_expect(parser, TOK_RPAREN);
			return root;
		default:
			parser_error(parser, peek, "Unexpected token here");
		}
	}
}
