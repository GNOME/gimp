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

#include <gtk/gtk.h>
#include <pango/pangoft2.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "paint-funcs/paint-funcs.h"

#include "base/base-types.h"
#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"

#include "widgets/gimpfontselection.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "gimpeditselectiontool.h"
#include "gimptexttool.h"
#include "gimptool.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "libgimp_glue.h"
#include "floating_sel.h"
#include "undo_types.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


#define DEFAULT_FONT       "sans Normal"
#define DEFAULT_FONT_SIZE  50


/*  the text tool structures  */

typedef struct _TextOptions TextOptions;

struct _TextOptions
{
  GimpToolOptions  tool_options;

  gchar           *fontname_d;
  GtkWidget       *font_selection;

  gdouble          size;
  gdouble          size_d;
  GtkObject       *size_w;

  gdouble          border;
  gdouble          border_d;
  GtkObject       *border_w;

  GimpUnit         unit;
  GimpUnit         unit_d;
  GtkWidget       *unit_w;
};


static void   gimp_text_tool_class_init      (GimpTextToolClass *klass);
static void   gimp_text_tool_init            (GimpTextTool      *tool);

static void   gimp_text_tool_finalize        (GObject         *object);

static void   text_tool_control              (GimpTool        *tool,
					      ToolAction       tool_action,
					      GimpDisplay     *gdisp);
static void   text_tool_button_press         (GimpTool        *tool,
					      GdkEventButton  *bevent,
					      GimpDisplay     *gdisp);
static void   text_tool_button_release       (GimpTool        *tool,
					      GdkEventButton  *bevent,
					      GimpDisplay     *gdisp);
static void   text_tool_cursor_update        (GimpTool        *tool,
					      GdkEventMotion  *mevent,
					      GimpDisplay     *gdisp);
static void   text_tool_render               (GimpTextTool    *text_tool);

static TextOptions * text_tool_options_new   (GimpTextTool    *text_tool);
static void          text_tool_options_reset (GimpToolOptions *tool_options);


/*  local variables  */

static TextOptions   *text_tool_options = NULL;
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
			      GIMP_STOCK_TOOL_TEXT);
}

GType
gimp_text_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpTextToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_text_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpTextTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_text_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TOOL,
					  "GimpTextTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_text_tool_class_init (GimpTextToolClass *klass)
{
  GObjectClass  *object_class;
  GimpToolClass *tool_class;

  object_class = G_OBJECT_CLASS (klass);
  tool_class   = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_text_tool_finalize;

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
 
  text_tool->pango_context = pango_ft2_get_context ();

  /*  The tool options  */
  if (! text_tool_options)
    {
      text_tool_options = text_tool_options_new (text_tool);

      tool_manager_register_tool_options (GIMP_TYPE_TEXT_TOOL,
					  (GimpToolOptions *) text_tool_options);
    }

  tool->tool_cursor = GIMP_TEXT_TOOL_CURSOR;
  tool->scroll_lock = TRUE;  /* Disallow scrolling */
}

static void
gimp_text_tool_finalize (GObject *object)
{
  GimpTextTool *text_tool;

  text_tool = GIMP_TEXT_TOOL (object);

  if (text_tool->pango_context)
    {
      g_object_unref (text_tool->pango_context);
      text_tool->pango_context = NULL;
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
text_tool_options_reset (GimpToolOptions *tool_options)
{
  TextOptions *options;
  GtkWidget   *spinbutton;

  options = (TextOptions *) tool_options;

  gimp_font_selection_set_fontname 
    (GIMP_FONT_SELECTION (options->font_selection), options->fontname_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->size_w),
			    options->size_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->border_w),
			    options->border_d);
  
  /* resetting the unit menu is a bit tricky ... */
  options->unit = options->unit_d;
  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (options->unit_w),
                           options->unit_d);
  spinbutton =
    g_object_get_data (G_OBJECT (options->unit_w), "set_digits");
  while (spinbutton)
    {
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), 0);
      spinbutton =
        g_object_get_data (G_OBJECT (spinbutton), "set_digits");
    }
}

static TextOptions *
text_tool_options_new (GimpTextTool *text_tool)
{
  TextOptions *options;
  GtkWidget   *vbox;
  GtkWidget   *table;
  GtkWidget   *size_spinbutton;
  GtkWidget   *border_spinbutton;

  options = g_new0 (TextOptions, 1);
  tool_options_init ((GimpToolOptions *) options, text_tool_options_reset);

  options->fontname_d                   = DEFAULT_FONT;
  options->border  = options->border_d  = 0;
  options->size    = options->size_d    = DEFAULT_FONT_SIZE;
  options->unit    = options->unit_d    = GIMP_UNIT_PIXEL;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  options->font_selection = gimp_font_selection_new (text_tool->pango_context);
  gimp_font_selection_set_fontname 
    (GIMP_FONT_SELECTION (options->font_selection), DEFAULT_FONT);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (options->font_selection),
                      FALSE, FALSE, 0);
  gtk_widget_show (options->font_selection);
  
  /*  the size entries */
  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (table), FALSE, FALSE, 0);

  options->size_w = gtk_adjustment_new (options->size_d, 1e-5, 32767.0,
                                        1.0, 50.0, 0.0);
  size_spinbutton = 
    gtk_spin_button_new (GTK_ADJUSTMENT (options->size_w), 1.0, 0.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (size_spinbutton), TRUE);
  g_signal_connect (G_OBJECT (options->size_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->size);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Size:"), 1.0, 0.5,
                             size_spinbutton, 1, FALSE);

  options->border_w = gtk_adjustment_new (options->border_d, 1e-5, 32767.0,
                                          1.0, 50.0, 0.0);
  border_spinbutton =
    gtk_spin_button_new (GTK_ADJUSTMENT (options->border_w), 1.0, 0.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (border_spinbutton), TRUE);
  g_signal_connect (G_OBJECT (options->border_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->border);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Border:"), 1.0, 0.5,
                             border_spinbutton, 1, FALSE);

  options->unit_w =
    gimp_unit_menu_new ("%a", options->unit_d, TRUE, FALSE, TRUE);
  g_signal_connect (G_OBJECT (options->unit_w), "unit_changed",
                    G_CALLBACK (gimp_unit_menu_update),
                    &options->unit);
  g_object_set_data (G_OBJECT (options->unit_w), "set_digits",
                     size_spinbutton);
  g_object_set_data (G_OBJECT (size_spinbutton), "set_digits",
                     border_spinbutton);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Unit:"), 1.0, 0.5,
                             options->unit_w, 1, FALSE);

  gtk_widget_show (table);

  return options;
}

static void
text_tool_control (GimpTool    *tool,
		   ToolAction   action,
		   GimpDisplay *gdisp)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      break;

    default:
      break;
    }
}

static void
text_tool_button_press (GimpTool       *tool,
			GdkEventButton *bevent,
			GimpDisplay    *gdisp)
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
    /* if there is a floating selection, and this aint it, use the move tool */
    if (gimp_layer_is_floating_sel (layer))
      {
	init_edit_selection (tool, gdisp, bevent, EDIT_LAYER_TRANSLATE);
	return;
      }

  text_tool_render (GIMP_TEXT_TOOL (tool));
}

static void
text_tool_button_release (GimpTool       *tool,
			  GdkEventButton *bevent,
			  GimpDisplay    *gdisp)
{
  tool->state = INACTIVE;
}

static void
text_tool_cursor_update (GimpTool       *tool,
			 GdkEventMotion *mevent,
			 GimpDisplay    *gdisp)
{
  GimpDisplayShell *shell;
  GimpLayer        *layer;
  gint              x, y;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &x, &y, FALSE, FALSE);

  if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage, x, y)))
    /* if there is a floating selection, and this aint it ... */
    if (gimp_layer_is_floating_sel (layer))
      {
	gimp_display_shell_install_tool_cursor (shell,
                                                GDK_FLEUR,
                                                GIMP_MOVE_TOOL_CURSOR,
                                                GIMP_CURSOR_MODIFIER_NONE);
	return;
      }

  gimp_display_shell_install_tool_cursor (shell,
                                          GDK_XTERM,
                                          GIMP_TEXT_TOOL_CURSOR,
                                          GIMP_CURSOR_MODIFIER_NONE);
}

static void
text_tool_render (GimpTextTool *text_tool)
{
  GimpDisplay          *gdisp;
  PangoFontDescription *font_desc;
  gchar                *fontname;
  gchar                *text;
  gdouble               border; 
  gdouble               size;
  gdouble               factor;

  gdisp = text_tool->gdisp;
  
  font_desc = gimp_font_selection_get_font_desc 
    (GIMP_FONT_SELECTION (text_tool_options->font_selection));

  if (!font_desc)
    {
      g_message (_("No font choosen or font invalid."));
      return;
    }
  
  size   = text_tool_options->size;
  border = text_tool_options->border;

  switch (text_tool_options->unit)
    {
    case GIMP_UNIT_PIXEL:
      break;
    default:
      factor = (gdisp->gimage->xresolution / 
                gimp_unit_get_factor (text_tool_options->unit));
      size   *= factor;
      border *= factor;
      break;
    }

  pango_font_description_set_size (font_desc, PANGO_SCALE * MAX (1, size));
  fontname = pango_font_description_to_string (font_desc);
  pango_font_description_free (font_desc);

  text = "gimp";  /* FIXME */

  text_render (gdisp->gimage, gimp_image_active_drawable (gdisp->gimage),
	       text_tool->click_x, text_tool->click_y,
	       fontname, text, border,
               TRUE /* antialias */);

  g_free (fontname);

  gdisplays_flush ();
}

GimpLayer *
text_render (GimpImage    *gimage,
	     GimpDrawable *drawable,
	     gint          text_x,
	     gint          text_y,
	     const gchar  *fontname,
	     const gchar  *text,
	     gint          border,
	     gint          antialias)
{
  PangoFontDescription *font_desc;
  PangoContext         *context;
  PangoLayout          *layout;
  PangoRectangle        ink;
  PangoRectangle        logical;
  GimpImageType         layer_type;
  GimpLayer            *layer = NULL;

  g_return_val_if_fail (fontname != NULL, FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  if (border < 0)
    border = 0;

  /*  determine the layer type  */
  if (drawable)
    layer_type = gimp_drawable_type_with_alpha (drawable);
  else
    layer_type = gimp_image_base_type_with_alpha (gimage);

  font_desc = pango_font_description_from_string (fontname);
  g_return_val_if_fail (font_desc != NULL, NULL);
  if (!font_desc)
    return NULL;
  
  context = pango_ft2_get_context ();
  layout = pango_layout_new (context);
  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);
  pango_layout_set_text (layout, text, -1);

  pango_layout_get_pixel_extents (layout, &ink, &logical);

  g_print ("ink rect: %d x %d @ %d, %d\n", 
           ink.width, ink.height, ink.x, ink.y);
  g_print ("logical rect: %d x %d @ %d, %d\n", 
           logical.width, logical.height, logical.x, logical.y);
  
  if (ink.width > 0 && ink.height > 0)
    {
      TileManager *mask;
      PixelRegion  textPR;
      PixelRegion  maskPR;
      FT_Bitmap    bitmap;
      guchar      *black = NULL;
      guchar       color[MAX_CHANNELS];
      gint         width;
      gint         height;
      gint         y;

      bitmap.width  = ink.width;
      bitmap.rows   = ink.height;
      bitmap.pitch  = ink.width;
      if (bitmap.pitch & 3)
        bitmap.pitch += 4 - (bitmap.pitch & 3);

      bitmap.buffer = g_malloc0 (bitmap.rows * bitmap.pitch);
      
      pango_ft2_render_layout (&bitmap, layout, 
                               - ink.x,
                               - ink.height - ink.y);
     
      width  = ink.width  + 2 * border;
      height = ink.height + 2 * border;

      mask = tile_manager_new (width, height, 1);
      pixel_region_init (&maskPR, mask, 0, 0, width, height, TRUE);

      if (border)
        black = g_malloc0 (width);

      for (y = 0; y < border; y++)
        pixel_region_set_row (&maskPR, 0, y, width, black);
      for (; y < height - border; y++)
        {
          if (border)
            {
              pixel_region_set_row (&maskPR, 0, y, border, black);
              pixel_region_set_row (&maskPR, width - border, y, border, black);
            }
          pixel_region_set_row (&maskPR, border, y, bitmap.width,
                                bitmap.buffer + (y - border) * bitmap.pitch);
        }
      for (; y < height; y++)
        pixel_region_set_row (&maskPR, 0, y, width, black);

      g_free (black);
      g_free (bitmap.buffer);

      layer = gimp_layer_new (gimage, width, height, layer_type,
                              _("Text Layer"), OPAQUE_OPACITY, NORMAL_MODE);

      /*  color the layer buffer  */
      gimp_image_get_foreground (gimage, drawable, color);
      color[GIMP_DRAWABLE (layer)->bytes - 1] = OPAQUE_OPACITY;
      pixel_region_init (&textPR, GIMP_DRAWABLE (layer)->tiles,
			 0, 0, width, height, TRUE);
      color_region (&textPR, color);

      /*  apply the text mask  */
      pixel_region_init (&textPR, GIMP_DRAWABLE (layer)->tiles,
                         0, 0, width, height, TRUE);
      pixel_region_init (&maskPR, mask,
			 0, 0, width, height, FALSE);
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

      /*  If the drawable is NULL, create a new layer  */
      if (drawable == NULL)
	gimp_image_add_layer (gimage, layer, -1);
      /*  Otherwise, instantiate the text as the new floating selection */
      else
	floating_sel_attach (layer, drawable);

      /*  end the group undo  */
      undo_push_group_end (gimage);

      tile_manager_destroy (mask);
    }

  g_object_unref (layout);
  g_object_unref (context);  

  return layer;
}

gboolean
text_get_extents (const gchar *fontname,
		  const gchar *text,
		  gint        *width,
		  gint        *height,
		  gint        *ascent,
		  gint        *descent)
{
  PangoFontDescription *font_desc;
  PangoContext         *context;
  PangoLayout          *layout;
  PangoRectangle        rect;

  g_return_val_if_fail (fontname != NULL, FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  font_desc = pango_font_description_from_string (fontname);
  if (!font_desc)
    return FALSE;
  
  context = pango_ft2_get_context ();
  layout = pango_layout_new (context);
  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);

  pango_layout_set_text (layout, text, -1);

  pango_layout_get_pixel_extents (layout, &rect, NULL);

  if (width)
    *width = rect.width;
  if (height)
    *height = rect.height;
  if (ascent)
    *ascent = -rect.y;
  if (descent)
    *descent = rect.height + rect.y;

  g_object_unref (layout);
  g_object_unref (context);  

  return TRUE;
}
