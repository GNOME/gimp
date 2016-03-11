/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimp-edit.h"
#include "core/gimpdrawable-bucket-fill.h"
#include "core/gimperror.h"
#include "core/gimpfilloptions.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimppickable.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpbucketfilloptions.h"
#include "gimpbucketfilltool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static gboolean gimp_bucket_fill_tool_initialize   (GimpTool              *tool,
                                                    GimpDisplay           *display,
                                                    GError               **error);
static void   gimp_bucket_fill_tool_button_release (GimpTool              *tool,
                                                    const GimpCoords      *coords,
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
                                                    const GimpCoords      *coords,
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
                GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                GIMP_CONTEXT_PROP_MASK_BACKGROUND |
                GIMP_CONTEXT_PROP_MASK_OPACITY    |
                GIMP_CONTEXT_PROP_MASK_PAINT_MODE |
                GIMP_CONTEXT_PROP_MASK_PATTERN,
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

  tool_class->initialize     = gimp_bucket_fill_tool_initialize;
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
  gimp_tool_control_set_action_opacity  (tool->control,
                                         "context/context-opacity-set");
  gimp_tool_control_set_action_object_1 (tool->control,
                                         "context/context-pattern-select-set");
}

static gboolean
gimp_bucket_fill_tool_initialize (GimpTool     *tool,
                                  GimpDisplay  *display,
                                  GError      **error)
{
  GimpImage    *image    = gimp_display_get_image (display);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Cannot modify the pixels of layer groups."));
      return FALSE;
    }

  if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("The active layer's pixels are locked."));
      return FALSE;
    }

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The active layer is not visible."));
      return FALSE;
    }

  return TRUE;
}

static void
gimp_bucket_fill_tool_button_release (GimpTool              *tool,
                                      const GimpCoords      *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      GimpButtonReleaseType  release_type,
                                      GimpDisplay           *display)
{
  GimpBucketFillOptions *options = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  GimpImage             *image   = gimp_display_get_image (display);

  if ((release_type == GIMP_BUTTON_RELEASE_CLICK ||
       release_type == GIMP_BUTTON_RELEASE_NO_MOTION) &&
      gimp_image_coords_in_active_pickable (image, coords,
                                            options->sample_merged, TRUE))
    {
      GimpDrawable    *drawable = gimp_image_get_active_drawable (image);
      GimpContext     *context  = GIMP_CONTEXT (options);
      GimpFillOptions *fill_options;
      gboolean         success;
      GError          *error = NULL;

      fill_options = gimp_fill_options_new (image->gimp);

      success = gimp_fill_options_set_by_fill_mode (fill_options, context,
                                                    options->fill_mode,
                                                    &error);

      if (success)
        {
          gimp_context_set_opacity (GIMP_CONTEXT (fill_options),
                                    gimp_context_get_opacity (context));
          gimp_context_set_paint_mode (GIMP_CONTEXT (fill_options),
                                       gimp_context_get_paint_mode (context));

          if (options->fill_selection)
            {
              success = gimp_edit_fill (image, drawable, fill_options, NULL);
            }
          else
            {
              gint x = coords->x;
              gint y = coords->y;

              if (! options->sample_merged)
                {
                  gint off_x, off_y;

                  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

                  x -= off_x;
                  y -= off_y;
                }

              gimp_drawable_bucket_fill (drawable, fill_options,
                                         options->fill_transparent,
                                         options->fill_criterion,
                                         options->threshold / 255.0,
                                         options->sample_merged,
                                         options->diagonal_neighbors,
                                         x, y);
            }
        }

      g_object_unref (fill_options);

      if (success)
        {
          gimp_image_flush (image);
        }
      else
        {
          gimp_message_literal (display->gimp, G_OBJECT (display),
                                GIMP_MESSAGE_WARNING, error->message);
          g_clear_error (&error);
        }
    }

  GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);

  tool->display  = NULL;
  tool->drawable = NULL;
}

static void
gimp_bucket_fill_tool_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpBucketFillOptions *options = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  if (key == gimp_get_toggle_behavior_mask ())
    {
      switch (options->fill_mode)
        {
        case GIMP_BUCKET_FILL_FG:
          g_object_set (options, "fill-mode", GIMP_BUCKET_FILL_BG, NULL);
          break;

        case GIMP_BUCKET_FILL_BG:
          g_object_set (options, "fill-mode", GIMP_BUCKET_FILL_FG, NULL);
          break;

        default:
          break;
        }
    }
  else if (key == gimp_get_extend_selection_mask ())
    {
      g_object_set (options, "fill-selection", ! options->fill_selection, NULL);
    }
}

static void
gimp_bucket_fill_tool_cursor_update (GimpTool         *tool,
                                     const GimpCoords *coords,
                                     GdkModifierType   state,
                                     GimpDisplay      *display)
{
  GimpBucketFillOptions *options  = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  GimpCursorModifier     modifier = GIMP_CURSOR_MODIFIER_BAD;
  GimpImage             *image    = gimp_display_get_image (display);

  if (gimp_image_coords_in_active_pickable (image, coords,
                                            options->sample_merged, TRUE))
    {
      GimpDrawable *drawable = gimp_image_get_active_drawable (image);

      if (! gimp_viewable_get_children (GIMP_VIEWABLE (drawable)) &&
          ! gimp_item_is_content_locked (GIMP_ITEM (drawable))    &&
          gimp_item_is_visible (GIMP_ITEM (drawable)))
        {
          switch (options->fill_mode)
            {
            case GIMP_BUCKET_FILL_FG:
              modifier = GIMP_CURSOR_MODIFIER_FOREGROUND;
              break;

            case GIMP_BUCKET_FILL_BG:
              modifier = GIMP_CURSOR_MODIFIER_BACKGROUND;
              break;

            case GIMP_BUCKET_FILL_PATTERN:
              modifier = GIMP_CURSOR_MODIFIER_PATTERN;
              break;
            }
        }
    }

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}
