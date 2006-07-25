// Controller.m
//

#import "Controller.h"
#import <AGRegex/AGRegex.h>

@implementation Controller

- (IBAction)find:(id)sender {
	NSString *string = [textView string];
	NSRange searchRange, selectedRange = [textView selectedRange];
	
	searchRange.location = selectedRange.location + selectedRange.length;
	if (lastMatchWasEmpty && searchRange.location < [string length]) searchRange.location++;
	searchRange.length = [string length] - searchRange.location;
	
	int options = 0;
	if ([caseInsensitiveButton state] == NSOnState) options |= AGRegexCaseInsensitive;
	if ([dotAllButton state] == NSOnState) options |= AGRegexDotAll;
	if ([extendedButton state] == NSOnState) options |= AGRegexExtended;
	if ([lazyButton state] == NSOnState) options |= AGRegexLazy;
	if ([multilineButton state] == NSOnState) options |= AGRegexMultiline;
	
	AGRegex *regex = [AGRegex regexWithPattern:[patternField stringValue] options:options];	
    AGRegexMatch *match = [regex findInString:string range:searchRange];
    
    if (match) {
		[matchField setStringValue:[NSString stringWithFormat:@"%@ %@", NSStringFromRange([match range]), [match group]]];
		[textView setSelectedRange:[match range]];
		[textView scrollRangeToVisible:[textView selectedRange]];
		if ([match range].length == 0)
			lastMatchWasEmpty = YES;
	}
	else {
		[matchField setStringValue:@"Not found"];
		NSBeep();
	}
}

- (IBAction)replace:(id)sender {
	NSString *string = [textView string];
	int options = 0;
	
	if ([caseInsensitiveButton state] == NSOnState) options |= AGRegexCaseInsensitive;
	if ([dotAllButton state] == NSOnState) options |= AGRegexDotAll;
	if ([extendedButton state] == NSOnState) options |= AGRegexExtended;
	if ([lazyButton state] == NSOnState) options |= AGRegexLazy;
	if ([multilineButton state] == NSOnState) options |= AGRegexMultiline;

	AGRegex *regex = [AGRegex regexWithPattern:[patternField stringValue] options:options];
	if ([regex findInString:string])
		[textView setString:[regex replaceWithString:[replacementField stringValue] inString:string]];
	else {
		[matchField setStringValue:@"Not found"];
		NSBeep();
	}
}

- (IBAction)openDocument:(id)sender {
	NSOpenPanel *openPanel = [NSOpenPanel openPanel];
	if ([openPanel runModalForTypes:nil] == NSOKButton) {
		[textView setString:[NSString stringWithContentsOfFile:[openPanel filename]]];
		[textView setSelectedRange:NSMakeRange(0, 0)];
		[matchField setStringValue:@""];
	}
}

- (void)textViewDidChangeSelection:(NSNotification *)notification {
	lastMatchWasEmpty = NO;
}

@end
