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
 *   print_driver_callback()  - Update the current printer driver...
 *   media_size_callback()    - Update the current media size...
 *   print_command_callback() - Update the print command...
 *   output_type_callback()   - Update the current output type...
 *   print_callback()         - Start the print...
 *
 * Revision History:
 *
 *   See ChangeLog
 */
#include "config.h"

#include "print_gimp.h"

#include <signal.h>
#ifdef __EMX__
#define INCL_DOSDEVICES
#define INCL_DOSERRORS
#include <os2.h>
#endif

#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/*
 * Types...
 */

/* Concrete type to represent image */
typedef struct
{
  GDrawable *drawable;
  GPixelRgn rgn;
} Gimp_Image_t;


/*
 * Local functions...
 */

static void	printrc_load         (void);
void            printrc_save         (void);
static int	compare_printers     (plist_t *p1, plist_t *p2);
static void	get_system_printers  (void);

static void	query                 (void);
static void	run                   (char *, int, GParam *, int *, GParam **);
static int	do_print_dialog       (void);

extern void     gimp_create_main_window (void);

/*
 * Externals
 */
extern GtkObject *brightness_adjustment; /* Adjustment object for brightness */
extern GtkObject *saturation_adjustment; /* Adjustment object for saturation */
extern GtkObject *density_adjustment;    /* Adjustment object for density */
extern GtkObject *contrast_adjustment;   /* Adjustment object for contrast */
extern GtkObject *red_adjustment;        /* Adjustment object for red */
extern GtkObject *green_adjustment;      /* Adjustment object for green */
extern GtkObject *blue_adjustment;       /* Adjustment object for blue */
extern GtkObject *gamma_adjustment;      /* Adjustment object for gamma */

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
  "ps2",		/* Name of printer "driver" */
  "",			/* Name of PPD file */
  OUTPUT_COLOR,		/* Color or grayscale output */
  "",			/* Output resolution */
  "",			/* Size of output media */
  "",			/* Type of output media */
  "",			/* Source of output media */
  100,			/* Output brightness */
  100.0,		/* Scaling (100% means entire printable area, */
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
  1.0			/* Density */
};

gint		 plist_current = 0;	/* Current system printer */
gint		 plist_count = 0;	/* Number of system printers */
plist_t		 plist[MAX_PLIST];	/* System printers */

gint		 saveme = FALSE;         /* True if print should proceed */
gint		 runme = FALSE;		/* True if print should proceed */
const printer_t *current_printer = 0;	/* Current printer index */
gint32           image_ID;	        /* image ID */

gint             image_width;
gint             image_height;

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


/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query(void)
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
    { PARAM_FLOAT,	"saturation",	"Saturation (0-1000%)" },
    { PARAM_FLOAT,	"density",	"Density (0-200%)" },
  };
  static int		nargs = sizeof(args) / sizeof(args[0]);

  gimp_install_procedure(
      "file_print",
      "This plug-in prints images from The GIMP.",
      "Prints images to PostScript, PCL, or ESC/P2 printers.",
      "Michael Sweet <mike@easysw.com> and Robert Krawitz <rlk@alum.mit.edu>",
      "Copyright 1997-2000 by Michael Sweet and Robert Krawitz",
      PLUG_IN_VERSION,
      N_("<Image>/File/Print..."),
      "RGB*,GRAY*,INDEXED*",
      PROC_PLUG_IN,
      nargs,
      0,
      args,
      NULL);
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
 * 'run()' - Run the plug-in...
 */

#define PRINT_LUT

static void
run(char   *name,		/* I - Name of print program. */
    int    nparams,		/* I - Number of parameters passed in */
    GParam *param,		/* I - Parameter values */
    int    *nreturn_vals,	/* O - Number of return values */
    GParam **return_vals)	/* O - Return values */
{
  GDrawable	*drawable;	/* Drawable for image */
  GRunModeType	run_mode;	/* Current run mode */
  FILE		*prn;		/* Print file/command */
  lut_t		lut;		/* 16-bit lookup table for brightness */
  guchar	*cmap;		/* Colormap (indexed images only) */
  int		ncolors;	/* Number of colors in colormap */
  GParam	*values;	/* Return values */
#ifdef __EMX__
  char		*tmpfile;	/* temp filename */
#endif
  gint32         drawable_ID;   /* drawable ID */
  GimpExportReturnType export = EXPORT_CANCEL;    /* return value of gimp_export_image() */

  INIT_I18N_UI();

 /*
  * Initialize parameter data...
  */

  current_printer = get_printer_by_index(0);
  run_mode = param[0].data.d_int32;

  values = g_new(GParam, 1);

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  image_ID = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  /*  eventually export the image */ 
  switch (run_mode)
    {
    case RUN_INTERACTIVE:
    case RUN_WITH_LAST_VALS:
      gimp_ui_init ("print", TRUE);
      export = gimp_export_image (&image_ID, &drawable_ID, "Print", 
				  (CAN_HANDLE_RGB | CAN_HANDLE_GRAY | CAN_HANDLE_INDEXED |
				   CAN_HANDLE_ALPHA));
      if (export == EXPORT_CANCEL)
	{
	  *nreturn_vals = 1;
	  values[0].data.d_status = STATUS_CANCEL;
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
    case RUN_INTERACTIVE :
      /*
       * Possibly retrieve data...
       */

        gimp_get_data(PLUG_IN_NAME, &vars);

        current_printer = get_printer_by_driver(vars.driver);

	/*
	 * Get information from the dialog...
	 */

	if (!do_print_dialog())
          return;
        break;

    case RUN_NONINTERACTIVE :
       /*
        * Make sure all the arguments are present...
        */

        if (nparams < 11)
	  values[0].data.d_status = STATUS_CALLING_ERROR;
	else
	{
	  strcpy(vars.output_to, param[3].data.d_string);
	  strcpy(vars.driver, param[4].data.d_string);
	  strcpy(vars.ppd_file, param[5].data.d_string);
	  vars.output_type = param[6].data.d_int32;
	  strcpy(vars.resolution, param[7].data.d_string);
	  strcpy(vars.media_size, param[8].data.d_string);
	  strcpy(vars.media_type, param[9].data.d_string);
	  strcpy(vars.media_source, param[10].data.d_string);

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
            vars.saturation = param[22].data.d_float;
          else
            vars.saturation = 1.0;

          if (nparams > 23)
            vars.density = param[23].data.d_float;
          else
            vars.density = 1.0;
	}

        current_printer = get_printer_by_driver(vars.driver);
        break;

    case RUN_WITH_LAST_VALS :
       /*
        * Possibly retrieve data...
        */

	gimp_get_data(PLUG_IN_NAME, &vars);

        current_printer = get_printer_by_driver(vars.driver);
	break;

    default :
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
      gimp_tile_cache_ntiles((drawable->height + gimp_tile_width() - 1) /
                             gimp_tile_width() + 1);
    else
      gimp_tile_cache_ntiles((drawable->width + gimp_tile_width() - 1) /
                             gimp_tile_width() + 1);

   /*
    * Open the file/execute the print command...
    */

    if (plist_current > 0)
#ifndef __EMX__
      prn = popen(vars.output_to, "w");
#else
      /* OS/2 PRINT command doesn't support print from stdin, use temp file */
      prn = (tmpfile = get_tmp_filename()) ? fopen(tmpfile, "w") : NULL;
#endif
    else
      prn = fopen(vars.output_to, "wb");

    if (prn != NULL)
      {
	Gimp_Image_t image;
	image.drawable = drawable;
	vars.density *= current_printer->density;
	compute_lut(&lut, current_printer->gamma, gimp_gamma(), &vars);
	/*
	 * Is the image an Indexed type?  If so we need the colormap...
	 */

	if (gimp_image_base_type (image_ID) == INDEXED)
	  cmap = gimp_image_get_cmap (image_ID, &ncolors);
	else
	  {
	    cmap    = NULL;
	    ncolors = 0;
	  }

	/*
	 * Finally, call the print driver to send the image to the printer and
	 * close the output file/command...
	 */

	(*current_printer->print) (current_printer->model,
				   1, prn, &image, cmap, &lut, &vars);

	if (plist_current > 0)
#ifndef __EMX__
	  pclose(prn);
#else
	{ /* PRINT temp file */
	  char *s;
	  fclose(prn);
	  s = g_strconcat(vars.output_to, tmpfile, NULL);
	  if (system(s) != 0)
	    values[0].data.d_status = STATUS_EXECUTION_ERROR;
	  g_free(s);
	  remove(tmpfile);
	  g_free(tmpfile);
	}
#endif
	else
	  fclose(prn);
      }
    else
      values[0].data.d_status = STATUS_EXECUTION_ERROR;

   /*
    * Store data...
    */

    if (run_mode == RUN_INTERACTIVE)
      gimp_set_data(PLUG_IN_NAME, &vars, sizeof(vars));
  }

 /*
  * Detach from the drawable...
  */

  gimp_drawable_detach(drawable);

  if (export == EXPORT_EXPORT)
    gimp_image_delete (image_ID);
}

/*
 * 'do_print_dialog()' - Pop up the print dialog...
 */

gint
do_print_dialog (void)
{
  /*
   * Initialize the program's display...
   */
  gimp_ui_init ("print", TRUE);

#ifdef SIGBUS
  signal (SIGBUS, SIG_DFL);
#endif
  signal (SIGSEGV, SIG_DFL);

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

  return runme;
}

static void
initialize_printer(plist_t *printer)
{
  printer->v.output_type = vars.output_type;
  printer->v.scaling = vars.scaling;
  printer->v.orientation = vars.orientation;
  printer->v.left = 0;
  printer->v.top = 0;
  printer->v.gamma = vars.gamma;
  printer->v.contrast = vars.contrast;
  printer->v.brightness = vars.brightness;
  printer->v.red = vars.red;
  printer->v.green = vars.green;
  printer->v.blue = vars.blue;
  printer->v.linear = vars.linear;
  printer->v.saturation = vars.saturation;
  printer->v.density = vars.density;
}

/*
 * 'printrc_load()' - Load the printer resource configuration file.
 */

static void
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

  initialize_printer(&key);

 /*
  * Get the printer list...
  */

  get_system_printers();

 /*
  * Generate the filename for the current user...
  */

  filename = gimp_personal_rc_file ("printrc");
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
     /*
      * Read the command-delimited printer definition data.  Note that
      * we can't use sscanf because %[^,] fails if the string is empty...
      */

      if ((commaptr = strchr(line, ',')) == NULL)
        continue;	/* Skip old printer definitions */

      strncpy(key.name, line, commaptr - line);
      key.name[commaptr - line] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.v.output_to, lineptr, commaptr - lineptr);
      key.v.output_to[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.v.driver, lineptr, commaptr - lineptr);
      key.v.driver[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.v.ppd_file, lineptr, commaptr - lineptr);
      key.v.ppd_file[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      key.v.output_type = atoi(lineptr);
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.v.resolution, lineptr, commaptr - lineptr);
      key.v.resolution[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.v.media_size, lineptr, commaptr - lineptr);
      key.v.media_size[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.v.media_type, lineptr, commaptr - lineptr);
      key.v.media_type[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
	{
	  strcpy(key.v.media_source, lineptr);
	  keepgoing = 0;
	  key.v.media_source[strlen(key.v.media_source) - 1] = '\0';  /* Drop NL */
	}
      else
	{
	  strncpy(key.v.media_source, lineptr, commaptr - lineptr);
	  key.v.media_source[commaptr - lineptr] = '\0';
	  lineptr = commaptr + 1;
	}


      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.brightness = atoi(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.scaling = atof(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.orientation = atoi(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.left = atoi(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.top = atoi(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.gamma = atof(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.contrast = atoi(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.red = atoi(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.green = atoi(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.blue = atoi(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.linear = atoi(lineptr);
	  lineptr = commaptr + 1;
	}

      if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.saturation = atof(lineptr);
	  lineptr = commaptr + 1;
	}
	  
      if ((keepgoing == 0))
	{
	  keepgoing = 0;
	}
      else
	{
	  key.v.density = atof(lineptr);
	  lineptr = commaptr + 1;
	}
	  
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
          else if (plist_count < MAX_PLIST - 1)
    	    {
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


 /*
  * Generate the filename for the current user...
  */

  filename = gimp_personal_rc_file ("printrc");
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

    fputs("#PRINTRC " PLUG_IN_VERSION "\n", fp);

    for (i = 0, p = plist; i < plist_count; i ++, p ++)
      fprintf(fp, "%s,%s,%s,%s,%d,%s,%s,%s,%s,%d,%.3f,%d,%d,%d,%.3f,%d,%d,%d,%d,%d,%.3f,%.3f\n",
              p->name, p->v.output_to, p->v.driver, p->v.ppd_file,
	      p->v.output_type, p->v.resolution, p->v.media_size,
	      p->v.media_type, p->v.media_source, p->v.brightness,
	      p->v.scaling, p->v.orientation, p->v.left, p->v.top,
	      p->v.gamma, p->v.contrast, p->v.red, p->v.green, p->v.blue,
	      p->v.linear, p->v.saturation, p->v.density);

    fclose(fp);
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
  return (strcmp(p1->name, p2->name));
}


/*
 *  get_system_printers() - Get a complete list of printers from the spooler.
 */

static void
get_system_printers (void)
{
  int	i;
  char defname[17];
#if defined(LPC_COMMAND) || defined(LPSTAT_COMMAND)
  FILE	*pfile;
  char	line[129];
#endif
#if defined(LPSTAT_COMMAND)
  char  name[17];
#endif
#ifdef __EMX__
  BYTE  pnum;
#endif


  defname[0] = '\0';

  memset(plist, 0, sizeof(plist));
  plist_count = 1;
  strcpy(plist[0].name, _("File"));
  plist[0].v.output_to[0] = '\0';
  strcpy(plist[0].v.driver, "ps2");
  initialize_printer (&plist[0]);

#ifdef LPC_COMMAND
  if ((pfile = popen(LPC_COMMAND " status < /dev/null", "r")) != NULL)
  {
    while (fgets(line, sizeof(line), pfile) != NULL &&
           plist_count < MAX_PLIST)
      if (strchr(line, ':') != NULL && line[0] != ' ' && 
	  line[0] != '\t' && strncmp(line,"Press RETURN to continue",24))
      {
        *strchr(line, ':') = '\0';
        strcpy(plist[plist_count].name, line);
        sprintf(plist[plist_count].v.output_to, LPR_COMMAND " -P%s -l", line);
        strcpy(plist[plist_count].v.driver, "ps2");
	initialize_printer(&plist[plist_count]);
        plist_count ++;
      }

    pclose(pfile);
  }
#endif /* LPC_COMMAND */

#ifdef LPSTAT_COMMAND
  if ((pfile = popen(LPSTAT_COMMAND " -d -p", "r")) != NULL)
  {
    while (fgets(line, sizeof(line), pfile) != NULL &&
           plist_count < MAX_PLIST)
    {
      if (sscanf(line, "printer %s", name) == 1)
      {
	strcpy(plist[plist_count].name, name);
	sprintf(plist[plist_count].v.output_to, LP_COMMAND " -s -d%s", name);
        strcpy(plist[plist_count].v.driver, "ps2");
	initialize_printer(&plist[plist_count]);
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
	  sprintf(plist[plist_count].name, "LPT%d:", i);
	  sprintf(plist[plist_count].v.output_to, "PRINT /D:LPT%d /B ", i);
          strcpy(plist[plist_count].v.driver, "ps2");
	  initialize_printer(&plist[plist_count]);
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

void
Image_init(Image image)
{
  Gimp_Image_t *gimage = (Gimp_Image_t *) image;
  gimp_pixel_rgn_init(&(gimage->rgn), gimage->drawable, 0, 0,
		      gimage->drawable->width, gimage->drawable->height,
		      FALSE, FALSE);
}

int
Image_bpp(Image image)
{
  Gimp_Image_t *gimage = (Gimp_Image_t *) image;
  return gimage->drawable->bpp;
}

int
Image_width(Image image)
{
  Gimp_Image_t *gimage = (Gimp_Image_t *) image;
  return gimage->drawable->width;
}

int
Image_height(Image image)
{
  Gimp_Image_t *gimage = (Gimp_Image_t *) image;
  return gimage->drawable->height;
}

void
Image_get_col(Image image, unsigned char *data, int column)
{
  Gimp_Image_t *gimage = (Gimp_Image_t *) image;
  gimp_pixel_rgn_get_col(&(gimage->rgn), data, column, 0,
			 gimage->drawable->height);
}

void
Image_get_row(Image image, unsigned char *data, int row)
{
  Gimp_Image_t *gimage = (Gimp_Image_t *) image;
  gimp_pixel_rgn_get_row(&(gimage->rgn), data, 0, row,
			 gimage->drawable->width);
}

void
Image_progress_init(Image image)
{
  image = image;
  gimp_progress_init(_("Printing..."));
}

void
Image_note_progress(Image image, double current, double total)
{
  image = image;
  gimp_progress_update(current / total);
}

const char *
Image_get_pluginname(Image image)
{
  static char pluginname[] = PLUG_IN_NAME " plug-in V" PLUG_IN_VERSION
    "for GIMP";
  image = image;
  return pluginname;
}

/*
 * End of "$Id$".
 */
