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

#include "print.h"

#include <math.h>
#include <signal.h>
#ifdef __EMX__
#define INCL_DOSDEVICES
#define INCL_DOSERRORS
#include <os2.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/stdplugins-intl.h>

#define PLUG_IN_VERSION		"3.0.5 - 13 Jan 2000"
#define PLUG_IN_NAME		"Print"


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

/* Concrete type to represent image */
typedef struct
{
  GDrawable *drawable;
  GPixelRgn rgn;
} Gimp_Image_t;


/*
 * Local functions...
 */

static void	printrc_load(void);
static void	printrc_save(void);
static int	compare_printers(plist_t *p1, plist_t *p2);
static void	get_printers(void);

static void	query(void);
static void	run(char *, int, GParam *, int *, GParam **);
static void     init_gtk (void);
static int	do_print_dialog(void);
static void	brightness_update(GtkAdjustment *);
static void	brightness_callback(GtkWidget *);
static void	saturation_update(GtkAdjustment *);
static void	saturation_callback(GtkWidget *);
static void	density_update(GtkAdjustment *);
static void	density_callback(GtkWidget *);
static void	contrast_update(GtkAdjustment *);
static void	contrast_callback(GtkWidget *);
static void	red_update(GtkAdjustment *);
static void	red_callback(GtkWidget *);
static void	green_update(GtkAdjustment *);
static void	green_callback(GtkWidget *);
static void	blue_update(GtkAdjustment *);
static void	blue_callback(GtkWidget *);
static void	gamma_update(GtkAdjustment *);
static void	gamma_callback(GtkWidget *);
static void	scaling_update(GtkAdjustment *);
static void	scaling_callback(GtkWidget *);
static void	plist_callback(GtkWidget *, gint);
static void	media_size_callback(GtkWidget *, gint);
static void	media_type_callback(GtkWidget *, gint);
static void	media_source_callback(GtkWidget *, gint);
static void	resolution_callback(GtkWidget *, gint);
static void	output_type_callback(GtkWidget *, gint);
static void	linear_callback(GtkWidget *, gint);
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
	1.0			/* Density */
};

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
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Photo 700"),	"escp2-700",	1,	6,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Photo EX"),	"escp2-ex",	1,	7,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { N_("EPSON Stylus Photo EX"),	"escp2-photo",	1,	8,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
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
		*saturation_scale,	/* Scale for saturation */
		*saturation_entry,	/* Text entry widget for saturation */
		*density_scale,		/* Scale for density */
		*density_entry,		/* Text entry widget for density */
		*contrast_scale,	/* Scale for contrast */
		*contrast_entry,	/* Text entry widget for contrast */
		*red_scale,		/* Scale for red */
		*red_entry,		/* Text entry widget for red */
		*green_scale,		/* Scale for green */
		*green_entry,		/* Text entry widget for green */
		*blue_scale,		/* Scale for blue */
		*blue_entry,		/* Text entry widget for blue */
		*gamma_scale,		/* Scale for gamma */
		*gamma_entry,		/* Text entry widget for gamma */
		*output_gray,		/* Output type toggle, black */
		*output_color,		/* Output type toggle, color */
		*linear_on,		/* Linear toggle, on */
		*linear_off,		/* Linear toggle, off */
		*setup_dialog,		/* Setup dialog window */
		*printer_driver,	/* Printer driver widget */
		*ppd_file,		/* PPD file entry */
		*ppd_button,		/* PPD file browse button */
		*output_cmd,		/* Output command text entry */
		*ppd_browser,		/* File selection dialog for PPD files */
		*file_browser;		/* FSD for print files */

GtkObject	*scaling_adjustment,	/* Adjustment object for scaling */
		*brightness_adjustment,	/* Adjustment object for brightness */
		*saturation_adjustment,	/* Adjustment object for saturation */
		*density_adjustment,	/* Adjustment object for density */
		*contrast_adjustment,	/* Adjustment object for contrast */
		*red_adjustment,	/* Adjustment object for red */
		*green_adjustment,	/* Adjustment object for green */
		*blue_adjustment,	/* Adjustment object for blue */
		*gamma_adjustment;	/* Adjustment object for gamma */

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
    { PARAM_INT32,	"contrast",	"Top offset (points, -1 = centered)" },
    { PARAM_INT32,	"red",		"Top offset (points, -1 = centered)" },
    { PARAM_INT32,	"green",	"Top offset (points, -1 = centered)" },
    { PARAM_INT32,	"blue",		"Top offset (points, -1 = centered)" },
    { PARAM_INT32,	"linear",	"Linear output (0 = normal, 1 = linear)" },
    { PARAM_FLOAT,	"saturation",	"Saturation (0-1000%)" },
    { PARAM_FLOAT,	"density",	"Density (0-200%)" },
  };
  static int		nargs = sizeof(args) / sizeof(args[0]);

  INIT_I18N();

  gimp_install_procedure(
      "file_print",
      _("This plug-in prints images from The GIMP."),
      _("Prints images to PostScript, PCL, or ESC/P2 printers."),
      "Michael Sweet <mike@easysw.com> and Robert Krawitz <rlk@alum.mit.edu>",
      "Copyright 1997-1999 by Michael Sweet and Robert Krawitz",
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
  printer_t	*printer;	/* Printer driver entry */
  int		i;		/* Looping var */
  lut_t		lut;		/* 16-bit lookup table for brightness */
  guchar	*cmap;		/* Colormap (indexed images only) */
  int		ncolors;	/* Number of colors in colormap */
  GParam	*values;	/* Return values */
#ifdef __EMX__
  char		*tmpfile;	/* temp filename */
#endif
  gint32         image_ID;      /* image ID */
  gint32         drawable_ID;   /* drawable ID */
  GimpExportReturnType export = EXPORT_CANCEL;    /* return value of gimp_export_image() */

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

  image_ID = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  /*  eventually export the image */ 
  switch (run_mode)
    {
    case RUN_INTERACTIVE:
    case RUN_WITH_LAST_VALS:
      init_gtk ();
      export = gimp_export_image (&image_ID, &drawable_ID, "Print", 
				  (CAN_HANDLE_RGB | CAN_HANDLE_GRAY | CAN_HANDLE_INDEXED |
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

        for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
          if (strcmp(printers[i].driver, vars.driver) == 0)
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
            vars.gamma = 0.0;

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

        for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
          if (strcmp(printers[i].driver, vars.driver) == 0)
            current_printer = i;
        break;

    case RUN_WITH_LAST_VALS :
       /*
        * Possibly retrieve data...
        */

	gimp_get_data(PLUG_IN_NAME, &vars);

        for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
          if (strcmp(printers[i].driver, vars.driver) == 0)
            current_printer = i;
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
	printer      = printers + current_printer;
	vars.density *= printer->density;
	compute_lut(&lut, printer->gamma, gimp_gamma(), &vars);
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

	(*printer->print)(printer->model, 1, prn, &image, cmap, &lut, &vars);

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

static void 
init_gtk ()
{
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("print");
  
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
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
		*hbbox,	        /* button_box for OK/Cancel buttons */
		*button,	/* OK/Cancel buttons */
		*scale,		/* Scale widget */
		*entry,		/* Text entry widget */
		*menu,		/* Menu of drivers/sizes */
		*item,		/* Menu item */
		*option,	/* Option menu button */
		*box;		/* Box container */
  GtkObject	*scale_data;	/* Scale data (limits) */
  GSList	*group;		/* Grouping for output type */
  GSList	*linear_group;	/* Grouping for linear scale */
  static char	*orients[] =	/* Orientation strings */
  {
    N_("Auto"),
    N_("Portrait"),
    N_("Landscape")
  };
  gchar *plug_in_name;


 /*
  * Initialize the program's display...
  */
  init_gtk ();
  gdk_set_use_xshm (gimp_use_xshm());

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

  plug_in_name = g_strdup_printf (_("Print v%s"), PLUG_IN_VERSION);
  gtk_window_set_title(GTK_WINDOW(dialog), plug_in_name);
  g_free (plug_in_name);
  
  gtk_window_set_wmclass(GTK_WINDOW(dialog), "print", "Gimp");
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width(GTK_CONTAINER(dialog), 0);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		     (GtkSignalFunc)close_callback, NULL);

 /*
  * Top-level table for dialog...
  */

  table = gtk_table_new(17, 4, FALSE);
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
  }

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

  label = gtk_label_new(_("Density:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 7, 8, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 3, 4, 7, 8, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  linear_off = button = gtk_radio_button_new_with_label(NULL, _("Normal scale"));
  linear_group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
  if (vars.linear == 0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     (GtkSignalFunc)linear_callback,
		     (gpointer)0);
  gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  linear_on = button = gtk_radio_button_new_with_label(linear_group, _("Experimental linear scale"));
  if (vars.linear == 1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     (GtkSignalFunc)linear_callback,
		     (gpointer)1);
  gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

 /*
  * Scaling...
  */

  label = gtk_label_new(_("Scaling:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 8, 9, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 8, 9, GTK_FILL, GTK_FILL, 0, 0);
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
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 9, 10, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 9, 10, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  brightness_adjustment = scale_data =
      gtk_adjustment_new((float)vars.brightness, 0.0, 401.0, 1.0, 1.0, 1.0);

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
  * Gamma slider...
  */

  label = gtk_label_new(_("Gamma:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 10, 11, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 10, 11, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  gamma_adjustment = scale_data =
      gtk_adjustment_new((float)vars.gamma, 0.1, 4.0, 0.001, 0.01, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc)gamma_update, NULL);

  gamma_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
  gtk_widget_set_usize(scale, 200, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

  gamma_entry = entry = gtk_entry_new();
  sprintf(s, "%5.3f", vars.gamma);
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)gamma_callback, NULL);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
  gtk_widget_set_usize(entry, 40, 0);
  gtk_widget_show(entry);


 /*
  * Contrast slider...
  */

  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);

  label = gtk_label_new(_("Contrast:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 11, 12, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);
  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 11, 12, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  contrast_adjustment = scale_data =
      gtk_adjustment_new((float)vars.contrast, 25.0, 401.0, 1.0, 1.0, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc)contrast_update, NULL);

  contrast_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
  gtk_widget_set_usize(scale, 200, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

  contrast_entry = entry = gtk_entry_new();
  sprintf(s, "%d", vars.contrast);
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)contrast_callback, NULL);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
  gtk_widget_set_usize(entry, 40, 0);
  gtk_widget_show(entry);

 /*
  * Red slider...
  */

  label = gtk_label_new(_("Red:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 12, 13, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 12, 13, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  red_adjustment = scale_data =
      gtk_adjustment_new((float)vars.red, 0.0, 201.0, 1.0, 1.0, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc)red_update, NULL);

  red_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
  gtk_widget_set_usize(scale, 200, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

  red_entry = entry = gtk_entry_new();
  sprintf(s, "%d", vars.red);
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)red_callback, NULL);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
  gtk_widget_set_usize(entry, 40, 0);
  gtk_widget_show(entry);

 /*
  * Green slider...
  */

  label = gtk_label_new(_("Green:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 13, 14, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 13, 14, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  green_adjustment = scale_data =
      gtk_adjustment_new((float)vars.green, 0.0, 201.0, 1.0, 1.0, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc)green_update, NULL);

  green_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
  gtk_widget_set_usize(scale, 200, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

  green_entry = entry = gtk_entry_new();
  sprintf(s, "%d", vars.green);
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)green_callback, NULL);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
  gtk_widget_set_usize(entry, 40, 0);
  gtk_widget_show(entry);

 /*
  * Blue slider...
  */

  label = gtk_label_new(_("Blue:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 14, 15, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 14, 15, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  blue_adjustment = scale_data =
      gtk_adjustment_new((float)vars.blue, 0.0, 201.0, 1.0, 1.0, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc)blue_update, NULL);

  blue_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
  gtk_widget_set_usize(scale, 200, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

  blue_entry = entry = gtk_entry_new();
  sprintf(s, "%d", vars.blue);
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)blue_callback, NULL);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
  gtk_widget_set_usize(entry, 40, 0);
  gtk_widget_show(entry);

 /*
  * Saturation slider...
  */

  label = gtk_label_new(_("Saturation:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 15, 16, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 15, 16, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  saturation_adjustment = scale_data =
      gtk_adjustment_new((float)vars.saturation, 0.1, 10.0, 0.001, 0.01, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc)saturation_update, NULL);

  saturation_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
  gtk_widget_set_usize(scale, 200, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

  saturation_entry = entry = gtk_entry_new();
  sprintf(s, "%5.3f", vars.saturation);
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)saturation_callback, NULL);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
  gtk_widget_set_usize(entry, 40, 0);
  gtk_widget_show(entry);

 /*
  * Density slider...
  */

  label = gtk_label_new(_("Density:"));
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 16, 17, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  box = gtk_hbox_new(FALSE, 8);
  gtk_table_attach(GTK_TABLE(table), box, 1, 4, 16, 17, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(box);

  density_adjustment = scale_data =
      gtk_adjustment_new((float)vars.density, 0.1, 3.0, 0.001, 0.01, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc)density_update, NULL);

  density_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
  gtk_widget_set_usize(scale, 200, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

  density_entry = entry = gtk_entry_new();
  sprintf(s, "%5.3f", vars.density);
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)density_callback, NULL);
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
    if (plist[i].active)
      item = gtk_menu_item_new_with_label(gettext(plist[i].name));
    else
      {
	char buf[18];
	buf[0] = '*';
	memcpy(buf + 1, plist[i].name, 17);
	item = gtk_menu_item_new_with_label(gettext(buf));
      }
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       (GtkSignalFunc)plist_callback,
		       (gpointer)i);
    gtk_widget_show(item);
  }

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

  button = gtk_button_new_with_label (_("Print"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) print_callback,
		      NULL);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) cancel_callback, NULL);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

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
  }

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

  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) setup_ok_callback, NULL);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)setup_cancel_callback, NULL);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);


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
    plist[plist_current].v.brightness = adjustment->value;

    sprintf(s, "%d", vars.brightness);

    gtk_signal_handler_block_by_data(GTK_OBJECT(brightness_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(brightness_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(brightness_entry), NULL);
  }
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
    }
  }
}


/*
 * 'contrast_update()' - Update the contrast field using the scale.
 */

static void
contrast_update(GtkAdjustment *adjustment)	/* I - New value */
{
  char	s[255];					/* Text buffer */


  if (vars.contrast != adjustment->value)
  {
    vars.contrast = adjustment->value;
    plist[plist_current].v.contrast = adjustment->value;

    sprintf(s, "%d", vars.contrast);

    gtk_signal_handler_block_by_data(GTK_OBJECT(contrast_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(contrast_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(contrast_entry), NULL);
  }
}


/*
 * 'contrast_callback()' - Update the contrast scale using the text entry.
 */

static void
contrast_callback(GtkWidget *widget)	/* I - Entry widget */
{
  gint		new_value;		/* New scaling value */


  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (vars.contrast != new_value)
  {
    if ((new_value >= GTK_ADJUSTMENT(contrast_adjustment)->lower) &&
	(new_value < GTK_ADJUSTMENT(contrast_adjustment)->upper))
    {
      GTK_ADJUSTMENT(contrast_adjustment)->value = new_value;

      gtk_signal_emit_by_name(contrast_adjustment, "value_changed");
    }
  }
}


/*
 * 'red_update()' - Update the red field using the scale.
 */

static void
red_update(GtkAdjustment *adjustment)	/* I - New value */
{
  char	s[255];					/* Text buffer */


  if (vars.red != adjustment->value)
  {
    vars.red = adjustment->value;
    plist[plist_current].v.red = adjustment->value;

    sprintf(s, "%d", vars.red);

    gtk_signal_handler_block_by_data(GTK_OBJECT(red_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(red_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(red_entry), NULL);
  }
}


/*
 * 'red_callback()' - Update the red scale using the text entry.
 */

static void
red_callback(GtkWidget *widget)	/* I - Entry widget */
{
  gint		new_value;		/* New scaling value */


  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (vars.red != new_value)
  {
    if ((new_value >= GTK_ADJUSTMENT(red_adjustment)->lower) &&
	(new_value < GTK_ADJUSTMENT(red_adjustment)->upper))
    {
      GTK_ADJUSTMENT(red_adjustment)->value = new_value;

      gtk_signal_emit_by_name(red_adjustment, "value_changed");
    }
  }
}


/*
 * 'green_update()' - Update the green field using the scale.
 */

static void
green_update(GtkAdjustment *adjustment)	/* I - New value */
{
  char	s[255];					/* Text buffer */


  if (vars.green != adjustment->value)
  {
    vars.green = adjustment->value;
    plist[plist_current].v.green = adjustment->value;

    sprintf(s, "%d", vars.green);

    gtk_signal_handler_block_by_data(GTK_OBJECT(green_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(green_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(green_entry), NULL);
  }
}


/*
 * 'green_callback()' - Update the green scale using the text entry.
 */

static void
green_callback(GtkWidget *widget)	/* I - Entry widget */
{
  gint		new_value;		/* New scaling value */


  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (vars.green != new_value)
  {
    if ((new_value >= GTK_ADJUSTMENT(green_adjustment)->lower) &&
	(new_value < GTK_ADJUSTMENT(green_adjustment)->upper))
    {
      GTK_ADJUSTMENT(green_adjustment)->value = new_value;

      gtk_signal_emit_by_name(green_adjustment, "value_changed");
    }
  }
}


/*
 * 'blue_update()' - Update the blue field using the scale.
 */

static void
blue_update(GtkAdjustment *adjustment)	/* I - New value */
{
  char	s[255];					/* Text buffer */


  if (vars.blue != adjustment->value)
  {
    vars.blue = adjustment->value;
    plist[plist_current].v.blue = adjustment->value;

    sprintf(s, "%d", vars.blue);

    gtk_signal_handler_block_by_data(GTK_OBJECT(blue_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(blue_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(blue_entry), NULL);
  }
}


/*
 * 'blue_callback()' - Update the blue scale using the text entry.
 */

static void
blue_callback(GtkWidget *widget)	/* I - Entry widget */
{
  gint		new_value;		/* New scaling value */


  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (vars.blue != new_value)
  {
    if ((new_value >= GTK_ADJUSTMENT(blue_adjustment)->lower) &&
	(new_value < GTK_ADJUSTMENT(blue_adjustment)->upper))
    {
      GTK_ADJUSTMENT(blue_adjustment)->value = new_value;

      gtk_signal_emit_by_name(blue_adjustment, "value_changed");
    }
  }
}


/*
 * 'gamma_update()' - Update the gamma field using the scale.
 */

static void
gamma_update(GtkAdjustment *adjustment)	/* I - New value */
{
  char	s[255];					/* Text buffer */


  if (vars.gamma != adjustment->value)
  {
    vars.gamma = adjustment->value;
    plist[plist_current].v.gamma = adjustment->value;

    sprintf(s, "%5.3f", vars.gamma);

    gtk_signal_handler_block_by_data(GTK_OBJECT(gamma_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(gamma_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(gamma_entry), NULL);

  }
}


/*
 * 'gamma_callback()' - Update the gamma scale using the text entry.
 */

static void
gamma_callback(GtkWidget *widget)	/* I - Entry widget */
{
  gint		new_value;		/* New scaling value */


  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (vars.gamma != new_value)
  {
    if ((new_value >= GTK_ADJUSTMENT(gamma_adjustment)->lower) &&
	(new_value < GTK_ADJUSTMENT(gamma_adjustment)->upper))
    {
      GTK_ADJUSTMENT(gamma_adjustment)->value = new_value;

      gtk_signal_emit_by_name(gamma_adjustment, "value_changed");
    }
  }
}



/*
 * 'saturation_update()' - Update the saturation field using the scale.
 */

static void
saturation_update(GtkAdjustment *adjustment)	/* I - New value */
{
  char	s[255];					/* Text buffer */


  if (vars.saturation != adjustment->value)
  {
    vars.saturation = adjustment->value;
    plist[plist_current].v.saturation = adjustment->value;

    sprintf(s, "%5.3f", vars.saturation);

    gtk_signal_handler_block_by_data(GTK_OBJECT(saturation_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(saturation_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(saturation_entry), NULL);
  }
}


/*
 * 'saturation_callback()' - Update the saturation scale using the text entry.
 */

static void
saturation_callback(GtkWidget *widget)	/* I - Entry widget */
{
  gint		new_value;		/* New scaling value */


  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (vars.saturation != new_value)
  {
    if ((new_value >= GTK_ADJUSTMENT(saturation_adjustment)->lower) &&
	(new_value < GTK_ADJUSTMENT(saturation_adjustment)->upper))
    {
      GTK_ADJUSTMENT(saturation_adjustment)->value = new_value;

      gtk_signal_emit_by_name(saturation_adjustment, "value_changed");
    }
  }
}

/*
 * 'density_update()' - Update the density field using the scale.
 */

static void
density_update(GtkAdjustment *adjustment)	/* I - New value */
{
  char	s[255];					/* Text buffer */


  if (vars.density != adjustment->value)
  {
    vars.density = adjustment->value;
    plist[plist_current].v.density = adjustment->value;

    sprintf(s, "%4.3f", vars.density);

    gtk_signal_handler_block_by_data(GTK_OBJECT(density_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(density_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(density_entry), NULL);
  }
}


/*
 * 'density_callback()' - Update the density scale using the text entry.
 */

static void
density_callback(GtkWidget *widget)	/* I - Entry widget */
{
  gint		new_value;		/* New scaling value */


  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (vars.density != new_value)
  {
    if ((new_value >= GTK_ADJUSTMENT(density_adjustment)->lower) &&
	(new_value < GTK_ADJUSTMENT(density_adjustment)->upper))
    {
      GTK_ADJUSTMENT(density_adjustment)->value = new_value;

      gtk_signal_emit_by_name(density_adjustment, "value_changed");
    }
  }
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
    plist[plist_current].v.scaling = vars.scaling;

    sprintf(s, "%.1f", adjustment->value);

    gtk_signal_handler_block_by_data(GTK_OBJECT(scaling_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(scaling_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(scaling_entry), NULL);

    preview_update();
  }
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
      }
    }
  }
  else if (widget == scaling_ppi)
  {
    GTK_ADJUSTMENT(scaling_adjustment)->lower = 50.0;
    GTK_ADJUSTMENT(scaling_adjustment)->upper = 1201.0;
    GTK_ADJUSTMENT(scaling_adjustment)->value = 72.0;
    vars.scaling = 0.0;
    plist[plist_current].v.scaling = vars.scaling;
    gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
  }
  else if (widget == scaling_percent)
  {
    GTK_ADJUSTMENT(scaling_adjustment)->lower = 5.0;
    GTK_ADJUSTMENT(scaling_adjustment)->upper = 101.0;
    GTK_ADJUSTMENT(scaling_adjustment)->value = 100.0;
    vars.scaling = 0.0;
    plist[plist_current].v.scaling = vars.scaling;
    gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
  }
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
		*item0 = 0;	/* First menu item */


  if (*menu != NULL)
  {
    gtk_widget_destroy(*menu);
    *menu = NULL;
  }

  if (num_items == 0)
  {
    gtk_widget_hide(option);
    return;
  }

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
  }

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
    }
  }

  if (i == num_items)
  {
    gtk_option_menu_set_history(GTK_OPTION_MENU(option), 0);
    gtk_signal_emit_by_name(GTK_OBJECT(item0), "activate");
  }

  gtk_widget_show(option);
}


static void
do_misc_updates()
{
  char s[255];
  vars.scaling = plist[plist_current].v.scaling;
  vars.orientation = plist[plist_current].v.orientation;
  vars.left = plist[plist_current].v.left;
  vars.top = plist[plist_current].v.top;

  if (plist[plist_current].v.scaling < 0)
    {
      float tmp = -plist[plist_current].v.scaling;
      plist[plist_current].v.scaling = -plist[plist_current].v.scaling;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scaling_ppi), TRUE);
      GTK_ADJUSTMENT(scaling_adjustment)->lower = 50.0;
      GTK_ADJUSTMENT(scaling_adjustment)->upper = 1201.0;
      sprintf(s, "%.1f", tmp);
      GTK_ADJUSTMENT(scaling_adjustment)->value = tmp;
      gtk_signal_handler_block_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_entry_set_text(GTK_ENTRY(scaling_entry), s);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
    }
  else
    {
      float tmp = plist[plist_current].v.scaling;
      GTK_ADJUSTMENT(scaling_adjustment)->lower = 5.0;
      GTK_ADJUSTMENT(scaling_adjustment)->upper = 101.0;
      sprintf(s, "%.1f", tmp);
      GTK_ADJUSTMENT(scaling_adjustment)->value = tmp;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scaling_percent), TRUE);
      gtk_signal_handler_block_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_entry_set_text(GTK_ENTRY(scaling_entry), s);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
    }    

  GTK_ADJUSTMENT(brightness_adjustment)->value = plist[plist_current].v.brightness;
  gtk_signal_emit_by_name(brightness_adjustment, "value_changed");

  GTK_ADJUSTMENT(gamma_adjustment)->value = plist[plist_current].v.gamma;
  gtk_signal_emit_by_name(gamma_adjustment, "value_changed");

  GTK_ADJUSTMENT(contrast_adjustment)->value = plist[plist_current].v.contrast;
  gtk_signal_emit_by_name(contrast_adjustment, "value_changed");

  GTK_ADJUSTMENT(red_adjustment)->value = plist[plist_current].v.red;
  gtk_signal_emit_by_name(red_adjustment, "value_changed");

  GTK_ADJUSTMENT(green_adjustment)->value = plist[plist_current].v.green;
  gtk_signal_emit_by_name(green_adjustment, "value_changed");

  GTK_ADJUSTMENT(blue_adjustment)->value = plist[plist_current].v.blue;
  gtk_signal_emit_by_name(blue_adjustment, "value_changed");

  GTK_ADJUSTMENT(saturation_adjustment)->value = plist[plist_current].v.saturation;
  gtk_signal_emit_by_name(saturation_adjustment, "value_changed");

  GTK_ADJUSTMENT(density_adjustment)->value = plist[plist_current].v.density;
  gtk_signal_emit_by_name(density_adjustment, "value_changed");

  if (plist[plist_current].v.output_type == OUTPUT_GRAY)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_gray), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_color), TRUE);

  if (plist[plist_current].v.linear == 0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linear_off), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linear_off), TRUE);

  preview_update();
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

  if (p->v.driver[0] != '\0')
  {
    strcpy(vars.driver, p->v.driver);

    for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
      if (strcmp(printers[i].driver, vars.driver) == 0)
      {
        current_printer = i;
        break;
      }
  }

  strcpy(vars.ppd_file, p->v.ppd_file);
  strcpy(vars.media_size, p->v.media_size);
  strcpy(vars.media_type, p->v.media_type);
  strcpy(vars.media_source, p->v.media_source);
  strcpy(vars.resolution, p->v.resolution);
  strcpy(vars.output_to, p->v.output_to);
  do_misc_updates();

 /*
  * Now get option parameters...
  */

  printer = printers + current_printer;

  if (num_media_sizes > 0)
  {
    for (i = 0; i < num_media_sizes; i ++)
      free(media_sizes[i]);
    free(media_sizes);
  }

  media_sizes = (*(printer->parameters))(printer->model,
                                         p->v.ppd_file,
                                         "PageSize", &num_media_sizes);
  if (vars.media_size[0] == '\0')
    strcpy(vars.media_size, media_sizes[0]);
  plist_build_menu(media_size, &media_size_menu, num_media_sizes, media_sizes,
                   p->v.media_size, media_size_callback);

  if (num_media_types > 0)
  {
    for (i = 0; i < num_media_types; i ++)
      free(media_types[i]);
    free(media_types);
  }

  media_types = (*(printer->parameters))(printer->model,
                                         p->v.ppd_file,
                                         "MediaType", &num_media_types);
  if (vars.media_type[0] == '\0' && media_types != NULL)
    strcpy(vars.media_type, media_types[0]);
  plist_build_menu(media_type, &media_type_menu, num_media_types, media_types,
                   p->v.media_type, media_type_callback);

  if (num_media_sources > 0)
  {
    for (i = 0; i < num_media_sources; i ++)
      free(media_sources[i]);
    free(media_sources);
  }

  media_sources = (*(printer->parameters))(printer->model,
                                           p->v.ppd_file,
                                           "InputSlot", &num_media_sources);
  if (vars.media_source[0] == '\0' && media_sources != NULL)
    strcpy(vars.media_source, media_sources[0]);
  plist_build_menu(media_source, &media_source_menu, num_media_sources,
		   media_sources, p->v.media_source, media_source_callback);

  if (num_resolutions > 0)
  {
    for (i = 0; i < num_resolutions; i ++)
      free(resolutions[i]);
    free(resolutions);
  }

  resolutions = (*(printer->parameters))(printer->model,
                                         p->v.ppd_file,
                                         "Resolution", &num_resolutions);
  if (vars.resolution[0] == '\0' && resolutions != NULL)
    strcpy(vars.resolution, resolutions[0]);
  plist_build_menu(resolution, &resolution_menu, num_resolutions, resolutions,
                   p->v.resolution, resolution_callback);
}


/*
 * 'media_size_callback()' - Update the current media size...
 */

static void
media_size_callback(GtkWidget *widget,		/* I - Media size option menu */
                    gint      data)		/* I - Data */
{
  strcpy(vars.media_size, media_sizes[data]);
  strcpy(plist[plist_current].v.media_size, media_sizes[data]);
  vars.left       = -1;
  vars.top        = -1;
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;

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
  strcpy(plist[plist_current].v.media_type, media_types[data]);
}


/*
 * 'media_source_callback()' - Update the current media source...
 */

static void
media_source_callback(GtkWidget *widget,	/* I - Media source option menu */
                      gint      data)		/* I - Data */
{
  strcpy(vars.media_source, media_sources[data]);
  strcpy(plist[plist_current].v.media_source, media_sources[data]);
}


/*
 * 'resolution_callback()' - Update the current resolution...
 */

static void
resolution_callback(GtkWidget *widget,		/* I - Media size option menu */
                    gint      data)		/* I - Data */
{
  strcpy(vars.resolution, resolutions[data]);
  strcpy(plist[plist_current].v.resolution, resolutions[data]);
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
  plist[plist_current].v.orientation = vars.orientation;
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;

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
    plist[plist_current].v.output_type = data;
  }
}

/*
 * 'linear_callback()' - Update the current linear gradient mode...
 */

static void
linear_callback(GtkWidget *widget,	/* I - Output type button */
		gint      data)	/* I - Data */
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
  {
    vars.linear = data;
    plist[plist_current].v.linear = data;
  }
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
    if (strcmp(plist[plist_current].v.driver, printers[i].driver) == 0)
    {
      current_printer = i;
      break;
    }

  gtk_option_menu_set_history(GTK_OPTION_MENU(printer_driver), current_printer);

  gtk_entry_set_text(GTK_ENTRY(ppd_file), plist[plist_current].v.ppd_file);

  if (strncmp(plist[plist_current].v.driver, "ps", 2) == 0)
    gtk_widget_show(ppd_file);
  else
    gtk_widget_show(ppd_file);

  gtk_entry_set_text(GTK_ENTRY(output_cmd), plist[plist_current].v.output_to);

  if (plist_current == 0)
    gtk_widget_hide(output_cmd);
  else
    gtk_widget_show(output_cmd);

  gtk_widget_show(setup_dialog);
}


static void
setup_ok_callback(void)
{
  strcpy(vars.driver, printers[current_printer].driver);
  strcpy(plist[plist_current].v.driver, printers[current_printer].driver);

  strcpy(vars.output_to, gtk_entry_get_text(GTK_ENTRY(output_cmd)));
  strcpy(plist[plist_current].v.output_to, vars.output_to);

  strcpy(vars.ppd_file, gtk_entry_get_text(GTK_ENTRY(ppd_file)));
  strcpy(plist[plist_current].v.ppd_file, vars.ppd_file);

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

  if (strncmp(printers[current_printer].driver, "ps", 2) == 0)
  {
    gtk_widget_show(ppd_file);
    gtk_widget_show(ppd_button);
  }
  else
  {
    gtk_widget_hide(ppd_file);
    gtk_widget_hide(ppd_button);
  }
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
		ta0 = 0, ta1 = 0;	/* Temporary areas */
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
    }
    ta0 = tw0 * th0;

    tw1 = page_height * vars.scaling / 100;
    th1 = tw1 * image_height / image_width;
    if (th1 > page_width)
    {
      th1 = page_width;
      tw1 = th1 * image_width / image_height;
    }
    ta1 = tw1 * th1;
  }

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
    }
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
  }

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
      plist[plist_current].v.left = vars.left;
    }
  }

  if (vars.top < 0)
    top = (page_height - print_height) / 2;
  else
  {
    top  = 10 * vars.top / 72;

    if (top > (page_height - print_height))
    {
      top      = page_height - print_height;
      vars.top = 72 * top / 10;
      plist[plist_current].v.top = vars.top;
    }
  }

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
  }

  vars.left += 72 * (event->x - mouse_x) / 10;
  vars.top  += 72 * (event->y - mouse_y) / 10;

  if (vars.left < 0)
    vars.left = 0;

  if (vars.top < 0)
    vars.top = 0;
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;

  preview_update();

  mouse_x = event->x;
  mouse_y = event->y;
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

    (void) memset(line, 0, 1024);
    while (fgets(line, sizeof(line), fp) != NULL)
    {
      int keepgoing = 1;
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
	  
      if ((p = bsearch(&key, plist + 1, plist_count - 1, sizeof(plist_t),
                       (int (*)(const void *, const void *))compare_printers)) != NULL)
	{
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
 * 'get_printers()' - Get a complete list of printers from the spooler.
 */

static void
get_printers(void)
{
  int	i;
  FILE	*pfile;
  char	line[129],
	defname[17];
#ifdef __EMX__
  BYTE  pnum;
#endif


  defname[0] = '\0';

  memset(plist, 0, sizeof(plist));
  plist_count = 1;
  strcpy(plist[0].name, _("File"));
  plist[0].v.output_to[0] = '\0';
  strcpy(plist[0].v.driver, "ps2");
  plist[0].v.output_type = OUTPUT_COLOR;

#ifdef LPC_COMMAND
  if ((pfile = popen(LPC_COMMAND " status", "r")) != NULL)
  {
    while (fgets(line, sizeof(line), pfile) != NULL &&
           plist_count < MAX_PLIST)
      if (strchr(line, ':') != NULL && line[0] != ' ' && line[0] != '\t')
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
    char name[17];	
	
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
