/*
 * "$Id$"
 *
 *   SGI image file plug-in for the GIMP.
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
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the plug-in...
 *   load_image()                - Load a PNG image into a new image window.
 *   save_image()                - Save the specified image to a PNG file.
 *   save_ok_callback()          - Destroy the save dialog and save the image.
 *   save_dialog()               - Pop up the save dialog.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.19  2000/01/25 17:46:54  mitch
 *   2000-01-25  Michael Natterer  <mitch@gimp.org>
 *
 *   	* configure.in
 *   	* po-plug-ins/POTFILES.in
 *   	* plug-ins/common/Makefile.am
 *   	* plug-ins/common/plugin-defs.pl
 *   	* plug-ins/megawidget/*: removed. (There were only 3 functions
 *   	left which were used by ~5 plugins, so I moved the resp. functions
 *   	to the plugins). More preview stuff to come...
 *
 *   	* app/airbrush_blob.c
 *   	* modules/colorsel_triangle.c
 *   	* modules/colorsel_water.c: use G_PI instead of M_PI.
 *
 *   	* app/procedural_db.h
 *   	* libgimp/gimpenums.h
 *   	* plug-ins/script-fu/script-fu-constants.c
 *   	* tools/pdbgen/enums.pl: new PDB return value STATUS_CANCEL which
 *   	indicates that "Cancel" was pressed in a plugin dialog. (Useful
 *   	only for file load/save plugins).
 *
 *   	* app/fileops.[ch]
 *   	* app/menus.c: changes to handle STATUS_CANCEL correctly. Did some
 *   	code cleanup in fileops.[ch]. Pop up a warning if File->Save
 *   	failed.
 *
 *   	* app/plug_in.c: return_val[0] is of type PDB_STATUS, not
 *   	PDB_INT32.
 *
 *   	* libgimp/gimpmath.h: new constant G_MAXRAND which equals to
 *   	RAND_MAX if it exists or to G_MAXINT otherwise.
 *
 *   	* libgimp/gimpwidgets.[ch]: new function gimp_random_seed_new()
 *   	which creates a spinbutton and a "Time" toggle.
 *   	Call the function which does the "set_sensitive" magic from the
 *   	radio button callback.
 *
 *   	* plug-ins/[75 plugins]:
 *
 *   	- Return STATUS_CANCEL in all file load/save dialogs if "Cancel"
 *   	  was pressed.
 *   	- Standardized the file plugins' "run" functions.
 *   	- Use G_PI and G_MAXRAND everywhere.
 *   	- Added tons of scales and spinbuttons instead of text entries.
 *   	- Applied uniform packing/spacings all over the place.
 *   	- Reorganized some UIs (stuff like moving the preview to the top
 *   	  left corner of the dialog).
 *   	- Removed many ui helper functions and callbacks and use the stuff
 *   	  from libgimp instead.
 *   	- I tried not to restrict the range of possible values when I
 *   	  replaced entries with spinbuttons/scales but may have failed,
 *   	  though in some cases. Please test ;-)
 *   	- #include <libgimp/gimpmath.h> where appropriate and use it's
 *   	  constants.
 *   	- Indentation, s/int/gint/ et.al., code cleanup.
 *
 *   	RFC: The plugins are definitely not useable with GIMP 1.0 any
 *   	     more, so shouldn't we remove all the remaining compatibility
 *   	     stuff ??? (like "#ifdef GIMP_HAVE_PARASITES")
 *
 *   Revision 1.18  2000/01/17 17:02:26  mitch
 *   2000-01-17  Michael Natterer  <mitch@gimp.org>
 *
 *   	* libgimp/gimpcolorbutton.c: emit the "color_changed" signal
 *   	whenever the user selects "Use FG/BG Color" from the popup menu.
 *
 *   	* libgimp/gimpwidgets.c: gimp_table_attach_aligned(): allow the
 *   	function to be called with label == NULL.
 *
 *   	* plug-ins/AlienMap/AlienMap.c
 *   	* plug-ins/AlienMap2/AlienMap2.c
 *   	* plug-ins/common/CEL.c
 *   	* plug-ins/common/CML_explorer.c
 *   	* plug-ins/common/apply_lens.c
 *   	* plug-ins/common/checkerboard.c
 *   	* plug-ins/common/engrave.c
 *   	* plug-ins/common/exchange.c
 *   	* plug-ins/common/gauss_iir.c
 *   	* plug-ins/common/gauss_rle.c
 *   	* plug-ins/common/illusion.c
 *   	* plug-ins/common/max_rgb.c
 *   	* plug-ins/common/mblur.c
 *   	* plug-ins/common/oilify.c
 *   	* plug-ins/common/sel_gauss.c
 *   	* plug-ins/common/shift.c
 *   	* plug-ins/common/smooth_palette.c
 *   	* plug-ins/common/sparkle.c
 *   	* plug-ins/common/video.c
 *   	* plug-ins/common/vpropagate.c
 *   	* plug-ins/common/warp.c
 *   	* plug-ins/sgi/sgi.c: more ui updates.
 *
 *   Revision 1.17  2000/01/13 15:39:25  mitch
 *   2000-01-13  Michael Natterer  <mitch@gimp.org>
 *
 *   	* app/gimpui.[ch]
 *   	* app/preferences_dialog.c: removed & renamed some functions from
 *   	gimpui.[ch] (see below).
 *
 *   	* libgimp/Makefile.am
 *   	* libgimp/gimpwidgets.[ch]; new files. Functions moved from
 *   	app/gimpui.[ch]. Added a constructor for the label + hscale +
 *   	entry combination used in many plugins (now hscale + spinbutton).
 *
 *   	* libgimp/gimpui.h: include gimpwidgets.h
 *
 *   	* plug-ins/megawidget/megawidget.[ch]: removed all functions
 *   	except the preview stuff (I'm not yet sure how to implement this
 *   	in libgimp because the libgimp preview should be general enough to
 *   	replace all the other plugin previews, too).
 *
 *   	* plug-ins/borderaverage/Makefile.am
 *   	* plug-ins/borderaverage/borderaverage.c
 *   	* plug-ins/common/plugin-defs.pl
 *   	* plug-ins/common/Makefile.am
 *   	* plug-ins/common/aa.c
 *   	* plug-ins/common/align_layers.c
 *   	* plug-ins/common/animationplay.c
 *   	* plug-ins/common/apply_lens.c
 *   	* plug-ins/common/blinds.c
 *   	* plug-ins/common/bumpmap.c
 *   	* plug-ins/common/checkerboard.c
 *   	* plug-ins/common/colorify.c
 *   	* plug-ins/common/convmatrix.c
 *   	* plug-ins/common/cubism.c
 *   	* plug-ins/common/curve_bend.c
 *   	* plug-ins/common/deinterlace.c
 *   	* plug-ins/common/despeckle.c
 *   	* plug-ins/common/destripe.c
 *   	* plug-ins/common/displace.c
 *   	* plug-ins/common/edge.c
 *   	* plug-ins/common/emboss.c
 *   	* plug-ins/common/hot.c
 *   	* plug-ins/common/nlfilt.c
 *   	* plug-ins/common/pixelize.c
 *   	* plug-ins/common/waves.c
 *   	* plug-ins/sgi/sgi.c
 *   	* plug-ins/sinus/sinus.c: ui updates like removing megawidget,
 *   	using the dialog constructor, I18N fixes, indentation, ...
 *
 *   Revision 1.16  2000/01/01 15:38:59  neo
 *   added gettext support
 *
 *
 *   --Sven
 *
 *   Revision 1.15  1999/11/27 02:54:25  neo
 *          * plug-ins/sgi/sgi.c: bail out nicely instead of aborting when
 *           saving fails.
 *
 *   --Sven
 *
 *   Revision 1.14  1999/11/26 20:58:26  neo
 *   more action_area beautifiction
 *
 *
 *   --Sven
 *
 *   Revision 1.13  1999/10/20 01:45:41  neo
 *   the rest of the save plug-ins !?
 *
 *
 *   --Sven
 *
 *   Revision 1.12  1999/04/23 06:32:41  asbjoer
 *   use MAIN macro
 *
 *   Revision 1.11  1999/04/15 21:49:06  yosh
 *   * applied gimp-lecorfec-99041[02]-0, changes follow
 *
 *   * plug-ins/FractalExplorer/Dialogs.h (make_color_map):
 *   replaced free with g_free to fix segfault.
 *
 *   * plug-ins/Lighting/lighting_preview.c (compute_preview):
 *   allocate xpostab and ypostab only when needed (it could also be
 *   allocated on stack with a compilation-fixed size like MapObject).
 *   It avoids to lose some Kb on each preview :)
 *   Also reindented (unfortunate C-c C-q) some other lines.
 *
 *   * plug-ins/Lighting/lighting_main.c (run):
 *   release allocated postabs.
 *
 *   * plug-ins/Lighting/lighting_ui.c:
 *   callbacks now have only one argument because gck widget use
 *   gtk_signal_connect_object. Caused segfault for scale widget.
 *
 *   * plug-ins/autocrop/autocrop.c (doit):
 *   return if image has only background (thus fixing a segfault).
 *
 *   * plug-ins/emboss/emboss.c (pluginCore, emboss_do_preview):
 *   replaced malloc/free with g_malloc/g_free (unneeded, but
 *   shouldn't everyone use glib calls ? :)
 *
 *   * plug-ins/flame/flame.c :
 *   replaced a segfaulting free, and several harmless malloc/free pairs.
 *
 *   * plug-ins/flame/megawidget.c (mw_preview_build):
 *   replaced harmless malloc/free pair.
 *   Note : mwp->bits is malloc'ed but seems to be never freed.
 *
 *   * plug-ins/fractaltrace/fractaltrace.c (pixels_free):
 *   replaced a bunch of segfaulting free.
 *   (pixels_get, dialog_show): replaced gtk_signal_connect_object
 *   with gtk_signal_connect to accomodate callbacks (caused STRANGE
 *   dialog behaviour, coz you destroyed buttons one by one).
 *
 *   * plug-ins/illusion/illusion.c (dialog):
 *   same gtk_signal_connect_object replacement for same reasons.
 *
 *   * plug-ins/libgck/gck/gckcolor.c :
 *   changed all gck_rgb_to_color* functions to use a static GdkColor
 *   instead of a malloc'ed area. Provided reentrant functions with
 *   the old behaviour (gck_rgb_to_color*_r). Made some private functions
 *   static, too.
 *   gck_rgb_to_gdkcolor now use the new functions while
 *   gck_rgb_to_gdkcolor_r is the reentrant version.
 *   Also affected by this change: gck_gc_set_foreground and
 *   gck_gc_set_background (no more free(color)).
 *
 *   * plug-ins/libgck/gck/gckcolor.h :
 *   added the gck_rgb_to_gdkcolor_r proto.
 *
 *   * plug-ins/lic/lic.c (ok_button_clicked, cancel_button_clicked) :
 *   segfault on gtk_widget_destroy, now calls gtk_main_quit.
 *   (dialog_destroy) : segfault on window closure when called by
 *   "destroy" event. Now called by "delete_event".
 *
 *   * plug-ins/megawidget/megawidget.c (mw_preview_build):
 *   replaced harmless malloc/free pair.
 *   Note : mwp->bits is malloc'ed but seems to be never freed.
 *
 *   * plug-ins/png/png.c (load_image):
 *   replaced 2 segfaulting free.
 *
 *   * plug-ins/print/print-ps.c (ps_print):
 *   replaced a segfaulting free (called many times :).
 *
 *   * plug-ins/sgi/sgi.c (load_image, save_image):
 *   replaced a bunch of segfaulting free, and did some harmless
 *   inits to avoid a few gcc warnings.
 *
 *   * plug-ins/wind/wind.c (render_wind):
 *   replaced a segfaulting free.
 *   (render_blast): replaced harmless malloc/free pair.
 *
 *   * plug-ins/bmp/bmpread.c (ReadImage):
 *   yet another free()/g_free() problem fixed.
 *
 *   * plug-ins/exchange/exchange.c (real_exchange):
 *   ditto.
 *
 *   * plug-ins/fp/fp.h: added Frames_Check_Button_In_A_Box proto.
 *   * plug-ins/fp/fp_gtk.c: closing subdialogs via window manager
 *   wasn't handled, thus leading to errors and crashes.
 *   Now delete_event signals the dialog control button
 *   to close a dialog with the good way.
 *
 *   * plug-ins/ifscompose/ifscompose.c (value_pair_create):
 *   tried to set events mask on scale widget (a NO_WINDOW widget).
 *
 *   * plug-ins/png/png.c (save_image):
 *   Replaced 2 free() with g_free() for g_malloc'ed memory.
 *   Mysteriously I corrected the loading bug but not the saving one :)
 *
 *   -Yosh
 *
 *   Revision 1.10  1999/01/15 17:34:37  unammx
 *   1999-01-15  Federico Mena Quintero  <federico@nuclecu.unam.mx>
 *
 *   	* Updated gtk_toggle_button_set_state() to
 *   	gtk_toggle_button_set_active() in all the files.
 *
 *   Revision 1.9  1998/06/06 23:22:19  yosh
 *   * adding Lighting plugin
 *
 *   * updated despeckle, png, sgi, and sharpen
 *
 *   -Yosh
 *
 *   Revision 1.5  1998/05/17 16:01:33  mike
 *   Removed signal handler stuff used for debugging.
 *   Added gtk_rc_parse().
 *   Removed extra variables.
 *
 *   Revision 1.4  1998/04/23  17:40:49  mike
 *   Updated to support 16-bit <unsigned> image data.
 *
 *   Revision 1.3  1997/11/14  17:17:59  mike
 *   Updated to dynamically allocate return params in the run() function.
 *   Added warning message about advanced RLE compression not being supported
 *   by SGI.
 *
 *   Revision 1.2  1997/07/25  20:44:05  mike
 *   Fixed image_load_sgi load error bug (causes GIMP hang/crash).
 *
 *   Revision 1.1  1997/06/18  00:55:28  mike
 *   Initial revision
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "sgi.h"		/* SGI image library definitions */

#include "libgimp/stdplugins-intl.h"


/*
 * Constants...
 */

#define PLUG_IN_VERSION  "1.1.1 - 17 May 1998"


/*
 * Local functions...
 */

static void	query       (void);
static void	run         (gchar   *name,
			     gint     nparams,
			     GParam  *param,
			     gint    *nreturn_vals,
			     GParam **return_vals);

static gint32	load_image  (gchar   *filename);
static gint	save_image  (gchar   *filename,
			     gint32   image_ID,
			     gint32   drawable_ID);

static void     init_gtk    (void);
static gint	save_dialog (void);

/*
 * Globals...
 */

GPlugInInfo	PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static gint  compression = SGI_COMP_RLE;
static gint  runme       = FALSE;


MAIN ()

static void
query (void)
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32,      "run_mode",     "Interactive, non-interactive" },
    { PARAM_STRING,     "filename",     "The name of the file to load" },
    { PARAM_STRING,     "raw_filename", "The name of the file to load" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE,      "image",        "Output image" },
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  static GParamDef save_args[] =
  {
    { PARAM_INT32,	"run_mode",	"Interactive, non-interactive" },
    { PARAM_IMAGE,	"image",	"Input image" },
    { PARAM_DRAWABLE,	"drawable",	"Drawable to save" },
    { PARAM_STRING,	"filename",	"The name of the file to save the image in" },
    { PARAM_STRING,	"raw_filename",	"The name of the file to save the image in" },
    { PARAM_INT32,	"compression",	"Compression level (0 = none, 1 = RLE, 2 = ARLE)" }
  };
  static gint 	nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  INIT_I18N();

  gimp_install_procedure ("file_sgi_load",
			  _("Loads files in SGI image file format"),
			  _("This plug-in loads SGI image files."),
			  "Michael Sweet <mike@easysw.com>",
			  "Copyright 1997-1998 by Michael Sweet",
			  PLUG_IN_VERSION,
			  "<Load>/SGI",
			  NULL,
			  PROC_PLUG_IN,
			  nload_args,
			  nload_return_vals,
			  load_args,
			  load_return_vals);

  gimp_install_procedure ("file_sgi_save",
			  _("Saves files in SGI image file format"),
			  _("This plug-in saves SGI image files."),
			  "Michael Sweet <mike@easysw.com>",
			  "Copyright 1997-1998 by Michael Sweet",
			  PLUG_IN_VERSION,
			  "<Save>/SGI",
			  "RGB*,GRAY*",
			  PROC_PLUG_IN,
			  nsave_args,
			  0,
			  save_args,
			  NULL);

  gimp_register_magic_load_handler ("file_sgi_load",
				    "rgb,bw,sgi,icon",
				    "",
				    "0,short,474");
  gimp_register_save_handler       ("file_sgi_save",
				    "rgb,bw,sgi,icon",
				    "");
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType  run_mode;       
  GStatusType   status = STATUS_SUCCESS;
  gint32	image_ID;	
  gint32        drawable_ID;
  GimpExportReturnType export = EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

  INIT_I18N_UI();

  if (strcmp (name, "file_sgi_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[1].type         = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  status = STATUS_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_sgi_save") == 0)
    {
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */ 
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  init_gtk ();
	  export = gimp_export_image (&image_ID, &drawable_ID, "SGI", 
				      (CAN_HANDLE_RGB |
				       CAN_HANDLE_GRAY |
				       CAN_HANDLE_ALPHA));
	  if (export == EXPORT_CANCEL)
	    {
	      values[0].data.d_status = STATUS_CANCEL;
	      return;
	    }
	  break;
	default:
	  break;
	}

      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  /*
	   * Possibly retrieve data...
	   */
          gimp_get_data ("file_sgi_save", &compression);

	  /*
	   * Then acquire information with a dialog...
	   */
          if (!save_dialog ())
            status = STATUS_CANCEL;
          break;

	case RUN_NONINTERACTIVE:
	  /*
	   * Make sure all the arguments are there!
	   */
          if (nparams != 6)
	    {
	      status = STATUS_CALLING_ERROR;
	    }
          else
	    {
	      compression = param[5].data.d_int32;

	      if (compression < 0 || compression > 2)
		status = STATUS_CALLING_ERROR;
	    };
          break;

	case RUN_WITH_LAST_VALS:
	  /*
	   * Possibly retrieve data...
	   */
          gimp_get_data ("file_sgi_save", &compression);
          break;

	default:
          break;
	};

      if (status == STATUS_SUCCESS)
	{
	  if (save_image (param[3].data.d_string, image_ID, drawable_ID))
	    {
	      gimp_set_data ("file_sgi_save",
			     &compression, sizeof (compression));
	    }
	  else
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
	}

      if (export == EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else
    {
      status = STATUS_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}


/*
 * 'load_image()' - Load a PNG image into a new image window.
 */

static gint32
load_image (gchar *filename)	/* I - File to load */
{
  int		i,		/* Looping var */
		x,		/* Current X coordinate */
		y,		/* Current Y coordinate */
		image_type,	/* Type of image */
		layer_type,	/* Type of drawable/layer */
		tile_height,	/* Height of tile in GIMP */
		count;		/* Count of rows to put in image */
  sgi_t		*sgip;		/* File pointer */
  gint32	image,		/* Image */
		layer;		/* Layer */
  GDrawable	*drawable;	/* Drawable for layer */
  GPixelRgn	pixel_rgn;	/* Pixel region for layer */
  guchar	**pixels,	/* Pixel rows */
		*pixel,		/* Pixel data */
		*pptr;		/* Current pixel */
  gushort      **rows;	        /* SGI image data */
  gchar		*progress;	/* Title for progress display... */


 /*
  * Open the file for reading...
  */

  sgip = sgiOpen(filename, SGI_READ, 0, 0, 0, 0, 0);
  if (sgip == NULL)
    {
      g_message ("can't open image file\n");
      return -1;
    };

  if (strrchr(filename, '/') != NULL)
    progress = g_strdup_printf (_("Loading %s:"), strrchr(filename, '/') + 1);
  else
    progress = g_strdup_printf (_("Loading %s:"), filename);

  gimp_progress_init(progress);
  g_free (progress);

  /*
   * Get the image dimensions and create the image...
   */
  image_type = 0; /* shut up warnings */
  layer_type = 0;
  switch (sgip->zsize)
    {
    case 1 :	/* Grayscale */
      image_type = GRAY;
      layer_type = GRAY_IMAGE;
      break;

    case 2 :	/* Grayscale + alpha */
      image_type = GRAY;
      layer_type = GRAYA_IMAGE;
      break;

    case 3 :	/* RGB */
      image_type = RGB;
      layer_type = RGB_IMAGE;
      break;

    case 4 :	/* RGBA */
      image_type = RGB;
      layer_type = RGBA_IMAGE;
      break;
    }

  image = gimp_image_new(sgip->xsize, sgip->ysize, image_type);
  if (image == -1)
    {
      g_message ("can't allocate new image\n");
      return -1;
    }

  gimp_image_set_filename(image, filename);

  /*
   * Create the "background" layer to hold the image...
   */

  layer = gimp_layer_new (image, _("Background"), sgip->xsize, sgip->ysize,
			  layer_type, 100, NORMAL_MODE);
  gimp_image_add_layer(image, layer, 0);

  /*
   * Get the drawable and set the pixel region for our load...
   */

  drawable = gimp_drawable_get(layer);

  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, drawable->width,
                      drawable->height, TRUE, FALSE);

  /*
   * Temporary buffers...
   */

  tile_height = gimp_tile_height();
  pixel       = g_new(guchar, tile_height * sgip->xsize * sgip->zsize);
  pixels      = g_new(guchar *, tile_height);

  for (i = 0; i < tile_height; i ++)
    pixels[i] = pixel + sgip->xsize * sgip->zsize * i;

  rows    = g_new(unsigned short *, sgip->zsize);
  rows[0] = g_new(unsigned short, sgip->xsize * sgip->zsize);

  for (i = 1; i < sgip->zsize; i ++)
    rows[i] = rows[0] + i * sgip->xsize;

  /*
   * Load the image...
   */

  for (y = 0, count = 0;
       y < sgip->ysize;
       y ++, count ++)
    {
      if (count >= tile_height)
	{
	  gimp_pixel_rgn_set_rect (&pixel_rgn, pixel,
				   0, y - count, drawable->width, count);
	  count = 0;

	  gimp_progress_update((double)y / (double)sgip->ysize);
	}

      for (i = 0; i < sgip->zsize; i ++)
	if (sgiGetRow(sgip, rows[i], sgip->ysize - 1 - y, i) < 0)
	  printf("sgiGetRow(sgip, rows[i], %d, %d) failed!\n",
		 sgip->ysize - 1 - y, i);

      if (sgip->bpp == 1)
	{
	  /*
	   * 8-bit (unsigned) pixels...
	   */

	  for (x = 0, pptr = pixels[count]; x < sgip->xsize; x ++)
	    for (i = 0; i < sgip->zsize; i ++, pptr ++)
	      *pptr = rows[i][x];
	}
      else
	{
	  /*
	   * 16-bit (unsigned) pixels...
	   */

	  for (x = 0, pptr = pixels[count]; x < sgip->xsize; x ++)
	    for (i = 0; i < sgip->zsize; i ++, pptr ++)
	      *pptr = rows[i][x] >> 8;
	}
    }

  /*
   * Do the last n rows (count always > 0)
   */

  gimp_pixel_rgn_set_rect (&pixel_rgn, pixel, 0,
			   y - count, drawable->width, count);

  /*
   * Done with the file...
   */

  sgiClose (sgip);

  g_free (pixel);
  g_free (pixels);
  g_free (rows[0]);
  g_free (rows);

  /*
   * Update the display...
   */

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return (image);
}


/*
 * 'save_image()' - Save the specified image to a SGI file.
 */

static gint
save_image (gchar  *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  gint        i, j,        /* Looping var */
              x,           /* Current X coordinate */
              y,           /* Current Y coordinate */
              tile_height, /* Height of tile in GIMP */
              count,       /* Count of rows to put in image */
              zsize;       /* Number of channels in file */
  sgi_t	     *sgip;        /* File pointer */
  GDrawable  *drawable;    /* Drawable for layer */
  GPixelRgn   pixel_rgn;   /* Pixel region for layer */
  guchar    **pixels,      /* Pixel rows */
             *pixel,       /* Pixel data */
             *pptr;        /* Current pixel */
  gushort   **rows;        /* SGI image data */
  gchar      *progress;    /* Title for progress display... */

  /*
   * Get the drawable for the current image...
   */

  drawable = gimp_drawable_get (drawable_ID);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width,
		       drawable->height, FALSE, FALSE);

  zsize = 0;
  switch (gimp_drawable_type(drawable_ID))
    {
    case GRAY_IMAGE :
      zsize = 1;
      break;
    case GRAYA_IMAGE :
      zsize = 2;
      break;
    case RGB_IMAGE :
      zsize = 3;
      break;
    case RGBA_IMAGE :
      zsize = 4;
      break;
    default:
      g_message ("SGI: Image must be of type RGB or GRAY\n");
      return FALSE;
      break;
    }

  /*
   * Open the file for writing...
   */

  sgip = sgiOpen (filename, SGI_WRITE, compression, 1, drawable->width,
		  drawable->height, zsize);
  if (sgip == NULL)
    {
      g_message ("SGI: Can't create image file\n");
      return FALSE;
    };

  if (strrchr(filename, '/') != NULL)
    progress = g_strdup_printf (_("Saving %s:"), strrchr(filename, '/') + 1);
  else
    progress = g_strdup_printf (_("Saving %s:"), filename);

  gimp_progress_init(progress);
  g_free (progress);

  /*
   * Allocate memory for "tile_height" rows...
   */

  tile_height = gimp_tile_height();
  pixel       = g_new(guchar, tile_height * drawable->width * zsize);
  pixels      = g_new(guchar *, tile_height);

  for (i = 0; i < tile_height; i ++)
    pixels[i]= pixel + drawable->width * zsize * i;

  rows    = g_new (gushort *, sgip->zsize);
  rows[0] = g_new (gushort, sgip->xsize * sgip->zsize);

  for (i = 1; i < sgip->zsize; i ++)
    rows[i] = rows[0] + i * sgip->xsize;

  /*
   * Save the image...
   */

  for (y = 0; y < drawable->height; y += count)
    {
      /*
       * Grab more pixel data...
       */

      if ((y + tile_height) >= drawable->height)
	count = drawable->height - y;
      else
	count = tile_height;

      gimp_pixel_rgn_get_rect (&pixel_rgn, pixel, 0, y, drawable->width, count);

      /*
       * Convert to shorts and write each color plane separately...
       */

      for (i = 0, pptr = pixels[0]; i < count; i ++)
	{
	  for (x = 0; x < drawable->width; x ++)
	    for (j = 0; j < zsize; j ++, pptr ++)
	      rows[j][x] = *pptr;

	  for (j = 0; j < zsize; j ++)
	    sgiPutRow (sgip, rows[j], drawable->height - 1 - y - i, j);
	};

      gimp_progress_update ((double) y / (double) drawable->height);
    };

  /*
   * Done with the file...
   */

  sgiClose (sgip);

  g_free (pixel);
  g_free (pixels);
  g_free (rows[0]);
  g_free (rows);

  return TRUE;
}

static void
save_ok_callback (GtkWidget *widget,
                  gpointer   data)
{
  runme = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void 
init_gtk (void)
{
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("sgi");
  
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
}

static gint
save_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;

  dlg = gimp_dialog_new (_("Save as SGI"), "sgi",
			 gimp_plugin_help_func, "filters/sgi.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, FALSE, FALSE,

			 _("OK"), save_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  frame = gimp_radio_group_new2 (TRUE, _("Compression Type"),
				 gimp_radio_button_update,
				 &compression, (gpointer) compression,

				 _("No Compression"),
				 (gpointer) SGI_COMP_NONE, NULL,
				 _("RLE Compression"),
				 (gpointer) SGI_COMP_RLE, NULL,
				 _("Aggressive RLE\n(Not Supported by SGI)"),
				 (gpointer) SGI_COMP_ARLE, NULL,

				 NULL);

  gtk_container_set_border_width (GTK_CONTAINER(frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return runme;
}
