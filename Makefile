all: linux cygwin windows

linux:
	gcc descent_ai.c -o descent_ai -D LINUX

cygwin:
	gcc descent_ai.c -o descent_ai_cyg.exe -D LINUX
	
windows:
	i686-w64-mingw32-gcc descent_ai.c -o descent_ai_win.exe -D WINDOWS

release:
	zip -r ./releases/Descent_AI_Win ./Quests descent_ai_win.exe
	zip -r ./releases/Descent_AI_Cyg ./Quests descent_ai_Cyg.exe
	zip -r ./releases/Descent_AI_Lin ./Quests descent_ai
	
clean:
	rm -rf import.txt
	rm -rf save.sav
	rm -rf descent_ai
	#this is here because cygwin adds .exe to the linux build
	rm -rf descent_ai.exe
	rm -rf descent_ai_cyg.exe
	rm -rf descent_ai_win.exe

tag: 
	#NOTE: import.txt needs to be created, and will be deleted by make clean.  This is a safety to make sure tags are not accidentally created.
	svn commit . -F import.txt
	svn cp -F import.txt https://github.com/zaren171/Descent-AI.git/trunk https://github.com/zaren171/Descent-AI.git/tags/V0.1