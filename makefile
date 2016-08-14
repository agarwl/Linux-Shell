all:
	g++ server-slow.c  -pthread  -o server-slow	
	g++ get-one-file-sig.c  -o get-one-file-sig
	g++ get-one-file.c  -o get-one-file
	g++ client-shell.c -pthread  -o client-shell