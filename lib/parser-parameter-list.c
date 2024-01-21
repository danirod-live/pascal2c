#include "parser.h"
#include "token.h"
#include <stdio.h>

static expr_t *do_parse_idtype_block(parser_t *);
static expr_t *do_parse_parameter_list(parser_t *);

expr_t *
parser_parameter_list(parser_t *parser)
{
	expr_t *root = NULL;
	token_t *peekable = parser_peek(parser);

	/* Maybe we don't have to parse a list at all. */
	if (peekable->type == TOK_LPAREN) {
		root = do_parse_parameter_list(parser);
	}

	return root;
}

static expr_t *
do_parse_parameter_list(parser_t *parser)
{
	token_t *token;
	expr_t *root = NULL, *idtype, *group = NULL;

	/* Must be an LPAREN, since we come from the IF. */
	token = parser_token(parser);

	for (;;) {
		idtype = do_parse_idtype_block(parser);

		if (root == NULL) {
			root = new_binary(token, idtype, NULL);
			group = root;
		} else {
			group->exp_right = new_binary(token, idtype, NULL);
			group = group->exp_right;
		}

		/* Check if we have a semicolon or an right-paren. */
		token = parser_token(parser);
		if (token->type == TOK_RPAREN) {
			group->exp_right = new_literal(token);
			break;
		} else if (token->type != TOK_SEMICOLON) {
			parser_error(parser,
			             token,
			             "Expected RPAREN or SEMICOLON");
		}
	}

	return root;
}

static expr_t *
do_parse_idtype_block(parser_t *parser)
{
	token_t *type;
	expr_t *parlist, *var = NULL;

	/* Maybe we need the VAR keyword. */
	if (parser_peek(parser)->type == TOK_VAR) {
		var = new_literal(parser_token(parser));
	}

	/* Parameter list. */
	parlist = parse_identifier_list(parser);

	/* Get the data type. */
	parser_consume(parser, TOK_COLON);
	type = parser_token(parser);
	if (type->type != TOK_IDENTIFIER) {
		parser_error(parser, type, "Expected an IDENTIFIER");
	}

	return new_binary(type, var, parlist);
}
