build: task1.c LineParser.c LineParser.h test.c
	gcc -m32 -Wall -g  -o myShell task1.c



clean:
	rm -f myShell

