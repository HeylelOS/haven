# hvn-init 1 2020-06-25 HeylelOS

## NAME
Initialize a module.

## SYNOPSIS
- **hvn-init** [-S sources] [-e extension] targets...

## DESCRIPTION
Create a subdirectory for each target in sources. And creates a hello world if it is not considered a library, and the language is supported.

## OPTIONS
- -S : Sources, where to create the modules, by default src.
- -e : Extension used for hello worlds, by default .c.
- targets : Names of the modules to create, a name prefixed with lib will be considered a library, else an executable.

## AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

## SEE ALSO
hvn(1), hvn-cfgen(1), hvn-mkgen(1), hvn-mando(1), hvn-pkgen(1).

