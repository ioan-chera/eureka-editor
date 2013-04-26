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
	if (main_win)
		delete main_win;
	
	main_win = NULL;
}


void Main_ProgStatus(const char *msg)
{
	main_win->Prog_Clear();
	main_win->progress->label(msg);
}


//------------------------------------------------------------------------

#include "glbsp.h"

static glbsp::nodebuildinfo_t nb_info;
static volatile glbsp::nodebuildcomms_t nb_comms;

static int display_mode = glbsp::DIS_INVALID;
static int progress_limit;
static int progress_last;

static char message_buf[MSG_BUF_LEN];

#define CANCEL_COLOR  fl_color_cube(3,1,1)


static const char *glbsp_ErrorString(glbsp::glbsp_ret_e ret)
{
	switch (ret)
	{
		case glbsp::GLBSP_E_OK: return "OK";

		 // the arguments were bad/inconsistent.
		case glbsp::GLBSP_E_BadArgs: return "Bad Arguments";

		// the info was bad/inconsistent, but has been fixed
		case glbsp::GLBSP_E_BadInfoFixed: return "Bad Args (fixed)";

		// file errors
		case glbsp::GLBSP_E_ReadError:  return "Read Error";
		case glbsp::GLBSP_E_WriteError: return "Write Error";

		// building was cancelled
		case glbsp::GLBSP_E_Cancelled: return "Cancelled by User";

		// an unknown error occurred (this is the catch-all value)
		case glbsp::GLBSP_E_Unknown:

		default: return "Unknown Error";
	}
}


static void GB_PrintMsg(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsnprintf(message_buf, MSG_BUF_LEN, str, args);
	va_end(args);

	message_buf[MSG_BUF_LEN-1] = 0;

//??	LogPrintf("GLBSP: %s", message_buf);
}

static void GB_FatalError(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsnprintf(message_buf, MSG_BUF_LEN, str, args);
	va_end(args);

	message_buf[MSG_BUF_LEN-1] = 0;

	FatalError("glBSP Failure:\n%s", message_buf);
	/* NOT REACHED */
}

static void GB_Ticker(void)
{
	static int test;

	if (((++test) & 1) != 0)
		return;

	Fl::check();

	if (want_quit /* now a "cancel" button */)
	{
		want_quit = false;

		nb_comms.cancelled = TRUE;
	}
}

static glbsp::boolean_g GB_DisplayOpen(glbsp::displaytype_e type)
{
	display_mode = type;
	return TRUE;
}

static void GB_DisplaySetTitle(const char *str)
{
	/* does nothing */
}

static void GB_DisplaySetBarText(int barnum, const char *str)
{
	if (display_mode == glbsp::DIS_BUILDPROGRESS && barnum == 1)
	{
		// needed ??
	}
}

static void GB_DisplaySetBarLimit(int barnum, int limit)
{
	if (display_mode == glbsp::DIS_BUILDPROGRESS && barnum == 2)
	{
		progress_limit = MAX(1, limit);
		progress_last  = -1;
	}
}

static void GB_DisplaySetBar(int barnum, int count)
{
	if (display_mode == glbsp::DIS_BUILDPROGRESS && barnum == 2)
	{
		int perc = count * 100.0 / progress_limit;

		if (perc > progress_last)
		{
			main_win->Prog_Set(perc);

			progress_last = perc;
		}
	}
}

static void GB_DisplayClose(void)
{
	/* does nothing */
}

static const glbsp::nodebuildfuncs_t  build_funcs =
{
	GB_FatalError,
	GB_PrintMsg,
	GB_Ticker,

	GB_DisplayOpen,
	GB_DisplaySetTitle,
	GB_DisplaySetBar,
	GB_DisplaySetBarLimit,
	GB_DisplaySetBarText,
	GB_DisplayClose
};


static bool Main_RunGLBSP(const char *filename)
{
	display_mode = glbsp::DIS_INVALID;

	memcpy(&nb_info,  &glbsp::default_buildinfo,  sizeof(glbsp::default_buildinfo));
	memcpy((void*)&nb_comms, &glbsp::default_buildcomms, sizeof(glbsp::nodebuildcomms_t));

	nb_info.input_file  = glbsp::GlbspStrDup(filename);
	nb_info.output_file = glbsp::GlbspStrDup(filename);

	nb_info.fast          = FALSE;
	nb_info.quiet         = TRUE;
	nb_info.mini_warnings = FALSE;

	nb_info.pack_sides   = FALSE;
	nb_info.force_normal = TRUE;


	glbsp::glbsp_ret_e  ret;

	ret = glbsp::CheckInfo(&nb_info, &nb_comms);

	if (ret != glbsp::GLBSP_E_OK)
	{
		// check info failure (unlikely to happen)
		GB_PrintMsg("\n");
		GB_PrintMsg("Param Check FAILED: %s\n", glbsp_ErrorString(ret));
		GB_PrintMsg("Reason: %s\n\n", nb_comms.message);
		Main_ProgStatus("glBSP Error");
		return false;
	}


	ret = glbsp::BuildNodes(&nb_info, &build_funcs, &nb_comms);

	if (ret == glbsp::GLBSP_E_Cancelled)
	{
		GB_PrintMsg("\n");
		GB_PrintMsg("Building CANCELLED.\n\n");
		Main_ProgStatus("Cancelled");
		return false;
	}

	if (ret != glbsp::GLBSP_E_OK)
	{
		// build nodes failed
		GB_PrintMsg("\n");
		GB_PrintMsg("Building FAILED: %s\n", glbsp_ErrorString(ret));
		GB_PrintMsg("Reason: %s\n\n", nb_comms.message);
		Main_ProgStatus("glBSP Error");
		return false;
	}

	Main_ProgStatus("Success");

	return true;
}


static void Main_Build()
{
	want_build = false;

	main_win->Prog_Clear();


	Fl_Native_File_Chooser chooser;

	chooser.title("Pick output filename");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.filter("Wads\t*.wad");

	//??  chooser.directory("xxx");

	switch (chooser.show())
	{
		case -1:
//			LogPrintf("Export Map: error choosing file:\n");
//			LogPrintf("   %s\n", chooser.errmsg());

			fl_alert("%s", chooser.errmsg());
			return;

		case 1:
//			LogPrintf("Export Map: cancelled by user\n");
			return;

		default:
			break;  // OK
	}

	/// if extension is missing then add ".wad"
	char filename[FL_PATH_MAX];

	strcpy(filename, chooser.filename());

	char *pos = (char *)fl_filename_ext(filename);
	if (! *pos)
		strcat(filename, ".wad");

fprintf(stderr, "BUILD FILENAME = '%s'\n", filename);


	// convert quit button into a "cancel" button
	main_win->quit->label("Cancel");
	main_win->quit->labelcolor(CANCEL_COLOR);
	main_win->quit->labelfont(FL_HELVETICA_BOLD);

	Fl::wait(0.2);


	Main_RunGLBSP(filename);

	main_win->quit->label("Quit");
	main_win->quit->labelcolor(FL_BLACK);
	main_win->quit->labelfont(FL_HELVETICA);

	Fl::wait(0.2);
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

		if (want_build)
			Main_Build();
    }


	/* that's all folks! */


	init_progress = 2;

	Main_CloseWindow();


	init_progress = 0;

	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
