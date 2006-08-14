// This file is hereby placed in the Public Domain -- Neil Stevens

#import <Cocoa/Cocoa.h>

@interface LauncherApp : NSObject
{
	IBOutlet id window;

	// Game
	IBOutlet id compatibilityLevelButton;
	IBOutlet id gameButton;
	IBOutlet id launchButton;
	IBOutlet id gameMenu;

	// Options
	IBOutlet id respawnMonstersButton;
	IBOutlet id fastMonstersButton;
	IBOutlet id noMonstersButton;

	IBOutlet id fullscreenButton;
	IBOutlet id resolutionComboBox;
	IBOutlet id graphicsModeComboBox;

	// Debug options
	IBOutlet id disableGraphicsButton;
	IBOutlet id disableJoystickButton;
	IBOutlet id disableMouseButton;
	IBOutlet id disableMusicButton;
	IBOutlet id disableSoundButton;
	IBOutlet id disableSoundEffectsButton;
	IBOutlet id configFileButtonController;

	// Demo options
	IBOutlet id noDemoButton;
	IBOutlet id playDemoButton;
	IBOutlet id fastDemoButton;
	IBOutlet id timeDemoButton;
	IBOutlet id demoMatrix;

	IBOutlet id ffToLevelField;
	IBOutlet id demoFileButtonController;

	// Wad options
	IBOutlet id wadViewController;

	// Drawers
	IBOutlet id wadDrawer;
	IBOutlet id demoDrawer;
	IBOutlet id debugDrawer;

	// Console
	IBOutlet id consoleController;
}

- (NSString *)wadPath;
- (void)awakeFromNib;
- (void)windowWillClose:(NSNotification *)notification;

- (IBAction)openWebsite:(id)sender;

- (void)loadDefaults;
- (void)saveDefaults;

- (NSString *)wadForIndex:(int)index;
- (NSString *)selectedWad;
- (void)updateGameWad;
- (void)watcher:(id)watcher receivedNotification:(NSString *)notification
        forPath:(NSString *)path;

// Game
- (void)tryToLaunch;
- (IBAction)startClicked:(id)sender;
- (void)taskEnded:(id)sender;
- (IBAction)gameButtonClicked:(id)sender;

// Tools
- (IBAction)showGameFolderClicked:(id)sender;
- (IBAction)showConsoleClicked:(id)sender;

// Options
- (IBAction)disableSoundClicked:(id)sender;

// Demo options
- (IBAction)demoButtonClicked:(id)sender;

@end

@interface LaunchCommand : NSScriptCommand
- (id)performDefaultImplementation;
@end
