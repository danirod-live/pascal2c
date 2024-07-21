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

#include <parser.h>
#include <stdlib.h>

static void p_append(expr2_t *exp, expr_block_t *block);
static void p_block(parser_t *parser, expr_block_t *block);
static expr2_t *p_label(parser_t *parser);
static expr2_t *p_const(parser_t *parser);
static expr2_t *p_const_line(parser_t *parser);
static expr2_t *p_type(parser_t *parser);
static expr2_t *p_type_line(parser_t *parser);
static expr2_t *p_var(parser_t *parser);
static expr2_t *p_var_line(parser_t *parser);
static expr2_t *p_procedure(parser_t *parser);
static expr2_t *p_function(parser_t *parser);

expr2_t *
parser2_block(parser_t *parser)
{
	expr2_t *aux, *exp = expression_new(EXP_BLOCK);
	expr_block_t *block = &(E_BLOCK(*exp));
	token_t *token;

	for (;;) {
		token = parser_peek(parser);
		switch (token->type) {
		case TOK_LABEL:
			aux = p_label(parser);
			p_append(aux, block);
			break;
		case TOK_CONST:
			aux = p_const(parser);
			p_append(aux, block);
			break;
		case TOK_TYPE:
			aux = p_type(parser);
			p_append(aux, block);
			break;
		case TOK_VAR:
			aux = p_var(parser);
			p_append(aux, block);
			break;
		case TOK_PROCEDURE:
			aux = p_procedure(parser);
			p_append(aux, block);
			break;
		case TOK_FUNCTION:
			aux = p_function(parser);
			p_append(aux, block);
			break;
		case TOK_BEGIN:
			p_block(parser, block);
			return exp;
		default:
			parser_error(parser, token, "Unexpected token type");
		}
	}
}

static expr2_t *
p_label(parser_t *parser)
{
	expr2_t *aux, *exp = expression_new(EXP_BLOCK_LABEL);
	expr_block_label_t *label = &(E_BLOCK_LABEL(*exp));
	int next;
	token_t *token;

	parser_token_expect(parser, TOK_LABEL);

	for (;;) {
		aux = parser2_identifier(parser);

		next = label->label_count++;
		label->labels = realloc(label->labels,
		                        sizeof(expr2_t *) * label->label_count);
		label->labels[next] = aux;

		token = parser_token(parser);
		if (token->type == TOK_SEMICOLON) {
			return exp;
		} else if (token->type != TOK_COMMA) {
			parser_error(parser,
			             token,
			             "Expected either COMMA or SEMICOLON");
		}
	}
}

static void
p_append(expr2_t *exp, expr_block_t *block)
{
	int next;

	next = block->preblock_count++;
	block->preblocks = realloc(block->preblocks,
	                           sizeof(expr2_t *) * block->preblock_count);
	block->preblocks[next] = exp;
}

static void
p_block(parser_t *parser, expr_block_t *block)
{
	expr2_t *aux;
	int next;
	token_t *token;

	parser_token_expect(parser, TOK_BEGIN);

	for (;;) {
		aux = parser2_statement(parser);

		next = block->statement_count++;
		block->statements =
		    realloc(block->statements,
		            sizeof(expr2_t *) * block->statement_count);
		block->statements[next] = aux;

		token = parser_token(parser);
		if (token->type == TOK_END) {
			return;
		} else if (token->type != TOK_SEMICOLON) {
			parser_error(parser,
			             token,
			             "Expected either SEMICOLON or END");
		}
	}
}

static expr2_t *
p_const(parser_t *parser)
{
	expr2_t *aux, *exp = expression_new(EXP_BLOCK_CONST);
	expr_block_const_t *constb = &(E_BLOCK_CONST(*exp));
	int next;
	token_t *token;

	parser_token_expect(parser, TOK_CONST);

	for (;;) {
		aux = p_const_line(parser);

		next = constb->line_count++;
		constb->lines = realloc(constb->lines,
		                        sizeof(expr2_t *) * constb->line_count);
		constb->lines[next] = aux;

		token = parser_peek(parser);
		if (token->type != TOK_IDENTIFIER) {
			return exp;
		}
	}
}

static expr2_t *
p_const_line(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_BLOCK_CONST_LINE);
	expr_block_const_line_t *line = &(E_BLOCK_CONST_LINE(*exp));
	line->name = parser2_identifier(parser);
	parser_token_expect(parser, TOK_EQUAL);
	line->value = parser2_constant(parser);
	parser_token_expect(parser, TOK_SEMICOLON);
	return exp;
}

static expr2_t *
p_type(parser_t *parser)
{
	expr2_t *aux, *exp = expression_new(EXP_BLOCK_TYPE);
	expr_block_type_t *typeb = &(E_BLOCK_TYPE(*exp));
	int next;
	token_t *token;

	parser_token_expect(parser, TOK_TYPE);

	for (;;) {
		aux = p_type_line(parser);

		next = typeb->line_count++;
		typeb->lines = realloc(typeb->lines,
		                       sizeof(expr2_t *) * typeb->line_count);
		typeb->lines[next] = aux;

		token = parser_peek(parser);
		if (token->type != TOK_IDENTIFIER) {
			return exp;
		}
	}
}
static expr2_t *
p_type_line(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_BLOCK_TYPE_LINE);
	expr_block_type_line_t *line = &(E_BLOCK_TYPE_LINE(*exp));
	line->name = parser2_identifier(parser);
	parser_token_expect(parser, TOK_EQUAL);
	line->value = parser2_type(parser);
	parser_token_expect(parser, TOK_SEMICOLON);
	return exp;
}
static expr2_t *
p_var(parser_t *parser)
{
	expr2_t *aux, *exp = expression_new(EXP_BLOCK_VAR);
	expr_block_var_t *varb = &(E_BLOCK_VAR(*exp));
	int next;
	token_t *token;

	parser_token_expect(parser, TOK_VAR);

	for (;;) {
		aux = p_var_line(parser);

		next = varb->line_count++;
		varb->lines =
		    realloc(varb->lines, sizeof(expr2_t *) * varb->line_count);
		varb->lines[next] = aux;

		token = parser_peek(parser);
		if (token->type != TOK_IDENTIFIER) {
			return exp;
		}
	}
}

static expr2_t *
p_var_line(parser_t *parser)
{
	expr2_t *aux, *exp = expression_new(EXP_BLOCK_VAR_LINE);
	expr_block_var_line_t *line = &(E_BLOCK_VAR_LINE(*exp));
	int next;
	token_t *token;

	for (;;) {
		aux = parser2_identifier(parser);

		next = line->identifier_count++;
		line->identifiers =
		    realloc(line->identifiers,
		            sizeof(expr2_t *) * line->identifier_count);
		line->identifiers[next] = aux;

		token = parser_token(parser);
		if (token->type == TOK_COLON) {
			line->type = parser2_type(parser);
			parser_token_expect(parser, TOK_SEMICOLON);
			return exp;
		} else if (token->type != TOK_COMMA) {
			parser_error(parser,
			             token,
			             "Expected either COMMA or COLON");
		}
	}
}

static expr2_t *
p_procedure(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_BLOCK_PROCEDURE);
	expr_block_procedure_t *proc = &(E_BLOCK_PROCEDURE(*exp));

	parser_token_expect(parser, TOK_PROCEDURE);
	proc->identifier = parser2_identifier(parser);
	proc->parlist = parser2_parameter_list(parser);
	parser_token_expect(parser, TOK_SEMICOLON);
	proc->block = parser2_block(parser);
	parser_token_expect(parser, TOK_SEMICOLON);

	return exp;
}

static expr2_t *
p_function(parser_t *parser)
{
	expr2_t *exp = expression_new(EXP_BLOCK_FUNCTION);
	expr_block_function_t *func = &(E_BLOCK_FUNCTION(*exp));

	parser_token_expect(parser, TOK_FUNCTION);
	func->identifier = parser2_identifier(parser);
	func->parlist = parser2_parameter_list(parser);
	parser_token_expect(parser, TOK_COLON);
	func->returntype = parser2_identifier(parser);
	parser_token_expect(parser, TOK_SEMICOLON);
	func->block = parser2_block(parser);
	parser_token_expect(parser, TOK_SEMICOLON);

	return exp;
}
