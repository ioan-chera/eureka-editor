//------------------------------------------------------------------------
//  MAIN PROGRAM
//------------------------------------------------------------------------

#include "main.h"


/*
 *  Global variables
 */

// progress during initialisation:
//   0 = nothing yet
//   1 = read early options, set up logging
//   2 = read all options, and possibly a config file
//   3 = inited FLTK, opened main window
int  init_progress;


static void Main_CloseWindow();


/*
 *  Show an error message and terminate the program
 */
#define MSG_BUF_LEN  2000

void FatalError(const char *fmt, ...)
{
	va_list arg_ptr;

	static char buffer[MSG_BUF_LEN];

	va_start(arg_ptr, fmt);
	vsnprintf(buffer, MSG_BUF_LEN-1, fmt, arg_ptr);
	va_end(arg_ptr);

	buffer[MSG_BUF_LEN-1] = 0;

	if (init_progress < 3)
	{
		fprintf(stderr, "\nFATAL ERROR: %s", buffer);
	}

	if (init_progress >= 3)
	{
// FIXME		DLG_ShowError("%s", buffer);

		init_progress = 2;

		Main_CloseWindow();
	}
#ifdef WIN32
	else
	{
		MessageBox(NULL, buffer, "Silage : Error",
		           MB_ICONEXCLAMATION | MB_OK |
				   MB_SYSTEMMODAL | MB_SETFOREGROUND);
	}
#endif

	init_progress = 0;

	exit(2);
}


static void Main_OpenWindow()
{
	/*
	 *  Create the window
	 */
	Fl::visual(FL_RGB);


	Fl::background(236, 232, 228);
	Fl::background2(255, 255, 255);
	Fl::foreground(0, 0, 0);

	Fl::scheme("plastic");


#ifdef UNIX
	Fl_File_Icon::load_system_icons();
#endif


	int screen_w = Fl::w();
	int screen_h = Fl::h();

//	DebugPrintf("Detected Screen Size: %dx%d\n", screen_w, screen_h);


	main_win = new UI_MainWin("Silage v" SILAGE_VERSION);

	// show window (pass some dummy arguments)
	{
		int   argc = 1;
		char *argv[2];

		argv[0] = strdup("Silage.exe");
		argv[1] = NULL;

		main_win->show(argc, argv);
	}

	// kill the stupid bright background of the "plastic" scheme
	{
		delete Fl::scheme_bg_;
		Fl::scheme_bg_ = NULL;

		main_win->image(NULL);
	}
}


static void Main_CloseWindow()
{
}


/* ----- user information ----------------------------- */

static void ShowHelp()
{
	printf(	"\n"
			"*** Silage v" SILAGE_VERSION " ***\n"
			"\n");

	fflush(stdout);
}


/*
 *  the driving program
 */
int main(int argc, char *argv[])
{
	init_progress = 0;

	int r;

	/* TODO: argument handling */

	if (argc >= 2 &&
	    ( strcmp(argv[1], "/?") == 0 ||
	      strcmp(argv[1], "-h") == 0 ||
	      strcmp(argv[1], "-help") == 0 ||
	      strcmp(argv[1], "--help") == 0 ||
	      strcmp(argv[1], "-version") == 0 ||
	      strcmp(argv[1], "--version") == 0 ))
	{
		ShowHelp();
		return 0;
	}


	init_progress = 1;


	Main_OpenWindow();

	init_progress = 3;


    while (! want_quit)
    {
        Fl::wait(0.2);
    }


quit:
	/* that's all folks! */


	init_progress = 2;

	Main_CloseWindow();


	init_progress = 0;

	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
