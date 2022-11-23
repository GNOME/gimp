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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmaasync.h"
#include "core/ligmacancelable.h"
#include "core/ligmacontainer.h"
#include "core/ligmadrawable-bucket-fill.h"
#include "core/ligmadrawable-edit.h"
#include "core/ligmadrawablefilter.h"
#include "core/ligmaerror.h"
#include "core/ligmafilloptions.h"
#include "core/ligmaimage.h"
#include "core/ligmaimageproxy.h"
#include "core/ligmaitem.h"
#include "core/ligmalineart.h"
#include "core/ligmapickable.h"
#include "core/ligmapickable-contiguous-region.h"
#include "core/ligmaprogress.h"
#include "core/ligmaprojection.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmawaitable.h"

#include "gegl/ligma-gegl-nodes.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "ligmabucketfilloptions.h"
#include "ligmabucketfilltool.h"
#include "ligmacoloroptions.h"
#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"

#include "ligma-intl.h"


struct _LigmaBucketFillToolPrivate
{
  LigmaLineArt        *line_art;
  LigmaImage          *line_art_image;
  LigmaDisplayShell   *line_art_shell;
  GList              *line_art_bindings;

  /* For preview */
  GeglNode           *graph;
  GeglNode           *fill_node;
  GeglNode           *offset_node;

  GeglBuffer         *fill_mask;

  LigmaDrawableFilter *filter;

  /* Temp property save */
  LigmaBucketFillMode  fill_mode;
  LigmaBucketFillArea  fill_area;
};


/*  local function prototypes  */

static void     ligma_bucket_fill_tool_constructed      (GObject               *object);
static void     ligma_bucket_fill_tool_finalize         (GObject               *object);

static void     ligma_bucket_fill_tool_button_press     (LigmaTool              *tool,
                                                        const LigmaCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        LigmaButtonPressType    press_type,
                                                        LigmaDisplay           *display);
static void     ligma_bucket_fill_tool_motion           (LigmaTool              *tool,
                                                        const LigmaCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        LigmaDisplay           *display);
static void     ligma_bucket_fill_tool_button_release   (LigmaTool              *tool,
                                                        const LigmaCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        LigmaButtonReleaseType  release_type,
                                                        LigmaDisplay           *display);
static void     ligma_bucket_fill_tool_modifier_key     (LigmaTool              *tool,
                                                        GdkModifierType        key,
                                                        gboolean               press,
                                                        GdkModifierType        state,
                                                        LigmaDisplay           *display);
static void     ligma_bucket_fill_tool_cursor_update    (LigmaTool              *tool,
                                                        const LigmaCoords      *coords,
                                                        GdkModifierType        state,
                                                        LigmaDisplay           *display);
static void     ligma_bucket_fill_tool_options_notify   (LigmaTool              *tool,
                                                        LigmaToolOptions       *options,
                                                        const GParamSpec      *pspec);

static void ligma_bucket_fill_tool_line_art_computing_start (LigmaBucketFillTool *tool);
static void ligma_bucket_fill_tool_line_art_computing_end   (LigmaBucketFillTool *tool);

static gboolean ligma_bucket_fill_tool_coords_in_active_pickable
                                                       (LigmaBucketFillTool    *tool,
                                                        LigmaDisplay           *display,
                                                        const LigmaCoords      *coords);

static void     ligma_bucket_fill_tool_start            (LigmaBucketFillTool    *tool,
                                                        const LigmaCoords      *coords,
                                                        LigmaDisplay           *display);
static void     ligma_bucket_fill_tool_preview          (LigmaBucketFillTool    *tool,
                                                        const LigmaCoords      *coords,
                                                        LigmaDisplay           *display,
                                                        LigmaFillOptions       *fill_options);
static void     ligma_bucket_fill_tool_commit           (LigmaBucketFillTool    *tool);
static void     ligma_bucket_fill_tool_halt             (LigmaBucketFillTool    *tool);
static void     ligma_bucket_fill_tool_filter_flush     (LigmaDrawableFilter    *filter,
                                                        LigmaTool              *tool);
static void     ligma_bucket_fill_tool_create_graph     (LigmaBucketFillTool    *tool);

static void     ligma_bucket_fill_tool_reset_line_art   (LigmaBucketFillTool    *tool);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaBucketFillTool, ligma_bucket_fill_tool,
                            LIGMA_TYPE_COLOR_TOOL)

#define parent_class ligma_bucket_fill_tool_parent_class


void
ligma_bucket_fill_tool_register (LigmaToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (LIGMA_TYPE_BUCKET_FILL_TOOL,
                LIGMA_TYPE_BUCKET_FILL_OPTIONS,
                ligma_bucket_fill_options_gui,
                LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND |
                LIGMA_CONTEXT_PROP_MASK_OPACITY    |
                LIGMA_CONTEXT_PROP_MASK_PAINT_MODE |
                LIGMA_CONTEXT_PROP_MASK_PATTERN,
                "ligma-bucket-fill-tool",
                _("Bucket Fill"),
                _("Bucket Fill Tool: Fill selected area with a color or pattern"),
                N_("_Bucket Fill"), "<shift>B",
                NULL, LIGMA_HELP_TOOL_BUCKET_FILL,
                LIGMA_ICON_TOOL_BUCKET_FILL,
                data);
}

static void
ligma_bucket_fill_tool_class_init (LigmaBucketFillToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaToolClass *tool_class   = LIGMA_TOOL_CLASS (klass);

  object_class->constructed  = ligma_bucket_fill_tool_constructed;
  object_class->finalize     = ligma_bucket_fill_tool_finalize;

  tool_class->button_press   = ligma_bucket_fill_tool_button_press;
  tool_class->motion         = ligma_bucket_fill_tool_motion;
  tool_class->button_release = ligma_bucket_fill_tool_button_release;
  tool_class->modifier_key   = ligma_bucket_fill_tool_modifier_key;
  tool_class->cursor_update  = ligma_bucket_fill_tool_cursor_update;
  tool_class->options_notify = ligma_bucket_fill_tool_options_notify;
}

static void
ligma_bucket_fill_tool_init (LigmaBucketFillTool *bucket_fill_tool)
{
  LigmaTool *tool = LIGMA_TOOL (bucket_fill_tool);

  ligma_tool_control_set_scroll_lock     (tool->control, TRUE);
  ligma_tool_control_set_wants_click     (tool->control, TRUE);
  ligma_tool_control_set_tool_cursor     (tool->control,
                                         LIGMA_TOOL_CURSOR_BUCKET_FILL);
  ligma_tool_control_set_action_opacity  (tool->control,
                                         "context/context-opacity-set");
  ligma_tool_control_set_action_object_1 (tool->control,
                                         "context/context-pattern-select-set");

  bucket_fill_tool->priv =
    ligma_bucket_fill_tool_get_instance_private (bucket_fill_tool);
}

static void
ligma_bucket_fill_tool_constructed (GObject *object)
{
  LigmaTool              *tool        = LIGMA_TOOL (object);
  LigmaBucketFillTool    *bucket_tool = LIGMA_BUCKET_FILL_TOOL (object);
  LigmaBucketFillOptions *options     = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  Ligma                  *ligma        = LIGMA_CONTEXT (options)->ligma;
  LigmaContext           *context     = ligma_get_user_context (ligma);
  GList                 *bindings    = NULL;
  GBinding              *binding;
  LigmaLineArt           *line_art;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_tool_control_set_motion_mode (tool->control,
                                     options->fill_area == LIGMA_BUCKET_FILL_LINE_ART ?
                                     LIGMA_MOTION_MODE_EXACT : LIGMA_MOTION_MODE_COMPRESS);

  line_art = ligma_context_take_line_art (context);
  binding = g_object_bind_property (options,  "fill-transparent",
                                    line_art, "select-transparent",
                                    G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bindings = g_list_prepend (bindings, binding);
  binding = g_object_bind_property (options,  "line-art-threshold",
                                    line_art, "threshold",
                                    G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bindings = g_list_prepend (bindings, binding);
  binding = g_object_bind_property (options,  "line-art-max-grow",
                                    line_art, "max-grow",
                                    G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bindings = g_list_prepend (bindings, binding);
  binding = g_object_bind_property (options,  "line-art-automatic-closure",
                                    line_art, "automatic-closure",
                                    G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bindings = g_list_prepend (bindings, binding);
  binding = g_object_bind_property (options,  "line-art-max-gap-length",
                                    line_art, "spline-max-length",
                                    G_BINDING_SYNC_CREATE | G_BINDING_DEFAULT);
  bindings = g_list_prepend (bindings, binding);
  binding = g_object_bind_property (options,  "line-art-max-gap-length",
                                    line_art, "segment-max-length",
                                    G_BINDING_SYNC_CREATE | G_BINDING_DEFAULT);
  bindings = g_list_prepend (bindings, binding);
  g_signal_connect_swapped (line_art, "computing-start",
                            G_CALLBACK (ligma_bucket_fill_tool_line_art_computing_start),
                            tool);
  g_signal_connect_swapped (line_art, "computing-end",
                            G_CALLBACK (ligma_bucket_fill_tool_line_art_computing_end),
                            tool);
  ligma_line_art_bind_gap_length (line_art, TRUE);
  bucket_tool->priv->line_art = line_art;
  bucket_tool->priv->line_art_bindings = bindings;

  ligma_bucket_fill_tool_reset_line_art (bucket_tool);

  g_signal_connect_swapped (options, "notify::line-art-source",
                            G_CALLBACK (ligma_bucket_fill_tool_reset_line_art),
                            tool);
  g_signal_connect_swapped (context, "display-changed",
                            G_CALLBACK (ligma_bucket_fill_tool_reset_line_art),
                            tool);

  LIGMA_COLOR_TOOL (tool)->pick_target =
    (options->fill_mode == LIGMA_BUCKET_FILL_BG) ?
    LIGMA_COLOR_PICK_TARGET_BACKGROUND : LIGMA_COLOR_PICK_TARGET_FOREGROUND;
}

static void
ligma_bucket_fill_tool_finalize (GObject *object)
{
  LigmaBucketFillTool    *tool     = LIGMA_BUCKET_FILL_TOOL (object);
  LigmaBucketFillOptions *options  = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  Ligma                  *ligma     = LIGMA_CONTEXT (options)->ligma;
  LigmaContext           *context  = ligma_get_user_context (ligma);

  if (tool->priv->line_art_image)
    {
      g_signal_handlers_disconnect_by_data (ligma_image_get_layers (tool->priv->line_art_image), tool);
      g_signal_handlers_disconnect_by_data (tool->priv->line_art_image, tool);
      tool->priv->line_art_image = NULL;
    }
  if (tool->priv->line_art_shell)
    {
      g_signal_handlers_disconnect_by_func (
        tool->priv->line_art_shell,
        ligma_bucket_fill_tool_reset_line_art,
        tool);
      tool->priv->line_art_shell = NULL;
    }

  /* We don't free the line art object, but gives temporary ownership to
   * the user context which will free it if a timer runs out.
   *
   * This way, we allow people to not suffer a new computational delay
   * if for instance they just needed to switch tools for a few seconds
   * while the source layer stays the same.
   */
  g_signal_handlers_disconnect_by_data (tool->priv->line_art, tool);
  g_list_free_full (tool->priv->line_art_bindings,
                    (GDestroyNotify) g_binding_unbind);
  ligma_context_store_line_art (context, tool->priv->line_art);
  tool->priv->line_art = NULL;

  g_signal_handlers_disconnect_by_data (options, tool);
  g_signal_handlers_disconnect_by_data (context, tool);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
ligma_bucket_fill_tool_coords_in_active_pickable (LigmaBucketFillTool *tool,
                                                 LigmaDisplay        *display,
                                                 const LigmaCoords   *coords)
{
  LigmaBucketFillOptions *options       = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  LigmaDisplayShell      *shell         = ligma_display_get_shell (display);
  LigmaImage             *image         = ligma_display_get_image (display);
  gboolean               sample_merged = FALSE;

  switch (options->fill_area)
    {
    case LIGMA_BUCKET_FILL_SELECTION:
      break;

    case LIGMA_BUCKET_FILL_SIMILAR_COLORS:
      sample_merged = options->sample_merged;
      break;

    case LIGMA_BUCKET_FILL_LINE_ART:
      sample_merged = options->line_art_source ==
                      LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED;
      break;
    }

  return ligma_image_coords_in_active_pickable (image, coords,
                                               shell->show_all, sample_merged,
                                               TRUE);
}

static void
ligma_bucket_fill_tool_start (LigmaBucketFillTool *tool,
                             const LigmaCoords   *coords,
                             LigmaDisplay        *display)
{
  LigmaBucketFillOptions *options   = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  LigmaContext           *context   = LIGMA_CONTEXT (options);
  LigmaImage             *image     = ligma_display_get_image (display);
  GList                 *drawables = ligma_image_get_selected_drawables (image);

  g_return_if_fail (! tool->priv->filter);
  g_return_if_fail (g_list_length (drawables) == 1);

  ligma_line_art_freeze (tool->priv->line_art);

  LIGMA_TOOL (tool)->display  = display;
  g_list_free (LIGMA_TOOL (tool)->drawables);
  LIGMA_TOOL (tool)->drawables = drawables;

  ligma_bucket_fill_tool_create_graph (tool);

  tool->priv->filter = ligma_drawable_filter_new (drawables->data, _("Bucket fill"),
                                                 tool->priv->graph,
                                                 LIGMA_ICON_TOOL_BUCKET_FILL);

  ligma_drawable_filter_set_region (tool->priv->filter,
                                   LIGMA_FILTER_REGION_DRAWABLE);

  /* We only set these here, and don't need to update it since we assume
   * the settings can't change while the fill started.
   */
  ligma_drawable_filter_set_mode (tool->priv->filter,
                                 ligma_context_get_paint_mode (context),
                                 LIGMA_LAYER_COLOR_SPACE_AUTO,
                                 LIGMA_LAYER_COLOR_SPACE_AUTO,
                                 ligma_layer_mode_get_paint_composite_mode (ligma_context_get_paint_mode (context)));
  ligma_drawable_filter_set_opacity (tool->priv->filter,
                                    ligma_context_get_opacity (context));

  g_signal_connect (tool->priv->filter, "flush",
                    G_CALLBACK (ligma_bucket_fill_tool_filter_flush),
                    tool);
}

static void
ligma_bucket_fill_tool_preview (LigmaBucketFillTool *tool,
                               const LigmaCoords   *coords,
                               LigmaDisplay        *display,
                               LigmaFillOptions    *fill_options)
{
  LigmaBucketFillOptions *options   = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  LigmaDisplayShell      *shell     = ligma_display_get_shell (display);
  LigmaImage             *image     = ligma_display_get_image (display);
  GList                 *drawables = ligma_image_get_selected_drawables (image);
  LigmaDrawable          *drawable;

  g_return_if_fail (g_list_length (drawables) == 1);

  drawable = drawables->data;
  g_list_free (drawables);

  if (tool->priv->filter)
    {
      GeglBuffer *fill     = NULL;
      gdouble     x        = coords->x;
      gdouble     y        = coords->y;

      if (options->fill_area == LIGMA_BUCKET_FILL_SIMILAR_COLORS)
        {
          if (! options->sample_merged)
            {
              gint off_x, off_y;

              ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

              x -= (gdouble) off_x;
              y -= (gdouble) off_y;
            }
          fill = ligma_drawable_get_bucket_fill_buffer (drawable,
                                                       fill_options,
                                                       options->fill_transparent,
                                                       options->fill_criterion,
                                                       options->threshold / 255.0,
                                                       shell->show_all,
                                                       options->sample_merged,
                                                       options->diagonal_neighbors,
                                                       x, y,
                                                       &tool->priv->fill_mask,
                                                       &x, &y, NULL, NULL);
        }
      else
        {
          gint source_off_x = 0;
          gint source_off_y = 0;

          if (options->line_art_source != LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED)
            {
              LigmaPickable *input;

              input = ligma_line_art_get_input (tool->priv->line_art);
              g_return_if_fail (LIGMA_IS_ITEM (input));

              ligma_item_get_offset (LIGMA_ITEM (input), &source_off_x, &source_off_y);

              x -= (gdouble) source_off_x;
              y -= (gdouble) source_off_y;
            }
          fill = ligma_drawable_get_line_art_fill_buffer (drawable,
                                                         tool->priv->line_art,
                                                         fill_options,
                                                         options->line_art_source ==
                                                         LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED,
                                                         options->fill_as_line_art &&
                                                         options->fill_mode != LIGMA_BUCKET_FILL_PATTERN,
                                                         options->fill_as_line_art_threshold / 255.0,
                                                         options->line_art_stroke,
                                                         options->stroke_options,
                                                         x, y,
                                                         &tool->priv->fill_mask,
                                                         &x, &y, NULL, NULL);
          if (options->line_art_source != LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED)
            {
              gint off_x, off_y;

              ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

              x -= (gdouble) off_x - (gdouble) source_off_x;
              y -= (gdouble) off_y - (gdouble) source_off_y;
            }
        }
      if (fill)
        {
          gegl_node_set (tool->priv->fill_node,
                         "buffer", fill,
                         NULL);
          gegl_node_set (tool->priv->offset_node,
                         "x", x,
                         "y", y,
                         NULL);
          ligma_drawable_filter_apply (tool->priv->filter, NULL);
          g_object_unref (fill);
        }
    }
}

static void
ligma_bucket_fill_tool_commit (LigmaBucketFillTool *tool)
{
  if (tool->priv->filter)
    {
      ligma_drawable_filter_commit (tool->priv->filter,
                                   LIGMA_PROGRESS (tool), FALSE);
      ligma_image_flush (ligma_display_get_image (LIGMA_TOOL (tool)->display));
    }
}

static void
ligma_bucket_fill_tool_halt (LigmaBucketFillTool *tool)
{
  if (tool->priv->graph)
    {
      g_clear_object (&tool->priv->graph);
      tool->priv->fill_node   = NULL;
      tool->priv->offset_node = NULL;
    }

  if (tool->priv->filter)
    {
      ligma_drawable_filter_abort (tool->priv->filter);
      g_clear_object (&tool->priv->filter);
    }

  g_clear_object (&tool->priv->fill_mask);

  if (ligma_line_art_is_frozen (tool->priv->line_art))
    ligma_line_art_thaw (tool->priv->line_art);

  LIGMA_TOOL (tool)->display   = NULL;
  g_list_free (LIGMA_TOOL (tool)->drawables);
  LIGMA_TOOL (tool)->drawables = NULL;
}

static void
ligma_bucket_fill_tool_filter_flush (LigmaDrawableFilter *filter,
                                    LigmaTool           *tool)
{
  LigmaImage *image = ligma_display_get_image (tool->display);

  ligma_projection_flush (ligma_image_get_projection (image));
}

static void
ligma_bucket_fill_tool_create_graph (LigmaBucketFillTool *tool)
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
ligma_bucket_fill_tool_button_press (LigmaTool            *tool,
                                    const LigmaCoords    *coords,
                                    guint32              time,
                                    GdkModifierType      state,
                                    LigmaButtonPressType  press_type,
                                    LigmaDisplay         *display)
{
  LigmaBucketFillTool    *bucket_tool = LIGMA_BUCKET_FILL_TOOL (tool);
  LigmaBucketFillOptions *options     = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  LigmaGuiConfig         *config      = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaImage             *image       = ligma_display_get_image (display);
  LigmaItem              *locked_item = NULL;
  GList                 *drawables   = ligma_image_get_selected_drawables (image);
  LigmaDrawable          *drawable;

  if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
    {
      LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
      return;
    }

  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        ligma_tool_message_literal (tool, display,
                                   _("Cannot fill multiple layers. Select only one layer."));
      else
        ligma_tool_message_literal (tool, display, _("No selected drawables."));

      g_list_free (drawables);
      return;
    }

  drawable = drawables->data;
  g_list_free (drawables);

  if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable)))
    {
      ligma_tool_message_literal (tool, display,
                                 _("Cannot modify the pixels of layer groups."));
      return;
    }

  if (! ligma_item_is_visible (LIGMA_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      ligma_tool_message_literal (tool, display,
                                 _("The active layer is not visible."));
      return;
    }

  if (ligma_item_is_content_locked (LIGMA_ITEM (drawable), &locked_item))
    {
      ligma_tool_message_literal (tool, display,
                                 _("The selected layer's pixels are locked."));
      ligma_tools_blink_lock_box (display->ligma, locked_item);
      return;
    }

  if (options->fill_area == LIGMA_BUCKET_FILL_LINE_ART &&
      ! ligma_line_art_get_input (bucket_tool->priv->line_art))
    {
      ligma_tool_message_literal (tool, display,
                                 _("No valid line art source selected."));
      ligma_blink_dockable (display->ligma, "ligma-tool-options", "line-art-source", NULL, NULL);
      return;
    }

  if (press_type == LIGMA_BUTTON_PRESS_NORMAL &&
      ligma_bucket_fill_tool_coords_in_active_pickable (bucket_tool,
                                                       display, coords))
    {
      LigmaContext     *context  = LIGMA_CONTEXT (options);
      LigmaFillOptions *fill_options;
      GError          *error = NULL;

      fill_options = ligma_fill_options_new (image->ligma, NULL, FALSE);

      if (ligma_fill_options_set_by_fill_mode (fill_options, context,
                                              options->fill_mode,
                                              &error))
        {
          ligma_fill_options_set_antialias (fill_options, options->antialias);
          ligma_fill_options_set_feather (fill_options, options->feather,
                                         options->feather_radius);

          ligma_context_set_opacity (LIGMA_CONTEXT (fill_options),
                                    ligma_context_get_opacity (context));
          ligma_context_set_paint_mode (LIGMA_CONTEXT (fill_options),
                                       ligma_context_get_paint_mode (context));

          if (options->fill_area == LIGMA_BUCKET_FILL_SELECTION)
            {
              ligma_drawable_edit_fill (drawable, fill_options, NULL);
              ligma_image_flush (image);
            }
          else /* LIGMA_BUCKET_FILL_SIMILAR_COLORS || LIGMA_BUCKET_FILL_LINE_ART */
            {
              ligma_bucket_fill_tool_start (bucket_tool, coords, display);
              ligma_bucket_fill_tool_preview (bucket_tool, coords, display,
                                             fill_options);
            }
        }
      else
        {
          ligma_message_literal (display->ligma, G_OBJECT (display),
                                LIGMA_MESSAGE_WARNING, error->message);
          g_clear_error (&error);
        }

      g_object_unref (fill_options);
    }

  LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);
}

static void
ligma_bucket_fill_tool_motion (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              guint32           time,
                              GdkModifierType   state,
                              LigmaDisplay      *display)
{
  LigmaBucketFillTool    *bucket_tool = LIGMA_BUCKET_FILL_TOOL (tool);
  LigmaBucketFillOptions *options     = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  LigmaImage             *image       = ligma_display_get_image (display);

  LIGMA_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
    return;

  if (ligma_bucket_fill_tool_coords_in_active_pickable (bucket_tool,
                                                       display, coords) &&
      /* Fill selection only needs to happen once. */
      options->fill_area != LIGMA_BUCKET_FILL_SELECTION)
    {
      LigmaContext     *context  = LIGMA_CONTEXT (options);
      LigmaFillOptions *fill_options;
      GError          *error = NULL;

      fill_options = ligma_fill_options_new (image->ligma, NULL, FALSE);

      if (ligma_fill_options_set_by_fill_mode (fill_options, context,
                                              options->fill_mode,
                                              &error))
        {
          ligma_fill_options_set_antialias (fill_options, options->antialias);
          ligma_fill_options_set_feather (fill_options, options->feather,
                                         options->feather_radius);

          ligma_context_set_opacity (LIGMA_CONTEXT (fill_options),
                                    ligma_context_get_opacity (context));
          ligma_context_set_paint_mode (LIGMA_CONTEXT (fill_options),
                                       ligma_context_get_paint_mode (context));

          ligma_bucket_fill_tool_preview (bucket_tool, coords, display,
                                         fill_options);
        }
      else
        {
          ligma_message_literal (display->ligma, G_OBJECT (display),
                                LIGMA_MESSAGE_WARNING, error->message);
          g_clear_error (&error);
        }

      g_object_unref (fill_options);
    }
}

static void
ligma_bucket_fill_tool_button_release (LigmaTool              *tool,
                                      const LigmaCoords      *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      LigmaButtonReleaseType  release_type,
                                      LigmaDisplay           *display)
{
  LigmaBucketFillTool    *bucket_tool = LIGMA_BUCKET_FILL_TOOL (tool);
  LigmaBucketFillOptions *options = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
    {
      LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time,
                                                      state, release_type,
                                                      display);
      return;
    }

  if (release_type != LIGMA_BUTTON_RELEASE_CANCEL)
    ligma_bucket_fill_tool_commit (bucket_tool);

  if (options->fill_area != LIGMA_BUCKET_FILL_SELECTION)
    ligma_bucket_fill_tool_halt (bucket_tool);

  LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);
}

static void
ligma_bucket_fill_tool_modifier_key (LigmaTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    LigmaDisplay     *display)
{
  LigmaBucketFillOptions *options = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  if (key == GDK_MOD1_MASK)
    {
      if (press)
        {
          LIGMA_BUCKET_FILL_TOOL (tool)->priv->fill_mode = options->fill_mode;

          switch (options->fill_mode)
            {
            case LIGMA_BUCKET_FILL_FG:
              g_object_set (options, "fill-mode", LIGMA_BUCKET_FILL_BG, NULL);
              break;

            default:
              /* LIGMA_BUCKET_FILL_BG || LIGMA_BUCKET_FILL_PATTERN */
              g_object_set (options, "fill-mode", LIGMA_BUCKET_FILL_FG, NULL);
              break;

              break;
            }
        }
      else /* release */
        {
          g_object_set (options, "fill-mode",
                        LIGMA_BUCKET_FILL_TOOL (tool)->priv->fill_mode,
                        NULL);
        }
    }
  else if (key == ligma_get_toggle_behavior_mask ())
    {
      LigmaToolInfo *info = ligma_get_tool_info (display->ligma,
                                               "ligma-color-picker-tool");

      if (! ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
        {
          switch (LIGMA_COLOR_TOOL (tool)->pick_target)
            {
            case LIGMA_COLOR_PICK_TARGET_BACKGROUND:
              ligma_tool_push_status (tool, display,
                                     _("Click in any image to pick the "
                                       "background color"));
              break;

            case LIGMA_COLOR_PICK_TARGET_FOREGROUND:
            default:
              ligma_tool_push_status (tool, display,
                                     _("Click in any image to pick the "
                                       "foreground color"));
              break;
            }

          LIGMA_TOOL (tool)->display = display;
          ligma_color_tool_enable (LIGMA_COLOR_TOOL (tool),
                                  LIGMA_COLOR_OPTIONS (info->tool_options));
        }
      else
        {
          ligma_tool_pop_status (tool, display);
          ligma_color_tool_disable (LIGMA_COLOR_TOOL (tool));
          LIGMA_TOOL (tool)->display = NULL;
        }
    }
  else if (key == ligma_get_extend_selection_mask ())
    {
      if (press)
        {
          LIGMA_BUCKET_FILL_TOOL (tool)->priv->fill_area = options->fill_area;

          switch (options->fill_area)
            {
            case LIGMA_BUCKET_FILL_SIMILAR_COLORS:
              g_object_set (options,
                            "fill-area", LIGMA_BUCKET_FILL_SELECTION,
                            NULL);
              break;

            default:
              /* LIGMA_BUCKET_FILL_SELECTION || LIGMA_BUCKET_FILL_LINE_ART */
              g_object_set (options,
                            "fill-area", LIGMA_BUCKET_FILL_SIMILAR_COLORS,
                            NULL);
              break;
            }
        }
      else /* release */
        {
          g_object_set (options, "fill-area",
                        LIGMA_BUCKET_FILL_TOOL (tool)->priv->fill_area,
                        NULL);
        }
    }
}

static void
ligma_bucket_fill_tool_cursor_update (LigmaTool         *tool,
                                     const LigmaCoords *coords,
                                     GdkModifierType   state,
                                     LigmaDisplay      *display)
{
  LigmaBucketFillTool    *bucket_tool = LIGMA_BUCKET_FILL_TOOL (tool);
  LigmaBucketFillOptions *options     = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  LigmaGuiConfig         *config      = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaCursorModifier     modifier    = LIGMA_CURSOR_MODIFIER_BAD;
  LigmaImage             *image       = ligma_display_get_image (display);

  if (ligma_bucket_fill_tool_coords_in_active_pickable (bucket_tool,
                                                       display, coords))
    {
      GList        *drawables = ligma_image_get_selected_drawables (image);
      LigmaDrawable *drawable  = NULL;

      if (g_list_length (drawables) == 1)
        drawable = drawables->data;

      if (drawable                                                   &&
          ! ligma_viewable_get_children (LIGMA_VIEWABLE (drawable))    &&
          ! ligma_item_is_content_locked (LIGMA_ITEM (drawable), NULL) &&
          (ligma_item_is_visible (LIGMA_ITEM (drawable)) ||
           config->edit_non_visible))
        {
          switch (options->fill_mode)
            {
            case LIGMA_BUCKET_FILL_FG:
              modifier = LIGMA_CURSOR_MODIFIER_FOREGROUND;
              break;

            case LIGMA_BUCKET_FILL_BG:
              modifier = LIGMA_CURSOR_MODIFIER_BACKGROUND;
              break;

            case LIGMA_BUCKET_FILL_PATTERN:
              modifier = LIGMA_CURSOR_MODIFIER_PATTERN;
              break;
            }
        }

      g_list_free (drawables);
    }

  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_bucket_fill_tool_options_notify (LigmaTool         *tool,
                                      LigmaToolOptions  *options,
                                      const GParamSpec *pspec)
{
  LigmaBucketFillTool    *bucket_tool    = LIGMA_BUCKET_FILL_TOOL (tool);
  LigmaBucketFillOptions *bucket_options = LIGMA_BUCKET_FILL_OPTIONS (options);

  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "fill-area"))
    {
      /* We want more motion events when the tool is used in a paint tool
       * fashion. Unfortunately we only set exact mode in line art fill,
       * because we can't as easily remove events from the similar color
       * mode just because a point has already been selected  (unless
       * threshold were 0, but that's an edge case).
       */
      ligma_tool_control_set_motion_mode (tool->control,
                                         bucket_options->fill_area == LIGMA_BUCKET_FILL_LINE_ART ?
                                         LIGMA_MOTION_MODE_EXACT : LIGMA_MOTION_MODE_COMPRESS);

      ligma_bucket_fill_tool_reset_line_art (bucket_tool);
    }
  else if (! strcmp (pspec->name, "fill-mode"))
    {
      if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
        ligma_tool_pop_status (tool, tool->display);

      switch (bucket_options->fill_mode)
        {
        case LIGMA_BUCKET_FILL_BG:
          LIGMA_COLOR_TOOL (tool)->pick_target = LIGMA_COLOR_PICK_TARGET_BACKGROUND;
          if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
            ligma_tool_push_status (tool, tool->display,
                                   _("Click in any image to pick the "
                                     "background color"));
          break;

        case LIGMA_BUCKET_FILL_FG:
        default:
          LIGMA_COLOR_TOOL (tool)->pick_target = LIGMA_COLOR_PICK_TARGET_FOREGROUND;
          if (ligma_color_tool_is_enabled (LIGMA_COLOR_TOOL (tool)))
            ligma_tool_push_status (tool, tool->display,
                                   _("Click in any image to pick the "
                                     "foreground color"));
          break;
        }
    }
}

static void
ligma_bucket_fill_tool_line_art_computing_start (LigmaBucketFillTool *tool)
{
  LigmaBucketFillOptions *options = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  gtk_widget_show (options->line_art_busy_box);
}

static void
ligma_bucket_fill_tool_line_art_computing_end (LigmaBucketFillTool *tool)
{
  LigmaBucketFillOptions *options = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);

  gtk_widget_hide (options->line_art_busy_box);
}

static void
ligma_bucket_fill_tool_reset_line_art (LigmaBucketFillTool *tool)
{
  LigmaBucketFillOptions *options  = LIGMA_BUCKET_FILL_TOOL_GET_OPTIONS (tool);
  LigmaLineArt           *line_art = tool->priv->line_art;
  LigmaDisplayShell      *shell    = NULL;
  LigmaImage             *image    = NULL;

  if (options->fill_area == LIGMA_BUCKET_FILL_LINE_ART)
    {
      LigmaContext *context;
      LigmaDisplay *display;

      context = ligma_get_user_context (LIGMA_CONTEXT (options)->ligma);
      display = ligma_context_get_display (context);

      if (display)
        {
          shell = ligma_display_get_shell (display);
          image = ligma_display_get_image (display);
        }
    }

  if (image != tool->priv->line_art_image)
    {
      if (tool->priv->line_art_image)
        {
          g_signal_handlers_disconnect_by_data (ligma_image_get_layers (tool->priv->line_art_image), tool);
          g_signal_handlers_disconnect_by_data (tool->priv->line_art_image, tool);
        }

      tool->priv->line_art_image = image;

      if (image)
        {
          g_signal_connect_swapped (image, "selected-layers-changed",
                                    G_CALLBACK (ligma_bucket_fill_tool_reset_line_art),
                                    tool);
          g_signal_connect_swapped (image, "selected-channels-changed",
                                    G_CALLBACK (ligma_bucket_fill_tool_reset_line_art),
                                    tool);

          g_signal_connect_swapped (ligma_image_get_layers (image), "add",
                                    G_CALLBACK (ligma_bucket_fill_tool_reset_line_art),
                                    tool);
          g_signal_connect_swapped (ligma_image_get_layers (image), "remove",
                                    G_CALLBACK (ligma_bucket_fill_tool_reset_line_art),
                                    tool);
          g_signal_connect_swapped (ligma_image_get_layers (image), "reorder",
                                    G_CALLBACK (ligma_bucket_fill_tool_reset_line_art),
                                    tool);
        }
    }

  if (shell != tool->priv->line_art_shell)
    {
      if (tool->priv->line_art_shell)
        {
          g_signal_handlers_disconnect_by_func (
            tool->priv->line_art_shell,
            ligma_bucket_fill_tool_reset_line_art,
            tool);
        }

      tool->priv->line_art_shell = shell;

      if (shell)
        {
          g_signal_connect_swapped (shell, "notify::show-all",
                                    G_CALLBACK (ligma_bucket_fill_tool_reset_line_art),
                                    tool);
        }
    }

  if (image)
    {
      GList        *drawables = ligma_image_get_selected_drawables (image);
      LigmaDrawable *drawable  = NULL;

      if (g_list_length (drawables) == 1)
        {
          drawable = drawables->data;
          if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable)))
            drawable = NULL;
        }
      g_list_free (drawables);

      if (options->line_art_source == LIGMA_LINE_ART_SOURCE_SAMPLE_MERGED)
        {
          LigmaImageProxy *image_proxy = ligma_image_proxy_new (image);

          ligma_image_proxy_set_show_all (image_proxy, shell->show_all);

          ligma_line_art_set_input (line_art, LIGMA_PICKABLE (image_proxy));

          g_object_unref (image_proxy);
        }
      else if (drawable)
        {
          LigmaItem      *parent;
          LigmaContainer *container;
          LigmaObject    *neighbour = NULL;
          LigmaPickable  *source    = NULL;
          gint           index;

          parent = ligma_item_get_parent (LIGMA_ITEM (drawable));
          if (parent)
            container = ligma_viewable_get_children (LIGMA_VIEWABLE (parent));
          else
            container = ligma_image_get_layers (image);

          index = ligma_item_get_index (LIGMA_ITEM (drawable));

          if (options->line_art_source == LIGMA_LINE_ART_SOURCE_ACTIVE_LAYER)
            source = LIGMA_PICKABLE (drawable);
          else if (options->line_art_source == LIGMA_LINE_ART_SOURCE_LOWER_LAYER)
            neighbour = ligma_container_get_child_by_index (container, index + 1);
          else if (options->line_art_source == LIGMA_LINE_ART_SOURCE_UPPER_LAYER)
            neighbour = ligma_container_get_child_by_index (container, index - 1);

          source = neighbour ? LIGMA_PICKABLE (neighbour) : source;
          ligma_line_art_set_input (line_art, source);
        }
      else
        {
          ligma_line_art_set_input (line_art, NULL);
        }
    }
  else
    {
      ligma_line_art_set_input (line_art, NULL);
    }
}
