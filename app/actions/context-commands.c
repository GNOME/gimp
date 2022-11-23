/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "core/ligma.h"
#include "core/ligmabrushgenerated.h"
#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"
#include "core/ligmalist.h"
#include "core/ligmapaintinfo.h"
#include "core/ligmatoolinfo.h"

#include "paint/ligmapaintoptions.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmapaletteeditor.h"
#include "widgets/ligmacolormapeditor.h"

#include "actions.h"
#include "context-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void     context_select_object    (LigmaActionSelectType  select_type,
                                          LigmaContext          *context,
                                          LigmaContainer        *container);
static gint     context_paint_mode_index (LigmaLayerMode         paint_mode,
                                          const LigmaLayerMode  *modes,
                                          gint                  n_modes);

static void     context_select_color     (LigmaActionSelectType  select_type,
                                          LigmaRGB              *color,
                                          gboolean              use_colormap,
                                          gboolean              use_palette);

static gint     context_get_color_index  (gboolean              use_colormap,
                                          gboolean              use_palette,
                                          const LigmaRGB        *color);
static gint     context_max_color_index  (gboolean              use_colormap,
                                          gboolean              use_palette);
static gboolean context_set_color_index  (gint                  index,
                                          gboolean              use_colormap,
                                          gboolean              use_palette,
                                          LigmaRGB              *color);

static LigmaPaletteEditor  * context_get_palette_editor  (void);
static LigmaColormapEditor * context_get_colormap_editor (void);


/*  public functions  */

void
context_colors_default_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaContext *context;
  return_if_no_context (context, data);

  ligma_context_set_default_colors (context);
}

void
context_colors_swap_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaContext *context;
  return_if_no_context (context, data);

  ligma_context_swap_colors (context);
}

#define SELECT_COLOR_CMD_CALLBACK(name, fgbg, use_colormap, use_palette) \
void \
context_##name##_##fgbg##ground_cmd_callback (LigmaAction *action, \
                                              GVariant   *value, \
                                              gpointer    data) \
{ \
  LigmaContext          *context; \
  LigmaRGB               color; \
  LigmaActionSelectType  select_type; \
  return_if_no_context (context, data); \
  \
  select_type = (LigmaActionSelectType) g_variant_get_int32 (value); \
  \
  ligma_context_get_##fgbg##ground (context, &color); \
  context_select_color (select_type, &color, \
                        use_colormap, use_palette); \
  ligma_context_set_##fgbg##ground (context, &color); \
}

SELECT_COLOR_CMD_CALLBACK (palette,  fore, FALSE, TRUE)
SELECT_COLOR_CMD_CALLBACK (palette,  back, FALSE, TRUE)
SELECT_COLOR_CMD_CALLBACK (colormap, fore, TRUE,  FALSE)
SELECT_COLOR_CMD_CALLBACK (colormap, back, TRUE,  FALSE)
SELECT_COLOR_CMD_CALLBACK (swatch,   fore, TRUE,  TRUE)
SELECT_COLOR_CMD_CALLBACK (swatch,   back, TRUE,  TRUE)

void
context_foreground_red_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_foreground (context, &color);
  color.r = action_select_value (select_type,
                                 color.r,
                                 0.0, 1.0, 1.0,
                                 1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  ligma_context_set_foreground (context, &color);
}

void
context_foreground_green_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_foreground (context, &color);
  color.g = action_select_value (select_type,
                                 color.g,
                                 0.0, 1.0, 1.0,
                                 1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  ligma_context_set_foreground (context, &color);
}

void
context_foreground_blue_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_foreground (context, &color);
  color.b = action_select_value (select_type,
                                 color.b,
                                 0.0, 1.0, 1.0,
                                 1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  ligma_context_set_foreground (context, &color);
}

void
context_background_red_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_background (context, &color);
  color.r = action_select_value (select_type,
                                 color.r,
                                 0.0, 1.0, 1.0,
                                 1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  ligma_context_set_background (context, &color);
}

void
context_background_green_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_background (context, &color);
  color.g = action_select_value (select_type,
                                 color.g,
                                 0.0, 1.0, 1.0,
                                 1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  ligma_context_set_background (context, &color);
}

void
context_background_blue_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_background (context, &color);
  color.b = action_select_value (select_type,
                                 color.b,
                                 0.0, 1.0, 1.0,
                                 1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
  ligma_context_set_background (context, &color);
}

void
context_foreground_hue_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaHSV               hsv;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_foreground (context, &color);
  ligma_rgb_to_hsv (&color, &hsv);
  hsv.h = action_select_value (select_type,
                               hsv.h,
                               0.0, 1.0, 1.0,
                               1.0 / 360.0, 0.01, 0.1, 0.0, FALSE);
  ligma_hsv_to_rgb (&hsv, &color);
  ligma_context_set_foreground (context, &color);
}

void
context_foreground_saturation_cmd_callback (LigmaAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaHSV               hsv;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_foreground (context, &color);
  ligma_rgb_to_hsv (&color, &hsv);
  hsv.s = action_select_value (select_type,
                               hsv.s,
                               0.0, 1.0, 1.0,
                               0.01, 0.01, 0.1, 0.0, FALSE);
  ligma_hsv_to_rgb (&hsv, &color);
  ligma_context_set_foreground (context, &color);
}

void
context_foreground_value_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaHSV               hsv;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_foreground (context, &color);
  ligma_rgb_to_hsv (&color, &hsv);
  hsv.v = action_select_value (select_type,
                               hsv.v,
                               0.0, 1.0, 1.0,
                               0.01, 0.01, 0.1, 0.0, FALSE);
  ligma_hsv_to_rgb (&hsv, &color);
  ligma_context_set_foreground (context, &color);
}

void
context_background_hue_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaHSV               hsv;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_background (context, &color);
  ligma_rgb_to_hsv (&color, &hsv);
  hsv.h = action_select_value (select_type,
                               hsv.h,
                               0.0, 1.0, 1.0,
                               1.0 / 360.0, 0.01, 0.1, 0.0, FALSE);
  ligma_hsv_to_rgb (&hsv, &color);
  ligma_context_set_background (context, &color);
}

void
context_background_saturation_cmd_callback (LigmaAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaHSV               hsv;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_background (context, &color);
  ligma_rgb_to_hsv (&color, &hsv);
  hsv.s = action_select_value (select_type,
                               hsv.s,
                               0.0, 1.0, 1.0,
                               0.01, 0.01, 0.1, 0.0, FALSE);
  ligma_hsv_to_rgb (&hsv, &color);
  ligma_context_set_background (context, &color);
}

void
context_background_value_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaContext          *context;
  LigmaRGB               color;
  LigmaHSV               hsv;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  ligma_context_get_background (context, &color);
  ligma_rgb_to_hsv (&color, &hsv);
  hsv.v = action_select_value (select_type,
                               hsv.v,
                               0.0, 1.0, 1.0,
                               0.01, 0.01, 0.1, 0.0, FALSE);
  ligma_hsv_to_rgb (&hsv, &color);
  ligma_context_set_background (context, &color);
}

void
context_opacity_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaContext          *context;
  LigmaToolInfo         *tool_info;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  tool_info = ligma_context_get_tool (context);

  if (tool_info && LIGMA_IS_TOOL_OPTIONS (tool_info->tool_options))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (tool_info->tool_options),
                              "opacity",
                              1.0 / 255.0, 0.01, 0.1, 0.1, FALSE);
    }
}

void
context_paint_mode_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaContext          *context;
  LigmaToolInfo         *tool_info;
  LigmaLayerMode        *modes;
  gint                  n_modes;
  LigmaLayerMode         paint_mode;
  gint                  index;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  paint_mode = ligma_context_get_paint_mode (context);

  modes = ligma_layer_mode_get_context_array (paint_mode,
                                             LIGMA_LAYER_MODE_CONTEXT_PAINT,
                                             &n_modes);
  index = context_paint_mode_index (paint_mode, modes, n_modes);
  index = action_select_value (select_type,
                               index, 0, n_modes - 1, 0,
                               0.0, 1.0, 1.0, 0.0, FALSE);
  paint_mode = modes[index];
  g_free (modes);

  ligma_context_set_paint_mode (context, paint_mode);

  tool_info = ligma_context_get_tool (context);

  if (tool_info && LIGMA_IS_TOOL_OPTIONS (tool_info->tool_options))
    {
      LigmaDisplay *display;
      const char  *value_desc;

      ligma_enum_get_value (LIGMA_TYPE_LAYER_MODE, paint_mode,
                           NULL, NULL, &value_desc, NULL);

      display = action_data_get_display (data);

      if (value_desc && display)
        {
          action_message (display, G_OBJECT (tool_info->tool_options),
                          _("Paint Mode: %s"), value_desc);
        }
    }
}

void
context_tool_select_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaContext          *context;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context, context->ligma->tool_info_list);
}

void
context_brush_select_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaContext          *context;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context,
                         ligma_data_factory_get_container (context->ligma->brush_factory));
}

void
context_pattern_select_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaContext          *context;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context,
                         ligma_data_factory_get_container (context->ligma->pattern_factory));
}

void
context_palette_select_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaContext          *context;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context,
                         ligma_data_factory_get_container (context->ligma->palette_factory));
}

void
context_gradient_select_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaContext          *context;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context,
                         ligma_data_factory_get_container (context->ligma->gradient_factory));
}

void
context_font_select_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaContext          *context;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  context_select_object (select_type,
                         context,
                         ligma_data_factory_get_container (context->ligma->font_factory));
}

void
context_brush_spacing_cmd_callback (LigmaAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  LigmaContext          *context;
  LigmaBrush            *brush;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  brush = ligma_context_get_brush (context);

  if (LIGMA_IS_BRUSH (brush) && ligma_data_is_writable (LIGMA_DATA (brush)))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (brush),
                              "spacing",
                              1.0, 5.0, 20.0, 0.1, FALSE);
    }
}

void
context_brush_shape_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaContext             *context;
  LigmaBrush               *brush;
  LigmaBrushGeneratedShape  shape;
  return_if_no_context (context, data);

  shape = (LigmaBrushGeneratedShape) g_variant_get_int32 (value);

  brush = ligma_context_get_brush (context);

  if (LIGMA_IS_BRUSH_GENERATED (brush) &&
      ligma_data_is_writable (LIGMA_DATA (brush)))
    {
      LigmaBrushGenerated *generated = LIGMA_BRUSH_GENERATED (brush);
      LigmaDisplay        *display;
      const char         *value_desc;

      ligma_brush_generated_set_shape (generated, shape);

      ligma_enum_get_value (LIGMA_TYPE_BRUSH_GENERATED_SHAPE, shape,
                           NULL, NULL, &value_desc, NULL);
      display = action_data_get_display (data);

      if (value_desc && display)
        {
          action_message (display, G_OBJECT (brush),
                          _("Brush Shape: %s"), value_desc);
        }
    }
}

void
context_brush_radius_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaContext          *context;
  LigmaBrush            *brush;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  brush = ligma_context_get_brush (context);

  if (LIGMA_IS_BRUSH_GENERATED (brush) &&
      ligma_data_is_writable (LIGMA_DATA (brush)))
    {
      LigmaBrushGenerated *generated = LIGMA_BRUSH_GENERATED (brush);
      LigmaDisplay        *display;
      gdouble             radius;
      gdouble             min_radius;

      radius = ligma_brush_generated_get_radius (generated);

      /* If the user uses a high precision radius adjustment command
       * then we allow a minimum radius of 0.1 px, otherwise we set the
       * minimum radius to 1.0 px and adjust the radius to 1.0 px if it
       * is less than 1.0 px. This prevents irritating 0.1, 1.1, 2.1 etc
       * radius sequences when 1.0 px steps are used.
       */
      switch (select_type)
        {
        case LIGMA_ACTION_SELECT_SMALL_PREVIOUS:
        case LIGMA_ACTION_SELECT_SMALL_NEXT:
        case LIGMA_ACTION_SELECT_PERCENT_PREVIOUS:
        case LIGMA_ACTION_SELECT_PERCENT_NEXT:
          min_radius = 0.1;
          break;

        default:
          min_radius = 1.0;

          if (radius < 1.0)
            radius = 1.0;
          break;
        }

      radius = action_select_value (select_type,
                                    radius,
                                    min_radius, 4000.0, min_radius,
                                    0.1, 1.0, 10.0, 0.05, FALSE);
      ligma_brush_generated_set_radius (generated, radius);

      display = action_data_get_display (data);

      if (display)
        {
          action_message (action_data_get_display (data), G_OBJECT (brush),
                          _("Brush Radius: %2.2f"), radius);
        }
    }
}

void
context_brush_spikes_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaContext          *context;
  LigmaBrush            *brush;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  brush = ligma_context_get_brush (context);

  if (LIGMA_IS_BRUSH_GENERATED (brush) &&
      ligma_data_is_writable (LIGMA_DATA (brush)))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (brush),
                              "spikes",
                              0.0, 1.0, 4.0, 0.1, FALSE);
    }
}

void
context_brush_hardness_cmd_callback (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  LigmaContext          *context;
  LigmaBrush            *brush;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  brush = ligma_context_get_brush (context);

  if (LIGMA_IS_BRUSH_GENERATED (brush) &&
      ligma_data_is_writable (LIGMA_DATA (brush)))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (brush),
                              "hardness",
                              0.001, 0.01, 0.1, 0.1, FALSE);
    }
}

void
context_brush_aspect_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaContext          *context;
  LigmaBrush            *brush;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  brush = ligma_context_get_brush (context);

  if (LIGMA_IS_BRUSH_GENERATED (brush) &&
      ligma_data_is_writable (LIGMA_DATA (brush)))
    {
      action_select_property (select_type,
                              action_data_get_display (data),
                              G_OBJECT (brush),
                              "aspect-ratio",
                              0.1, 1.0, 4.0, 0.1, FALSE);
    }
}

void
context_brush_angle_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaContext          *context;
  LigmaBrush            *brush;
  LigmaActionSelectType  select_type;
  return_if_no_context (context, data);

  select_type = (LigmaActionSelectType) g_variant_get_int32 (value);

  brush = ligma_context_get_brush (context);

  if (LIGMA_IS_BRUSH_GENERATED (brush) &&
      ligma_data_is_writable (LIGMA_DATA (brush)))
    {
      LigmaBrushGenerated *generated = LIGMA_BRUSH_GENERATED (brush);
      LigmaDisplay        *display;
      gdouble             angle;

      angle = ligma_brush_generated_get_angle (generated);

      if (select_type == LIGMA_ACTION_SELECT_FIRST)
        angle = 0.0;
      else if (select_type == LIGMA_ACTION_SELECT_LAST)
        angle = 90.0;
      else
        angle = action_select_value (select_type,
                                     angle,
                                     0.0, 180.0, 0.0,
                                     0.1, 1.0, 15.0, 0.1, TRUE);

      ligma_brush_generated_set_angle (generated, angle);

      display = action_data_get_display (data);

      if (display)
        {
          action_message (action_data_get_display (data), G_OBJECT (brush),
                          _("Brush Angle: %2.2f"), angle);
        }
    }
}

void
context_toggle_dynamics_cmd_callback (LigmaAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  LigmaContext   *context;
  LigmaPaintInfo *paint_info;
  gboolean       enabled;

  return_if_no_context (context, data);

  paint_info = ligma_context_get_paint_info (context);
  if (paint_info)
    {
      LigmaPaintOptions *paint_options;
      LigmaDisplay      *display;

      paint_options = paint_info->paint_options;
      enabled = ligma_paint_options_are_dynamics_enabled (paint_options);

      ligma_paint_options_enable_dynamics (paint_options, ! enabled);

      display = action_data_get_display (data);

      if (enabled)
        action_message (display, G_OBJECT (paint_options),
                        _("Dynamics disabled"));
      else
        action_message (display, G_OBJECT (paint_options),
                        _("Dynamics enabled"));
    }
}


/*  private functions  */

static void
context_select_object (LigmaActionSelectType  select_type,
                       LigmaContext          *context,
                       LigmaContainer        *container)
{
  LigmaObject *current;

  current =
    ligma_context_get_by_type (context,
                              ligma_container_get_children_type (container));

  current = action_select_object (select_type, container, current);

  if (current)
    ligma_context_set_by_type (context,
                              ligma_container_get_children_type (container),
                              current);
}

static gint
context_paint_mode_index (LigmaLayerMode        paint_mode,
                          const LigmaLayerMode *modes,
                          gint                 n_modes)
{
  gint i = 0;

  while (i < (n_modes - 1) && modes[i] != paint_mode)
    i++;

  return i;
}

static void
context_select_color (LigmaActionSelectType  select_type,
                      LigmaRGB              *color,
                      gboolean              use_colormap,
                      gboolean              use_palette)
{
  gint index;
  gint max;

  index = context_get_color_index (use_colormap, use_palette, color);
  max   = context_max_color_index (use_colormap, use_palette);

  index = action_select_value (select_type,
                               index,
                               0, max, 0,
                               0, 1, 4, 0, FALSE);

  context_set_color_index (index, use_colormap, use_palette, color);
}

static gint
context_get_color_index (gboolean       use_colormap,
                         gboolean       use_palette,
                         const LigmaRGB *color)
{
  if (use_colormap)
    {
      LigmaColormapEditor *editor = context_get_colormap_editor ();

      if (editor)
        {
          gint index = ligma_colormap_editor_get_index (editor, color);

          if (index != -1)
            return index;
        }
    }

  if (use_palette)
    {
      LigmaPaletteEditor *editor = context_get_palette_editor ();

      if (editor)
        {
          gint index = ligma_palette_editor_get_index (editor, color);

          if (index != -1)
            return index;
        }
    }

  return 0;
}

static gint
context_max_color_index (gboolean use_colormap,
                         gboolean use_palette)
{
  if (use_colormap)
    {
      LigmaColormapEditor *editor = context_get_colormap_editor ();

      if (editor)
        {
          gint index = ligma_colormap_editor_max_index (editor);

          if (index != -1)
            return index;
        }
    }

  if (use_palette)
    {
      LigmaPaletteEditor *editor = context_get_palette_editor ();

      if (editor)
        {
          gint index = ligma_palette_editor_max_index (editor);

          if (index != -1)
            return index;
        }
    }

  return 0;
}

static gboolean
context_set_color_index (gint      index,
                         gboolean  use_colormap,
                         gboolean  use_palette,
                         LigmaRGB  *color)
{
  if (use_colormap)
    {
      LigmaColormapEditor *editor = context_get_colormap_editor ();

      if (editor && ligma_colormap_editor_set_index (editor, index, color))
        return TRUE;
    }

  if (use_palette)
    {
      LigmaPaletteEditor *editor = context_get_palette_editor ();

      if (editor && ligma_palette_editor_set_index (editor, index, color))
        return TRUE;
    }

  return FALSE;
}

static LigmaPaletteEditor *
context_get_palette_editor (void)
{
  GtkWidget *widget;

  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (ligma_dialog_factory_get_singleton ()), NULL);

  widget = ligma_dialog_factory_find_widget (ligma_dialog_factory_get_singleton (),
                                            "ligma-palette-editor");
  if (widget)
    return LIGMA_PALETTE_EDITOR (gtk_bin_get_child (GTK_BIN (widget)));

  return NULL;
}

static LigmaColormapEditor *
context_get_colormap_editor (void)
{
  GtkWidget *widget;

  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (ligma_dialog_factory_get_singleton ()), NULL);

  widget = ligma_dialog_factory_find_widget (ligma_dialog_factory_get_singleton (),
                                            "ligma-indexed-palette");
  if (widget)
    return LIGMA_COLORMAP_EDITOR (gtk_bin_get_child (GTK_BIN (widget)));

  return NULL;
}
