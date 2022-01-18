# Documentation Rules for metamath.exe

This is a **draft**.   It was suggested to move parts or all of this to a
different location, in particular to metamath/set.mm/CONTRIBUTING.md,
where it can serve as a template for a wider audience.

Here are some links on how to improve documentation in general:

https://www.oreilly.com/content/the-eight-rules-of-good-documentation/

https://guides.lib.berkeley.edu/how-to-write-good-documentation

---

1. Documentation is mostly collected in the ``doc`` subfolder of metamath.  As
    an entry point to documentation the following files are kept in the main
    folder of Metamath:
    - README.TXT
    - LICENSE.TXT.
    Both files are pure [ASCII](https://en.wikipedia.org/wiki/ASCII) encoded
    [plain text](https://en.wikipedia.org/wiki/Plain_text) files.
    
The following rules are strong recommendations.  We encourage to follow them as
much as possible.  But in the end we think that some sort of documentation is
better than no documentation, so bypassing any of them is possible if somehow
inevitable.
    
2. If your documentation is simple enough to be expressed as
    [plain text](https://en.wikipedia.org/wiki/Plain_text), we recommend to
    rely as much as possible on [ASCII](https://en.wikipedia.org/wiki/ASCII)
    encoding.  If you need special characters, fall back to
    [UTF-8](https://en.wikipedia.org/wiki/UTF-8).

3. If your documentation is best viewed with some formatting in effect we
    recommend using GitHub's markdown extensions (GFM) as a format style
    embedded in plain text.  This rich text formatting is designed to be still
    legible in simple editors.  Its format is in detail documented
    [here](https://github.github.com/gfm), and is an extension to the
    widespread [Markdown](https://commonmark.org/help/) format.
    
4. Source code documentation within sources should follow the
    [Doxygen](https://www.doxygen.nl/index.html) style, so documentation of
    variables and functions can be generated automatically.  Doxygen extracts
    marked comment blocks from C sources and creates HTML pages based upon them.
    In addition it supports Markdown to format descriptions appropriately.  See
    https://www.doxygen.nl/manual/docblocks.html.
