//
//  AppDelegate.m
//  EurekaApp
//
//  Created by Ioan on 20.11.2012.
//  Copyright (c) 2012 Ioan Chera
//

#import "AppDelegate.h"

//#define main mainLine
#include "main.h"


static int gArgc;
static char **gArgv;
static BOOL gFinderLaunch;
static BOOL   gCalledAppMainline = FALSE;


int main_ORIGINAL(int argc, char *argv[]);

@implementation AppDelegate
@synthesize bundlePath;
//@synthesize settingsWindowController;

- (id)init
{
	self = [super init];
	if(self)
	{
		pwadName = nil;
	}
	return self;
}
- (void)dealloc
{
	[pwadName release];
    [super dealloc];
}

//
// applicationDataDirectory
//
// Copied from Apple documentation
//

#if 0   // DISABLED: URLsForDirectory requires OS X 10.6. Minimum supported should be 10.5.
- (NSURL*)applicationDataDirectory
{
    NSFileManager* sharedFM = [NSFileManager defaultManager];
    NSArray* possibleURLs = [sharedFM URLsForDirectory:NSApplicationSupportDirectory
											 inDomains:NSUserDomainMask];
    NSURL* appSupportDir = nil;
    NSURL* appDirectory = nil;
	
    if ([possibleURLs count] >= 1) {
        // Use the first directory (if multiple are returned)
        appSupportDir = [possibleURLs objectAtIndex:0];
    }
	
    // If a valid app support directory exists, add the
    // app's bundle ID to it to specify the final directory.
    if (appSupportDir) {
        NSString* appBundleID = [[NSBundle mainBundle] bundleIdentifier];
        appDirectory = [appSupportDir URLByAppendingPathComponent:appBundleID];
    }
	
    return appDirectory;
}
#endif

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
    // Set working directory
    // prepare working directory

	// FIXME: not done yet in the C++ code
	bundlePath = [[NSBundle mainBundle] bundlePath];

	install_dir = StringDup([bundlePath cStringUsingEncoding:NSUTF8StringEncoding]);
    // home_dir set inside the main program

	[self launchMainLine];
}


- (void)launchMainLine
{
    int exitcode = main_ORIGINAL(gArgc, gArgv);
    
    exit(exitcode);
}

// Note: some data has been borrowed from SDLMain.m
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	const char *temparg;
    size_t arglen;
    char *arg;
    char **newargv;
	
    if (!gFinderLaunch)  /* MacOS is passing command line args. */
        return FALSE;
	
    if (gCalledAppMainline)  /* app has started, ignore this document. */
        return FALSE;
	
    temparg = [filename UTF8String];
    arglen = strlen(temparg) + 1;
    arg = (char *) malloc(arglen);
    if (arg == NULL)
        return FALSE;
	
    newargv = (char **) realloc(gArgv, sizeof (char *) * (gArgc + 2));
    if (newargv == NULL)
    {
        free(arg);
        return FALSE;
    }
    gArgv = newargv;
	
    strlcpy(arg, temparg, arglen);
    gArgv[gArgc++] = arg;
    gArgv[gArgc] = NULL;
    return TRUE;
}
@end

/* Replacement for NSApplicationMain */

static void CustomApplicationMain (int argc, char **argv)
{
    NSAutoreleasePool	*pool = [[NSAutoreleasePool alloc] init];
    AppDelegate				*delegate;
	
    /* Ensure the application object is initialised */
    [NSApplication sharedApplication];
	
    /* Set up the menubar */
    [NSApp setMainMenu:[[NSMenu alloc] init]];
	
    /* Create SDLMain and make it the app delegate */
    delegate = [[AppDelegate alloc] init];
    [NSApp setDelegate:delegate];
	
    /* Start the main event loop */
    [NSApp run];
	
    [delegate release];
    [pool release];
}

#ifdef main
#undef main
#endif

int main(int argc, char *argv[])
{
	// FIXME: doesn't seem to work from command-line…
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        gArgv = (char **) malloc(sizeof (char *) * 2);
        gArgv[0] = argv[0];
        gArgv[1] = NULL;
        gArgc = 1;
        gFinderLaunch = YES;
    } else {
        int i;
        gArgc = argc;
        gArgv = (char **) malloc(sizeof (char *) * (argc+1));
        for (i = 0; i <= argc; i++)
            gArgv[i] = argv[i];
        gFinderLaunch = NO;
    }
	
	
	// Preprocessor definition: main=mainLine
	//return mainLine(gArgc, gArgv);
	CustomApplicationMain(gArgc, gArgv);
}
