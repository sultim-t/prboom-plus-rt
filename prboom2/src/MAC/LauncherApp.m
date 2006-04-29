#import "LauncherApp.h"

@implementation LauncherApp

- (void)awakeFromNib
{
    /* Set Icon */
    NSString *iconPath = [[NSBundle mainBundle] pathForResource:@"Launcher.icns" ofType:nil];
    [NSApp setApplicationIconImage:[[NSImage alloc] initWithContentsOfFile:iconPath]];

	wads = [[NSMutableArray arrayWithCapacity:3] retain];
	[self loadDefaults];
}

- (void)windowWillClose:(NSNotification *)notification
{
	[NSApp terminate:window];
}

- (void)loadDefaults
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	if([defaults boolForKey:@"Saved"] == true)
	{
		[gameButton setObjectValue:[defaults objectForKey:@"Game"]];
		[respawnMonstersButton setObjectValue:[defaults objectForKey:@"Respawn Monsters"]];
		[fastMonstersButton setObjectValue:[defaults objectForKey:@"Fast Monsters"]];
		[noMonstersButton setObjectValue:[defaults objectForKey:@"No Monsters"]];
		[disableGraphicsButton setObjectValue:[defaults objectForKey:@"Disable Graphics"]];
		[disableJoystickButton setObjectValue:[defaults objectForKey:@"Disable Joystick"]];
		[disableMouseButton setObjectValue:[defaults objectForKey:@"Disable Mouse"]];
		[disableMusicButton setObjectValue:[defaults objectForKey:@"Disable Music"]];
		[disableSoundButton setObjectValue:[defaults objectForKey:@"Disable Sound"]];
		[disableSoundEffectsButton setObjectValue:[defaults objectForKey:@"Disable Sound Effects"]];
		[wads setArray:[defaults stringArrayForKey:@"Wads"]];
	}

	[self disableSoundClicked:disableSoundButton];
	[self gameButtonClicked:gameButton];
	[self demoButtonClicked:demoMatrix];
	[self tableViewSelectionDidChange:nil];
}

- (void)saveDefaults
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	[defaults setBool:true forKey:@"Saved"];

	[defaults setObject:[gameButton objectValue] forKey:@"Game"];
	[defaults setObject:[respawnMonstersButton objectValue] forKey:@"Respawn Monsters"];
	[defaults setObject:[fastMonstersButton objectValue] forKey:@"Fast Monsters"];
	[defaults setObject:[noMonstersButton objectValue] forKey:@"No Monsters"];
	[defaults setObject:[disableGraphicsButton objectValue] forKey:@"Disable Graphics"];
	[defaults setObject:[disableJoystickButton objectValue] forKey:@"Disable Joystick"];
	[defaults setObject:[disableMouseButton objectValue] forKey:@"Disable Mouse"];
	[defaults setObject:[disableMusicButton objectValue] forKey:@"Disable Music"];
	[defaults setObject:[disableSoundButton objectValue] forKey:@"Disable Sound"];
	[defaults setObject:[disableSoundEffectsButton objectValue] forKey:@"Disable Sound Effects"];
	[defaults setObject:wads forKey:@"Wads"];

	[defaults synchronize];
}

- (IBAction)startClicked:(id)sender
{
	[self saveDefaults];

	NSString *path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"PrBoom"];
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
	[args insertObject:@"-file" atIndex:[args count]];
	int i;
	for(i = 0; i < [wads count]; ++i)
		[args insertObject:[wads objectAtIndex:i] atIndex:[args count]];

	// Demo
	if([demoMatrix selectedCell] != noDemoButton)
	{
		if([demoMatrix selectedCell] == playDemoButton)
			[args insertObject:@"-playdemo" atIndex:[args count]];
		else if([demoMatrix selectedCell] == timeDemoButton)
			[args insertObject:@"-timedemo" atIndex:[args count]];
		else if([demoMatrix selectedCell] == fastDemoButton)
			[args insertObject:@"-fastdemo" atIndex:[args count]];

		[args insertObject:[demoFileField stringValue] atIndex:[args count]];

		if([[ffToLevelField stringValue] length] > 0)
		{
			[args insertObject:@"-ffmap" atIndex:[args count]];
			[args insertObject:[ffToLevelField stringValue] atIndex:[args count]];
		}
	}

	// Execute
	NSTask *task = [NSTask launchedTaskWithLaunchPath:path arguments:args];
}

- (IBAction)gameButtonClicked:(id)sender
{
	long game = [[gameButton objectValue] longValue];
	if(game == 0)
		[compatibilityLevelButton setObjectValue:[NSNumber numberWithLong:2]];
	else if(game == 1)
		[compatibilityLevelButton setObjectValue:[NSNumber numberWithLong:3]];
	else if(game == 2)
		[compatibilityLevelButton setObjectValue:[NSNumber numberWithLong:2]];
	else if(game == 3)
		[compatibilityLevelButton setObjectValue:[NSNumber numberWithLong:4]];
	else if(game == 4)
		[compatibilityLevelButton setObjectValue:[NSNumber numberWithLong:4]];
	else if(game == 5)
		[compatibilityLevelButton setObjectValue:[NSNumber numberWithLong:7]];
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
	bool enabled = [demoMatrix selectedCell] != noDemoButton;
	[chooseDemoFileButton setEnabled:enabled];
	[demoFileField setEnabled:enabled];
	[ffToLevelField setEnabled:enabled];
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
