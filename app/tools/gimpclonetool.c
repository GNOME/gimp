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

#include "operations/layer-modes/gimp-layer-modes.h"

#include "core/gimppickable.h"

#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-mask-combine.h"

#include "paint/gimpclone.h"
#include "paint/gimpcloneoptions.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpcanvasbufferpreview.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpclonetool.h"
#include "gimpcloneoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void             gimp_clone_tool_motion         (GimpTool         *tool,
                                                        const GimpCoords *coords,
                                                        guint32           time,
                                                        GdkModifierType   state,
                                                        GimpDisplay      *display);
static GimpCanvasItem * gimp_clone_tool_get_outline    (GimpPaintTool    *paint_tool,
                                                        GimpDisplay      *display,
                                                        gdouble           x,
                                                        gdouble           y);
static gboolean         gimp_clone_tool_is_alpha_only (GimpPaintTool     *paint_tool,
                                                       GimpDrawable      *drawable);


G_DEFINE_TYPE (GimpCloneTool, gimp_clone_tool, GIMP_TYPE_SOURCE_TOOL)

#define parent_class gimp_clone_tool_parent_class


void
gimp_clone_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_CLONE_TOOL,
                GIMP_TYPE_CLONE_OPTIONS,
                gimp_clone_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PROP_MASK_EXPAND   |
                GIMP_CONTEXT_PROP_MASK_PATTERN,
                "gimp-clone-tool",
                _("Clone"),
                _("Clone Tool: Selectively copy from an image or pattern, using a brush"),
                N_("_Clone"), "C",
                NULL, GIMP_HELP_TOOL_CLONE,
                GIMP_ICON_TOOL_CLONE,
                data);
}

static void
gimp_clone_tool_class_init (GimpCloneToolClass *klass)
{
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpPaintToolClass *paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  tool_class->motion              = gimp_clone_tool_motion;

  paint_tool_class->is_alpha_only = gimp_clone_tool_is_alpha_only;
  paint_tool_class->get_outline   = gimp_clone_tool_get_outline;
}

static void
gimp_clone_tool_init (GimpCloneTool *clone)
{
  GimpTool       *tool        = GIMP_TOOL (clone);
  GimpPaintTool  *paint_tool  = GIMP_PAINT_TOOL (tool);
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (tool);

  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_CLONE);
  gimp_tool_control_set_action_object_2 (tool->control,
                                         "context-pattern-select-set");

  paint_tool->status      = _("Click to clone");
  paint_tool->status_ctrl = _("%s to set a new clone source");

  source_tool->status_paint           = _("Click to clone");
  source_tool->status_set_source      = _("Click to set a new clone source");
  source_tool->status_set_source_ctrl = _("%s to set a new clone source");
  source_tool->status_set_source_ctrl_click = _("%s-click to set a new clone source");
}

static void
gimp_clone_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);
}

static GimpCanvasItem *
gimp_clone_tool_get_outline (GimpPaintTool *paint_tool,
                             GimpDisplay   *display,
                             gdouble        x,
                             gdouble        y)
{
  GimpCanvasItem    *item        = NULL;
  GimpDisplayShell  *shell       = gimp_display_get_shell (display);
  GimpTool          *tool        = GIMP_TOOL (paint_tool);
  GimpSourceTool    *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpPaintOptions  *paint_options;
  GimpSourceOptions *source_options;
  gfloat             brush_size;

  paint_options  = GIMP_PAINT_TOOL_GET_OPTIONS (paint_tool);
  source_options = GIMP_SOURCE_TOOL_GET_OPTIONS (source_tool);

  brush_size = paint_options->brush_size;
  if (source_options->src_pickable && ! source_tool->src_handle)
    {
      GeglBuffer    *src_buffer;
      GeglBuffer    *preview_buffer;
      GeglRectangle  bounding_box;

      bounding_box = gimp_display_shell_get_bounding_box (shell);
      src_buffer   = gimp_pickable_get_buffer (source_options->src_pickable);

      preview_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                        bounding_box.width,
                                                        bounding_box.height),
                                        babl_format ("R'G'B'A float"));

      gimp_gegl_buffer_copy (src_buffer,
                             GEGL_RECTANGLE (source_options->src_x - (brush_size / 2),
                                             source_options->src_y - (brush_size / 2),
                                             brush_size, brush_size),
                             GEGL_ABYSS_NONE,
                             preview_buffer,
                             GEGL_RECTANGLE (x - (brush_size / 2),
                                             y - (brush_size / 2), brush_size,
                                             brush_size));

      gimp_gegl_mask_combine_ellipse (preview_buffer, GIMP_CHANNEL_OP_INTERSECT,
                                      x - (brush_size / 2),
                                      y - (brush_size / 2), brush_size,
                                      brush_size, FALSE);

      item = gimp_canvas_buffer_preview_new (shell, preview_buffer);
      g_object_unref (preview_buffer);
    }
  else
    {
      gimp_paint_tool_set_draw_circle (paint_tool, TRUE, brush_size);

    }

  return item;
}

static gboolean
gimp_clone_tool_is_alpha_only (GimpPaintTool *paint_tool,
                               GimpDrawable  *drawable)
{
  GimpPaintOptions *paint_options = GIMP_PAINT_TOOL_GET_OPTIONS (paint_tool);
  GimpContext      *context       = GIMP_CONTEXT (paint_options);
  GimpLayerMode     paint_mode    = gimp_context_get_paint_mode (context);

  return gimp_layer_mode_is_alpha_only (paint_mode);
}
