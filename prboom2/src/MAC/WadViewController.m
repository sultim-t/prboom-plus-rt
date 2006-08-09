// This file is hereby placed in the Public Domain -- Neil Stevens

#import "WadViewController.h"

@implementation WadViewController

- (id)init
{
	if(self = [super init])
	{
		wads = [[NSMutableArray arrayWithCapacity:3] retain];
	}
	return self;
}

- (void)dealloc
{
	[wads release];
	[super dealloc];
}

- (IBAction)add:(id)sender
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:true];
	[panel setCanChooseFiles:true];
	[panel setCanChooseDirectories:false];
	NSArray *types = [NSArray arrayWithObjects:@"wad", @"WAD", @"DEH", @"deh", nil];
	[panel beginSheetForDirectory:nil file:nil types:types
	       modalForWindow:[NSApp mainWindow]  modalDelegate:self
	       didEndSelector:@selector(addEnded:returnCode:contextInfo:)
	       contextInfo:nil];
}

- (void)addEnded:(NSOpenPanel *)panel returnCode:(int)code contextInfo:(void *)info
{
	if(code == NSCancelButton) return;

	int i;
	for(i = 0; i < [[panel filenames] count]; ++i)
		[wads insertObject:[[panel filenames] objectAtIndex:i] atIndex:[wads count]];

	[view noteNumberOfRowsChanged];
}

- (IBAction)remove:(id)sender
{
	[wads removeObjectsAtIndexes:[view selectedRowIndexes]];
	[view selectRowIndexes:[NSIndexSet indexSetWithIndex:-1] byExtendingSelection:false];
	[view noteNumberOfRowsChanged];
}

- (NSArray *)wads
{
	return [NSArray arrayWithArray:wads];
}

- (void)setWads:(NSArray *)newWads
{
	[wads setArray:newWads];
	[view noteNumberOfRowsChanged];
	[self tableViewSelectionDidChange:nil];
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
	[removeButton setEnabled:([view selectedRow] > -1)];
}

- (int)numberOfRowsInTableView:(NSTableView *)tableView
{
	return [wads count];
}

- (id)tableView:(NSTableView *)tableView
                objectValueForTableColumn:(NSTableColumn *)column
                row:(int)row
{
	NSString *columnId = [column identifier];
	if([columnId isEqualToString:@"Path"])
		return [wads objectAtIndex:row];
	else if([columnId isEqualToString:@"Icon"])
		return [[NSWorkspace sharedWorkspace] iconForFile:[wads objectAtIndex:row]];
	else
		return nil;
}

- (void)tableView:(NSTableView *)tableView
                  setObjectValue:(id)object
                  forTableColumn:(NSTableColumn *)column
                  row:(int)row
{
	NSString *columnId = [[column identifier] stringValue];
	if([columnId isEqualToString:@"Path"])
		[wads replaceObjectAtIndex:row withObject:object];
}

@end
