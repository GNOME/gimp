/* GIMP - The GNU Image Manipulation Program
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpdrawable-bucket-fill.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimppickable.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpbucketfilloptions.h"
#include "gimpbucketfilltool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_bucket_fill_tool_button_release (GimpTool              *tool,
                                                    GimpCoords            *coords,
                                                    guint32                time,
                                                    GdkModifierType        state,
                                                    GimpButtonReleaseType  release_type,
                                                    GimpDisplay           *display);
static void   gimp_bucket_fill_tool_modifier_key   (GimpTool              *tool,
                                                    GdkModifierType        key,
                                                    gboolean               press,
                                                    GdkModifierType        state,
                                                    GimpDisplay           *display);
static void   gimp_bucket_fill_tool_cursor_update  (GimpTool              *tool,
                                                    GimpCoords            *coords,
                                                    GdkModifierType        state,
                                                    GimpDisplay           *display);


G_DEFINE_TYPE (GimpBucketFillTool, gimp_bucket_fill_tool, GIMP_TYPE_TOOL)

#define parent_class gimp_bucket_fill_tool_parent_class


void
gimp_bucket_fill_tool_register (GimpToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (GIMP_TYPE_BUCKET_FILL_TOOL,
                GIMP_TYPE_BUCKET_FILL_OPTIONS,
                gimp_bucket_fill_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK |
                GIMP_CONTEXT_BACKGROUND_MASK |
                GIMP_CONTEXT_OPACITY_MASK    |
                GIMP_CONTEXT_PAINT_MODE_MASK |
                GIMP_CONTEXT_PATTERN_MASK,
                "gimp-bucket-fill-tool",
                _("Bucket Fill"),
                _("Bucket Fill Tool: Fill selected area with a color or pattern"),
                N_("_Bucket Fill"), "<shift>B",
                NULL, GIMP_HELP_TOOL_BUCKET_FILL,
                GIMP_STOCK_TOOL_BUCKET_FILL,
                data);
}

static void
gimp_bucket_fill_tool_class_init (GimpBucketFillToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->button_release = gimp_bucket_fill_tool_button_release;
  tool_class->modifier_key   = gimp_bucket_fill_tool_modifier_key;
  tool_class->cursor_update  = gimp_bucket_fill_tool_cursor_update;
}

static void
gimp_bucket_fill_tool_init (GimpBucketFillTool *bucket_fill_tool)
{
  GimpTool *tool = GIMP_TOOL (bucket_fill_tool);

  gimp_tool_control_set_scroll_lock     (tool->control, TRUE);
  gimp_tool_control_set_wants_click     (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_BUCKET_FILL);
  gimp_tool_control_set_action_value_1  (tool->control,
                                         "context/context-opacity-set");
  gimp_tool_control_set_action_object_1 (tool->control,
                                         "context/context-pattern-select-set");
}

static void
gimp_bucket_fill_tool_button_release (GimpTool              *tool,
                                      GimpCoords            *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      GimpButtonReleaseType  release_type,
                                      GimpDisplay           *display)
{
  GimpBucketFillOptions *options = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  if ((release_type == GIMP_BUTTON_RELEASE_CLICK ||
       release_type == GIMP_BUTTON_RELEASE_NO_MOTION) &&
      gimp_image_coords_in_active_pickable (display->image, coords,
                                            options->sample_merged, TRUE))
    {
      GimpDrawable *drawable = gimp_image_get_active_drawable (display->image);
      GimpContext  *context  = GIMP_CONTEXT (options);
      gint          x, y;

      x = coords->x;
      y = coords->y;

      if (! options->sample_merged)
        {
          gint off_x, off_y;

          gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

          x -= off_x;
          y -= off_y;
        }

      gimp_drawable_bucket_fill (drawable,
                                 context,
                                 options->fill_mode,
                                 gimp_context_get_paint_mode (context),
                                 gimp_context_get_opacity (context),
                                 ! options->fill_selection,
                                 options->fill_transparent,
                                 options->fill_criterion,
                                 options->threshold,
                                 options->sample_merged,
                                 x, y);

      gimp_image_flush (display->image);
    }

  GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);
}

static void
gimp_bucket_fill_tool_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpBucketFillOptions *options = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  if (key == GDK_CONTROL_MASK)
    {
      switch (options->fill_mode)
        {
        case GIMP_FG_BUCKET_FILL:
          g_object_set (options, "fill-mode", GIMP_BG_BUCKET_FILL, NULL);
          break;

        case GIMP_BG_BUCKET_FILL:
          g_object_set (options, "fill-mode", GIMP_FG_BUCKET_FILL, NULL);
          break;

        default:
          break;
        }
    }
  else if (key == GDK_SHIFT_MASK)
    {
      g_object_set (options, "fill-selection", ! options->fill_selection, NULL);
    }
}

static void
gimp_bucket_fill_tool_cursor_update (GimpTool        *tool,
                                     GimpCoords      *coords,
                                     GdkModifierType  state,
                                     GimpDisplay     *display)
{
  GimpBucketFillOptions *options  = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  GimpCursorModifier     modifier = GIMP_CURSOR_MODIFIER_BAD;

  if (gimp_image_coords_in_active_pickable (display->image, coords,
                                            options->sample_merged, TRUE))
    {
      switch (options->fill_mode)
        {
        case GIMP_FG_BUCKET_FILL:
          modifier = GIMP_CURSOR_MODIFIER_FOREGROUND;
          break;

        case GIMP_BG_BUCKET_FILL:
          modifier = GIMP_CURSOR_MODIFIER_BACKGROUND;
          break;

        case GIMP_PATTERN_BUCKET_FILL:
          modifier = GIMP_CURSOR_MODIFIER_PATTERN;
          break;
        }
    }

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}
