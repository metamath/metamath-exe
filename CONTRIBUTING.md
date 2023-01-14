# Contributing and Acceptance

We're a friendly community, and we would love to have more collaborators!

## Maintenance mode

We view metamath-exe as being in maintenance mode. Although we do envisage
fixing bugs (at least serious ones) and making small updates to reflect
ongoing changes to our processes or metamath proof databases (especially
those in the https://github.com/metamath/set.mm repository), we do not
encourage developing significant new functionality in metamath-exe.

Why not? metamath.exe is the oldest of the metamath tools and is showing its
age in a lot of ways including that C is probably not the language that
anyone would choose for this kind of code today.

Here's what Norm Megill, who wrote metamath-exe and did almost all of
its maintenance until his passing in 2021, said:

> Another issue with the metamath program that people have requested is
> putting it on GitHub. However, strange as it may seem I actually don't
> want to encourage major contributions at this point in time. Almost
> every unsolicited patch I've been sent has had to be rewritten by me
> to fix memory leaks and other issues, and it can take a lot of my time
> to analyze them in complete detail. C is a dangerous language if you
> don't have intimate knowledge of all details of the program's data
> structures etc. If I put it on GitHub, I would dread having someone
> make a massive change that would require a week or more of my time
> to analyze. Ensuring bug-free code can take more time than it took
> to write it. (metamath mailing list, 2017)

Although metamath-exe is now on github, we endorse the general sentiment
about hazards of making major contributions to metamath-exe.

Are we saying we want no major contributions to metamath tools? Not at
all! Some work on metamath tools (for example, the only
implementation to date of the rather important definition checker) have been
done in [mmj2](https://github.com/digama0/mmj2) (although that code
has its own issues), and a lot of the recent work towards community
maintained metamath tools has focused on the
[metamath-knife](https://github.com/david-a-wheeler/metamath-knife)
verifier and/or tools built on top of it.

Having said that, metamath.exe does a lot of things which are not covered
by other tools, and we expect that replacements, for example based on
metamath-knife, will not be written overnight. We aren't making a rigid
"never change metamath-exe" rule but we are saying that we expect such
changes will be primarily to tweak parts of metamath-exe which are getting
in our way, more so than expand what it can do.

## Coding standards

No need to worry about versions of the C language older than C99.
We would like to have our code usable by the
[CompCert C compiler](https://compcert.org/compcert-C.html) (as
described on that page it supports almost full C11, so this isn't
a big constraint). We think other compilers people are using probably
support C11 but if you want to use a feature not found in C99
this might be worth asking about in your pull request description.

Some existing parts of metamath-exe have commented out code as a way of
showing alternative possibilities or old versions of the code. Now that
it is in git, we plan to remove most or all of this.

We suspect some features of metamath-exe are being used lightly if at
all. We are potentially open to removing them to make maintenance
easier, but this should be done carefully (for example by proposing a
github issue, trying to find existing users, suggesting alternatives,
and the like). We'd hate to have this sort of thing prevent anyone
from upgrading to the latest version of metamath-exe.

## What are the requirements for acceptance (merging)?

We don't have as many automated checks on this repository as
would be ideal. So unless/until we add that, some of the following
may need to be checked manually.

We expect all code merged to compile and not break existing
functionality.

To be merged, a pull request must be approved by a different existing
contributor (someone who's already had some previous contributions accepted).
You can approve a change by viewing the pull request, selecting
the tab "Files Changed", clicking on the "Review Changes" button,
clicking on "Approve", and submitting the review.
You can also approve by using the comment "+1".
Feel free to approve something others have approved, that makes it clear
there's no serious issue.

Once a pull request is approved (other than the person
originally proposing the change), any maintainer can merge it,
including the approver.
A maintainer is a member or owner of the Metamath GitHub organization.

We especially encourage discussion and, perhaps, letting a bit of time
pass before merging, if a change seems large or risky (given the general
philosophy described in the "Maintenance mode" section above).

We encourage changes to be smaller, focusing on a single logical idea.
That makes changes much easier to review.
It also makes it easy to accept some changes while not accepting others.

The following people are active and are willing to be contacted
concerning metamath-exe. Please propose a change to this file if you want
to be listed here.

* Mario Carneiro (@digama0)

## Documentation Rules

Here are some links on how to improve documentation in general:

https://www.oreilly.com/content/the-eight-rules-of-good-documentation/

https://guides.lib.berkeley.edu/how-to-write-good-documentation

---

1. Documentation is mostly collected in the ``doc`` subfolder of metamath.  As
    an entry point to documentation a README.TXT is kept in the main folder of
    Metamath.  This file is pure [ASCII](https://en.wikipedia.org/wiki/ASCII)
    encoded [plain text](https://en.wikipedia.org/wiki/Plain_text).

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

## For more information

For more general information, see the [README.md](README.md) file.
