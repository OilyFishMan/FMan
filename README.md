# F-man
C/Ncurses File manager

## Purpose
Because literally all modern file managers are all fundamentally flawed,
leading to the collapse of society and permanent brain rot for many people.

## Building and running
To build the project, run `make build`. To run it, run `make run` or `./build/fman`.<br/>
Any other options will be listed in the makefile or by running `make --help`.
### Requirements
- Clang : C compiler (or change it to gcc by doing)
- LibC : Will be installed as a dependency of clang for most package managers
- Ncurses : terminal graphics/input management library

## Controls
### Movement
- up/down : move up/down one line
- w/s : move up/down one screen
- enter : enter directory
### Commands
- '?' : find command with name
- 'find' : find file in directory with name
- 'rm' : remove file with name
- 'mkf' : create file with name
- 'mkd' : create directory with name
