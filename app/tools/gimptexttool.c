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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpfontselection.h"

#include "display/gimpdisplay.h"

#include "gimpeditselectiontool.h"
#include "gimptexttool.h"
#include "tool_options.h"

#include "gimprc.h"
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


/*  local function prototypes  */

static void   gimp_text_tool_class_init      (GimpTextToolClass *klass);
static void   gimp_text_tool_init            (GimpTextTool      *tool);

static void   gimp_text_tool_finalize        (GObject         *object);

static void   text_tool_control              (GimpTool        *tool,
					      GimpToolAction   action,
					      GimpDisplay     *gdisp);
static void   text_tool_button_press         (GimpTool        *tool,
                                              GimpCoords      *coords,
                                              guint32          time,
					      GdkModifierType  state,
					      GimpDisplay     *gdisp);
static void   text_tool_button_release       (GimpTool        *tool,
                                              GimpCoords      *coords,
                                              guint32          time,
					      GdkModifierType  state,
					      GimpDisplay     *gdisp);
static void   text_tool_cursor_update        (GimpTool        *tool,
                                              GimpCoords      *coords,
					      GdkModifierType  state,
					      GimpDisplay     *gdisp);

static void   text_tool_render               (GimpTextTool    *text_tool);

static GimpToolOptions * text_tool_options_new   (GimpToolInfo    *tool_info);
static void              text_tool_options_reset (GimpToolOptions *tool_options);


/*  local variables  */

static GimpToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_text_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_TEXT_TOOL,
                text_tool_options_new,
                FALSE,
                "gimp-text-tool",
                _("Text Tool"),
                _("Add text to the image"),
                N_("/Tools/Text"), "T",
                NULL, "tools/text.html",
                GIMP_STOCK_TOOL_TEXT,
                data);
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


/*  private functions  */

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
 
  text_tool->pango_context = pango_ft2_get_context (gimprc.monitor_xres,
                                                    gimprc.monitor_yres);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TEXT_TOOL_CURSOR);
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

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
text_tool_control (GimpTool       *tool,
		   GimpToolAction  action,
		   GimpDisplay    *gdisp)
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

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
text_tool_button_press (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
			GdkModifierType  state,
			GimpDisplay     *gdisp)
{
  GimpTextTool *text_tool;
  GimpLayer    *layer;

  text_tool = GIMP_TEXT_TOOL (tool);

  text_tool->gdisp = gdisp;

  gimp_tool_control_activate (tool->control);
  tool->gdisp = gdisp;

  text_tool->click_x = coords->x;
  text_tool->click_y = coords->y;

  if ((layer = gimp_image_pick_correlate_layer (gdisp->gimage,
                                                text_tool->click_x,
                                                text_tool->click_y)))
    /* if there is a floating selection, and this aint it, use the move tool */
    if (gimp_layer_is_floating_sel (layer))
      {
	init_edit_selection (tool, gdisp, coords, EDIT_LAYER_TRANSLATE);
	return;
      }

  text_tool_render (GIMP_TEXT_TOOL (tool));
}

static void
text_tool_button_release (GimpTool        *tool,
                          GimpCoords      *coords,
                          guint32          time,
			  GdkModifierType  state,
			  GimpDisplay     *gdisp)
{
  gimp_tool_control_halt (tool->control);    /* sets paused_count to 0 -- is this ok? */
}

static void
text_tool_cursor_update (GimpTool        *tool,
                         GimpCoords      *coords,
			 GdkModifierType  state,
			 GimpDisplay     *gdisp)
{
  GimpLayer *layer;

  layer = gimp_image_pick_correlate_layer (gdisp->gimage, coords->x, coords->y);

  if (layer && gimp_layer_is_floating_sel (layer))
    {
      /* if there is a floating selection, and this aint it ... */

      gimp_tool_control_set_cursor_modifier (tool->control,
                                             GIMP_CURSOR_MODIFIER_MOVE);
    }
  else
    {
      gimp_tool_control_set_cursor_modifier (tool->control,
                                             GIMP_CURSOR_MODIFIER_NONE);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
text_tool_render (GimpTextTool *text_tool)
{
  TextOptions          *options;
  GimpDisplay          *gdisp;
  PangoFontDescription *font_desc;
  gchar                *fontname;
  gchar                *text;
  gdouble               border; 
  gdouble               size;
  gdouble               factor;

  options = (TextOptions *) GIMP_TOOL (text_tool)->tool_info->tool_options;

  gdisp = text_tool->gdisp;
  
  font_desc = gimp_font_selection_get_font_desc 
    (GIMP_FONT_SELECTION (options->font_selection));

  if (!font_desc)
    {
      g_message (_("No font choosen or font invalid."));
      return;
    }
  
  size   = options->size;
  border = options->border;

  switch (options->unit)
    {
    case GIMP_UNIT_PIXEL:
      break;
    default:
      factor = (gdisp->gimage->xresolution /
                gimp_unit_get_factor (options->unit));
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

  gimp_image_flush (gdisp->gimage);
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
  gdouble               xres;
  gdouble               yres;

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
  
  gimp_image_get_resolution (gimage, &xres, &yres);
  context = pango_ft2_get_context (xres, yres);

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
      
      pango_ft2_render_layout (&bitmap, layout, - ink.x, - ink.y);
     
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
                              _("Text Layer"),
                              GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

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
      undo_push_group_start (gimage, TEXT_UNDO_GROUP);

      /*  Set the layer offsets  */
      GIMP_DRAWABLE (layer)->offset_x = text_x;
      GIMP_DRAWABLE (layer)->offset_y = text_y;

      /*  If there is a selection mask clear it--
       *  this might not always be desired, but in general,
       *  it seems like the correct behavior.
       */
      if (! gimp_image_mask_is_empty (gimage))
	gimp_image_mask_clear (gimage);

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

  /* FIXME: resolution */
  context = pango_ft2_get_context (72.0, 72.0);

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


/*  tool options stuff  */

static GimpToolOptions *
text_tool_options_new (GimpToolInfo *tool_info)
{
  TextOptions  *options;
  PangoContext *pango_context;
  GtkWidget    *vbox;
  GtkWidget    *table;
  GtkWidget    *size_spinbutton;
  GtkWidget    *border_spinbutton;

  options = g_new0 (TextOptions, 1);

  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = text_tool_options_reset;

  options->fontname_d                   = DEFAULT_FONT;
  options->border  = options->border_d  = 0;
  options->size    = options->size_d    = DEFAULT_FONT_SIZE;
  options->unit    = options->unit_d    = GIMP_UNIT_PIXEL;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (table), FALSE, FALSE, 0);

  pango_context = pango_ft2_get_context (gimprc.monitor_xres,
                                         gimprc.monitor_yres);

  options->font_selection = gimp_font_selection_new (pango_context);

  g_object_unref (G_OBJECT (pango_context));

  gimp_font_selection_set_fontname 
    (GIMP_FONT_SELECTION (options->font_selection), DEFAULT_FONT);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Font:"), 1.0, 0.5,
                             options->font_selection, 2, FALSE);

  options->size_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                          _("Size:"), -1, 50,
                                          options->size,
                                          1.0, 256.0, 1.0, 50.0, 1,
                                          FALSE, 1e-5, 32767.0,
                                          NULL, NULL);

  size_spinbutton = GIMP_SCALE_ENTRY_SPINBUTTON (options->size_w);

  g_signal_connect (G_OBJECT (options->size_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->size);

  options->border_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                                            _("Border:"), -1, 50,
                                            options->border,
                                            0.0, 100.0, 1.0, 50.0, 1,
                                            FALSE, 0.0, 32767.0,
                                            NULL, NULL);

  border_spinbutton = GIMP_SCALE_ENTRY_SPINBUTTON (options->border_w);

  g_signal_connect (G_OBJECT (options->border_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->border);

  options->unit_w = gimp_unit_menu_new ("%a", options->unit, 
                                        TRUE, FALSE, TRUE);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("Unit:"), 1.0, 0.5,
                             options->unit_w, 2, TRUE);

  g_object_set_data (G_OBJECT (options->unit_w), "set_digits",
                     size_spinbutton);
  g_object_set_data (G_OBJECT (size_spinbutton), "set_digits",
                     border_spinbutton);

  g_signal_connect (G_OBJECT (options->unit_w), "unit_changed",
                    G_CALLBACK (gimp_unit_menu_update),
                    &options->unit);

  gtk_widget_show (table);

  return (GimpToolOptions *) options;
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
