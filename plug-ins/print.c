/*
 * "$Id$"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997 Michael Sweet (mike@easysw.com)
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
 *   $Log$
 *   Revision 1.1  1997/11/24 22:03:35  sopwith
 *   Initial revision
 *
 *   Revision 1.3  1997/10/03 22:18:15  nobody
 *   updated print er too the latest version
 *
 *   : ----------------------------------------------------------------------
 *
 *   Revision 1.8  1997/10/02  17:57:26  mike
 *   Added printrc support.
 *   Added printer list (spooler support).
 *   Added gamma/dot gain correction values for all printers.
 *
 *   Revision 1.7  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.6  1997/07/30  18:47:39  mike
 *   Added scaling, orientation, and offset options.
 *   Added first cut at preview window.
 *
 *   Revision 1.5  1997/07/26  18:38:23  mike
 *   Whoops - wasn't grabbing the colormap for indexed images properly...
 *
 *   Revision 1.4  1997/07/03  13:13:26  mike
 *   Updated documentation for 1.0 release.
 *
 *   Revision 1.3  1997/07/03  13:07:05  mike
 *   Updated EPSON driver short names.
 *   Changed brightness lut formula for better control.
 *
 *   Revision 1.2  1997/07/02  15:22:17  mike
 *   Added GUI with printer/media/output selection controls.
 *
 *   Revision 1.1  1997/07/02  13:51:53  mike
 *   Initial revision
 */

#include "print.h"
#include <math.h>
#include <signal.h>


/*
 * Constants for GUI...
 */

#define SCALE_WIDTH		64
#define ENTRY_WIDTH		64
#define PREVIEW_SIZE		180	/* Assuming max media size of 18" */
#define MAX_PLIST		100


/*
 * Types...
 */

typedef struct		/**** Printer List ****/
{
  char	name[17],	/* Name of printer */
	command[255],	/* Printer command */
	driver[33];	/* Short name of printer driver */
  int	output_type;	/* Color/B&W */
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
static int	print_dialog(void);
static void	dialog_create_ivalue(char *, GtkTable *, int, gint *, int, int);
static void	dialog_iscale_update(GtkAdjustment *, gint *);
static void	dialog_ientry_update(GtkWidget *, gint *);
static void	plist_callback(GtkWidget *, gint);
static void	print_driver_callback(GtkWidget *, gint);
static void	media_size_callback(GtkWidget *, gint);
static void	print_command_callback(GtkWidget *, gpointer);
static void	output_type_callback(GtkWidget *, gint);
static void	orientation_callback(GtkWidget *, gint);
static void	print_callback(GtkWidget *, gpointer);
static void	cancel_callback(GtkWidget *, gpointer);
static void	close_callback(GtkWidget *, gpointer);
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
	short_name[33];		/* Name of printer "driver" */
  gint	media_size,		/* Size of output media */
	output_type,		/* Color or grayscale output */
	brightness,		/* Output brightness */
	scaling,		/* Scaling, percent of printable area */
	orientation,		/* Orientation - 0 = port., 1 = land., -1 = auto */
	left,			/* Offset from lower-lefthand corner, 10ths */
	top;			/* ... */
}		vars =
{
	"|lp",			/* Name of file or command to print to */
	"ps",			/* Name of printer "driver" */
	MEDIA_LETTER,		/* Size of output media */
	OUTPUT_COLOR,		/* Color or grayscale output */
	100,			/* Output brightness */
	100,			/* Scaling (100% means entire printable area */
	-1,			/* Orientation (-1 = automatic) */
	-1,			/* X offset (-1 = center) */
	-1			/* Y offset (-1 = center) */
};

GtkWidget	*printer_driver,	/* Printer driver widget */
		*output_gray,		/* Output type toggle, black */
		*output_color,		/* Output type toggle, color */
		*output_to;		/* Output file/command text entry */

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
  { "PostScript Printer",	"ps",		72,	72,	1,	1,	0,	1.000,	1.000,	ps_print },
  { "HP DeskJet 500, 520",	"pcl-500",	300,	300,	0,	0,	500,	0.541,	0.548,	pcl_print },
  { "HP DeskJet 500C, 540C",	"pcl-501",	300,	300,	0,	1,	501,	0.541,	0.548,	pcl_print },
  { "HP DeskJet 550C, 560C",	"pcl-550",	300,	300,	0,	1,	550,	0.541,	0.548,	pcl_print },
  { "HP DeskJet 600 series",	"pcl-600",	600,	300,	0,	1,	600,	0.541,	0.548,	pcl_print },
  { "HP DeskJet 800 series",	"pcl-800",	600,	600,	0,	1,	800,	0.541,	0.548,	pcl_print },
  { "HP DeskJet 1200C, 1600C",	"pcl-1200",	300,	300,	0,	1,	1200,	0.541,	0.548,	pcl_print },
  { "HP LaserJet II series",	"pcl-2",	300,	300,	0,	0,	2,	0.563,	0.596,	pcl_print },
  { "HP LaserJet III series",	"pcl-3",	300,	300,	0,	0,	3,	0.563,	0.596,	pcl_print },
  { "HP LaserJet 4 series",	"pcl-4",	600,	600,	1,	0,	4,	0.599,	0.615,	pcl_print },
  { "HP LaserJet 5 series",	"pcl-5",	600,	600,	1,	0,	4,	0.599,	0.615,	pcl_print },
  { "HP LaserJet 6 series",	"pcl-6",	600,	600,	1,	0,	4,	0.599,	0.615,	pcl_print },
  { "EPSON Stylus Color",	"escp2",	720,	720,	0,	1,	0,	0.325,	0.478,	escp2_print },
  { "EPSON Stylus Color Pro",	"escp2-pro",	720,	720,	0,	1,	1,	0.325,	0.478,	escp2_print },
  { "EPSON Stylus Color Pro XL","escp2-proxl",	720,	720,	1,	1,	1,	0.325,	0.478,	escp2_print },
  { "EPSON Stylus Color 1500",	"escp2-1500",	720,	720,	1,	1,	2,	0.325,	0.478,	escp2_print },
  { "EPSON Stylus Color 400",	"escp2-400",	720,	720,	0,	1,	1,	0.530,	0.482,	escp2_print },
  { "EPSON Stylus Color 500",	"escp2-500",	720,	720,	0,	1,	1,	0.530,	0.482,	escp2_print },
  { "EPSON Stylus Color 600",	"escp2-600",	720,	720,	0,	1,	3,	0.530,	0.482,	escp2_print },
  { "EPSON Stylus Color 800",	"escp2-800",	720,	720,	0,	1,	4,	0.530,	0.482,	escp2_print },
  { "EPSON Stylus Color 1520",	"escp2-1520",	720,	720,	1,	1,	4,	0.530,	0.482,	escp2_print },
  { "EPSON Stylus Color 3000",	"escp2-3000",	720,	720,	1,	1,	4,	0.530,	0.482,	escp2_print }
};


/*
 * 'main()' - Main entry - just call gimp_main()...
 */

int
main(int  argc,		/* I - Number of command-line args */
     char *argv[])	/* I - Command-line args */
{
  return (gimp_main(argc, argv));
}


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
    { PARAM_INT32,	"media_size",	"Media size (0 = A/letter, 1 = legal, 2 = B/tabloid, 3 = A4, 4 = A3)" },
    { PARAM_INT32,	"output_type",	"Output type (0 = gray, 1 = color)" },
    { PARAM_INT32,	"brightness",	"Brightness (0-200%)" },
    { PARAM_INT32,	"scaling",	"Output scaling (0-100%)" },
    { PARAM_INT32,	"orientation",	"Output orientation (-1 = auto, 0 = portrait, 1 = landscape)" },
    { PARAM_INT32,	"left",		"Left offset (10ths inches, -1 = centered)" },
    { PARAM_INT32,	"top",		"Top offset (10ths inches, -1 = centered)" }
  };
  static int		nargs = sizeof(args) / sizeof(args[0]);


  gimp_install_procedure(
      "file_print",
      "This plug-in prints images from The GIMP.",
      "Prints images to PostScript, PCL, or ESC/P2 printers.",
      "Michael Sweet <mike@easysw.com>",
      "Copyright 1997 by Michael Sweet",
      PLUG_IN_VERSION,
      "<Image>/File/Print...",
      "RGB*,GRAY*INDEXED*",
      PROC_PLUG_IN,
      nargs,
      0,
      args,
      NULL);
}


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
  int		i, j;		/* Looping vars */
  float		brightness;	/* Computed brightness */
  guchar	lut[256];	/* Lookup table for brightness */
  guchar	*cmap;		/* Colormap (indexed images only) */
  int		ncolors;	/* Number of colors in colormap */
  static GParam	values[1];	/* Return values */


 /*
  * Initialize parameter data...
  */

  run_mode = param[0].data.d_int32;

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

	if (!print_dialog())
          return;
        break;

    case RUN_NONINTERACTIVE :
       /*
        * Make sure all the arguments are present...
        */

        if (nparams < 8)
	  values[0].data.d_status = STATUS_CALLING_ERROR;
	else
	{
	  strcpy(vars.output_to, param[3].data.d_string);
	  strcpy(vars.short_name, param[4].data.d_string);
	  vars.media_size  = param[5].data.d_int32;
	  vars.output_type = param[6].data.d_int32;
	  vars.brightness  = param[7].data.d_int32;

          if (nparams > 8)
            vars.scaling = param[8].data.d_int32;
          else
            vars.scaling = 100;

          if (nparams > 9)
            vars.orientation = param[9].data.d_int32;
          else
            vars.orientation = -1;

          if (nparams > 10)
            vars.left = param[10].data.d_int32;
          else
            vars.left = -1;

          if (nparams > 11)
            vars.top = param[11].data.d_int32;
          else
            vars.top = -1;

          for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
            if (strcmp(printers[i].short_name, vars.short_name) == 0)
              current_printer = i;
	};
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

  signal(SIGBUS, SIG_DFL);
  signal(SIGSEGV, SIG_DFL);

  if (values[0].data.d_status == STATUS_SUCCESS)
  {
   /*
    * Set the tile cache size...
    */

    gimp_tile_cache_ntiles((drawable->width + gimp_tile_width() - 1) /
                           gimp_tile_width());

   /*
    * Open the file/execute the print command...
    */

    if (vars.output_to[0] == '|')
      prn = popen(vars.output_to + 1, "w");
    else
      prn = fopen(vars.output_to, "w");

    if (prn != NULL)
    {
     /*
      * Got an output file/command, now compute a brightness lookup table...
      */

      printer    = printers + current_printer;
      brightness = 25500.0 * printer->density / vars.brightness;

      for (i = 0; i < 256; i ++)
      {
        j = 255.5 - brightness * (1.0 - pow((float)i / 255.0, printer->gamma));
        if (j < 0)
          lut[i] = 0;
        else if (j < 255)
          lut[i] = j;
        else
          lut[i] = 255;
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

      (*printer->print)(prn, drawable, vars.media_size, printer->xdpi, printer->ydpi,
                        vars.output_type, printer->model, lut, cmap,
                        vars.orientation, vars.scaling, vars.left, vars.top);

      if (vars.output_to[0] == '|')
        pclose(prn);
      else
        fclose(prn);
    }
    else
      values[0].data.d_status == STATUS_EXECUTION_ERROR;

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
 * 'print_dialog()' - Pop up the print dialog...
 */

int
print_dialog(void)
{
  int		i;		/* Looping var */
  GtkWidget	*dialog,	/* Dialog window */
		*table,		/* Table "container" for controls */
		*label,		/* Label string */
		*button,	/* OK/Cancel buttons */
		*entry,		/* Print file/command text entry */
		*menu,		/* Menu of drivers/sizes */
		*item,		/* Menu item */
		*option;	/* Option menu button */
  GSList	*group;		/* Grouping for output type */
  gint		argc;		/* Fake argc for GUI */
  gchar		**argv;		/* Fake argv for GUI */
  static char	*sizes[] =	/* Size strings */
  {
    "A/Letter (8.5x11\")",
    "Legal (8.5x14\")",
    "B/Tabloid (11x17\")",
    "A4 (210x297mm)",
    "A3 (297x420mm)"
  };
  static char	*orients[] =	/* Orientation strings */
  {
    "Auto",
    "Portrait",
    "Landscape"
  };


 /*
  * Initialize the program's display...
  */

  argc    = 1;
  argv    = g_new(gchar *, 1);
  argv[0] = g_strdup("print");

  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());
  gdk_set_use_xshm(gimp_use_xshm());

 /*
  * Get printrc options...
  */

  printrc_load();

 /*
  * Dialog window...
  */

  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Print");
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width(GTK_CONTAINER(dialog), 0);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		     (GtkSignalFunc) close_callback,
		     NULL);

 /*
  * Top-level table for dialog...
  */

  table = gtk_table_new(13, 3, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 6);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, FALSE, FALSE, 0);
  gtk_widget_show(table);

 /*
  * Drawing area for page preview...
  */

  preview = (GtkDrawingArea *)gtk_drawing_area_new();
  gtk_drawing_area_size(preview, PREVIEW_SIZE, PREVIEW_SIZE);
  gtk_table_attach(GTK_TABLE(table), (GtkWidget *)preview, 0, 3, 0, 4, 0, 0, 0, 0);
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
  * Printer option menu...
  */

  label = gtk_label_new("Printer:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  menu = gtk_menu_new();
  for (i = 0; i < plist_count; i ++)
  {
    if (strcmp(plist[i].command, vars.output_to) == 0)
      plist_current = i;

    item = gtk_menu_item_new_with_label(plist[i].name);
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       (GtkSignalFunc)plist_callback,
		       (gpointer)i);
    gtk_widget_show(item);
  };

  option = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option, 1, 3, 4, 5,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option), plist_current);
  gtk_widget_show(option);

 /*
  * Printer driver option menu...
  */

  label = gtk_label_new("Driver:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  menu = gtk_menu_new();
  for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
  {
    item = gtk_menu_item_new_with_label(printers[i].long_name);
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       (GtkSignalFunc)print_driver_callback,
		       (gpointer)i);
    gtk_widget_show(item);
  };

  printer_driver = option = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option, 1, 3, 5, 6,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option), current_printer);
  gtk_widget_show(option);

 /*
  * Output type toggles...
  */

  label = gtk_label_new("Output Type:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 6, 7, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  output_gray = button = gtk_radio_button_new_with_label(NULL, "B&W");
  group  = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     (GtkSignalFunc)output_type_callback,
		     (gpointer)OUTPUT_GRAY);
  gtk_table_attach(GTK_TABLE(table), button, 1, 2, 6, 7, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(button);
  if (vars.output_type == 0)
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), TRUE);

  output_color = button = gtk_radio_button_new_with_label(group, "Color");
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     (GtkSignalFunc)output_type_callback,
		     (gpointer)OUTPUT_COLOR);
  gtk_table_attach(GTK_TABLE(table), button, 2, 3, 6, 7, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(button);
  if (vars.output_type == 1)
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), TRUE);

 /*
  * Media size option menu...
  */

  label = gtk_label_new("Media Size:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 7, 8, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  menu = gtk_menu_new();
  for (i = 0; i < (sizeof(sizes) / sizeof(sizes[0])); i ++)
  {
    item = gtk_menu_item_new_with_label(sizes[i]);
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       (GtkSignalFunc)media_size_callback,
		       (gpointer)i);
    gtk_widget_show(item);
  };

  option = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option, 1, 3, 7, 8,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option), vars.media_size);
  gtk_widget_show(option);

 /*
  * Orientation option menu...
  */

  label = gtk_label_new("Orientation:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 8, 9, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  menu = gtk_menu_new();
  for (i = 0; i < (sizeof(orients) / sizeof(orients[0])); i ++)
  {
    item = gtk_menu_item_new_with_label(orients[i]);
    gtk_menu_append(GTK_MENU(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       (GtkSignalFunc)orientation_callback,
		       (gpointer)(i - 1));
    gtk_widget_show(item);
  };

  option = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option, 1, 3, 8, 9,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option), vars.orientation + 1);
  gtk_widget_show(option);

 /*
  * Scaling slider...
  */

  dialog_create_ivalue("Scaling:", GTK_TABLE(table), 9, &(vars.scaling), 1, 101);

 /*
  * Brightness slider...
  */

  dialog_create_ivalue("Brightness:", GTK_TABLE(table), 10, &(vars.brightness), 50, 201);

 /*
  * Print file/command...
  */

  label = gtk_label_new("File/|Command:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 11, 12, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  output_to = entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), vars.output_to);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)print_command_callback, NULL);
  gtk_table_attach(GTK_TABLE(table), entry, 1, 3, 11, 12, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(entry);

 /*
  * Print, cancel buttons...
  */

  gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 6);

  button = gtk_button_new_with_label("Print");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)print_callback,
		     dialog);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc)cancel_callback,
		     dialog);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show(button);

 /*
  * Show it and wait for the user to do something...
  */

  gtk_widget_show(dialog);

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
 * 'dialog_create_ivalue()' - Create an integer value control...
 */

static void
dialog_create_ivalue(char     *title,	/* I - Label for control */
                     GtkTable *table,	/* I - Table container to use */
                     int      row,	/* I - Row # for container */
                     gint     *value,	/* I - Value holder */
                     int      left,	/* I - Minimum value for slider */
                     int      right)	/* I - Maximum value for slider */
{
  GtkWidget	*label,		/* Control label */
		*scale,		/* Scale widget */
		*entry;		/* Text widget */
  GtkObject	*scale_data;	/* Scale data */
  char		buf[256];	/* String buffer */


 /*
  * Label...
  */

  label = gtk_label_new(title);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

 /*
  * Scale...
  */

  scale_data = gtk_adjustment_new(*value, left, right, 1.0, 1.0, 1.0);

  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc) dialog_iscale_update,
		     value);

  scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
  gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_widget_show(scale);

 /*
  * Text entry...
  */

  entry = gtk_entry_new();
  gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
  gtk_object_set_user_data(scale_data, entry);
  gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
  sprintf(buf, "%d", *value);
  gtk_entry_set_text(GTK_ENTRY(entry), buf);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc) dialog_ientry_update,
		     value);
  gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(entry);
}


/*
 * 'dialog_iscale_update()' - Update the value field using the scale.
 */

static void
dialog_iscale_update(GtkAdjustment *adjustment,	/* I - New value */
                     gint          *value)	/* I - Current value */
{
  GtkWidget	*entry;		/* Text entry widget */
  char		buf[256];	/* Text buffer */


  if (*value != adjustment->value)
  {
    *value = adjustment->value;

    entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
    sprintf(buf, "%d", *value);

    gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);

    if (value == &(vars.scaling))
      preview_update();
  };
}


/*
 * 'dialog_ientry_update()' - Update the value field using the text entry.
 */

static void
dialog_ientry_update(GtkWidget *widget,	/* I - Entry widget */
                     gint      *value)	/* I - Current value */
{
  GtkAdjustment	*adjustment;
  gint		new_value;


  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (*value != new_value)
  {
    adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

    if ((new_value >= adjustment->lower) &&
	(new_value <= adjustment->upper))
    {
      *value            = new_value;
      adjustment->value = new_value;

      gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
    };
  };
}


/*
 * 'plist_callback()' - Update the current system printer...
 */

static void
plist_callback(GtkWidget *widget,	/* I - Driver option menu */
               gint      data)		/* I - Data */
{
  int	i;


  plist_current = data;

  if (plist[plist_current].driver[0] != '\0')
  {
    strcpy(vars.short_name, plist[plist_current].driver);

    for (i = 0; i < (sizeof(printers) / sizeof(printers[0])); i ++)
      if (strcmp(printers[i].short_name, vars.short_name) == 0)
      {
        current_printer = i;
        break;
      };

    gtk_option_menu_set_history(GTK_OPTION_MENU(printer_driver), current_printer);
  };

  if (plist[plist_current].command[0] != '\0')
  {
    strcpy(vars.output_to, plist[plist_current].command);
    gtk_entry_set_text(GTK_ENTRY(output_to), vars.output_to);
  };

  if (plist[plist_current].output_type == OUTPUT_GRAY)
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(output_gray), TRUE);
  else
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(output_color), TRUE);
}


/*
 * 'print_driver_callback()' - Update the current printer driver...
 */

static void
print_driver_callback(GtkWidget *widget,	/* I - Driver option menu */
                      gint      data)		/* I - Data */
{
  current_printer = data;

  strcpy(vars.short_name, printers[current_printer].short_name);
  strcpy(plist[plist_current].driver, printers[current_printer].short_name);
}


/*
 * 'media_size_callback()' - Update the current media size...
 */

static void
media_size_callback(GtkWidget *widget,		/* I - Media size option menu */
                    gint      data)		/* I - Data */
{
  vars.media_size = data;
  vars.left       = -1;
  vars.top        = -1;

  preview_update();
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
 * 'print_command_callback()' - Update the print command...
 */

static void
print_command_callback(GtkWidget *widget,	/* I - Command text field */
                       gpointer  data)		/* I - Data */
{
  strcpy(vars.output_to, gtk_entry_get_text(GTK_ENTRY(widget)));
  strcpy(plist[plist_current].command, vars.output_to);
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
print_callback(GtkWidget *widget,	/* I - OK button widget */
               gpointer  data)		/* I - Dialog window */
{
  runme = TRUE;
  gtk_widget_destroy(GTK_WIDGET(data));
}


/*
 * 'cancel_callback()' - Cancel the print...
 */

static void
cancel_callback(GtkWidget *widget,	/* I - Cancel button widget */
                gpointer  data)		/* I - Dialog window */
{
  gtk_widget_destroy(GTK_WIDGET(data));
}


/*
 * 'close_callback()' - Exit the print dialog application.
 */

static void
close_callback(GtkWidget *widget,	/* I - Dialog window */
               gpointer  data)		/* I - Dialog window */
{
  gtk_main_quit();
}


static void
preview_update(void)
{
  int		orient,		/* True orientation of page */
		tw0, tw1,	/* Temporary page_widths */
		th0, th1,	/* Temporary page_heights */
		ta0, ta1;	/* Temporary areas */
  static GdkGC	*gc = NULL;


  gdk_window_clear(preview->widget.window);

  if (gc == NULL)
    gc = gdk_gc_new(preview->widget.window);

  page_width  = media_width(vars.media_size, 10);
  page_height = media_height(vars.media_size, 10);

  if (vars.orientation == ORIENT_AUTO)
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

    if (ta0 >= ta1)
      orient = ORIENT_PORTRAIT;
    else
      orient = ORIENT_LANDSCAPE;
  }
  else
    orient = vars.orientation;

  if (orient == ORIENT_LANDSCAPE)
  {
    page_width  = media_height(vars.media_size, 10);
    page_height = media_width(vars.media_size, 10);
  };

  page_left = (PREVIEW_SIZE - page_width) / 2;
  page_top  = (PREVIEW_SIZE - page_height) / 2;

  gdk_draw_rectangle(preview->widget.window, gc, 0,
                     (PREVIEW_SIZE - page_width - 5) / 2,
                     (PREVIEW_SIZE - page_height - 10) / 2,
                     page_width + 5, page_height + 10);

  print_width  = page_width * vars.scaling / 100;
  print_height = print_width * image_height / image_width;
  if (print_height > page_height)
  {
    print_height = page_height;
    print_width  = print_height * image_width / image_height;
  };

  if (vars.left < 0 || vars.top < 0)
    gdk_draw_rectangle(preview->widget.window, gc, 1,
                       (PREVIEW_SIZE - print_width) / 2,
                       (PREVIEW_SIZE - print_height) / 2,
                       print_width, print_height);
  else
  {
    if (vars.left > (page_width - print_width))
      vars.left = page_width - print_width;
    if (vars.top > (page_height - print_height))
      vars.top = page_height - print_height;

    gdk_draw_rectangle(preview->widget.window, gc, 1,
                       page_left + vars.left,
                       page_top + vars.top,
                       print_width, print_height);
  };

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
    vars.left = (page_width - print_width) / 2;
    vars.top  = (page_height - print_height) / 2;
  };

  vars.left += event->x - mouse_x;
  vars.top  += event->y - mouse_y;

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
  FILE		*fp;		/* Printrc file */
  char		line[1024];	/* Line in printrc file */
  plist_t	*p,		/* Current printer */
		key;		/* Search key */


 /*
  * Get the printer list...
  */

  get_printers();

 /*
  * Generate the filename for the current user...
  */

  if (getenv("HOME") == NULL)
    strcpy(line, "/.gimp/printrc");
  else
    sprintf(line, "%s/.gimp/printrc", getenv("HOME"));

  if ((fp = fopen(line, "r")) != NULL)
  {
   /*
    * File exists - read the contents and update the printer list...
    */

    while (fgets(line, sizeof(line), fp) != NULL)
    {
      if (line[0] == '#')
        continue;	/* Comment */

      if (sscanf(line, "%s%s%d%*[ \t]%[^\n]", key.name, key.driver,
                 &(key.output_type), key.command) == 4)
        if ((p = bsearch(&key, plist, plist_count, sizeof(plist_t),
                         (int (*)(const void *, const void *))compare_printers)) != NULL)
          memcpy(p, &key, sizeof(plist_t));
    };

    fclose(fp);
  };
}


/*
 * 'printrc_save()' - Save the current printer resource configuration.
 */

static void
printrc_save(void)
{
  FILE		*fp;		/* Printrc file */
  char		filename[1024];	/* Printrc filename */
  int		i;		/* Looping var */
  plist_t	*p;		/* Current printer */


 /*
  * Generate the filename for the current user...
  */

  if (getenv("HOME") == NULL)
    strcpy(filename, "/.gimp/printrc");
  else
    sprintf(filename, "%s/.gimp/printrc", getenv("HOME"));

  if ((fp = fopen(filename, "w")) != NULL)
  {
   /*
    * Write the contents of the printer list...
    */

    fputs("#PRINTRC " PLUG_IN_VERSION "\n", fp);

    for (i = 0, p = plist; i < plist_count; i ++, p ++)
      fprintf(fp, "%s %s %d %s\n", p->name, p->driver, p->output_type, p->command);

    fclose(fp);
  };
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
	name[17],
	defname[17];


  defname[0] = '\0';

  memset(plist, 0, sizeof(plist));

#ifndef sun	/* Sun Solaris merges LPR and LP queues */
  if (access("/usr/etc/lpc", 0) == 0 &&
      (pfile = popen("/usr/etc/lpc status", "r")) != NULL)
  {
    while (fgets(line, sizeof(line), pfile) != NULL &&
           plist_count < MAX_PLIST)
      if (strchr(line, ':') != NULL)
      {
        *strchr(line, ':') = '\0';
        strcpy(plist[plist_count].name, line);
        sprintf(plist[plist_count].command, "|lpr -P%s -l", line);
        strcpy(plist[plist_count].driver, "ps");
        plist[plist_count].output_type = OUTPUT_COLOR;
        plist_count ++;
      };

    pclose(pfile);
  };
#endif /* !sun */

  if (access("/usr/bin/lpstat", 0) == 0 &&
      (pfile = popen("/usr/bin/lpstat -d -p", "r")) != NULL)
  {
    while (fgets(line, sizeof(line), pfile) != NULL &&
           plist_count < MAX_PLIST)
    {
      if (sscanf(line, "printer %s", name) == 1)
      {
	strcpy(plist[plist_count].name, name);
#ifdef __sgi /* SGI still uses the SVR3 spooler */
	sprintf(plist[plist_count].command, "|lp -s -oraw -d%s", name);
#else
	sprintf(plist[plist_count].command, "|lp -s -oraw -d %s", name);
#endif /* __sgi */
        strcpy(plist[plist_count].driver, "ps");
        plist[plist_count].output_type = OUTPUT_COLOR;
        plist_count ++;
      }
      else
        sscanf(line, "system default destination: %s", defname);
    };

    pclose(pfile);
  };

  if (plist_count > 1)
    qsort(plist, plist_count, sizeof(plist_t),
          (int (*)(const void *, const void *))compare_printers);

  if (defname[0] != '\0')
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
