#import "LauncherApp.h"

#import <Foundation/NSArray.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSString.h>
#import <Foundation/NSFileManager.h>
#import "RMUDAnsiTextView.h"
#import "UKKQueue.h"

#include <fcntl.h>

@implementation LauncherApp

- (NSString *)wadPath
{
	return [@"~/Library/Application Support/PrBoom" stringByExpandingTildeInPath];
}

- (void)awakeFromNib
{
	[[NSFileManager defaultManager] createDirectoryAtPath:[self wadPath]
	                                attributes:nil];
	[[UKKQueue sharedQueue] setDelegate:self];
	[[UKKQueue sharedQueue] addPath:[self wadPath]];

	wads = [[NSMutableArray arrayWithCapacity:3] retain];
	[self loadDefaults];

	// Check if the task is done
	[[NSNotificationCenter defaultCenter]
	addObserver:self selector:@selector(taskComplete:)
     name:NSTaskDidTerminateNotification object:nil];

	// Check if the task printed any output
	[NSTimer scheduledTimerWithTimeInterval:1.0 target:self
	         selector:@selector(taskReadTimer:) userInfo:nil repeats:true];

	// Save Prefs on exit
	[[NSNotificationCenter defaultCenter] addObserver:self
	 selector:@selector(saveDefaults)
	 name:NSApplicationWillTerminateNotification object:nil];
}

- (void)windowWillClose:(NSNotification *)notification
{
	[NSApp terminate:window];
}

- (void)openWebsite:(id)sender
{
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://prboom.sourceforge.net/"]];
}

- (void)loadDefaults
{
	[window setFrameUsingName:@"Launcher"];
	[consoleWindow setFrameUsingName:@"Console"];

	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	if([defaults boolForKey:@"Saved"] == true)
	{
		[gameButton setObjectValue:[defaults objectForKey:@"Game"]];
		[respawnMonstersButton setObjectValue:[defaults objectForKey:@"Respawn Monsters"]];
		[fastMonstersButton setObjectValue:[defaults objectForKey:@"Fast Monsters"]];
		[noMonstersButton setObjectValue:[defaults objectForKey:@"No Monsters"]];
		[fullscreenButton setObjectValue:[defaults objectForKey:@"Full Screen Graphics"]];
		[disableGraphicsButton setObjectValue:[defaults objectForKey:@"Disable Graphics"]];
		[disableJoystickButton setObjectValue:[defaults objectForKey:@"Disable Joystick"]];
		[disableMouseButton setObjectValue:[defaults objectForKey:@"Disable Mouse"]];
		[disableMusicButton setObjectValue:[defaults objectForKey:@"Disable Music"]];
		[disableSoundButton setObjectValue:[defaults objectForKey:@"Disable Sound"]];
		[disableSoundEffectsButton setObjectValue:[defaults objectForKey:@"Disable Sound Effects"]];
		[wads setArray:[defaults stringArrayForKey:@"Wads"]];

		// Store the compat level in terms of the Prboom values, rather than
		// our internal indices.  That means we have to add one when we read
		// the settings, and subtract one when we save it
		long compatIndex = [[defaults objectForKey:@"Compatibility Level"]
		                     longValue] + 1;
		[compatibilityLevelButton setObjectValue:[NSNumber
		                                          numberWithLong:compatIndex]];
	}
	else
	{
		[compatibilityLevelButton setObjectValue:[NSNumber numberWithLong:0]];
	}

	[self disableSoundClicked:disableSoundButton];
	[self demoButtonClicked:demoMatrix];
	[self tableViewSelectionDidChange:nil];
	[self updateGameWad];
}

- (void)saveDefaults
{
	[window saveFrameUsingName:@"Launcher"];
	[consoleWindow saveFrameUsingName:@"Console"];

	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	[defaults setBool:true forKey:@"Saved"];

	[defaults setObject:[gameButton objectValue] forKey:@"Game"];
	[defaults setObject:[respawnMonstersButton objectValue] forKey:@"Respawn Monsters"];
	[defaults setObject:[fastMonstersButton objectValue] forKey:@"Fast Monsters"];
	[defaults setObject:[noMonstersButton objectValue] forKey:@"No Monsters"];
	[defaults setObject:[fullscreenButton objectValue] forKey:@"Full Screen Graphics"];
	[defaults setObject:[disableGraphicsButton objectValue] forKey:@"Disable Graphics"];
	[defaults setObject:[disableJoystickButton objectValue] forKey:@"Disable Joystick"];
	[defaults setObject:[disableMouseButton objectValue] forKey:@"Disable Mouse"];
	[defaults setObject:[disableMusicButton objectValue] forKey:@"Disable Music"];
	[defaults setObject:[disableSoundButton objectValue] forKey:@"Disable Sound"];
	[defaults setObject:[disableSoundEffectsButton objectValue] forKey:@"Disable Sound Effects"];
	[defaults setObject:wads forKey:@"Wads"];

	// Store the compat level in terms of the Prboom values, rather than
	// our internal indices.  That means we have to add one when we read
	// the settings, and subtract one when we save it
	long compatLevel = [[compatibilityLevelButton objectValue] longValue] - 1;
	[defaults setObject:[NSNumber numberWithLong:compatLevel] forKey:@"Compatibility Level"];

	[defaults synchronize];
}

- (NSString *)wadForIndex:(int)index
{
	if(index == 0)
		return @"doom.wad";
	else if(index == 1)
		return @"doomu.wad";
	else if(index == 2)
		return @"doom2.wad";
	else if(index == 3)
		return @"tnt.wad";
	else if(index == 4)
		return @"plutonia.wad";
	else if(index == 5)
		return @"freedoom.wad";
	else
		return nil;
}

- (NSString *)selectedWad
{
	return [self wadForIndex:[[gameButton objectValue] longValue]];
}

- (void)updateGameWad
{
	int i;
	for(i = 0; i < [gameMenu numberOfItems]; ++i)
	{
		NSString *path = [[[self wadPath] stringByAppendingString:@"/"]
		                  stringByAppendingString:[self wadForIndex:i]];
		bool exists = [[NSFileManager defaultManager] fileExistsAtPath:path];
		[[gameMenu itemAtIndex:i] setEnabled:exists];
		if([[gameButton objectValue] longValue] == i)
			[launchButton setEnabled:exists];
	}
}

- (void)watcher:(id)watcher receivedNotification:(NSString *)notification
        forPath:(NSString *)path
{
	[self updateGameWad];
}

- (IBAction)startClicked:(id)sender
{
	[self saveDefaults];

	NSString *path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"PrBoom"];
	NSMutableArray *args = [NSMutableArray arrayWithCapacity:10];

	// redirect all output to stdout
	[args insertObject:@"-cout" atIndex:[args count]];
	[args insertObject:@"ICWEFDA" atIndex:[args count]];

	[args insertObject:@"-cerr" atIndex:[args count]];

	// Game
	[args insertObject:@"-iwad" atIndex:[args count]];
	[args insertObject:[self selectedWad] atIndex:[args count]];

	// Compat
	long compatLevel = [[compatibilityLevelButton objectValue] longValue] - 1;
	[args insertObject:@"-complevel" atIndex:[args count]];
	[args insertObject:[[NSNumber numberWithLong:compatLevel] stringValue]
	      atIndex:[args count]];

	// Options
	if([fastMonstersButton state] == NSOnState)
		[args insertObject:@"-fast" atIndex:[args count]];
	if([noMonstersButton state] == NSOnState)
		[args insertObject:@"-nomonsters" atIndex:[args count]];
	if([respawnMonstersButton state] == NSOnState)
		[args insertObject:@"-respawn" atIndex:[args count]];

	if([fullscreenButton state] == NSOnState)
		[args insertObject:@"-fullscreen" atIndex:[args count]];
	else
		[args insertObject:@"-nofullscreen" atIndex:[args count]];

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
	standardOutput = [[NSPipe alloc] init];
	[standardOutput retain];
	[consoleTextView clear];

	doomTask = [[NSTask alloc] init];
	[doomTask retain];
	[doomTask setLaunchPath:path];
	[doomTask setArguments:args];
	[doomTask setStandardOutput:standardOutput];

	[launchButton setEnabled:false];
	[doomTask launch];
}

- (void)taskReadTimer:(NSTimer *)timer
{
	if(doomTask == nil)
		return;

	// NSFileHandle doesn't do nonblocking IO, so we'll do it ourselves
	int fd = [[standardOutput fileHandleForReading] fileDescriptor];

	int flags = fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);

	void *buffer = malloc(1000000);
	int size = read(fd, buffer, 1000000);

	if(size > 0)
	{
		NSData *data = [NSData dataWithBytesNoCopy:buffer length:size
		                freeWhenDone:true];

		// Stick the data into the console text view
		NSString *string = [[NSString alloc] initWithData:data
		                    encoding:NSUTF8StringEncoding];
		[consoleTextView appendAnsiString:string];
	}
}

- (void)taskComplete:(NSNotification *)notification
{
	if(doomTask && ![doomTask isRunning])
	{
		// Read last data from stdout
		[self taskReadTimer:nil];

		if ([doomTask terminationStatus] != 0)
			[[consoleWindow windowController] showWindow:nil];
		[doomTask release];
		doomTask = nil;
		[standardOutput release];
		[launchButton setEnabled:true];
	}
}

- (IBAction)gameButtonClicked:(id)sender
{
	[self updateGameWad];
}

- (IBAction)showGameFolderClicked:(id)sender
{
	[[NSWorkspace sharedWorkspace] openFile:[self wadPath] withApplication:@"Finder"];
}

- (IBAction)showConsoleClicked:(id)sender
{
	[[consoleWindow windowController] showWindow:sender];
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

@implementation LaunchCommand

- (id)performDefaultImplementation
{
	// TODO: Factor out the launch code from startClicked: for use here, too.
}

@end
