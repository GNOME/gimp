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

static void       gimp_warp_tool_options_notify     (GimpTool              *tool,
                                                     GimpToolOptions       *options,
                                                     const GParamSpec      *pspec);
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

static void       gimp_warp_tool_draw               (GimpDrawTool          *draw_tool);

static void       gimp_warp_tool_create_image_map   (GimpWarpTool          *wt,
                                                     GimpDrawable          *drawable);
static void       gimp_warp_tool_image_map_flush    (GimpImageMap          *image_map,
                                                     GimpTool              *tool);
static void       gimp_warp_tool_image_map_update   (GimpWarpTool          *wt);
static void       gimp_warp_tool_act_on_coords      (GimpWarpTool          *wt,
                                                     gint                   x,
                                                     gint                   y);

G_DEFINE_TYPE (GimpWarpTool, gimp_warp_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_warp_tool_parent_class


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

  tool_class->options_notify = gimp_warp_tool_options_notify;
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

      if (wt->render_node)
        {
          g_object_unref (wt->render_node);
          wt->render_node = NULL;
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

  if (wt->render_node)
    {
      g_object_unref (wt->render_node);
      wt->render_node = NULL;
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

  wt->coords_buffer = gegl_buffer_new (&bbox, format);

  gimp_warp_tool_create_image_map (wt, drawable);
  gimp_draw_tool_start (GIMP_DRAW_TOOL (wt), display);
}

static void
gimp_warp_tool_options_notify (GimpTool         *tool,
                               GimpToolOptions  *options,
                               const GParamSpec *pspec)
{
  GimpWarpTool *wt = GIMP_WARP_TOOL (tool);

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! tool->display)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (strcmp (pspec->name, "foo") == 0)
    {
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
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
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
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

  gimp_draw_tool_pause (draw_tool);

  gimp_draw_tool_resume (draw_tool);
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
  GimpDrawTool    *draw_tool = GIMP_DRAW_TOOL (tool);

  if (display != tool->display)
    gimp_warp_tool_start (wt, display);

  gimp_warp_tool_act_on_coords (wt, coords->x, coords->y);
  gimp_warp_tool_image_map_update (wt);

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

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
    }
  else
    {
    }

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

static void
gimp_warp_tool_draw (GimpDrawTool *draw_tool)
{
  GimpWarpTool    *wt        = GIMP_WARP_TOOL (draw_tool);
}

static void
gimp_warp_tool_create_render_node (GimpWarpTool *wt)
{
  GeglNode        *coords, *render; /* Render nodes */
  GeglNode        *input, *output; /* Proxy nodes*/
  GeglNode        *node; /* wraper to be returned */

  g_return_if_fail (wt->render_node == NULL);
  /* render_node is not supposed to be recreated */

  node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  coords = gegl_node_new_child (node,
                               "operation", "gegl:buffer-source",
                               "buffer",    wt->coords_buffer,
                               NULL);

  render = gegl_node_new_child (node,
                                "operation", "gegl:map-relative",
                                NULL);

  gegl_node_connect_to (input, "output",
                        render, "input");

  gegl_node_connect_to (coords, "output",
                        render, "aux");

  gegl_node_connect_to (render, "output",
                        output, "input");

  wt->render_node = node;
  wt->coords_node = coords;
}

static void
gimp_warp_tool_create_image_map (GimpWarpTool *wt,
                                 GimpDrawable *drawable)
{
  if (!wt->render_node)
    gimp_warp_tool_create_render_node (wt);

  wt->image_map = gimp_image_map_new (drawable,
                                      _("Warp transform"),
                                      wt->render_node,
                                      NULL,
                                      NULL);

  g_signal_connect (wt->image_map, "flush",
                    G_CALLBACK (gimp_warp_tool_image_map_flush),
                    wt);
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
gimp_warp_tool_image_map_update (GimpWarpTool *wt)
{
  GimpTool         *tool  = GIMP_TOOL (wt);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);
  GimpItem         *item  = GIMP_ITEM (tool->drawable);
  gint              x, y;
  gint              w, h;
  gint              off_x, off_y;
  GeglRectangle     visible;

  gimp_display_shell_untransform_viewport (shell, &x, &y, &w, &h);

  gimp_item_get_offset (item, &off_x, &off_y);

  gimp_rectangle_intersect (x, y, w, h,
                            off_x,
                            off_y,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item),
                            &visible.x,
                            &visible.y,
                            &visible.width,
                            &visible.height);

  visible.x -= off_x;
  visible.y -= off_y;

  gimp_image_map_apply (wt->image_map, &visible);
}

static void
gimp_warp_tool_act_on_coords (GimpWarpTool *wt,
                              gint x,
                              gint y)
{
  GeglBufferIterator  *it;
  Babl                *format;
  GeglRectangle        area = {x - 30,
                               y - 30,
                               60,
                               60};

  format = babl_format_n (babl_type ("float"), 2);
  it = gegl_buffer_iterator_new (wt->coords_buffer, &area, format, GEGL_BUFFER_READWRITE);

  while (gegl_buffer_iterator_next (it))
    {
      /* iterate inside the roi */
      gint    n_pixels = it->length;
      gfloat *coords   = it->data[0];

      x = it->roi->x; /* initial x         */
      y = it->roi->y; /* and y coordinates */

      while (n_pixels--)
        {
          coords[0] += 2;
          coords[1] += 2;

          coords += 2;

          /* update x and y coordinates */
          x++;
          if (x >= (it->roi->x + it->roi->width))
            {
              x = it->roi->x;
              y++;
            }
        }
    }
}
