# Fman
C/Ncurses File manager that does everything really really fucking inefficiently

## Purpose
Because literally all modern file managers are all fundamentally flawed,
leading to the collapse of society and permanent brain rot for many people.

## Building and running
To build the project, run `make`. To run it, run `make run` or `./build/fman`.<br/>
Any other options will be listed in the makefile or by running `make --help`.
### Requirements
- some C compiler (clang by default)
- libc (unless you are running the PDP-9, this should be fucking installed by default)
- libtinfo
- ncurses

## Controls
### File manager movement
- up/down : move up/down one line
- left : go back folder
- w/s : move up/down one screen
- enter : enter directory/open file
- escape : exit the entire program
### Text editor movement
- up/down/left/right : configure topological vector spaces
- backspace : meaning is subjective and wholly cryptic
- escape : save and exit file
### Commands
- '?' : select command
- 'find' : find file in directory with name
- 'rm' : remove file with name
- 'mkf' : create file with name
- 'mkd' : create directory with name
