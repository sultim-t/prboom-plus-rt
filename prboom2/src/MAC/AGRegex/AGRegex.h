// AGRegex.h
//
// Copyright (c) 2002 Aram Greenman. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import <Foundation/NSObject.h>
#import <Foundation/NSRange.h>

@class AGRegex, NSArray, NSString;

/*!
@enum Options 
Options defined for -initWithPattern:options:. Two or more options can be combined with the bitwise OR operator.
@constant AGRegexCaseInsensitive Matching is case insensitive. Equivalent to /i in Perl.
@constant AGRegexDotAll Dot metacharacter matches any character including newline. Equivalent to /s in Perl.
@constant AGRegexExtended Allow whitespace and comments in the pattern. Equivalent to /x in Perl.
@constant AGRegexLazy Makes greedy quantifiers lazy and lazy quantifiers greedy. No equivalent in Perl.
@constant AGRegexMultiline Caret and dollar anchors match at newline. Equivalent to /m in Perl.
*/
enum {
	AGRegexCaseInsensitive = 1,
	AGRegexDotAll = 2,
	AGRegexExtended = 4,
	AGRegexLazy = 8,
	AGRegexMultiline = 16
};

/*!
@class AGRegexMatch
@abstract A single occurence of a regular expression.
@discussion An AGRegexMatch represents a single occurence of a regular expression within the target string. The range of each subpattern within the target string is returned by -range, -rangeAtIndex:, or -rangeNamed:. The part of the target string that matched each subpattern is returned by -group, -groupAtIndex:, or -groupNamed:.
*/
@interface AGRegexMatch : NSObject {
	AGRegex *regex;
	NSString *string;
	int *matchv;
	int count;
}

/*!
@method count
The number of capturing subpatterns, including the pattern itself. */
- (int)count;

/*!
@method group
Returns the part of the target string that matched the pattern. */
- (NSString *)group;

/*!
@method groupAtIndex:
Returns the part of the target string that matched the subpattern at the given index or nil if it wasn't matched. The subpatterns are indexed in order of their opening parentheses, 0 is the entire pattern, 1 is the first capturing subpattern, and so on. */
- (NSString *)groupAtIndex:(int)idx;

/*!
@method groupNamed:
Returns the part of the target string that matched the subpattern of the given name or nil if it wasn't matched. */
- (NSString *)groupNamed:(NSString *)name;

/*!
@method range
Returns the range of the target string that matched the pattern. */
- (NSRange)range;

/*!
@method rangeAtIndex:
Returns the range of the target string that matched the subpattern at the given index or {NSNotFound, 0} if it wasn't matched. The subpatterns are indexed in order of their opening parentheses, 0 is the entire pattern, 1 is the first capturing subpattern, and so on. */
- (NSRange)rangeAtIndex:(int)idx;

/*!
@method rangeNamed:
Returns the range of the target string that matched the subpattern of the given name or {NSNotFound, 0} if it wasn't matched. */
- (NSRange)rangeNamed:(NSString *)name;

/*!
@method string
Returns the target string. */
- (NSString *)string;

@end

/*!
@class AGRegex
@abstract An Perl-compatible regular expression class.
@discussion An AGRegex is created with -initWithPattern: or -initWithPattern:options: or the corresponding class methods +regexWithPattern: or +regexWithPattern:options:. These take a regular expression pattern string and the bitwise OR of zero or more option flags. For example:

<code>&nbsp;&nbsp;&nbsp;&nbsp;AGRegex *regex = [[AGRegex alloc] initWithPattern:&#64;"(paran|andr)oid" options:AGRegexCaseInsensitive];</code>

Matching is done with -findInString: or -findInString:range: which look for the first occurrence of the pattern in the target string and return an AGRegexMatch or nil if the pattern was not found.

<code>&nbsp;&nbsp;&nbsp;&nbsp;AGRegexMatch *match = [regex findInString:&#64;"paranoid android"];</code>
    
A match object returns a captured subpattern by -group, -groupAtIndex:, or -groupNamed:, or the range of a captured subpattern by -range, -rangeAtIndex:, or -rangeNamed:. The subpatterns are indexed in order of their opening parentheses, 0 is the entire pattern, 1 is the first capturing subpattern, and so on. -count returns the total number of subpatterns, including the pattern itself. The following prints the result of our last match case:

<code>&nbsp;&nbsp;&nbsp;&nbsp;for (i = 0; i &lt; [match count]; i++)<br />
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;NSLog(&#64;"%d %&#64; %&#64;", i, NSStringFromRange([match rangeAtIndex:i]), [match groupAtIndex:i]);</code>

<code>&nbsp;&nbsp;&nbsp;&nbsp;0 {0, 8} paranoid<br />
&nbsp;&nbsp;&nbsp;&nbsp;1 {0, 5} paran</code>

If any of the subpatterns didn't match, -groupAtIndex: will  return nil, and -rangeAtIndex: will return {NSNotFound, 0}. For example, if we change our original pattern to "(?:(paran)|(andr))oid" we will get the following output:

<code>&nbsp;&nbsp;&nbsp;&nbsp;0 {0, 8} paranoid<br />
&nbsp;&nbsp;&nbsp;&nbsp;1 {0, 5} paran<br />
&nbsp;&nbsp;&nbsp;&nbsp;2 {2147483647, 0} (null)</code>

-findAllInString: and -findAllInString:range: return an NSArray of all non-overlapping occurrences of the pattern in the target string. -findEnumeratorInString: and -findEnumeratorInString:range: return an NSEnumerator for all non-overlapping occurrences of the pattern in the target string. For example,

<code>&nbsp;&nbsp;&nbsp;&nbsp;NSArray *all = [regex findAllInString:&#64;"paranoid android"];</code>

The first object in the returned array is the match case for "paranoid" and the second object is the match case for "android".

AGRegex provides the methods -replaceWithString:inString: and -replaceWithString:inString:limit: to perform substitution on strings.

<code>&nbsp;&nbsp;&nbsp;&nbsp;AGRegex *regex = [AGRegex regexWithPattern:&#64;"remote"];<br />
&nbsp;&nbsp;&nbsp;&nbsp;NSString *result = [regex replaceWithString:&#64;"complete" inString:&#64;"remote control"]; // result is "complete control"</code>

Captured subpatterns can be interpolated into the replacement string using the syntax $x or ${x} where x is the index or name of the subpattern. $0 and $& both refer to the entire pattern. Additionally, the case modifier sequences \U...\E, \L...\E, \u, and \l are allowed in the replacement string. All other escape sequences are handled literally.

<code>&nbsp;&nbsp;&nbsp;&nbsp;AGRegex *regex = [AGRegex regexWithPattern:&#64;"[usr]"];<br />
&nbsp;&nbsp;&nbsp;&nbsp;NSString *result = [regex replaceWithString:&#64;"\\u$&amp;." inString:&#64;"Back in the ussr"]; // result is "Back in the U.S.S.R."</code>

Note that you have to escape a backslash to get it into an NSString literal. 

Named subpatterns may also be used in the pattern and replacement strings, like in Python. 

<code>&nbsp;&nbsp;&nbsp;&nbsp;AGRegex *regex = [AGRegex regexWithPattern:&#64;"(?P&lt;who&gt;\\w+) is a (?P&lt;what&gt;\\w+)"];<br />
&nbsp;&nbsp;&nbsp;&nbsp;NSString *result = [regex replaceWithString:&#64;"Jackie is a $what, $who is a runt" inString:&#64;"Judy is a punk"]); // result is "Jackie is a punk, Judy is a runt"</code>

Finally, AGRegex provides -splitString: and -splitString:limit: which return an NSArray created by splitting the target string at each occurrence of the pattern. For example:

<code>&nbsp;&nbsp;&nbsp;&nbsp;AGRegex *regex = [AGRegex regexWithPattern:&#64;"ea?"];<br />
&nbsp;&nbsp;&nbsp;&nbsp;NSArray *result = [regex splitString:&#64;"Repeater"]; // result is "R", "p", "t", "r"</code>

If there are captured subpatterns, they are returned in the array. 

<code>&nbsp;&nbsp;&nbsp;&nbsp;AGRegex *regex = [AGRegex regexWithPattern:&#64;"e(a)?"];<br />
&nbsp;&nbsp;&nbsp;&nbsp;NSArray *result = [regex splitString:&#64;"Repeater"]; // result is "R", "p", "a", "t", "r"</code>

In Perl, this would return "R", undef, "p", "a", "t", undef, "r". Unfortunately, there is no convenient way to represent this in an NSArray. (NSNull could be used in place of undef, but then all members of the array couldn't be expected to be NSStrings.)
*/
@interface AGRegex : NSObject {
	void *regex;
	void *extra;
	int groupCount;
}

/*!
@method regexWithPattern:
Creates a new regex using the given pattern string. Returns nil if the pattern string is invalid. */
+ (id)regexWithPattern:(NSString *)pat;

/*!
@method regexWithPattern:options:
Creates a new regex using the given pattern string and option flags. Returns nil if the pattern string is invalid. */
+ (id)regexWithPattern:(NSString *)pat options:(int)opts;


/*!
@method initWithPattern:
Initializes the regex using the given pattern string. Returns nil if the pattern string is invalid. */
- (id)initWithPattern:(NSString *)pat;

/*!
@method initWithPattern:options:
Initializes the regex using the given pattern string and option flags. Returns nil if the pattern string is invalid. */
- (id)initWithPattern:(NSString *)pat options:(int)opts;

/*!
@method findInString:
Calls findInString:range: using the full range of the target string. */
- (AGRegexMatch *)findInString:(NSString *)str;

/*!
@method findInString:range:
Returns an AGRegexMatch for the first occurrence of the regex in the given range of the target string or nil if none is found. */
- (AGRegexMatch *)findInString:(NSString *)str range:(NSRange)r;

/*!
@method findAllInString:
Calls findAllInString:range: using the full range of the target string. */
- (NSArray *)findAllInString:(NSString *)str;

/*!
@method findAllInString:range:
Returns an array of all non-overlapping occurrences of the regex in the given range of the target string. The members of the array are AGRegexMatches. */
- (NSArray *)findAllInString:(NSString *)str range:(NSRange)r;

/*!
@method findEnumeratorInString:
Calls findEnumeratorInString:range: using the full range of the target string. */
- (NSEnumerator *)findEnumeratorInString:(NSString *)str;

/*!
@method findEnumeratorInString:range:
Returns an enumerator for all non-overlapping occurrences of the regex in the given range of the target string. The objects returned by the enumerator are AGRegexMatches. */
- (NSEnumerator *)findEnumeratorInString:(NSString *)str range:(NSRange)r;

/*!
@method replaceWithString:inString:
Calls replaceWithString:inString:limit: with no limit. */
- (NSString *)replaceWithString:(NSString *)rep inString:(NSString *)str;

/*!
@method replaceWithString:inString:limit:
Returns the string created by replacing occurrences of the regex in the target string with the replacement string. If the limit is positive, no more than that many replacements will be made.

Captured subpatterns can be interpolated into the replacement string using the syntax $x or ${x} where x is the index or name of the subpattern. $0 and $&amp; both refer to the entire pattern. Additionally, the case modifier sequences \U...\E, \L...\E, \u, and \l are allowed in the replacement string. All other escape sequences are handled literally. */
- (NSString *)replaceWithString:(NSString *)rep inString:(NSString *)str limit:(int)limit;

/*!
@method splitString:
Call splitString:limit: with no limit. */
- (NSArray *)splitString:(NSString *)str;

/*!
@method splitString:limit:
Returns an array of strings created by splitting the target string at each occurrence of the pattern. If the limit is positive, no more than that many splits will be made. If there are captured subpatterns, they are returned in the array.  */
- (NSArray *)splitString:(NSString *)str limit:(int)lim;

@end