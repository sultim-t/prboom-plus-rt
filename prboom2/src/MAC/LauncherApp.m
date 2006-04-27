#import "LauncherApp.h"

@implementation LauncherApp

- (void)awakeFromNib
{
	[self disableSoundClicked:disableSoundButton];
	[self demoButtonClicked:demoMatrix];
	[self tableViewSelectionDidChange:nil];
}

- (IBAction)addWadClicked:(id)sender
{
}

- (IBAction)chooseDemoFileClicked:(id)sender
{
}

- (IBAction)disableSoundClicked:(id)sender
{
	bool state = [disableSoundButton state] != NSOnState;
	[disableSoundEffectsButton setEnabled:state];
	[disableMusicButton setEnabled:state];
}

- (IBAction)removeWadClicked:(id)sender
{
}

- (IBAction)startClicked:(id)sender
{
	NSString *path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"game"];
	NSArray *array = [NSArray array];
	NSTask *task = [NSTask launchedTaskWithLaunchPath:path arguments:array];
}

- (IBAction)demoButtonClicked:(id)sender
{
	id selected = [demoMatrix selectedCell];
	[chooseDemoFileButton setEnabled:selected != noDemoButton];
	[demoFileField setEnabled:selected != noDemoButton];
	[warpToLevelField setEnabled:selected == playDemoWarpButton];
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
	[removeWadButton setEnabled:([wadView selectedRow] > -1)];
}

- (int)numberOfRowsInTableView:(NSTableView *)tableView
{
	return 0;
}

- (id)tableView:(NSTableView *)tableView
                objectValueForTableColumn:(NSTableColumn *)column
                row:(int)row
{
	return @"";
}

- (void)tableView:(NSTableView *)tableView
                  setObjectValue:(id)object
                  forTableColumn:(NSTableColumn *)column
                  row:(int)row
{
}

@end
