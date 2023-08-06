package main

import (
	"fmt"
	"io"
	"os"

	"danirod.es/pkg/pascal2c/tokenizer"
)

func getTargetFile() (*os.File, error) {
	if len(os.Args) > 1 {
		filename := os.Args[1]
		return os.Open(filename)
	}
	return os.Stdin, nil
}

func main() {
	file, err := getTargetFile()
	if err != nil {
		panic(err) // TODO
	}
	defer file.Close()

	contents, err := io.ReadAll(file)
	if err != nil {
		panic(err) // TODO
	}

	tokenizer := tokenizer.NewTokenizer(contents)
	for token := tokenizer.NextToken(); !token.Eof(); token = tokenizer.NextToken() {
		fmt.Println(token.String())
	}
}
