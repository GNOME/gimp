/* GIMP - The GNU Image Manipulation Program
 *
 * gimpwarptool.c
 * Copyright (C) 2011 Michael Muré <batolettre@gmail.com>
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

#include <string.h>
#include <stdlib.h>

#include <gegl.h>
#include "gegl-utils.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpdrawable-shadow.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"
#include "core/gimplayer.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpwarptool.h"
#include "gimpwarpoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void       gimp_warp_tool_start              (GimpWarpTool          *wt,
                                                     GimpDisplay           *display);

static void       gimp_warp_tool_button_press       (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpButtonPressType    press_type,
                                                     GimpDisplay           *display);
static void       gimp_warp_tool_button_release     (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpButtonReleaseType  release_type,
                                                     GimpDisplay           *display);
static gboolean   gimp_warp_tool_key_press          (GimpTool              *tool,
                                                     GdkEventKey           *kevent,
                                                     GimpDisplay           *display);
static void       gimp_warp_tool_motion             (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpDisplay           *display);
static void       gimp_warp_tool_control            (GimpTool              *tool,
                                                     GimpToolAction         action,
                                                     GimpDisplay           *display);
static void       gimp_warp_tool_cursor_update      (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     GdkModifierType        state,
                                                     GimpDisplay           *display);
static void       gimp_warp_tool_oper_update        (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     GdkModifierType        state,
                                                     gboolean               proximity,
                                                     GimpDisplay           *display);

static gboolean   gimp_warp_tool_stroke_timer       (gpointer               data);
static void       gimp_warp_tool_draw               (GimpDrawTool          *draw_tool);

static void       gimp_warp_tool_create_graph       (GimpWarpTool          *wt);
static void       gimp_warp_tool_create_image_map   (GimpWarpTool          *wt,
                                                     GimpDrawable          *drawable);
static void       gimp_warp_tool_image_map_update   (GimpWarpTool          *wt);
static void       gimp_warp_tool_image_map_flush    (GimpImageMap          *image_map,
                                                     GimpTool              *tool);
static void       gimp_warp_tool_add_op             (GimpWarpTool          *wt);
static void       gimp_warp_tool_undo               (GimpWarpTool          *wt);

G_DEFINE_TYPE (GimpWarpTool, gimp_warp_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_warp_tool_parent_class

#define STROKE_PERIOD 100

void
gimp_warp_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_WARP_TOOL,
                GIMP_TYPE_WARP_OPTIONS,
                gimp_warp_options_gui,
                0,
                "gimp-warp-tool",
                _("Warp Transform"),
                _("Warp Transform: Deform with different tools"),
                N_("_Warp Transform"), "W",
                NULL, GIMP_HELP_TOOL_WARP,
                GIMP_STOCK_TOOL_WARP,
                data);
}

static void
gimp_warp_tool_class_init (GimpWarpToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->button_press   = gimp_warp_tool_button_press;
  tool_class->button_release = gimp_warp_tool_button_release;
  tool_class->key_press      = gimp_warp_tool_key_press;
  tool_class->motion         = gimp_warp_tool_motion;
  tool_class->control        = gimp_warp_tool_control;
  tool_class->cursor_update  = gimp_warp_tool_cursor_update;
  tool_class->oper_update    = gimp_warp_tool_oper_update;

  draw_tool_class->draw      = gimp_warp_tool_draw;
}

static void
gimp_warp_tool_init (GimpWarpTool *self)
{
  GimpTool *tool = GIMP_TOOL (self);

  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE           |
                                     GIMP_DIRTY_IMAGE_STRUCTURE |
                                     GIMP_DIRTY_DRAWABLE        |
                                     GIMP_DIRTY_SELECTION);
  gimp_tool_control_set_wants_click (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_PERSPECTIVE);

  self->coords_buffer   = NULL;
  self->render_node     = NULL;
  self->image_map       = NULL;
}

static void
gimp_warp_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  GimpWarpTool *wt = GIMP_WARP_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      if (wt->coords_buffer)
        {
          gegl_buffer_destroy (wt->coords_buffer);
          wt->coords_buffer = NULL;
        }

      if (wt->graph)
        {
          g_object_unref (wt->graph);
          wt->graph = NULL;
        }

      if (wt->image_map)
        {
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_map_abort (wt->image_map);
          g_object_unref (wt->image_map);
          wt->image_map = NULL;

          gimp_tool_control_set_preserve (tool->control, FALSE);

          gimp_image_flush (gimp_display_get_image (tool->display));
        }

      tool->display = NULL;
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_warp_tool_start (GimpWarpTool *wt,
                      GimpDisplay  *display)
{
  GimpTool      *tool     = GIMP_TOOL (wt);
  GimpImage     *image    = gimp_display_get_image (display);
  GimpDrawable  *drawable = gimp_image_get_active_drawable (image);
  Babl          *format;
  gint           x1, x2, y1, y2;
  GeglRectangle  bbox;

  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

  tool->display = display;

  if (wt->coords_buffer)
    {
      gegl_buffer_destroy (wt->coords_buffer);
      wt->coords_buffer = NULL;
    }

  if (wt->graph)
    {
      g_object_unref (wt->graph);
      wt->graph = NULL;
    }

  if (wt->image_map)
    {
      gimp_image_map_abort (wt->image_map);
      g_object_unref (wt->image_map);
      wt->image_map = NULL;
    }

  /* Create the coords buffer, with the size of the selection */
  format = babl_format_n (babl_type ("float"), 2);

  gimp_channel_bounds (gimp_image_get_mask (image), &x1, &y1, &x2, &y2);

  bbox.x      = MIN (x1, x2);
  bbox.y      = MIN (y1, y2);
  bbox.width  = ABS (x1 - x2);
  bbox.height = ABS (y1 - y2);

  printf ("Initialize coordinate buffer (%d,%d) at %d,%d\n", bbox.width, bbox.height, bbox.x, bbox.y);
  wt->coords_buffer = gegl_buffer_new (&bbox, format);

  gegl_rectangle_set (&wt->last_region, 0, 0, 0, 0);

  gimp_warp_tool_create_image_map (wt, drawable);
  gimp_draw_tool_start (GIMP_DRAW_TOOL (wt), display);
}

static gboolean
gimp_warp_tool_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  GimpWarpTool *wt = GIMP_WARP_TOOL (tool);

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      return TRUE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      gimp_tool_control_set_preserve (tool->control, TRUE);

      gimp_image_map_commit (wt->image_map);
      g_object_unref (wt->image_map);
      wt->image_map = NULL;

      gimp_tool_control_set_preserve (tool->control, FALSE);

      gimp_image_flush (gimp_display_get_image (display));

      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return TRUE;

    case GDK_KEY_Escape:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_warp_tool_motion (GimpTool         *tool,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       GimpDisplay      *display)
{
  GimpWarpTool    *wt       = GIMP_WARP_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  wt->cursor_x = coords->x;
  wt->cursor_y = coords->y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_warp_tool_oper_update (GimpTool         *tool,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            GimpDisplay      *display)
{
  GimpWarpTool *wt        = GIMP_WARP_TOOL (tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (!gimp_draw_tool_is_active (draw_tool))
    {
      gimp_warp_tool_start (wt, display);
    }
  else
    {
      if (display == tool->display)
        {
          gimp_draw_tool_pause (draw_tool);

          wt->cursor_x        = coords->x;
          wt->cursor_y        = coords->y;

          gimp_draw_tool_resume (draw_tool);
        }
    }
}

static void
gimp_warp_tool_button_press (GimpTool            *tool,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type,
                             GimpDisplay         *display)
{
  GimpWarpTool    *wt        = GIMP_WARP_TOOL (tool);

  if (display != tool->display)
    gimp_warp_tool_start (wt, display);

  wt->current_stroke = gegl_path_new ();
  gegl_path_append (wt->current_stroke,
                    'M', coords->x, coords->y);

  gimp_warp_tool_add_op (wt);

  gimp_warp_tool_image_map_update (wt);

  wt->stroke_timer = g_timeout_add (STROKE_PERIOD, gimp_warp_tool_stroke_timer, wt);

  gimp_tool_control_activate (tool->control);
}

void
gimp_warp_tool_button_release (GimpTool              *tool,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  GimpWarpTool    *wt      = GIMP_WARP_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (wt));

  gimp_tool_control_halt (tool->control);

  g_source_remove (wt->stroke_timer);

  printf ("%s\n", gegl_path_to_string (wt->current_stroke));

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      gimp_warp_tool_undo (wt);
    }
  else
    {
      gimp_warp_tool_image_map_update (wt);
    }

  gegl_rectangle_set (&wt->last_region, 0, 0, 0, 0);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_warp_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_PLUS;

  if (tool->display)
    {
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
    }

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
gimp_warp_tool_stroke_timer (gpointer data)
{
  GimpWarpTool    *wt        = GIMP_WARP_TOOL (data);

  gegl_path_append (wt->current_stroke,
                    'L', wt->cursor_x, wt->cursor_y);

  gimp_warp_tool_image_map_update (wt);

  return TRUE;
}

static void
gimp_warp_tool_draw (GimpDrawTool *draw_tool)
{
  GimpWarpTool     *wt      = GIMP_WARP_TOOL (draw_tool);
  GimpWarpOptions  *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);

  gimp_draw_tool_add_arc (draw_tool,
                          FALSE,
                          wt->cursor_x - options->effect_size * 0.5,
                          wt->cursor_y - options->effect_size * 0.5,
                          options->effect_size,
                          options->effect_size,
                          0.0, 2.0 * G_PI);
}

static void
gimp_warp_tool_create_graph (GimpWarpTool *wt)
{
  GeglNode        *coords, *render; /* Render nodes */
  GeglNode        *input, *output; /* Proxy nodes*/
  GeglNode        *graph; /* wraper to be returned */

  g_return_if_fail (wt->graph == NULL);
  /* render_node is not supposed to be recreated */

  graph = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (graph, "input");
  output = gegl_node_get_output_proxy (graph, "output");

  coords = gegl_node_new_child (graph,
                               "operation", "gegl:buffer-source",
                               "buffer",    wt->coords_buffer,
                               NULL);

  render = gegl_node_new_child (graph,
                                "operation", "gegl:map-relative",
                                NULL);

  gegl_node_connect_to (input, "output",
                        render, "input");

  gegl_node_connect_to (coords, "output",
                        render, "aux");

  gegl_node_connect_to (render, "output",
                        output, "input");

  wt->graph = graph;
  wt->render_node = render;
  wt->read_coords_buffer_node = coords;
}

static void
gimp_warp_tool_create_image_map (GimpWarpTool *wt,
                                 GimpDrawable *drawable)
{
  if (!wt->graph)
    gimp_warp_tool_create_graph (wt);

  wt->image_map = gimp_image_map_new (drawable,
                                      _("Warp transform"),
                                      wt->graph,
                                      NULL,
                                      NULL);

  g_object_set (wt->image_map, "gegl-caching", TRUE, NULL);

  g_signal_connect (wt->image_map, "flush",
                    G_CALLBACK (gimp_warp_tool_image_map_flush),
                    wt);
}

static void
gimp_warp_tool_image_map_update (GimpWarpTool *wt)
{
  GimpWarpOptions  *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);
  GeglRectangle     region;
  GeglRectangle     to_update;

  region.x = wt->cursor_x - options->effect_size * 0.5;
  region.y = wt->cursor_y - options->effect_size * 0.5;
  region.width = options->effect_size;
  region.height = options->effect_size;

  gegl_rectangle_bounding_box (&to_update, &region, &wt->last_region);

  gegl_rectangle_copy (&wt->last_region, &region);

  printf("update rect: (%d,%d), %dx%d\n", to_update.x, to_update.y, to_update.width, to_update.height);

  gimp_image_map_apply_region (wt->image_map, &to_update);
}

static void
gimp_warp_tool_image_map_flush (GimpImageMap *image_map,
                                GimpTool     *tool)
{
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush_now (gimp_image_get_projection (image));
  gimp_display_flush_now (tool->display);
}

static void
gimp_warp_tool_add_op (GimpWarpTool *wt)
{
  GimpWarpOptions *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);
  GeglNode *new_op, *last_op;

  g_return_if_fail (GEGL_IS_NODE (wt->render_node));

  new_op = gegl_node_new_child (wt->graph,
                                "operation", "gegl:warp",
                                "behavior", options->behavior,
                                "strength", options->effect_strength,
                                "size", options->effect_size,
                                "hardness", options->effect_hardness,
                                "stroke", wt->current_stroke,
                                NULL);

  last_op = gegl_node_get_producer (wt->render_node, "aux", NULL);

  gegl_node_disconnect (wt->render_node, "aux");

  gegl_node_connect_to (last_op, "output", new_op, "input");
  gegl_node_connect_to (new_op, "output", wt->render_node, "aux");
}

static void
gimp_warp_tool_undo (GimpWarpTool *wt)
{
  GeglNode      *to_delete;
  GeglNode      *previous;
  const gchar   *type;
  GeglPath      *stroke;
  gdouble        min_x;
  gdouble        max_x;
  gdouble        min_y;
  gdouble        max_y;
  gdouble        size;
  GeglRectangle  bbox;

  to_delete = gegl_node_get_producer (wt->render_node, "aux", NULL);
  type = gegl_node_get_operation(to_delete);

  if (strcmp (type, "gegl:warp"))
    return;

  previous = gegl_node_get_producer (to_delete, "input", NULL);

  gegl_node_disconnect (to_delete, "input");
  gegl_node_connect_to (previous, "output", wt->render_node, "aux");

  gegl_node_get (to_delete, "stroke", &stroke, NULL);
  gegl_node_get (to_delete, "size", &size, NULL);

  if (stroke)
  {
    gegl_path_get_bounds (stroke, &min_x, &max_x, &min_y, &max_y);
    bbox.x      = min_x - size * 0.5;
    bbox.y      = min_y - size * 0.5;
    bbox.width  = max_x - min_x + size;
    bbox.height = max_y - min_y + size;

    gimp_image_map_abort (wt->image_map);
    gimp_image_map_apply_region (wt->image_map, &bbox);
  }

  g_object_unref (stroke);
  g_object_unref (to_delete);
}
