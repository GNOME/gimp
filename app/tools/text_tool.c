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
#include <stdio.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkprivate.h>
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
#include "tools.h"
#include "undo.h"

#include "tile_manager_pvt.h"
#include "drawable_pvt.h"

#include "libgimp/gimpintl.h"

#define PIXELS 0
#define POINTS 1

#define FOUNDRY    0
#define FAMILY     1
#define WEIGHT     2
#define SLANT      3
#define SET_WIDTH  4
#define PIXEL_SIZE 6
#define POINT_SIZE 7
#define SPACING   10
#define REGISTRY  12
#define ENCODING  13

#define SUPERSAMPLE 3



typedef struct _TextTool TextTool;
struct _TextTool
{
  TextToolOptions options;
  GtkWidget *shell;
  GtkWidget *antialias_toggle;
  int click_x;
  int click_y;
  void *gdisp_ptr;
};



static void       text_button_press       (Tool *, GdkEventButton *, gpointer);
static void       text_button_release     (Tool *, GdkEventButton *, gpointer);
static void       text_motion             (Tool *, GdkEventMotion *, gpointer);
static void       text_cursor_update      (Tool *, GdkEventMotion *, gpointer);
static void       text_control            (Tool *, int, gpointer);

static void       text_antialias_update   (GtkWidget *, gpointer);
static void       text_spinbutton_update  (GtkWidget *, gpointer);
static void       text_create_dialog      (TextTool *);
static void       text_ok_callback        (GtkWidget *, gpointer);
static void       text_cancel_callback    (GtkWidget *, gpointer);
static gint       text_delete_callback    (GtkWidget *, GdkEvent *, gpointer);

static void       text_init_render        (TextTool *);
static void       text_gdk_image_to_region (GdkImage *, int, PixelRegion *);
static char*      text_get_field          (char *, int);
static int        text_get_extents        (char *, char *, int *, int *, int *, int *);
static int        text_get_xlfd           (double, int, char *, char *, char *,
                                           char *, char *, char *, char *,
                                           char *, char *);
static void       text_size_multiply      (char **fontname, int);
static Layer *    text_render             (GImage *, GimpDrawable *, int, int, char *, char *, int, int);

static Argument * text_tool_invoker                      (Argument *);
static Argument * text_tool_invoker_ext                  (Argument *);
static Argument * text_tool_invoker_fontname             (Argument *);
static Argument * text_tool_get_extents_invoker          (Argument *);
static Argument * text_tool_get_extents_invoker_ext      (Argument *);
static Argument * text_tool_get_extents_invoker_fontname (Argument *);

static void       init_text_options       (TextToolOptions *);

static TextTool *the_text_tool = NULL;


Tool*
tools_new_text ()
{
  Tool * tool;

  tool = g_malloc (sizeof (Tool));
  if (!the_text_tool)
    {
      the_text_tool = g_malloc (sizeof (TextTool));
      init_text_options(&the_text_tool->options);
      the_text_tool->shell = NULL;
    }

  tool->type = TEXT;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /* Do not allow scrolling */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) the_text_tool;
  tool->button_press_func = text_button_press;
  tool->button_release_func = text_button_release;
  tool->motion_func = text_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = text_cursor_update;
  tool->control_func = text_control;
  tool->preserve = TRUE;

  return tool;
}

void
tools_free_text (Tool *tool)
{
}


static void
init_text_options(TextToolOptions *options)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkWidget *antialias_toggle;
  GtkAdjustment *adj;

  /*  the new options structure  */
  options->antialias = TRUE;
  options->border    = 0;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);

  /*  the main label  */
  label = gtk_label_new (_("Text Options"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* antialias toggle */
  antialias_toggle = gtk_check_button_new_with_label(_("Antialiasing"));
  gtk_box_pack_start(GTK_BOX(vbox), antialias_toggle,
		     FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(antialias_toggle), "toggled",
		     (GtkSignalFunc) text_antialias_update,
		     &options->antialias);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(antialias_toggle),
			      options->antialias);
  gtk_widget_show(antialias_toggle);

  /* Create the border hbox, border spinner, and label  */
  hbox = gtk_hbox_new(FALSE, 2);
  label = gtk_label_new (_("Border: "));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  adj = (GtkAdjustment *) gtk_adjustment_new(options->border, 0.0,
					     32767.0, 1.0, 50.0, 0.0);
  spinbutton = gtk_spin_button_new(adj, 1.0, 0.0);
  gtk_spin_button_set_shadow_type(GTK_SPIN_BUTTON(spinbutton),
				  GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinbutton), TRUE);
  gtk_widget_set_usize(spinbutton, 75, 0);
  gtk_signal_connect(GTK_OBJECT (spinbutton), "changed",
		     (GtkSignalFunc) text_spinbutton_update,
		     &options->border);
  gtk_box_pack_start(GTK_BOX(hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show(spinbutton);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  /* Register this selection options widget with the main tools
   * options dialog */

  tools_register_options (TEXT, vbox);
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

  if ((layer = gimage_pick_correlate_layer (gdisp->gimage, text_tool->click_x, text_tool->click_y)))
    /*  If there is a floating selection, and this aint it, use the move tool  */
    if (layer_is_floating_sel (layer))
      {
	init_edit_selection (tool, gdisp_ptr, bevent, LayerTranslate);
	return;
      }

  if (!text_tool->shell)
    text_create_dialog (text_tool);

  if (!GTK_WIDGET_VISIBLE (text_tool->shell))
    gtk_widget_show (text_tool->shell);
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
text_control (Tool     *tool,
	      int       action,
	      gpointer  gdisp_ptr)
{
  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (the_text_tool->shell && GTK_WIDGET_VISIBLE (the_text_tool->shell))
	gtk_widget_hide (the_text_tool->shell);
      break;
    }
}



static void
text_create_dialog (TextTool *text_tool)
{
  /* Create the shell and vertical & horizontal boxes */
  text_tool->shell = gtk_font_selection_dialog_new (_("Text Tool"));
  gtk_window_set_wmclass (GTK_WINDOW (text_tool->shell), "text_tool", "Gimp");
  gtk_window_set_title (GTK_WINDOW (text_tool->shell), _("Text Tool"));
  gtk_window_set_policy (GTK_WINDOW (text_tool->shell), FALSE, TRUE, TRUE);
  gtk_window_position (GTK_WINDOW (text_tool->shell), GTK_WIN_POS_MOUSE);
  gtk_widget_set (GTK_WIDGET (text_tool->shell),
		  "GtkWindow::auto_shrink", FALSE,
		  NULL);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (text_tool->shell), "delete_event",
		      GTK_SIGNAL_FUNC (text_delete_callback),
		      text_tool);

  /* ok and cancel buttons */
  gtk_signal_connect (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG
				  (text_tool->shell)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC(text_ok_callback),
		      text_tool);
  
  gtk_signal_connect (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG
				  (text_tool->shell)->cancel_button),
		      "clicked", GTK_SIGNAL_FUNC(text_cancel_callback),
		      text_tool);

  /* Show the shell */
  gtk_widget_show (text_tool->shell);
}

static void
text_ok_callback (GtkWidget *w,
		  gpointer   client_data)
{
  TextTool * text_tool;

  text_tool = (TextTool *) client_data;

  if (GTK_WIDGET_VISIBLE (text_tool->shell))
    gtk_widget_hide (text_tool->shell);

  text_init_render (text_tool);
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
  TextTool * text_tool;

  text_tool = (TextTool *) client_data;

  if (GTK_WIDGET_VISIBLE (text_tool->shell))
    gtk_widget_hide (text_tool->shell);
}


static void
text_antialias_update (GtkWidget *w,
		       gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}


static void
text_spinbutton_update (GtkWidget *widget,
			gpointer   data)
{
  int *val;

  val = data;
  *val = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
}



static void
text_init_render (TextTool *text_tool)
{
  GDisplay *gdisp;
  char *fontname;
  char *text;
  int antialias = text_tool->options.antialias;

  fontname = gtk_font_selection_dialog_get_font_name(
    GTK_FONT_SELECTION_DIALOG( text_tool->shell));
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

  text = gtk_font_selection_dialog_get_preview_text(
    GTK_FONT_SELECTION_DIALOG( text_tool->shell));

  /* strdup it since the render function strtok()s the text */
  text = g_strdup(text);

  text_render (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
	       text_tool->click_x, text_tool->click_y,
	       fontname, text, text_tool->options.border, antialias);

  gdisplays_flush ();

  g_free(fontname);
  g_free(text);
}

static void
text_gdk_image_to_region (GdkImage    *image,
			  int          scale,
			  PixelRegion *textPR)
{
  int black_pixel;
  int pixel;
  int value;
  int scalex, scaley;
  int scale2;
  int x, y;
  int i, j;
  unsigned char * data;

  scale2 = scale * scale;
  black_pixel = BlackPixel (DISPLAY, DefaultScreen (DISPLAY));
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

static Layer *
text_render (GImage *gimage,
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
  black.pixel = BlackPixel (DISPLAY, DefaultScreen (DISPLAY));
  white.pixel = WhitePixel (DISPLAY, DefaultScreen (DISPLAY));

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
	fatal_error (_("sanity check failed: could not get gdk image"));

      if (image->depth != 1)
	fatal_error (_("sanity check failed: image should have 1 bit per pixel"));

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


static int
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
text_field_edges(char *fontname,
		int   field_num,
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

static char*
text_get_field (char *fontname,
		int   field_num)
{
  char *t1, *t2;
  char *field;

  /* we assume this is a valid fontname...that is, it has 14 fields */
  text_field_edges(fontname, field_num, &t1, &t2);

  if (t1 != t2)
    {
      field = g_malloc (1 + (long) t2 - (long) t1);
      strncpy (field, t1, (long) t2 - (long) t1);
      field[(long) t2 - (long) t1] = 0;
      return field;
    }

  return g_strdup ("(nil)");
}


/* Multiply the point and pixel sizes in *fontname by "mul", which
 * must be positive.  If either point or pixel sizes are "*" then they
 * are left untouched.  The memory *fontname is g_free()d, and
 * *fontname is replaced by a fresh allocation of the correct size. */
static void
text_size_multiply(char **fontname, int mul)
{
  char *pixel_str;
  char *point_str;
  char *newfont;
  char *end;
  int pixel = -1;
  int point = -1;
  char new_pixel[12];
  char new_point[12];

  /* slice the font spec around the size fields */
  text_field_edges(*fontname, PIXEL_SIZE, &pixel_str, &end);
  text_field_edges(*fontname, POINT_SIZE, &point_str, &end);

  *(pixel_str-1) = 0;
  *(point_str-1) = 0;

  if (*pixel_str != '*')
    pixel = atoi(pixel_str);

  if (*point_str != '*')
    point = atoi(point_str);

  pixel *= mul;
  point *= mul;

  /* convert the pixel and point sizes back to text */
#define TO_TXT(x) \
do {						\
  if (x >= 0)					\
      sprintf(new_ ## x, "%d", x);		\
  else						\
      sprintf(new_ ## x, "*");			\
} while(0)

  TO_TXT(pixel);
  TO_TXT(point);
#undef TO_TXT

  newfont = g_strdup_printf("%s-%s-%s%s", *fontname, new_pixel, new_point, end);

  g_free(*fontname);

  *fontname = newfont;
}


static int
text_get_xlfd (double  size,
               int     size_type,
               char   *foundry,
               char   *family,
               char   *weight,
               char   *slant,
               char   *set_width,
               char   *spacing,
               char   *registry,
               char   *encoding,
               char   *fontname)
{
  char pixel_size[12], point_size[12];

  if (size > 0)
    {
      switch (size_type)
        {
        case PIXELS:
          sprintf (pixel_size, "%d", (int) size);
          sprintf (point_size, "*");
          break;
        case POINTS:
          sprintf (pixel_size, "*");
          sprintf (point_size, "%d", (int) (size * 10));
          break;
        }

      /* create the fontname */
      sprintf (fontname, "-%s-%s-%s-%s-%s-*-%s-%s-75-75-%s-*-%s-%s",
               foundry,
               family,
               weight,
               slant,
               set_width,
               pixel_size, point_size,
               spacing,
               registry, encoding);
      return TRUE;
    }
  else
    return FALSE;
}



/****************************************/
/*  The text_tool procedure definition  */
ProcArg text_tool_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the affected drawable: (-1 for a new text layer)"
  },
  { PDB_FLOAT,
    "x",
    "the x coordinate for the left side of text bounding box"
  },
  { PDB_FLOAT,
    "y",
    "the y coordinate for the top of text bounding box"
  },
  { PDB_STRING,
    "text",
    "the text to generate"
  },
  { PDB_INT32,
    "border",
    "the size of the border: border >= 0"
  },
  { PDB_INT32,
    "antialias",
    "generate antialiased text"
  },
  { PDB_FLOAT,
    "size",
    "the size of text in either pixels or points"
  },
  { PDB_INT32,
    "size_type",
    "the units of the specified size: { PIXELS (0), POINTS (1) }"
  },
  { PDB_STRING,
    "foundry",
    "the font foundry, \"*\" for any"
  },
  { PDB_STRING,
    "family",
    "the font family, \"*\" for any"
  },
  { PDB_STRING,
    "weight",
    "the font weight, \"*\" for any"
  },
  { PDB_STRING,
    "slant",
    "the font slant, \"*\" for any"
  },
  { PDB_STRING,
    "set_width",
    "the font set-width parameter, \"*\" for any"
  },
  { PDB_STRING,
    "spacing",
    "the font spacing, \"*\" for any"
  }
};

ProcArg text_tool_out_args[] =
{
  { PDB_LAYER,
    "text_layer",
    "the new text layer"
  }
};

ProcRecord text_tool_proc =
{
  "gimp_text",
  "Add text at the specified location as a floating selection or a new layer.",
  "This tool requires font information in the form of seven parameters: {size, foundry, family, weight, slant, set_width, spacing}.  The font size can either be specified in units of pixels or points, and the appropriate metric is specified using the size_type "
  "argument.  The x and y parameters together control the placement of the new text by specifying the upper left corner of the text bounding box.  If the antialias parameter is non-zero, the generated text will blend more smoothly with underlying layers. " 
  "This option requires more time and memory to compute than non-antialiased text; the resulting floating selection or layer, however, will require the same amount of memory with or without antialiasing.  If the specified drawable parameter is valid, the "
  "text will be created as a floating selection attached to the drawable.  If the drawable parameter is not valid (-1), the text will appear as a new layer.  Finally, a border can be specified around the final rendered text.  The border is measured in pixels.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  15,
  text_tool_args,

  /*  Output arguments  */
  1,
  text_tool_out_args,

  /*  Exec method  */
  { { text_tool_invoker } },
};

/********************************************/
/*  The text_tool_ext procedure definition  */
ProcArg text_tool_args_ext[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the affected drawable: (-1 for a new text layer)"
  },
  { PDB_FLOAT,
    "x",
    "the x coordinate for the left side of text bounding box"
  },
  { PDB_FLOAT,
    "y",
    "the y coordinate for the top of text bounding box"
  },
  { PDB_STRING,
    "text",
    "the text to generate"
  },
  { PDB_INT32,
    "border",
    "the size of the border: border >= 0"
  },
  { PDB_INT32,
    "antialias",
    "generate antialiased text"
  },
  { PDB_FLOAT,
    "size",
    "the size of text in either pixels or points"
  },
  { PDB_INT32,
    "size_type",
    "the units of the specified size: { PIXELS (0), POINTS (1) }"
  },
  { PDB_STRING,
    "foundry",
    "the font foundry, \"*\" for any"
  },
  { PDB_STRING,
    "family",
    "the font family, \"*\" for any"
  },
  { PDB_STRING,
    "weight",
    "the font weight, \"*\" for any"
  },
  { PDB_STRING,
    "slant",
    "the font slant, \"*\" for any"
  },
  { PDB_STRING,
    "set_width",
    "the font set-width parameter, \"*\" for any"
  },
  { PDB_STRING,
    "spacing",
    "the font spacing, \"*\" for any"
  },
  { PDB_STRING,
    "registry",
    "the font registry, \"*\" for any"
  },
  { PDB_STRING,
    "encoding",
    "the font encoding, \"*\" for any"
  }
};

ProcArg text_tool_out_args_ext[] =
{
  { PDB_LAYER,
    "text_layer",
    "the new text layer"
  }
};

ProcRecord text_tool_proc_ext =
{
  "gimp_text_ext",
  "Add text at the specified location as a floating selection or a new layer.",
  "This tool requires font information in the form of nine parameters: {size, foundry, family, weight, slant, set_width, spacing, registry, encoding}.  The font size can either be specified in units of pixels or points, and the appropriate metric is specified using the size_type "
  "argument.  The x and y parameters together control the placement of the new text by specifying the upper left corner of the text bounding box.  If the antialias parameter is non-zero, the generated text will blend more smoothly with underlying layers. " 
  "This option requires more time and memory to compute than non-antialiased text; the resulting floating selection or layer, however, will require the same amount of memory with or without antialiasing.  If the specified drawable parameter is valid, the "
  "text will be created as a floating selection attached to the drawable.  If the drawable parameter is not valid (-1), the text will appear as a new layer.  Finally, a border can be specified around the final rendered text.  The border is measured in pixels.",
  "Martin Edlman",
  "Spencer Kimball & Peter Mattis",
  "1998",
  PDB_INTERNAL,
    
  /*  Input arguments  */
  17,
  text_tool_args_ext,

  /*  Output arguments  */
  1,
  text_tool_out_args_ext,

  /*  Exec method  */
  { { text_tool_invoker_ext } },
};

/*************************************************/
/*  The text_tool_fontname procedure definition  */
ProcArg text_tool_args_fontname[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the affected drawable: (-1 for a new text layer)"
  },
  { PDB_FLOAT,
    "x",
    "the x coordinate for the left side of text bounding box"
  },
  { PDB_FLOAT,
    "y",
    "the y coordinate for the top of text bounding box"
  },
  { PDB_STRING,
    "text",
    "the text to generate"
  },
  { PDB_INT32,
    "border",
    "the size of the border: border >= 0"
  },
  { PDB_INT32,
    "antialias",
    "generate antialiased text"
  },
  { PDB_FLOAT,
    "size",
    "the size of text in either pixels or points"
  },
  { PDB_INT32,
    "size_type",
    "the units of the specified size: { PIXELS (0), POINTS (1) }"
  },
  { PDB_STRING,
    "fontname",
    "the fontname (conforming to the X Logical Font Description Conventions)"
  }
};

ProcArg text_tool_out_args_fontname[] =
{
  { PDB_LAYER,
    "text_layer",
    "the new text layer"
  }
};

ProcRecord text_tool_proc_fontname =
{
  "gimp_text_fontname",
  "Add text at the specified location as a floating selection or a new layer.",
  "This tool requires font information as a fontname conforming to the 'X Logical Font Description Conventions'. You can specify the fontsize in units of pixels or points, and the appropriate metric is specified using the size_type argument. The fontsize specified in the fontname is silently ignored."
  "The x and y parameters together control the placement of the new text by specifying the upper left corner of the text bounding box.  If the antialias parameter is non-zero, the generated text will blend more smoothly with underlying layers.  "
  "This option requires more time and memory to compute than non-antialiased text; the resulting floating selection or layer, however, will require the same amount of memory with or without antialiasing.  If the specified drawable parameter is valid, the "
  "text will be created as a floating selection attached to the drawable.  If the drawable parameter is not valid (-1), the text will appear as a new layer.  Finally, a border can be specified around the final rendered text.  The border is measured in pixels.",
  "Martin Edlman, Sven Neumann",
  "Spencer Kimball & Peter Mattis",
  "1998",
  PDB_INTERNAL,
    
  /*  Input arguments  */
  10,
  text_tool_args_fontname,

  /*  Output arguments  */
  1,
  text_tool_out_args_fontname,

  /*  Exec method  */
  { { text_tool_invoker_fontname } },
};

/**********************/
/*  TEXT_GET_EXTENTS  */

ProcArg text_tool_get_extents_args[] =
{
  { PDB_STRING,
    "text",
    "the text to generate"
  },
  { PDB_FLOAT,
    "size",
    "the size of text in either pixels or points"
  },
  { PDB_INT32,
    "size_type",
    "the units of the specified size: { PIXELS (0), POINTS (1) }"
  },
  { PDB_STRING,
    "foundry",
    "the font foundry, \"*\" for any"
  },
  { PDB_STRING,
    "family",
    "the font family, \"*\" for any"
  },
  { PDB_STRING,
    "weight",
    "the font weight, \"*\" for any"
  },
  { PDB_STRING,
    "slant",
    "the font slant, \"*\" for any"
  },
  { PDB_STRING,
    "set_width",
    "the font set-width parameter, \"*\" for any"
  },
  { PDB_STRING,
    "spacing",
    "the font spacing, \"*\" for any"
  }
};

ProcArg text_tool_get_extents_out_args[] =
{
  { PDB_INT32,
    "width",
    "the width of the specified text"
  },
  { PDB_INT32,
    "height",
    "the height of the specified text"
  },
  { PDB_INT32,
    "ascent",
    "the ascent of the specified font"
  },
  { PDB_INT32,
    "descent",
    "the descent of the specified font"
  }
};

ProcRecord text_tool_get_extents_proc =
{
  "gimp_text_get_extents",
  "Get extents of the bounding box for the specified text",
  "This tool returns the width and height of a bounding box for the specified text string with the specified font information.  Ascent and descent for the specified font are returned as well.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  9,
  text_tool_get_extents_args,

  /*  Output arguments  */
  4,
  text_tool_get_extents_out_args,

  /*  Exec method  */
  { { text_tool_get_extents_invoker } },
};

/**************************/
/*  TEXT_GET_EXTENTS_EXT  */
ProcArg text_tool_get_extents_args_ext[] =
{
  { PDB_STRING,
    "text",
    "the text to generate"
  },
  { PDB_FLOAT,
    "size",
    "the size of text in either pixels or points"
  },
  { PDB_INT32,
    "size_type",
    "the units of the specified size: { PIXELS (0), POINTS (1) }"
  },
  { PDB_STRING,
    "foundry",
    "the font foundry, \"*\" for any"
  },
  { PDB_STRING,
    "family",
    "the font family, \"*\" for any"
  },
  { PDB_STRING,
    "weight",
    "the font weight, \"*\" for any"
  },
  { PDB_STRING,
    "slant",
    "the font slant, \"*\" for any"
  },
  { PDB_STRING,
    "set_width",
    "the font set-width parameter, \"*\" for any"
  },
  { PDB_STRING,
    "spacing",
    "the font spacing, \"*\" for any"
  },
  { PDB_STRING,
    "registry",
    "the font registry, \"*\" for any"
  },
  { PDB_STRING,
    "encoding",
    "the font encoding, \"*\" for any"
  }
};

ProcArg text_tool_get_extents_out_args_ext[] =
{
  { PDB_INT32,
    "width",
    "the width of the specified text"
  },
  { PDB_INT32,
    "height",
    "the height of the specified text"
  },
  { PDB_INT32,
    "ascent",
    "the ascent of the specified font"
  },
  { PDB_INT32,
    "descent",
    "the descent of the specified font"
  }
};

ProcRecord text_tool_get_extents_proc_ext =
{
  "gimp_text_get_extents_ext",
  "Get extents of the bounding box for the specified text",
  "This tool returns the width and height of a bounding box for the specified text string with the specified font information.  Ascent and descent for the specified font are returned as well.",
  "Martin Edlman",
  "Spencer Kimball & Peter Mattis",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  11,
  text_tool_get_extents_args_ext,

  /*  Output arguments  */
  4,
  text_tool_get_extents_out_args_ext,

  /*  Exec method  */
  { { text_tool_get_extents_invoker_ext } },
};

/*******************************/
/*  TEXT_GET_EXTENTS_FONTNAME  */
ProcArg text_tool_get_extents_args_fontname[] =
{
  { PDB_STRING,
    "text",
    "the text to generate"
  },
  { PDB_FLOAT,
    "size",
    "the size of text in either pixels or points"
  },
  { PDB_INT32,
    "size_type",
    "the units of the specified size: { PIXELS (0), POINTS (1) }"
  },
  { PDB_STRING,
    "fontname",
    "the fontname (conforming to the X Logical Font Description Conventions)"
  }  
};

ProcArg text_tool_get_extents_out_args_fontname[] =
{
  { PDB_INT32,
    "width",
    "the width of the specified text"
  },
  { PDB_INT32,
    "height",
    "the height of the specified text"
  },
  { PDB_INT32,
    "ascent",
    "the ascent of the specified font"
  },
  { PDB_INT32,
    "descent",
    "the descent of the specified font"
  }
};

ProcRecord text_tool_get_extents_proc_fontname =
{
  "gimp_text_get_extents_fontname",
  "Get extents of the bounding box for the specified text",
  "This tool returns the width and height of a bounding box for the specified text string with the specified font information.  Ascent and descent for the specified font are returned as well.",
  "Martin Edlman, Sven Neumann",
  "Spencer Kimball & Peter Mattis",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  text_tool_get_extents_args_fontname,

  /*  Output arguments  */
  4,
  text_tool_get_extents_out_args_fontname,

  /*  Exec method  */
  { { text_tool_get_extents_invoker_fontname } },
};


static Argument *
text_tool_invoker (Argument *args)
{
  int i;
  Argument argv[17];
 
  for (i=0; i<15; i++)
    argv[i] = args[i];
  argv[15].arg_type = PDB_STRING;
  argv[15].value.pdb_pointer = (gpointer)"*";
  argv[16].arg_type = PDB_STRING;
  argv[16].value.pdb_pointer = (gpointer)"*";
  return text_tool_invoker_ext (argv);
}

static Argument *
text_tool_invoker_fontname (Argument *args)
{
  int i;
  char *fontname;
  Argument argv[17];
  
  for (i=0; i<9; i++)
    argv[i] = args[i];

  fontname = (char *) args[9].value.pdb_pointer;

  /*
    Should check for a valid fontname here, probably using
     if (!text_is_xlfd_font_name (fontname)) return
  */

  argv[9].arg_type = PDB_STRING;
  argv[9].value.pdb_pointer = text_get_field (fontname, FOUNDRY);
  argv[10].arg_type = PDB_STRING;
  argv[10].value.pdb_pointer = text_get_field (fontname, FAMILY);
  argv[11].arg_type = PDB_STRING;
  argv[11].value.pdb_pointer = text_get_field (fontname, WEIGHT);
  argv[12].arg_type = PDB_STRING;
  argv[12].value.pdb_pointer = text_get_field (fontname, SLANT);
  argv[13].arg_type = PDB_STRING;
  argv[13].value.pdb_pointer = text_get_field (fontname, SET_WIDTH);
  argv[14].arg_type = PDB_STRING;
  argv[14].value.pdb_pointer = text_get_field (fontname, SPACING);
  argv[15].arg_type = PDB_STRING;
  argv[15].value.pdb_pointer = text_get_field (fontname, REGISTRY);
  argv[16].arg_type = PDB_STRING;
  argv[16].value.pdb_pointer = text_get_field (fontname, ENCODING);

  return text_tool_invoker_ext (argv);
}

static Argument *
text_tool_invoker_ext (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  Layer *text_layer;
  GimpDrawable *drawable;
  double x, y;
  char *text;
  int border;
  int antialias;
  double size;
  int size_type;
  char *foundry;
  char *family;
  char *weight;
  char *slant;
  char *set_width;
  char *spacing;
  char *registry;
  char *encoding;
  int int_value;
  double fp_value;
  char fontname[2048];
  Argument *return_args;

  text_layer  = NULL;
  drawable    = NULL;
  x           = 0;
  y           = 0;
  text        = NULL;
  border      = FALSE;
  antialias   = FALSE;
  size        = 0;
  size_type   = PIXELS;
  foundry     = NULL;
  family      = NULL;
  weight      = NULL;
  slant       = NULL;
  set_width   = NULL;
  spacing     = NULL;
  registry    = NULL;
  encoding    = NULL;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable && gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  x, y coordinates  */
  if (success)
    {
      x = args[2].value.pdb_float;
      y = args[3].value.pdb_float;
    }
  /*  text  */
  if (success)
    text = (char *) args[4].value.pdb_pointer;
  /*  border  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      if (int_value >= -1)
	border = int_value;
      else
	success = FALSE;
    }
  /*  antialias  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      antialias = (int_value) ? TRUE : FALSE;
    }
  /*  size  */
  if (success)
    {
      fp_value = args[7].value.pdb_float;
      if (fp_value > 0)
	size = fp_value;
      else
	success = FALSE;
    }
  /*  size type  */
  if (success)
    {
      int_value = args[8].value.pdb_int;
      switch (int_value)
	{
	case 0: size_type = PIXELS; break;
	case 1: size_type = POINTS; break;
	default: success = FALSE;
	}
    }
  /*  foundry, family, weight, slant, set_width, spacing, registry, encoding  */
  if (success)
    {
      foundry = (char *) args[9].value.pdb_pointer;
      family = (char *) args[10].value.pdb_pointer;
      weight = (char *) args[11].value.pdb_pointer;
      slant = (char *) args[12].value.pdb_pointer;
      set_width = (char *) args[13].value.pdb_pointer;
      spacing = (char *) args[14].value.pdb_pointer;
      registry = (char *) args[15].value.pdb_pointer;
      encoding = (char *) args[16].value.pdb_pointer;
    }

  /*  increase the size by SUPERSAMPLE if we're antialiasing  */
  if (antialias)
    size *= SUPERSAMPLE;

  /*  attempt to get the xlfd for the font  */
  if (success)
    success = text_get_xlfd (size, size_type, foundry, family, weight,
			     slant, set_width, spacing, registry, encoding, fontname);

  /*  call the text render procedure  */
  if (success)
    success = ((text_layer = text_render (gimage, drawable, x, y, fontname,
					  text, border, antialias)) != NULL);

  return_args = procedural_db_return_args (&text_tool_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(text_layer));

  return return_args;
}


static Argument *
text_tool_get_extents_invoker (Argument *args)
{
  int i;
  Argument argv[11];

  for (i=0; i<9; i++)
    argv[i] = args[i];
  argv[9].arg_type = PDB_STRING;
  argv[9].value.pdb_pointer = (gpointer)"*";
  argv[10].arg_type = PDB_STRING;
  argv[10].value.pdb_pointer = (gpointer)"*";
  return text_tool_get_extents_invoker_ext (argv);
}

static Argument *
text_tool_get_extents_invoker_fontname (Argument *args)
{
  int i;
  gchar *fontname;
  Argument argv[17];
  
  for (i=0; i<3; i++)
    argv[i] = args[i];

  fontname = args[3].value.pdb_pointer;

  argv[3].arg_type = PDB_STRING;
  argv[3].value.pdb_pointer = text_get_field (fontname, FOUNDRY);
  argv[4].arg_type = PDB_STRING;
  argv[4].value.pdb_pointer = text_get_field (fontname, FAMILY);
  argv[5].arg_type = PDB_STRING;
  argv[5].value.pdb_pointer = text_get_field (fontname, WEIGHT);
  argv[6].arg_type = PDB_STRING;
  argv[6].value.pdb_pointer = text_get_field (fontname, SLANT);
  argv[7].arg_type = PDB_STRING;
  argv[7].value.pdb_pointer = text_get_field (fontname, SET_WIDTH);
  argv[8].arg_type = PDB_STRING;
  argv[8].value.pdb_pointer = text_get_field (fontname, SPACING);
  argv[9].arg_type = PDB_STRING;
  argv[9].value.pdb_pointer = text_get_field (fontname, REGISTRY);
  argv[10].arg_type = PDB_STRING;
  argv[10].value.pdb_pointer = text_get_field (fontname, ENCODING);

  return text_tool_get_extents_invoker_ext (argv);
}

static Argument *
text_tool_get_extents_invoker_ext (Argument *args)
{
  int success = TRUE;
  char *text;
  double size;
  int size_type;
  char *foundry;
  char *family;
  char *weight;
  char *slant;
  char *set_width;
  char *spacing;
  char *registry;
  char *encoding;
  int width, height;
  int ascent, descent;
  int int_value;
  double fp_value;
  char fontname[2048];
  Argument *return_args;

  size = 0.0;
  size_type = PIXELS;

  /*  text  */
  if (success)
    text = (char *) args[0].value.pdb_pointer;
  /*  size  */
  if (success)
    {
      fp_value = args[1].value.pdb_float;
      if (fp_value > 0)
	size = fp_value;
      else
	success = FALSE;
    }
  /*  size type  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      switch (int_value)
	{
	case 0: size_type = PIXELS; break;
	case 1: size_type = POINTS; break;
	default: success = FALSE;
	}
    }
  /*  foundry, family, weight, slant, set_width, spacing, registry, encoding  */
  if (success)
    {
      foundry = (char *) args[3].value.pdb_pointer;
      family = (char *) args[4].value.pdb_pointer;
      weight = (char *) args[5].value.pdb_pointer;
      slant = (char *) args[6].value.pdb_pointer;
      set_width = (char *) args[7].value.pdb_pointer;
      spacing = (char *) args[8].value.pdb_pointer;
      registry = (char *) args[9].value.pdb_pointer;
      encoding = (char *) args[10].value.pdb_pointer;
    }

  /*  attempt to get the xlfd for the font  */
  if (success)
    success = text_get_xlfd (size, size_type, foundry, family, weight,
			     slant, set_width, spacing, registry, encoding, fontname);

  /*  call the text render procedure  */
  if (success)
    success = text_get_extents (fontname, text, &width, &height, &ascent, &descent);

  return_args = procedural_db_return_args (&text_tool_get_extents_proc, success);

  if (success)
    {
      return_args[1].value.pdb_int = width;
      return_args[2].value.pdb_int = height;
      return_args[3].value.pdb_int = ascent;
      return_args[4].value.pdb_int = descent;
    }

  return return_args;
}

