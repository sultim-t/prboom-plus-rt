
AGRegex provides Perl-compatible pattern matching to Cocoa applications.

Regular expression support is provided by the PCRE library package, which is open source software, written by Philip Hazel, and copyright by the University of Cambridge, England.

<http://www.pcre.org>
<ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre>

For complete regular expression syntax see the PCRE documentation.

NOTES

If you are only using valid ASCII strings, you can save some overhead by turning off UTF-8 support. In the framework target, select "Settings", scroll down to "GCC Compiler Settings", and under "Other C Compiler Flags", delete the line -DSUPPORT_UTF8. If you do this, note that some methods will raise an exception if you pass a string that can't be converted to ASCII.

If you want to build AGRegex with a newer version of PCRE than that which is included, build PCRE according to the normal instructions so that all the necessary files get generated and then just copy them over the older ones. Be sure to enable UTF-8 support when you build PCRE if you are using it in AGRegex. It is also possible to link to just link to libpcre instead of compiling it directly into the framework, but it would have to be installed separately.

BUGS

-replaceWithString:inString:... won't see a backreference preceded by a backslash in the replacement string, whether the backslash itself is escaped or not.

-splitString:... will split at an empty match immediately following a non-empty match. While this is not necessarily a bug, it is at least inconsistent with Perl's split operator.

CONTACT

<grnmn@users.sourceforge.net>
