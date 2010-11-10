/* GIMP - The GNU Image Manipulation Program
 *
 * gimpcagetool.c
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpdrawable-shadow.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"
#include "core/gimplayer.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"

#include "gegl/gimpcageconfig.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasprogress.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-items.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpcagetool.h"
#include "gimpcageoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void       gimp_cage_tool_finalize           (GObject               *object);
static void       gimp_cage_tool_start              (GimpCageTool          *ct,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_halt               (GimpCageTool          *ct);
static void       gimp_cage_tool_button_press       (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpButtonPressType    press_type,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_button_release     (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpButtonReleaseType  release_type,
                                                     GimpDisplay           *display);
static gboolean   gimp_cage_tool_key_press          (GimpTool              *tool,
                                                     GdkEventKey           *kevent,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_motion             (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_control            (GimpTool              *tool,
                                                     GimpToolAction         action,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_cursor_update      (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     GdkModifierType        state,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_oper_update        (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     GdkModifierType        state,
                                                     gboolean               proximity,
                                                     GimpDisplay           *display);

static void       gimp_cage_tool_draw               (GimpDrawTool          *draw_tool);

static gint       gimp_cage_tool_is_on_handle       (GimpCageConfig        *gcc,
                                                     GimpDrawTool          *draw_tool,
                                                     GimpDisplay           *display,
                                                     GimpCageMode           mode,
                                                     gdouble                x,
                                                     gdouble                y,
                                                     gint                   handle_size);

static void       gimp_cage_tool_switch_to_deform   (GimpCageTool          *ct);
static void       gimp_cage_tool_remove_last_handle (GimpCageTool          *ct);
static void       gimp_cage_tool_compute_coef       (GimpCageTool          *ct,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_create_image_map   (GimpCageTool          *ct,
                                                     GimpDrawable          *drawable);
static void       gimp_cage_tool_image_map_flush    (GimpImageMap          *image_map,
                                                     GimpTool              *tool);

static GeglNode * gimp_cage_tool_create_render_node (GimpCageTool          *ct);


G_DEFINE_TYPE (GimpCageTool, gimp_cage_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_cage_tool_parent_class


void
gimp_cage_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_CAGE_TOOL,
                GIMP_TYPE_CAGE_OPTIONS,
                gimp_cage_options_gui,
                0,
                "gimp-cage-tool",
                _("Cage Transform"),
                _("Cage Transform: Deform a selection with a cage"),
                N_("_Cage Transform"), "<shift>G",
                NULL, GIMP_HELP_TOOL_CAGE,
                GIMP_STOCK_TOOL_CAGE,
                data);
}

static void
gimp_cage_tool_class_init (GimpCageToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = gimp_cage_tool_finalize;

  tool_class->button_press   = gimp_cage_tool_button_press;
  tool_class->button_release = gimp_cage_tool_button_release;
  tool_class->key_press      = gimp_cage_tool_key_press;
  tool_class->motion         = gimp_cage_tool_motion;
  tool_class->control        = gimp_cage_tool_control;
  tool_class->cursor_update  = gimp_cage_tool_cursor_update;
  tool_class->oper_update    = gimp_cage_tool_oper_update;

  draw_tool_class->draw      = gimp_cage_tool_draw;
}

static void
gimp_cage_tool_init (GimpCageTool *self)
{
  GimpTool *tool = GIMP_TOOL (self);

  self->config          = g_object_new (GIMP_TYPE_CAGE_CONFIG, NULL);
  self->hovering_handle = -1;
  self->moving_handle   = -1;
  self->cage_complete   = FALSE;

  self->coef            = NULL;
  self->image_map       = NULL;

  gimp_tool_control_set_wants_click (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_PERSPECTIVE);
}

static void
gimp_cage_tool_finalize (GObject *object)
{
  GimpCageTool *ct = GIMP_CAGE_TOOL (object);

  gimp_cage_tool_halt (ct);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_cage_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_cage_tool_halt (GIMP_CAGE_TOOL (tool));
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_cage_tool_start (GimpCageTool *ct,
                      GimpDisplay  *display)
{
  GimpTool     *tool     = GIMP_TOOL (ct);
  GimpImage    *image    = gimp_display_get_image (display);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);
  gint          off_x;
  gint          off_y;

  gimp_tool_control_activate (tool->control);

  tool->display = display;

  if (ct->config)
    {
      g_object_unref (ct->config);
      ct->config = NULL;
    }

  if (ct->image_map)
    {
      gimp_image_map_abort (ct->image_map);
      g_object_unref (ct->image_map);
      ct->image_map = NULL;
    }

  ct->config          = g_object_new (GIMP_TYPE_CAGE_CONFIG, NULL);
  ct->hovering_handle = -1;
  ct->moving_handle   = -1;
  ct->cage_complete   = FALSE;

  /* Setting up cage offset to convert the cage point coords to
   * drawable coords
   */
  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  ct->config->offset_x = off_x;
  ct->config->offset_y = off_y;

  gimp_draw_tool_start (GIMP_DRAW_TOOL (ct), display);
}

static gboolean
gimp_cage_tool_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  GimpCageTool *ct = GIMP_CAGE_TOOL (tool);

  switch (kevent->keyval)
    {
    case GDK_BackSpace:
      if (! ct->cage_complete)
        gimp_cage_tool_remove_last_handle (ct);
      return TRUE;

    case GDK_Return:
    case GDK_KP_Enter:
    case GDK_ISO_Enter:
      if (ct->cage_complete)
        {
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_map_commit (ct->image_map);
          g_object_unref (ct->image_map);
          ct->image_map = NULL;

          gimp_tool_control_set_preserve (tool->control, FALSE);

          gimp_image_flush (gimp_display_get_image (display));

          gimp_cage_tool_halt (ct);
        }
      return TRUE;

    case GDK_Escape:
      gimp_cage_tool_halt (ct);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_cage_tool_motion (GimpTool         *tool,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       GimpDisplay      *display)
{
  GimpCageTool *ct = GIMP_CAGE_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  ct->cursor_x = coords->x;
  ct->cursor_y = coords->y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_cage_tool_oper_update (GimpTool         *tool,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            GimpDisplay      *display)
{
  GimpCageTool    *ct        = GIMP_CAGE_TOOL (tool);
  GimpDrawTool    *draw_tool = GIMP_DRAW_TOOL (tool);
  GimpCageOptions *options   = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  GimpCageConfig  *config    = ct->config;
  gint             handle    = -1;

  if (config)
    handle = gimp_cage_tool_is_on_handle (config,
                                          draw_tool,
                                          display,
                                          options->cage_mode,
                                          coords->x,
                                          coords->y,
                                          GIMP_TOOL_HANDLE_SIZE_CIRCLE);

  gimp_draw_tool_pause (draw_tool);

  ct->hovering_handle = handle;
  ct->cursor_x        = coords->x;
  ct->cursor_y        = coords->y;

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_cage_tool_button_press (GimpTool            *tool,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type,
                             GimpDisplay         *display)
{
  GimpCageTool *ct = GIMP_CAGE_TOOL (tool);

  if (display != tool->display)
    gimp_cage_tool_start (ct, display);

  ct->moving_handle = ct->hovering_handle;
}

void
gimp_cage_tool_button_release (GimpTool              *tool,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  GimpCageTool    *ct      = GIMP_CAGE_TOOL (tool);
  GimpCageOptions *options = GIMP_CAGE_TOOL_GET_OPTIONS (ct);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (ct));

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      /* operation canceled, do nothing */
    }
  else if (ct->moving_handle == 0 && release_type == GIMP_BUTTON_RELEASE_CLICK)
    {
      /* user clicked on the first handle, we close the cage and
       * switch to deform mode
       */
      if (ct->config->n_cage_vertices > 2 && ! ct->cage_complete)
        {
          GimpImage    *image    = gimp_display_get_image (display);
          GimpDrawable *drawable = gimp_image_get_active_drawable (image);

          ct->cage_complete = TRUE;
          gimp_cage_tool_switch_to_deform (ct);

          gimp_cage_config_reverse_cage_if_needed (ct->config);
          gimp_cage_tool_compute_coef (ct, display);

          gimp_cage_tool_create_image_map (ct, drawable);

          gimp_tool_push_status (tool, display, _("Press ENTER to commit the transform"));
        }
    }
  else if (ct->moving_handle == -1)
    {
      /* user released outside any handles, add one if the cage is not
       * complete yet
       */
      if (! ct->cage_complete)
        {
          gimp_cage_config_add_cage_point (ct->config,
                                           ct->cursor_x, ct->cursor_y);
        }
    }
  else
    {
      /* user moved a handle
       */
      gimp_cage_config_move_cage_point (ct->config,
                                        options->cage_mode,
                                        ct->moving_handle,
                                        ct->cursor_x,
                                        ct->cursor_y);

      if (ct->cage_complete)
        {
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

          gimp_image_map_apply (ct->image_map, &visible);
        }
    }

  ct->moving_handle = -1;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_cage_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpCageTool       *ct       = GIMP_CAGE_TOOL (tool);
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_PLUS;

  if (tool->display)
    {
      if (ct->hovering_handle != -1)
        {
          modifier = GIMP_CURSOR_MODIFIER_MOVE;
        }
      else
        {
          if (ct->cage_complete)
            modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
    }

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_cage_tool_draw (GimpDrawTool *draw_tool)
{
  GimpCageTool    *ct        = GIMP_CAGE_TOOL (draw_tool);
  GimpCageOptions *options   = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  GimpCageConfig  *config    = ct->config;
  GimpCanvasGroup *stroke_group;
  GimpVector2     *vertices;
  gint             n_vertices;
  gint             i;

  n_vertices = config->n_cage_vertices;

  if (n_vertices < 1)
    return;

  stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

  if (options->cage_mode == GIMP_CAGE_MODE_CAGE_CHANGE)
    vertices = config->cage_vertices;
  else
    vertices = config->cage_vertices_d;

  gimp_draw_tool_push_group (draw_tool, stroke_group);

  if (! ct->cage_complete && (ct->hovering_handle == -1 ||
                              (ct->hovering_handle == 0 && n_vertices > 2)))
    {
      gimp_draw_tool_add_line (draw_tool,
                               vertices[n_vertices - 1].x + ct->config->offset_x,
                               vertices[n_vertices - 1].y + ct->config->offset_y,
                               ct->cursor_x,
                               ct->cursor_y);
    }

  gimp_draw_tool_pop_group (draw_tool);

  for (i = 0; i < n_vertices; i++)
    {
      GimpHandleType handle;
      gdouble        x1, y1;

      if (i == ct->moving_handle)
        {
          x1 = ct->cursor_x;
          y1 = ct->cursor_y;
        }
      else
        {
          x1 = vertices[i].x;
          y1 = vertices[i].y;
        }

      if (i > 0 || ct->cage_complete)
        {
          gdouble x2, y2;
          gint    point2;

          if (i == 0)
            point2 = n_vertices - 1;
          else
            point2 = i - 1;

          if (point2 == ct->moving_handle)
            {
              x2 = ct->cursor_x;
              y2 = ct->cursor_y;
            }
          else
            {
              x2 = vertices[point2].x;
              y2 = vertices[point2].y;
            }

          gimp_draw_tool_push_group (draw_tool, stroke_group);

          gimp_draw_tool_add_line (draw_tool,
                                   x1 + ct->config->offset_x,
                                   y1 + ct->config->offset_y,
                                   x2 + ct->config->offset_x,
                                   y2 + ct->config->offset_y);

          gimp_draw_tool_pop_group (draw_tool);
        }

      if (i == ct->hovering_handle)
        handle = GIMP_HANDLE_FILLED_CIRCLE;
      else
        handle = GIMP_HANDLE_CIRCLE;

      gimp_draw_tool_add_handle (draw_tool,
                                 handle,
                                 x1 + ct->config->offset_x,
                                 y1 + ct->config->offset_y,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_HANDLE_ANCHOR_CENTER);
    }
}

static gint
gimp_cage_tool_is_on_handle (GimpCageConfig *gcc,
                             GimpDrawTool   *draw_tool,
                             GimpDisplay    *display,
                             GimpCageMode    mode,
                             gdouble         x,
                             gdouble         y,
                             gint            handle_size)
{
  gint    i;
  gdouble vert_x;
  gdouble vert_y;
  gdouble dist = G_MAXDOUBLE;

  g_return_val_if_fail (GIMP_IS_CAGE_CONFIG (gcc), -1);

  if (gcc->n_cage_vertices == 0)
    return -1;

  for (i = 0; i < gcc->n_cage_vertices; i++)
    {
      if (mode == GIMP_CAGE_MODE_CAGE_CHANGE)
        {
          vert_x = gcc->cage_vertices[i].x + gcc->offset_x;
          vert_y = gcc->cage_vertices[i].y + gcc->offset_y;
        }
      else
        {
          vert_x = gcc->cage_vertices_d[i].x + gcc->offset_x;
          vert_y = gcc->cage_vertices_d[i].y + gcc->offset_y;
        }

      dist = gimp_draw_tool_calc_distance_square (GIMP_DRAW_TOOL (draw_tool),
                                                  display,
                                                  x, y,
                                                  vert_x, vert_y);

      if (dist <= SQR (handle_size / 2))
        return i;
    }

  return -1;
}

static void
gimp_cage_tool_remove_last_handle (GimpCageTool *ct)
{
  GimpCageConfig *config = ct->config;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (ct));

  gimp_cage_config_remove_last_cage_point (config);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (ct));
}

static void
gimp_cage_tool_switch_to_deform (GimpCageTool *ct)
{
  GimpCageOptions *options = GIMP_CAGE_TOOL_GET_OPTIONS (ct);

  g_object_set (options, "cage-mode", GIMP_CAGE_MODE_DEFORM, NULL);
}

static void
gimp_cage_tool_compute_coef (GimpCageTool *ct,
                             GimpDisplay  *display)
{
  GimpDisplayShell *shell  = gimp_display_get_shell (display);
  GimpCageConfig   *config = ct->config;
  Babl             *format;
  GeglNode         *gegl;
  GeglNode         *input;
  GeglNode         *output;
  GeglProcessor    *processor;
  GimpCanvasItem   *p;
  GimpProgress     *progress;
  GeglBuffer       *buffer;
  gdouble           value;

  {
    gint x, y, w, h;

    gimp_display_shell_untransform_viewport (shell, &x, &y, &w, &h);

    p = gimp_canvas_progress_new (shell, GIMP_HANDLE_ANCHOR_CENTER,
                                  x + w / 2, y + h / 2);
    gimp_display_shell_add_item (shell, p);
    g_object_unref (p);
  }

  progress = gimp_progress_start (GIMP_PROGRESS (p),
                                  _("Coefficient computation"),
                                  FALSE);
  gimp_widget_flush_expose (shell->canvas);

  if (ct->coef)
    {
      gegl_buffer_destroy (ct->coef);
      ct->coef = NULL;
    }

  format = babl_format_n (babl_type ("float"),
                          config->n_cage_vertices * 2);


  gegl = gegl_node_new ();

  input = gegl_node_new_child (gegl,
                               "operation", "gimp:cage-coef-calc",
                               "config",    ct->config,
                               NULL);

  output = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-sink",
                                "buffer",    &buffer,
                                "format",    format,
                                NULL);

  gegl_node_connect_to (input, "output",
                        output, "input");

  processor = gegl_node_new_processor (output, NULL);

  while (gegl_processor_work (processor, &value))
    {
      gimp_progress_set_value (progress, value);
      gimp_widget_flush_expose (shell->canvas);
    }

  gimp_progress_end (progress);

  gegl_processor_destroy (processor);

  ct->coef = buffer;
  g_object_unref (gegl);

  gimp_display_shell_remove_item (shell, p);
}

static GeglNode *
gimp_cage_tool_create_render_node (GimpCageTool *ct)
{
  GimpCageOptions *options  = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  GeglNode        *coef, *cage, *render; /* Render nodes */
  GeglNode        *input, *output; /* Proxy nodes*/
  GeglNode        *node; /* wraper to be returned */

  node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  coef = gegl_node_new_child (node,
                              "operation", "gegl:buffer-source",
                              "buffer",    ct->coef,
                              NULL);

  cage = gegl_node_new_child (node,
                              "operation",        "gimp:cage-transform",
                              "config",           ct->config,
                              "fill_plain_color", options->fill_plain_color,
                              NULL);

  render = gegl_node_new_child (node,
                                "operation", "gegl:map-absolute",
                                NULL);

  gegl_node_connect_to (input, "output",
                        cage, "input");

  gegl_node_connect_to (coef, "output",
                        cage, "aux");

  gegl_node_connect_to (input, "output",
                        render, "input");

  gegl_node_connect_to (cage, "output",
                        render, "aux");

  gegl_node_connect_to (render, "output",
                        output, "input");

  return node;
}

static void
gimp_cage_tool_create_image_map (GimpCageTool *ct,
                                 GimpDrawable *drawable)
{
  GeglNode *node;

  node = gimp_cage_tool_create_render_node (ct);

  ct->image_map = gimp_image_map_new (drawable,
                                      _("Cage transform"),
                                      node,
                                      NULL,
                                      NULL);

  g_object_unref (node);

  g_signal_connect (ct->image_map, "flush",
                    G_CALLBACK (gimp_cage_tool_image_map_flush),
                    ct);
}

static void
gimp_cage_tool_image_map_flush (GimpImageMap *image_map,
                                GimpTool     *tool)
{
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush_now (gimp_image_get_projection (image));
  gimp_display_flush_now (tool->display);
}

static void
gimp_cage_tool_halt (GimpCageTool *ct)
{
  GimpTool     *tool      = GIMP_TOOL (ct);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (ct);

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);

  if (ct->config)
    {
      g_object_unref (ct->config);
      ct->config = NULL;
    }

  if (ct->coef)
    {
      gegl_buffer_destroy (ct->coef);
      ct->coef = NULL;
    }

  if (ct->image_map)
    {
      gimp_tool_control_set_preserve (tool->control, TRUE);

      gimp_image_map_abort (ct->image_map);
      g_object_unref (ct->image_map);
      ct->image_map = NULL;

      gimp_tool_control_set_preserve (tool->control, FALSE);

      gimp_image_flush (gimp_display_get_image (tool->display));
    }

  tool->display  = NULL;
}
