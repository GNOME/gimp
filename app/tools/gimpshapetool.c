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
#include "core/gimpimage-merge.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimprasterizable.h"

#include "path/gimpbezierstroke.h"
#include "path/gimppath.h"
#include "path/gimpvectorlayer.h"
#include "path/gimpvectorlayeroptions.h"

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


#define MAX_NUMBER_OF_POINTS 102


#define GIMP_SHAPE_TOOL_GET_OPTIONS(t)  (GIMP_SHAPE_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))

/*  local function prototypes  */
static void             gimp_shape_tool_constructed    (GObject               *object);
static void             gimp_shape_tool_dispose        (GObject               *object);

static void             gimp_shape_tool_control        (GimpTool              *tool,
                                                        GimpToolAction         action,
                                                        GimpDisplay           *display);
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

static void             gimp_shape_tool_draw           (GimpDrawTool          *draw_tool);
static void             gimp_shape_tool_halt           (GimpShapeTool         *shape_tool);

static GimpPath *       gimp_shape_tool_create_path    (GimpShapeTool         *shape_tool,
                                                        GimpImage             *image);
static void             gimp_shape_tool_update_polygon (GimpShapeTool         *shape_tool,
                                                        gboolean               is_star);
static gchar    *       gimp_shape_tool_get_name       (GimpShapeTool         *shape_tool);

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
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = gimp_shape_tool_constructed;
  object_class->dispose      = gimp_shape_tool_dispose;

  tool_class->control        = gimp_shape_tool_control;
  tool_class->button_press   = gimp_shape_tool_button_press;
  tool_class->button_release = gimp_shape_tool_button_release;
  tool_class->motion         = gimp_shape_tool_motion;
  tool_class->oper_update    = gimp_shape_tool_oper_update;
  tool_class->cursor_update  = gimp_shape_tool_cursor_update;
  tool_class->is_destructive = FALSE;

  draw_tool_class->draw      = gimp_shape_tool_draw;
}

static void
gimp_shape_tool_init (GimpShapeTool *shape_tool)
{
  GimpTool *tool = GIMP_TOOL (shape_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_MOVE);
}

static void
gimp_shape_tool_constructed (GObject *object)
{
  GimpShapeTool *shape_tool = GIMP_SHAPE_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* TODO: Get max value + 2 from number of sides property */
  shape_tool->points = g_new (GimpVector2, MAX_NUMBER_OF_POINTS);
}

static void
gimp_shape_tool_dispose (GObject *object)
{
  GimpShapeTool *shape_tool = GIMP_SHAPE_TOOL (object);

  G_OBJECT_CLASS (parent_class)->dispose (object);

  g_free (shape_tool->points);
}

static void
gimp_shape_tool_control (GimpTool       *tool,
                         GimpToolAction  action,
                         GimpDisplay    *display)
{
  GimpShapeTool *shape_tool = GIMP_SHAPE_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_shape_tool_halt (shape_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_shape_tool_button_press (GimpTool            *tool,
                              const GimpCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              GimpButtonPressType  press_type,
                              GimpDisplay         *display)
{
  GimpShapeTool *shape_tool = GIMP_SHAPE_TOOL (tool);

  if (tool->display && display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      tool->display = display;

      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
    }
  gimp_tool_control_activate (tool->control);

  /* Set starting point */
  shape_tool->start_x = coords->x;
  shape_tool->start_y = coords->y;
  shape_tool->drawing = TRUE;
}

static void
gimp_shape_tool_button_release (GimpTool              *tool,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpShapeTool     *shape_tool = GIMP_SHAPE_TOOL (tool);
  GimpShapeOptions  *options    = GIMP_SHAPE_TOOL_GET_OPTIONS (tool);
  GimpImage         *image      = gimp_display_get_image (display);

  if (shape_tool->drawing == TRUE)
    {
      GimpPath        *path         = NULL;
      GimpVectorLayer *vector_layer = NULL;
      gchar           *path_name;
      gchar           *undo_string;

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));
      shape_tool->drawing = FALSE;

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

      path_name   = gimp_shape_tool_get_name (shape_tool);
      undo_string = g_strdup_printf (_("Add %s"), path_name);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_DRAWABLE,
                                   undo_string);
      g_free (path_name);
      g_free (undo_string);

      path = gimp_shape_tool_create_path (shape_tool, image);
      if (path)
        {
          GimpVectorLayerOptions *vector_options = NULL;

          vector_layer = gimp_vector_layer_new (image, path,
                                                gimp_get_user_context (image->gimp));
          gimp_image_add_layer (image, GIMP_LAYER (vector_layer),
                                GIMP_IMAGE_ACTIVE_PARENT,
                                -1, TRUE);
          gimp_vector_layer_set (vector_layer, NULL,
                                 "enable-fill", options->enable_fill,
                                 NULL);

          vector_options = gimp_vector_layer_get_options (vector_layer);
          g_object_set (vector_options->stroke_options,
                        "width", options->stroke_width,
                        "unit",  options->stroke_unit,
                        NULL);

          gimp_item_set_visible (GIMP_ITEM (vector_layer), TRUE, FALSE);
          gimp_vector_layer_refresh (vector_layer);
        }

      /* TODO: Possible use stroke/fill rather than making a vector layer */
      if (vector_layer                 &&
          options->rasterize_on_commit &&
          ! gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (vector_layer)))
        {
          GList *layers = NULL;

          gimp_rasterizable_rasterize (GIMP_RASTERIZABLE (vector_layer),
                                       FALSE);

          layers = g_list_prepend (NULL, GIMP_LAYER (vector_layer));
          gimp_image_merge_down (image, layers,
                                 gimp_get_user_context (image->gimp),
                                 GIMP_EXPAND_AS_NECESSARY,
                                 NULL, NULL, NULL);

          gimp_image_remove_path (image, path, TRUE, NULL);
          path = NULL;
        }

      if (path)
        gimp_image_flush (image);

      gimp_image_undo_group_end (image);
    }

  gimp_tool_control_halt (tool->control);
}

static void
gimp_shape_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{
  GimpShapeTool *shape_tool = GIMP_SHAPE_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* Update cursor */
  shape_tool->current_x = coords->x;
  shape_tool->current_y = coords->y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
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

static void
gimp_shape_tool_draw (GimpDrawTool *draw_tool)
{
  GimpShapeTool    *shape_tool = GIMP_SHAPE_TOOL (draw_tool);
  GimpShapeOptions *options    = GIMP_SHAPE_TOOL_GET_OPTIONS (shape_tool);

  if (shape_tool->drawing)
    {
      if (options->shape_type == GIMP_SHAPE_MODE_LINE)
        {
          gimp_draw_tool_add_line (draw_tool, shape_tool->start_x,
                                   shape_tool->start_y, shape_tool->current_x,
                                   shape_tool->current_y);
        }
      else if (options->shape_type == GIMP_SHAPE_MODE_RECTANGLE)
        {
          gimp_draw_tool_add_rectangle (draw_tool, options->enable_fill,
                                        MIN (shape_tool->start_x, shape_tool->current_x),
                                        MIN (shape_tool->start_y, shape_tool->current_y),
                                        ABS (shape_tool->start_x - shape_tool->current_x),
                                        ABS (shape_tool->start_y - shape_tool->current_y));
        }
      else if (options->shape_type == GIMP_SHAPE_MODE_ARC)
        {
          /* Since we can't use negative width/height to flip the circle, we
           * swap the start and current x,y coordinates based on where we're
           * dragging the circle */
          gimp_draw_tool_add_arc (draw_tool, options->enable_fill,
                                  MIN (shape_tool->start_x, shape_tool->current_x),
                                  MIN (shape_tool->start_y, shape_tool->current_y),
                                  ABS (shape_tool->start_x - shape_tool->current_x),
                                  ABS (shape_tool->start_y - shape_tool->current_y),
                                  0, 2 * G_PI);

        }
      else if (options->shape_type == GIMP_SHAPE_MODE_POLYGON ||
               options->shape_type == GIMP_SHAPE_MODE_STAR)
        {
          gboolean is_star = (options->shape_type == GIMP_SHAPE_MODE_STAR);
          gint     coeff   = (is_star) ? 2 : 1;

          gimp_shape_tool_update_polygon (shape_tool, is_star);

          gimp_draw_tool_add_lines (draw_tool, shape_tool->points,
                                    (options->number_of_sides * coeff) + 1,
                                    NULL, options->enable_fill);
        }
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
gimp_shape_tool_halt (GimpShapeTool *shape_tool)
{
  GimpTool *tool = GIMP_TOOL (shape_tool);

  if (tool->display)
    gimp_tool_pop_status (tool, tool->display);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), NULL);

  tool->display = NULL;
}

static GimpPath *
gimp_shape_tool_create_path (GimpShapeTool *shape_tool,
                             GimpImage     *image)
{
  GimpStroke       *stroke;
  GimpPath         *path    = NULL;
  GimpShapeOptions *options = GIMP_SHAPE_TOOL_GET_OPTIONS (shape_tool);
  GimpCoords        next    = GIMP_COORDS_DEFAULT_VALUES;
  gchar            *path_name;

  path_name = gimp_shape_tool_get_name (shape_tool);

  path = gimp_path_new (image, path_name);
  gimp_image_add_path (image, path,
                       GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  g_free (path_name);

  if (options->shape_type == GIMP_SHAPE_MODE_LINE)
    {
      next.x = shape_tool->start_x;
      next.y = shape_tool->start_y;
      stroke = gimp_bezier_stroke_new_moveto (&next);

      next.x = shape_tool->current_x;
      next.y = shape_tool->current_y;
      gimp_bezier_stroke_lineto (stroke, &next);

      gimp_path_stroke_add (path, stroke);
      g_object_unref (stroke);
    }
  else if (options->shape_type == GIMP_SHAPE_MODE_RECTANGLE)
    {
      next.x = shape_tool->start_x;
      next.y = shape_tool->start_y;
      stroke = gimp_bezier_stroke_new_moveto (&next);

      next.x = shape_tool->current_x;
      gimp_bezier_stroke_lineto (stroke, &next);

      next.y = shape_tool->current_y;
      gimp_bezier_stroke_lineto (stroke, &next);

      next.x = shape_tool->start_x;
      gimp_bezier_stroke_lineto (stroke, &next);

      gimp_stroke_close (stroke);

      gimp_path_stroke_add (path, stroke);
      g_object_unref (stroke);
    }
  else if (options->shape_type == GIMP_SHAPE_MODE_ARC)
    {
      gdouble rx = (shape_tool->start_x - shape_tool->current_x) / 2.0f;
      gdouble ry = (shape_tool->start_y - shape_tool->current_y) / 2.0f;

      next.x = shape_tool->start_x - rx;
      next.y = shape_tool->start_y - ry;

      stroke = gimp_bezier_stroke_new_ellipse (&next, ABS (rx), ABS (ry), 0.0);
      gimp_path_stroke_add (path, stroke);
      g_object_unref (stroke);
    }
  else if (options->shape_type == GIMP_SHAPE_MODE_POLYGON ||
           options->shape_type == GIMP_SHAPE_MODE_STAR)
    {
      gint n_sides = options->number_of_sides;

      if (options->shape_type == GIMP_SHAPE_MODE_STAR)
        n_sides *= 2;

      next.x = shape_tool->points[0].x;
      next.y = shape_tool->points[0].y;
      stroke = gimp_bezier_stroke_new_moveto (&next);

      for (gint i = 1; i < n_sides; i++)
        {
          next.x = shape_tool->points[i].x;
          next.y = shape_tool->points[i].y;
          gimp_bezier_stroke_lineto (stroke, &next);
        }

      gimp_stroke_close (stroke);

      gimp_path_stroke_add (path, stroke);
      g_object_unref (stroke);
    }

  return path;
}

static void
gimp_shape_tool_update_polygon (GimpShapeTool *shape_tool,
                                gboolean       is_star)
{
  GimpShapeOptions *options = GIMP_SHAPE_TOOL_GET_OPTIONS (shape_tool);

  gint              coeff   = (is_star) ? 2 : 1;
  gint              n_sides = options->number_of_sides * coeff;
  gdouble           rx      = shape_tool->current_x - shape_tool->start_x;
  gdouble           ry      = shape_tool->current_y - shape_tool->start_y;
  gdouble           radius  = sqrt ((rx * rx) + (ry * ry));
  gdouble           angle   = (2 * G_PI) / n_sides;
  gdouble           offset  = atan2 (ry, rx);

  for (gint i = 0; i <= n_sides; i++)
    {
      gdouble loop_angle = (i * angle) + offset;
      gdouble new_x      = radius * cos (loop_angle);
      gdouble new_y      = radius * sin (loop_angle);

      if (is_star)
        {
          gdouble star_radius = radius * ((i % 2) + 1) / 2;

          new_x  = star_radius * cos (loop_angle);
          new_y  = star_radius * sin (loop_angle);
        }

      shape_tool->points[i].x = shape_tool->start_x + new_x;
      shape_tool->points[i].y = shape_tool->start_y + new_y;
    }
}

static gchar *
gimp_shape_tool_get_name (GimpShapeTool *shape_tool)
{
  GimpShapeOptions *options    = GIMP_SHAPE_TOOL_GET_OPTIONS (shape_tool);
  gchar            *shape_name = NULL;

  switch (options->shape_type)
    {
      case GIMP_SHAPE_MODE_LINE:
        shape_name = g_strdup (_("Line"));
        break;

      case GIMP_SHAPE_MODE_RECTANGLE:
        shape_name = g_strdup (_("Rectangle"));
        break;

      case GIMP_SHAPE_MODE_ARC:
        shape_name = g_strdup (_("Circle"));
        break;

      case GIMP_SHAPE_MODE_POLYGON:
        shape_name = g_strdup (_("Polygon"));
        break;

      case GIMP_SHAPE_MODE_STAR:
        shape_name = g_strdup (_("Star"));
        break;

      default:
        break;
    }

  return shape_name;
}
