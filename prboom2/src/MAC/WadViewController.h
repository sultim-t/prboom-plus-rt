// This file is hereby placed in the Public Domain -- Neil Stevens

#import <Cocoa/Cocoa.h>

@interface WadViewController : NSObject
{
	// Wad options
	NSMutableArray *wads;

	IBOutlet id view;
	IBOutlet id removeButton;
}

- (id)init;
- (void)dealloc;

// UI
- (IBAction)add:(id)sender;
- (void)addEnded:(NSOpenPanel *)panel returnCode:(int)code contextInfo:(void *)info;
- (IBAction)remove:(id)sender;

// Preferences saving
- (NSArray *)wads;
- (void)setWads:(NSArray *)newWads;

// Table view and data source
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
