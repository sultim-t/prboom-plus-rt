#import "LauncherApp.h"

@implementation LauncherApp

- (void)awakeFromNib
{
	[self disableSoundClicked:disableSoundButton];
	[self demoButtonClicked:demoMatrix];
	[self tableViewSelectionDidChange:nil];

	wads = [[NSMutableArray arrayWithCapacity:3] retain];
}

- (void)windowWillClose:(NSNotification *)notification
{
	[NSApp terminate:window];
}

- (IBAction)startClicked:(id)sender
{
	NSString *path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"game"];
	NSArray *array = [NSArray array];
	NSTask *task = [NSTask launchedTaskWithLaunchPath:path arguments:array];
}

- (IBAction)disableSoundClicked:(id)sender
{
	bool state = [disableSoundButton state] != NSOnState;
	[disableSoundEffectsButton setEnabled:state];
	[disableMusicButton setEnabled:state];
}

- (IBAction)chooseDemoFileClicked:(id)sender
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:false];
	[panel setCanChooseFiles:true];
	[panel setCanChooseDirectories:false];
	NSArray *types = [NSArray arrayWithObjects:@"lmp", @"LMP", nil];
	[panel beginSheetForDirectory:nil file:nil types:types
	       modalForWindow:window  modalDelegate:self
	       didEndSelector:@selector(chooseDemoFileEnded:returnCode:contextInfo:)
	       contextInfo:nil];
}

- (void)chooseDemoFileEnded:(NSOpenPanel *)panel returnCode:(int)code contextInfo:(void *)info
{
	if(code == NSCancelButton) return;
	[demoFileField setStringValue:[[panel filenames] objectAtIndex:0]];
}

- (IBAction)demoButtonClicked:(id)sender
{
	id selected = [demoMatrix selectedCell];
	[chooseDemoFileButton setEnabled:selected != noDemoButton];
	[demoFileField setEnabled:selected != noDemoButton];
	[warpToLevelField setEnabled:selected == playDemoWarpButton];
}

- (IBAction)addWadClicked:(id)sender
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:true];
	[panel setCanChooseFiles:true];
	[panel setCanChooseDirectories:false];
	NSArray *types = [NSArray arrayWithObjects:@"wad", @"WAD", nil];
	[panel beginSheetForDirectory:nil file:nil types:types
	       modalForWindow:window  modalDelegate:self
	       didEndSelector:@selector(addWadEnded:returnCode:contextInfo:)
	       contextInfo:nil];
}

- (void)addWadEnded:(NSOpenPanel *)panel returnCode:(int)code contextInfo:(void *)info
{
	if(code == NSCancelButton) return;

	int i;
	for(i = 0; i < [[panel filenames] count]; ++i)
		[wads insertObject:[[panel filenames] objectAtIndex:i] atIndex:[wads count]];

	[wadView noteNumberOfRowsChanged];
}

- (IBAction)removeWadClicked:(id)sender
{
	[wads removeObjectAtIndex:[wadView selectedRow]];
	[wadView selectRowIndexes:[NSIndexSet indexSetWithIndex:-1] byExtendingSelection:false];
	[wadView noteNumberOfRowsChanged];
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
	[removeWadButton setEnabled:([wadView selectedRow] > -1)];
}

- (int)numberOfRowsInTableView:(NSTableView *)tableView
{
	return [wads count];
}

- (id)tableView:(NSTableView *)tableView
                objectValueForTableColumn:(NSTableColumn *)column
                row:(int)row
{
	// We only have one column
	return [wads objectAtIndex:row];
}

- (void)tableView:(NSTableView *)tableView
                  setObjectValue:(id)object
                  forTableColumn:(NSTableColumn *)column
                  row:(int)row
{
	// We only have one column
	[wads replaceObjectAtIndex:row withObject:object];
}

@end
