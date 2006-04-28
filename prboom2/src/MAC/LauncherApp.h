#import <Cocoa/Cocoa.h>

@interface LauncherApp : NSObject
{
    IBOutlet id chooseDemoFileButton;
    IBOutlet id compatibilityLevelButton;
    IBOutlet id compatMenu;
    IBOutlet id demoFileField;
    IBOutlet id demoMatrix;
    IBOutlet id disableGraphicsButton;
    IBOutlet id disableJoystickButton;
    IBOutlet id disableMouseButton;
    IBOutlet id disableMusicButton;
    IBOutlet id disableSoundButton;
    IBOutlet id disableSoundEffectsButton;
    IBOutlet id fastDemoButton;
    IBOutlet id fastMonstersButton;
    IBOutlet id gameButton;
    IBOutlet id gameMenu;
    IBOutlet id mainMenu;
    IBOutlet id noDemoButton;
    IBOutlet id noMonstersButton;
    IBOutlet id playDemoButton;
    IBOutlet id playDemoWarpButton;
    IBOutlet id removeWadButton;
    IBOutlet id respawnMonstersButton;
    IBOutlet id timeDemoButton;
    IBOutlet id wadView;
    IBOutlet id warpToLevelField;
    IBOutlet id window;

    NSMutableArray *wads;
}

- (void)awakeFromNib;

- (IBAction)startClicked:(id)sender;

- (IBAction)disableSoundClicked:(id)sender;

- (IBAction)chooseDemoFileClicked:(id)sender;
- (void)chooseDemoFileEnded:(NSOpenPanel *)panel returnCode:(int)code contextInfo:(void *)info;
- (IBAction)demoButtonClicked:(id)sender;

// Wad handling
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
