# hvn-pkgen 1 2020-07-28 HeylelOS

## NAME
Create a package by executing a list of executables in a constrained environment.

## SYNOPSIS
- **hvn-pkgen** [-l log] -a app -d data -t toolchain recipes...

## DESCRIPTION
Run a list of command in an isolated root hierarchy and system subject to a specific toolchain.
The goal is to create a set of executables (or scripts) and an environement called and app.
You can then specify a sequence of recipes to execute one after another.
In the end, you create a stable ecosystem for compilation, packaging, tests you can easily reuse
just by modifying one recipe at a time or changing the toolchain.
It can be compared to a container without security concerns, only exploiting isolation capacities.
When a recipe is executed, it's environment is composed of the following hierarchy.
- **/dev** : Usual device filesystem.
- **/sys** : Usual sysfs.
- **/proc** : Usual procfs.
- **/app** : Read only binding to the specified app directory.
- **/app/environ** : File containing a line-separated list of environment variables.
- **/app/\<recipe\>** : An executable recipe of the app.
- **/data** : Exported data, working directory of each ran recipes.
- **/usr** : Read only binding to the specified toolchain directory.
- **/bin** : Symbolic link to "usr/bin".

## OPTIONS
- -l log : Specify a surrogate stdout/stderr for the entire process.
- -a app : Directory containing the **environ** environment file, and all recipes.
- -d data : Directory which can be used to transfer data in and out executions, current working directory of all recipes.
- -t toolchain : Directory containing all utilities used to execute a script, fetch a repo, compile, link, etc...

## AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

## SEE ALSO
hvn(1), hvn-cfgen(1), hvn-init(1), hvn-mando(1), hvn-mkgen(1), namespaces(7), user\_namespaces(7).

