#import "LauncherApp.h"

@implementation LauncherApp

- (void)awakeFromNib
{
	[self disableSoundClicked:disableSoundButton];
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
	printf("%s\n", [path UTF8String]);
	NSArray *array = [NSArray array];
	NSTask *task = [NSTask launchedTaskWithLaunchPath:path arguments:array];
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
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
