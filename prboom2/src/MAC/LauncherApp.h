#import <Cocoa/Cocoa.h>

@interface LauncherApp : NSObject
{
	IBOutlet id window;

	// Game
	IBOutlet id compatibilityLevelButton;
	IBOutlet id gameButton;

	// Options
	IBOutlet id respawnMonstersButton;
	IBOutlet id fastMonstersButton;
	IBOutlet id noMonstersButton;

	// Debug options
	IBOutlet id disableGraphicsButton;
	IBOutlet id disableJoystickButton;
	IBOutlet id disableMouseButton;
	IBOutlet id disableMusicButton;
	IBOutlet id disableSoundButton;
	IBOutlet id disableSoundEffectsButton;

	// Wad options
	NSMutableArray *wads;

	IBOutlet id wadView;
	IBOutlet id removeWadButton;

	// Demo options
	IBOutlet id noDemoButton;
	IBOutlet id playDemoButton;
	IBOutlet id fastDemoButton;
	IBOutlet id timeDemoButton;
	IBOutlet id demoMatrix;

	IBOutlet id ffToLevelField;
	IBOutlet id chooseDemoFileButton;
	IBOutlet id demoFileField;
}

- (void)awakeFromNib;
- (void)windowWillClose:(NSNotification *)notification;

- (void)loadDefaults;
- (void)saveDefaults;

// Game
- (IBAction)startClicked:(id)sender;
- (IBAction)gameButtonClicked:(id)sender;

// Options
- (IBAction)disableSoundClicked:(id)sender;

// Demo options
- (IBAction)chooseDemoFileClicked:(id)sender;
- (void)chooseDemoFileEnded:(NSOpenPanel *)panel returnCode:(int)code contextInfo:(void *)info;
- (IBAction)demoButtonClicked:(id)sender;

// Wad options
- (IBAction)addWadClicked:(id)sender;
- (void)addWadEnded:(NSOpenPanel *)panel returnCode:(int)code contextInfo:(void *)info;
- (IBAction)removeWadClicked:(id)sender;

// Wad table view and data source
- (void)tableViewSelectionDidChange:(NSNotification *)notification;
- (int)numberOfRowsInTableView:(NSTableView *)tableView;
- (id)tableView:(NSTableView *)tableView
                objectValueForTableColumn:(NSTableColumn *)column
                row:(int)row;
- (void)tableView:(NSTableView *)tableView
                  setObjectValue:(id)object
                  forTableColumn:(NSTableColumn *)column
                  row:(int)row;
@end

@interface LaunchCommand : NSScriptCommand
- (id)performDefaultImplementation;
@end
