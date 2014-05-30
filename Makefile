musique : multipoint
	./multipoint /dev/input/wacom-touch

multipoint : multipoint.c Makefile
	gcc -Os -Wall -Wextra multipoint.c -o multipoint -lSDL2 -lm  
	
clean : 
	rm multipoint.c
