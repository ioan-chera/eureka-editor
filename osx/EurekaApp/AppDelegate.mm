//------------------------------------------------------------------------
//  OS X EDITOR APP DELEGATE IMPLEMENTATION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2013 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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

//
// Module by Ioan Chera
//

// Thanks to Darrell Walisser, Max Horn et al. for their SDLMain.m code -
//   reliable base for cross-platform apps ported to OS X.

#import "AppDelegate.h"

#include "main.h"


static int gArgc;
static char **gArgv;
static BOOL gFinderLaunch;
static BOOL   gCalledAppMainline = FALSE;

// Prototype to cross-platform main entry function
int EurekaMain(int argc, char *argv[]);

@implementation AppDelegate


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
	global::install_dir = [[[NSBundle mainBundle] resourcePath]
				   cStringUsingEncoding:NSUTF8StringEncoding];

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
    int exitcode = EurekaMain(gArgc, gArgv);
    
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
//static void CustomApplicationMain (int argc, char **argv)
//{
//    NSAutoreleasePool	*pool = [[NSAutoreleasePool alloc] init];
//    AppDelegate				*delegate;
//	
//    // Ensure the application object is initialised
//    [NSApplication sharedApplication];
//	
//    // Set up the menubar
//    [NSApp setMainMenu:[[NSMenu alloc] init]];
//	
//    // Create SDLMain and make it the app delegate
//    delegate = [[AppDelegate alloc] init];
//    [[NSApplication sharedApplication] setDelegate:delegate];
//	
//    // Start the main event loop
//    [NSApp run];
//	
//    [delegate release];
//    [pool release];
//}

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
	
	// Set install_dir from here (it's dependent on project build settings)
	// Current value (might change): .app package root directory
	global::install_dir = [[[NSBundle mainBundle] resourcePath]
				   cStringUsingEncoding:NSUTF8StringEncoding];

	return EurekaMain(gArgc, gArgv);
	
//	CustomApplicationMain(gArgc, gArgv);
}
