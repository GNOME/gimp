/*
 * "$Id$"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contents:
 *
 *   main()                   - Main entry - just call gimp_main()...
 *   query()                  - Respond to a plug-in query...
 *   run()                    - Run the plug-in...
 *   print_dialog()           - Pop up the print dialog...
 *   dialog_create_ivalue()   - Create an integer value control...
 *   dialog_iscale_update()   - Update the value field using the scale.
 *   dialog_ientry_update()   - Update the value field using the text entry.
 *   print_driver_callback()  - Update the current printer driver...
 *   media_size_callback()    - Update the current media size...
 *   print_command_callback() - Update the print command...
 *   output_type_callback()   - Update the current output type...
 *   print_callback()         - Start the print...
 *   cancel_callback()        - Cancel the print...
 *   close_callback()         - Exit the print dialog application.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#include "config.h"

#include "print_gimp.h"

#ifdef GIMP_1_0
#include <math.h>
#endif

#include <signal.h>
#include <sys/wait.h>
#ifdef __EMX__
#define INCL_DOSDEVICES
#define INCL_DOSERRORS
#include <os2.h>
#endif

#include "libgimp/stdplugins-intl.h"


/*
 * Local functions...
 */

static void	printrc_load(void);
void	        printrc_save(void);
static int	compare_printers(plist_t *p1, plist_t *p2);
static void	get_system_printers(void);

static void	query (void);
static void	run (char *, int, GParam *, int *, GParam **);
static int	do_print_dialog (char *proc_name);

extern void     gimp_create_main_window (void);

#if 0
static void	cleanupfunc(void);
#endif

/*
 * Globals...
 */

GPlugInInfo	PLUG_IN_INFO =		/* Plug-in information */
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

vars_t vars =
{
	"",			/* Name of file or command to print to */
	"ps2",			/* Name of printer "driver" */
	"",			/* Name of PPD file */
	OUTPUT_COLOR,		/* Color or grayscale output */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	100,			/* Output brightness */
	100.0,			/* Scaling (100% means entire printable area, */
				/*          -XXX means scale by PPI) */
	-1,			/* Orientation (-1 = automatic) */
	-1,			/* X offset (-1 = center) */
	-1,			/* Y offset (-1 = center) */
	1.0,			/* Screen gamma */
	100,			/* Contrast */
	100,			/* Red */
	100,			/* Green */
	100,			/* Blue */
	0,			/* Linear */
	1.0,			/* Output saturation */
	1.0,			/* Density */
	IMAGE_CONTINUOUS,	/* Image type */
	0,			/* Unit 0=Inch */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0			/* Page height */
};

int		plist_current = 0,	/* Current system printer */
		plist_count = 0;	/* Number of system printers */
plist_t		*plist;			/* System printers */

int		saveme = FALSE;		/* True if print should proceed */
int		runme = FALSE;		/* True if print should proceed */
const printer_t *current_printer = 0;	/* Current printer index */
gint32          image_ID;	        /* image ID */

const char *image_filename;
int image_width;
int image_height;

static void
check_plist(int count)
{
  static int current_plist_size = 0;
  if (count <= current_plist_size)
    return;
  else if (current_plist_size == 0)
    {
      current_plist_size = count;
      plist = malloc(current_plist_size * sizeof(plist_t));
    }
  else
    {
      current_plist_size *= 2;
      if (current_plist_size < count)
	current_plist_size = count;
      plist = realloc(plist, current_plist_size * sizeof(plist_t));
    }
}

/*
 * 'main()' - Main entry - just call gimp_main()...
 */

#if 0
int
main(int  argc,		/* I - Number of command-line args */
     char *argv[])	/* I - Command-line args */
{
  return (gimp_main(argc, argv));
}
#else
MAIN()
#endif

static int print_finished = 0;

#if 0
void
cleanupfunc(void)
{
}
#endif

/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query (void)
{
  static GParamDef	args[] =
  {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Input drawable" },
    { PARAM_STRING,	"output_to",	"Print command or filename (| to pipe to command)" },
    { PARAM_STRING,	"driver",	"Printer driver short name" },
    { PARAM_STRING,	"ppd_file",	"PPD file" },
    { PARAM_INT32,	"output_type",	"Output type (0 = gray, 1 = color)" },
    { PARAM_STRING,	"resolution",	"Resolution (\"300\", \"720\", etc.)" },
    { PARAM_STRING,	"media_size",	"Media size (\"Letter\", \"A4\", etc.)" },
    { PARAM_STRING,	"media_type",	"Media type (\"Plain\", \"Glossy\", etc.)" },
    { PARAM_STRING,	"media_source",	"Media source (\"Tray1\", \"Manual\", etc.)" },
    { PARAM_INT32,	"brightness",	"Brightness (0-400%)" },
    { PARAM_FLOAT,	"scaling",	"Output scaling (0-100%, -PPI)" },
    { PARAM_INT32,	"orientation",	"Output orientation (-1 = auto, 0 = portrait, 1 = landscape)" },
    { PARAM_INT32,	"left",		"Left offset (points, -1 = centered)" },
    { PARAM_INT32,	"top",		"Top offset (points, -1 = centered)" },
    { PARAM_FLOAT,	"gamma",	"Output gamma (0.1 - 3.0)" },
    { PARAM_INT32,	"contrast",	"Contrast" },
    { PARAM_INT32,	"red",		"Red level" },
    { PARAM_INT32,	"green",	"Green level" },
    { PARAM_INT32,	"blue",		"Blue level" },
    { PARAM_INT32,	"linear",	"Linear output (0 = normal, 1 = linear)" },
    { PARAM_INT32,	"image_type",	"Image type (0 = line art, 1 = solid tones, 2 = continuous tone, 3 = monochrome)"},
    { PARAM_FLOAT,	"saturation",	"Saturation (0-1000%)" },
    { PARAM_FLOAT,	"density",	"Density (0-200%)" },
    { PARAM_STRING,	"ink_type",	"Type of ink or cartridge" },
    { PARAM_STRING,	"dither_algorithm", "Dither algorithm" },
    { PARAM_INT32,	"unit",		"Unit 0=Inches 1=Metric" },
  };
  static gint nargs = sizeof(args) / sizeof(args[0]);

  static gchar *blurb = "This plug-in prints images from The GIMP.";
  static gchar *help  = "Prints images to PostScript, PCL, or ESC/P2 printers.";
  static gchar *auth  = "Michael Sweet <mike@easysw.com> and Robert Krawitz <rlk@alum.mit.edu>";
  static gchar *copy  = "Copyright 1997-2000 by Michael Sweet and Robert Krawitz";
  static gchar *types = "RGB*,GRAY*,INDEXED*";

  gimp_install_procedure ("file_print",
			  blurb, help, auth, copy,
			  PLUG_IN_VERSION,
			  N_("<Image>/File/Print..."),
			  types,
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
}


#ifdef __EMX__
static char *
get_tmp_filename()
{
  char *tmp_path, *s, filename[80];

  tmp_path = getenv("TMP");
  if (tmp_path == NULL)
    tmp_path = "";

  sprintf(filename, "gimp_print_tmp.%d", getpid());
  s = tmp_path = g_strconcat(tmp_path, "\\", filename, NULL);
  if (!s)
    return NULL;
  for ( ; *s; s++)
    if (*s == '/') *s = '\\';
  return tmp_path;
}
#endif

/*
 * 'usr1_handler()' - Make a note when we receive SIGUSR1.
 */

static volatile int usr1_interrupt;

static void
usr1_handler (int signal)
{
  usr1_interrupt = 1;
}

/*
 * 'run()' - Run the plug-in...
 */

/* #define DEBUG_STARTUP */


#ifdef DEBUG_STARTUP
volatile int SDEBUG = 1;
#endif

static void
run (char   *name,		/* I - Name of print program. */
     int    nparams,		/* I - Number of parameters passed in */
     GParam *param,		/* I - Parameter values */
     int    *nreturn_vals,	/* O - Number of return values */
     GParam **return_vals)	/* O - Return values */
{
  GDrawable	*drawable;	/* Drawable for image */
  GRunModeType	 run_mode;	/* Current run mode */
  FILE		*prn;		/* Print file/command */
  int		 ncolors;	/* Number of colors in colormap */
  GParam	*values;	/* Return values */
#ifdef __EMX__
  char		*tmpfile;	/* temp filename */
#endif
  gint32         drawable_ID;   /* drawable ID */
#ifndef GIMP_1_0
  GimpExportReturnType export = EXPORT_CANCEL;    /* return value of gimp_export_image() */
#endif
  int		ppid = getpid (), /* PID of plugin */
		opid,		/* PID of output process */
		cpid = 0,	/* PID of control/monitor process */
		pipefd[2];	/* Fds of the pipe connecting all the above */
  int		dummy;
#ifdef DEBUG_STARTUP
  while (SDEBUG)
    ;
#endif

  INIT_LOCALE ("gimp-print");

  /*
   * Initialize parameter data...
   */

  current_printer = get_printer_by_index (0);
  run_mode = param[0].data.d_int32;

  values = g_new (GParam, 1);

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  image_ID = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  image_filename = gimp_image_get_filename (image_ID);
  if (strchr(image_filename, '/'))
    image_filename = strrchr(image_filename, '/') + 1;

#ifndef GIMP_1_0
  /*  eventually export the image */
  switch (run_mode)
    {
    case RUN_INTERACTIVE:
    case RUN_WITH_LAST_VALS:
      gimp_ui_init ("print", TRUE);
      export = gimp_export_image (&image_ID, &drawable_ID, "Print",
				  (CAN_HANDLE_RGB |
				   CAN_HANDLE_GRAY |
				   CAN_HANDLE_INDEXED |
				   CAN_HANDLE_ALPHA));
      if (export == EXPORT_CANCEL)
	{
	  *nreturn_vals = 1;
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	  return;
	}
      break;
    default:
      break;
    }
#endif

  /*
   * Get drawable...
   */

  drawable = gimp_drawable_get (drawable_ID);

  image_width  = drawable->width;
  image_height = drawable->height;

  /*
   * See how we will run
   */

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_NAME, &vars);
      vars.page_width = 0;
      vars.page_height = 0;

      current_printer = get_printer_by_driver (vars.driver);

      /*
       * Get information from the dialog...
       */

      if (!do_print_dialog (name))
	goto cleanup;
      break;

    case RUN_NONINTERACTIVE:
      /*
       * Make sure all the arguments are present...
       */
      if (nparams < 11)
	values[0].data.d_status = STATUS_CALLING_ERROR;
      else
	{
	  strcpy (vars.output_to, param[3].data.d_string);
	  strcpy (vars.driver, param[4].data.d_string);
	  strcpy (vars.ppd_file, param[5].data.d_string);
	  vars.output_type = param[6].data.d_int32;
	  strcpy (vars.resolution, param[7].data.d_string);
	  strcpy (vars.media_size, param[8].data.d_string);
	  strcpy (vars.media_type, param[9].data.d_string);
	  strcpy (vars.media_source, param[10].data.d_string);

          if (nparams > 11)
	    vars.brightness = param[11].data.d_int32;
	  else
	    vars.brightness = 100;

          if (nparams > 12)
            vars.scaling = param[12].data.d_float;
          else
            vars.scaling = 100.0;

          if (nparams > 13)
            vars.orientation = param[13].data.d_int32;
          else
            vars.orientation = -1;

          if (nparams > 14)
            vars.left = param[14].data.d_int32;
          else
            vars.left = -1;

          if (nparams > 15)
            vars.top = param[15].data.d_int32;
          else
            vars.top = -1;

          if (nparams > 16)
            vars.gamma = param[16].data.d_float;
          else
            vars.gamma = 1.0;

          if (nparams > 17)
	    vars.contrast = param[17].data.d_int32;
	  else
	    vars.contrast = 100;

          if (nparams > 18)
	    vars.red = param[18].data.d_int32;
	  else
	    vars.red = 100;

          if (nparams > 19)
	    vars.green = param[19].data.d_int32;
	  else
	    vars.green = 100;

          if (nparams > 20)
	    vars.blue = param[20].data.d_int32;
	  else
	    vars.blue = 100;

          if (nparams > 21)
            vars.linear = param[21].data.d_int32;
          else
            vars.linear = 0;

          if (nparams > 22)
            vars.image_type = param[22].data.d_int32;
          else
            vars.image_type = IMAGE_CONTINUOUS;

          if (nparams > 23)
            vars.saturation = param[23].data.d_float;
          else
            vars.saturation = 1.0;

          if (nparams > 24)
            vars.density = param[24].data.d_float;
          else
            vars.density = 1.0;

	  if (nparams > 25)
	    strcpy (vars.ink_type, param[25].data.d_string);
	  else
	    memset (vars.ink_type, 0, 64);

	  if (nparams > 26)
	    strcpy (vars.dither_algorithm, param[26].data.d_string);
	  else
	    memset (vars.dither_algorithm, 0, 64);

          if (nparams > 27)
            vars.unit = param[27].data.d_int32;
          else
            vars.unit = 0;
	}

      current_printer = get_printer_by_driver (vars.driver);
      break;

    case RUN_WITH_LAST_VALS:
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_NAME, &vars);
      vars.page_width = 0;
      vars.page_height = 0;

      current_printer = get_printer_by_driver (vars.driver);
      break;

    default:
      values[0].data.d_status = STATUS_CALLING_ERROR;
      break;;
    }

  /*
   * Print the image...
   */
  if (values[0].data.d_status == STATUS_SUCCESS)
    {
      /*
       * Set the tile cache size...
       */

      if (drawable->height > drawable->width)
	gimp_tile_cache_ntiles ((drawable->height + gimp_tile_width () - 1) /
				gimp_tile_width () + 1);
      else
	gimp_tile_cache_ntiles ((drawable->width + gimp_tile_width () - 1) /
				gimp_tile_width () + 1);

      /*
       * Open the file/execute the print command...
       */

      if (plist_current > 0)
#ifndef __EMX__
      {
	/*
	 * The following IPC code is only necessary because the GIMP kills
	 * plugins with SIGKILL if its "Cancel" button is pressed; this
	 * gives the plugin no chance whatsoever to clean up after itself.
	 */
	usr1_interrupt = 0;
	signal (SIGUSR1, usr1_handler);
	if (pipe (pipefd) != 0) {
	  prn = NULL;
	} else {
	  cpid = fork ();
	  if (cpid < 0) {
	    prn = NULL;
	  } else if (cpid == 0) {
	    /* LPR monitor process.  Printer output is piped to us. */
	    opid = fork ();
	    if (opid < 0) {
	      /* Errors will cause the plugin to get a SIGPIPE.  */
	      exit (1);
	    } else if (opid == 0) {
	      dup2 (pipefd[0], 0);
	      close (pipefd[0]);
	      close (pipefd[1]);
	      execl("/bin/sh", "/bin/sh", "-c", vars.output_to, NULL);
	      /* NOTREACHED */
	      exit (1);
	    } else {
	      /*
	       * If the print plugin gets SIGKILLed by gimp, we kill lpr
	       * in turn.  If the plugin signals us with SIGUSR1 that it's
	       * finished printing normally, we close our end of the pipe,
	       * and go away.
	       */
	      close (pipefd[0]);
	      while (usr1_interrupt == 0) {
	        if (kill (ppid, 0) < 0) {
		  /* The print plugin has been killed!  */
		  kill (opid, SIGTERM);
		  waitpid (opid, &dummy, 0);
		  close (pipefd[1]);
		  /*
		   * We do not want to allow cleanup before exiting.
		   * The exiting parent has already closed the connection
		   * to the X server; if we try to clean up, we'll notice
		   * that fact and complain.
		   */
		  _exit (0);
	        }
	        sleep (5);
	      }
	      /* We got SIGUSR1.  */
	      close (pipefd[1]);
	      /*
	       * We do not want to allow cleanup before exiting.
	       * The exiting parent has already closed the connection
	       * to the X server; if we try to clean up, we'll notice
	       * that fact and complain.
	       */
	      _exit (0);
	    }
	  } else {
	    close (pipefd[0]);
	    /* Parent process.  We generate the printer output. */
	    prn = fdopen (pipefd[1], "w");
	    /* and fall through... */
	  }
	}
      }
#else
      /* OS/2 PRINT command doesn't support print from stdin, use temp file */
      prn = (tmpfile = get_tmp_filename ()) ? fopen (tmpfile, "w") : NULL;
#endif
      else
	prn = fopen (vars.output_to, "wb");

      if (prn != NULL)
	{
	  Image image = Image_GDrawable_new(drawable);
	  vars.app_gamma = gimp_gamma();
	  merge_printvars(&vars, &(current_printer->printvars));

	  /*
	   * Is the image an Indexed type?  If so we need the colormap...
	   */

	  if (gimp_image_base_type (image_ID) == INDEXED)
	    vars.cmap = gimp_image_get_cmap (image_ID, &ncolors);
	  else
	    vars.cmap    = NULL;

	  /*
	   * Finally, call the print driver to send the image to the printer
	   * and close the output file/command...
	   */

	  if (verify_printer_params(current_printer, &vars))
	    (*current_printer->print) (current_printer, 1, prn, image, &vars);
	  else
	    values[0].data.d_status = STATUS_EXECUTION_ERROR;

	  if (plist_current > 0)
#ifndef __EMX__
	  {
	    fclose (prn);
	    kill (cpid, SIGUSR1);
	    waitpid (cpid, &dummy, 0);
	  }
#else
	  { /* PRINT temp file */
	    char *s;
	    fclose (prn);
	    s = g_strconcat (vars.output_to, tmpfile, NULL);
	    if (system(s) != 0)
	      values[0].data.d_status = STATUS_EXECUTION_ERROR;
	    g_free (s);
	    remove (tmpfile);
	    g_free( tmpfile);
	  }
#endif
	  else
	    fclose (prn);
	  print_finished = 1;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;

      /*
       * Store data...
       */

      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data (PLUG_IN_NAME, &vars, sizeof (vars));
    }

  /*
   * Detach from the drawable...
   */
  gimp_drawable_detach (drawable);


 cleanup:
#ifndef GIMP_1_0
  if (export == EXPORT_EXPORT)
    gimp_image_delete (image_ID);
#endif
}

/*
 * 'do_print_dialog()' - Pop up the print dialog...
 */

static gint
do_print_dialog (gchar *proc_name)
{
#ifdef GIMP_1_0
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("print");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm (gimp_use_xshm());
#endif

  /*
   * Get printrc options...
   */
  printrc_load ();

  /*
   * Print dialog window...
   */
  gimp_create_main_window ();

  gtk_main ();
  gdk_flush ();

  /*
   * Set printrc options...
   */
  if (saveme)
    printrc_save ();

  /*
   * Return ok/cancel...
   */
  return (runme);
}

static void
initialize_printer(plist_t *printer)
{
  printer->name[0] = '\0';
  printer->active=0;
  memcpy(&(printer->v), &vars, sizeof(vars));
}

#define GET_MANDATORY_STRING_PARAM(param)		\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  strncpy(key.param, lineptr, commaptr - line);		\
  key.param[commaptr - lineptr] = '\0';			\
  lineptr = commaptr + 1;				\
} while (0)

#define GET_MANDATORY_INT_PARAM(param)			\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  key.param = atoi(lineptr);				\
  lineptr = commaptr + 1;				\
} while (0)

#define GET_OPTIONAL_STRING_PARAM(param)			\
do {								\
  if ((commaptr = strchr(lineptr, ',')) == NULL)		\
    {								\
      strcpy(key.v.param, lineptr);				\
      keepgoing = 0;						\
      key.v.param[strlen(key.v.param) - 1] = '\0';		\
    }								\
  else								\
    {								\
      strncpy(key.v.param, lineptr, commaptr - lineptr);	\
      key.v.param[commaptr - lineptr] = '\0';			\
      lineptr = commaptr + 1;					\
    }								\
} while (0)

#define GET_OPTIONAL_INT_PARAM(param)					\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      key.v.param = atoi(lineptr);					\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

#define GET_OPTIONAL_FLOAT_PARAM(param)					\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      key.v.param = atof(lineptr);					\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

/*
 * 'printrc_load()' - Load the printer resource configuration file.
 */
void
printrc_load(void)
{
  int		i;		/* Looping var */
  FILE		*fp;		/* Printrc file */
  char		*filename;	/* Its name */
  char		line[1024],	/* Line in printrc file */
		*lineptr,	/* Pointer in line */
		*commaptr;	/* Pointer to next comma */
  plist_t	*p,		/* Current printer */
		key;		/* Search key */
#if (GIMP_MINOR_VERSION == 0)
  char		*home;		/* Home dir */
#endif

  check_plist(1);

 /*
  * Get the printer list...
  */

  get_system_printers();

 /*
  * Generate the filename for the current user...
  */

#if (GIMP_MINOR_VERSION == 0)
  home = getenv("HOME");
  if (home == NULL)
    filename=g_strdup("/.gimp/printrc");
  else
    filename = malloc(strlen(home) + 15);
    sprintf(filename, "%s/.gimp/printrc", home);
#else
  filename = gimp_personal_rc_file ("printrc");
#endif

#ifdef __EMX__
  _fnslashify(filename);
#endif

#ifndef __EMX__
  if ((fp = fopen(filename, "r")) != NULL)
#else
  if ((fp = fopen(filename, "rt")) != NULL)
#endif
  {
   /*
    * File exists - read the contents and update the printer list...
    */

    (void) memset(line, 0, 1024);
    while (fgets(line, sizeof(line), fp) != NULL)
    {
      int keepgoing = 1;
      if (line[0] == '#')
        continue;	/* Comment */
      initialize_printer(&key);
      lineptr = line;
     /*
      * Read the command-delimited printer definition data.  Note that
      * we can't use sscanf because %[^,] fails if the string is empty...
      */

      GET_MANDATORY_STRING_PARAM(name);
      GET_MANDATORY_STRING_PARAM(v.output_to);
      GET_MANDATORY_STRING_PARAM(v.driver);

      if (! get_printer_by_driver(key.v.driver))
	continue;

      GET_MANDATORY_STRING_PARAM(v.ppd_file);
      GET_MANDATORY_INT_PARAM(v.output_type);
      GET_MANDATORY_STRING_PARAM(v.resolution);
      GET_MANDATORY_STRING_PARAM(v.media_size);
      GET_MANDATORY_STRING_PARAM(v.media_type);

      GET_OPTIONAL_STRING_PARAM(media_source);
      GET_OPTIONAL_INT_PARAM(brightness);
      GET_OPTIONAL_FLOAT_PARAM(scaling);
      GET_OPTIONAL_INT_PARAM(orientation);
      GET_OPTIONAL_INT_PARAM(left);
      GET_OPTIONAL_INT_PARAM(top);
      GET_OPTIONAL_FLOAT_PARAM(gamma);
      GET_OPTIONAL_INT_PARAM(contrast);
      GET_OPTIONAL_INT_PARAM(red);
      GET_OPTIONAL_INT_PARAM(green);
      GET_OPTIONAL_INT_PARAM(blue);
      GET_OPTIONAL_INT_PARAM(linear);
      GET_OPTIONAL_INT_PARAM(image_type);
      GET_OPTIONAL_FLOAT_PARAM(saturation);
      GET_OPTIONAL_FLOAT_PARAM(density);
      GET_OPTIONAL_STRING_PARAM(ink_type);
      GET_OPTIONAL_STRING_PARAM(dither_algorithm);
      GET_OPTIONAL_INT_PARAM(unit);

/*
 * The format of the list is the File printer followed by a qsort'ed list
 * of system printers. So, if we want to update the file printer, it is
 * always first in the list, else call bsearch.
 */
      if ((strcmp(key.name, _("File")) == 0) && (strcmp(plist[0].name,
	   _("File")) == 0))
	{
#ifdef DEBUG
	  printf("Updated File printer directly\n");
#endif
	  p = &plist[0];
	  memcpy(p, &key, sizeof(plist_t));
	  p->active = 1;
	}
      else
	{
          if ((p = bsearch(&key, plist + 1, plist_count - 1, sizeof(plist_t),
                       (int (*)(const void *, const void *))compare_printers))
	      != NULL)
	    {
#ifdef DEBUG
	      printf("Updating printer %s.\n", key.name);
#endif
	      memcpy(p, &key, sizeof(plist_t));
	      p->active = 1;
	    }
          else
    	    {
#ifdef DEBUG
              fprintf(stderr, "Adding new printer from printrc file: %s\n",
                key.name);
#endif
	      check_plist(plist_count + 1);
	      p = plist + plist_count;
	      memcpy(p, &key, sizeof(plist_t));
	      p->active = 0;
	      plist_count++;
	    }
	}
    }

    fclose(fp);
  }

  g_free (filename);

 /*
  * Select the current printer as necessary...
  */

  if (vars.output_to[0] != '\0')
  {
    for (i = 0; i < plist_count; i ++)
      if (strcmp(vars.output_to, plist[i].v.output_to) == 0)
        break;

    if (i < plist_count)
      plist_current = i;
  }
}


/*
 * 'printrc_save()' - Save the current printer resource configuration.
 */
void
printrc_save(void)
{
  FILE		*fp;		/* Printrc file */
  char	       *filename;	/* Printrc filename */
  int		i;		/* Looping var */
  plist_t	*p;		/* Current printer */
#if (GIMP_MINOR_VERSION == 0)
  char		*home;		/* Home dir */
#endif


 /*
  * Generate the filename for the current user...
  */

#if (GIMP_MINOR_VERSION == 0)
  home = getenv("HOME");
  if (home == NULL)
    filename=g_strdup("/.gimp/printrc");
  else
    filename = malloc(strlen(home) + 15);
    sprintf(filename, "%s/.gimp/printrc", home);
#else
  filename = gimp_personal_rc_file ("printrc");
#endif

#ifdef __EMX__
  _fnslashify(filename);
#endif

#ifndef __EMX__
  if ((fp = fopen(filename, "w")) != NULL)
#else
  if ((fp = fopen(filename, "wt")) != NULL)
#endif
  {
   /*
    * Write the contents of the printer list...
    */

#ifdef DEBUG
    fprintf(stderr, "Number of printers: %d\n", plist_count);
#endif

    fputs("#PRINTRC " PLUG_IN_VERSION "\n", fp);

    for (i = 0, p = plist; i < plist_count; i ++, p ++)
      {
	fprintf(fp, "%s,%s,%s,%s,%d,%s,%s,%s,%s,",
		p->name, p->v.output_to, p->v.driver, p->v.ppd_file,
		p->v.output_type, p->v.resolution, p->v.media_size,
		p->v.media_type, p->v.media_source);
	fprintf(fp, "%d,%.3f,%d,%d,%d,%.3f,",
		p->v.brightness, p->v.scaling, p->v.orientation, p->v.left,
		p->v.top, p->v.gamma);
	fprintf(fp, "%d,%d,%d,%d,%d,%d,%.3f,%.3f,%s,%s,%d,\n",
		p->v.contrast, p->v.red, p->v.green, p->v.blue,
		p->v.linear, p->v.image_type, p->v.saturation, p->v.density,
		p->v.ink_type, p->v.dither_algorithm, p->v.unit);

#ifdef DEBUG
        fprintf(stderr, "Wrote printer %d: %s\n", i, p->name);
#endif

      }
    fclose(fp);
  } else {
    fprintf(stderr,"could not open printrc file \"%s\"\n",filename);
  }
  g_free (filename);
}


/*
 * 'compare_printers()' - Compare system printer names for qsort().
 */

static int
compare_printers(plist_t *p1,	/* I - First printer to compare */
                 plist_t *p2)	/* I - Second printer to compare */
{
  return (strcmp(p1->v.output_to, p2->v.output_to));
}


/*
 * 'get_system_printers()' - Get a complete list of printers from the spooler.
 */

static void
get_system_printers(void)
{
  int   i;
  char  defname[17];
#if defined(LPC_COMMAND) || defined(LPSTAT_COMMAND)
  FILE *pfile;
  char  line[129];
#endif
#if defined(LPSTAT_COMMAND)
  char  name[17];
#endif
#ifdef __EMX__
  BYTE  pnum;
#endif


  defname[0] = '\0';

  check_plist(1);
  plist_count = 1;
  initialize_printer(&plist[0]);
  strcpy(plist[0].name, _("File"));
  plist[0].v.output_to[0] = '\0';
  strcpy(plist[0].v.driver, "ps2");
  plist[0].v.output_type = OUTPUT_COLOR;

#ifdef LPC_COMMAND
  if ((pfile = popen(LPC_COMMAND " status < /dev/null", "r")) != NULL)
  {
    while (fgets(line, sizeof(line), pfile) != NULL) {
      if (!strncmp(line,"Press RETURN to continue",24)) {
	char *ptr= index(line,':')+2;
	if (ptr && strlen(ptr)<(ptr-line))
	  strcpy(line,ptr);
      }
      if (strchr(line, ':') != NULL && line[0] != ' ' && line[0] != '\t')
	{
	  check_plist(plist_count + 1);
	  *strchr(line, ':') = '\0';
	  initialize_printer(&plist[plist_count]);
	  strcpy(plist[plist_count].name, line);
	  sprintf(plist[plist_count].v.output_to,LPR_COMMAND" -P%s -l",line);
	  strcpy(plist[plist_count].v.driver, "ps2");
	  plist_count ++;
	}
    }
    pclose(pfile);
  }
#endif /* LPC_COMMAND */

#ifdef LPSTAT_COMMAND
  if ((pfile = popen(LPSTAT_COMMAND " -d -p", "r")) != NULL)
  {
    while (fgets(line, sizeof(line), pfile) != NULL)
    {
      if ((sscanf(line, "printer %s", name) == 1) ||
	  (sscanf(line, "Printer: %s", name) == 1))
      {
	check_plist(plist_count + 1);
	initialize_printer(&plist[plist_count]);
	strcpy(plist[plist_count].name, name);
	sprintf(plist[plist_count].v.output_to, LP_COMMAND " -s -d%s", name);
        strcpy(plist[plist_count].v.driver, "ps2");
        plist_count ++;
      }
      else
        sscanf(line, "system default destination: %s", defname);
    }

    pclose(pfile);
  }
#endif /* LPSTAT_COMMAND */

#ifdef __EMX__
  if (DosDevConfig(&pnum, DEVINFO_PRINTER) == NO_ERROR)
    {
      for (i = 1; i <= pnum; i++)
	{
	  check_plist(plist_count + 1);
	  initialize_printer(&plist[plist_count]);
	  sprintf(plist[plist_count].name, "LPT%d:", i);
	  sprintf(plist[plist_count].v.output_to, "PRINT /D:LPT%d /B ", i);
          strcpy(plist[plist_count].v.driver, "ps2");
          plist_count ++;
	}
    }
#endif

  if (plist_count > 2)
    qsort(plist + 1, plist_count - 1, sizeof(plist_t),
          (int (*)(const void *, const void *))compare_printers);

  if (defname[0] != '\0' && vars.output_to[0] == '\0')
  {
    for (i = 0; i < plist_count; i ++)
      if (strcmp(defname, plist[i].name) == 0)
        break;

    if (i < plist_count)
      plist_current = i;
  }
}

/*
 * End of "$Id$".
 */
