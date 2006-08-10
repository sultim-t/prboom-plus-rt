// This file is hereby placed in the Public Domain -- Neil Stevens

#import <Cocoa/Cocoa.h>

@interface FileButtonController : NSObject
{
	IBOutlet NSButton *button;
	IBOutlet id field;

	NSArray *types;
	bool allowMultiple;
}

- (id)init;
- (void)dealloc;

- (void)setTypes:(NSArray *)typeArray;
- (void)setAllowMultiple:(bool)allow;

- (IBAction)buttonClicked:(id)sender;
- (void)panelEnded:(NSOpenPanel *)panel returnCode:(int)code contextInfo:(void *)info;

- (id)field;
- (void)setEnabled:(BOOL)enabled;

@end
