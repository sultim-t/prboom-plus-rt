// This file is hereby placed in the Public Domain -- Neil Stevens

#import <Cocoa/Cocoa.h>

@interface DrawerButton : NSButton
{
	IBOutlet id drawer;
}

- (void)drawerDidClose:(NSNotification *)notification;
- (void)drawerDidOpen:(NSNotification *)notification;
- (void)updateTitle;

@end
