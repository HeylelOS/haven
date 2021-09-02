# NAME
hvn-web - Format markdown html pages.

# SYNOPSIS
**hvn-web** [-i \<input\>] [-o \<output\>] [-H \<head\>] [-T \<tail\>] [\<render options\>...]

# DESCRIPTION
Ease creation of webpages by redacting them in markdown, and then format in html.

# OPTIONS

-i \<input\> : Input file, reads from stdin if none specified.

-o \<output\> : Output file, renders in the current directory, into a file with input's basename with a stripped extension.

-H \<head\> : HTML head file to insert into the output before the input's content.

-T \<tail\> : HTML tail file to insert into the output after the input's content.

# RENDER OPTIONS

sourcepos : Include source position attribute.

hardbreaks : Render soft breaks (newlines inside paragraphs in the CommonMark source) as hard line breaks in the target format.

nobreaks : Render soft breaks as spaces.

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
hvn-man(1), cmark(3).

