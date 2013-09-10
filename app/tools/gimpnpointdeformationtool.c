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
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplayshell.h"
#include "display/gimpcanvasbufferpreview.h"

#include "gimptooloptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimpnpointdeformationtool.h"
#include "gimpnpointdeformationoptions.h"

#include "gimp-intl.h"

#include <npd/npd_common.h>

#define GIMP_NPD_DEBUG
#define GIMP_NPD_MAXIMUM_DEFORMATION_DELAY 100000 /* 100000 microseconds == 10 FPS */
#define GIMP_NPD_DRAW_INTERVAL                 50 /*     50 milliseconds == 20 FPS */

void            gimp_n_point_deformation_tool_start                   (GimpNPointDeformationTool *npd_tool,
                                                                       GimpDisplay               *display);
void            gimp_n_point_deformation_tool_halt                    (GimpNPointDeformationTool *npd_tool);
static void     gimp_n_point_deformation_tool_set_options             (GeglNode                  *npd_node,
                                                                       GimpNPointDeformationOptions *npd_options);
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
static void     gimp_n_point_deformation_tool_clear_selected_points_list
                                                                      (GimpNPointDeformationTool *npd_tool);
gboolean        gimp_n_point_deformation_tool_add_cp_to_selection     (GimpNPointDeformationTool *npd_tool,
                                                                       NPDControlPoint           *cp);
gboolean        gimp_n_point_deformation_tool_is_cp_in_area           (NPDControlPoint           *cp,
                                                                       gfloat                     x0,
                                                                       gfloat                     y0,
                                                                       gfloat                     x1,
                                                                       gfloat                     y1,
                                                                       gfloat                     offset_x,
                                                                       gfloat                     offset_y,
                                                                       gfloat                     cp_radius);
static void     gimp_n_point_deformation_tool_remove_cp_from_selection
                                                                      (GimpNPointDeformationTool *npd_tool,
                                                                       NPDControlPoint           *cp);
gpointer        gimp_n_point_deformation_tool_deform_thread_func      (gpointer                   data);
gboolean        gimp_n_point_deformation_tool_canvas_update_thread_func
                                                                      (GimpNPointDeformationTool *npd_tool);
static void     gimp_n_point_deformation_tool_perform_deformation     (GimpNPointDeformationTool *npd_tool);
static void     gimp_n_point_deformation_tool_halt_threads            (GimpNPointDeformationTool *npd_tool);
GimpCanvasItem *gimp_n_point_deformation_tool_add_preview             (GimpNPointDeformationTool *npd_tool);

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
                _("N-Point Deformation Tool: Rubber-like deformation of an image using points"),
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

  gimp_tool_control_set_tool_cursor          (tool->control,
                                              GIMP_TOOL_CURSOR_PERSPECTIVE);
  gimp_tool_control_set_preserve             (tool->control, FALSE);
  gimp_tool_control_set_wants_click          (tool->control, TRUE);
  gimp_tool_control_set_wants_all_key_events (tool->control, TRUE);
  gimp_tool_control_set_scroll_lock          (tool->control, TRUE);
  gimp_tool_control_set_handle_empty_image   (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask           (tool->control,
                                              GIMP_DIRTY_IMAGE           |
                                              GIMP_DIRTY_IMAGE_STRUCTURE |
                                              GIMP_DIRTY_DRAWABLE        |
                                              GIMP_DIRTY_SELECTION       |
                                              GIMP_DIRTY_ACTIVE_DRAWABLE);

  npd_tool->active    = FALSE;
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
      /* stop the tool only when it has been started */
      if (npd_tool->active)
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
  GeglNode      *graph, *node, *source, *sink;
  GimpImage     *image;
  GimpDrawable  *drawable;
  GeglBuffer    *source_buffer, *preview_buffer;
  NPDModel      *model;
  gint           offset_x, offset_y;
  GimpNPointDeformationOptions *npd_options;

  g_return_if_fail (GIMP_IS_N_POINT_DEFORMATION_TOOL (npd_tool));

  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

  tool->display = npd_tool->display = display;

  npd_tool->active = TRUE;

  image         = gimp_display_get_image (display);
  drawable      = gimp_image_get_active_drawable (image);
  source_buffer = gimp_drawable_get_buffer (drawable);

  preview_buffer  = gegl_buffer_new (gegl_buffer_get_extent (source_buffer),
                                     babl_format ("cairo-ARGB32"));

  graph    = gegl_node_new ();

  source   = gegl_node_new_child (graph,
                                  "operation", "gegl:buffer-source",
                                  "buffer", source_buffer,
                                  NULL);
  node     = gegl_node_new_child (graph,
                                  "operation", "gegl:npd",
                                  NULL);
  sink     = gegl_node_new_child (graph,
                                  "operation", "gegl:write-buffer",
                                  "buffer", preview_buffer,
                                  NULL);

  gegl_node_link_many (source, node, sink, NULL);

  /* initialize some options */
  npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);
  npd_options->mesh_visible = TRUE;
  gimp_n_point_deformation_options_set_pause_deformation (npd_options, FALSE);
  gimp_n_point_deformation_tool_set_options (node, npd_options);

  /* compute and get model */
  gegl_node_process (node);
  gegl_node_get (node, "model", &model, NULL);

  npd_tool->drawable               = drawable;
  npd_tool->graph                  = graph;
  npd_tool->model                  = model;
  npd_tool->node                   = node;
  npd_tool->sink                   = sink;
  npd_tool->preview_buffer         = preview_buffer;
  npd_tool->selected_cp            = NULL;
  npd_tool->hovering_cp            = NULL;
  npd_tool->selected_cps           = NULL;
  npd_tool->previous_cps_positions = NULL;
  npd_tool->rubber_band            = FALSE;

  /* get drawable's offset */
  gimp_item_get_offset (GIMP_ITEM (drawable),
                        &offset_x, &offset_y);
  npd_tool->offset_x = offset_x;
  npd_tool->offset_y = offset_y;
  gimp_npd_debug (("offset: %f %f\n", npd_tool->offset_x, npd_tool->offset_y));

  /* start draw tool */
  gimp_draw_tool_start (draw_tool, display);

  /* create and start a deformation thread */
  npd_tool->deform_thread =
          g_thread_new ("deform thread",
                        (GThreadFunc) gimp_n_point_deformation_tool_deform_thread_func,
                        npd_tool);

  /* create and start canvas update thread */
  npd_tool->draw_timeout_id =
          gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE,
                                        GIMP_NPD_DRAW_INTERVAL,
                                        (GSourceFunc) gimp_n_point_deformation_tool_canvas_update_thread_func,
                                        npd_tool,
                                        NULL);
}

void
gimp_n_point_deformation_tool_halt (GimpNPointDeformationTool *npd_tool)
{
  GimpTool     *tool      = GIMP_TOOL (npd_tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (npd_tool);

  gimp_n_point_deformation_tool_halt_threads (npd_tool);

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);  
  
  gimp_n_point_deformation_tool_clear_selected_points_list (npd_tool);
  
  if (npd_tool->model != NULL)
    npd_destroy_model (npd_tool->model);

  tool->display = npd_tool->display = NULL;
}

static void
gimp_n_point_deformation_tool_set_options (GeglNode                     *npd_node,
                                           GimpNPointDeformationOptions *npd_options)
{
  gegl_node_set (npd_node,
                 "square size",       (gint) npd_options->square_size,
                 "rigidity",          (gint) npd_options->rigidity,
                 "ASAP deformation",  npd_options->ASAP_deformation,
                 "MLS weights",       npd_options->MLS_weights,
                 "MLS weights alpha", npd_options->MLS_weights_alpha,
                 "mesh visible",      npd_options->mesh_visible,
                 NULL);
}

static void
gimp_n_point_deformation_tool_options_notify (GimpTool         *tool,
                                              GimpToolOptions  *options,
                                              const GParamSpec *pspec)
{
  GimpNPointDeformationTool    *npd_tool    = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  GimpNPointDeformationOptions *npd_options = GIMP_N_POINT_DEFORMATION_OPTIONS (options);
  GimpDrawTool                 *draw_tool   = GIMP_DRAW_TOOL (tool);

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (!npd_tool->active) return;

  gimp_draw_tool_pause (draw_tool);

  gimp_npd_debug (("npd options notify\n"));
  gimp_n_point_deformation_tool_set_options (npd_tool->node, npd_options);

  gimp_draw_tool_resume (draw_tool);
}

static gboolean
gimp_n_point_deformation_tool_key_press (GimpTool    *tool,
                                         GdkEventKey *kevent,
                                         GimpDisplay *display)
{
  GimpNPointDeformationTool    *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  GimpNPointDeformationOptions *npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);
  NPDModel                     *model = npd_tool->model;
  NPDControlPoint              *cp;
  GArray                       *cps = model->control_points;

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      /* if there is at least one control point, remove last added control point */
      if (cps->len > 0)
        {
          cp = &g_array_index (cps, NPDControlPoint, cps->len - 1);
          gimp_npd_debug (("removing last cp %p\n", cp));
          gimp_n_point_deformation_tool_remove_cp_from_selection (npd_tool, cp);
          npd_remove_control_point (model, cp);
        }
      break;

    case GDK_KEY_Delete:
      if (npd_tool->selected_cps != NULL)
        {
          /* if there is at least one selected control point, remove it */
          npd_remove_control_points (model, npd_tool->selected_cps);
          gimp_n_point_deformation_tool_clear_selected_points_list (npd_tool);
        }
      break;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      gimp_n_point_deformation_tool_halt_threads (npd_tool);

      npd_options->mesh_visible = FALSE;
      gimp_n_point_deformation_tool_set_options (npd_tool->node,
                                                 npd_options);
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      break;

    case GDK_KEY_Escape:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      break;

    case GDK_KEY_KP_Space:
    case GDK_KEY_space:
      gimp_n_point_deformation_options_toggle_pause_deformation (npd_options);
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
  GimpNPointDeformationTool    *npd_tool    = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  GimpNPointDeformationOptions *npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);
  GimpCursorModifier            modifier    = GIMP_CURSOR_MODIFIER_PLUS;

  if (!npd_tool->active)
    {
      modifier = GIMP_CURSOR_MODIFIER_NONE;
    }
  else
  if (gimp_n_point_deformation_options_is_deformation_paused (npd_options))
    {
      modifier = GIMP_CURSOR_MODIFIER_BAD;
    }
  else
  if (npd_tool->hovering_cp != NULL)
    {
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
    }

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_n_point_deformation_tool_clear_selected_points_list (GimpNPointDeformationTool *npd_tool)
{
  g_list_free      (npd_tool->selected_cps);
  g_list_free_full (npd_tool->previous_cps_positions, g_free);
  npd_tool->selected_cps           = NULL;
  npd_tool->previous_cps_positions = NULL;
}

gboolean
gimp_n_point_deformation_tool_add_cp_to_selection (GimpNPointDeformationTool *npd_tool,
                                                   NPDControlPoint           *cp)
{
  if (!g_list_find (npd_tool->selected_cps, cp))
    {
      /* control point isn't selected, so we can add it to the list
       * of selected control points */
      NPDPoint *cp_point_copy = g_new (NPDPoint, 1);
      *cp_point_copy = cp->point;
      npd_tool->selected_cps = g_list_append (npd_tool->selected_cps, cp);
      npd_tool->previous_cps_positions = g_list_append (npd_tool->previous_cps_positions,
                                                        cp_point_copy);
      return TRUE;
    }

  return FALSE;
}

static void
gimp_n_point_deformation_tool_remove_cp_from_selection (GimpNPointDeformationTool *npd_tool,
                                                        NPDControlPoint           *cp)
{
  npd_tool->selected_cps = g_list_remove (npd_tool->selected_cps, cp);
  npd_tool->previous_cps_positions = g_list_remove (npd_tool->previous_cps_positions,
                                                    cp);
}

static void
gimp_n_point_deformation_tool_button_press (GimpTool            *tool,
                                            const GimpCoords    *coords,
                                            guint32              time,
                                            GdkModifierType      state,
                                            GimpButtonPressType  press_type,
                                            GimpDisplay         *display)
{
  GimpNPointDeformationTool    *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  GimpNPointDeformationOptions *npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);
  NPDControlPoint              *cp;
  GList                       **selected_cps = &npd_tool->selected_cps;
  GList                       **previous_cps_positions = &npd_tool->previous_cps_positions;

  if (display != tool->display)
    {
      /* this is the first click on the drawable - just start the tool */
      gimp_n_point_deformation_tool_start (npd_tool, display);
      return;
    }

  /* this is at least second click on the drawable - do usual work */
  if (gimp_n_point_deformation_options_is_deformation_paused (npd_options)) return;

  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state, press_type, display);

  npd_tool->selected_cp = NULL;
  
  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    {
      cp = npd_tool->hovering_cp;
      if (cp != NULL)
        {
          /* there is a control point at cursor's position */
          npd_tool->selected_cp = cp;

          if (!g_list_find (*selected_cps, cp))
            {
              /* control point isn't selected, so we can add it to the list
               * of selected control points */

              if (!(state & GDK_SHIFT_MASK))
                {
                  /* <SHIFT> isn't pressed, so this isn't a multiselection -
                   * clear the list of selected control points */
                  gimp_n_point_deformation_tool_clear_selected_points_list (npd_tool);
                }

              gimp_n_point_deformation_tool_add_cp_to_selection (npd_tool, cp);

              gimp_npd_debug (("prev length: %d\n", g_list_length (*previous_cps_positions)));
            }
          else
          if (state & GDK_SHIFT_MASK)
            {
              /* control point is selected and <SHIFT> is pressed - remove
               * control point from selected points */
              gimp_n_point_deformation_tool_remove_cp_from_selection (npd_tool, cp);
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

          *selected_cps           = g_list_first (*selected_cps);
          *previous_cps_positions = g_list_first (*previous_cps_positions);
        }
      
      npd_tool->movement_start_x = coords->x;
      npd_tool->movement_start_y = coords->y;
    }
}

gboolean
gimp_n_point_deformation_tool_is_cp_in_area (NPDControlPoint  *cp,
                                             gfloat            x0,
                                             gfloat            y0,
                                             gfloat            x1,
                                             gfloat            y1,
                                             gfloat            offset_x,
                                             gfloat            offset_y,
                                             gfloat            cp_radius)
{
  NPDPoint p = cp->point;
  p.x += offset_x;
  p.y += offset_y;

  return p.x >= x0 - cp_radius && p.x <= x1 + cp_radius &&
         p.y >= y0 - cp_radius && p.y <= y1 + cp_radius;
}

static void
gimp_n_point_deformation_tool_button_release (GimpTool             *tool,
                                              const GimpCoords     *coords,
                                              guint32               time,
                                              GdkModifierType       state,
                                              GimpButtonReleaseType release_type,
                                              GimpDisplay           *display)
{
  GimpNPointDeformationTool    *npd_tool    = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  GimpNPointDeformationOptions *npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);
  NPDModel                     *model       = npd_tool->model;
  NPDPoint                      p;
  NPDControlPoint              *cp;
  GArray                       *cps         = model->control_points;
  gint                          i;

  if (gimp_n_point_deformation_options_is_deformation_paused (npd_options)) return;

  GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state, release_type, display);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (npd_tool));
  
  if (release_type == GIMP_BUTTON_RELEASE_CLICK)
    {
      if (npd_tool->hovering_cp == NULL)
        {
          gimp_npd_debug (("cp doesn't exist, adding\n"));
          p.x = coords->x - npd_tool->offset_x;
          p.y = coords->y - npd_tool->offset_y;
          npd_add_control_point (model, &p);
        }
    }
  else
  if (release_type == GIMP_BUTTON_RELEASE_NORMAL)
    {
      if (npd_tool->rubber_band)
        {
          gint x0 = MIN (npd_tool->movement_start_x, npd_tool->cursor_x);
          gint y0 = MIN (npd_tool->movement_start_y, npd_tool->cursor_y);
          gint x1 = MAX (npd_tool->movement_start_x, npd_tool->cursor_x);
          gint y1 = MAX (npd_tool->movement_start_y, npd_tool->cursor_y);

          if (!(state & GDK_SHIFT_MASK))
            {
              /* <SHIFT> isn't pressed, so we want a clear selection */
              gimp_n_point_deformation_tool_clear_selected_points_list (npd_tool);
            }

          for (i = 0; i < cps->len; i++)
            {
              cp = &g_array_index (cps, NPDControlPoint, i);
              if (gimp_n_point_deformation_tool_is_cp_in_area (cp,
                                                               x0, y0,
                                                               x1, y1,
                                                               npd_tool->offset_x, npd_tool->offset_y,
                                                               npd_tool->cp_scaled_radius))
                {
                  /* control point is situated in an area defined by rubber band */
                  gimp_n_point_deformation_tool_add_cp_to_selection (npd_tool, cp);
                  gimp_npd_debug(("%p selected\n", cp));
                }
            }
        }
    }
  else
  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
    }
  
  npd_tool->rubber_band = FALSE;

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

  gimp_draw_tool_pause (draw_tool);
  
  if (npd_tool->active)
    {
      NPDModel         *model = npd_tool->model;
      GimpDisplayShell *shell = gimp_display_get_shell (display);
      NPDPoint          p;

      npd_tool->cp_scaled_radius = model->control_point_radius / shell->scale_x;
      p.x = coords->x - npd_tool->offset_x;
      p.y = coords->y - npd_tool->offset_y;
      npd_tool->hovering_cp = npd_get_control_point_with_radius_at (model,
                                                                   &p,
                                                                    npd_tool->cp_scaled_radius);
    }

  npd_tool->cursor_x = coords->x;
  npd_tool->cursor_y = coords->y;

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_n_point_deformation_tool_draw (GimpDrawTool *draw_tool)
{
  GimpNPointDeformationTool *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (draw_tool);
  NPDModel                  *model = npd_tool->model;
  NPDControlPoint           *cp;
  NPDPoint                   p;
  GimpHandleType             handle_type;
  gint                       i, x0, y0, x1, y1;

  g_return_if_fail (model != NULL);
  
  x0 = MIN (npd_tool->movement_start_x, npd_tool->cursor_x);
  y0 = MIN (npd_tool->movement_start_y, npd_tool->cursor_y);
  x1 = MAX (npd_tool->movement_start_x, npd_tool->cursor_x);
  y1 = MAX (npd_tool->movement_start_y, npd_tool->cursor_y);
  
  for (i = 0; i < model->control_points->len; i++)
    {
      cp   = &g_array_index (model->control_points, NPDControlPoint, i);
      p    = cp->point;
      p.x += npd_tool->offset_x;
      p.y += npd_tool->offset_y;
      
      handle_type = GIMP_HANDLE_CIRCLE;

      /* check if cursor is hovering over a control point or
       * if a control point is situated in an area defined by rubber band */
      if (cp == npd_tool->hovering_cp ||
         (npd_tool->rubber_band &&
          gimp_n_point_deformation_tool_is_cp_in_area (cp,
                                                       x0, y0,
                                                       x1, y1,
                                                       npd_tool->offset_x, npd_tool->offset_y,
                                                       npd_tool->cp_scaled_radius)))
        {
          handle_type = GIMP_HANDLE_FILLED_CIRCLE;
        }

      gimp_draw_tool_add_handle (draw_tool,
                                 handle_type,
                                 p.x,
                                 p.y,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

      if (g_list_find (npd_tool->selected_cps, cp) != NULL)
        {
          gimp_draw_tool_add_handle (draw_tool,
                                     GIMP_HANDLE_SQUARE,
                                     p.x,
                                     p.y,
                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                     GIMP_HANDLE_ANCHOR_CENTER);
        }
    }

  if (npd_tool->rubber_band)
    {
      /* draw a rubber band */
      gimp_draw_tool_add_rectangle (draw_tool,
                                    FALSE,
                                    x0,
                                    y0,
                                    x1 - x0,
                                    y1 - y0);
    }

    gimp_n_point_deformation_tool_add_preview (npd_tool);
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

  gimp_draw_tool_pause (draw_tool);

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
  else
    {
      /* activate a rubber band selection */
      npd_tool->rubber_band = TRUE;
    }

  npd_tool->cursor_x = coords->x;
  npd_tool->cursor_y = coords->y;

  gimp_draw_tool_resume (draw_tool);
}

gboolean
gimp_n_point_deformation_tool_canvas_update_thread_func (GimpNPointDeformationTool *npd_tool)
{
  if (npd_tool->drawable == NULL) return FALSE;

  gimp_npd_debug (("canvas update thread\n"));
  gimp_draw_tool_pause(GIMP_DRAW_TOOL(npd_tool));
  gimp_draw_tool_resume(GIMP_DRAW_TOOL(npd_tool));
  gdk_window_process_updates (
          gtk_widget_get_window (gimp_display_get_shell (npd_tool->display)->canvas),
          FALSE);
  gimp_npd_debug (("canvas update thread stop\n"));
  
  return TRUE;
}

gpointer
gimp_n_point_deformation_tool_deform_thread_func (gpointer data)
{
  GimpNPointDeformationTool    *npd_tool    = data;
  GimpNPointDeformationOptions *npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);
  guint64                       start, duration;

  while (npd_tool->active) {
    start = g_get_monotonic_time ();

    /* perform the deformation only if the tool hasn't been paused */
    if (!gimp_n_point_deformation_options_is_deformation_paused (npd_options))
      {
        gimp_n_point_deformation_tool_perform_deformation (npd_tool);
      }

    duration = g_get_monotonic_time () - start;
    if (duration < GIMP_NPD_MAXIMUM_DEFORMATION_DELAY)
      {
        g_usleep (GIMP_NPD_MAXIMUM_DEFORMATION_DELAY - duration);
      }
  }

  gimp_npd_debug (("deform thread exit\n"));

  return NULL;
}

static void
gimp_n_point_deformation_tool_perform_deformation (GimpNPointDeformationTool *npd_tool)
{
  gimp_npd_debug (("gegl_node_invalidated\n"));
  gegl_node_invalidated (npd_tool->node, NULL, FALSE);

  gimp_npd_debug (("gegl_node_process\n"));
  gegl_node_process (npd_tool->sink);
}

static void
gimp_n_point_deformation_tool_halt_threads (GimpNPointDeformationTool *npd_tool)
{
  gimp_npd_debug (("waiting for deform thread to finish\n"));
  npd_tool->active = FALSE;

  /* wait for deformation thread to finish its work */
  g_thread_join (npd_tool->deform_thread);

  /* remove canvas update timeout */
  g_source_remove (npd_tool->draw_timeout_id);
  gimp_npd_debug (("finished\n"));
}

GimpCanvasItem *
gimp_n_point_deformation_tool_add_preview  (GimpNPointDeformationTool *npd_tool)
{
  GimpCanvasItem *item;
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (npd_tool);

  item = gimp_canvas_buffer_preview_new (gimp_display_get_shell (draw_tool->display),
                                         npd_tool->preview_buffer);

  gimp_draw_tool_add_preview (draw_tool, item);
  g_object_unref (item);

  return item;
}