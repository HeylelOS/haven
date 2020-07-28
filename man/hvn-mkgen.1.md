# hvn-mkgen 1 2020-06-25 HeylelOS

## NAME
Create Makefile rules.

## SYNOPSIS
- **hvn-mkgen** [-S sources] [-d dependencies] [filename]

## DESCRIPTION
The program will analyze the hierarchy under sources, and for each module create a link target.
Each module is either considered a library or an executable, depending on whether they are prefixed with lib or not. Then, each file which has a rule associated with its extension is created a target. The target is the name of the source, with the extension replaced with .o. Note that two files with the same basenames will create a target collision!

## OPTIONS
- -S : Sources, where to look up for modules, by default src.
- -d : Dependency list for a module. It starts with the name of the module, separated of its dependencies with an equal character, then followed by a comma separated list of other modules it depends on.
- filename : Name of the created files containing the rules, by default Makefile.rules.

## AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

## SEE ALSO
hvn(1), hvn-cfgen(1), hvn-init(1), hvn-mando(1), hvn-pkgen(1).

