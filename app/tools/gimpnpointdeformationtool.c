/* GIMP - The GNU Image Manipulation Program
 *
 * gimpnpointdeformationtool.c
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
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
#include <gdk/gdkkeysyms.h>
#include <glib/gprintf.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "display/gimpdisplay.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-shadow.h"
#include "core/gimpdrawable-private.h"
#include "core/gimpimage.h"
#include "core/core-types.h"
#include "core/gimpimagemap.h"
#include "core/gimpprojection.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimptooloptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimpnpointdeformationtool.h"
#include "gimpnpointdeformationoptions.h"

#include "gimp-intl.h"

#include <npd/npd_common.h>

#define GIMP_NPD_DEBUG

void            gimp_n_point_deformation_tool_start                   (GimpNPointDeformationTool *npd_tool,
                                                                       GimpDisplay               *display);
void            gimp_n_point_deformation_tool_halt                    (GimpNPointDeformationTool *npd_tool);
static void     gimp_n_point_deformation_tool_options_notify          (GimpTool                  *tool,
                                                                       GimpToolOptions           *options,
                                                                       const GParamSpec          *pspec);
static void     gimp_n_point_deformation_tool_button_press            (GimpTool                  *tool,
                                                                       const GimpCoords          *coords,
                                                                       guint32                    time,
                                                                       GdkModifierType            state,
                                                                       GimpButtonPressType        press_type,
                                                                       GimpDisplay               *display);
static void     gimp_n_point_deformation_tool_button_release          (GimpTool                  *tool,
                                                                       const GimpCoords          *coords,
                                                                       guint32                    time,
                                                                       GdkModifierType            state,
                                                                       GimpButtonReleaseType      release_type,
                                                                       GimpDisplay               *display);
static gboolean gimp_n_point_deformation_tool_key_press               (GimpTool                  *tool,
                                                                       GdkEventKey               *kevent,
                                                                       GimpDisplay               *display);
static void     gimp_n_point_deformation_tool_modifier_key            (GimpTool                  *tool,
                                                                       GdkModifierType            key,
                                                                       gboolean                   press,
                                                                       GdkModifierType            state,
                                                                       GimpDisplay               *display);
static void     gimp_n_point_deformation_tool_cursor_update           (GimpTool                  *tool,
                                                                       const GimpCoords          *coords,
                                                                       GdkModifierType            state,
                                                                       GimpDisplay               *display);
static void     gimp_n_point_deformation_tool_draw                    (GimpDrawTool              *draw_tool);
static void     gimp_n_point_deformation_tool_control                 (GimpTool                  *tool,
                                                                       GimpToolAction             action,
                                                                       GimpDisplay               *display);
static void     gimp_n_point_deformation_tool_oper_update             (GimpTool                  *tool,
                                                                       const GimpCoords          *coords,
                                                                       GdkModifierType            state,
                                                                       gboolean                   proximity,
                                                                       GimpDisplay               *display);
static void     gimp_n_point_deformation_tool_motion                  (GimpTool                  *tool,
                                                                       const GimpCoords          *coords,
                                                                       guint32                    time,
                                                                       GdkModifierType            state,
                                                                       GimpDisplay               *display);
#ifdef GIMP_NPD_DEBUG
#define gimp_npd_debug(x) g_printf x
#else
#define gimp_npd_debug(x)
#endif

G_DEFINE_TYPE (GimpNPointDeformationTool, gimp_n_point_deformation_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_n_point_deformation_tool_parent_class


void
gimp_n_point_deformation_tool_register (GimpToolRegisterCallback  callback,
                                        gpointer                  data)
{
  (* callback) (GIMP_TYPE_N_POINT_DEFORMATION_TOOL,
                GIMP_TYPE_N_POINT_DEFORMATION_OPTIONS,
                gimp_n_point_deformation_options_gui,
                0,
                "gimp-n-point-deformation-tool",
                _("N-Point Deformation"),
                _("N-Point Deformation Tool: Deform image using points"),
                N_("_N-Point Deformation"), "<shift>N",
                NULL, GIMP_HELP_TOOL_N_POINT_DEFORMATION,
                GIMP_STOCK_TOOL_N_POINT_DEFORMATION,
                data);
}

static void
gimp_n_point_deformation_tool_class_init (GimpNPointDeformationToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->options_notify = gimp_n_point_deformation_tool_options_notify;
  tool_class->button_press   = gimp_n_point_deformation_tool_button_press;
  tool_class->button_release = gimp_n_point_deformation_tool_button_release;
  tool_class->key_press      = gimp_n_point_deformation_tool_key_press;
  tool_class->modifier_key   = gimp_n_point_deformation_tool_modifier_key;
  tool_class->control        = gimp_n_point_deformation_tool_control;
  tool_class->motion         = gimp_n_point_deformation_tool_motion;
  tool_class->oper_update    = gimp_n_point_deformation_tool_oper_update;
  tool_class->cursor_update  = gimp_n_point_deformation_tool_cursor_update;
  
  draw_tool_class->draw      = gimp_n_point_deformation_tool_draw;
}

static void
gimp_n_point_deformation_tool_init (GimpNPointDeformationTool *npd_tool)
{
  GimpTool      *tool       = GIMP_TOOL (npd_tool);

  gimp_tool_control_set_tool_cursor  (tool->control,
                                      GIMP_TOOL_CURSOR_PERSPECTIVE);
  gimp_tool_control_set_preserve     (tool->control, FALSE);
  gimp_tool_control_set_wants_click  (tool->control, TRUE);
  gimp_tool_control_set_wants_double_click  (tool->control, TRUE);
  gimp_tool_control_set_scroll_lock  (tool->control, TRUE);
  gimp_tool_control_set_handle_empty_image (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask   (tool->control,
                                      GIMP_DIRTY_IMAGE           |
                                      GIMP_DIRTY_IMAGE_STRUCTURE |
                                      GIMP_DIRTY_DRAWABLE        |
                                      GIMP_DIRTY_SELECTION       |
                                      GIMP_DIRTY_ACTIVE_DRAWABLE);
  
  npd_tool->active = FALSE;
  npd_tool->selected_cp = NULL;
  npd_tool->hovering_cp = NULL;
  npd_tool->selected_cps = NULL;
  npd_tool->previous_cps_positions = NULL;
}

static void
gimp_n_point_deformation_tool_control (GimpTool       *tool,
                                       GimpToolAction  action,
                                       GimpDisplay    *display)
{
  GimpNPointDeformationTool *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  
  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_n_point_deformation_tool_halt (npd_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

void
gimp_n_point_deformation_tool_start (GimpNPointDeformationTool *npd_tool,
                                     GimpDisplay               *display)
{
  GimpTool      *tool       = GIMP_TOOL (npd_tool);
  GimpDrawTool  *draw_tool  = GIMP_DRAW_TOOL (npd_tool);
  GeglNode      *graph;
  GeglNode      *node;
  GeglNode      *source;
  GeglNode      *sink;
  GimpImage     *image;
  GimpDrawable  *drawable;
  GeglBuffer    *buf;
  GeglBuffer    *shadow;
  NPDModel      *model;
  
  g_return_if_fail (GIMP_IS_N_POINT_DEFORMATION_TOOL (npd_tool));
  
  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
  
  tool->display = display;
  gimp_draw_tool_start (draw_tool, display);
  
  npd_tool->active = TRUE;  

  image    = gimp_display_get_image (display);
  drawable = gimp_image_get_active_drawable (image);
  buf      = drawable->private->buffer;
  
  shadow   = gegl_buffer_new (gegl_buffer_get_extent (buf), gegl_buffer_get_format(buf));
  
  graph    = gegl_node_new ();

  source   = gegl_node_new_child (graph, "operation", "gegl:buffer-source",
                                       "buffer", buf,
                                       NULL);
  node     = gegl_node_new_child (graph, "operation", "gegl:npd", NULL);

  sink     = gegl_node_new_child (graph, "operation", "gegl:write-buffer",
                                      "buffer", shadow,
                                      NULL);

  gegl_node_link_many (source, node, sink, NULL);
  
  gegl_node_process (sink);
  
  gegl_node_get (node, "model", &model, NULL);
  
  npd_tool->buf = buf;
  npd_tool->drawable = drawable;
  npd_tool->graph = graph;
  npd_tool->model = model;
  npd_tool->node = node;
  npd_tool->source = source;
  npd_tool->shadow = shadow;
  npd_tool->sink = sink;
  npd_tool->selected_cp = NULL;
}

void
gimp_n_point_deformation_tool_halt (GimpNPointDeformationTool *npd_tool)
{
  GimpTool     *tool      = GIMP_TOOL (npd_tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (npd_tool);
  
  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);
  
  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);  
  
  tool->display = NULL;
}

static void
gimp_n_point_deformation_tool_options_notify (GimpTool         *tool,
                                              GimpToolOptions  *options,
                                              const GParamSpec *pspec)
{
}

static gboolean
gimp_n_point_deformation_tool_key_press (GimpTool    *tool,
                                         GdkEventKey *kevent,
                                         GimpDisplay *display)
{
  GimpNPointDeformationTool *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  NPDModel                  *model = npd_tool->model;
  GList                    **selected_cps = &npd_tool->selected_cps;
  GList                     *last_selected_cp;
  NPDControlPoint           *cp;
  GArray                    *cps = model->control_points;
  gint                       i;
  
  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      /* if there is at least one control point,
       * remove last added control point */
      if (cps->len > 0)
        {
          cp = &g_array_index (cps, NPDControlPoint, cps->len - 1);
          gimp_npd_debug (("removing cp %p\n", cp));
          npd_remove_control_point (model, cp);
          *selected_cps = g_list_remove (*selected_cps, cp);
        }
      break;
    case GDK_KEY_Delete:
      
      break;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
    case GDK_KEY_Escape:
      break;

    default:
      return FALSE;
  }
  
  return TRUE;
}

static void
gimp_n_point_deformation_tool_modifier_key (GimpTool        *tool,
                                            GdkModifierType  key,
                                            gboolean         press,
                                            GdkModifierType  state,
                                            GimpDisplay     *display)
{
}

static void
gimp_n_point_deformation_tool_cursor_update (GimpTool         *tool,
                                             const GimpCoords *coords,
                                             GdkModifierType   state,
                                             GimpDisplay      *display)
{
}

#define gimp_n_point_deformation_tool_clear_selected_points_list()             \
g_list_free      (*selected_cps);                                              \
g_list_free_full (*previous_cps_positions, g_free);                            \
*selected_cps           = NULL;                                                \
*previous_cps_positions = NULL

static void
gimp_n_point_deformation_tool_button_press (GimpTool            *tool,
                                            const GimpCoords    *coords,
                                            guint32              time,
                                            GdkModifierType      state,
                                            GimpButtonPressType  press_type,
                                            GimpDisplay         *display)
{
  GimpNPointDeformationTool *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  NPDModel                  *model;
  NPDPoint                   p;
  NPDControlPoint           *cp;
  GList                    **selected_cps = &npd_tool->selected_cps;
  GList                    **previous_cps_positions = &npd_tool->previous_cps_positions;
  
  if (display != tool->display)
    {
      gimp_n_point_deformation_tool_start (npd_tool, display);
    }
  
  model = npd_tool->model;
  
  gimp_tool_control_activate (tool->control);
  
  npd_tool->selected_cp = NULL;
  
  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    {
      p.x = coords->x; p.y = coords->y;
      cp = npd_get_control_point_at (model, &p);
      
      if (cp == NULL)
        {
          /* there isn't a control point at cursor's position - clear the list
           * of selected control points */
          gimp_n_point_deformation_tool_clear_selected_points_list ();
        }
      else
        {
          /* there is a control point at cursor's position */
          npd_tool->selected_cp = cp;

          if (!g_list_find (*selected_cps, cp))
            {
              /* control point isn't selected, so we add it to the list
               * of selected control points */
              NPDPoint *cp_point_copy = g_new (NPDPoint, 1);

              if (!(state & GDK_SHIFT_MASK))
                {
                  /* <SHIFT> isn't pressed, so this isn't a multiselection -
                   * clear the list of selected control points */
                  gimp_n_point_deformation_tool_clear_selected_points_list ();
                }

              *cp_point_copy = cp->point;

              *selected_cps           = g_list_append (*selected_cps,
                                                        cp);
              *previous_cps_positions = g_list_append (*previous_cps_positions,
                                                       cp_point_copy);

              gimp_npd_debug (("prev length: %d\n", g_list_length (*previous_cps_positions)));
            }

          /* update previous positions of control points */
          while (*selected_cps != NULL)
            {
              NPDPoint *p = (*previous_cps_positions)->data;
              cp          = (*selected_cps)->data;
              npd_set_point_coordinates (p, &cp->point);

              if (g_list_next (*selected_cps) == NULL) break;
              *selected_cps           = g_list_next (*selected_cps);
              *previous_cps_positions = g_list_next (*previous_cps_positions);
            }

          *selected_cps          = g_list_first (*selected_cps);
          *previous_cps_positions = g_list_first (*previous_cps_positions);
        }
      
      npd_tool->movement_start_x = coords->x;
      npd_tool->movement_start_y = coords->y;
    }
}

static void
gimp_n_point_deformation_tool_button_release (GimpTool             *tool,
                                              const GimpCoords     *coords,
                                              guint32               time,
                                              GdkModifierType       state,
                                              GimpButtonReleaseType release_type,
                                              GimpDisplay           *display)
{
  GimpNPointDeformationTool *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  NPDModel                  *model = npd_tool->model;
  NPDPoint                   p;
  NPDControlPoint           *cp;
  GeglBuffer                *buffer;
  
  gimp_tool_control_halt (tool->control);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (npd_tool));
  
  if (release_type == GIMP_BUTTON_RELEASE_CLICK)
    {
      /* TODO - solve bug */
      p.x = coords->x; p.y = coords->y;
      cp = npd_get_control_point_at (model, &p);
      if (cp == NULL)
        {
          gimp_npd_debug (("cp doesn't exist, adding\n"));
          npd_add_control_point (model, &p);
        }
    }
  else
  if (release_type == GIMP_BUTTON_RELEASE_NORMAL)
    {
    }
  else
  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
    }

  buffer = npd_tool->drawable->private->buffer;
  
  gegl_node_invalidated (npd_tool->node, NULL, FALSE);
  
  gegl_node_process (npd_tool->sink);
  gegl_buffer_copy (npd_tool->shadow, NULL,
                    npd_tool->buf, NULL);
  gimp_drawable_update (npd_tool->drawable,
                        0,
                        0,
                        gegl_buffer_get_width (buffer),
                        gegl_buffer_get_height (buffer));
  gimp_image_flush (gimp_display_get_image (display));
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (npd_tool));
}

static void
gimp_n_point_deformation_tool_oper_update (GimpTool         *tool,
                                           const GimpCoords *coords,
                                           GdkModifierType   state,
                                           gboolean          proximity,
                                           GimpDisplay      *display)
{
  GimpNPointDeformationTool *npd_tool  = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (tool);
  
  if (npd_tool->active)
    {
      NPDModel *model = npd_tool->model;
      NPDPoint p;
      p.x = coords->x; p.y = coords->y;
      npd_tool->hovering_cp = npd_get_control_point_at (model, &p);
    }
  
  gimp_draw_tool_pause (draw_tool);

  npd_tool->cursor_x = coords->x;
  npd_tool->cursor_y = coords->y;

  gimp_draw_tool_resume (draw_tool);  
}

static void
gimp_n_point_deformation_tool_draw (GimpDrawTool *draw_tool)
{
  GimpNPointDeformationTool *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (draw_tool);
  NPDModel                  *model = npd_tool->model;
  gint                       i;
  NPDControlPoint           *cp;
  GimpHandleType             handle_type;

  g_return_if_fail (model != NULL);
  
  for (i = 0; i < model->control_points->len; i++)
    {
      cp = &g_array_index (model->control_points, NPDControlPoint, i);
      
      if (cp == npd_tool->hovering_cp)
        handle_type = GIMP_HANDLE_FILLED_CIRCLE;
      else
        handle_type = GIMP_HANDLE_CIRCLE;

      gimp_draw_tool_add_handle (draw_tool,
                                 handle_type,
                                 cp->point.x,
                                 cp->point.y,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

      if (g_list_find (npd_tool->selected_cps, cp) != NULL)
        {
          gimp_draw_tool_add_handle (draw_tool,
                                     GIMP_HANDLE_SQUARE,
                                     cp->point.x,
                                     cp->point.y,
                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                     GIMP_HANDLE_ANCHOR_CENTER);
        }
    }
}

static void
gimp_n_point_deformation_tool_motion (GimpTool         *tool,
                                      const GimpCoords *coords,
                                      guint32           time,
                                      GdkModifierType   state,
                                      GimpDisplay      *display)
{
  GimpNPointDeformationTool *npd_tool     = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  GimpDrawTool              *draw_tool    = GIMP_DRAW_TOOL (tool);
  NPDControlPoint           *selected_cp  = npd_tool->selected_cp;
  GList                     *selected_cps = npd_tool->selected_cps;
  GList                     *previous_cps_positions = npd_tool->previous_cps_positions;
  gdouble                    shift_x      = coords->x - npd_tool->movement_start_x;
  gdouble                    shift_y      = coords->y - npd_tool->movement_start_y;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (selected_cp != NULL)
    {
      while (selected_cps != NULL)
        {
          NPDControlPoint *cp = selected_cps->data;
          NPDPoint *p = &cp->point;
          NPDPoint *prev = previous_cps_positions->data;

          p->x = prev->x + shift_x;
          p->y = prev->y + shift_y;
          
          selected_cps = g_list_next (selected_cps);
          previous_cps_positions = g_list_next (previous_cps_positions);
        }
    }
  
  npd_tool->cursor_x = coords->x;
  npd_tool->cursor_y = coords->y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}
