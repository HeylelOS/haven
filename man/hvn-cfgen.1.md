# hvn-cfgen 1 2020-06-24 HeylelOS

## NAME
Generate configure script.

## SYNOPSIS
- **hvn-cfgen** [-S sources] [-l languages] [filename]

## DESCRIPTION
Generate a configure script suitable to create a Makefile for hvn-based Makefile rules.

## OPTIONS
- -S : Sources, where to look up for modules.
- -l : Comma separated list of builtin languages compilers to look for in the system. Supports C and C++.
- filename : Name of the configure script to create, by default configure.

## AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

## SEE ALSO
hvn(1), hvn-init(1), hvn-mkgen(1), hvn-mando(1), hvn-pkgen(1).

