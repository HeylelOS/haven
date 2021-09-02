# NAME
hvn-man - Format markdown to groff manpages.

# SYNOPSIS
**hvn-man** [-i \<input\>] [-o \<output\>] [-p \<pagename\>] [-s \<section\>] [-w \<width\>] [\<render options\>...]

# DESCRIPTION
Ease creation of manpages by redacting them in markdown, and then format in groff.

# OPTIONS

-i \<input\> : Input file, reads from stdin if none specified.

-o \<output\> : Output file, renders in the current directory, into a file with input's basename with a stripped extension.

-p \<pagename\> : Manpage name, parsed from output filename. Defaults to `?` if impossible to determine.

-s \<section\> : Manpage section number, between 1 and 8 inclusive, parsed from output name. Defaults to 1 if impossible to determine.

-w \<width\> : Wrapping width, 0 means no wrap, defaults to 0.

# RENDER OPTIONS

sourcepos : Include source position attribute.

hardbreaks : Render soft breaks (newlines inside paragraphs in the CommonMark source) as hard line breaks in the target format.
If this option is specified, hard wrapping is disabled for CommonMark output, regardless of the wrapping width.

nobreaks : Render soft breaks as spaces. If this option is specified, hard wrapping is disabled for all output formats, regardless of the wrapping width.

smart : Use smart punctuation. Straight double and single quotes will be rendered as curly quotes, depending on their position.
`--` will be rendered as an en-dash. `---` will be rendered as an em-dash. `...` will be rendered as ellipses.

safe : Omit raw HTML and potentially dangerous URLs (default).
Raw HTML is replaced by a placeholder comment and potentially dangerous URLs are replaced by empty strings.
Potentially dangerous URLs are those that begin with `javascript:`, `vbscript:`, `file:`, or `data:`
(except for `image/png`, `image/gif`, `image/jpeg`, or `image/webp` mime types).

unsafe : Render raw HTML or potentially dangerous URLs, overriding the safe behavior.

validate-utf8 : Validate UTF-8, replacing illegal sequences with U+FFFD.

# AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

# SEE ALSO
hvn-web(1), man(1), cmark(3).

