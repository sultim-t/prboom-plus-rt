// This file is hereby placed in the Public Domain -- Neil Stevens

#import "FileButtonController.h"

@implementation FileButtonController

- (id)init
{
	[super init];
	types = [[NSArray alloc] init];
	allowMultiple = false;
	return self;
}

- (void)dealloc
{
	[types release];
	[super dealloc];
}

- (void)setTypes:(NSArray *)typeArray
{
	[types release];
	types = [[NSArray alloc] initWithArray:typeArray];
}

- (void)setAllowMultiple:(bool)allow
{
	allowMultiple = allow;
}

- (IBAction)buttonClicked:(id)sender
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:allowMultiple];
	[panel setCanChooseFiles:true];
	[panel setCanChooseDirectories:false];
	[panel beginSheetForDirectory:nil file:nil types:types
	       modalForWindow:[NSApp mainWindow]  modalDelegate:self
	       didEndSelector:@selector(panelEnded:returnCode:contextInfo:)
	       contextInfo:nil];
}

- (void)panelEnded:(NSOpenPanel *)panel returnCode:(int)code contextInfo:(void *)info
{
	if(code == NSCancelButton) return;
	[field setStringValue:[[panel filenames] objectAtIndex:0]];
}

- (id)field
{
	return field;
}

- (void)setEnabled:(BOOL)enabled
{
	[field setEnabled:enabled];
	[button setEnabled:enabled];
}

@end
