/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>

#if GDK_WINDOWING == GDK_WINDOWING_WIN32
# define POINTS POINTS_win32	/* Guard against clash */
#endif
#include <gdk/gdkprivate.h>
#if GDK_WINDOWING == GDK_WINDOWING_WIN32
# undef POINTS
#endif

#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "drawable.h"
#include "edit_selection.h"
#include "errors.h"
#include "floating_sel.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "general.h"
#include "global_edit.h"
#include "interface.h"
#include "palette.h"
#include "procedural_db.h"
#include "selection.h"
#include "text_tool.h"
#include "tool_options_ui.h"
#include "tools.h"
#include "undo.h"

#include "tile_manager_pvt.h"
#include "drawable_pvt.h"

#include "libgimp/gimplimits.h"
#include "libgimp/gimpintl.h"

#define FOUNDRY      0
#define FAMILY       1
#define WEIGHT       2
#define SLANT        3
#define SET_WIDTH    4
#define PIXEL_SIZE   6
#define POINT_SIZE   7
#define XRESOLUTION  8
#define YRESOLUTION  9
#define SPACING     10
#define REGISTRY    12
#define ENCODING    13

/*  the text tool structures  */

typedef struct _TextTool TextTool;
struct _TextTool
{
  int   click_x;
  int   click_y;
  void *gdisp_ptr;
};

typedef struct _TextOptions TextOptions;
struct _TextOptions
{
  ToolOptions  tool_options;

  int          antialias;
  int          antialias_d;
  GtkWidget   *antialias_w;

  int          border;
  int          border_d;
  GtkObject   *border_w;

  int          use_dyntext;
  int          use_dyntext_d;
  GtkWidget   *use_dyntext_w;
};


/*  the text tool options  */
static TextOptions *text_options = NULL;

/*  local variables  */
static TextTool  *the_text_tool   = NULL;
static GtkWidget *text_tool_shell = NULL;


static void   text_button_press   (Tool *, GdkEventButton *, gpointer);
static void   text_button_release (Tool *, GdkEventButton *, gpointer);
static void   text_motion         (Tool *, GdkEventMotion *, gpointer);
static void   text_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   text_control        (Tool *, ToolAction,       gpointer);

static void   text_create_dialog   (void);
static void   text_ok_callback     (GtkWidget *, gpointer);
static void   text_cancel_callback (GtkWidget *, gpointer);
static gint   text_delete_callback (GtkWidget *, GdkEvent *, gpointer);

static void   text_init_render         (TextTool *);
static void   text_gdk_image_to_region (GdkImage *, int, PixelRegion *);
static void   text_size_multiply       (char **fontname, int);
static void   text_set_resolution      (char **fontname, double, double);

Layer       * text_render (GImage *, GimpDrawable *,
			   int, int, char *, char *, int, int);


/*  functions  */

static void
text_options_reset (void)
{
  TextOptions *options = text_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->antialias_w),
				options->antialias_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->border_w),
			    options->border_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_dyntext_w),
				options->use_dyntext_d);
}

static TextOptions *
text_options_new (void)
{
  TextOptions *options;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkWidget *sep;

  /*  the new text tool options structure  */
  options = (TextOptions *) g_malloc (sizeof (TextOptions));
  tool_options_init ((ToolOptions *) options,
		     N_("Text Tool Options"),
		     text_options_reset);
  options->antialias   = options->antialias_d   = TRUE;
  options->border      = options->border_d      = 0;
  options->use_dyntext = options->use_dyntext_d = FALSE;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  antialias toggle  */
  options->antialias_w =
    gtk_check_button_new_with_label (_("Antialiasing"));
  gtk_signal_connect (GTK_OBJECT (options->antialias_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->antialias);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->antialias_w),
				options->antialias_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->antialias_w,
		      FALSE, FALSE, 0);
  gtk_widget_show (options->antialias_w);

  /*  the border spinbutton  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Border:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->border_w =
    gtk_adjustment_new (options->border_d, 0.0, 32767.0, 1.0, 50.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (options->border_w), "changed",
		      (GtkSignalFunc) tool_options_int_adjustment_update,
		      &options->border);
  spinbutton =
    gtk_spin_button_new (GTK_ADJUSTMENT (options->border_w), 1.0, 0.0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  gtk_widget_show (hbox);

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  /*  the dynamic text toggle  */
  options->use_dyntext_w =
    gtk_check_button_new_with_label (_("Use Dynamic Text"));
  gtk_signal_connect (GTK_OBJECT (options->use_dyntext_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->use_dyntext);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_dyntext_w),
				options->use_dyntext_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->use_dyntext_w,
		      FALSE, FALSE, 0);
  gtk_widget_show (options->use_dyntext_w);

  /*  let the toggle callback set the sensitive states  */
  gtk_widget_set_sensitive (options->antialias_w, ! options->use_dyntext_d);
  gtk_widget_set_sensitive (spinbutton, ! options->use_dyntext_d);
  gtk_widget_set_sensitive (label, ! options->use_dyntext_d);
  gtk_object_set_data (GTK_OBJECT (options->use_dyntext_w), "inverse_sensitive",
		       options->antialias_w);
  gtk_object_set_data (GTK_OBJECT (options->antialias_w), "inverse_sensitive",
		       spinbutton);
  gtk_object_set_data (GTK_OBJECT (spinbutton), "inverse_sensitive", label);

  return options;
}

Tool*
tools_new_text ()
{
  Tool * tool;

  /*  The tool options  */
  if (! text_options)
    {
      text_options = text_options_new ();
      tools_register (TEXT, (ToolOptions *) text_options);
    }

  /*  the new text tool structure  */
  tool = (Tool *) g_malloc (sizeof (Tool));
  the_text_tool = (TextTool *) g_malloc (sizeof (TextTool));

  tool->type = TEXT;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /* Do not allow scrolling */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) the_text_tool;

  tool->button_press_func = text_button_press;
  tool->button_release_func = text_button_release;
  tool->motion_func = text_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->modifier_key_func = standard_modifier_key_func;
  tool->cursor_update_func = text_cursor_update;
  tool->control_func = text_control;

  tool->preserve = TRUE;

  return tool;
}

void
tools_free_text (Tool *tool)
{
  g_free (tool->private);
  the_text_tool = NULL;

  if (text_tool_shell && GTK_WIDGET_VISIBLE (text_tool_shell))
    gtk_widget_hide (text_tool_shell);
}

static void
text_call_gdyntext (GDisplay *gdisp)
{
  ProcRecord *proc_rec;
  Argument   *args;

  /*  find the gDynText PDB record  */
  if ((proc_rec = procedural_db_lookup ("plug_in_dynamic_text")) == NULL)
    {
      g_message (_("text_call_gdyntext: gDynText procedure lookup failed"));
      return;
    }

  /*  plug-in arguments as if called by <Image>/Filters/...  */
  args = g_new (Argument, 3);
  args[0].arg_type = PDB_INT32;
  args[0].value.pdb_int = RUN_INTERACTIVE;
  args[1].arg_type = PDB_IMAGE;
  args[1].value.pdb_int = (gint32) pdb_image_to_id (gdisp->gimage);
  args[2].arg_type = PDB_DRAWABLE;
  args[2].value.pdb_int = (gint32) gimage_active_drawable (gdisp->gimage)->ID;

  plug_in_run (proc_rec, args, 3, FALSE, TRUE, gdisp->ID);

  g_free (args);
}

static void
text_button_press (Tool           *tool,
		   GdkEventButton *bevent,
		   gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Layer *layer;
  TextTool *text_tool;

  gdisp = gdisp_ptr;

  text_tool = tool->private;
  text_tool->gdisp_ptr = gdisp_ptr;

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &text_tool->click_x, &text_tool->click_y,
			       TRUE, 0);

  if ((layer = gimage_pick_correlate_layer (gdisp->gimage,
					    text_tool->click_x,
					    text_tool->click_y)))
    /*  If there is a floating selection, and this aint it, use the move tool  */
    if (layer_is_floating_sel (layer))
      {
	init_edit_selection (tool, gdisp_ptr, bevent, LayerTranslate);
	return;
      }

  if (text_options->use_dyntext)
    {
      text_call_gdyntext (gdisp);
      return;
    }

  if (! text_tool_shell)
    text_create_dialog ();

  if (!GTK_WIDGET_VISIBLE (text_tool_shell))
    gtk_widget_show (text_tool_shell);
}

static void
text_button_release (Tool           *tool,
		     GdkEventButton *bevent,
		     gpointer        gdisp_ptr)
{
  tool->state = INACTIVE;
}

static void
text_motion (Tool           *tool,
	     GdkEventMotion *mevent,
	     gpointer        gdisp_ptr)
{
}

static void
text_cursor_update (Tool           *tool,
		    GdkEventMotion *mevent,
		    gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Layer *layer;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);

  if ((layer = gimage_pick_correlate_layer (gdisp->gimage, x, y)))
    /*  if there is a floating selection, and this aint it...  */
    if (layer_is_floating_sel (layer))
      {
	gdisplay_install_tool_cursor (gdisp, GDK_FLEUR);
	return;
      }

  gdisplay_install_tool_cursor (gdisp, GDK_XTERM);
}

static void
text_control (Tool       *tool,
	      ToolAction  action,
	      gpointer    gdisp_ptr)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (text_tool_shell && GTK_WIDGET_VISIBLE (text_tool_shell))
	gtk_widget_hide (text_tool_shell);
      break;

    default:
      break;
    }
}

static void
text_create_dialog (void)
{
  /* Create the shell */
  text_tool_shell = gtk_font_selection_dialog_new (_("Text Tool"));
  gtk_window_set_wmclass (GTK_WINDOW (text_tool_shell), "text_tool", "Gimp");
  gtk_window_set_title (GTK_WINDOW (text_tool_shell), _("Text Tool"));
  gtk_window_set_policy (GTK_WINDOW (text_tool_shell), FALSE, TRUE, TRUE);
  gtk_window_position (GTK_WINDOW (text_tool_shell), GTK_WIN_POS_MOUSE);
  gtk_widget_set (GTK_WIDGET (text_tool_shell),
		  "GtkWindow::auto_shrink", FALSE,
		  NULL);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (text_tool_shell), "delete_event",
		      GTK_SIGNAL_FUNC (text_delete_callback),
		      NULL);

  /* ok and cancel buttons */
  gtk_signal_connect (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG
				  (text_tool_shell)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC(text_ok_callback),
		      NULL);

  gtk_signal_connect (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG
				  (text_tool_shell)->cancel_button),
		      "clicked", GTK_SIGNAL_FUNC(text_cancel_callback),
		      NULL);

  /* Show the shell */
  gtk_widget_show (text_tool_shell);
}

static void
text_ok_callback (GtkWidget *w,
		  gpointer   client_data)
{
  if (GTK_WIDGET_VISIBLE (text_tool_shell))
    gtk_widget_hide (text_tool_shell);

  if (the_text_tool)
    text_init_render (the_text_tool);
}

static gint
text_delete_callback (GtkWidget *w,
		      GdkEvent  *e,
		      gpointer   client_data)
{
  text_cancel_callback (w, client_data);
  
  return TRUE;
}

static void
text_cancel_callback (GtkWidget *w,
		      gpointer   client_data)
{
  if (GTK_WIDGET_VISIBLE (text_tool_shell))
    gtk_widget_hide (text_tool_shell);
}

static void
text_init_render (TextTool *text_tool)
{
  GDisplay *gdisp;
  char *fontname;
  char *text;
  int antialias = text_options->antialias;

  fontname = gtk_font_selection_dialog_get_font_name
    (GTK_FONT_SELECTION_DIALOG (text_tool_shell));
  if (!fontname)
    return;

  gdisp = (GDisplay *) text_tool->gdisp_ptr;

  /* override the user's antialias setting if this is an indexed image */
  if (gimage_base_type (gdisp->gimage) == INDEXED)
    antialias = FALSE;

  /* If we're anti-aliasing, request a larger font than user specified.
   * This will probably produce a font which isn't available if fonts
   * are not scalable on this particular X server.  TODO: Ideally, should
   * grey out anti-alias on these kinds of servers. */
  if (antialias)
    text_size_multiply(&fontname, SUPERSAMPLE);

  /*  If the text size is specified in points, it's size will be scaled
   *  correctly according to the image's resolution.
   *  FIXME: this currently can't be activated for the PDB, as the text has
   *         to be rendered in the size "text_get_extents" returns.
   *  TODO: add resolution parameters to "text_get_extents"
   */
  text_set_resolution (&fontname,
		       gdisp->gimage->xresolution,
		       gdisp->gimage->yresolution);

  text = gtk_font_selection_dialog_get_preview_text
    (GTK_FONT_SELECTION_DIALOG (text_tool_shell));

  /* strdup it since the render function strtok()s the text */
  text = g_strdup(text);

  text_render (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
	       text_tool->click_x, text_tool->click_y,
	       fontname, text, text_options->border, antialias);

  gdisplays_flush ();

  g_free(fontname);
  g_free(text);
}

static void
text_gdk_image_to_region (GdkImage    *image,
			  int          scale,
			  PixelRegion *textPR)
{
  GdkColor black;
  int black_pixel;
  int pixel;
  int value;
  int scalex, scaley;
  int scale2;
  int x, y;
  int i, j;
  unsigned char * data;

  scale2 = scale * scale;
/* GDK_WINDOWING is defined only with GTk+ 1.3 */
#if !defined(GDK_WINDOWING) || (GDK_WINDOWING == GDK_WINDOWING_X11)
  black.red = black.green = black.blue = 0;
  gdk_colormap_alloc_color (gdk_colormap_get_system (), &black, FALSE, TRUE);
  black_pixel = black.pixel;
#else
  black_pixel = 0;
#endif
  data = textPR->data;

  for (y = 0, scaley = 0; y < textPR->h; y++, scaley += scale)
    {
      for (x = 0, scalex = 0; x < textPR->w; x++, scalex += scale)
	{
	  value = 0;

	  for (i = scaley; i < scaley + scale; i++)
	    for (j = scalex; j < scalex + scale; j++)
	      {
		pixel = gdk_image_get_pixel (image, j, i);
		if (pixel == black_pixel)
		  value ++;
	      }

	  /*  store the alpha value in the data  */
	  *data++= (unsigned char) ((value * 255) / scale2);

	}
    }
}

GimpLayer *
text_render (GimpImage *gimage,
	     GimpDrawable *drawable,
	     int     text_x,
	     int     text_y,
	     char   *fontname,
	     char   *text,
	     int     border,
	     int     antialias)
{
  GdkFont *font;
  GdkPixmap *pixmap;
  GdkImage *image;
  GdkGC *gc;
  GdkColor black, white;
  Layer *layer;
  TileManager *mask, *newmask;
  PixelRegion textPR, maskPR;
  int layer_type;
  unsigned char color[MAX_CHANNELS];
  char *str;
  int nstrs;
  int crop;
  int line_width, line_height;
  int pixmap_width, pixmap_height;
  int text_width, text_height;
  int width, height;
  int x, y, k;
  void * pr;

  /*  determine the layer type  */
  if (drawable)
    layer_type = drawable_type_with_alpha (drawable);
  else
    layer_type = gimage_base_type_with_alpha (gimage);

  /* scale the text based on the antialiasing amount */
  if (antialias)
    antialias = SUPERSAMPLE;
  else
    antialias = 1;

  /* Dont crop the text if border is negative */
  crop = (border >= 0);
  if (!crop) border = 0;

  /* load the font in */
  gdk_error_warnings = 0;
  gdk_error_code = 0;
  font = gdk_font_load (fontname);
  gdk_error_warnings = 1;
  if (!font || (gdk_error_code == -1))
  {
      g_message(_("Font '%s' not found.%s"),
		fontname,
		antialias? _("\nIf you don't have scalable fonts, "
		"try turning off antialiasing in the tool options.") : "");
      return NULL;
  }

  /* determine the bounding box of the text */
  width = -1;
  height = 0;
  line_height = font->ascent + font->descent;

  nstrs = 0;
  str = strtok (text, "\n");
  while (str)
    {
      nstrs += 1;

      /* gdk_string_measure will give the correct width of the
       *  string. However, we'll add a little "fudge" factor just
       *  to be sure.
       */
      line_width = gdk_string_measure (font, str) + 5;
      if (line_width > width)
	width = line_width;
      height += line_height;

      str = strtok (NULL, "\n");
    }

  /* We limit the largest pixmap we create to approximately 200x200.
   * This is approximate since it depends on the amount of antialiasing.
   * Basically, we want the width and height to be divisible by the antialiasing
   *  amount. (Which lies in the range 1-10).
   * This avoids problems on some X-servers (Xinside) which have problems
   *  with large pixmaps. (Specifically pixmaps which are larger - width
   *  or height - than the screen).
   */
  pixmap_width = TILE_WIDTH * antialias;
  pixmap_height = TILE_HEIGHT * antialias;

  /* determine the actual text size based on the amount of antialiasing */
  text_width = width / antialias;
  text_height = height / antialias;

  /* create the pixmap of depth 1 */
  pixmap = gdk_pixmap_new (NULL, pixmap_width, pixmap_height, 1);

  /* create the gc */
  gc = gdk_gc_new (pixmap);
  gdk_gc_set_font (gc, font);

  /*  get black and white pixels for this gdisplay  */
  black.red = black.green = black.blue = 0;
  white.red = white.green = white.blue = 65535;
#if !defined(GDK_WINDOWING) || (GDK_WINDOWING == GDK_WINDOWING_X11)
  gdk_colormap_alloc_color (gdk_colormap_get_system (), &black, FALSE, TRUE);
  gdk_colormap_alloc_color (gdk_colormap_get_system (), &white, FALSE, TRUE);
#else
  black.pixel = 0;
  white.pixel = 1;
#endif

  /* Render the text into the pixmap.
   * Since the pixmap may not fully bound the text (because we limit its size)
   *  we must tile it around the texts actual bounding box.
   */
  mask = tile_manager_new (text_width, text_height, 1);
  pixel_region_init (&maskPR, mask, 0, 0, text_width, text_height, TRUE);

  for (pr = pixel_regions_register (1, &maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      /* erase the pixmap */
      gdk_gc_set_foreground (gc, &white);
      gdk_draw_rectangle (pixmap, gc, 1, 0, 0, pixmap_width, pixmap_height);
      gdk_gc_set_foreground (gc, &black);

      /* adjust the x and y values */
      x = -maskPR.x * antialias;
      y = font->ascent - maskPR.y * antialias;
      str = text;

      for (k = 0; k < nstrs; k++)
	{
	  gdk_draw_string (pixmap, font, gc, x, y, str);
	  str += strlen (str) + 1;
	  y += line_height;
	}

      /* create the GdkImage */
      image = gdk_image_get (pixmap, 0, 0, pixmap_width, pixmap_height);

      if (!image)
	fatal_error ("sanity check failed: could not get gdk image");

      if (image->depth != 1)
	fatal_error ("sanity check failed: image should have 1 bit per pixel");

      /* convert the GdkImage bitmap to a region */
      text_gdk_image_to_region (image, antialias, &maskPR);

      /* free the image */
      gdk_image_destroy (image);
    }

  /*  Crop the mask buffer  */
  newmask = crop ? crop_buffer (mask, border) : mask;
  if (newmask != mask)
    tile_manager_destroy (mask);

  if (newmask && 
      (layer = layer_new (gimage, newmask->width,
			 newmask->height, layer_type,
			 _("Text Layer"), OPAQUE_OPACITY, NORMAL_MODE)))
    {
      /*  color the layer buffer  */
      gimage_get_foreground (gimage, drawable, color);
      color[GIMP_DRAWABLE(layer)->bytes - 1] = OPAQUE_OPACITY;
      pixel_region_init (&textPR, GIMP_DRAWABLE(layer)->tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, TRUE);
      color_region (&textPR, color);

      /*  apply the text mask  */
      pixel_region_init (&textPR, GIMP_DRAWABLE(layer)->tiles, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, TRUE);
      pixel_region_init (&maskPR, newmask, 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height, FALSE);
      apply_mask_to_region (&textPR, &maskPR, OPAQUE_OPACITY);

      /*  Start a group undo  */
      undo_push_group_start (gimage, EDIT_PASTE_UNDO);

      /*  Set the layer offsets  */
      GIMP_DRAWABLE(layer)->offset_x = text_x;
      GIMP_DRAWABLE(layer)->offset_y = text_y;

      /*  If there is a selection mask clear it--
       *  this might not always be desired, but in general,
       *  it seems like the correct behavior.
       */
      if (! gimage_mask_is_empty (gimage))
	channel_clear (gimage_get_mask (gimage));

      /*  If the drawable id is invalid, create a new layer  */
      if (drawable == NULL)
	gimage_add_layer (gimage, layer, -1);
      /*  Otherwise, instantiate the text as the new floating selection */
      else
	floating_sel_attach (layer, drawable);

      /*  end the group undo  */
      undo_push_group_end (gimage);

      tile_manager_destroy (newmask);
    }
  else 
    {
      if (newmask) 
	{
	  g_message (_("text_render: could not allocate image"));
          tile_manager_destroy (newmask);
	}
      layer = NULL;
    }

  /* free the pixmap */
  gdk_pixmap_unref (pixmap);

  /* free the gc */
  gdk_gc_destroy (gc);

  /* free the font */
  gdk_font_unref (font);

  return layer;
}


int
text_get_extents (char *fontname,
		  char *text,
		  int  *width,
		  int  *height,
		  int  *ascent,
		  int  *descent)
{
  GdkFont *font;
  char *str;
  int nstrs;
  int line_width, line_height;

  /* load the font in */
  gdk_error_warnings = 0;
  gdk_error_code = 0;
  font = gdk_font_load (fontname);
  gdk_error_warnings = 1;
  if (!font || (gdk_error_code == -1))
    return FALSE;

  /* determine the bounding box of the text */
  *width = -1;
  *height = 0;
  *ascent = font->ascent;
  *descent = font->descent;
  line_height = *ascent + *descent;

  nstrs = 0;
  str = strtok (text, "\n");
  while (str)
    {
      nstrs += 1;

      /* gdk_string_measure will give the correct width of the
       *  string. However, we'll add a little "fudge" factor just
       *  to be sure.
       */
      line_width = gdk_string_measure (font, str) + 5;
      if (line_width > *width)
	*width = line_width;
      *height += line_height;

      str = strtok (NULL, "\n");
    }

  if (*width < 0)
    return FALSE;
  else
    return TRUE;
}


static void
text_field_edges(char  *fontname,
		 int    field_num,
		 /* RETURNS: */
		 char **start,
		 char **end)
{
  char *t1, *t2;

  t1 = fontname;

  while (*t1 && (field_num >= 0))
    if (*t1++ == '-')
      field_num--;

  t2 = t1;
  while (*t2 && (*t2 != '-'))
    t2++;

  *start = t1;
  *end   = t2;
}

/* convert sizes back to text */
#define TO_TXT(x) \
{						\
  if (x >= 0)					\
      g_snprintf (new_ ## x, 16, "%d", x);	\
  else						\
      g_snprintf (new_ ## x, 16, "*");		\
}

/* Multiply the point and pixel sizes in *fontname by "mul", which
 * must be positive.  If either point or pixel sizes are "*" then they
 * are left untouched.  The memory *fontname is g_free()d, and
 * *fontname is replaced by a fresh allocation of the correct size.
 */
static void
text_size_multiply (char **fontname,
		    int    mul)
{
  char *pixel_str;
  char *point_str;
  char *newfont;
  char *end;
  int pixel = -1;
  int point = -1;
  char new_pixel[16];
  char new_point[16];

  /* slice the font spec around the size fields */
  text_field_edges(*fontname, PIXEL_SIZE, &pixel_str, &end);
  text_field_edges(*fontname, POINT_SIZE, &point_str, &end);

  *(pixel_str - 1) = 0;
  *(point_str - 1) = 0;

  if (*pixel_str != '*')
    pixel = atoi(pixel_str);

  if (*point_str != '*')
    point = atoi(point_str);

  pixel *= mul;
  point *= mul;

  /* convert the pixel and point sizes back to text */
  TO_TXT(pixel);
  TO_TXT(point);

  newfont = g_strdup_printf("%s-%s-%s%s", *fontname, new_pixel, new_point, end);

  g_free(*fontname);

  *fontname = newfont;
}

static void
text_set_resolution (char   **fontname,
		     double   xresolution,
		     double   yresolution)
{
  char *size_str;
  char *xres_str;
  char *yres_str;
  char *newfont;
  char *end;
  char new_size[16];
  char new_xres[16];
  char new_yres[16];
  double points;

  int size;
  int xres;
  int yres;

  /* get the point size string */
  text_field_edges (*fontname, POINT_SIZE, &size_str, &end);

  /* don't set the resolution if the point size is unspecified */
  if (xresolution < GIMP_MIN_RESOLUTION ||
      yresolution < GIMP_MIN_RESOLUTION ||
      *size_str == '*')
    return;

  points = atof (size_str);

  /*  X allows only integer resolution values, so we do some
   *  ugly calculations (starting with yres because the size of
   *  a font is it's height)
   */
  if (yresolution < 1.0)
    {
      points /= (1.0 / yresolution);
      xresolution *= (1.0 / yresolution);
      yresolution = 1.0;
    }

  /*  res may be != (int) res
   *  (important only for very small resolutions)
   */
  points *= yresolution / (double) (int) yresolution;
  xresolution /= yresolution / (double) (int) yresolution;

  /* finally, if xres became invalid by the above calculations */
  xresolution = BOUNDS (xresolution, 1.0, GIMP_MAX_RESOLUTION);

  /* slice the font spec around the resolution fields */
  text_field_edges (*fontname, XRESOLUTION, &xres_str, &end);
  text_field_edges (*fontname, YRESOLUTION, &yres_str, &end);

  *(size_str - 1) = 0;
  *(xres_str - 1) = 0;
  *(yres_str - 1) = 0;

  /* convert the resolutions to text */
  size = (int) points;
  xres = (int) xresolution;
  yres = (int) yresolution;

  TO_TXT (size);
  TO_TXT (xres);
  TO_TXT (yres);

  newfont = g_strdup_printf ("%s-%s-%s-%s%s",
			     *fontname, new_size, new_xres, new_yres, end);

  g_free (*fontname);

  *fontname = newfont;
}

#undef TO_TXT
