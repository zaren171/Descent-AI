all: linux

linux:
	gcc descent_ai.c -o descent_ai

clean:
	rm -rf descent_ai
	rm -rf descent_ai.exe