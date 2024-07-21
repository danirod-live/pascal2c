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
#include "scanner.h"
#include "token.h"
#include <parser.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FGETS_SIZE 80
#define BUFFER_SIZE 4096
static char buffer[BUFFER_SIZE];

typedef void (*print_expr2_detail)(expr2_t *exp);
static void print_expr2(expr2_t *exp);

static void
print_expr2_array(expr2_t **exps, unsigned int len)
{
	int i;

	printf("[");
	for (i = 0; i < len; i++) {
		print_expr2(exps[i]);
		if (i + 1 < len)
			printf(",");
	}

	printf("]");
}

static void
print_expr2_identifier(expr2_t *exp)
{
	printf("{\"token\": \"%s\"}", E_IDENTIFIER(*exp).token->meta);
}

static void
print_expr2_unsigned_number(expr2_t *exp)
{
	printf("{\"token\": \"%s\"}", E_UNSIGNED_NUMBER(*exp).token->meta);
}

static void
print_expr2_unsigned_integer(expr2_t *exp)
{
	printf("{\"token\": \"%s\"}", E_UNSIGNED_INTEGER(*exp).token->meta);
}

static void
print_expr2_variable(expr2_t *exp)
{
	printf("{\"identifier\": ");
	print_expr2(E_VARIABLE(*exp).identifier);
	printf(", \"paths\": ");
	print_expr2_array(E_VARIABLE(*exp).paths, E_VARIABLE(*exp).path_count);
	printf("}");
}

static void
print_expr2_variable_path_dot(expr2_t *exp)
{
	printf("{\"identifier\": ");
	print_expr2(E_VARIABLE_PATH_DOT(*exp).identifier);
	printf("}");
}

static void
print_expr2_variable_path_array(expr2_t *exp)
{
	printf("{\"paths\": ");
	print_expr2_array(E_VARIABLE_PATH_ARRAY(*exp).expressions,
	                  E_VARIABLE_PATH_ARRAY(*exp).expression_count);
	printf("}");
}

static void
print_expr2_string(expr2_t *exp)
{
	char *ptr;

	printf("{\"string\": \"");
	ptr = E_STRING(*exp).token->meta;
	while (ptr && *ptr) {
		switch (*ptr) {
		case '\\':
		case '"':
			printf("\\%c", *ptr);
			break;
		default:
			printf("%c", *ptr);
			break;
		}
		ptr++;
	}

	printf("\"}");
}

static void
print_expr2_unsigned_constant(expr2_t *exp)
{
	printf("{\"inner\": ");
	print_expr2(E_UNSIGNED_CONSTANT(*exp).inner);
	printf("}");
}

static void
print_expr2_constant(expr2_t *exp)
{
	printf("{\"sign\": \"%c\", ", E_CONSTANT(*exp).sign);
	printf("\"inner\": ");
	print_expr2(E_CONSTANT(*exp).inner);
	printf("}");
}

static void
print_expr2_expression_node(expr_t *exp)
{
	const char *token_type;

	printf("{");

	switch (exp->type) {
	case UNARY:
		printf("\"type\": \"UNARY\"");
		break;
	case BINARY:
		printf("\"type\": \"BINARY\"");
		break;
	case GROUPING:
		printf("\"type\": \"GROUPING\"");
		break;
	case LITERAL:
		printf("\"type\": \"LITERAL\"");
		break;
	}

	if (exp->token) {
		if (exp->token->type) {
			token_type = tokentype_string(exp->token->type);
			printf(", \"token\": \"%s\"", token_type);
		}
		if (exp->token->meta) {
			printf(", \"meta\": \"%s\"", exp->token->meta);
		}
	}
	if (exp->exp_left) {
		printf(", \"left\": ");
		print_expr2_expression_node(exp->exp_left);
	}
	if (exp->exp_right) {
		printf(", \"right\": ");
		print_expr2_expression_node(exp->exp_right);
	}

	printf("}");
}

static void
print_expr2_expression(expr2_t *exp)
{
	print_expr2_expression_node(E_EXPRESSION(*exp).exp);
}

static void
print_expr2_simple_type(expr2_t *exp)
{
	printf("{\"type\": \"");
	switch (E_SIMPLE_TYPE(*exp).type) {
	case SIMPLE_TYPE_SINGLE:
		printf("single");
		break;
	case SIMPLE_TYPE_RANGE:
		printf("range");
		break;
	case SIMPLE_TYPE_LIST:
		printf("list");
		break;
	}
	printf("\", ");

	printf("\"components\": ");
	print_expr2_array(E_SIMPLE_TYPE(*exp).components,
	                  E_SIMPLE_TYPE(*exp).component_count);
	printf("}");
}

static void
print_expr2_set_type(expr2_t *exp)
{
	expr_set_type_t *set = &(E_SET_TYPE(*exp));

	printf("{\"packed\": %s, \"simple_type\": ",
	       set->packed ? "true" : "false");
	print_expr2_simple_type(E_SET_TYPE(*exp).simple_type);
	printf("}");
}

static void
print_expr2_record_type(expr2_t *exp)
{
	expr_record_type_t *set = &(E_RECORD_TYPE(*exp));

	printf("{\"packed\": %s, \"fields\": ", set->packed ? "true" : "false");
	print_expr2(set->fields);
	printf("}");
}

static void
print_expr2_array_type(expr2_t *exp)
{
	expr_array_type_t *arr = &(E_ARRAY_TYPE(*exp));

	printf("{\"packed\": %s, \"type\": ", arr->packed ? "true" : "false");
	print_expr2(arr->type);
	printf(", \"dimensions\": ");
	print_expr2_array(arr->inner_types, arr->inner_type_count);
	printf("}");
}

static void
print_expr2_ref_type(expr2_t *exp)
{
	print_expr2_identifier(E_REFERENCE_TYPE(*exp).identifier);
}

static void
print_expr2_file_type(expr2_t *exp)
{
	expr_file_type_t *file = &(E_FILE_TYPE(*exp));

	printf("{\"packed\": %s, \"type\": ", file->packed ? "true" : "false");
	if (file->type == NULL) {
		printf("null");
	} else {
		print_expr2(file->type);
	}
	printf("}");
}

static void
print_expr2_plist(expr2_t *exp)
{
	int i;
	expr_plist_t *plist = &(E_PLIST(*exp));

	printf("{\"chunks\": [");
	for (i = 0; i < plist->chunks_count; i++) {
		print_expr2(plist->chunks[i]);
		if (i + 1 < plist->chunks_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_plist_chunk(expr2_t *exp)
{
	int i;
	expr_plist_chunk_t *chunk = &(E_PLIST_CHUNK(*exp));

	printf("{\"data_type\": ");
	print_expr2(chunk->data_type);

	printf(", \"identifiers\": [");
	for (i = 0; i < chunk->identifier_count; i++) {
		print_expr2(chunk->identifiers[i]);
		if (i + 1 < chunk->identifier_count)
			printf(", ");
	}
	printf("], \"reference\": %s", chunk->is_reference ? "true" : "false");

	printf("}");
}

static void
print_expr2_field_list_case(expr2_t *exp)
{
	int i;
	expr_field_list_case_t *case_st = &(E_FIELD_LIST_CASE(*exp));

	printf("{\"identifier\": ");
	if (case_st->identifier == NULL) {
		printf("null");
	} else {
		print_expr2(case_st->identifier);
	}
	printf(", \"type\": ");
	print_expr2(case_st->type);
	printf(", \"cases\": [");
	for (i = 0; i < case_st->rows_count; i++) {
		print_expr2(case_st->rows[i]);
		if (i + 1 < case_st->rows_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_field_list_row(expr2_t *exp)
{
	int i;
	expr_field_list_row_t *row = &(E_FIELD_LIST_ROW(*exp));

	printf("{\"data_type\": ");
	print_expr2(row->data_type);

	printf(", \"identifiers\": [");
	for (i = 0; i < row->identifier_count; i++) {
		print_expr2(row->identifiers[i]);
		if (i + 1 < row->identifier_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_field_list_case_row(expr2_t *exp)
{
	int i;
	expr_field_list_case_row_t *row = &(E_FIELD_LIST_CASE_ROW(*exp));

	printf("{\"consts\": [");
	for (i = 0; i < row->constant_count; i++) {
		print_expr2(row->constants[i]);
		if (i + 1 < row->constant_count)
			printf(", ");
	}
	printf("], \"fields\": ");
	print_expr2(row->field_list);
	printf("}");
}

static void
print_expr2_field_list(expr2_t *exp)
{
	int i;
	expr_field_list_t *flist = &(E_FIELD_LIST(*exp));

	printf("{\"chunks\": [");
	for (i = 0; i < flist->row_count; i++) {
		print_expr2(flist->rows[i]);
		if (i + 1 < flist->row_count)
			printf(", ");
	}
	printf("], \"cases\": ");
	if (flist->case_stmt == NULL) {
		printf("null");
	} else {
		print_expr2_field_list_case(flist->case_stmt);
	}
	printf("}");
}

static void
print_expr2_stmt_label(expr2_t *exp)
{
	printf("{\"label\": ");
	print_expr2(E_STMT_LABEL(*exp).label);
	printf(", \"expression\": ");
	print_expr2(E_STMT_LABEL(*exp).nested);
	printf("}");
}

static void
print_expr2_stmt_assign(expr2_t *exp)
{
	printf("{\"identifier\": ");
	print_expr2(E_STMT_ASSIGNMENT(*exp).identifier);
	printf(", \"expression\": ");
	print_expr2(E_STMT_ASSIGNMENT(*exp).expression);
	printf("}");
}

static void
print_expr2_stmt_function_call(expr2_t *exp)
{
	int i;
	expr_stmt_function_call_t *call = &(E_STMT_FUNCTION_CALL(*exp));

	printf("{\"identifier\": ");
	print_expr2(call->identifier);
	printf(", \"arguments\": [");
	for (i = 0; i < call->param_count; i++) {
		print_expr2(call->params[i]);
		if (i + 1 < call->param_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_stmt_begin(expr2_t *exp)
{
	int i;
	expr_stmt_begin_t *begin = &(E_STMT_BEGIN(*exp));
	printf("{\"statements\": [");
	for (i = 0; i < begin->statement_count; i++) {
		print_expr2(begin->statements[i]);
		if (i + 1 < begin->statement_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_if_stmt(expr2_t *exp)
{
	expr_stmt_if_t *ifstmt = &(E_STMT_IF(*exp));
	printf("{\"condition\": ");
	print_expr2(ifstmt->condition);
	printf(", \"then\": ");
	print_expr2(ifstmt->then);
	if (ifstmt->else_) {
		printf(", \"else\": ");
		print_expr2(ifstmt->else_);
	} else {
		printf(", \"else\": null");
	}
	printf("}");
}

static void
print_expr2_repeat_stmt(expr2_t *exp)
{
	int i;
	expr_stmt_repeat_t *repeat = &(E_STMT_REPEAT(*exp));

	printf("{\"statements\": [");
	for (i = 0; i < repeat->statement_count; i++) {
		print_expr2(repeat->statements[i]);
		if (i + 1 < repeat->statement_count)
			printf(", ");
	}
	printf("], \"condition\": ");
	print_expr2(repeat->condition);
	printf("}");
}

static void
print_expr2_while_stmt(expr2_t *exp)
{
	expr_stmt_while_t *whilestmt = &(E_STMT_WHILE(*exp));

	printf("{\"condition\": ");
	print_expr2(whilestmt->condition);
	printf(", \"statement\": ");
	print_expr2(whilestmt->statement);
	printf("}");
}

static void
print_expr2_for_stmt(expr2_t *exp)
{
	expr_stmt_for_t *forstmt = &(E_STMT_FOR(*exp));

	printf("{\"identifier\": ");
	print_expr2(forstmt->identifier);
	printf(", \"start\": ");
	print_expr2(forstmt->start);
	printf(", \"end\": ");
	print_expr2(forstmt->end);
	printf(", \"direction\": %d, \"statement\": ", forstmt->direction);
	print_expr2(forstmt->statement);
	printf("}");
}

static void
print_expr2_case_branch_stmt(expr2_t *exp)
{
	int i;
	expr_stmt_case_branch_t *branch = &(E_STMT_CASE_BRANCH(*exp));

	printf("{\"constants\": [");
	for (i = 0; i < branch->constant_count; i++) {
		print_expr2(branch->constants[i]);
		if (i + 1 < branch->constant_count)
			printf(", ");
	}
	printf("], \"statement\": ");
	print_expr2(branch->statement);
	printf("}");
}

static void
print_expr2_case_stmt(expr2_t *exp)
{
	int i;
	expr_stmt_case_t *casestmt = &(E_STMT_CASE(*exp));

	printf("{\"expression\": ");
	print_expr2(casestmt->expression);
	printf(", \"conditions\": [");
	for (i = 0; i < casestmt->case_count; i++) {
		print_expr2(casestmt->cases[i]);
		if (i + 1 < casestmt->case_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_with_stmt(expr2_t *exp)
{
	int i;
	expr_stmt_with_t *with = &(E_STMT_WITH(*exp));

	printf("{\"variables\": [");
	for (i = 0; i < with->variable_count; i++) {
		print_expr2(with->variables[i]);
		if (i + 1 < with->variable_count)
			printf(", ");
	}
	printf("], \"statement\": ");
	print_expr2(with->statement);
	printf("}");
}

static void
print_expr2_goto_stmt(expr2_t *exp)
{
	expr_stmt_goto_t *gotostmt = &(E_STMT_GOTO(*exp));
	printf("{\"identifier\": ");
	print_expr2(gotostmt->identifier);
	printf("}");
}

static void
print_expr2_exit_stmt(expr2_t *exp)
{
	expr_stmt_exit_t *exitstmt = &(E_STMT_EXIT(*exp));
	printf("{\"program\": %s, \"identifier\": ",
	       exitstmt->program ? "true" : "false");
	if (exitstmt->identifier) {
		print_expr2(exitstmt->identifier);
	} else {
		printf("null");
	}
	printf("}");
}

static void
print_expr2_block(expr2_t *exp)
{
	int i;
	expr_block_t *block = &(E_BLOCK(*exp));

	printf("{\"preblocks\": [");
	for (i = 0; i < block->preblock_count; i++) {
		print_expr2(block->preblocks[i]);
		if (i + 1 < block->preblock_count)
			printf(", ");
	}
	printf("], \"statements\": [");
	for (i = 0; i < block->statement_count; i++) {
		print_expr2(block->statements[i]);
		if (i + 1 < block->statement_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_block_label(expr2_t *exp)
{
	int i;
	expr_block_label_t *label = &(E_BLOCK_LABEL(*exp));

	printf("{\"labels\": [");
	for (i = 0; i < label->label_count; i++) {
		print_expr2(label->labels[i]);
		if (i + 1 < label->label_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_block_const(expr2_t *exp)
{
	int i;
	expr_block_const_t *constb = &(E_BLOCK_CONST(*exp));

	printf("{\"constants\": [");
	for (i = 0; i < constb->line_count; i++) {
		print_expr2(constb->lines[i]);
		if (i + 1 < constb->line_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_block_const_line(expr2_t *exp)
{
	expr_block_const_line_t *line = &(E_BLOCK_CONST_LINE(*exp));
	printf("{\"name\": ");
	print_expr2(line->name);
	printf(", \"value\": ");
	print_expr2(line->value);
	printf("}");
}

static void
print_expr2_block_type(expr2_t *exp)
{
	int i;
	expr_block_type_t *typeb = &(E_BLOCK_TYPE(*exp));

	printf("{\"types\": [");
	for (i = 0; i < typeb->line_count; i++) {
		print_expr2(typeb->lines[i]);
		if (i + 1 < typeb->line_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_block_type_line(expr2_t *exp)
{
	expr_block_type_line_t *line = &(E_BLOCK_TYPE_LINE(*exp));
	printf("{\"name\": ");
	print_expr2(line->name);
	printf(", \"value\": ");
	print_expr2(line->value);
	printf("}");
}

static void
print_expr2_block_var(expr2_t *exp)
{
	int i;
	expr_block_var_t *varb = &(E_BLOCK_VAR(*exp));

	printf("{\"types\": [");
	for (i = 0; i < varb->line_count; i++) {
		print_expr2(varb->lines[i]);
		if (i + 1 < varb->line_count)
			printf(", ");
	}
	printf("]}");
}

static void
print_expr2_block_var_line(expr2_t *exp)
{
	int i;
	expr_block_var_line_t *line = &(E_BLOCK_VAR_LINE(*exp));

	printf("{\"identifiers\": [");
	for (i = 0; i < line->identifier_count; i++) {
		print_expr2(line->identifiers[i]);
		if (i + 1 < line->identifier_count)
			printf(", ");
	}
	printf("], \"type\": ");
	print_expr2(line->type);
	printf("}");
}

static void
print_expr2_block_procedure(expr2_t *exp)
{
	expr_block_procedure_t *proc = &(E_BLOCK_PROCEDURE(*exp));
	printf("{\"identifier\": ");
	print_expr2(proc->identifier);
	printf(", \"parameters\": ");
	print_expr2(proc->parlist);
	printf(", \"block\": ");
	print_expr2(proc->block);
	printf("}");
}

static void
print_expr2_block_function(expr2_t *exp)
{
	expr_block_function_t *func = &(E_BLOCK_FUNCTION(*exp));
	printf("{\"identifier\": ");
	print_expr2(func->identifier);
	printf(", \"parameters\": ");
	print_expr2(func->parlist);
	printf(", \"returntype\": ");
	print_expr2(func->returntype);
	printf(", \"block\": ");
	print_expr2(func->block);
	printf("}");
}

static void
print_expr2_program(expr2_t *exp)
{
	int i;
	expr_program_t *program = &(E_PROGRAM(*exp));

	printf("{\"identifier\": ");
	print_expr2(program->identifier);
	printf(", \"subidentifiers\": [");
	for (i = 0; i < program->subident_count; i++) {
		print_expr2(program->subidents[i]);
		if (i + 1 < program->subident_count)
			printf(", ");
	}
	printf("], \"block\": ");
	print_expr2(program->block);
	printf("}");
}

static void
print_expr2(expr2_t *exp)
{
	char *type;
	print_expr2_detail detail_func;

	switch (exp->type) {
	case EXP_IDENTIFIER:
		type = "Identifier";
		detail_func = print_expr2_identifier;
		break;
	case EXP_UNSIGNED_NUMBER:
		type = "UnsignedNumber";
		detail_func = print_expr2_unsigned_number;
		break;
	case EXP_UNSIGNED_INTEGER:
		type = "UnsignedInteger";
		detail_func = print_expr2_unsigned_integer;
		break;
	case EXP_VARIABLE:
		type = "Variable";
		detail_func = print_expr2_variable;
		break;
	case EXP_VARIABLE_PATH_DOT:
		type = "VariableDot";
		detail_func = print_expr2_variable_path_dot;
		break;
	case EXP_VARIABLE_PATH_CARET:
		type = "VariableCaret";
		detail_func = NULL;
		break;
	case EXP_VARIABLE_PATH_ARRAY:
		type = "VariableArray";
		detail_func = print_expr2_variable_path_array;
		break;
	case EXP_NIL:
		type = "NilConstant";
		detail_func = NULL;
		break;
	case EXP_STRING:
		type = "String";
		detail_func = print_expr2_string;
		break;
	case EXP_UNSIGNED_CONSTANT:
		type = "UnsignedConstant";
		detail_func = print_expr2_unsigned_constant;
		break;
	case EXP_CONSTANT:
		type = "Constant";
		detail_func = print_expr2_constant;
		break;
	case EXP_EXPRESSION:
		type = "Expression";
		detail_func = print_expr2_expression;
		break;
	case EXP_SIMPLE_TYPE:
		type = "SimpleType";
		detail_func = print_expr2_simple_type;
		break;
	case EXP_REFERENCE_TYPE:
		type = "ReferenceType";
		detail_func = print_expr2_ref_type;
		break;
	case EXP_SET_TYPE:
		type = "SetType";
		detail_func = print_expr2_set_type;
		break;
	case EXP_RECORD_TYPE:
		type = "RecordType";
		detail_func = print_expr2_record_type;
		break;
	case EXP_ARRAY_TYPE:
		type = "ArrayType";
		detail_func = print_expr2_array_type;
		break;
	case EXP_FILE_TYPE:
		type = "FileType";
		detail_func = print_expr2_file_type;
		break;
	case EXP_PLIST:
		type = "ParameterList";
		detail_func = print_expr2_plist;
		break;
	case EXP_PLIST_CHUNK:
		type = "ParameterListChunk";
		detail_func = print_expr2_plist_chunk;
		break;
	case EXP_FIELD_LIST:
		type = "FieldList";
		detail_func = print_expr2_field_list;
		break;
	case EXP_FIELD_LIST_ROW:
		type = "FieldListRow";
		detail_func = print_expr2_field_list_row;
		break;
	case EXP_FIELD_LIST_CASE:
		type = "FieldListCase";
		detail_func = print_expr2_field_list_case;
		break;
	case EXP_FIELD_LIST_CASE_ROW:
		type = "FieldListCaseRow";
		detail_func = print_expr2_field_list_case_row;
		break;
	case EXP_STMT_LABEL:
		type = "StatementLabel";
		detail_func = print_expr2_stmt_label;
		break;
	case EXP_STMT_ASSIGN:
		type = "StatementAssign";
		detail_func = print_expr2_stmt_assign;
		break;
	case EXP_STMT_FUNCTION_CALL:
		type = "StatementFunctionCall";
		detail_func = print_expr2_stmt_function_call;
		break;
	case EXP_STMT_BEGIN:
		type = "StatementBegin";
		detail_func = print_expr2_stmt_begin;
		break;
	case EXP_STMT_IF:
		type = "StatementIf";
		detail_func = print_expr2_if_stmt;
		break;
	case EXP_STMT_REPEAT:
		type = "StatementRepeat";
		detail_func = print_expr2_repeat_stmt;
		break;
	case EXP_STMT_WHILE:
		type = "StatementWhile";
		detail_func = print_expr2_while_stmt;
		break;
	case EXP_STMT_FOR:
		type = "StatementFor";
		detail_func = print_expr2_for_stmt;
		break;
	case EXP_STMT_CASE_BRANCH:
		type = "StatementCaseBranch";
		detail_func = print_expr2_case_branch_stmt;
		break;
	case EXP_STMT_CASE:
		type = "StatementCase";
		detail_func = print_expr2_case_stmt;
		break;
	case EXP_STMT_WITH:
		type = "StatementWith";
		detail_func = print_expr2_with_stmt;
		break;
	case EXP_STMT_GOTO:
		type = "StatementGoto";
		detail_func = print_expr2_goto_stmt;
		break;
	case EXP_STMT_EXIT:
		type = "StatementExit";
		detail_func = print_expr2_exit_stmt;
		break;
	case EXP_BLOCK:
		type = "Block";
		detail_func = print_expr2_block;
		break;
	case EXP_BLOCK_LABEL:
		type = "BlockLabel";
		detail_func = print_expr2_block_label;
		break;
	case EXP_BLOCK_CONST:
		type = "BlockConst";
		detail_func = print_expr2_block_const;
		break;
	case EXP_BLOCK_CONST_LINE:
		type = "BlockConstLine";
		detail_func = print_expr2_block_const_line;
		break;
	case EXP_BLOCK_TYPE:
		type = "BlockType";
		detail_func = print_expr2_block_type;
		break;
	case EXP_BLOCK_TYPE_LINE:
		type = "BlockTypeLine";
		detail_func = print_expr2_block_type_line;
		break;
	case EXP_BLOCK_VAR:
		type = "BlockVar";
		detail_func = print_expr2_block_var;
		break;
	case EXP_BLOCK_VAR_LINE:
		type = "BlockVarLine";
		detail_func = print_expr2_block_var_line;
		break;
	case EXP_BLOCK_PROCEDURE:
		type = "BlockProcedure";
		detail_func = print_expr2_block_procedure;
		break;
	case EXP_BLOCK_FUNCTION:
		type = "BlockFunction";
		detail_func = print_expr2_block_function;
		break;
	case EXP_PROGRAM:
		type = "Program";
		detail_func = print_expr2_program;
		break;
	default:
		type = NULL;
		detail_func = NULL;
	}

	if (type != NULL) {
		printf("{\"type\": \"%s\"", type);
		if (detail_func) {
			printf(", \"data\": ");
			detail_func(exp);
		}
		printf("}");
	}
}

static void
read_file()
{
	int offt = 0;
	char partial_buffer[FGETS_SIZE];

	while (fgets(partial_buffer, FGETS_SIZE, stdin) != NULL) {
		// append whatever we have read into the buffer
		strncpy(buffer + offt, partial_buffer, FGETS_SIZE);
		offt += strnlen(partial_buffer, FGETS_SIZE);
	}
}

static int
read_keyboard()
{
	int offt = 0;
	char partial_buffer[FGETS_SIZE];
	int is_new_line;

	// mark the start of the string with a zero to detect empty string
	buffer[0] = 0;

	printf(">>> ");

	while (fgets(partial_buffer, FGETS_SIZE, stdin) != NULL) {
		char *pbuf_start = partial_buffer;
		int partial_buffer_len = strnlen(partial_buffer, FGETS_SIZE);

		// skip whitespace at the beginning of the read line and treat
		// empty strings as if no characters had been read
		while (*pbuf_start == '\n' || *pbuf_start == '\r'
		       || *pbuf_start == '\t')
			pbuf_start++;
		if (*pbuf_start == 0) {
			break;
		}

		// append whatever we have read into the buffer
		strncpy(buffer + offt, pbuf_start, FGETS_SIZE);
		offt += strnlen(pbuf_start, FGETS_SIZE);

		// check if partial_buffer ended with line break
		is_new_line = partial_buffer[partial_buffer_len - 1] == '\n';
		if (is_new_line) {
			printf("... ");
		}
	}
	return offt;
}

int
eval_code()
{
	int successful = 1;
	scanner_t *scanner;
	parser_t *parser;

	scanner = scanner_init(buffer, strnlen(buffer, BUFFER_SIZE));
	if (!scanner) {
		successful = 0;
		goto pre_cleanup_scanner;
	}
	parser = parser_new();
	if (!parser) {
		successful = 0;
		goto pre_cleanup_parser;
	}

	parser_load_tokens(parser, scanner);
	expr2_t *ident = parser2_program(parser);
	print_expr2(ident);

pre_cleanup_parser:
	if (parser)
		parser_free(parser);
pre_cleanup_scanner:
	if (scanner)
		scanner_free(scanner);
	return successful;
}

void
main_loop()
{
	int len;
	do {
		len = read_keyboard();
		if (len > 0) {
			eval_code();
			printf("\n");
		}
	} while (len > 0);
}

void
single_read()
{
	read_file();
	eval_code();
}

int
main(int argc, char **argv)
{
	strcpy(buffer, "");

	if (isatty(0)) {
		puts("Entering REPL. Send empty prompt to exit.");
		main_loop();
	} else {
		single_read();
	}
}
