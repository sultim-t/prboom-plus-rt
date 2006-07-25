// Controller.h
//

#import <Cocoa/Cocoa.h>

@interface Controller : NSObject {
	IBOutlet NSTextField *patternField, *replacementField, *matchField;
	IBOutlet NSButton *caseInsensitiveButton, *dotAllButton, *extendedButton, *lazyButton, *multilineButton;
	IBOutlet NSTextView *textView;
	BOOL lastMatchWasEmpty;
}
- (IBAction)find:(id)sender;
- (IBAction)replace:(id)sender;
- (IBAction)openDocument:(id)sender;
@end
