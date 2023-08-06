package tokenizer

type TokenType uint

const (
	TokenEof = iota

	TokenAssign      // :=
	TokenAsterisk    // *
	TokenBegin       // BEGIN
	TokenColon       // :
	TokenComma       // ,
	TokenControlCode // #24
	TokenConst       // CONST
	TokenDigit       // 1234567890
	TokenDiv         // DIV
	TokenDot         // .
	TokenEnd         // END
	TokenEqual       // =
	TokenIdentifier  // println
	TokenLparen      // (
	TokenMinus       // -
	TokenMod         // MOD
	TokenPlus        // +
	TokenProgram     // PROGRAM
	TokenSlash       // /
	TokenString      // 'hello'
	TokenRparen      // )
	TokenSemicolon   // ;
	TokenVar         // VAR
)

var keywordTokens = map[string]TokenType{
	"begin":   TokenBegin,
	"const":   TokenConst,
	"div":     TokenDiv,
	"end":     TokenEnd,
	"mod":     TokenMod,
	"program": TokenProgram,
	"var":     TokenVar,
}

var stringTokens = map[TokenType]string{
	TokenEof:         "TOK_EOF",
	TokenAssign:      "TOK_ASSIGN",
	TokenAsterisk:    "TOK_ASTERISK",
	TokenBegin:       "TOK_BEGIN",
	TokenColon:       "TOK_COLON",
	TokenComma:       "TOK_COMMA",
	TokenConst:       "TOK_CONST",
	TokenControlCode: "TOK_CONTROLCODE",
	TokenDigit:       "TOK_DIGIT",
	TokenDiv:         "TOK_DIV",
	TokenDot:         "TOK_DOT",
	TokenEnd:         "TOK_END",
	TokenEqual:       "TOK_EQUAL",
	TokenIdentifier:  "TOK_IDENTIFIER",
	TokenLparen:      "TOK_LPAREN",
	TokenMinus:       "TOK_MINUS",
	TokenMod:         "TOK_MOD",
	TokenPlus:        "TOK_PLUS",
	TokenProgram:     "TOK_PROGRAM",
	TokenRparen:      "TOK_RPAREN",
	TokenSemicolon:   "TOK_SEMICOLON",
	TokenSlash:       "TOK_SLASH",
	TokenString:      "TOK_STRING",
	TokenVar:         "TOK_VAR",
}

func (t TokenType) String() string {
	if rep, found := stringTokens[t]; found {
		return rep
	}
	return "TOK_UNKNOWN"
}
