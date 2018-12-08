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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpasync.h"
#include "core/gimpcancelable.h"
#include "core/gimpdrawable-bucket-fill.h"
#include "core/gimpdrawable-edit.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimperror.h"
#include "core/gimpfilloptions.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimplineart.h"
#include "core/gimppickable.h"
#include "core/gimppickable-contiguous-region.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"
#include "core/gimptoolinfo.h"
#include "core/gimpwaitable.h"

#include "gegl/gimp-gegl-nodes.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpbucketfilloptions.h"
#include "gimpbucketfilltool.h"
#include "gimpcoloroptions.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"

#include "gimp-intl.h"


struct _GimpBucketFillToolPrivate
{
  GimpLineArt        *line_art;
  GWeakRef            cached_image;
  GWeakRef            cached_drawable;

  /* For preview */
  GeglNode           *graph;
  GeglNode           *fill_node;
  GeglNode           *offset_node;

  GeglBuffer         *fill_mask;

  GimpDrawableFilter *filter;

  /* Temp property save */
  GimpBucketFillMode  fill_mode;
};

/*  local function prototypes  */

static void     gimp_bucket_fill_tool_constructed      (GObject               *object);
static void     gimp_bucket_fill_tool_finalize         (GObject               *object);

static gboolean gimp_bucket_fill_tool_initialize       (GimpTool              *tool,
                                                        GimpDisplay           *display,
                                                        GError               **error);

static void     gimp_bucket_fill_tool_start            (GimpBucketFillTool    *tool,
                                                        const GimpCoords      *coords,
                                                        GimpDisplay           *display);
static void     gimp_bucket_fill_tool_preview          (GimpBucketFillTool    *tool,
                                                        const GimpCoords      *coords,
                                                        GimpDisplay           *display,
                                                        GimpFillOptions       *fill_options);
static void     gimp_bucket_fill_tool_commit           (GimpBucketFillTool    *tool);
static void     gimp_bucket_fill_tool_halt             (GimpBucketFillTool    *tool);
static void     gimp_bucket_fill_tool_filter_flush     (GimpDrawableFilter    *filter,
                                                        GimpTool              *tool);
static void     gimp_bucket_fill_tool_create_graph     (GimpBucketFillTool    *tool);

static void     gimp_bucket_fill_tool_button_press     (GimpTool              *tool,
                                                        const GimpCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        GimpButtonPressType    press_type,
                                                        GimpDisplay           *display);
static void     gimp_bucket_fill_tool_motion           (GimpTool              *tool,
                                                        const GimpCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        GimpDisplay           *display);
static void     gimp_bucket_fill_tool_button_release   (GimpTool              *tool,
                                                        const GimpCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        GimpButtonReleaseType  release_type,
                                                        GimpDisplay           *display);
static void     gimp_bucket_fill_tool_modifier_key     (GimpTool              *tool,
                                                        GdkModifierType        key,
                                                        gboolean               press,
                                                        GdkModifierType        state,
                                                        GimpDisplay           *display);
static void     gimp_bucket_fill_tool_cursor_update    (GimpTool              *tool,
                                                        const GimpCoords      *coords,
                                                        GdkModifierType        state,
                                                        GimpDisplay           *display);

static gboolean gimp_bucket_fill_tool_connect_handlers (gpointer               data);
static void     gimp_bucket_fill_tool_options_notified (GimpBucketFillOptions *options,
                                                        GParamSpec            *pspec,
                                                        GimpBucketFillTool    *tool);
static void     gimp_bucket_fill_reset_line_art        (GimpBucketFillTool    *tool,
                                                        GimpBucketFillOptions *options);
static void     gimp_bucket_fill_tool_image_changed    (GimpContext           *context,
                                                        GimpImage             *image,
                                                        GimpBucketFillTool    *tool);
static void     gimp_bucket_fill_tool_drawable_changed (GimpImage            *image,
                                                        GimpBucketFillTool   *tool);


G_DEFINE_TYPE_WITH_PRIVATE (GimpBucketFillTool, gimp_bucket_fill_tool, GIMP_TYPE_COLOR_TOOL)

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
                GIMP_ICON_TOOL_BUCKET_FILL,
                data);
}

static void
gimp_bucket_fill_tool_class_init (GimpBucketFillToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->constructed  = gimp_bucket_fill_tool_constructed;
  object_class->finalize     = gimp_bucket_fill_tool_finalize;

  tool_class->initialize     = gimp_bucket_fill_tool_initialize;
  tool_class->button_press   = gimp_bucket_fill_tool_button_press;
  tool_class->motion         = gimp_bucket_fill_tool_motion;
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

  bucket_fill_tool->priv = gimp_bucket_fill_tool_get_instance_private (bucket_fill_tool);
}

static void
gimp_bucket_fill_tool_constructed (GObject *object)
{
  GimpTool              *tool    = GIMP_TOOL (object);
  GimpBucketFillOptions *options = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (object);
  Gimp                  *gimp    = GIMP_CONTEXT (options)->gimp;
  GimpLineArt           *line_art;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  line_art = gimp_line_art_new ();
  g_object_bind_property (options,  "fill-transparent",
                          line_art, "select-transparent",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (options,  "line-art-threshold",
                          line_art, "threshold",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (options,  "line-art-max-grow",
                          line_art, "max-grow",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (options,  "line-art-spline-max-len",
                          line_art, "spline-max-length",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (options,  "line-art-segment-max-len",
                          line_art, "segment-max-length",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  GIMP_BUCKET_FILL_TOOL (tool)->priv->line_art = line_art;

  /* Avoid computing initial line art several times (for every option
   * property as it gets deserialized) if tool is selected when starting
   * GIMP with an image.
   */
  if (gimp_is_restored (gimp))
    gimp_bucket_fill_tool_connect_handlers (tool);
  else
    g_idle_add (gimp_bucket_fill_tool_connect_handlers, tool);

  GIMP_COLOR_TOOL (tool)->pick_target = (options->fill_mode == GIMP_BUCKET_FILL_BG) ?
                                           GIMP_COLOR_PICK_TARGET_BACKGROUND : GIMP_COLOR_PICK_TARGET_FOREGROUND;
}

static void
gimp_bucket_fill_tool_finalize (GObject *object)
{
  GimpBucketFillTool    *tool     = GIMP_BUCKET_FILL_TOOL (object);
  GimpBucketFillOptions *options  = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  Gimp                  *gimp     = GIMP_CONTEXT (options)->gimp;
  GimpContext           *context  = gimp_get_user_context (gimp);
  GimpImage             *image    = g_weak_ref_get (&tool->priv->cached_image);

  g_clear_object (&tool->priv->line_art);

  if (image)
    {
      g_signal_handlers_disconnect_by_data (image, tool);
      g_object_unref (image);
    }

  g_signal_handlers_disconnect_by_data (options, tool);
  g_signal_handlers_disconnect_by_data (context, tool);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The active layer is not visible."));
      return FALSE;
    }

  if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The active layer's pixels are locked."));
      if (error)
        gimp_tools_blink_lock_box (display->gimp, GIMP_ITEM (drawable));
      return FALSE;
    }

  return TRUE;
}

static void
gimp_bucket_fill_tool_start (GimpBucketFillTool *tool,
                             const GimpCoords   *coords,
                             GimpDisplay        *display)
{
  GimpBucketFillOptions *options  = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  GimpContext           *context  = GIMP_CONTEXT (options);
  GimpImage             *image    = gimp_display_get_image (display);
  GimpDrawable          *drawable = gimp_image_get_active_drawable (image);

  g_return_if_fail (! tool->priv->filter);

  gimp_line_art_freeze (tool->priv->line_art);

  GIMP_TOOL (tool)->display  = display;
  GIMP_TOOL (tool)->drawable = drawable;

  gimp_bucket_fill_tool_create_graph (tool);

  tool->priv->filter = gimp_drawable_filter_new (drawable, _("Bucket fill"),
                                                 tool->priv->graph,
                                                 GIMP_ICON_TOOL_BUCKET_FILL);

  gimp_drawable_filter_set_region (tool->priv->filter, GIMP_FILTER_REGION_DRAWABLE);

  /* We only set these here, and don't need to update it since we assume
   * the settings can't change while the fill started.
   */
  gimp_drawable_filter_set_mode (tool->priv->filter,
                                 gimp_context_get_paint_mode (context),
                                 GIMP_LAYER_COLOR_SPACE_AUTO,
                                 GIMP_LAYER_COLOR_SPACE_AUTO,
                                 gimp_layer_mode_get_paint_composite_mode (gimp_context_get_paint_mode (context)));
  gimp_drawable_filter_set_opacity (tool->priv->filter,
                                    gimp_context_get_opacity (context));

  g_signal_connect (tool->priv->filter, "flush",
                    G_CALLBACK (gimp_bucket_fill_tool_filter_flush),
                    tool);
}

static void
gimp_bucket_fill_tool_preview (GimpBucketFillTool *tool,
                               const GimpCoords   *coords,
                               GimpDisplay        *display,
                               GimpFillOptions    *fill_options)
{
  GimpBucketFillOptions *options  = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  GimpImage             *image    = gimp_display_get_image (display);
  GimpDrawable          *drawable = gimp_image_get_active_drawable (image);

  if (tool->priv->filter)
    {
      GeglBuffer *fill     = NULL;
      gdouble     x        = coords->x;
      gdouble     y        = coords->y;

      if (! options->sample_merged)
        {
          gint off_x, off_y;

          gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

          x -= (gdouble) off_x;
          y -= (gdouble) off_y;
        }

      fill = gimp_drawable_get_bucket_fill_buffer (drawable,
                                                   tool->priv->line_art,
                                                   fill_options,
                                                   options->fill_transparent,
                                                   options->fill_criterion,
                                                   options->threshold / 255.0,
                                                   options->sample_merged,
                                                   options->diagonal_neighbors,
                                                   x, y, &tool->priv->fill_mask,
                                                   &x, &y, NULL, NULL);
      if (fill)
        {
          gegl_node_set (tool->priv->fill_node,
                         "buffer", fill,
                         NULL);
          gegl_node_set (tool->priv->offset_node,
                         "x", x,
                         "y", y,
                         NULL);
          gimp_drawable_filter_apply (tool->priv->filter, NULL);
          g_object_unref (fill);
        }
    }
}

static void
gimp_bucket_fill_tool_commit (GimpBucketFillTool *tool)
{
  if (tool->priv->filter)
    {
      gimp_drawable_filter_commit (tool->priv->filter,
                                   GIMP_PROGRESS (tool), FALSE);
      gimp_image_flush (gimp_display_get_image (GIMP_TOOL (tool)->display));
    }
}

static void
gimp_bucket_fill_tool_halt (GimpBucketFillTool *tool)
{
  if (tool->priv->graph)
    {
      g_clear_object (&tool->priv->graph);
      tool->priv->fill_node      = NULL;
      tool->priv->offset_node    = NULL;
    }
  if (tool->priv->filter)
    {
      gimp_drawable_filter_abort (tool->priv->filter);
      g_clear_object (&tool->priv->filter);
    }
  g_clear_object (&tool->priv->fill_mask);

  gimp_line_art_thaw (tool->priv->line_art);

  GIMP_TOOL (tool)->display  = NULL;
  GIMP_TOOL (tool)->drawable = NULL;
}

static void
gimp_bucket_fill_tool_filter_flush (GimpDrawableFilter *filter,
                                    GimpTool           *tool)
{
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush (gimp_image_get_projection (image));
}

static void
gimp_bucket_fill_tool_create_graph (GimpBucketFillTool *tool)
{
  GeglNode *graph;
  GeglNode *output;
  GeglNode *fill_node;
  GeglNode *offset_node;

  g_return_if_fail (! tool->priv->graph     &&
                    ! tool->priv->fill_node &&
                    ! tool->priv->offset_node);

  graph = gegl_node_new ();

  fill_node = gegl_node_new_child (graph,
                                   "operation", "gegl:buffer-source",
                                   NULL);
  offset_node = gegl_node_new_child (graph,
                                     "operation", "gegl:translate",
                                     NULL);
  output = gegl_node_get_output_proxy (graph, "output");
  gegl_node_link_many (fill_node, offset_node, output, NULL);

  tool->priv->graph       = graph;
  tool->priv->fill_node   = fill_node;
  tool->priv->offset_node = offset_node;
}

static void
gimp_bucket_fill_tool_button_press (GimpTool            *tool,
                                    const GimpCoords    *coords,
                                    guint32              time,
                                    GdkModifierType      state,
                                    GimpButtonPressType  press_type,
                                    GimpDisplay         *display)
{
  GimpBucketFillTool    *bucket_tool = GIMP_BUCKET_FILL_TOOL (tool);
  GimpBucketFillOptions *options     = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  GimpImage             *image       = gimp_display_get_image (display);

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
      return;
    }

  if (press_type == GIMP_BUTTON_PRESS_NORMAL &&
      gimp_image_coords_in_active_pickable (image, coords,
                                            options->sample_merged, TRUE))
    {
      GimpDrawable    *drawable = gimp_image_get_active_drawable (image);
      GimpContext     *context  = GIMP_CONTEXT (options);
      GimpFillOptions *fill_options;
      GError          *error = NULL;

      fill_options = gimp_fill_options_new (image->gimp, NULL, FALSE);

      if (gimp_fill_options_set_by_fill_mode (fill_options, context,
                                              options->fill_mode,
                                              &error))
        {
          gimp_fill_options_set_antialias (fill_options, options->antialias);

          gimp_context_set_opacity (GIMP_CONTEXT (fill_options),
                                    gimp_context_get_opacity (context));
          gimp_context_set_paint_mode (GIMP_CONTEXT (fill_options),
                                       gimp_context_get_paint_mode (context));

          if (options->fill_selection)
            {
              gimp_drawable_edit_fill (drawable, fill_options, NULL);
              gimp_image_flush (image);
            }
          else
            {
              gimp_bucket_fill_tool_start (bucket_tool, coords, display);
              gimp_bucket_fill_tool_preview (bucket_tool, coords, display, fill_options);
            }
        }
      else
        {
          gimp_message_literal (display->gimp, G_OBJECT (display),
                                GIMP_MESSAGE_WARNING, error->message);
          g_clear_error (&error);
        }

      g_object_unref (fill_options);
    }

  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);
}

static void
gimp_bucket_fill_tool_motion (GimpTool         *tool,
                              const GimpCoords *coords,
                              guint32           time,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpBucketFillTool    *bucket_tool = GIMP_BUCKET_FILL_TOOL (tool);
  GimpBucketFillOptions *options     = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  GimpImage             *image       = gimp_display_get_image (display);

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    return;

  if (gimp_image_coords_in_active_pickable (image, coords,
                                            options->sample_merged, TRUE) &&
      /* Fill selection only needs to happen once. */
      ! options->fill_selection)
    {
      GimpContext     *context  = GIMP_CONTEXT (options);
      GimpFillOptions *fill_options;
      GError          *error = NULL;

      fill_options = gimp_fill_options_new (image->gimp, NULL, FALSE);

      if (gimp_fill_options_set_by_fill_mode (fill_options, context,
                                              options->fill_mode,
                                              &error))
        {
          gimp_fill_options_set_antialias (fill_options, options->antialias);

          gimp_context_set_opacity (GIMP_CONTEXT (fill_options),
                                    gimp_context_get_opacity (context));
          gimp_context_set_paint_mode (GIMP_CONTEXT (fill_options),
                                       gimp_context_get_paint_mode (context));

          gimp_bucket_fill_tool_preview (bucket_tool, coords, display, fill_options);
        }
      else
        {
          gimp_message_literal (display->gimp, G_OBJECT (display),
                                GIMP_MESSAGE_WARNING, error->message);
          g_clear_error (&error);
        }

      g_object_unref (fill_options);
    }
}

static void
gimp_bucket_fill_tool_button_release (GimpTool              *tool,
                                      const GimpCoords      *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      GimpButtonReleaseType  release_type,
                                      GimpDisplay           *display)
{
  GimpBucketFillTool *bucket_tool = GIMP_BUCKET_FILL_TOOL (tool);
  gboolean            commit;

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time,
                                                      state, release_type,
                                                      display);
      return;
    }

  commit = (release_type != GIMP_BUTTON_RELEASE_CANCEL);

  if (commit)
    gimp_bucket_fill_tool_commit (bucket_tool);

  gimp_bucket_fill_tool_halt (bucket_tool);

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

  if (key == GDK_MOD1_MASK)
    {
      if (press)
        {
          GIMP_BUCKET_FILL_TOOL (tool)->priv->fill_mode = options->fill_mode;
          switch (options->fill_mode)
            {
            case GIMP_BUCKET_FILL_FG:
              g_object_set (options, "fill-mode", GIMP_BUCKET_FILL_BG, NULL);
              break;

            default: /* GIMP_BUCKET_FILL_BG && GIMP_BUCKET_FILL_PATTERN */
              g_object_set (options, "fill-mode", GIMP_BUCKET_FILL_FG, NULL);
              break;

              break;
            }
        }
      else /* release */
        {
          g_object_set (options, "fill-mode",
                        GIMP_BUCKET_FILL_TOOL (tool)->priv->fill_mode,
                        NULL);
        }
    }
  else if (key == gimp_get_toggle_behavior_mask ())
    {
      GimpToolInfo *info = gimp_get_tool_info (display->gimp,
                                               "gimp-color-picker-tool");
      if (! gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
        {
          switch (GIMP_COLOR_TOOL (tool)->pick_target)
            {
            case GIMP_COLOR_PICK_TARGET_BACKGROUND:
              gimp_tool_push_status (tool, display,
                                     _("Click in any image to pick the "
                                       "background color"));
              break;

            case GIMP_COLOR_PICK_TARGET_FOREGROUND:
            default:
              gimp_tool_push_status (tool, display,
                                     _("Click in any image to pick the "
                                       "foreground color"));
              break;
            }
          GIMP_TOOL (tool)->display = display;
          gimp_color_tool_enable (GIMP_COLOR_TOOL (tool),
                                  GIMP_COLOR_OPTIONS (info->tool_options));
        }
      else
        {
          gimp_tool_pop_status (tool, display);
          gimp_color_tool_disable (GIMP_COLOR_TOOL (tool));
          GIMP_TOOL (tool)->display = NULL;
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

static gboolean
gimp_bucket_fill_tool_connect_handlers (gpointer data)
{
  GimpBucketFillTool    *tool    = GIMP_BUCKET_FILL_TOOL (data);
  GimpBucketFillOptions *options = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  Gimp                  *gimp    = GIMP_CONTEXT (options)->gimp;

  if (gimp_is_restored (gimp))
    {
      GimpContext *context = gimp_get_user_context (gimp);
      GimpImage   *image   = gimp_context_get_image (context);

      g_signal_connect (options, "notify::fill-criterion",
                        G_CALLBACK (gimp_bucket_fill_tool_options_notified),
                        tool);
      g_signal_connect (options, "notify::sample-merged",
                        G_CALLBACK (gimp_bucket_fill_tool_options_notified),
                        tool);
      g_signal_connect (options, "notify::fill-mode",
                        G_CALLBACK (gimp_bucket_fill_tool_options_notified),
                        tool);

      g_signal_connect (context, "image-changed",
                        G_CALLBACK (gimp_bucket_fill_tool_image_changed),
                        tool);
      gimp_bucket_fill_tool_image_changed (context, image, GIMP_BUCKET_FILL_TOOL (tool));

      return G_SOURCE_REMOVE;
    }
  return G_SOURCE_CONTINUE;
}

static void
gimp_bucket_fill_tool_options_notified (GimpBucketFillOptions *options,
                                        GParamSpec            *pspec,
                                        GimpBucketFillTool    *tool)
{
  if (! strcmp (pspec->name, "fill-criterion") ||
      ! strcmp (pspec->name, "sample-merged"))
    {
      gimp_bucket_fill_reset_line_art (tool, options);
    }
  else if (! strcmp (pspec->name, "fill-mode"))
    {
      if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
        gimp_tool_pop_status (GIMP_TOOL (tool), GIMP_TOOL (tool)->display);

      switch (options->fill_mode)
        {
        case GIMP_BUCKET_FILL_BG:
          GIMP_COLOR_TOOL (tool)->pick_target = GIMP_COLOR_PICK_TARGET_BACKGROUND;
          if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
            gimp_tool_push_status (GIMP_TOOL (tool), GIMP_TOOL (tool)->display,
                                   _("Click in any image to pick the "
                                     "background color"));
          break;

        case GIMP_BUCKET_FILL_FG:
        default:
          GIMP_COLOR_TOOL (tool)->pick_target = GIMP_COLOR_PICK_TARGET_FOREGROUND;
          if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
            gimp_tool_push_status (GIMP_TOOL (tool), GIMP_TOOL (tool)->display,
                                   _("Click in any image to pick the "
                                     "foreground color"));
          break;
        }
    }
}

static void
gimp_bucket_fill_reset_line_art (GimpBucketFillTool    *tool,
                                 GimpBucketFillOptions *options)
{
  GimpImage   *prev_image = g_weak_ref_get (&tool->priv->cached_image);
  GimpContext *context    = gimp_get_user_context (GIMP_CONTEXT (options)->gimp);
  GimpImage   *image      = gimp_context_get_image (context);

  if (prev_image)
    {
      g_signal_handlers_disconnect_by_data (prev_image, tool);
      g_object_unref (prev_image);
    }
  g_weak_ref_set (&tool->priv->cached_image, image ? image : NULL);
  g_weak_ref_set (&tool->priv->cached_drawable, NULL);

  if (image && options->fill_criterion == GIMP_SELECT_CRITERION_LINE_ART)
    {
      GimpBucketFillOptions *options  = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
      GimpDrawable          *drawable = gimp_image_get_active_drawable (image);

      g_signal_connect (image, "active-layer-changed",
                        G_CALLBACK (gimp_bucket_fill_tool_drawable_changed),
                        tool);
      g_signal_connect (image, "active-channel-changed",
                        G_CALLBACK (gimp_bucket_fill_tool_drawable_changed),
                        tool);

      g_weak_ref_set (&tool->priv->cached_drawable, drawable ? drawable : NULL);

      if (options->sample_merged)
        gimp_line_art_set_input (tool->priv->line_art, GIMP_PICKABLE (image));
      else if (drawable)
        gimp_line_art_set_input (tool->priv->line_art, GIMP_PICKABLE (drawable));
      else
        gimp_line_art_set_input (tool->priv->line_art, NULL);
    }
  else
    {
      gimp_line_art_set_input (tool->priv->line_art, NULL);
    }
}

static void
gimp_bucket_fill_tool_image_changed (GimpContext        *context,
                                     GimpImage          *image,
                                     GimpBucketFillTool *tool)
{
  GimpImage *prev_image = g_weak_ref_get (&tool->priv->cached_image);

  if (image != prev_image)
    {
      if (prev_image)
        {
          g_signal_handlers_disconnect_by_data (prev_image, tool);
        }

      g_weak_ref_set (&tool->priv->cached_image, image ? image : NULL);
      g_weak_ref_set (&tool->priv->cached_drawable, NULL);
      if (image)
        {
          GimpBucketFillOptions *options = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

          g_signal_connect (image, "active-layer-changed",
                            G_CALLBACK (gimp_bucket_fill_tool_drawable_changed),
                            tool);
          g_signal_connect (image, "active-channel-changed",
                            G_CALLBACK (gimp_bucket_fill_tool_drawable_changed),
                            tool);
          gimp_bucket_fill_tool_drawable_changed (image, tool);

          if (options->sample_merged)
            gimp_line_art_set_input (tool->priv->line_art,
                                     GIMP_PICKABLE (image));
        }
      else
        {
          gimp_line_art_set_input (tool->priv->line_art, NULL);
        }
    }
  if (prev_image)
    g_object_unref (prev_image);
}

static void
gimp_bucket_fill_tool_drawable_changed (GimpImage          *image,
                                        GimpBucketFillTool *tool)
{
  GimpDrawable *drawable      = gimp_image_get_active_drawable (image);
  GimpDrawable *prev_drawable = g_weak_ref_get (&tool->priv->cached_drawable);

  if (drawable != prev_drawable)
    {
      GimpBucketFillOptions *options = GIMP_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

      g_weak_ref_set (&tool->priv->cached_drawable, drawable ? drawable : NULL);
      if (! options->sample_merged)
        gimp_line_art_set_input (tool->priv->line_art,
                                 drawable ? GIMP_PICKABLE (drawable) : NULL);
    }
  if (prev_drawable)
    g_object_unref (prev_drawable);
}
