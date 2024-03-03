#include <parser.h>
#include <stdlib.h>

static int parser_block_prologue(token_t *token);

static expr_t *constblock(parser_t *parser);
static expr_t *constexpression(parser_t *parser);
static expr_t *typeblock(parser_t *parser);
static expr_t *typeexpression(parser_t *parser);
static expr_t *varblock(parser_t *parser);
static expr_t *varexpression(parser_t *parser);
static expr_t *functionproc(parser_t *parser);
static expr_t *beginblock(parser_t *parser);
static token_t *newsemi();

expr_t *
parser_block(parser_t *parser)
{
	token_t *token;
	expr_t *root, *next;

	root = new_binary(newsemi(), NULL, NULL);
	next = root;

	for (;;) {
		token = parser_peek(parser);
		switch (token->type) {
		case TOK_CONST:
			next->exp_left = constblock(parser);
			break;
		case TOK_TYPE:
			next->exp_left = typeblock(parser);
			break;
		case TOK_VAR:
			next->exp_left = varblock(parser);
			break;
		case TOK_FUNCTION:
		case TOK_PROCEDURE:
			next->exp_left = functionproc(parser);
			break;
		case TOK_BEGIN:
			next->exp_left = beginblock(parser);
			return root;
		case TOK_EOF:
			parser_error(parser,
			             token,
			             "You did not close the block");
		default:
			parser_error(parser, token, "Token invalid at block");
		}

		/* So there is another part on this block. */
		next->exp_right = new_binary(newsemi(), NULL, NULL);
		next = next->exp_right;
	}
}

static token_t *
newsemi()
{
	token_t *tok = malloc(sizeof(token_t));
	tok->type = TOK_SEMICOLON;
	tok->meta = NULL;
	return tok;
}

static int
parser_block_prologue(token_t *token)
{
	switch (token->type) {
	case TOK_CONST:
	case TOK_TYPE:
	case TOK_VAR:
	case TOK_PROCEDURE:
	case TOK_FUNCTION:
	case TOK_BEGIN:
	case TOK_EOF:
		return 1;
	default:
		return 0;
	}
}

static expr_t *
constblock(parser_t *parser)
{
	token_t *constroot, *semicolon, *peek;
	expr_t *root, *next;

	constroot = parser_token_expect(parser, TOK_CONST);
	root = new_binary(constroot, NULL, NULL);
	next = root;

	for (;;) {
		next->exp_left = constexpression(parser);
		semicolon = parser_token_expect(parser, TOK_SEMICOLON);

		/* Check if we done. */
		peek = parser_peek(parser);
		if (parser_block_prologue(peek)) {
			return root;
		}

		/* Prepare for the next constant to assign. */
		next->exp_right = new_binary(semicolon, NULL, NULL);
		next = next->exp_right;
	}
}

static expr_t *
constexpression(parser_t *parser)
{
	token_t *equal;
	expr_t *ident, *constant;

	ident = parser_identifier(parser);
	equal = parser_token_expect(parser, TOK_EQUAL);
	constant = parser_constant(parser);
	return new_binary(equal, ident, constant);
}

static expr_t *
typeblock(parser_t *parser)
{
	token_t *constroot, *semicolon, *peek;
	expr_t *root, *next;

	constroot = parser_token_expect(parser, TOK_TYPE);
	root = new_binary(constroot, NULL, NULL);
	next = root;

	for (;;) {
		next->exp_left = typeexpression(parser);
		semicolon = parser_token_expect(parser, TOK_SEMICOLON);

		/* Check if we done. */
		peek = parser_peek(parser);
		if (parser_block_prologue(peek)) {
			return root;
		}

		/* Prepare for the next constant to assign. */
		next->exp_right = new_binary(semicolon, NULL, NULL);
		next = next->exp_right;
	}
}

static expr_t *
typeexpression(parser_t *parser)
{
	token_t *equal;
	expr_t *ident, *constant;

	ident = parser_identifier(parser);
	equal = parser_token_expect(parser, TOK_EQUAL);
	constant = parser_type(parser);
	return new_binary(equal, ident, constant);
}

static expr_t *
varblock(parser_t *parser)
{
	token_t *vartoken, *semicolon, *peek;
	expr_t *root, *next;

	vartoken = parser_token_expect(parser, TOK_VAR);
	root = new_binary(vartoken, NULL, NULL);
	next = root;

	for (;;) {
		next->exp_left = varexpression(parser);
		semicolon = parser_token_expect(parser, TOK_SEMICOLON);

		/* Are we done? */
		peek = parser_peek(parser);
		if (parser_block_prologue(peek)) {
			return root;
		}

		/* Prepare for next. */
		next->exp_right = new_binary(semicolon, NULL, NULL);
		next = next->exp_right;
	}
}

static expr_t *
varexpression(parser_t *parser)
{
	token_t *colon;
	expr_t *identifiers, *type;

	identifiers = parser_identifier_list(parser);
	colon = parser_token_expect(parser, TOK_COLON);
	type = parser_type(parser);
	return new_binary(colon, identifiers, type);
}

static expr_t *
functionproc(parser_t *parser)
{
	expr_t *block, *ident, *parlist, *prototype;
	token_t *keyword;

	/* Read the function prototype. */
	keyword = parser_token(parser);
	ident = parser_identifier(parser);
	parlist = parser_parameter_list(parser);
	prototype = new_binary(ident->token, parlist, NULL);

	if (keyword->type == TOK_FUNCTION) {
		/* Take the return type and add it to the prototype. */
		parser_token_expect(parser, TOK_COLON);
		prototype->exp_right = parser_type(parser);
	}

	parser_token_expect(parser, TOK_SEMICOLON);
	block = parser_block(parser);
	parser_token_expect(parser, TOK_SEMICOLON);

	return new_binary(keyword, prototype, block);
}

static expr_t *
beginblock(parser_t *parser)
{
	token_t *begin, *separator;
	expr_t *root, *next;

	begin = parser_token_expect(parser, TOK_BEGIN);
	root = new_unary(begin, NULL);
	next = root;

	for (;;) {
		next->exp_left = parser_statement(parser);

		separator = parser_token(parser);
		switch (separator->type) {
		case TOK_SEMICOLON:
			next->exp_right = new_binary(separator, NULL, NULL);
			next = next->exp_right;
			break;
		case TOK_END:
			return root;
		default:
			parser_error(parser,
			             separator,
			             "Unexpected character here");
		}
	}
}
