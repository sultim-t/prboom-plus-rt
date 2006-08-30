// Copyright (C) 2006 Neil Stevens <neil@hakubi.us>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name(s) of the author(s) shall not be
// used in advertising or otherwise to promote the sale, use or other dealings
// in this Software without prior written authorization from the author(s).

#import "ANSIString.h"

@implementation ANSIString

static int findNext(NSString *string, NSString *needle, int start)
{
	if(start >= [string length])
	{
		return NSNotFound;
	}
	else
	{
		int i = [string rangeOfString:needle options:nil
		               range:NSMakeRange(start, [string length] - start)].location;
		return i;
	}
}

static NSFont *defaultFont(void)
{
	return [[[NSFont userFixedPitchFontOfSize:12] retain] autorelease];
}

static NSColor *ansiColor(int c, int bold)
{
	float value = bold ? 1.0 : 0.5;
	float a = 1.0;
	float r = 0.0;
	float g = 0.0;
	float b = 0.0;
	if(c & 1)
		r = value;
	if(c & 2)
		g = value;
	if(c & 4)
		b = value;

	NSColor *color = [NSColor colorWithCalibratedRed:r green:g blue:b alpha:a];
	return [[color retain] autorelease];
}

static NSDictionary *attributes(bool bold, bool blink, bool reverse, int bg, int fg, int isReset)
{
	// TODO: implement blinking?

	int bgColor = bg;
	int fgColor = fg;
	int bgBold = false;
	if(reverse)
	{
		bgColor = fg;
		fgColor = bg;
	}
	// Special case hackery
	// Make the reset case be black on white
	if(isReset)
	{
		fgColor = 0;
		bgColor = 7;
		bgBold = true;
	}

	NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:
	     defaultFont(), NSFontAttributeName,
	     ansiColor(fgColor, bold), NSForegroundColorAttributeName,
	     ansiColor(bgColor, bgBold), NSBackgroundColorAttributeName, nil];
	return [[attributes retain] autorelease];
}

+ (NSAttributedString *)parseColorCodes:(NSString *)ansiString
{
	NSMutableAttributedString *retval = [[[NSMutableAttributedString alloc]
	                                    initWithString:@""] autorelease];
	int length = [ansiString length];
	int last = 0;
	int current = -1;

	static const int DefaultFGColor = 7;
	static const int DefaultBGColor = 0;
	bool bold = false;
	bool blink = false;
	bool reverse = false;
	bool isReset = true;
	int bgColor = DefaultBGColor;
	int fgColor = DefaultFGColor;

	while((current = findNext(ansiString, @"\e[", current + 1)) != NSNotFound)
	{
		int updateLength = current - last + 1;
		NSAttributedString *update = [[[NSAttributedString alloc]
		  initWithString:[ansiString substringWithRange:NSMakeRange(last, updateLength)]
		  attributes:attributes(bold, blink, reverse, bgColor, fgColor, isReset)]
		  autorelease];
		[retval appendAttributedString:update];
		last = current;

		int end = findNext(ansiString, @"m", current + 1);
		if(end == NSNotFound)
		{
			current = length;
		}
		else
		{
			bool valid = true;
			int i;
			for(i = current + 2; valid && (i < end); ++i)
			{
				unichar c = [ansiString characterAtIndex:i];
				switch(c)
				{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case ';':
					break;
				default:
					valid = false;
				}
			}
			if(valid)
			{
				NSArray *codes = [[ansiString substringWithRange:
				                  NSMakeRange(current + 2, end - current - 2)]
				                  componentsSeparatedByString:@";"];
				for(i = 0; i < [codes count]; ++i)
				{
					int c = [[codes objectAtIndex:i] intValue];
					switch(c)
					{
					case 0:
						bold = false;
						blink = false;
						reverse = false;
						isReset = true;
						bgColor = DefaultBGColor;
						fgColor = DefaultFGColor;
						break;
					case 1:
						bold = true;
						isReset = false;
						break;
					case 5:
						blink = true;
						isReset = false;
						break;
					case 7:
						reverse = true;
						isReset = false;
						break;
					case 30:
					case 31:
					case 32:
					case 33:
					case 34:
					case 35:
					case 36:
					case 37:
						fgColor = c - 30;
						isReset = false;
						break;
					case 40:
					case 41:
					case 42:
					case 43:
					case 44:
					case 45:
					case 46:
					case 47:
						bgColor = c - 40;
						isReset = false;
						break;
					}
				}

				int codeLength = end - current + 1;
				current += codeLength - 1;
				last = current + 1;
			}
		}
	}
	if(last < length)
	{
		NSAttributedString *rest = [[[NSAttributedString alloc]
		      initWithString:[ansiString substringFromIndex:last]
		      attributes:attributes(bold, blink, reverse, bgColor, fgColor, isReset)]
		      autorelease];
		[retval appendAttributedString:rest];
	}

	return retval;
}

@end
