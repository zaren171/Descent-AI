# Descent-AI
AI program to automate the Overlord player in Descent, Journeys in the Dark 1st Edition

The base of this work was performed, for the most part, in Cygwin.  Cygwin does closely mirror a Linux environment, but there may be some differences between the comments made in this document and running in Linux (This document is written from a Cygwin perspective).  The benefit driving the base work in Cygwin is the ability to build in a Linux-like shell, while also being able to output Windows executable files for a more general audience.

~~File and Folder Descriptions~~
/Quests = a folder that contains all of the quest files
descent_ai.c = c code for the ai
Makefile = Makefile used for building the ai
README.md = this file

~~Building the AI~~
There are separate build flows depending on the operating system you are using.  This section outlines the various commands and their functions.  
"make all" = Run all build flows, then makes releases
"make linux" = Run Linux build flow, executable called descent_ai (if built in cygwin .exe will be added)
"make cygwin" = Same as Linux, but executable is called descent_ai_cyg.exe
"make windows" = Run Windows build flow, executable called descent_ai_win.exe
"make releases" = Makes releases folder, bundles each executable with quests folder in a .zip archive
"make clean" = remove unnecessary files

