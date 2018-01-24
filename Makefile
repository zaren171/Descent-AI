all: linux cygwin windows release

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

tag: all
	svn cp -F import.txt https://github.com/zaren171/Descent-AI.git/trunk https://github.com/zaren171/Descent-AI.git/tags/V0.1
	svn import -F import.txt releases/Descent_AI_Cyg.zip https://github.com/zaren171/Descent-AI.git/tags/V0.1/releases/Descent_AI_Cyg.zip
	svn import -F import.txt releases/Descent_AI_Win.zip https://github.com/zaren171/Descent-AI.git/tags/V0.1/releases/Descent_AI_Win.zip
	svn import -F import.txt releases/Descent_AI_Lin.zip https://github.com/zaren171/Descent-AI.git/tags/V0.1/releases/Descent_AI_Lin.zip