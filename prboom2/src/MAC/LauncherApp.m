// This file is hereby placed in the Public Domain -- Neil Stevens

#import "ConsoleController.h"
#import "FileButtonController.h"
#import "LauncherApp.h"
#import "UKKQueue.h"
#import "WadViewController.h"

#include <fcntl.h>

static LauncherApp *LApp;

@implementation LauncherApp

- (NSString *)wadPath
{
	return [@"~/Library/Application Support/PrBoom" stringByExpandingTildeInPath];
}

- (void)awakeFromNib
{
	LApp = self;

	[[NSFileManager defaultManager] createDirectoryAtPath:[self wadPath]
	                                attributes:nil];
	[[UKKQueue sharedQueue] setDelegate:self];
	[[UKKQueue sharedQueue] addPath:[self wadPath]];

	[demoFileButtonController
	     setTypes:[NSArray arrayWithObjects:@"lmp", @"LMP", nil]];

	[configFileButtonController
	     setTypes:[NSArray arrayWithObjects:@"cfg", @"CFG", nil]];

	[self loadDefaults];

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
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	[resolutionComboBox setObjectValue:[[resolutionComboBox dataSource]
	                    comboBox:resolutionComboBox
	                    objectValueForItemAtIndex:0]];
	[graphicsModeComboBox
	        setObjectValue:[graphicsModeComboBox itemObjectValueAtIndex:0]];

	if([defaults boolForKey:@"Saved 2.4.6"])
	{
		id res = [defaults objectForKey:@"Resolution"];
		if(NSNotFound != [[resolutionComboBox dataSource]
		                  comboBox:resolutionComboBox
		                  indexOfItemWithStringValue:res])
			[resolutionComboBox setObjectValue:res];

		id mode = [defaults objectForKey:@"Graphics Mode"];
		if(NSNotFound != [graphicsModeComboBox
		                  indexOfItemWithObjectValue:mode])
			[graphicsModeComboBox setObjectValue:[defaults objectForKey:@"Graphics Mode"]];
	}

	if([defaults boolForKey:@"Saved 2.4.5"])
	{
		[window setFrameUsingName:@"Launcher"];
		if([[defaults objectForKey:@"Wad Drawer State"] boolValue])
			[wadDrawer open];
		if([[defaults objectForKey:@"Debug Drawer State"] boolValue])
			[debugDrawer open];
		if([[defaults objectForKey:@"Demo Drawer State"] boolValue])
			[demoDrawer open];
		if([[defaults objectForKey:@"Console State"] boolValue])
		{
			[consoleController showWindow:self];
			[window makeKeyAndOrderFront:self];
		}

		[[configFileButtonController field] setObjectValue:[defaults objectForKey:@"Config File"]];
	}

	[[consoleController window] setFrameUsingName:@"Console"];

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
		[wadViewController setWads:[defaults stringArrayForKey:@"Wads"]];

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
	[self updateGameWad];
}

- (void)saveDefaults
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

	[defaults setBool:true forKey:@"Saved"];
	[defaults setBool:true forKey:@"Saved 2.4.5"];
	[defaults setBool:true forKey:@"Saved 2.4.6"];

	[defaults setObject:[resolutionComboBox objectValue] forKey:@"Resolution"];
	[defaults setObject:[graphicsModeComboBox objectValue] forKey:@"Graphics Mode"];

	[window saveFrameUsingName:@"Launcher"];
	[[consoleController window] saveFrameUsingName:@"Console"];

	[defaults setObject:[NSNumber numberWithBool:[wadDrawer state]] forKey:@"Wad Drawer State"];
	[defaults setObject:[NSNumber numberWithBool:[debugDrawer state]] forKey:@"Debug Drawer State"];
	[defaults setObject:[NSNumber numberWithBool:[demoDrawer state]] forKey:@"Demo Drawer State"];

	[defaults setObject:[NSNumber numberWithBool:[[consoleController window] isVisible]] forKey:@"Console State"];

	[defaults setObject:[[configFileButtonController field] objectValue] forKey:@"Config File"];

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
	[defaults setObject:[wadViewController wads] forKey:@"Wads"];

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

- (void)tryToLaunch
{
	if([launchButton isEnabled])
		[self startClicked:self];
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
		[args insertObject:@"-nowindow" atIndex:[args count]];
	else
		[args insertObject:@"-window" atIndex:[args count]];

	[args insertObject:@"-geom" atIndex:[args count]];
	[args insertObject:[resolutionComboBox objectValue] atIndex:[args count]];

	if([[graphicsModeComboBox objectValue] hasPrefix:@"OpenGL"])
	{
		[args insertObject:@"-vidmode" atIndex:[args count]];
		[args insertObject:@"gl" atIndex:[args count]];
	}
	else if([[graphicsModeComboBox objectValue] hasPrefix:@"8"])
	{
		[args insertObject:@"-vidmode" atIndex:[args count]];
		[args insertObject:@"8" atIndex:[args count]];
	}

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
	if([[[configFileButtonController field] stringValue] length] > 0)
	{
		[args insertObject:@"-config" atIndex:[args count]];
		[args insertObject:[[configFileButtonController field] stringValue] atIndex:[args count]];
	}

	// Extra wads
	[args insertObject:@"-file" atIndex:[args count]];
	int i;
	NSArray *wads = [wadViewController wads];
	for(i = 0; i < [wads count]; ++i)
	{
		NSString *path = [wads objectAtIndex:i];
		if([[path pathExtension] caseInsensitiveCompare:@"wad"] == NSOrderedSame)
			[args insertObject:[wads objectAtIndex:i] atIndex:[args count]];
	}

	// Dehacked
	[args insertObject:@"-deh" atIndex:[args count]];
	for(i = 0; i < [wads count]; ++i)
	{
		NSString *path = [wads objectAtIndex:i];
		if([[path pathExtension] caseInsensitiveCompare:@"deh"] == NSOrderedSame)
			[args insertObject:[wads objectAtIndex:i] atIndex:[args count]];
	}

	// Demo
	if([demoMatrix selectedCell] != noDemoButton)
	{
		if([demoMatrix selectedCell] == playDemoButton)
			[args insertObject:@"-playdemo" atIndex:[args count]];
		else if([demoMatrix selectedCell] == timeDemoButton)
			[args insertObject:@"-timedemo" atIndex:[args count]];
		else if([demoMatrix selectedCell] == fastDemoButton)
			[args insertObject:@"-fastdemo" atIndex:[args count]];

		[args insertObject:[[demoFileButtonController field] stringValue] atIndex:[args count]];

		if([[ffToLevelField stringValue] length] > 0)
		{
			[args insertObject:@"-ffmap" atIndex:[args count]];
			[args insertObject:[ffToLevelField stringValue] atIndex:[args count]];
		}
	}

	[launchButton setEnabled:false];
	[consoleController launch:path args:args delegate:self];
}

- (void)taskEnded:(id)sender
{
	[launchButton setEnabled:true];
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
	[consoleController showWindow:sender];
}

- (IBAction)disableSoundClicked:(id)sender
{
	bool state = [disableSoundButton state] != NSOnState;
	[disableSoundEffectsButton setEnabled:state];
	[disableMusicButton setEnabled:state];
}

- (IBAction)demoButtonClicked:(id)sender
{
	bool enabled = [demoMatrix selectedCell] != noDemoButton;
	[demoFileButtonController setEnabled:enabled];
	[ffToLevelField setEnabled:enabled];
}

@end

@implementation LaunchCommand

- (id)performDefaultImplementation
{
	[LApp tryToLaunch];
}

@end
