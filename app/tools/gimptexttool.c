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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifndef GDK_WINDOWING_WIN32
#include <gdk/gdkx.h>
#endif

#include <gdk/gdkprivate.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile-manager-crop.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpeditselectiontool.h"
#include "gimptexttool.h"
#include "gimptool.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "app_procs.h"
#include "errors.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "plug_in.h"
#include "undo.h"

#include "libgimp/gimpintl.h"

#define WANT_TEXT_BITS
#include "icons.h"


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

typedef struct _TextOptions TextOptions;

struct _TextOptions
{
  GimpToolOptions  tool_options;

  gboolean     antialias;
  gboolean     antialias_d;
  GtkWidget   *antialias_w;

  gint         border;
  gint         border_d;
  GtkObject   *border_w;

  gboolean     use_dyntext;
  gboolean     use_dyntext_d;
  GtkWidget   *use_dyntext_w;
};


static void   gimp_text_tool_class_init      (GimpTextToolClass *klass);
static void   gimp_text_tool_init            (GimpTextTool      *tool);

static void   gimp_text_tool_destroy         (GtkObject       *object);

static void   text_tool_control              (GimpTool        *tool,
					      ToolAction       tool_action,
					      GDisplay        *gdisp);
static void   text_tool_button_press         (GimpTool        *tool,
					      GdkEventButton  *bevent,
					      GDisplay        *gdisp);
static void   text_tool_button_release       (GimpTool        *tool,
					      GdkEventButton  *bevent,
					      GDisplay        *gdisp);
static void   text_tool_cursor_update        (GimpTool        *tool,
					      GdkEventMotion  *mevent,
					      GDisplay        *gdisp);

static TextOptions * text_tool_options_new   (void);
static void          text_tool_options_reset (GimpToolOptions *tool_options);

static void   text_dialog_create             (void);
static void   text_dialog_ok_callback        (GtkWidget       *widget,
					      gpointer         data);
static void   text_dialog_cancel_callback    (GtkWidget       *widget,
					      gpointer         data);
static gint   text_dialog_delete_callback    (GtkWidget       *widget,
					      GdkEvent        *event,
					      gpointer         data);

static void   text_init_render               (GimpTextTool    *text_tool);
static void   text_gdk_image_to_region       (GdkImage        *image,
					      gint             ,
					      PixelRegion     *);
static void   text_size_multiply             (gchar          **fontname,
					      gint             size);
static void   text_set_resolution            (gchar          **fontname,
					      gdouble          xres,
					      gdouble          yres);


/*  local variables  */

static TextOptions   *text_tool_options = NULL;
static GtkWidget     *text_tool_shell   = NULL;

static GimpToolClass *parent_class      = NULL;


/*  functions  */

void
gimp_text_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_TEXT_TOOL,
			      FALSE,
			      "gimp:text_tool",
			      _("Text Tool"),
			      _("Add text to the image"),
			      N_("/Tools/Text"), "T",
			      NULL, "tools/text.html",
			      (const gchar **) text_bits);
}

GtkType
gimp_text_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
	"GimpTextTool",
	sizeof (GimpTextTool),
	sizeof (GimpTextToolClass),
	(GtkClassInitFunc) gimp_text_tool_class_init,
	(GtkObjectInitFunc) gimp_text_tool_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_text_tool_class_init (GimpTextToolClass *klass)
{
  GtkObjectClass *object_class;
  GimpToolClass  *tool_class;

  object_class = (GtkObjectClass *) klass;
  tool_class   = (GimpToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_TOOL);

  object_class->destroy = gimp_text_tool_destroy;

  tool_class->control        = text_tool_control;
  tool_class->button_press   = text_tool_button_press;
  tool_class->button_release = text_tool_button_release;
  tool_class->cursor_update  = text_tool_cursor_update;
}

static void
gimp_text_tool_init (GimpTextTool *text_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (text_tool);
 
  /*  The tool options  */
  if (! text_tool_options)
    {
      text_tool_options = text_tool_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_TEXT_TOOL,
					  (GimpToolOptions *) text_tool_options);
    }

  tool->tool_cursor = GIMP_TEXT_TOOL_CURSOR;

  tool->scroll_lock = TRUE;  /* Disallow scrolling */
}

static void
gimp_text_tool_destroy (GtkObject *object)
{
  if (text_tool_shell)
    gimp_dialog_hide (text_tool_shell);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
text_tool_options_reset (GimpToolOptions *tool_options)
{
  TextOptions *options;

  options = (TextOptions *) tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->antialias_w),
				options->antialias_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->border_w),
			    options->border_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_dyntext_w),
				options->use_dyntext_d);
}

static TextOptions *
text_tool_options_new (void)
{
  TextOptions *options;
  GtkWidget   *vbox;
  GtkWidget   *hbox;
  GtkWidget   *label;
  GtkWidget   *spinbutton;
  GtkWidget   *sep;

  options = g_new0 (TextOptions, 1);
  tool_options_init ((GimpToolOptions *) options,
		     text_tool_options_reset);

  options->antialias   = options->antialias_d   = TRUE;
  options->border      = options->border_d      = 0;
  options->use_dyntext = options->use_dyntext_d = FALSE;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  antialias toggle  */
  options->antialias_w =
    gtk_check_button_new_with_label (_("Antialiasing"));
  gtk_signal_connect (GTK_OBJECT (options->antialias_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
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
  gtk_signal_connect (GTK_OBJECT (options->border_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &options->border);
  spinbutton =
    gtk_spin_button_new (GTK_ADJUSTMENT (options->border_w), 1.0, 0.0);
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
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
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
  g_object_set_data (G_OBJECT (options->use_dyntext_w), "inverse_sensitive",
		       spinbutton);
  g_object_set_data (G_OBJECT (spinbutton), "inverse_sensitive", label);
  g_object_set_data (G_OBJECT (label), "inverse_sensitive",
		       options->antialias_w);

  return options;
}

static void
text_call_gdyntext (GDisplay *gdisp)
{
  ProcRecord *proc_rec;
  Argument   *args;

  /*  find the gDynText PDB record  */
  if ((proc_rec = procedural_db_lookup (gdisp->gimage->gimp,
					"plug_in_dynamic_text")) == NULL)
    {
      g_message ("text_call_gdyntext: gDynText procedure lookup failed");
      return;
    }

  /*  plug-in arguments as if called by <Image>/Filters/...  */
  args = g_new (Argument, 3);
  args[0].arg_type      = GIMP_PDB_INT32;
  args[0].value.pdb_int = RUN_INTERACTIVE;
  args[1].arg_type      = GIMP_PDB_IMAGE;
  args[1].value.pdb_int = (gint32) gimp_image_get_ID (gdisp->gimage);
  args[2].arg_type      = GIMP_PDB_DRAWABLE;
  args[2].value.pdb_int = (gint32) gimp_drawable_get_ID (gimp_image_active_drawable (gdisp->gimage));

  plug_in_run (proc_rec, args, 3, FALSE, TRUE, gdisp->ID);

  g_free (args);
}

static void
text_tool_control (GimpTool   *tool,
		   ToolAction  action,
		   GDisplay   *gdisp)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (text_tool_shell)
	gimp_dialog_hide (text_tool_shell);
      break;

    default:
      break;
    }
}

static void
text_tool_button_press (GimpTool       *tool,
			GdkEventButton *bevent,
			GDisplay       *gdisp)
{
  GimpTextTool *text_tool;
  GimpLayer    *layer;

  text_tool = GIMP_TEXT_TOOL (tool);

  text_tool->gdisp = gdisp;

  tool->state = ACTIVE;
  tool->gdisp = gdisp;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &text_tool->click_x, &text_tool->click_y,
			       TRUE, 0);

  if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage,
						text_tool->click_x,
						text_tool->click_y)))
    /*  If there is a floating selection, and this aint it, use the move tool  */
    if (gimp_layer_is_floating_sel (layer))
      {
	init_edit_selection (tool, gdisp, bevent, EDIT_LAYER_TRANSLATE);

	return;
      }

  if (text_tool_options->use_dyntext)
    {
      text_call_gdyntext (gdisp);
      return;
    }

  if (! text_tool_shell)
    text_dialog_create ();

  if (! GTK_WIDGET_VISIBLE (text_tool_shell))
    gtk_widget_show (text_tool_shell);
}

static void
text_tool_button_release (GimpTool       *tool,
			  GdkEventButton *bevent,
			  GDisplay       *gdisp)
{
  tool->state = INACTIVE;
}

static void
text_tool_cursor_update (GimpTool       *tool,
			 GdkEventMotion *mevent,
			 GDisplay       *gdisp)
{
  GimpLayer *layer;
  gint       x, y;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &x, &y, FALSE, FALSE);

  if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage, x, y)))
    /*  if there is a floating selection, and this aint it...  */
    if (gimp_layer_is_floating_sel (layer))
      {
	gdisplay_install_tool_cursor (gdisp,
				      GDK_FLEUR,
				      GIMP_MOVE_TOOL_CURSOR,
				      GIMP_CURSOR_MODIFIER_NONE);
	return;
      }

  gdisplay_install_tool_cursor (gdisp,
				GDK_XTERM,
				GIMP_TEXT_TOOL_CURSOR,
				GIMP_CURSOR_MODIFIER_NONE);
}

static void
text_dialog_create (void)
{
  text_tool_shell = gtk_font_selection_dialog_new (_("Text Tool"));

  gtk_window_set_wmclass (GTK_WINDOW (text_tool_shell), "text_tool", "Gimp");
  gtk_window_set_policy (GTK_WINDOW (text_tool_shell), FALSE, TRUE, FALSE);
  gtk_window_set_position (GTK_WINDOW (text_tool_shell), GTK_WIN_POS_MOUSE);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (text_tool_shell), "delete_event",
		      GTK_SIGNAL_FUNC (text_dialog_delete_callback),
		      text_tool_shell);

  /* ok and cancel buttons */
  gtk_signal_connect (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG
				  (text_tool_shell)->ok_button), "clicked",
		      GTK_SIGNAL_FUNC (text_dialog_ok_callback),
		      text_tool_shell);

  gtk_signal_connect (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG
				  (text_tool_shell)->cancel_button), "clicked",
		      GTK_SIGNAL_FUNC (text_dialog_cancel_callback),
		      text_tool_shell);

  /* Show the shell */
  gtk_widget_show (text_tool_shell);
}

static void
text_dialog_ok_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpTool *active_tool;

  gimp_dialog_hide (data);

  active_tool = tool_manager_get_active (the_gimp);

  if (active_tool && GIMP_IS_TEXT_TOOL (active_tool))
    {
      text_init_render (GIMP_TEXT_TOOL (active_tool));
    }
}

static gint
text_dialog_delete_callback (GtkWidget *widget,
			     GdkEvent  *event,
			     gpointer   data)
{
  text_dialog_cancel_callback (widget, data);

  return TRUE;
}

static void
text_dialog_cancel_callback (GtkWidget *widget,
			     gpointer   data)
{
  gimp_dialog_hide (GTK_WIDGET (data));
}

static void
text_init_render (GimpTextTool *text_tool)
{
  GDisplay *gdisp;
  gchar    *fontname;
  gchar    *text;
  gboolean  antialias = text_tool_options->antialias;

  fontname = gtk_font_selection_dialog_get_font_name
    (GTK_FONT_SELECTION_DIALOG (text_tool_shell));

  if (! fontname)
    return;

  gdisp = text_tool->gdisp;

  /* override the user's antialias setting if this is an indexed image */
  if (gimp_image_base_type (gdisp->gimage) == INDEXED)
    antialias = FALSE;

  /* If we're anti-aliasing, request a larger font than user specified.
   * This will probably produce a font which isn't available if fonts
   * are not scalable on this particular X server.  TODO: Ideally, should
   * grey out anti-alias on these kinds of servers. */
  if (antialias)
    text_size_multiply (&fontname, SUPERSAMPLE);

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
  text = g_strdup (text);

  text_render (gdisp->gimage, gimp_image_active_drawable (gdisp->gimage),
	       text_tool->click_x, text_tool->click_y,
	       fontname, text, text_tool_options->border, antialias);

  gdisplays_flush ();

  g_free (fontname);
  g_free (text);
}

static void
text_gdk_image_to_region (GdkImage    *image,
			  gint         scale,
			  PixelRegion *textPR)
{
  GdkColor black;
  gint black_pixel;
  gint pixel;
  gint value;
  gint scalex, scaley;
  gint scale2;
  gint x, y;
  gint i, j;
  guchar * data;

  scale2 = scale * scale;

#ifndef GDK_WINDOWING_WIN32
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
	  *data++= (guchar) ((value * 255) / scale2);
	}
    }
}

GimpLayer *
text_render (GimpImage    *gimage,
	     GimpDrawable *drawable,
	     gint          text_x,
	     gint          text_y,
	     gchar        *fontname,
	     gchar        *text,
	     gint          border,
	     gint          antialias)
{
  GdkFont     *font;
  GdkPixmap   *pixmap;
  GdkImage    *image;
  GdkGC       *gc;
  GdkColor     black, white;
  GimpLayer   *layer;
  TileManager *mask, *newmask;
  PixelRegion  textPR, maskPR;
  gint         layer_type;
  guchar       color[MAX_CHANNELS];
  gchar       *str;
  gint         nstrs;
  gboolean     crop;
  gint         line_width, line_height;
  gint         pixmap_width, pixmap_height;
  gint         text_width, text_height;
  gint         width, height;
  gint         x, y, k;
  void        *pr;
#ifndef GDK_WINDOWING_WIN32
  XFontStruct *xfs;
#endif

  /*  determine the layer type  */
  if (drawable)
    layer_type = gimp_drawable_type_with_alpha (drawable);
  else
    layer_type = gimp_image_base_type_with_alpha (gimage);

  /* scale the text based on the antialiasing amount */
  if (antialias)
    antialias = SUPERSAMPLE;
  else
    antialias = 1;

  /* Dont crop the text if border is negative */
  crop = (border >= 0);
  if (!crop)
    border = 0;

  /* load the font in */
  gdk_error_warnings = 0;
  gdk_error_code = 0;
#ifndef GDK_WINDOWING_WIN32
  font = gdk_font_load (fontname);
  if (!font)
    {
      g_message (_("Font '%s' not found."), fontname);
      return NULL;
    }
  xfs = GDK_FONT_XFONT (font);
  if (xfs->min_byte1 != 0 || xfs->max_byte1 != 0)
    {
      gchar *fname;

      gdk_font_unref (font);
      fname = g_strdup_printf ("%s,*", fontname);
      font = gdk_fontset_load (fname);
      g_free (fname);
    }
#else
  /* Just use gdk_fontset_load all the time. IMHO it could be like
   * this on all platforms?
   */
  font = gdk_fontset_load (fontname);
#endif
  gdk_error_warnings = 1;
  if (!font || (gdk_error_code == -1))
    {
      g_message (_("Font '%s' not found.%s"),
		 fontname,
		 antialias > 1 ?
		 _("\nIf you don't have scalable fonts, "
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
#ifndef GDK_WINDOWING_WIN32
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

  for (pr = pixel_regions_register (1, &maskPR); 
       pr != NULL; 
       pr = pixel_regions_process (pr))
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

      if (! image)
	gimp_fatal_error ("%s(): Sanity check failed: could not get gdk image",
			  G_GNUC_FUNCTION);

      if (image->depth != 1)
	gimp_fatal_error ("%s(): Sanity check failed: image should have 1 bit per pixel",
			  G_GNUC_FUNCTION);

      /* convert the GdkImage bitmap to a region */
      text_gdk_image_to_region (image, antialias, &maskPR);

      /* free the image */
      gdk_image_destroy (image);
    }

  /*  Crop the mask buffer  */
  newmask = crop ? tile_manager_crop (mask, border) : mask;
  if (newmask != mask)
    tile_manager_destroy (mask);

  if (newmask && 
      (layer = gimp_layer_new (gimage, 
			       tile_manager_width (newmask),
			       tile_manager_height (newmask), 
			       layer_type,
			       _("Text Layer"), OPAQUE_OPACITY, NORMAL_MODE)))
    {
      /*  color the layer buffer  */
      gimp_image_get_foreground (gimage, drawable, color);
      color[GIMP_DRAWABLE (layer)->bytes - 1] = OPAQUE_OPACITY;
      pixel_region_init (&textPR, GIMP_DRAWABLE (layer)->tiles,
			 0, 0,
			 GIMP_DRAWABLE (layer)->width,
			 GIMP_DRAWABLE (layer)->height, TRUE);
      color_region (&textPR, color);

      /*  apply the text mask  */
      pixel_region_init (&textPR, GIMP_DRAWABLE (layer)->tiles,
			 0, 0,
			 GIMP_DRAWABLE (layer)->width,
			 GIMP_DRAWABLE (layer)->height, TRUE);
      pixel_region_init (&maskPR, newmask,
			 0, 0,
			 GIMP_DRAWABLE (layer)->width,
			 GIMP_DRAWABLE (layer)->height, FALSE);
      apply_mask_to_region (&textPR, &maskPR, OPAQUE_OPACITY);

      /*  Start a group undo  */
      undo_push_group_start (gimage, TEXT_UNDO);

      /*  Set the layer offsets  */
      GIMP_DRAWABLE (layer)->offset_x = text_x;
      GIMP_DRAWABLE (layer)->offset_y = text_y;

      /*  If there is a selection mask clear it--
       *  this might not always be desired, but in general,
       *  it seems like the correct behavior.
       */
      if (! gimage_mask_is_empty (gimage))
	gimp_channel_clear (gimp_image_get_mask (gimage));

      /*  If the drawable id is invalid, create a new layer  */
      if (drawable == NULL)
	gimp_image_add_layer (gimage, layer, -1);
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
	  g_message ("text_render: could not allocate image");
          tile_manager_destroy (newmask);
	}
      layer = NULL;
    }

  /* free the pixmap */
  gdk_drawable_unref (pixmap);

  /* free the gc */
  gdk_gc_destroy (gc);

  /* free the font */
  gdk_font_unref (font);

  return layer;
}

gboolean
text_get_extents (gchar *fontname,
		  gchar *text,
		  gint  *width,
		  gint  *height,
		  gint  *ascent,
		  gint  *descent)
{
  GdkFont *font;
  gchar *str;
  gint   nstrs;
  gint   line_width, line_height;
#ifndef GDK_WINDOWING_WIN32
  XFontStruct *xfs;
#endif

  /* load the font in */
  gdk_error_warnings = 0;
  gdk_error_code = 0;
#ifndef GDK_WINDOWING_WIN32
  font = gdk_font_load (fontname);
  if (!font)
    return FALSE;

  xfs = GDK_FONT_XFONT (font);
  if (xfs->min_byte1 != 0 || xfs->max_byte1 != 0) 
    {
      gchar *fname;

      gdk_font_unref (font);
      fname = g_strdup_printf ("%s,*", fontname);
      font = gdk_fontset_load (fname);
      g_free (fname);
    }
#else
  /* Just use gdk_fontset_load all the time. IMHO it could be like
   * this on all platforms?
   */
  font = gdk_fontset_load (fontname);
#endif
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
text_field_edges (gchar  *fontname,
		  gint    field_num,
		  /* RETURNS: */
		  gchar **start,
		  gchar **end)
{
  gchar *t1, *t2;

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
{ \
  if (x >= 0) \
    g_snprintf (new_ ## x, sizeof (new_ ## x), "%d", x); \
  else \
    g_snprintf (new_ ## x, sizeof (new_ ## x), "*"); \
}

/* Multiply the point and pixel sizes in *fontname by "mul", which
 * must be positive.  If either point or pixel sizes are "*" then they
 * are left untouched.  The memory *fontname is g_free()d, and
 * *fontname is replaced by a fresh allocation of the correct size.
 */
static void
text_size_multiply (gchar **fontname,
		    gint    mul)
{
  gchar *pixel_str;
  gchar *point_str;
  gchar *newfont;
  gchar *end;
  gint pixel = -1;
  gint point = -1;
  gchar new_pixel[16];
  gchar new_point[16];

  /* slice the font spec around the size fields */
  text_field_edges (*fontname, PIXEL_SIZE, &pixel_str, &end);
  text_field_edges (*fontname, POINT_SIZE, &point_str, &end);

  *(pixel_str - 1) = 0;
  *(point_str - 1) = 0;

  if (*pixel_str != '*')
    pixel = atoi (pixel_str);

  if (*point_str != '*')
    point = atoi (point_str);

  pixel *= mul;
  point *= mul;

  /* convert the pixel and point sizes back to text */
  TO_TXT (pixel);
  TO_TXT (point);

  newfont = g_strdup_printf ("%s-%s-%s%s", *fontname, new_pixel, new_point, end);

  g_free (*fontname);

  *fontname = newfont;
}

static void
text_set_resolution (gchar   **fontname,
		     gdouble   xresolution,
		     gdouble   yresolution)
{
  gchar *size_str;
  gchar *xres_str;
  gchar *yres_str;
  gchar *newfont;
  gchar *end;
  gchar new_size[16];
  gchar new_xres[16];
  gchar new_yres[16];
  gdouble points;

  gint size;
  gint xres;
  gint yres;

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
  xresolution = CLAMP (xresolution, 1.0, GIMP_MAX_RESOLUTION);

  /* slice the font spec around the resolution fields */
  text_field_edges (*fontname, XRESOLUTION, &xres_str, &end);
  text_field_edges (*fontname, YRESOLUTION, &yres_str, &end);

  *(size_str - 1) = 0;
  *(xres_str - 1) = 0;
  *(yres_str - 1) = 0;

  /* convert the resolutions to text */
  size = (gint) points;
  xres = (gint) xresolution;
  yres = (gint) yresolution;

  TO_TXT (size);
  TO_TXT (xres);
  TO_TXT (yres);

  newfont = g_strdup_printf ("%s-%s-%s-%s%s",
			     *fontname, new_size, new_xres, new_yres, end);

  g_free (*fontname);

  *fontname = newfont;
}

#undef TO_TXT
