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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gegl-plugin.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <npd/npd_common.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h" /* playground */

#include "gegl/gimp-gegl-utils.h"
#include "gegl/gimp-gegl-apply-operation.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpcanvasbufferpreview.h"

#include "gimpnpointdeformationtool.h"
#include "gimpnpointdeformationoptions.h"
#include "gimptoolcontrol.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


//#define GIMP_NPD_DEBUG
#define GIMP_NPD_MAXIMUM_DEFORMATION_DELAY 100000 /* 100000 microseconds == 10 FPS */
#define GIMP_NPD_DRAW_INTERVAL                 50 /*     50 milliseconds == 20 FPS */


static void     gimp_n_point_deformation_tool_start                   (GimpNPointDeformationTool *npd_tool,
                                                                       GimpDisplay               *display);
static void     gimp_n_point_deformation_tool_halt                    (GimpNPointDeformationTool *npd_tool);
static void     gimp_n_point_deformation_tool_commit                  (GimpNPointDeformationTool *npd_tool);
static void     gimp_n_point_deformation_tool_set_options             (GimpNPointDeformationTool *npd_tool,
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
static gboolean gimp_n_point_deformation_tool_add_cp_to_selection     (GimpNPointDeformationTool *npd_tool,
                                                                       NPDControlPoint           *cp);
static gboolean gimp_n_point_deformation_tool_is_cp_in_area           (NPDControlPoint           *cp,
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
static gpointer gimp_n_point_deformation_tool_deform_thread_func      (gpointer                   data);
static gboolean gimp_n_point_deformation_tool_canvas_update_timeout   (GimpNPointDeformationTool *npd_tool);
static void     gimp_n_point_deformation_tool_perform_deformation     (GimpNPointDeformationTool *npd_tool);
static void     gimp_n_point_deformation_tool_halt_threads            (GimpNPointDeformationTool *npd_tool);
static void     gimp_n_point_deformation_tool_apply_deformation       (GimpNPointDeformationTool *npd_tool);

#ifdef GIMP_NPD_DEBUG
#define gimp_npd_debug(x) g_printf x
#else
#define gimp_npd_debug(x)
#endif

G_DEFINE_TYPE (GimpNPointDeformationTool, gimp_n_point_deformation_tool,
               GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_n_point_deformation_tool_parent_class


void
gimp_n_point_deformation_tool_register (GimpToolRegisterCallback  callback,
                                        gpointer                  data)
{
  /* we should not know that "data" is a Gimp*, but what the heck this
   * is experimental playground stuff
   */
  if (GIMP_GUI_CONFIG (GIMP (data)->config)->playground_npd_tool)
    (* callback) (GIMP_TYPE_N_POINT_DEFORMATION_TOOL,
                  GIMP_TYPE_N_POINT_DEFORMATION_OPTIONS,
                  gimp_n_point_deformation_options_gui,
                  0,
                  "gimp-n-point-deformation-tool",
                  _("N-Point Deformation"),
                  _("N-Point Deformation Tool: Rubber-like deformation of "
                    "image using points"),
                  N_("_N-Point Deformation"), "<shift>N",
                  NULL, GIMP_HELP_TOOL_N_POINT_DEFORMATION,
                  GIMP_ICON_TOOL_N_POINT_DEFORMATION,
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
  GimpTool *tool = GIMP_TOOL (npd_tool);

  gimp_tool_control_set_precision            (tool->control,
                                              GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor          (tool->control,
                                              GIMP_TOOL_CURSOR_PERSPECTIVE);
  gimp_tool_control_set_preserve             (tool->control, FALSE);
  gimp_tool_control_set_wants_click          (tool->control, TRUE);
  gimp_tool_control_set_wants_all_key_events (tool->control, TRUE);
  gimp_tool_control_set_handle_empty_image   (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask           (tool->control,
                                              GIMP_DIRTY_IMAGE           |
                                              GIMP_DIRTY_IMAGE_STRUCTURE |
                                              GIMP_DIRTY_DRAWABLE        |
                                              GIMP_DIRTY_SELECTION       |
                                              GIMP_DIRTY_ACTIVE_DRAWABLE);
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
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_n_point_deformation_tool_halt (npd_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_n_point_deformation_tool_commit (npd_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_n_point_deformation_tool_start (GimpNPointDeformationTool *npd_tool,
                                     GimpDisplay               *display)
{
  GimpTool                     *tool = GIMP_TOOL (npd_tool);
  GimpNPointDeformationOptions *npd_options;
  GimpImage                    *image;
  GeglBuffer                   *source_buffer;
  GeglBuffer                   *preview_buffer;
  NPDModel                     *model;

  npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

  image = gimp_display_get_image (display);

  tool->display  = display;
  tool->drawable = gimp_image_get_active_drawable (image);

  npd_tool->active = TRUE;

  /* create GEGL graph */
  source_buffer  = gimp_drawable_get_buffer (tool->drawable);

  preview_buffer = gegl_buffer_new (gegl_buffer_get_extent (source_buffer),
                                    babl_format ("cairo-ARGB32"));

  npd_tool->graph    = gegl_node_new ();

  npd_tool->source   = gegl_node_new_child (npd_tool->graph,
                                            "operation", "gegl:buffer-source",
                                            "buffer",    source_buffer,
                                            NULL);
  npd_tool->npd_node = gegl_node_new_child (npd_tool->graph,
                                            "operation", "gegl:npd",
                                            NULL);
  npd_tool->sink     = gegl_node_new_child (npd_tool->graph,
                                            "operation", "gegl:write-buffer",
                                            "buffer",    preview_buffer,
                                            NULL);

  gegl_node_link_many (npd_tool->source,
                       npd_tool->npd_node,
                       npd_tool->sink,
                       NULL);

  /* initialize some options */
  g_object_set (G_OBJECT (npd_options), "mesh-visible", TRUE, NULL);
  gimp_n_point_deformation_options_set_sensitivity (npd_options, TRUE);

  /* compute and get model */
  gegl_node_process (npd_tool->npd_node);
  gegl_node_get (npd_tool->npd_node, "model", &model, NULL);

  npd_tool->model          = model;
  npd_tool->preview_buffer = preview_buffer;
  npd_tool->selected_cp    = NULL;
  npd_tool->hovering_cp    = NULL;
  npd_tool->selected_cps   = NULL;
  npd_tool->rubber_band    = FALSE;
  npd_tool->lattice_points = g_new (GimpVector2,
                                    5 * model->hidden_model->num_of_bones);

  gimp_item_get_offset (GIMP_ITEM (tool->drawable),
                        &npd_tool->offset_x, &npd_tool->offset_y);
  gimp_npd_debug (("offset: %f %f\n", npd_tool->offset_x, npd_tool->offset_y));

  gimp_draw_tool_start (GIMP_DRAW_TOOL (npd_tool), display);

  gimp_n_point_deformation_tool_perform_deformation (npd_tool);

  /* hide original image */
  gimp_item_set_visible (GIMP_ITEM (tool->drawable), FALSE, FALSE);
  gimp_image_flush (image);

  /* create and start a deformation thread */
  npd_tool->deform_thread =
    g_thread_new ("deform thread",
                  (GThreadFunc) gimp_n_point_deformation_tool_deform_thread_func,
                  npd_tool);

  /* create and start canvas update timeout */
  npd_tool->draw_timeout_id =
    gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE,
                                  GIMP_NPD_DRAW_INTERVAL,
                                  (GSourceFunc) gimp_n_point_deformation_tool_canvas_update_timeout,
                                  npd_tool,
                                  NULL);
}

static void
gimp_n_point_deformation_tool_commit (GimpNPointDeformationTool *npd_tool)
{
  GimpTool *tool = GIMP_TOOL (npd_tool);

  if (npd_tool->active)
    {
      gimp_n_point_deformation_tool_halt_threads (npd_tool);

      gimp_tool_control_push_preserve (tool->control, TRUE);
      gimp_n_point_deformation_tool_apply_deformation (npd_tool);
      gimp_tool_control_pop_preserve (tool->control);

      /* show original/deformed image */
      gimp_item_set_visible (GIMP_ITEM (tool->drawable), TRUE, FALSE);
      gimp_image_flush (gimp_display_get_image (tool->display));

      npd_tool->active = FALSE;
    }
}

static void
gimp_n_point_deformation_tool_halt (GimpNPointDeformationTool *npd_tool)
{
  GimpTool                     *tool      = GIMP_TOOL (npd_tool);
  GimpDrawTool                 *draw_tool = GIMP_DRAW_TOOL (npd_tool);
  GimpNPointDeformationOptions *npd_options;

  npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  if (npd_tool->active)
    {
      gimp_n_point_deformation_tool_halt_threads (npd_tool);

      /* show original/deformed image */
      gimp_item_set_visible (GIMP_ITEM (tool->drawable), TRUE, FALSE);
      gimp_image_flush (gimp_display_get_image (tool->display));

      /* disable some options */
      gimp_n_point_deformation_options_set_sensitivity (npd_options, FALSE);

      npd_tool->active = FALSE;
    }

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  gimp_n_point_deformation_tool_clear_selected_points_list (npd_tool);

  g_clear_object (&npd_tool->graph);
  npd_tool->source   = NULL;
  npd_tool->npd_node = NULL;
  npd_tool->sink     = NULL;

  g_clear_object (&npd_tool->preview_buffer);
  g_clear_pointer (&npd_tool->lattice_points, g_free);

  tool->display  = NULL;
  tool->drawable = NULL;
}

static void
gimp_n_point_deformation_tool_set_options (GimpNPointDeformationTool    *npd_tool,
                                           GimpNPointDeformationOptions *npd_options)
{
  gegl_node_set (npd_tool->npd_node,
                 "square-size",       (gint) npd_options->square_size,
                 "rigidity",          (gint) npd_options->rigidity,
                 "asap-deformation",  npd_options->asap_deformation,
                 "mls-weights",       npd_options->mls_weights,
                 "mls-weights-alpha", npd_options->mls_weights_alpha,
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

  if (! npd_tool->active)
    return;

  gimp_draw_tool_pause (draw_tool);

  gimp_npd_debug (("npd options notify\n"));
  gimp_n_point_deformation_tool_set_options (npd_tool, npd_options);

  gimp_draw_tool_resume (draw_tool);
}

static gboolean
gimp_n_point_deformation_tool_key_press (GimpTool    *tool,
                                         GdkEventKey *kevent,
                                         GimpDisplay *display)
{
  GimpNPointDeformationTool *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (tool);

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      /* if there is at least one control point, remove last added
       *  control point
       */
      if (npd_tool->model                 &&
          npd_tool->model->control_points &&
          npd_tool->model->control_points->len > 0)
        {
          GArray          *cps = npd_tool->model->control_points;
          NPDControlPoint *cp  = &g_array_index (cps, NPDControlPoint,
                                                 cps->len - 1);

          gimp_npd_debug (("removing last cp %p\n", cp));
          gimp_n_point_deformation_tool_remove_cp_from_selection (npd_tool, cp);
          npd_remove_control_point (npd_tool->model, cp);
        }
      break;

    case GDK_KEY_Delete:
      if (npd_tool->model &&
          npd_tool->selected_cps)
        {
          /* if there is at least one selected control point, remove it */
          npd_remove_control_points (npd_tool->model, npd_tool->selected_cps);
          gimp_n_point_deformation_tool_clear_selected_points_list (npd_tool);
        }
      break;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
      break;

    case GDK_KEY_Escape:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
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
  GimpNPointDeformationTool *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  GimpCursorModifier         modifier = GIMP_CURSOR_MODIFIER_PLUS;

  if (! npd_tool->active)
    {
      modifier = GIMP_CURSOR_MODIFIER_NONE;
    }
  else if (npd_tool->hovering_cp)
    {
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
    }

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_n_point_deformation_tool_clear_selected_points_list (GimpNPointDeformationTool *npd_tool)
{
  if (npd_tool->selected_cps)
    {
      g_list_free (npd_tool->selected_cps);
      npd_tool->selected_cps = NULL;
    }
}

static gboolean
gimp_n_point_deformation_tool_add_cp_to_selection (GimpNPointDeformationTool *npd_tool,
                                                   NPDControlPoint           *cp)
{
  if (! g_list_find (npd_tool->selected_cps, cp))
    {
      npd_tool->selected_cps = g_list_append (npd_tool->selected_cps, cp);

      return TRUE;
    }

  return FALSE;
}

static void
gimp_n_point_deformation_tool_remove_cp_from_selection (GimpNPointDeformationTool *npd_tool,
                                                        NPDControlPoint           *cp)
{
  npd_tool->selected_cps = g_list_remove (npd_tool->selected_cps, cp);
}

static void
gimp_n_point_deformation_tool_button_press (GimpTool            *tool,
                                            const GimpCoords    *coords,
                                            guint32              time,
                                            GdkModifierType      state,
                                            GimpButtonPressType  press_type,
                                            GimpDisplay         *display)
{
  GimpNPointDeformationTool *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (tool);

  if (display != tool->display)
    {
      /* this is the first click on the drawable - just start the tool */
      gimp_n_point_deformation_tool_start (npd_tool, display);
    }

  npd_tool->selected_cp = NULL;

  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    {
      NPDControlPoint *cp = npd_tool->hovering_cp;

      if (cp)
        {
          /* there is a control point at cursor's position */
          npd_tool->selected_cp = cp;

          if (! g_list_find (npd_tool->selected_cps, cp))
            {
              /* control point isn't selected, so we can add it to the
               * list of selected control points
               */

              if (! (state & gimp_get_extend_selection_mask ()))
                {
                  /* <SHIFT> isn't pressed, so this isn't a
                   * multiselection - clear the list of selected
                   * control points
                   */
                  gimp_n_point_deformation_tool_clear_selected_points_list (npd_tool);
                }

              gimp_n_point_deformation_tool_add_cp_to_selection (npd_tool, cp);
            }
          else if (state & gimp_get_extend_selection_mask ())
            {
              /* control point is selected and <SHIFT> is pressed -
               * remove control point from selected points
               */
              gimp_n_point_deformation_tool_remove_cp_from_selection (npd_tool,
                                                                      cp);
            }
        }

      npd_tool->start_x = coords->x;
      npd_tool->start_y = coords->y;

      npd_tool->last_x = coords->x;
      npd_tool->last_y = coords->y;
    }

  gimp_tool_control_activate (tool->control);
}

static gboolean
gimp_n_point_deformation_tool_is_cp_in_area (NPDControlPoint *cp,
                                             gfloat           x0,
                                             gfloat           y0,
                                             gfloat           x1,
                                             gfloat           y1,
                                             gfloat           offset_x,
                                             gfloat           offset_y,
                                             gfloat           cp_radius)
{
  NPDPoint p = cp->point;

  p.x += offset_x;
  p.y += offset_y;

  return p.x >= x0 - cp_radius && p.x <= x1 + cp_radius &&
         p.y >= y0 - cp_radius && p.y <= y1 + cp_radius;
}

static void
gimp_n_point_deformation_tool_button_release (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonReleaseType  release_type,
                                              GimpDisplay           *display)
{
  GimpNPointDeformationTool *npd_tool = GIMP_N_POINT_DEFORMATION_TOOL (tool);

  gimp_tool_control_halt (tool->control);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (npd_tool));

  if (release_type == GIMP_BUTTON_RELEASE_CLICK)
    {
      if (! npd_tool->hovering_cp)
        {
          NPDPoint p;

          gimp_npd_debug (("cp doesn't exist, adding\n"));
          p.x = coords->x - npd_tool->offset_x;
          p.y = coords->y - npd_tool->offset_y;

          npd_add_control_point (npd_tool->model, &p);
        }
    }
  else if (release_type == GIMP_BUTTON_RELEASE_NORMAL)
    {
      if (npd_tool->rubber_band)
        {
          GArray *cps = npd_tool->model->control_points;
          gint    x0  = MIN (npd_tool->start_x, npd_tool->cursor_x);
          gint    y0  = MIN (npd_tool->start_y, npd_tool->cursor_y);
          gint    x1  = MAX (npd_tool->start_x, npd_tool->cursor_x);
          gint    y1  = MAX (npd_tool->start_y, npd_tool->cursor_y);
          gint    i;

          if (! (state & gimp_get_extend_selection_mask ()))
            {
              /* <SHIFT> isn't pressed, so we want a clear selection */
              gimp_n_point_deformation_tool_clear_selected_points_list (npd_tool);
            }

          for (i = 0; i < cps->len; i++)
            {
              NPDControlPoint *cp = &g_array_index (cps, NPDControlPoint, i);

              if (gimp_n_point_deformation_tool_is_cp_in_area (cp,
                                                               x0, y0,
                                                               x1, y1,
                                                               npd_tool->offset_x,
                                                               npd_tool->offset_y,
                                                               npd_tool->cp_scaled_radius))
                {
                  /* control point is situated in an area defined by
                   * rubber band
                  */
                  gimp_n_point_deformation_tool_add_cp_to_selection (npd_tool,
                                                                     cp);
                  gimp_npd_debug (("%p selected\n", cp));
                }
            }
        }
    }
  else if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      gimp_npd_debug (("gimp_button_release_cancel\n"));
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

      npd_tool->hovering_cp =
        npd_get_control_point_with_radius_at (model,
                                              &p,
                                              npd_tool->cp_scaled_radius);
    }

  npd_tool->cursor_x = coords->x;
  npd_tool->cursor_y = coords->y;

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_n_point_deformation_tool_prepare_lattice (GimpNPointDeformationTool *npd_tool)
{
  NPDHiddenModel *hm     = npd_tool->model->hidden_model;
  GimpVector2    *points = npd_tool->lattice_points;
  gint            i, j;

  for (i = 0; i < hm->num_of_bones; i++)
    {
      NPDBone *bone = &hm->current_bones[i];

      for (j = 0; j < 4; j++)
        gimp_vector2_set (&points[5 * i + j], bone->points[j].x, bone->points[j].y);
      gimp_vector2_set (&points[5 * i + j], bone->points[0].x, bone->points[0].y);
    }
}

static void
gimp_n_point_deformation_tool_draw_lattice (GimpNPointDeformationTool *npd_tool)
{
  GimpVector2 *points    = npd_tool->lattice_points;
  gint         n_squares = npd_tool->model->hidden_model->num_of_bones;
  gint         i;

  for (i = 0; i < n_squares; i++)
    gimp_draw_tool_add_lines (GIMP_DRAW_TOOL (npd_tool),
                              &points[5 * i], 5, NULL, FALSE);
}

static void
gimp_n_point_deformation_tool_draw (GimpDrawTool *draw_tool)
{
  GimpNPointDeformationTool    *npd_tool;
  GimpNPointDeformationOptions *npd_options;
  NPDModel                     *model;
  gint                          x0, y0, x1, y1;
  gint                          i;

  npd_tool    = GIMP_N_POINT_DEFORMATION_TOOL (draw_tool);
  npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  model = npd_tool->model;

  g_return_if_fail (model != NULL);

  /* draw lattice */
  if (npd_options->mesh_visible)
    gimp_n_point_deformation_tool_draw_lattice (npd_tool);

  x0 = MIN (npd_tool->start_x, npd_tool->cursor_x);
  y0 = MIN (npd_tool->start_y, npd_tool->cursor_y);
  x1 = MAX (npd_tool->start_x, npd_tool->cursor_x);
  y1 = MAX (npd_tool->start_y, npd_tool->cursor_y);

  for (i = 0; i < model->control_points->len; i++)
    {
      NPDControlPoint *cp = &g_array_index (model->control_points,
                                            NPDControlPoint, i);
      NPDPoint         p  = cp->point;
      GimpHandleType   handle_type;

      p.x += npd_tool->offset_x;
      p.y += npd_tool->offset_y;

      handle_type = GIMP_HANDLE_CIRCLE;

      /* check if cursor is hovering over a control point or if a
       * control point is situated in an area defined by rubber band
       */
      if (cp == npd_tool->hovering_cp ||
          (npd_tool->rubber_band &&
           gimp_n_point_deformation_tool_is_cp_in_area (cp,
                                                        x0, y0,
                                                        x1, y1,
                                                        npd_tool->offset_x,
                                                        npd_tool->offset_y,
                                                        npd_tool->cp_scaled_radius)))
        {
          handle_type = GIMP_HANDLE_FILLED_CIRCLE;
        }

      gimp_draw_tool_add_handle (draw_tool,
                                 handle_type,
                                 p.x, p.y,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

      if (g_list_find (npd_tool->selected_cps, cp))
        {
          gimp_draw_tool_add_handle (draw_tool,
                                     GIMP_HANDLE_SQUARE,
                                     p.x, p.y,
                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                     GIMP_HANDLE_ANCHOR_CENTER);
        }
    }

  if (npd_tool->rubber_band)
    {
      /* draw a rubber band */
      gimp_draw_tool_add_rectangle (draw_tool, FALSE,
                                    x0, y0, x1 - x0, y1 - y0);
    }

  if (npd_tool->preview_buffer)
    {
      GimpCanvasItem *item;

      item = gimp_canvas_buffer_preview_new (gimp_display_get_shell (draw_tool->display),
                                             npd_tool->preview_buffer);

      gimp_draw_tool_add_preview (draw_tool, item);
      g_object_unref (item);
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
gimp_n_point_deformation_tool_motion (GimpTool         *tool,
                                      const GimpCoords *coords,
                                      guint32           time,
                                      GdkModifierType   state,
                                      GimpDisplay      *display)
{
  GimpNPointDeformationTool *npd_tool  = GIMP_N_POINT_DEFORMATION_TOOL (tool);
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (tool);

  gimp_draw_tool_pause (draw_tool);

  if (npd_tool->selected_cp)
    {
      GList   *list;
      gdouble  shift_x = coords->x - npd_tool->last_x;
      gdouble  shift_y = coords->y - npd_tool->last_y;

      for (list = npd_tool->selected_cps; list; list = g_list_next (list))
        {
          NPDControlPoint *cp = list->data;

          cp->point.x += shift_x;
          cp->point.y += shift_y;
        }
    }
  else
    {
      /* activate a rubber band selection */
      npd_tool->rubber_band = TRUE;
    }

  npd_tool->cursor_x = coords->x;
  npd_tool->cursor_y = coords->y;

  npd_tool->last_x = coords->x;
  npd_tool->last_y = coords->y;

  gimp_draw_tool_resume (draw_tool);
}

static gboolean
gimp_n_point_deformation_tool_canvas_update_timeout (GimpNPointDeformationTool *npd_tool)
{
  if (! GIMP_TOOL (npd_tool)->drawable)
    return FALSE;

  gimp_npd_debug (("canvas update thread\n"));

  gimp_draw_tool_pause (GIMP_DRAW_TOOL(npd_tool));
  gimp_draw_tool_resume (GIMP_DRAW_TOOL(npd_tool));

  gimp_npd_debug (("canvas update thread stop\n"));

  return TRUE;
}

static gpointer
gimp_n_point_deformation_tool_deform_thread_func (gpointer data)
{
  GimpNPointDeformationTool    *npd_tool = data;
  GimpNPointDeformationOptions *npd_options;
  guint64                       start, duration;

  npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  npd_tool->deformation_active = TRUE;

  while (npd_tool->deformation_active)
    {
      start = g_get_monotonic_time ();

      gimp_n_point_deformation_tool_perform_deformation (npd_tool);

      if (npd_options->mesh_visible)
        gimp_n_point_deformation_tool_prepare_lattice (npd_tool);

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
  GObject *operation;

  g_object_get (npd_tool->npd_node,
                "gegl-operation", &operation,
                NULL);
  gimp_npd_debug (("gegl_operation_invalidate\n"));
  gegl_operation_invalidate (GEGL_OPERATION (operation), NULL, FALSE);
  g_object_unref (operation);

  gimp_npd_debug (("gegl_node_process\n"));
  gegl_node_process (npd_tool->sink);
}

static void
gimp_n_point_deformation_tool_halt_threads (GimpNPointDeformationTool *npd_tool)
{
  if (! npd_tool->deformation_active)
    return;

  gimp_npd_debug (("waiting for deform thread to finish\n"));
  npd_tool->deformation_active = FALSE;

  /* wait for deformation thread to finish its work */
  if (npd_tool->deform_thread)
    {
      g_thread_join (npd_tool->deform_thread);
      npd_tool->deform_thread = NULL;
    }

  /* remove canvas update timeout */
  if (npd_tool->draw_timeout_id)
    {
      g_source_remove (npd_tool->draw_timeout_id);
      npd_tool->draw_timeout_id = 0;
    }

  gimp_npd_debug (("finished\n"));
}

static void
gimp_n_point_deformation_tool_apply_deformation (GimpNPointDeformationTool *npd_tool)
{
  GimpTool                     *tool = GIMP_TOOL (npd_tool);
  GimpNPointDeformationOptions *npd_options;
  GeglBuffer                   *buffer;
  GimpImage                    *image;
  gint                          width, height, prev;

  npd_options = GIMP_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  image  = gimp_display_get_image (tool->display);
  buffer = gimp_drawable_get_buffer (tool->drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  prev = npd_options->rigidity;
  npd_options->rigidity = 0;
  gimp_n_point_deformation_tool_set_options (npd_tool, npd_options);
  npd_options->rigidity = prev;

  gimp_drawable_push_undo (tool->drawable, _("N-Point Deformation"), NULL,
                           0, 0, width, height);


  gimp_gegl_apply_operation (NULL, NULL, _("N-Point Deformation"),
                             npd_tool->npd_node,
                             gimp_drawable_get_buffer (tool->drawable),
                             NULL, FALSE);

  gimp_drawable_update (tool->drawable,
                        0, 0, width, height);

  gimp_projection_flush (gimp_image_get_projection (image));
}

#undef gimp_npd_debug
#ifdef GIMP_NPD_DEBUG
#undef GIMP_NPD_DEBUG
#endif
