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
	NSMutableArray *args = [NSMutableArray arrayWithCapacity:10];

	// Game
	[args insertObject:@"-iwad" atIndex:[args count]];
	long game = [[gameButton objectValue] longValue];
	if(game == 0)
		[args insertObject:@"doom.wad" atIndex:[args count]];
	else if(game == 1)
		[args insertObject:@"doomu.wad" atIndex:[args count]];
	else if(game == 2)
		[args insertObject:@"doom2.wad" atIndex:[args count]];
	else if(game == 3)
		[args insertObject:@"tnt.wad" atIndex:[args count]];
	else if(game == 4)
		[args insertObject:@"plutonia.wad" atIndex:[args count]];
	else if(game == 5)
		[args insertObject:@"freedoom.wad" atIndex:[args count]];

	// Compat
	[args insertObject:@"-complevel" atIndex:[args count]];
	[args insertObject:[[compatibilityLevelButton objectValue] stringValue]
	      atIndex:[args count]];

	// Options
	if([fastMonstersButton state] == NSOnState)
		[args insertObject:@"-fast" atIndex:[args count]];
	if([noMonstersButton state] == NSOnState)
		[args insertObject:@"-nomonsters" atIndex:[args count]];
	if([respawnMonstersButton state] == NSOnState)
		[args insertObject:@"-respawn" atIndex:[args count]];

	// Debug options
	if([disableGraphicsButton state] == NSOnState)
		[args insertObject:@"-nodraw" atIndex:[args count]];
	if([disableJoystickButton state] == NSOnState)
		[args insertObject:@"-nojoy" atIndex:[args count]];
	if([disableMouseButton state] == NSOnState)
		[args insertObject:@"-nomouse" atIndex:[args count]];
	if([disableSoundButton state] == NSOnState)
	{
		[args insertObject:@"-nosound" atIndex:[args count]];
	}
	else
	{
		if([disableMusicButton state] == NSOnState)
			[args insertObject:@"-nomusic" atIndex:[args count]];
		if([disableSoundEffectsButton state] == NSOnState)
			[args insertObject:@"-nosfx" atIndex:[args count]];
	}

	// Extra wads

	// Demo

	// Execute
	NSTask *task = [NSTask launchedTaskWithLaunchPath:path arguments:args];
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
