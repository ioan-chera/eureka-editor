//------------------------------------------------------------------------
//  OS X EDITOR APP DELEGATE IMPLEMENTATION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2013 Ioan Chera
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

// Thanks to Darrell Walisser, Max Horn et al. for their SDLMain.m code -
//   reliable base for cross-platform apps ported to OS X.

#import "AppDelegate.h"

#include "main.h"


static int gArgc;
static char **gArgv;
static BOOL gFinderLaunch;
static BOOL   gCalledAppMainline = FALSE;

// Prototype to cross-platform main entry function
int main_ORIGINAL(int argc, char *argv[]);

@implementation AppDelegate

//
// applicationDataDirectory
//
// Copied from Apple documentation
//
#if 0	// Not used for now…
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

//
// applicationDidFinishLaunching:
//
// After [NSApp run] was called.
// launchMainLine not called immediately because I needed application:openFile:
//
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	// Set install_dir from here (it's dependent on project build settings)
    // Current value (might change): .app package root directory
	install_dir = StringDup([[[NSBundle mainBundle] bundlePath]
							 cStringUsingEncoding:NSUTF8StringEncoding]);

    // home_dir is set inside the main program and doesn't depend on build
	// settings

	[self launchMainLine];	// start the program
}

//
// launchMainLine
//
// Start the program and retrieve the exit code
//
- (void)launchMainLine
{
    int exitcode = main_ORIGINAL(gArgc, gArgv);
    
    exit(exitcode);
}

//
// application:openFile:
//
// Handle OS X UI's non-command-line way of opening files.
// FIXME: handle files open this way at runtime.
//
// Note: code has been borrowed from SDLMain.m.
// SDL license is compatible with GPL v2
//
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

//
// CustomApplicationMain
//
// Replacement for NSApplicationMain. Also borrowed from SDLMain.m
//
static void CustomApplicationMain (int argc, char **argv)
{
    NSAutoreleasePool	*pool = [[NSAutoreleasePool alloc] init];
    AppDelegate				*delegate;
	
    // Ensure the application object is initialised
    [NSApplication sharedApplication];
	
    // Set up the menubar
    [NSApp setMainMenu:[[NSMenu alloc] init]];
	
    // Create SDLMain and make it the app delegate
    delegate = [[AppDelegate alloc] init];
    [NSApp setDelegate:delegate];
	
    // Start the main event loop
    [NSApp run];
	
    [delegate release];
    [pool release];
}

// Undefine main from main_ORIGINAL. This will be the real entry point.
#ifdef main
#undef main
#endif

//
// main
//
// Code borrowed from SDLMain.m
//
int main(int argc, char *argv[])
{
	if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 )
	{
		gArgv = (char **) malloc(sizeof (char *) * 2);
		gArgv[0] = argv[0];
		gArgv[1] = NULL;
		gArgc = 1;
		gFinderLaunch = YES;
	}
	else
	{
		int i;
		gArgc = argc;
		gArgv = (char **) malloc(sizeof (char *) * (argc+1));
		for (i = 0; i <= argc; i++)
			gArgv[i] = argv[i];
		gFinderLaunch = NO;
	}
	
	CustomApplicationMain(gArgc, gArgv);
}
