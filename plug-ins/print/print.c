/*
 * "$Id$"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com)
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

#include "print.h"
#include <math.h>
#include <signal.h>
#ifdef __EMX__
#define INCL_DOSDEVICES
#define INCL_DOSERRORS
#include <os2.h>
#endif

#include "libgimp/stdplugins-intl.h"

/*
 * Constants for GUI...
 */

#define SCALE_WIDTH		64
#define ENTRY_WIDTH		64
#define PREVIEW_SIZE		220	/* Assuming max media size of 22" */
#define MAX_PLIST		100


/*
 * Types...
 */

typedef struct		/**** Printer List ****/
{
  char	name[17],		/* Name of printer */
	command[255],		/* Printer command */
	driver[33],		/* Short name of printer driver */
	ppd_file[255];		/* PPD file for printer */
  int	output_type;		/* Color/B&W */
  char	resolution[33],		/* Resolution */
	media_size[33],		/* Media size */
	media_type[33],		/* Media type */
	media_source[33];	/* Media source */
} plist_t;


/*
 * Local functions...
 */

static void	printrc_load(void);
static void	printrc_save(void);
static int	compare_printers(plist_t *p1, plist_t *p2);
static void	get_printers(void);

static void	query(void);
static void	run(char *, int, GParam *, int *, GParam **);
static int	do_print_dialog(void);
static void	brightness_update(GtkAdjustment *);
static void	brightness_callback(GtkWidget *);
static void	scaling_update(GtkAdjustment *);
static void	scaling_callback(GtkWidget *);
static void	plist_callback(GtkWidget *, gint);
static void	media_size_callback(GtkWidget *, gint);
static void	media_type_callback(GtkWidget *, gint);
static void	media_source_callback(GtkWidget *, gint);
static void	resolution_callback(GtkWidget *, gint);
static void	output_type_callback(GtkWidget *, gint);
static void	orientation_callback(GtkWidget *, gint);
static void	print_callback(void);
static void	cancel_callback(void);
static void	close_callback(void);

static void	setup_open_callback(void);
static void	setup_ok_callback(void);
static void	setup_cancel_callback(void);
static void	ppd_browse_callback(void);
static void	ppd_ok_callback(void);
static void	ppd_cancel_callback(void);
static void	print_driver_callback(GtkWidget *, gint);

static void	file_ok_callback(void);
static void	file_cancel_callback(void);

static void	preview_update(void);
static void	preview_button_callback(GtkWidget *, GdkEventButton *);
static void	preview_motion_callback(GtkWidget *, GdkEventMotion *);


/*
 * Globals...
 */

GPlugInInfo	PLUG_IN_INFO =		/* Plug-in information */
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

struct					/* Plug-in variables */
{
  char	output_to[255],		/* Name of file or command to print to */
	short_name[33],		/* Name of printer "driver" */
	ppd_file[255];		/* PPD file */
  gint	output_type;		/* Color or grayscale output */
  char	resolution[33],		/* Resolution */
	media_size[33],		/* Media size */
	media_type[33],		/* Media type */
	media_source[33];	/* Media source */
  gint	brightness;		/* Output brightness */
  float	scaling;		/* Scaling, percent of printable area */
  gint	orientation,		/* Orientation - 0 = port., 1 = land., -1 = auto */
	left,			/* Offset from lower-lefthand corner, points */
	top;			/* ... */
}		vars =
{
	"",			/* Name of file or command to print to */
	"ps2",			/* Name of printer "driver" */
	"",			/* Name of PPD file */
	OUTPUT_COLOR,		/* Color or grayscale output */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	100,			/* Output brightness */
	100.0,			/* Scaling (100% means entire printable area, */
				/*          -XXX means scale by PPI) */
	-1,			/* Orientation (-1 = automatic) */
	-1,			/* X offset (-1 = center) */
	-1			/* Y offset (-1 = center) */
};

GtkWidget	*print_dialog,		/* Print dialog window */
		*media_size,		/* Media size option button */
		*media_size_menu=NULL,	/* Media size menu */
		*media_type,		/* Media type option button */
		*media_type_menu=NULL,	/* Media type menu */
		*media_source,		/* Media source option button */
		*media_source_menu=NULL,/* Media source menu */
		*resolution,		/* Resolution option button */
		*resolution_menu=NULL,	/* Resolution menu */
		*scaling_scale,		/* Scale widget for scaling */
		*scaling_entry,		/* Text entry widget for scaling */
		*scaling_percent,	/* Scale by percent */
		*scaling_ppi,		/* Scale by pixels-per-inch */
		*brightness_scale,	/* Scale for brightness */
		*brightness_entry,	/* Text entry widget for brightness */
		*output_gray,		/* Output type toggle, black */
		*output_color,		/* Output type toggle, color */
		*setup_dialog,		/* Setup dialog window */
		*printer_driver,	/* Printer driver widget */
		*ppd_file,		/* PPD file entry */
		*ppd_button,		/* PPD file browse button */
		*output_cmd,		/* Output command text entry */
		*ppd_browser,		/* File selection dialog for PPD files */
		*file_browser;		/* FSD for print files */

GtkObject	*scaling_adjustment,	/* Adjustment object for scaling */
		*brightness_adjustment;	/* Adjustment object for brightness */

int		num_media_sizes=0;	/* Number of media sizes */
char		**media_sizes;		/* Media size strings */
int		num_media_types=0;	/* Number of media types */
char		**media_types;		/* Media type strings */
int		num_media_sources=0;	/* Number of media sources */
char		**media_sources;	/* Media source strings */
int		num_resolutions=0;	/* Number of resolutions */
char		**resolutions;		/* Resolution strings */

GtkDrawingArea	*preview;		/* Preview drawing area widget */
int		mouse_x,		/* Last mouse X */
		mouse_y;		/* Last mouse Y */
int		image_width,		/* Width of image */
		image_height,		/* Height of image */
		page_left,		/* Left pixel column of page */
		page_top,		/* Top pixel row of page */
		page_width,		/* Width of page on screen */
		page_height,		/* Height of page on screen */
		print_width,		/* Printed width of image */
		print_height;		/* Printed height of image */

int		plist_current = 0,	/* Current system printer */
		plist_count = 0;	/* Number of system printers */
plist_t		plist[MAX_PLIST];	/* System printers */

int		runme = FALSE,		/* True if print should proceed */
		current_printer = 0;	/* Current printer index */

printer_t	printers[] =		/* List of supported printer types */
{
  { N_("PostScript Level 1"),	"ps",		1,	0,	1.000,	1.000,
    ps_parameters,	ps_media_size,	ps_imageable_area,	ps_print },
  { N_("PostScript Level 2"),	"ps2",		1,	1,	1.000,	1.000,
    ps_parameters,	ps_media_size,	ps_imageable_area,	ps_print },
  { N_("HP DeskJet 500, 520"),	"pcl-500",	0,	500,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP DeskJet 500C, 540C"),	"pcl-501",	1,	501,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP DeskJet 550C, 560C"),	"pcl-550",	1,	550,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP DeskJet 600 series"),	"pcl-600",	1,	600,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP DeskJet 800 series"),	"pcl-800",	1,	800,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP DeskJet 1100C, 1120C"),	"pcl-1100",	1,	1100,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP DeskJet 1200C, 1600C"),	"pcl-1200",	1,	1200,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP LaserJet II series"),	"pcl-2",	0,	2,	1.000,	0.596,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP LaserJet III series"),	"pcl-3",	0,	3,	1.000,	0.596,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP LaserJet 4 series"),	"pcl-4",	0,	4,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP LaserJet 4V, 4Si"),	"pcl-4v",	0,	5,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP LaserJet 5 series"),	"pcl-5",	0,	4,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP LaserJet 5Si"),		"pcl-5si",	0,	5,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("HP LaserJet 6 series"),	"pcl-6",	0,	4,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { N_("EPSON Stylus Color"),	"escp2",	1,	0,	0.597,	0.568,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Color Pro"),	"escp2-pro",	1,	1,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Color Pro XL"),"escp2-proxl",	1,	1,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Color 1500"),	"escp2-1500",	1,	2,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Color 400"),	"escp2-400",	1,	1,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Color 500"),	"escp2-500",	1,	1,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Color 600"),	"escp2-600",	1,	3,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Color 800"),	"escp2-800",	1,	4,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Color 1520"),	"escp2-1520",	1,	5,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Color 3000"),	"escp2-3000",	1,	5,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print }
};


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
    { PARAM_INT32,	"brightness",	"Brightness (0-200%)" },
    { PARAM_FLOAT,	"scaling",	"Output scaling (0-100%, -PPI)" },
    { PARAM_INT32,	"orientation",	"Output orientation (-1 = auto, 0 = portrait, 1 = landscape)" },
    { PARAM_INT32,	"left",		"Left offset (points, -1 = centered)" },
    { PARAM_INT32,	"top",		"Top offset (points, -1 = centered)" }
  };
  static int		nargs = sizeof(args) / sizeof(args[0]);

  INIT_I18N();

  gimp_install_procedure(
      "file_print",
      _("This plug-in prints images from The GIMP."),
      _("Prints images to PostScript, PCL, or ESC/P2 printers."),
      "Michael Sweet <mike@easysw.com>",
      "Copyright 1997-1998 by Michael Sweet",
      PLUG_IN_VERSION,
      N_("<Image>/File/Print"),
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
  printer_t	*printer;	/* Printer driver entry */
  int		i;		/* Looping var */
  float		brightness,	/* Computed brightness */
		screen_gamma,	/* Screen gamma correction */
		print_gamma,	/* Printer gamma correction */
		pixel;		/* Pixel value */
  guchar	lut[256];	/* Lookup table for brightness */
  guchar	*cmap;		/* Colormap (indexed images only) */
  int		ncolors;	/* Number of colors in colormap */
  GParam	*values;	/* Return values */
#ifdef __EMX__
  char		*tmpfile;	/* temp filename */
#endif

  INIT_I18N_UI();

 /*
  * Initialize parameter data...
  */

  run_mode = param[0].data.d_int32;

  values = g_new(GParam, 1);

  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

 /*
  * Get drawable...
  */

  drawable = gimp_drawable_get(param[2].data.d_drawable);

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

        for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
          if (strcmp(printers[i].short_name, vars.short_name) == 0)
            current_printer = i;

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
	  strcpy(vars.short_name, param[4].data.d_string);
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
	};

        for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
          if (strcmp(printers[i].short_name, vars.short_name) == 0)
            current_printer = i;
        break;

    case RUN_WITH_LAST_VALS :
       /*
        * Possibly retrieve data...
        */

	gimp_get_data(PLUG_IN_NAME, &vars);

        for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
          if (strcmp(printers[i].short_name, vars.short_name) == 0)
            current_printer = i;
	break;

    default :
        values[0].data.d_status = STATUS_CALLING_ERROR;
        break;;
  };

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
     /*
      * Got an output file/command, now compute a brightness lookup table...
      */

      printer      = printers + current_printer;
      brightness   = 100.0 / vars.brightness;
      screen_gamma = gimp_gamma() * brightness / 1.7;
      print_gamma  = 1.0 / printer->gamma;

      for (i = 0; i < 256; i ++)
      {
        pixel = 1.0 - pow((float)i / 255.0, screen_gamma);
        pixel = 255.5 - 255.0 * printer->density *
  	        	pow(brightness * pixel, print_gamma);

	if (pixel <= 0.0)
	  lut[i] = 0;
	else if (pixel >= 255.0)
	  lut[i] = 255;
	else
	  lut[i] = (int)pixel;
      };

     /*
      * Is the image an Indexed type?  If so we need the colormap...
      */

      if (gimp_image_base_type(param[1].data.d_image) == INDEXED)
        cmap = gimp_image_get_cmap(param[1].data.d_image, &ncolors);
      else
      {
        cmap    = NULL;
        ncolors = 0;
      };

     /*
      * Finally, call the print driver to send the image to the printer and
      * close the output file/command...
      */

      (*printer->print)(printer->model, vars.ppd_file, vars.resolution,
                        vars.media_size, vars.media_type, vars.media_source,
                        vars.output_type, vars.orientation, vars.scaling,
                        vars.left, vars.top, 1, prn, drawable, lut, cmap);

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
  };

 /*
  * Detach from the drawable...
  */

  gimp_drawable_detach(drawable);
}


/*
 * 'do_print_dialog()' - Pop up the print dialog...
 */

int
do_print_dialog(void)
{
  int		i;		/* Looping var */
  char		s[100];		/* Text string */
  GtkWidget	*dialog,	/* Dialog window */
		*table,		/* Table "container" for controls */
		*label,		/* Label string */
		*button,	/* OK/Cancel buttons */
		*scale,		/* Scale widget */
		*entry,		/* Text entry widget */
		*menu,		/* Menu of drivers/sizes */
		*item,		/* Menu item */
		*option,	/* Option menu button */
		*box;		/* Box container */
  GtkObject	*scale_data;	/* Scale data (limits) */
  GSList	*group;		/* Grouping for output type */
  gint		argc;		/* Fake argc for GUI */
  gchar		**argv;		/* Fake argv for GUI */
  static char	*orients[] =	/* Orientation strings */
  {
    N_("Auto"),
    N_("Portrait"),
    N_("Landscape")
  };
  char plug_in_name[80];


 /*
  * Initialize the program's display...
  */

  argc    = 1;
  argv    = g_new(gchar *, 1);
  argv[0] = g_strdup("print");

  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());
  gdk_set_use_xshm(gimp_use_xshm());

#ifdef SIGBUS
  signal(SIGBUS, SIG_DFL);
#endif
  signal(SIGSEGV, SIG_DFL);

 /*
  * Get printrc options...
  */

  printrc_load();

 /*
  * Print dialog window...
  */

  print_dialog = dialog = gtk_dialog_new();
  sprintf(plug_in_name, _("Print v%s"), PLUG_IN_VERSION);

  gtk_window_set_title(GTK_WINDOW(dialog), plug_in_name);
  gtk_window_set_wmclass(GTK_WINDOW(dialog), "print", "Gimp");
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width(GTK_CONTAINER(dialog), 0);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		     (GtkSignalFunc)close_callback, NULL);

 /*
  * Top-level table for dialog...
  */

  table = gtk_table_new(9, 4, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(table), 4);
  gtk_table_set_row_spacings(GTK_TABLE(table), 8);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, FALSE, FALSE, 0);
  gtk_widget_show(table);

 /*
  * Drawing area for page preview...
  */

  preview = (GtkDrawingArea *)gtk_drawing_area_new();
  gtk_drawing_area_size(preview, PREVIEW_SIZE, PREVIEW_SIZE);
  gtk_table_attach(GTK_TABLE(table), (GtkWidget *)preview, 0, 2, 0, 7,
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview), "expose_event",
		     (GtkSignalFunc)preview_update,
		     NULL);
  gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview), "button_press_event",
		     (GtkSignalFunc)preview_button_callback,
		     NULL);
  gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview), "button_release_event",
		     (GtkSignalFunc)preview_button_callback,
		     NULL);
  gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview), "motion_notify_event",
		     (GtkSignalFunc)preview_motion_callback,
		     NULL);
  gtk_widget_show((GtkWidget *)preview);

  gtk_widget_set_events((GtkWidget *)preview,
                        GDK_EXPOSURE_MASK | GDK_BUTTON_MOTION_MASK |
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

 /*
  * Media size option menu...
  */

  label = gtk_label_new(_("Media Size:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(table), box, 3, 4, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  media_size = option = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);

 /*
  * Media type option menu...
  */

  label = gtk_label_new(_("Media Type:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(table), box, 3, 4, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  media_type = option = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);

 /*
  * Media source option menu...
  */

  label = gtk_label_new(_("Media Source:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(table), box, 3, 4, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  media_source = option = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);

 /*
  * Orientation option menu...
  */

  label = gtk_label_new(_("Orientation:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  menu = gtk_menu_new();
  for (i = 0; i < (int)(sizeof(orients) / sizeof(orients[0])); i ++)
  {
    item = gtk_menu_item_new_with_label(gettext(orients[i]));
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       (GtkSignalFunc)orientation_callback,
		       (gpointer)(i - 1));
    gtk_widget_show(item);
  };

  box = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(table), box, 3, 4, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  option = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option), vars.orientation + 1);
  gtk_widget_show(option);

 /*
  * Resolution option menu...
  */

  label = gtk_label_new(_("Resolution:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(table), box, 3, 4, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  resolution = option = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);

 /*
  * Output type toggles...
  */

  label = gtk_label_new(_("Output Type:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 6, 7, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 3, 4, 6, 7, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  output_gray = button = gtk_radio_button_new_with_label(NULL, _("B&W"));
  group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
  if (vars.output_type == 0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     (GtkSignalFunc)output_type_callback,
		     (gpointer)OUTPUT_GRAY);
  gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  output_color = button = gtk_radio_button_new_with_label(group, _("Color"));
  if (vars.output_type == 1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     (GtkSignalFunc)output_type_callback,
		     (gpointer)OUTPUT_COLOR);
  gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

 /*
  * Scaling...
  */

  label = gtk_label_new(_("Scaling:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 7, 8, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 7, 8, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  if (vars.scaling < 0.0)
    scaling_adjustment = scale_data =
	gtk_adjustment_new(-vars.scaling, 50.0, 1201.0, 1.0, 1.0, 1.0);
  else
    scaling_adjustment = scale_data =
	gtk_adjustment_new(vars.scaling, 5.0, 101.0, 1.0, 1.0, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc)scaling_update, NULL);

  scaling_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
  gtk_widget_set_usize(scale, 200, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

  scaling_entry = entry = gtk_entry_new();
  sprintf(s, "%.1f", fabs(vars.scaling));
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
                     (GtkSignalFunc)scaling_callback, NULL);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
  gtk_widget_set_usize(entry, 60, 0);
  gtk_widget_show(entry);

  scaling_percent = button = gtk_radio_button_new_with_label(NULL, _("Percent"));
  group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
  if (vars.scaling > 0.0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     (GtkSignalFunc)scaling_callback, NULL);
  gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  scaling_ppi = button = gtk_radio_button_new_with_label(group, _("PPI"));
  if (vars.scaling < 0.0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
                     (GtkSignalFunc)scaling_callback, NULL);
  gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

 /*
  * Brightness slider...
  */

  label = gtk_label_new(_("Brightness:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 8, 9, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 8, 9, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  brightness_adjustment = scale_data =
      gtk_adjustment_new((float)vars.brightness, 50.0, 201.0, 1.0, 1.0, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc)brightness_update, NULL);

  brightness_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
  gtk_widget_set_usize(scale, 200, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

  brightness_entry = entry = gtk_entry_new();
  sprintf(s, "%d", vars.brightness);
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)brightness_callback, NULL);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
  gtk_widget_set_usize(entry, 40, 0);
  gtk_widget_show(entry);

 /*
  * Printer option menu...
  */

  label = gtk_label_new(_("Printer:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  menu = gtk_menu_new();
  for (i = 0; i < plist_count; i ++)
  {
    item = gtk_menu_item_new_with_label(gettext(plist[i].name));
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       (GtkSignalFunc)plist_callback,
		       (gpointer)i);
    gtk_widget_show(item);
  };

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 3, 4, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  option = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option), plist_current);
  gtk_widget_show(option);

  button = gtk_button_new_with_label(_("Setup"));
  gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)setup_open_callback, NULL);
  gtk_widget_show(button);

 /*
  * Print, cancel buttons...
  */

  gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(dialog)->action_area), FALSE);
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->action_area), 0);

  button = gtk_button_new_with_label(_("Cancel"));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)cancel_callback, NULL);
  gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  button = gtk_button_new_with_label(_("Print"));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)print_callback, NULL);
  gtk_widget_grab_default(button);
  gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

 /*
  * Setup dialog window...
  */

  setup_dialog = dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), _("Setup"));
  gtk_window_set_wmclass(GTK_WINDOW(dialog), "print", "Gimp");
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width(GTK_CONTAINER(dialog), 0);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		     (GtkSignalFunc)setup_cancel_callback, NULL);

 /*
  * Top-level table for dialog...
  */

  table = gtk_table_new(3, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(table), 4);
  gtk_table_set_row_spacings(GTK_TABLE(table), 8);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, FALSE, FALSE, 0);
  gtk_widget_show(table);

 /*
  * Printer driver option menu...
  */

  label = gtk_label_new(_("Driver:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  menu = gtk_menu_new();
  for (i = 0; i < (int)(sizeof(printers) / sizeof(printers[0])); i ++)
  {
    item = gtk_menu_item_new_with_label(gettext(printers[i].long_name));
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       (GtkSignalFunc)print_driver_callback,
		       (gpointer)i);
    gtk_widget_show(item);
  };

  printer_driver = option = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
  gtk_widget_show(option);

 /*
  * PPD file...
  */

  label = gtk_label_new(_("PPD File:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  ppd_file = entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 0);
  gtk_widget_show(entry);

  ppd_button = button = gtk_button_new_with_label(_("Browse"));
  gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)ppd_browse_callback, NULL);
  gtk_widget_show(button);

 /*
  * Print command...
  */

  label = gtk_label_new(_("Command:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  output_cmd = entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(entry);

 /*
  * OK, cancel buttons...
  */

  gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(dialog)->action_area), FALSE);
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->action_area), 0);

  button = gtk_button_new_with_label(_("Cancel"));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)setup_cancel_callback, NULL);
  gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  button = gtk_button_new_with_label(_("OK"));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)setup_ok_callback, NULL);
  gtk_widget_grab_default(button);
  gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

 /*
  * Output file selection dialog...
  */

  file_browser = gtk_file_selection_new(_("Print To File?"));
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_browser)->ok_button),
                     "clicked", (GtkSignalFunc)file_ok_callback, NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_browser)->cancel_button),
                     "clicked", (GtkSignalFunc)file_cancel_callback, NULL);

 /*
  * PPD file selection dialog...
  */

  ppd_browser = gtk_file_selection_new(_("PPD File?"));
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(ppd_browser));
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(ppd_browser)->ok_button),
                     "clicked", (GtkSignalFunc)ppd_ok_callback, NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(ppd_browser)->cancel_button),
                     "clicked", (GtkSignalFunc)ppd_cancel_callback, NULL);

 /*
  * Show the main dialog and wait for the user to do something...
  */

  plist_callback(NULL, plist_current);

  gtk_widget_show(print_dialog);

  gtk_main();
  gdk_flush();

 /*
  * Set printrc options...
  */

  printrc_save();

 /*
  * Return ok/cancel...
  */

  return (runme);
}


/*
 * 'brightness_update()' - Update the brightness field using the scale.
 */

static void
brightness_update(GtkAdjustment *adjustment)	/* I - New value */
{
  char	s[255];					/* Text buffer */


  if (vars.brightness != adjustment->value)
  {
    vars.brightness = adjustment->value;

    sprintf(s, "%d", vars.brightness);

    gtk_signal_handler_block_by_data(GTK_OBJECT(brightness_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(brightness_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(brightness_entry), NULL);

    preview_update();
  };
}


/*
 * 'brightness_callback()' - Update the brightness scale using the text entry.
 */

static void
brightness_callback(GtkWidget *widget)	/* I - Entry widget */
{
  gint		new_value;		/* New scaling value */


  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (vars.brightness != new_value)
  {
    if ((new_value >= GTK_ADJUSTMENT(brightness_adjustment)->lower) &&
	(new_value < GTK_ADJUSTMENT(brightness_adjustment)->upper))
    {
      GTK_ADJUSTMENT(brightness_adjustment)->value = new_value;

      gtk_signal_emit_by_name(brightness_adjustment, "value_changed");
    };
  };
}


/*
 * 'scaling_update()' - Update the scaling field using the scale.
 */

static void
scaling_update(GtkAdjustment *adjustment)	/* I - New value */
{
  char	s[255];					/* Text buffer */


  if (vars.scaling != adjustment->value)
  {
    if (GTK_TOGGLE_BUTTON(scaling_ppi)->active)
      vars.scaling = -adjustment->value;
    else
      vars.scaling = adjustment->value;

    sprintf(s, "%.1f", adjustment->value);

    gtk_signal_handler_block_by_data(GTK_OBJECT(scaling_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(scaling_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(scaling_entry), NULL);

    preview_update();
  };
}


/*
 * 'scaling_callback()' - Update the scaling scale using the text entry.
 */

static void
scaling_callback(GtkWidget *widget)	/* I - Entry widget */
{
  gfloat	new_value;		/* New scaling value */


  if (widget == scaling_entry)
  {
    new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));

    if (vars.scaling != new_value)
    {
      if ((new_value >= GTK_ADJUSTMENT(scaling_adjustment)->lower) &&
	  (new_value < GTK_ADJUSTMENT(scaling_adjustment)->upper))
      {
	GTK_ADJUSTMENT(scaling_adjustment)->value = new_value;

	gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
      };
    };
  }
  else if (widget == scaling_ppi)
  {
    GTK_ADJUSTMENT(scaling_adjustment)->lower = 50.0;
    GTK_ADJUSTMENT(scaling_adjustment)->upper = 1201.0;
    GTK_ADJUSTMENT(scaling_adjustment)->value = 72.0;
    vars.scaling = 0.0;
    gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
  }
  else if (widget == scaling_percent)
  {
    GTK_ADJUSTMENT(scaling_adjustment)->lower = 5.0;
    GTK_ADJUSTMENT(scaling_adjustment)->upper = 101.0;
    GTK_ADJUSTMENT(scaling_adjustment)->value = 100.0;
    vars.scaling = 0.0;
    gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
  };
}


/*
 * 'plist_build_menu()' - Build an option menu for the given parameters...
 */

static void
plist_build_menu(GtkWidget *option,				/* I - Option button */
                 GtkWidget **menu,				/* IO - Current menu */
                 int       num_items,				/* I - Number of items */
                 char      **items,				/* I - Menu items */
                 char      *cur_item,				/* I - Current item */
                 void      (*callback)(GtkWidget *, gint))	/* I - Callback */
{
  int		i;	/* Looping var */
  GtkWidget	*item,	/* Menu item */
		*item0;	/* First menu item */


  if (*menu != NULL)
  {
    gtk_widget_destroy(*menu);
    *menu = NULL;
  };

  if (num_items == 0)
  {
    gtk_widget_hide(option);
    return;
  };

  *menu = gtk_menu_new();

  for (i = 0; i < num_items; i ++)
  {
    item = gtk_menu_item_new_with_label(gettext(items[i]));
    if (i == 0)
      item0 = item;
    gtk_menu_append(GTK_MENU(*menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       (GtkSignalFunc)callback, (gpointer)i);
    gtk_widget_show(item);
  };

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), *menu);

#ifdef DEBUG
  printf("cur_item = \'%s\'\n", cur_item);
#endif /* DEBUG */

  for (i = 0; i < num_items; i ++)
  {
#ifdef DEBUG
    printf("item[%d] = \'%s\'\n", i, items[i]);
#endif /* DEBUG */

    if (strcmp(items[i], cur_item) == 0)
    {
      gtk_option_menu_set_history(GTK_OPTION_MENU(option), i);
      break;
    };
  };

  if (i == num_items)
  {
    gtk_option_menu_set_history(GTK_OPTION_MENU(option), 0);
    gtk_signal_emit_by_name(GTK_OBJECT(item0), "activate");
  };

  gtk_widget_show(option);
}


/*
 * 'plist_callback()' - Update the current system printer...
 */

static void
plist_callback(GtkWidget *widget,	/* I - Driver option menu */
               gint      data)		/* I - Data */
{
  int		i;			/* Looping var */
  printer_t	*printer;		/* Printer driver entry */
  plist_t	*p;


  plist_current = data;
  p             = plist + plist_current;

  if (p->driver[0] != '\0')
  {
    strcpy(vars.short_name, p->driver);

    for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
      if (strcmp(printers[i].short_name, vars.short_name) == 0)
      {
        current_printer = i;
        break;
      };
  };

  strcpy(vars.ppd_file, p->ppd_file);
  strcpy(vars.media_size, p->media_size);
  strcpy(vars.media_type, p->media_type);
  strcpy(vars.media_source, p->media_source);
  strcpy(vars.resolution, p->resolution);
  strcpy(vars.output_to, p->command);

  if (p->output_type == OUTPUT_GRAY)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_gray), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_color), TRUE);

 /*
  * Now get option parameters...
  */

  printer = printers + current_printer;

  if (num_media_sizes > 0)
  {
    for (i = 0; i < num_media_sizes; i ++)
      g_free(media_sizes[i]);
    g_free(media_sizes);
  };

  media_sizes = (*(printer->parameters))(printer->model,
                                         p->ppd_file,
                                         "PageSize", &num_media_sizes);
  if (vars.media_size[0] == '\0')
    strcpy(vars.media_size, media_sizes[0]);
  plist_build_menu(media_size, &media_size_menu, num_media_sizes, media_sizes,
                   p->media_size, media_size_callback);

  if (num_media_types > 0)
  {
    for (i = 0; i < num_media_types; i ++)
      g_free(media_types[i]);
    g_free(media_types);
  };

  media_types = (*(printer->parameters))(printer->model,
                                         p->ppd_file,
                                         "MediaType", &num_media_types);
  if (vars.media_type[0] == '\0' && media_types != NULL)
    strcpy(vars.media_type, media_types[0]);
  plist_build_menu(media_type, &media_type_menu, num_media_types, media_types,
                   p->media_type, media_type_callback);

  if (num_media_sources > 0)
  {
    for (i = 0; i < num_media_sources; i ++)
      g_free(media_sources[i]);
    g_free(media_sources);
  };

  media_sources = (*(printer->parameters))(printer->model,
                                           p->ppd_file,
                                           "InputSlot", &num_media_sources);
  if (vars.media_source[0] == '\0' && media_sources != NULL)
    strcpy(vars.media_source, media_sources[0]);
  plist_build_menu(media_source, &media_source_menu, num_media_sources, media_sources,
                   p->media_source, media_source_callback);

  if (num_resolutions > 0)
  {
    for (i = 0; i < num_resolutions; i ++)
      g_free(resolutions[i]);
    g_free(resolutions);
  };

  resolutions = (*(printer->parameters))(printer->model,
                                         p->ppd_file,
                                         "Resolution", &num_resolutions);
  if (vars.resolution[0] == '\0' && resolutions != NULL)
    strcpy(vars.resolution, resolutions[0]);
  plist_build_menu(resolution, &resolution_menu, num_resolutions, resolutions,
                   p->resolution, resolution_callback);
}


/*
 * 'media_size_callback()' - Update the current media size...
 */

static void
media_size_callback(GtkWidget *widget,		/* I - Media size option menu */
                    gint      data)		/* I - Data */
{
  strcpy(vars.media_size, media_sizes[data]);
  strcpy(plist[plist_current].media_size, media_sizes[data]);
  vars.left       = -1;
  vars.top        = -1;

  preview_update();
}


/*
 * 'media_type_callback()' - Update the current media type...
 */

static void
media_type_callback(GtkWidget *widget,		/* I - Media type option menu */
                    gint      data)		/* I - Data */
{
  strcpy(vars.media_type, media_types[data]);
  strcpy(plist[plist_current].media_type, media_types[data]);
}


/*
 * 'media_source_callback()' - Update the current media source...
 */

static void
media_source_callback(GtkWidget *widget,	/* I - Media source option menu */
                      gint      data)		/* I - Data */
{
  strcpy(vars.media_source, media_sources[data]);
  strcpy(plist[plist_current].media_source, media_sources[data]);
}


/*
 * 'resolution_callback()' - Update the current resolution...
 */

static void
resolution_callback(GtkWidget *widget,		/* I - Media size option menu */
                    gint      data)		/* I - Data */
{
  strcpy(vars.resolution, resolutions[data]);
  strcpy(plist[plist_current].resolution, resolutions[data]);
}


/*
 * 'orientation_callback()' - Update the current media size...
 */

static void
orientation_callback(GtkWidget *widget,		/* I - Orientation option menu */
                    gint      data)		/* I - Data */
{
  vars.orientation = data;
  vars.left        = -1;
  vars.top         = -1;

  preview_update();
}


/*
 * 'output_type_callback()' - Update the current output type...
 */

static void
output_type_callback(GtkWidget *widget,	/* I - Output type button */
                     gint      data)	/* I - Data */
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
  {
    vars.output_type = data;
    plist[plist_current].output_type = data;
  };
}


/*
 * 'print_callback()' - Start the print...
 */

static void
print_callback(void)
{
  if (plist_current > 0)
  {
    runme = TRUE;

    gtk_widget_destroy(print_dialog);
  }
  else
    gtk_widget_show(file_browser);
}


/*
 * 'cancel_callback()' - Cancel the print...
 */

static void
cancel_callback(void)
{
  gtk_widget_destroy(print_dialog);
}


/*
 * 'close_callback()' - Exit the print dialog application.
 */

static void
close_callback(void)
{
  gtk_main_quit();
}


static void
setup_open_callback(void)
{
  int	i;	/* Looping var */


  for (i = 0; i < (int)(sizeof(printers) / sizeof(printers[0])); i ++)
    if (strcmp(plist[plist_current].driver, printers[i].short_name) == 0)
    {
      current_printer = i;
      break;
    };

  gtk_option_menu_set_history(GTK_OPTION_MENU(printer_driver), current_printer);

  gtk_entry_set_text(GTK_ENTRY(ppd_file), plist[plist_current].ppd_file);

  if (strncmp(plist[plist_current].driver, "ps", 2) == 0)
    gtk_widget_show(ppd_file);
  else
    gtk_widget_show(ppd_file);

  gtk_entry_set_text(GTK_ENTRY(output_cmd), plist[plist_current].command);

  if (plist_current == 0)
    gtk_widget_hide(output_cmd);
  else
    gtk_widget_show(output_cmd);

  gtk_widget_show(setup_dialog);
}


static void
setup_ok_callback(void)
{
  strcpy(vars.short_name, printers[current_printer].short_name);
  strcpy(plist[plist_current].driver, printers[current_printer].short_name);

  strcpy(vars.output_to, gtk_entry_get_text(GTK_ENTRY(output_cmd)));
  strcpy(plist[plist_current].command, vars.output_to);

  strcpy(vars.ppd_file, gtk_entry_get_text(GTK_ENTRY(ppd_file)));
  strcpy(plist[plist_current].ppd_file, vars.ppd_file);

  plist_callback(NULL, plist_current);

  gtk_widget_hide(setup_dialog);
}


static void
setup_cancel_callback(void)
{
  gtk_widget_hide(setup_dialog);
}


/*
 * 'print_driver_callback()' - Update the current printer driver...
 */

static void
print_driver_callback(GtkWidget *widget,	/* I - Driver option menu */
                      gint      data)		/* I - Data */
{
  current_printer = data;

  if (strncmp(printers[current_printer].short_name, "ps", 2) == 0)
  {
    gtk_widget_show(ppd_file);
    gtk_widget_show(ppd_button);
  }
  else
  {
    gtk_widget_hide(ppd_file);
    gtk_widget_hide(ppd_button);
  };
}


static void
ppd_browse_callback(void)
{
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(ppd_browser),
                                  gtk_entry_get_text(GTK_ENTRY(ppd_file)));
  gtk_widget_show(ppd_browser);
}


static void
ppd_ok_callback(void)
{
  gtk_widget_hide(ppd_browser);
  gtk_entry_set_text(GTK_ENTRY(ppd_file),
      gtk_file_selection_get_filename(GTK_FILE_SELECTION(ppd_browser)));
}


static void
ppd_cancel_callback(void)
{
  gtk_widget_hide(ppd_browser);
}


static void
file_ok_callback(void)
{
  gtk_widget_hide(file_browser);
  strcpy(vars.output_to,
         gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_browser)));

  runme = TRUE;

  gtk_widget_destroy(print_dialog);
}


static void
file_cancel_callback(void)
{
  gtk_widget_hide(file_browser);

  gtk_widget_destroy(print_dialog);
}


static void
preview_update(void)
{
  int		temp,		/* Swapping variable */
		orient,		/* True orientation of page */
		tw0, tw1,	/* Temporary page_widths */
		th0, th1,	/* Temporary page_heights */
		ta0, ta1;	/* Temporary areas */
  int		left, right,	/* Imageable area */
		top, bottom,
		width, length;	/* Physical width */
  static GdkGC	*gc = NULL;	/* Graphics context */
  printer_t	*p;		/* Current printer driver */


  if (preview->widget.window == NULL)
    return;

  gdk_window_clear(preview->widget.window);

  if (gc == NULL)
    gc = gdk_gc_new(preview->widget.window);

  p = printers + current_printer;

  (*p->imageable_area)(p->model, vars.ppd_file, vars.media_size, &left, &right,
                       &bottom, &top);

  page_width  = 10 * (right - left) / 72;
  page_height = 10 * (top - bottom) / 72;

  (*p->media_size)(p->model, vars.ppd_file, vars.media_size, &width, &length);

  width  = 10 * width / 72;
  length = 10 * length / 72;

  if (vars.scaling < 0)
  {
    tw0 = -image_width * 10 / vars.scaling;
    th0 = tw0 * image_height / image_width;
    tw1 = tw0;
    th1 = th0;
  }
  else
  {
    tw0 = page_width * vars.scaling / 100;
    th0 = tw0 * image_height / image_width;
    if (th0 > page_height)
    {
      th0 = page_height;
      tw0 = th0 * image_width / image_height;
    };
    ta0 = tw0 * th0;

    tw1 = page_height * vars.scaling / 100;
    th1 = tw1 * image_height / image_width;
    if (th1 > page_width)
    {
      th1 = page_width;
      tw1 = th1 * image_width / image_height;
    };
    ta1 = tw1 * th1;
  };

  if (vars.orientation == ORIENT_AUTO)
  {
    if (vars.scaling < 0)
    {
      if ((th0 > page_height && tw0 <= page_height) ||
          (tw0 > page_width && th0 <= page_width))
        orient = ORIENT_LANDSCAPE;
      else
        orient = ORIENT_PORTRAIT;
    }
    else
    {
      if (ta0 >= ta1)
	orient = ORIENT_PORTRAIT;
      else
	orient = ORIENT_LANDSCAPE;
    };
  }
  else
    orient = vars.orientation;

  if (orient == ORIENT_LANDSCAPE)
  {
    temp         = page_width;
    page_width   = page_height;
    page_height  = temp;
    temp         = width;
    width        = length;
    length       = temp;
    print_width  = tw1;
    print_height = th1;
  }
  else
  {
    print_width  = tw0;
    print_height = th0;
  };

  page_left = (PREVIEW_SIZE - page_width) / 2;
  page_top  = (PREVIEW_SIZE - page_height) / 2;

  gdk_draw_rectangle(preview->widget.window, gc, 0,
                     (PREVIEW_SIZE - width) / 2,
                     (PREVIEW_SIZE - length) / 2,
                     width, length);


  if (vars.left < 0)
    left = (page_width - print_width) / 2;
  else
  {
    left = 10 * vars.left / 72;

    if (left > (page_width - print_width))
    {
      left      = page_width - print_width;
      vars.left = 72 * left / 10;
    };
  };

  if (vars.top < 0)
    top = (page_height - print_height) / 2;
  else
  {
    top  = 10 * vars.top / 72;

    if (top > (page_height - print_height))
    {
      top      = page_height - print_height;
      vars.top = 72 * top / 10;
    };
  };

  gdk_draw_rectangle(preview->widget.window, gc, 1,
                     page_left + left, page_top + top,
                     print_width, print_height);

  gdk_flush();
}


static void
preview_button_callback(GtkWidget      *w,
                        GdkEventButton *event)
{
  mouse_x = event->x;
  mouse_y = event->y;
}


static void
preview_motion_callback(GtkWidget      *w,
                        GdkEventMotion *event)
{
  if (vars.left < 0 || vars.top < 0)
  {
    vars.left = 72 * (page_width - print_width) / 20;
    vars.top  = 72 * (page_height - print_height) / 20;
  };

  vars.left += 72 * (event->x - mouse_x) / 10;
  vars.top  += 72 * (event->y - mouse_y) / 10;

  if (vars.left < 0)
    vars.left = 0;

  if (vars.top < 0)
    vars.top = 0;

  preview_update();

  mouse_x = event->x;
  mouse_y = event->y;
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


 /*
  * Get the printer list...
  */

  get_printers();

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

    while (fgets(line, sizeof(line), fp) != NULL)
    {
      if (line[0] == '#')
        continue;	/* Comment */

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

      strncpy(key.command, lineptr, commaptr - lineptr);
      key.command[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.driver, lineptr, commaptr - lineptr);
      key.driver[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.ppd_file, lineptr, commaptr - lineptr);
      key.ppd_file[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      key.output_type = atoi(lineptr);
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.resolution, lineptr, commaptr - lineptr);
      key.resolution[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.media_size, lineptr, commaptr - lineptr);
      key.media_size[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      if ((commaptr = strchr(lineptr, ',')) == NULL)
        continue;	/* Skip bad printer definitions */

      strncpy(key.media_type, lineptr, commaptr - lineptr);
      key.media_type[commaptr - lineptr] = '\0';
      lineptr = commaptr + 1;

      strcpy(key.media_source, lineptr);
      key.media_source[strlen(key.media_source) - 1] = '\0';	/* Drop NL */

      if ((p = bsearch(&key, plist + 1, plist_count - 1, sizeof(plist_t),
                       (int (*)(const void *, const void *))compare_printers)) != NULL)
        memcpy(p, &key, sizeof(plist_t));
    };

    fclose(fp);
  }

  g_free (filename);

 /*
  * Select the current printer as necessary...
  */

  if (vars.output_to[0] != '\0')
  {
    for (i = 0; i < plist_count; i ++)
      if (strcmp(vars.output_to, plist[i].command) == 0)
        break;

    if (i < plist_count)
      plist_current = i;
  };
}


/*
 * 'printrc_save()' - Save the current printer resource configuration.
 */

static void
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

    for (i = 1, p = plist + 1; i < plist_count; i ++, p ++)
      fprintf(fp, "%s,%s,%s,%s,%d,%s,%s,%s,%s\n",
              p->name, p->command, p->driver, p->ppd_file, p->output_type,
              p->resolution, p->media_size, p->media_type, p->media_source);

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
 * 'get_printers()' - Get a complete list of printers from the spooler.
 */

static void
get_printers(void)
{
  int	i;
  FILE	*pfile;
  char	command[255],
	line[129],
	name[17],
	defname[17];
#ifdef __EMX__
  BYTE  pnum;
#endif


  defname[0] = '\0';

  memset(plist, 0, sizeof(plist));
  plist_count = 1;
  strcpy(plist[0].name, _("File"));
  plist[0].command[0] = '\0';
  strcpy(plist[0].driver, "ps2");
  plist[0].output_type = OUTPUT_COLOR;

#ifdef LPC_COMMAND
  if ((pfile = popen(LPC_COMMAND " status", "r")) != NULL)
  {
    while (fgets(line, sizeof(line), pfile) != NULL &&
           plist_count < MAX_PLIST)
      if (strchr(line, ':') != NULL)
      {
        *strchr(line, ':') = '\0';
        strcpy(plist[plist_count].name, line);
        sprintf(plist[plist_count].command, LPR_COMMAND " -P%s -l", line);
        strcpy(plist[plist_count].driver, "ps2");
        plist[plist_count].output_type = OUTPUT_COLOR;
        plist_count ++;
      };

    pclose(pfile);
  };
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
	sprintf(plist[plist_count].command, LP_COMMAND " -s -d%s", name);
        strcpy(plist[plist_count].driver, "ps2");
        plist[plist_count].output_type = OUTPUT_COLOR;
        plist_count ++;
      }
      else
        sscanf(line, "system default destination: %s", defname);
    };

    pclose(pfile);
  };
#endif /* LPSTAT_COMMAND */

#ifdef __EMX__
  if (DosDevConfig(&pnum, DEVINFO_PRINTER) == NO_ERROR)
    {
      for (i = 1; i <= pnum; i++)
	{
	  sprintf(plist[plist_count].name, "LPT%d:", i);
	  sprintf(plist[plist_count].command, "PRINT /D:LPT%d /B ", i);
          strcpy(plist[plist_count].driver, "ps2");
          plist[plist_count].output_type = OUTPUT_COLOR;
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
  };
}


/*
 * End of "$Id$".
 */
