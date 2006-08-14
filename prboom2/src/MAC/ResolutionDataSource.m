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

#import <ApplicationServices/ApplicationServices.h>
#import "ResolutionDataSource.h"

@implementation ResolutionDataSource

static int resolutionSort(id first, id second, void *context)
{
	NSArray *firstComponents = [first componentsSeparatedByString:@"x"];
	NSArray *secondComponents = [second componentsSeparatedByString:@"x"];
	int firstW = [[firstComponents objectAtIndex:0] intValue];
	int firstH = [[firstComponents objectAtIndex:1] intValue];
	int secondW = [[secondComponents objectAtIndex:0] intValue];
	int secondH = [[secondComponents objectAtIndex:1] intValue];

	if(firstW < secondW)
		return NSOrderedAscending;
	else if(firstW > secondW)
		return NSOrderedDescending;
	else if(firstH < secondH)
		return NSOrderedAscending;
	else if(firstH > secondH)
		return NSOrderedDescending;
	else
		return NSOrderedSame;
}

static void addResolution(NSMutableArray *array, int w, int h)
{
	NSString *str = [NSString stringWithFormat:@"%ix%i", w, h];
	if(![array containsObject:str])
		[array addObject:str];
}

+ (NSArray *)resolutions
{
	NSMutableArray *retval = [[[NSMutableArray alloc] init] autorelease];
	NSArray *modes = (NSArray *)CGDisplayAvailableModes(CGMainDisplayID());
	int i;
	for(i = 0; i < [modes count]; ++i)
	{
		NSDictionary *mode = [modes objectAtIndex:i];
		int w = [[mode valueForKey:(NSString *)kCGDisplayWidth] intValue];
		int h = [[mode valueForKey:(NSString *)kCGDisplayHeight] intValue];
		float ratio = (float) w / (float) h;
		float r1 = 4.0 / 3.0;
		float r2 = 8.0 / 5.0;

		// Skip unsafe resolutions
		if(![mode objectForKey:(NSString *)kCGDisplayModeIsSafeForHardware])
			continue;

		// Bounds of supported PrBoom resolutions
		if(w > 2048)
			w = 2048;
		if(h > 1536)
			h = 1536;
		if(w < 320)
			continue;
		if(h < 200)
			continue;

		if(h * 4 / 3 == w)
		{
			// 4:3 screen (640x480)
			// Use as is, and offer 8:5
			addResolution(retval, w, h);
			addResolution(retval, w, w * 5 / 8);
		}
		else if(ratio > r1 && ratio < r2)
		{
			// Wide screen less than 8:5 (700x500)
			// Offer nearest 4:3 and 8:5
			addResolution(retval, w, w * 5 / 8);
			addResolution(retval, h * 4 / 3, h);
		}
		else if(h * 8 / 5 == w)
		{
			// 8:5 screen (320x200)
			// Use as is
			addResolution(retval, w, h);
		}
		else if(ratio < r1)
		{
			// Narrow screen (1280x1024)
			// Offer 4:3
			addResolution(retval, w, w * 3 / 4);
		}
		else
		{
			// Wide wide screen (2000x1000)
			// Offer 8:5
			addResolution(retval, h * 8 / 5, h);
		}
	}
	[retval sortUsingFunction:resolutionSort context:nil];
	return retval;
}

- (id)comboBox:(NSComboBox *)box objectValueForItemAtIndex:(int)i
{
	NSArray *modes = [ResolutionDataSource resolutions];
	return [modes objectAtIndex:i];
}

- (int)comboBox:(NSComboBox *)box indexOfItemWithStringValue:(NSString *)string
{
	NSArray *modes = [ResolutionDataSource resolutions];
	return [modes indexOfObject:string];
}

- (int)numberOfItemsInComboBox:(NSComboBox *)box
{
	NSArray *modes = [ResolutionDataSource resolutions];
	return [modes count];
}

@end
