//
//  AppDelegate.h
//  EurekaApp
//
//  Created by Ioan on 20.11.2012.
//  Copyright (c) 2012 Ioan Chera
//

#import <Cocoa/Cocoa.h>


//int mainCpp();
//void mainQuitCpp();

//extern const char *install_dir;  // install dir (e.g. /usr/share/eureka)
//extern const char *home_dir;     // home dir (e.g. $HOME/.eureka)




// IOAN 20121121
//int mainLine(int argc, char *argv[]);
//int NSMyApplicationMain(int argc, char *argv[]);

//@class EUSettingsWindowController;
@class EUExtraInterface;


@interface AppDelegate : NSObject
{
    NSString *bundlePath;
	NSString *pwadName;
    IBOutlet NSMenu *appMenu;
    
//    EUSettingsWindowController *settingsWindowController;
	
	IBOutlet EUExtraInterface *extraInterface;
}

@property (readonly) NSString *bundlePath;
//@property (readonly) EUSettingsWindowController *settingsWindowController;


- (void)launchMainLine;

@end
