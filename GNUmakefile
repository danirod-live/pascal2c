.PHONY: test

build/repl: build
	make -C build

build:
	cmake -B build

test: build/repl
	test/runtest.sh
