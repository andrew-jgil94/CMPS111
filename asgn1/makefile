make:	
	flex shell.l
	cc -c lex.yy.c
	cc -c argshell.c
	cc -o argshell argshell.o lex.yy.o -lfl

clean:
	$(RM) argshell
