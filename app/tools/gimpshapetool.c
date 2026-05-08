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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"
#include "core/gimprasterizable.h"

#include "path/gimpbezierstroke.h"
#include "path/gimppath.h"
#include "path/gimpvectorlayer.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsizebox.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimptoolgui.h"
#include "display/gimptooltransformgrid.h"

#include "gimpshapeoptions.h"
#include "gimpshapetool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define GIMP_SHAPE_TOOL_GET_OPTIONS(t)  (GIMP_SHAPE_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


#define COORDS_INIT   \
  {                   \
    .x         = 0.0, \
    .y         = 0.0, \
    .pressure  = 1.0, \
    .xtilt     = 0.0, \
    .ytilt     = 0.0, \
    .wheel     = 0.5, \
    .velocity  = 0.0, \
    .direction = 0.0  \
  }


/*  local function prototypes  */

static void             gimp_shape_tool_button_press   (GimpTool              *tool,
                                                        const GimpCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        GimpButtonPressType    press_type,
                                                        GimpDisplay           *display);
static void             gimp_shape_tool_button_release (GimpTool              *tool,
                                                        const GimpCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        GimpButtonReleaseType  release_type,
                                                        GimpDisplay           *display);
static void             gimp_shape_tool_motion         (GimpTool              *tool,
                                                        const GimpCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        GimpDisplay           *display);

static void             gimp_shape_tool_oper_update    (GimpTool              *tool,
                                                        const GimpCoords      *coords,
                                                        GdkModifierType        state,
                                                        gboolean               proximity,
                                                        GimpDisplay           *display);
static void             gimp_shape_tool_cursor_update  (GimpTool              *tool,
                                                        const GimpCoords      *coords,
                                                        GdkModifierType        state,
                                                        GimpDisplay           *display);



G_DEFINE_TYPE (GimpShapeTool, gimp_shape_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_shape_tool_parent_class


void
gimp_shape_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_SHAPE_TOOL,
                GIMP_TYPE_SHAPE_OPTIONS,
                gimp_shape_options_gui,
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                "gimp-shape-tool",
                _("Shape"),
                _("Shape Tool: Create vector shapes"),
                N_("Sh_ape"), NULL,
                NULL, GIMP_HELP_TOOL_SHAPE,
                GIMP_ICON_TOOL_SHAPE,
                data);
}

static void
gimp_shape_tool_class_init (GimpShapeToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->button_press   = gimp_shape_tool_button_press;
  tool_class->button_release = gimp_shape_tool_button_release;
  tool_class->motion         = gimp_shape_tool_motion;
  tool_class->oper_update    = gimp_shape_tool_oper_update;
  tool_class->cursor_update  = gimp_shape_tool_cursor_update;
  tool_class->is_destructive = FALSE;

  draw_tool_class->draw      = NULL;
}

static void
gimp_shape_tool_init (GimpShapeTool *shape_tool)
{
  GimpTool *tool = GIMP_TOOL (shape_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_MOVE);
}

static void
gimp_shape_tool_button_press (GimpTool            *tool,
                              const GimpCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              GimpButtonPressType  press_type,
                              GimpDisplay         *display)
{
  GimpShapeTool     *shape_tool     = GIMP_SHAPE_TOOL (tool);
  GimpShapeOptions  *options        = GIMP_SHAPE_TOOL_GET_OPTIONS (tool);
  GimpImage         *image          = gimp_display_get_image (display);
  GimpPath          *path;
  GimpStroke        *stroke;
  GimpVectorLayer   *vector_layer;
  GList             *selected_shape = NULL;
  gchar             *shape_name     = NULL;

  tool->display = display;

  shape_tool->start_x = coords->x;
  shape_tool->start_y = coords->y;

  if (options->shape_type == 0)
    shape_name = "Rectangle";
  else if (options->shape_type == 1)
    shape_name = "Circle";
  else if (options->shape_type == 2)
    shape_name = "Triangle";

  path = gimp_path_new (image, shape_name);
  gimp_image_add_path (image, path,
                       GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  if (options->shape_type == 0)
    {
      GimpCoords next = COORDS_INIT;

      next.x = coords->x;
      next.y = coords->y;

      stroke = gimp_bezier_stroke_new_moveto (coords);

      next.x++;
      gimp_bezier_stroke_lineto (stroke, &next);

      next.y++;
      gimp_bezier_stroke_lineto (stroke, &next);

      next.x--;
      gimp_bezier_stroke_lineto (stroke, &next);

      next.y--;
      gimp_bezier_stroke_lineto (stroke, &next);

      gimp_path_stroke_add (path, stroke);
      g_object_unref (stroke);
    }
  else if (options->shape_type == 1)
    {
      stroke = gimp_bezier_stroke_new_ellipse (coords, 1, 1, 180.0);
      gimp_path_stroke_add (path, stroke);
      g_object_unref (stroke);
    }
  else if (options->shape_type == 2)
    {
      GimpCoords next = COORDS_INIT;

      next.x = coords->x;
      next.y = coords->y;

      stroke = gimp_bezier_stroke_new_moveto (coords);

      next.x += 0.5;
      next.y--;
      gimp_bezier_stroke_lineto (stroke, &next);

      next.x += 0.5;
      next.y++;
      gimp_bezier_stroke_lineto (stroke, &next);

      next.x--;
      gimp_bezier_stroke_lineto (stroke, &next);

      gimp_path_stroke_add (path, stroke);
      g_object_unref (stroke);
    }

  vector_layer = gimp_vector_layer_new (image, path,
                                        gimp_get_user_context (image->gimp));
  gimp_image_add_layer (image, GIMP_LAYER (vector_layer),
                        GIMP_IMAGE_ACTIVE_PARENT,
                        -1, TRUE);
  gimp_item_set_visible (GIMP_ITEM (vector_layer), TRUE, FALSE);
  gimp_vector_layer_refresh (vector_layer);

  selected_shape = g_list_append (selected_shape, vector_layer);
  gimp_image_set_selected_layers (image, selected_shape);
  g_list_free (selected_shape);

  gimp_image_flush (image);

  gimp_tool_control_activate (tool->control);
}

static void
gimp_shape_tool_button_release (GimpTool              *tool,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpShapeTool     *shape   = GIMP_SHAPE_TOOL (tool);
  GimpShapeOptions  *options = GIMP_SHAPE_TOOL_GET_OPTIONS (tool);
  GimpImage         *image   = gimp_display_get_image (display);
  GimpPath          *path;
  GList             *shape_layers;
  gdouble            width;
  gdouble            height;

  gimp_tool_control_halt (tool->control);

  shape_layers = gimp_image_get_selected_layers (image);
  path         = gimp_vector_layer_get_path (shape_layers->data);

  width  = coords->x - shape->start_x;
  height = coords->y - shape->start_y;

  gimp_item_scale_by_factors_with_origin (GIMP_ITEM (path), ABS (width), ABS (height),
                                          shape->start_x, shape->start_y,
                                          shape->start_x, shape->start_y,
                                          GIMP_INTERPOLATION_NONE, NULL);
  gimp_vector_layer_refresh (shape_layers->data);

  if (options->rasterize_on_commit &&
      ! gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (shape_layers->data)))
    {
      gimp_rasterizable_rasterize (GIMP_RASTERIZABLE (shape_layers->data),
                                   FALSE);
    }

  gimp_image_flush (image);
}

static void
gimp_shape_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{
}

static void
gimp_shape_tool_oper_update (GimpTool         *tool,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             GimpDisplay      *display)
{
}

static void
gimp_shape_tool_cursor_update (GimpTool         *tool,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               GimpDisplay      *display)
{
  GimpCursorType      cursor      = GIMP_CURSOR_MOUSE;
  GimpToolCursorType  tool_cursor = GIMP_TOOL_CURSOR_MOVE;
  GimpCursorModifier  modifier    = GIMP_CURSOR_MODIFIER_NONE;

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}
