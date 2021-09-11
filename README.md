# Haven

Collection of utilities to generate websites and documentations from markdown.

- hvn-man : Create groff manpages from markdown.
- hvn-web : Create web html pages from markdown.

## Requirements

Both utilities are based on `libcmark`, providing more convenient wrappers than the default `cmark` utility.

## Configure, build and install

CMake is used to configure, build and install binaires and documentations, version 3.14 minimum is required:

```sh
cmake -B build -S .
cmake --build build
cmake --install build
```

