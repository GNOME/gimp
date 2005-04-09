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
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include <gimp-print/gimp-print.h>

#include "print_gimp.h"

#include "libgimp/stdplugins-intl.h"

/*
 * Local functions...
 */

static void	printrc_load (void);
void		printrc_save (void);
static int	compare_printers (gp_plist_t *p1, gp_plist_t *p2);
static void	get_system_printers (void);

static void	query           (void);
static void     run             (const gchar      *name,
				 gint              nparams,
				 const GimpParam  *param,
				 gint             *nreturn_vals,
				 GimpParam       **return_vals);
static int	do_print_dialog (const gchar      *proc_name);

/*
 * Globals...
 */

GimpPlugInInfo	PLUG_IN_INFO =		/* Plug-in information */
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

stp_vars_t vars = NULL;

int		plist_current = 0,	/* Current system printer */
		plist_count = 0;	/* Number of system printers */
gp_plist_t		*plist;			/* System printers */

int		saveme = FALSE;		/* True if print should proceed */
int		runme = FALSE;		/* True if print should proceed */
stp_printer_t current_printer = NULL;	/* Current printer index */
gint32          image_ID;	        /* image ID */

gchar *image_name;
gint   image_width;
gint   image_height;

static void
check_plist(int count)
{
  static int current_plist_size = 0;
  if (count <= current_plist_size)
    return;
  else if (current_plist_size == 0)
    {
      current_plist_size = count;
      plist = g_malloc (current_plist_size * sizeof(gp_plist_t));
    }
  else
    {
      current_plist_size *= 2;
      if (current_plist_size < count)
	current_plist_size = count;
      plist = g_realloc (plist, current_plist_size * sizeof(gp_plist_t));
    }
}

/*
 * 'main()' - Main entry - just call gimp_main()...
 */

MAIN()

static int print_finished = 0;

/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,	"run_mode",	"Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,	"image",	"Input image" },
    { GIMP_PDB_DRAWABLE,	"drawable",	"Input drawable" },
    { GIMP_PDB_STRING,	"output_to",	"Print command or filename (| to pipe to command)" },
    { GIMP_PDB_STRING,	"driver",	"Printer driver short name" },
    { GIMP_PDB_STRING,	"ppd_file",	"PPD file" },
    { GIMP_PDB_INT32,	"output_type",	"Output type (0 = gray, 1 = color)" },
    { GIMP_PDB_STRING,	"resolution",	"Resolution (\"300\", \"720\", etc.)" },
    { GIMP_PDB_STRING,	"media_size",	"Media size (\"Letter\", \"A4\", etc.)" },
    { GIMP_PDB_STRING,	"media_type",	"Media type (\"Plain\", \"Glossy\", etc.)" },
    { GIMP_PDB_STRING,	"media_source",	"Media source (\"Tray1\", \"Manual\", etc.)" },
    { GIMP_PDB_FLOAT,	"brightness",	"Brightness (0-400%)" },
    { GIMP_PDB_FLOAT,	"scaling",	"Output scaling (0-100%, -PPI)" },
    { GIMP_PDB_INT32,	"orientation",	"Output orientation (-1 = auto, 0 = portrait, 1 = landscape)" },
    { GIMP_PDB_INT32,	"left",		"Left offset (points, -1 = centered)" },
    { GIMP_PDB_INT32,	"top",		"Top offset (points, -1 = centered)" },
    { GIMP_PDB_FLOAT,	"gamma",	"Output gamma (0.1 - 3.0)" },
    { GIMP_PDB_FLOAT,	"contrast",	"Contrast" },
    { GIMP_PDB_FLOAT,	"cyan",		"Cyan level" },
    { GIMP_PDB_FLOAT,	"magenta",	"Magenta level" },
    { GIMP_PDB_FLOAT,	"yellow",		"Yellow level" },
    { GIMP_PDB_INT32,	"linear",	"Linear output (0 = normal, 1 = linear)" },
    { GIMP_PDB_INT32,	"image_type",	"Image type (0 = line art, 1 = solid tones, 2 = continuous tone, 3 = monochrome)"},
    { GIMP_PDB_FLOAT,	"saturation",	"Saturation (0-1000%)" },
    { GIMP_PDB_FLOAT,	"density",	"Density (0-200%)" },
    { GIMP_PDB_STRING,	"ink_type",	"Type of ink or cartridge" },
    { GIMP_PDB_STRING,	"dither_algorithm", "Dither algorithm" },
    { GIMP_PDB_INT32,	"unit",		"Unit 0=Inches 1=Metric" },
  };

  static gchar *blurb = "This plug-in prints images from The GIMP.";
  static gchar *help  = "Prints images to PostScript, PCL, or ESC/P2 printers.";
  static gchar *auth  = "Michael Sweet <mike@easysw.com> and Robert Krawitz <rlk@alum.mit.edu>";
  static gchar *copy  = "Copyright 1997-2000 by Michael Sweet and Robert Krawitz";
  static gchar *types = "RGB*,GRAY*,INDEXED*";

  gimp_install_procedure ("file_print_gimp",
			  blurb, help, auth, copy,
			  PLUG_IN_VERSION,
			  N_("_Print..."),
			  types,
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);

  gimp_plugin_menu_register ("file_print_gimp", "<Image>/File/Send");
  gimp_plugin_icon_register ("file_print_gimp",
                             GIMP_ICON_TYPE_STOCK_ID, GTK_STOCK_PRINT);
}


/*
 * 'usr1_handler()' - Make a note when we receive SIGUSR1.
 */

static volatile int usr1_interrupt;

static void
usr1_handler (int signal)
{
  usr1_interrupt = 1;
}

void
gimp_writefunc (void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *) file;
  fwrite (buf, 1, bytes, prn);
}

/*
 * 'run()' - Run the plug-in...
 */

/* #define DEBUG_STARTUP */

#ifdef DEBUG_STARTUP
volatile int SDEBUG = 1;
#endif

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpDrawable	*drawable;	/* Drawable for image */
  GimpRunMode    run_mode;	/* Current run mode */
  FILE		*prn = NULL;	/* Print file/command */
  gint		 ncolors;	/* Number of colors in colormap */
  GimpParam	 values[1];	/* Return values */
  gint32         drawable_ID;   /* drawable ID */
  GimpExportReturn export = GIMP_EXPORT_CANCEL;    /* return value of gimp_export_image() */
  gint		ppid = getpid (); /* PID of plugin */
  gint		opid; 	        /* PID of output process */
  gint          cpid = 0;	/* PID of control/monitor process */
  gint          pipefd[2];	/* Fds of the pipe connecting all the above */
  gint		dummy;

#ifdef DEBUG_STARTUP
  while (SDEBUG)
    ;
#endif

 /*
  * Initialise libgimpprint
  */

  stp_init ();

  bind_textdomain_codeset ("gimp-print", "UTF-8");

  INIT_I18N();

  vars = stp_allocate_copy (stp_default_settings ());
  stp_set_input_color_model (vars, COLOR_MODEL_RGB);
  stp_set_output_color_model (vars, COLOR_MODEL_RGB);
  /*
   * Initialize parameter data...
   */

  current_printer = stp_get_printer_by_index (0);
  run_mode = (GimpRunMode) param[0].data.d_int32;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  image_ID = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  image_name = gimp_image_get_name (image_ID);

  /*  eventually export the image */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init ("print", TRUE);
      export = gimp_export_image (&image_ID, &drawable_ID, "Print",
				  (GIMP_EXPORT_CAN_HANDLE_RGB |
				   GIMP_EXPORT_CAN_HANDLE_GRAY |
				   GIMP_EXPORT_CAN_HANDLE_INDEXED |
				   GIMP_EXPORT_CAN_HANDLE_ALPHA));
      if (export == GIMP_EXPORT_CANCEL)
	{
	  *nreturn_vals = 1;
	  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	  return;
	}
      break;
    default:
      break;
    }

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
    case GIMP_RUN_INTERACTIVE:
      /*
       * Get information from the dialog...
       */

      if (!do_print_dialog (name))
	goto cleanup;
      stp_copy_vars (vars, plist[plist_current].v);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*
       * Make sure all the arguments are present...
       */
      if (nparams < 11)
	values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      else
	{
	  stp_set_output_to    (vars, param[3].data.d_string);
	  stp_set_driver       (vars, param[4].data.d_string);
	  stp_set_ppd_file     (vars, param[5].data.d_string);
	  stp_set_output_type  (vars, param[6].data.d_int32);
	  stp_set_resolution   (vars, param[7].data.d_string);
	  stp_set_media_size   (vars, param[8].data.d_string);
	  stp_set_media_type   (vars, param[9].data.d_string);
	  stp_set_media_source (vars, param[10].data.d_string);

          if (nparams > 11)
	    stp_set_brightness (vars, param[11].data.d_float);

          if (nparams > 12)
            stp_set_scaling (vars, param[12].data.d_float);

          if (nparams > 13)
            stp_set_orientation (vars, param[13].data.d_int32);

          if (nparams > 14)
            stp_set_left (vars, param[14].data.d_int32);

          if (nparams > 15)
            stp_set_top (vars, param[15].data.d_int32);

          if (nparams > 16)
            stp_set_gamma (vars, param[16].data.d_float);

          if (nparams > 17)
	    stp_set_contrast (vars, param[17].data.d_float);

          if (nparams > 18)
	    stp_set_cyan (vars, param[18].data.d_float);

          if (nparams > 19)
	    stp_set_magenta (vars, param[19].data.d_float);

          if (nparams > 20)
	    stp_set_yellow (vars, param[20].data.d_float);

          if (nparams > 21)
            stp_set_image_type (vars, param[22].data.d_int32);

          if (nparams > 22)
            stp_set_saturation (vars, param[23].data.d_float);

          if (nparams > 23)
            stp_set_density (vars, param[24].data.d_float);

	  if (nparams > 24)
	    stp_set_ink_type (vars, param[25].data.d_string);

	  if (nparams > 25)
	    stp_set_dither_algorithm (vars, param[26].data.d_string);

          if (nparams > 26)
            stp_set_unit (vars, param[27].data.d_int32);
	}

      current_printer = stp_get_printer_by_driver (stp_get_driver(vars));
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      break;

    default:
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      break;
    }

  /*
   * Print the image...
   */
  if (values[0].data.d_status == GIMP_PDB_SUCCESS)
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
	      execl ("/bin/sh", "/bin/sh", "-c",
                     g_shell_quote (stp_get_output_to (vars)), NULL);
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
      else
	prn = g_fopen (stp_get_output_to(vars), "wb");

      if (prn != NULL)
	{
	  stp_image_t *image = Image_GimpDrawable_new (drawable);
	  stp_set_app_gamma (vars, gimp_gamma ());
	  stp_merge_printvars (vars,
                               stp_printer_get_printvars (current_printer));

	  /*
	   * Is the image an Indexed type?  If so we need the colormap...
	   */

	  if (gimp_image_base_type (image_ID) == GIMP_INDEXED)
	    stp_set_cmap (vars, gimp_image_get_colormap (image_ID, &ncolors));
	  else
	    stp_set_cmap (vars, NULL);

	  /*
	   * Finally, call the print driver to send the image to the printer
	   * and close the output file/command...
	   */

	  stp_set_outfunc (vars, gimp_writefunc);
	  stp_set_errfunc (vars, gimp_writefunc);
	  stp_set_outdata (vars, prn);
	  stp_set_errdata (vars, stderr);
	  if (stp_printer_get_printfuncs (current_printer)->verify
	      (current_printer, vars))
	    stp_printer_get_printfuncs (current_printer)->print
	      (current_printer, image, vars);
	  else
	    values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

	  if (plist_current > 0)
	  {
	    fclose (prn);
	    kill (cpid, SIGUSR1);
	    waitpid (cpid, &dummy, 0);
	  }
	  else
	    fclose (prn);
	  print_finished = 1;
	}
      else
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

      /*
       * Store data...
       */

      if (run_mode == GIMP_RUN_INTERACTIVE)
	gimp_set_data (PLUG_IN_NAME, vars, sizeof (vars));
    }

  /*
   * Detach from the drawable...
   */
  gimp_drawable_detach (drawable);

 cleanup:

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image_ID);

  stp_free_vars (vars);
}

/*
 * 'do_print_dialog()' - Pop up the print dialog...
 */

static gint
do_print_dialog (const gchar *proc_name)
{

  /*
   * Get printrc options...
   */
  printrc_load ();

  /*
   * Print dialog window...
   */
  gimp_create_main_window ();

  gtk_main ();

  /*
   * Set printrc options...
   */
  if (saveme)
    printrc_save ();

  /*
   * Return ok/cancel...
   */
  return runme;
}

void
initialize_printer (gp_plist_t *printer)
{
  printer->name[0] = '\0';
  printer->active  = 0;
  printer->v       = stp_allocate_vars ();
}

#define GET_MANDATORY_INTERNAL_STRING_PARAM(param)	\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  strncpy(key.param, lineptr, commaptr - line);		\
  key.param[commaptr - lineptr] = '\0';			\
  lineptr = commaptr + 1;				\
} while (0)

#define GET_MANDATORY_STRING_PARAM(param)		\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  stp_set_##param##_n(key.v, lineptr, commaptr - line);	\
  lineptr = commaptr + 1;				\
} while (0)

#define GET_MANDATORY_INT_PARAM(param)			\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  stp_set_##param(key.v, atoi(lineptr));		\
  lineptr = commaptr + 1;				\
} while (0)

#define GET_OPTIONAL_STRING_PARAM(param)			\
do {								\
  if ((commaptr = strchr(lineptr, ',')) == NULL)		\
    {								\
      stp_set_##param(key.v, lineptr);				\
      keepgoing = 0;						\
    }								\
  else								\
    {								\
      stp_set_##param##_n(key.v, lineptr, commaptr - lineptr);	\
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
      stp_set_##param(key.v, atoi(lineptr));				\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

#define IGNORE_OPTIONAL_PARAM(param)					\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
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
      const stp_vars_t maxvars = stp_maximum_settings();		\
      const stp_vars_t minvars = stp_minimum_settings();		\
      const stp_vars_t defvars = stp_default_settings();		\
      stp_set_##param(key.v, atof(lineptr));				\
      if (stp_get_##param(key.v) > 0 &&					\
	  (stp_get_##param(key.v) > stp_get_##param(maxvars) ||		\
	   stp_get_##param(key.v) < stp_get_##param(minvars)))		\
	stp_set_##param(key.v, stp_get_##param(defvars));		\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

static void *
psearch(const void *key, const void *base, size_t nmemb, size_t size,
	int (*compar)(const void *, const void *))
{
  int i;
  const char *cbase = (const char *) base;
  for (i = 0; i < nmemb; i++)
    {
      if ((*compar)(key, (const void *) cbase) == 0)
	return (void *) cbase;
      cbase += size;
    }
  return NULL;
}

int
add_printer(const gp_plist_t *key, int add_only)
{
  /*
   * The format of the list is the File printer followed by a qsort'ed list
   * of system printers. So, if we want to update the file printer, it is
   * always first in the list, else call psearch.
   */
  gp_plist_t *p;
  if (strcmp(_("File"), key->name) == 0
      && strcmp(plist[0].name, _("File")) == 0)
    {
      if (add_only)
	return 0;
      if (stp_get_printer_by_driver(stp_get_driver(key->v)))
	{
#ifdef DEBUG
	  printf("Updated File printer directly\n");
#endif
	  p = &plist[0];
	  memcpy(p, key, sizeof(gp_plist_t));
	  p->v = stp_allocate_copy(key->v);
	  p->active = 1;
	}
      return 1;
    }
  else if (stp_get_printer_by_driver(stp_get_driver(key->v)))
    {
      p = psearch(key, plist + 1, plist_count - 1,
		  sizeof(gp_plist_t),
		  (int (*)(const void *, const void *)) compare_printers);
      if (p == NULL)
	{
#ifdef DEBUG
	  fprintf(stderr, "Adding new printer from printrc file: %s\n",
		  key->name);
#endif
	  check_plist(plist_count + 1);
	  p = plist + plist_count;
	  plist_count++;
	  memcpy(p, key, sizeof(gp_plist_t));
	  p->v = stp_allocate_copy(key->v);
	  p->active = 0;
	}
      else
	{
	  if (add_only)
	    return 0;
#ifdef DEBUG
	  printf("Updating printer %s.\n", key->name);
#endif
	  memcpy(p, key, sizeof(gp_plist_t));
	  stp_copy_vars(p->v, key->v);
	  p->active = 1;
	}
    }
  return 1;
}

/*
 * 'printrc_load()' - Load the printer resource configuration file.
 */
void
printrc_load (void)
{
  gint		i;		/* Looping var */
  FILE		*fp;		/* Printrc file */
  gchar		*filename;	/* Its name */
  gchar		line[1024],	/* Line in printrc file */
		*lineptr,	/* Pointer in line */
		*commaptr;	/* Pointer to next comma */
  gp_plist_t	key;		/* Search key */
  gint		format = 0;	/* rc file format version */
  gint		system_printers; /* printer count before reading printrc */
  gchar *	current_printer = NULL; /* printer to select */

  check_plist(1);

 /*
  * Get the printer list...
  */

  get_system_printers();

  system_printers = plist_count - 1;

 /*
  * Generate the filename for the current user...
  */

  filename = gimp_personal_rc_file ("printrc");

  if ((fp = g_fopen(filename, "r")) != NULL)
  {
   /*
    * File exists - read the contents and update the printer list...
    */

    (void) memset(&key, 0, sizeof(gp_plist_t));
    initialize_printer(&key);
    strcpy(key.name, _("File"));
    (void) memset(line, 0, 1024);
    while (fgets(line, sizeof(line), fp) != NULL)
    {
      int keepgoing = 1;
      if (line[0] == '#')
      {
	if (strncmp("#PRINTRCv", line, 9) == 0)
	{
#ifdef DEBUG
          printf("Found printrc version tag: `%s'\n", line);
          printf("Version number: `%s'\n", &(line[9]));
#endif
	  (void) sscanf(&(line[9]), "%d", &format);
	}
        continue;	/* Comment */
      }
      if (format == 0)
      {
       /*
	* Read old format printrc lines...
	*/

        initialize_printer(&key);
        lineptr = line;

       /*
        * Read the command-delimited printer definition data.  Note that
        * we can't use sscanf because %[^,] fails if the string is empty...
        */

        GET_MANDATORY_INTERNAL_STRING_PARAM(name);
        GET_MANDATORY_STRING_PARAM(output_to);
        GET_MANDATORY_STRING_PARAM(driver);

        if (! stp_get_printer_by_driver(stp_get_driver(key.v)))
	  continue;

        GET_MANDATORY_STRING_PARAM(ppd_file);
        GET_MANDATORY_INT_PARAM(output_type);
        GET_MANDATORY_STRING_PARAM(resolution);
        GET_MANDATORY_STRING_PARAM(media_size);
        GET_MANDATORY_STRING_PARAM(media_type);

        GET_OPTIONAL_STRING_PARAM(media_source);
        GET_OPTIONAL_FLOAT_PARAM(brightness);
        GET_OPTIONAL_FLOAT_PARAM(scaling);
        GET_OPTIONAL_INT_PARAM(orientation);
        GET_OPTIONAL_INT_PARAM(left);
        GET_OPTIONAL_INT_PARAM(top);
        GET_OPTIONAL_FLOAT_PARAM(gamma);
        GET_OPTIONAL_FLOAT_PARAM(contrast);
        GET_OPTIONAL_FLOAT_PARAM(cyan);
        GET_OPTIONAL_FLOAT_PARAM(magenta);
        GET_OPTIONAL_FLOAT_PARAM(yellow);
        IGNORE_OPTIONAL_PARAM(linear);
        GET_OPTIONAL_INT_PARAM(image_type);
        GET_OPTIONAL_FLOAT_PARAM(saturation);
        GET_OPTIONAL_FLOAT_PARAM(density);
        GET_OPTIONAL_STRING_PARAM(ink_type);
        GET_OPTIONAL_STRING_PARAM(dither_algorithm);
        GET_OPTIONAL_INT_PARAM(unit);
	add_printer(&key, 0);
      }
      else if (format == 1)
      {
       /*
	* Read new format printrc lines...
	*/

	gchar *keyword, *end, *value;

	keyword = line;
	for (keyword = line; g_ascii_isspace (*keyword); keyword++)
	{
	  /* skip initial spaces... */
	}
	if (!g_ascii_isalpha (*keyword))
	  continue;
	for (end = keyword; g_ascii_isalnum (*end) || *end == '-'; end++)
	{
	  /* find end of keyword... */
	}
	value = end;
	while (g_ascii_isspace (*value)) {
	  /* skip over white space... */
	  value++;
	}
	if (*value != ':')
	  continue;
	value++;
	*end = '\0';
	while (g_ascii_isspace (*value)) {
	  /* skip over white space... */
	  value++;
	}
	for (end = value; *end && *end != '\n'; end++)
	{
	  /* find end of line... */
	}
	*end = '\0';
#ifdef DEBUG
        printf("Keyword = `%s', value = `%s'\n", keyword, value);
#endif
	if (strcasecmp("current-printer", keyword) == 0) {
	  if (current_printer)
	    g_free (current_printer);
	  current_printer = g_strdup(value);
	} else if (strcasecmp("printer", keyword) == 0) {
	  /* Switch to printer named VALUE */
	  add_printer(&key, 0);
#ifdef DEBUG
	  printf("output_to is now %s\n", stp_get_output_to(p->v));
#endif

	  initialize_printer(&key);
	  strncpy(key.name, value, 127);
	} else if (strcasecmp("destination", keyword) == 0) {
	  stp_set_output_to(key.v, value);
	} else if (strcasecmp("driver", keyword) == 0) {
	  stp_set_driver(key.v, value);
	} else if (strcasecmp("ppd-file", keyword) == 0) {
	  stp_set_ppd_file(key.v, value);
	} else if (strcasecmp("output-type", keyword) == 0) {
	  stp_set_output_type(key.v, atoi(value));
	} else if (strcasecmp("resolution", keyword) == 0) {
	  stp_set_resolution(key.v, value);
	} else if (strcasecmp("media-size", keyword) == 0) {
	  stp_set_media_size(key.v, value);
	} else if (strcasecmp("media-type", keyword) == 0) {
	  stp_set_media_type(key.v, value);
	} else if (strcasecmp("media-source", keyword) == 0) {
	  stp_set_media_source(key.v, value);
	} else if (strcasecmp("brightness", keyword) == 0) {
	  stp_set_brightness(key.v, atof(value));
	} else if (strcasecmp("scaling", keyword) == 0) {
	  stp_set_scaling(key.v, atof(value));
	} else if (strcasecmp("orientation", keyword) == 0) {
	  stp_set_orientation(key.v, atoi(value));
	} else if (strcasecmp("left", keyword) == 0) {
	  stp_set_left(key.v, atoi(value));
	} else if (strcasecmp("top", keyword) == 0) {
	  stp_set_top(key.v, atoi(value));
	} else if (strcasecmp("gamma", keyword) == 0) {
	  stp_set_gamma(key.v, atof(value));
	} else if (strcasecmp("contrast", keyword) == 0) {
	  stp_set_contrast(key.v, atof(value));
	} else if (strcasecmp("cyan", keyword) == 0) {
	  stp_set_cyan(key.v, atof(value));
	} else if (strcasecmp("magenta", keyword) == 0) {
	  stp_set_magenta(key.v, atof(value));
	} else if (strcasecmp("yellow", keyword) == 0) {
	  stp_set_yellow(key.v, atof(value));
	} else if (strcasecmp("linear", keyword) == 0) {
	  /* Ignore linear */
	} else if (strcasecmp("image-type", keyword) == 0) {
	  stp_set_image_type(key.v, atoi(value));
	} else if (strcasecmp("saturation", keyword) == 0) {
	  stp_set_saturation(key.v, atof(value));
	} else if (strcasecmp("density", keyword) == 0) {
	  stp_set_density(key.v, atof(value));
	} else if (strcasecmp("ink-type", keyword) == 0) {
	  stp_set_ink_type(key.v, value);
	} else if (strcasecmp("dither-algorithm", keyword) == 0) {
	  stp_set_dither_algorithm(key.v, value);
	} else if (strcasecmp("unit", keyword) == 0) {
	  stp_set_unit(key.v, atoi(value));
	} else if (strcasecmp("custom-page-width", keyword) == 0) {
	  stp_set_page_width(key.v, atoi(value));
	} else if (strcasecmp("custom-page-height", keyword) == 0) {
	  stp_set_page_height(key.v, atoi(value));
	} else {
	  /* Unrecognised keyword; ignore it... */
          printf("Unrecognized keyword `%s' in printrc; value `%s'\n", keyword, value);
	}
      }
      else
      {
       /*
        * We cannot read this file format...
        */
      }
    }
    if (format > 0)
      add_printer(&key, 0);
    fclose(fp);
  }

  g_free (filename);

 /*
  * Select the current printer as necessary...
  */

  if (format == 1)
  {
    if (current_printer)
    {
      for (i = 0; i < plist_count; i ++)
        if (strcmp(current_printer, plist[i].name) == 0)
	  plist_current = i;
    }
  }
  else
  {
    if (stp_get_output_to(vars)[0] != '\0')
    {
      for (i = 0; i < plist_count; i ++)
        if (strcmp(stp_get_output_to(vars), stp_get_output_to(plist[i].v))== 0)
          break;

      if (i < plist_count)
        plist_current = i;
    }
  }
}

/*
 * 'printrc_save()' - Save the current printer resource configuration.
 */
void
printrc_save (void)
{
  FILE		*fp;		/* Printrc file */
  gchar	       *filename;	/* Printrc filename */
  gint		i;		/* Looping var */
  gp_plist_t	*p;		/* Current printer */

 /*
  * Generate the filename for the current user...
  */

  filename = gimp_personal_rc_file ("printrc");

  if ((fp = g_fopen(filename, "w")) != NULL)
  {
   /*
    * Write the contents of the printer list...
    */

#ifdef DEBUG
    fprintf(stderr, "Number of printers: %d\n", plist_count);
#endif

    fputs("#PRINTRCv1 written by GIMP-PRINT " PLUG_IN_VERSION "\n", fp);

    fprintf(fp, "Current-Printer: %s\n", plist[plist_current].name);

    for (i = 0, p = plist; i < plist_count; i ++, p ++)
      {
	fprintf(fp, "\nPrinter: %s\n", p->name);
	fprintf(fp, "Destination: %s\n", stp_get_output_to(p->v));
	fprintf(fp, "Driver: %s\n", stp_get_driver(p->v));
	fprintf(fp, "PPD-File: %s\n", stp_get_ppd_file(p->v));
	fprintf(fp, "Output-Type: %d\n", stp_get_output_type(p->v));
	fprintf(fp, "Resolution: %s\n", stp_get_resolution(p->v));
	fprintf(fp, "Media-Size: %s\n", stp_get_media_size(p->v));
	fprintf(fp, "Media-Type: %s\n", stp_get_media_type(p->v));
	fprintf(fp, "Media-Source: %s\n", stp_get_media_source(p->v));
	fprintf(fp, "Brightness: %.3f\n", stp_get_brightness(p->v));
	fprintf(fp, "Scaling: %.3f\n", stp_get_scaling(p->v));
	fprintf(fp, "Orientation: %d\n", stp_get_orientation(p->v));
	fprintf(fp, "Left: %d\n", stp_get_left(p->v));
	fprintf(fp, "Top: %d\n", stp_get_top(p->v));
	fprintf(fp, "Gamma: %.3f\n", stp_get_gamma(p->v));
	fprintf(fp, "Contrast: %.3f\n", stp_get_contrast(p->v));
	fprintf(fp, "Cyan: %.3f\n", stp_get_cyan(p->v));
	fprintf(fp, "Magenta: %.3f\n", stp_get_magenta(p->v));
	fprintf(fp, "Yellow: %.3f\n", stp_get_yellow(p->v));
	fprintf(fp, "Image-Type: %d\n", stp_get_image_type(p->v));
	fprintf(fp, "Saturation: %.3f\n", stp_get_saturation(p->v));
	fprintf(fp, "Density: %.3f\n", stp_get_density(p->v));
	fprintf(fp, "Ink-Type: %s\n", stp_get_ink_type(p->v));
	fprintf(fp, "Dither-Algorithm: %s\n", stp_get_dither_algorithm(p->v));
	fprintf(fp, "Unit: %d\n", stp_get_unit(p->v));
	fprintf(fp, "Custom-Page-Width: %d\n", stp_get_page_width(p->v));
	fprintf(fp, "Custom-Page-Height: %d\n", stp_get_page_height(p->v));

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
compare_printers (gp_plist_t *p1,	/* I - First printer to compare */
                  gp_plist_t *p2)	/* I - Second printer to compare */
{
  return (strcmp(p1->name, p2->name));
}

/*
 * 'get_system_printers()' - Get a complete list of printers from the spooler.
 */

#define PRINTERS_NONE	0
#define PRINTERS_LPC	1
#define PRINTERS_LPSTAT	2

static void
get_system_printers (void)
{
  int   i;			/* Looping var */
  int	type;			/* 0 = none, 1 = lpc, 2 = lpstat */
  char	command[255];		/* Command to run */
  char  defname[128];		/* Default printer name */
  FILE *pfile;			/* Pipe to status command */
  char  line[255];		/* Line from status command */
  char	*ptr;			/* Pointer into line */
  char  name[128];		/* Printer name from status command */

  static const char *lpcs[] =   /* Possible locations of LPC... */
    {
      "/etc"
      "/usr/bsd",
      "/usr/etc",
      "/usr/libexec",
      "/usr/sbin"
    };

 /*
  * Setup defaults...
  */

  defname[0] = '\0';

  check_plist(1);
  plist_count = 1;
  initialize_printer(&plist[0]);
  strcpy(plist[0].name, _("File"));
  stp_set_driver(plist[0].v, "ps2");
  stp_set_output_type(plist[0].v, OUTPUT_COLOR);

 /*
  * Figure out what command to run...  We use lpstat if it is available over
  * lpc since Solaris, CUPS, etc. provide both commands.  No need to list
  * each printer twice...
  */

  if (!access("/usr/bin/lpstat", X_OK))
  {
    strcpy(command, "/usr/bin/lpstat -d -p");
    type = PRINTERS_LPSTAT;
  }
  else
  {
    for (i = 0; i < (sizeof(lpcs) / sizeof(lpcs[0])); i ++)
    {
      sprintf(command, "%s/lpc", lpcs[i]);

      if (!access(command, X_OK))
        break;
    }

    if (i < (sizeof(lpcs) / sizeof(lpcs[0])))
    {
      strcat(command, " status < /dev/null");
      type = PRINTERS_LPC;
    }
    else
      type = PRINTERS_NONE;
  }

 /*
  * Run the command, if any, to get the available printers...
  */

  if (type > PRINTERS_NONE)
  {
    if ((pfile = popen(command, "r")) != NULL)
    {
     /*
      * Read input as needed...
      */

      while (fgets(line, sizeof(line), pfile) != NULL)
        switch (type)
	{
	  char *result;
	  case PRINTERS_LPC :
	      if (!strncmp(line, "Press RETURN to continue", 24) &&
		  (ptr = strchr(line, ':')) != NULL &&
		  (strlen(ptr) - 2) < (ptr - line))
		strcpy(line, ptr + 2);

	      if ((ptr = strchr(line, ':')) != NULL &&
	          line[0] != ' ' && line[0] != '\t')
              {
		check_plist(plist_count + 1);
		*ptr = '\0';
		initialize_printer(&plist[plist_count]);
		strncpy(plist[plist_count].name, line,
			sizeof(plist[plist_count].name) - 1);
#ifdef DEBUG
                fprintf(stderr, "Adding new printer from lpc: <%s>\n",
                  line);
#endif
		result = g_strdup_printf("lpr -P%s -l", line);
		stp_set_output_to(plist[plist_count].v, result);
		g_free(result);
		stp_set_driver(plist[plist_count].v, "ps2");
		plist_count ++;
	      }
	      break;

	  case PRINTERS_LPSTAT :
	      if ((sscanf(line, "printer %127s", name) == 1) ||
		  (sscanf(line, "Printer: %127s", name) == 1))
	      {
		check_plist(plist_count + 1);
		initialize_printer(&plist[plist_count]);
		strncpy(plist[plist_count].name, name,
			sizeof(plist[plist_count].name) - 1);
#ifdef DEBUG
                fprintf(stderr, "Adding new printer from lpc: <%s>\n",
                  name);
#endif
		result = g_strdup_printf("lp -s -d%s -oraw", name);
		stp_set_output_to(plist[plist_count].v, result);
		g_free(result);
		stp_set_driver(plist[plist_count].v, "ps2");
        	plist_count ++;
	      }
	      else
        	sscanf(line, "system default destination: %127s", defname);
	      break;
	}

      pclose(pfile);
    }
  }

  if (plist_count > 2)
    qsort(plist + 1, plist_count - 1, sizeof(gp_plist_t),
          (int (*)(const void *, const void *))compare_printers);

  if (defname[0] != '\0' && stp_get_output_to(vars)[0] == '\0')
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
