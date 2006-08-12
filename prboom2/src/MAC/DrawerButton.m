// This file is hereby placed in the Public Domain -- Neil Stevens

#import "DrawerButton.h"

@implementation DrawerButton

- (void)drawerDidClose:(NSNotification *)notification
{
	[self updateTitle];
}

- (void)drawerDidOpen:(NSNotification *)notification
{
	[self updateTitle];
}

- (void)updateTitle
{
	int state = [drawer state];
	bool opening = state == NSDrawerOpenState | state == NSDrawerOpeningState;
	NSString *newText = opening ? @"Hide " : @"Show ";
	if([[self title] hasPrefix:@"Hide "] || [[self title] hasPrefix:@"Show "])
	{
		[self setTitle:[newText stringByAppendingString:
		      [[self title] substringFromIndex:5]]];
	}
}

@end
