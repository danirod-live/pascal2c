pascal2c is at the moment an Pascal AST interpreter currently in
development. The name is misleading because it may give the
impression that it converts Pascal code to C code, and this is the
intention, but at the moment I'll only work in the AST library
to decode and understand Pascal code, and maybe later I'll work in
a tool that spits the equivalent C code.

How to compile this
===================

Install CMake. Then:

		cmake -B build
		make -C build


What can I do with this?
========================

Currently nothing. Come back when the parser is able to generate
a tree for an entire Pascal source code file.


Work in progress
================

Current tasks (non-finite list):

- [x] Write a scanner that can convert an input into Pascal tokens
- [!] Write a parser that converts Pascal tokens into the expressions
- [ ] ??????

An interesting thing to test later is that the expression parser is
properly written by checking if I can convert back every expression into
the equivalent Pascal code.

Parts of the compiler grammar that are implemented:

- [x] Identifier
- [x] Variable
- [x] Unsigned number
- [x] Expression
- [x] Simple expression
- [x] Term
- [x] Factor
- [x] Unsigned constant
- [x] Parameter list
- [x] Unsigned integer
- [x] Constant
- [ ] Simple type
- [x] Field list
- [x] Type
- [ ] Statement
- [ ] Block
- [ ] Program


Bugs:

* parser_variable is not working (try "hello.world" or "hello[1]" and see)
