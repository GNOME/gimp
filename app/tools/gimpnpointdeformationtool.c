/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmanpointdeformationtool.c
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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaguiconfig.h" /* playground */

#include "gegl/ligma-gegl-utils.h"
#include "gegl/ligma-gegl-apply-operation.h"

#include "core/ligma.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmaprogress.h"
#include "core/ligmaprojection.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmacanvasbufferpreview.h"

#include "ligmanpointdeformationtool.h"
#include "ligmanpointdeformationoptions.h"
#include "ligmatoolcontrol.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


//#define LIGMA_NPD_DEBUG
#define LIGMA_NPD_MAXIMUM_DEFORMATION_DELAY 100000 /* 100000 microseconds == 10 FPS */
#define LIGMA_NPD_DRAW_INTERVAL                 50 /*     50 milliseconds == 20 FPS */


static void     ligma_n_point_deformation_tool_start                   (LigmaNPointDeformationTool *npd_tool,
                                                                       LigmaDisplay               *display);
static void     ligma_n_point_deformation_tool_halt                    (LigmaNPointDeformationTool *npd_tool);
static void     ligma_n_point_deformation_tool_commit                  (LigmaNPointDeformationTool *npd_tool);
static void     ligma_n_point_deformation_tool_set_options             (LigmaNPointDeformationTool *npd_tool,
                                                                       LigmaNPointDeformationOptions *npd_options);
static void     ligma_n_point_deformation_tool_options_notify          (LigmaTool                  *tool,
                                                                       LigmaToolOptions           *options,
                                                                       const GParamSpec          *pspec);
static void     ligma_n_point_deformation_tool_button_press            (LigmaTool                  *tool,
                                                                       const LigmaCoords          *coords,
                                                                       guint32                    time,
                                                                       GdkModifierType            state,
                                                                       LigmaButtonPressType        press_type,
                                                                       LigmaDisplay               *display);
static void     ligma_n_point_deformation_tool_button_release          (LigmaTool                  *tool,
                                                                       const LigmaCoords          *coords,
                                                                       guint32                    time,
                                                                       GdkModifierType            state,
                                                                       LigmaButtonReleaseType      release_type,
                                                                       LigmaDisplay               *display);
static gboolean ligma_n_point_deformation_tool_key_press               (LigmaTool                  *tool,
                                                                       GdkEventKey               *kevent,
                                                                       LigmaDisplay               *display);
static void     ligma_n_point_deformation_tool_modifier_key            (LigmaTool                  *tool,
                                                                       GdkModifierType            key,
                                                                       gboolean                   press,
                                                                       GdkModifierType            state,
                                                                       LigmaDisplay               *display);
static void     ligma_n_point_deformation_tool_cursor_update           (LigmaTool                  *tool,
                                                                       const LigmaCoords          *coords,
                                                                       GdkModifierType            state,
                                                                       LigmaDisplay               *display);
static void     ligma_n_point_deformation_tool_draw                    (LigmaDrawTool              *draw_tool);
static void     ligma_n_point_deformation_tool_control                 (LigmaTool                  *tool,
                                                                       LigmaToolAction             action,
                                                                       LigmaDisplay               *display);
static void     ligma_n_point_deformation_tool_oper_update             (LigmaTool                  *tool,
                                                                       const LigmaCoords          *coords,
                                                                       GdkModifierType            state,
                                                                       gboolean                   proximity,
                                                                       LigmaDisplay               *display);
static void     ligma_n_point_deformation_tool_motion                  (LigmaTool                  *tool,
                                                                       const LigmaCoords          *coords,
                                                                       guint32                    time,
                                                                       GdkModifierType            state,
                                                                       LigmaDisplay               *display);
static void     ligma_n_point_deformation_tool_clear_selected_points_list
                                                                      (LigmaNPointDeformationTool *npd_tool);
static gboolean ligma_n_point_deformation_tool_add_cp_to_selection     (LigmaNPointDeformationTool *npd_tool,
                                                                       NPDControlPoint           *cp);
static gboolean ligma_n_point_deformation_tool_is_cp_in_area           (NPDControlPoint           *cp,
                                                                       gfloat                     x0,
                                                                       gfloat                     y0,
                                                                       gfloat                     x1,
                                                                       gfloat                     y1,
                                                                       gfloat                     offset_x,
                                                                       gfloat                     offset_y,
                                                                       gfloat                     cp_radius);
static void     ligma_n_point_deformation_tool_remove_cp_from_selection
                                                                      (LigmaNPointDeformationTool *npd_tool,
                                                                       NPDControlPoint           *cp);
static gpointer ligma_n_point_deformation_tool_deform_thread_func      (gpointer                   data);
static gboolean ligma_n_point_deformation_tool_canvas_update_timeout   (LigmaNPointDeformationTool *npd_tool);
static void     ligma_n_point_deformation_tool_perform_deformation     (LigmaNPointDeformationTool *npd_tool);
static void     ligma_n_point_deformation_tool_halt_threads            (LigmaNPointDeformationTool *npd_tool);
static void     ligma_n_point_deformation_tool_apply_deformation       (LigmaNPointDeformationTool *npd_tool);

#ifdef LIGMA_NPD_DEBUG
#define ligma_npd_debug(x) g_printf x
#else
#define ligma_npd_debug(x)
#endif

G_DEFINE_TYPE (LigmaNPointDeformationTool, ligma_n_point_deformation_tool,
               LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_n_point_deformation_tool_parent_class


void
ligma_n_point_deformation_tool_register (LigmaToolRegisterCallback  callback,
                                        gpointer                  data)
{
  /* we should not know that "data" is a Ligma*, but what the heck this
   * is experimental playground stuff
   */
  if (LIGMA_GUI_CONFIG (LIGMA (data)->config)->playground_npd_tool)
    (* callback) (LIGMA_TYPE_N_POINT_DEFORMATION_TOOL,
                  LIGMA_TYPE_N_POINT_DEFORMATION_OPTIONS,
                  ligma_n_point_deformation_options_gui,
                  0,
                  "ligma-n-point-deformation-tool",
                  _("N-Point Deformation"),
                  _("N-Point Deformation Tool: Rubber-like deformation of "
                    "image using points"),
                  N_("_N-Point Deformation"), "<shift>N",
                  NULL, LIGMA_HELP_TOOL_N_POINT_DEFORMATION,
                  LIGMA_ICON_TOOL_N_POINT_DEFORMATION,
                  data);
}

static void
ligma_n_point_deformation_tool_class_init (LigmaNPointDeformationToolClass *klass)
{
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  tool_class->options_notify = ligma_n_point_deformation_tool_options_notify;
  tool_class->button_press   = ligma_n_point_deformation_tool_button_press;
  tool_class->button_release = ligma_n_point_deformation_tool_button_release;
  tool_class->key_press      = ligma_n_point_deformation_tool_key_press;
  tool_class->modifier_key   = ligma_n_point_deformation_tool_modifier_key;
  tool_class->control        = ligma_n_point_deformation_tool_control;
  tool_class->motion         = ligma_n_point_deformation_tool_motion;
  tool_class->oper_update    = ligma_n_point_deformation_tool_oper_update;
  tool_class->cursor_update  = ligma_n_point_deformation_tool_cursor_update;

  draw_tool_class->draw      = ligma_n_point_deformation_tool_draw;
}

static void
ligma_n_point_deformation_tool_init (LigmaNPointDeformationTool *npd_tool)
{
  LigmaTool *tool = LIGMA_TOOL (npd_tool);

  ligma_tool_control_set_precision            (tool->control,
                                              LIGMA_CURSOR_PRECISION_SUBPIXEL);
  ligma_tool_control_set_tool_cursor          (tool->control,
                                              LIGMA_TOOL_CURSOR_PERSPECTIVE);
  ligma_tool_control_set_preserve             (tool->control, FALSE);
  ligma_tool_control_set_wants_click          (tool->control, TRUE);
  ligma_tool_control_set_wants_all_key_events (tool->control, TRUE);
  ligma_tool_control_set_handle_empty_image   (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask           (tool->control,
                                              LIGMA_DIRTY_IMAGE           |
                                              LIGMA_DIRTY_IMAGE_STRUCTURE |
                                              LIGMA_DIRTY_DRAWABLE        |
                                              LIGMA_DIRTY_SELECTION       |
                                              LIGMA_DIRTY_ACTIVE_DRAWABLE);
}

static void
ligma_n_point_deformation_tool_control (LigmaTool       *tool,
                                       LigmaToolAction  action,
                                       LigmaDisplay    *display)
{
  LigmaNPointDeformationTool *npd_tool = LIGMA_N_POINT_DEFORMATION_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_n_point_deformation_tool_halt (npd_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_n_point_deformation_tool_commit (npd_tool);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_n_point_deformation_tool_start (LigmaNPointDeformationTool *npd_tool,
                                     LigmaDisplay               *display)
{
  LigmaTool                     *tool = LIGMA_TOOL (npd_tool);
  LigmaNPointDeformationOptions *npd_options;
  LigmaImage                    *image;
  GeglBuffer                   *source_buffer;
  GeglBuffer                   *preview_buffer;
  NPDModel                     *model;

  npd_options = LIGMA_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);

  image = ligma_display_get_image (display);

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = ligma_image_get_selected_drawables (image);

  npd_tool->active = TRUE;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  /* create GEGL graph */
  source_buffer  = ligma_drawable_get_buffer (tool->drawables->data);

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
  ligma_n_point_deformation_options_set_sensitivity (npd_options, TRUE);

  /* compute and get model */
  gegl_node_process (npd_tool->npd_node);
  gegl_node_get (npd_tool->npd_node, "model", &model, NULL);

  npd_tool->model          = model;
  npd_tool->preview_buffer = preview_buffer;
  npd_tool->selected_cp    = NULL;
  npd_tool->hovering_cp    = NULL;
  npd_tool->selected_cps   = NULL;
  npd_tool->rubber_band    = FALSE;
  npd_tool->lattice_points = g_new (LigmaVector2,
                                    5 * model->hidden_model->num_of_bones);

  ligma_item_get_offset (LIGMA_ITEM (tool->drawables->data),
                        &npd_tool->offset_x, &npd_tool->offset_y);
  ligma_npd_debug (("offset: %f %f\n", npd_tool->offset_x, npd_tool->offset_y));

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (npd_tool), display);

  ligma_n_point_deformation_tool_perform_deformation (npd_tool);

  /* hide original image */
  ligma_item_set_visible (LIGMA_ITEM (tool->drawables->data), FALSE, FALSE);
  ligma_image_flush (image);

  /* create and start a deformation thread */
  npd_tool->deform_thread =
    g_thread_new ("deform thread",
                  (GThreadFunc) ligma_n_point_deformation_tool_deform_thread_func,
                  npd_tool);

  /* create and start canvas update timeout */
  npd_tool->draw_timeout_id =
    gdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE,
                                  LIGMA_NPD_DRAW_INTERVAL,
                                  (GSourceFunc) ligma_n_point_deformation_tool_canvas_update_timeout,
                                  npd_tool,
                                  NULL);
}

static void
ligma_n_point_deformation_tool_commit (LigmaNPointDeformationTool *npd_tool)
{
  LigmaTool *tool = LIGMA_TOOL (npd_tool);

  if (npd_tool->active)
    {
      ligma_n_point_deformation_tool_halt_threads (npd_tool);

      ligma_tool_control_push_preserve (tool->control, TRUE);
      ligma_n_point_deformation_tool_apply_deformation (npd_tool);
      ligma_tool_control_pop_preserve (tool->control);

      /* show original/deformed image */
      ligma_item_set_visible (LIGMA_ITEM (tool->drawables->data), TRUE, FALSE);
      ligma_image_flush (ligma_display_get_image (tool->display));

      npd_tool->active = FALSE;
    }
}

static void
ligma_n_point_deformation_tool_halt (LigmaNPointDeformationTool *npd_tool)
{
  LigmaTool                     *tool      = LIGMA_TOOL (npd_tool);
  LigmaDrawTool                 *draw_tool = LIGMA_DRAW_TOOL (npd_tool);
  LigmaNPointDeformationOptions *npd_options;

  npd_options = LIGMA_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  if (npd_tool->active)
    {
      ligma_n_point_deformation_tool_halt_threads (npd_tool);

      /* show original/deformed image */
      ligma_item_set_visible (LIGMA_ITEM (tool->drawables->data), TRUE, FALSE);
      ligma_image_flush (ligma_display_get_image (tool->display));

      /* disable some options */
      ligma_n_point_deformation_options_set_sensitivity (npd_options, FALSE);

      npd_tool->active = FALSE;
    }

  if (ligma_draw_tool_is_active (draw_tool))
    ligma_draw_tool_stop (draw_tool);

  ligma_n_point_deformation_tool_clear_selected_points_list (npd_tool);

  g_clear_object (&npd_tool->graph);
  npd_tool->source   = NULL;
  npd_tool->npd_node = NULL;
  npd_tool->sink     = NULL;

  g_clear_object (&npd_tool->preview_buffer);
  g_clear_pointer (&npd_tool->lattice_points, g_free);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
ligma_n_point_deformation_tool_set_options (LigmaNPointDeformationTool    *npd_tool,
                                           LigmaNPointDeformationOptions *npd_options)
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
ligma_n_point_deformation_tool_options_notify (LigmaTool         *tool,
                                              LigmaToolOptions  *options,
                                              const GParamSpec *pspec)
{
  LigmaNPointDeformationTool    *npd_tool    = LIGMA_N_POINT_DEFORMATION_TOOL (tool);
  LigmaNPointDeformationOptions *npd_options = LIGMA_N_POINT_DEFORMATION_OPTIONS (options);
  LigmaDrawTool                 *draw_tool   = LIGMA_DRAW_TOOL (tool);

  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! npd_tool->active)
    return;

  ligma_draw_tool_pause (draw_tool);

  ligma_npd_debug (("npd options notify\n"));
  ligma_n_point_deformation_tool_set_options (npd_tool, npd_options);

  ligma_draw_tool_resume (draw_tool);
}

static gboolean
ligma_n_point_deformation_tool_key_press (LigmaTool    *tool,
                                         GdkEventKey *kevent,
                                         LigmaDisplay *display)
{
  LigmaNPointDeformationTool *npd_tool = LIGMA_N_POINT_DEFORMATION_TOOL (tool);

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

          ligma_npd_debug (("removing last cp %p\n", cp));
          ligma_n_point_deformation_tool_remove_cp_from_selection (npd_tool, cp);
          npd_remove_control_point (npd_tool->model, cp);
        }
      break;

    case GDK_KEY_Delete:
      if (npd_tool->model &&
          npd_tool->selected_cps)
        {
          /* if there is at least one selected control point, remove it */
          npd_remove_control_points (npd_tool->model, npd_tool->selected_cps);
          ligma_n_point_deformation_tool_clear_selected_points_list (npd_tool);
        }
      break;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, display);
      break;

    case GDK_KEY_Escape:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
      break;

    default:
      return FALSE;
  }

  return TRUE;
}

static void
ligma_n_point_deformation_tool_modifier_key (LigmaTool        *tool,
                                            GdkModifierType  key,
                                            gboolean         press,
                                            GdkModifierType  state,
                                            LigmaDisplay     *display)
{
}

static void
ligma_n_point_deformation_tool_cursor_update (LigmaTool         *tool,
                                             const LigmaCoords *coords,
                                             GdkModifierType   state,
                                             LigmaDisplay      *display)
{
  LigmaNPointDeformationTool *npd_tool = LIGMA_N_POINT_DEFORMATION_TOOL (tool);
  LigmaCursorModifier         modifier = LIGMA_CURSOR_MODIFIER_PLUS;

  if (! npd_tool->active)
    {
      modifier = LIGMA_CURSOR_MODIFIER_NONE;
    }
  else if (npd_tool->hovering_cp)
    {
      modifier = LIGMA_CURSOR_MODIFIER_MOVE;
    }

  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_n_point_deformation_tool_clear_selected_points_list (LigmaNPointDeformationTool *npd_tool)
{
  if (npd_tool->selected_cps)
    {
      g_list_free (npd_tool->selected_cps);
      npd_tool->selected_cps = NULL;
    }
}

static gboolean
ligma_n_point_deformation_tool_add_cp_to_selection (LigmaNPointDeformationTool *npd_tool,
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
ligma_n_point_deformation_tool_remove_cp_from_selection (LigmaNPointDeformationTool *npd_tool,
                                                        NPDControlPoint           *cp)
{
  npd_tool->selected_cps = g_list_remove (npd_tool->selected_cps, cp);
}

static void
ligma_n_point_deformation_tool_button_press (LigmaTool            *tool,
                                            const LigmaCoords    *coords,
                                            guint32              time,
                                            GdkModifierType      state,
                                            LigmaButtonPressType  press_type,
                                            LigmaDisplay         *display)
{
  LigmaNPointDeformationTool *npd_tool = LIGMA_N_POINT_DEFORMATION_TOOL (tool);

  if (display != tool->display)
    {
      /* this is the first click on the drawable - just start the tool */
      ligma_n_point_deformation_tool_start (npd_tool, display);
    }

  npd_tool->selected_cp = NULL;

  if (press_type == LIGMA_BUTTON_PRESS_NORMAL)
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

              if (! (state & ligma_get_extend_selection_mask ()))
                {
                  /* <SHIFT> isn't pressed, so this isn't a
                   * multiselection - clear the list of selected
                   * control points
                   */
                  ligma_n_point_deformation_tool_clear_selected_points_list (npd_tool);
                }

              ligma_n_point_deformation_tool_add_cp_to_selection (npd_tool, cp);
            }
          else if (state & ligma_get_extend_selection_mask ())
            {
              /* control point is selected and <SHIFT> is pressed -
               * remove control point from selected points
               */
              ligma_n_point_deformation_tool_remove_cp_from_selection (npd_tool,
                                                                      cp);
            }
        }

      npd_tool->start_x = coords->x;
      npd_tool->start_y = coords->y;

      npd_tool->last_x = coords->x;
      npd_tool->last_y = coords->y;
    }

  ligma_tool_control_activate (tool->control);
}

static gboolean
ligma_n_point_deformation_tool_is_cp_in_area (NPDControlPoint *cp,
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
ligma_n_point_deformation_tool_button_release (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              LigmaButtonReleaseType  release_type,
                                              LigmaDisplay           *display)
{
  LigmaNPointDeformationTool *npd_tool = LIGMA_N_POINT_DEFORMATION_TOOL (tool);

  ligma_tool_control_halt (tool->control);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (npd_tool));

  if (release_type == LIGMA_BUTTON_RELEASE_CLICK)
    {
      if (! npd_tool->hovering_cp)
        {
          NPDPoint p;

          ligma_npd_debug (("cp doesn't exist, adding\n"));
          p.x = coords->x - npd_tool->offset_x;
          p.y = coords->y - npd_tool->offset_y;

          npd_add_control_point (npd_tool->model, &p);
        }
    }
  else if (release_type == LIGMA_BUTTON_RELEASE_NORMAL)
    {
      if (npd_tool->rubber_band)
        {
          GArray *cps = npd_tool->model->control_points;
          gint    x0  = MIN (npd_tool->start_x, npd_tool->cursor_x);
          gint    y0  = MIN (npd_tool->start_y, npd_tool->cursor_y);
          gint    x1  = MAX (npd_tool->start_x, npd_tool->cursor_x);
          gint    y1  = MAX (npd_tool->start_y, npd_tool->cursor_y);
          gint    i;

          if (! (state & ligma_get_extend_selection_mask ()))
            {
              /* <SHIFT> isn't pressed, so we want a clear selection */
              ligma_n_point_deformation_tool_clear_selected_points_list (npd_tool);
            }

          for (i = 0; i < cps->len; i++)
            {
              NPDControlPoint *cp = &g_array_index (cps, NPDControlPoint, i);

              if (ligma_n_point_deformation_tool_is_cp_in_area (cp,
                                                               x0, y0,
                                                               x1, y1,
                                                               npd_tool->offset_x,
                                                               npd_tool->offset_y,
                                                               npd_tool->cp_scaled_radius))
                {
                  /* control point is situated in an area defined by
                   * rubber band
                  */
                  ligma_n_point_deformation_tool_add_cp_to_selection (npd_tool,
                                                                     cp);
                  ligma_npd_debug (("%p selected\n", cp));
                }
            }
        }
    }
  else if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
    {
      ligma_npd_debug (("ligma_button_release_cancel\n"));
    }

  npd_tool->rubber_band = FALSE;

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (npd_tool));
}

static void
ligma_n_point_deformation_tool_oper_update (LigmaTool         *tool,
                                           const LigmaCoords *coords,
                                           GdkModifierType   state,
                                           gboolean          proximity,
                                           LigmaDisplay      *display)
{
  LigmaNPointDeformationTool *npd_tool  = LIGMA_N_POINT_DEFORMATION_TOOL (tool);
  LigmaDrawTool              *draw_tool = LIGMA_DRAW_TOOL (tool);

  ligma_draw_tool_pause (draw_tool);

  if (npd_tool->active)
    {
      NPDModel         *model = npd_tool->model;
      LigmaDisplayShell *shell = ligma_display_get_shell (display);
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

  ligma_draw_tool_resume (draw_tool);
}

static void
ligma_n_point_deformation_tool_prepare_lattice (LigmaNPointDeformationTool *npd_tool)
{
  NPDHiddenModel *hm     = npd_tool->model->hidden_model;
  LigmaVector2    *points = npd_tool->lattice_points;
  gint            i, j;

  for (i = 0; i < hm->num_of_bones; i++)
    {
      NPDBone *bone = &hm->current_bones[i];

      for (j = 0; j < 4; j++)
        ligma_vector2_set (&points[5 * i + j], bone->points[j].x, bone->points[j].y);
      ligma_vector2_set (&points[5 * i + j], bone->points[0].x, bone->points[0].y);
    }
}

static void
ligma_n_point_deformation_tool_draw_lattice (LigmaNPointDeformationTool *npd_tool)
{
  LigmaVector2 *points    = npd_tool->lattice_points;
  gint         n_squares = npd_tool->model->hidden_model->num_of_bones;
  gint         i;

  for (i = 0; i < n_squares; i++)
    ligma_draw_tool_add_lines (LIGMA_DRAW_TOOL (npd_tool),
                              &points[5 * i], 5, NULL, FALSE);
}

static void
ligma_n_point_deformation_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaNPointDeformationTool    *npd_tool;
  LigmaNPointDeformationOptions *npd_options;
  NPDModel                     *model;
  gint                          x0, y0, x1, y1;
  gint                          i;

  npd_tool    = LIGMA_N_POINT_DEFORMATION_TOOL (draw_tool);
  npd_options = LIGMA_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  model = npd_tool->model;

  g_return_if_fail (model != NULL);

  /* draw lattice */
  if (npd_options->mesh_visible)
    ligma_n_point_deformation_tool_draw_lattice (npd_tool);

  x0 = MIN (npd_tool->start_x, npd_tool->cursor_x);
  y0 = MIN (npd_tool->start_y, npd_tool->cursor_y);
  x1 = MAX (npd_tool->start_x, npd_tool->cursor_x);
  y1 = MAX (npd_tool->start_y, npd_tool->cursor_y);

  for (i = 0; i < model->control_points->len; i++)
    {
      NPDControlPoint *cp = &g_array_index (model->control_points,
                                            NPDControlPoint, i);
      NPDPoint         p  = cp->point;
      LigmaHandleType   handle_type;

      p.x += npd_tool->offset_x;
      p.y += npd_tool->offset_y;

      handle_type = LIGMA_HANDLE_CIRCLE;

      /* check if cursor is hovering over a control point or if a
       * control point is situated in an area defined by rubber band
       */
      if (cp == npd_tool->hovering_cp ||
          (npd_tool->rubber_band &&
           ligma_n_point_deformation_tool_is_cp_in_area (cp,
                                                        x0, y0,
                                                        x1, y1,
                                                        npd_tool->offset_x,
                                                        npd_tool->offset_y,
                                                        npd_tool->cp_scaled_radius)))
        {
          handle_type = LIGMA_HANDLE_FILLED_CIRCLE;
        }

      ligma_draw_tool_add_handle (draw_tool,
                                 handle_type,
                                 p.x, p.y,
                                 LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                 LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                 LIGMA_HANDLE_ANCHOR_CENTER);

      if (g_list_find (npd_tool->selected_cps, cp))
        {
          ligma_draw_tool_add_handle (draw_tool,
                                     LIGMA_HANDLE_SQUARE,
                                     p.x, p.y,
                                     LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                     LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                     LIGMA_HANDLE_ANCHOR_CENTER);
        }
    }

  if (npd_tool->rubber_band)
    {
      /* draw a rubber band */
      ligma_draw_tool_add_rectangle (draw_tool, FALSE,
                                    x0, y0, x1 - x0, y1 - y0);
    }

  if (npd_tool->preview_buffer)
    {
      LigmaCanvasItem *item;

      item = ligma_canvas_buffer_preview_new (ligma_display_get_shell (draw_tool->display),
                                             npd_tool->preview_buffer);

      ligma_draw_tool_add_preview (draw_tool, item);
      g_object_unref (item);
    }

  LIGMA_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
ligma_n_point_deformation_tool_motion (LigmaTool         *tool,
                                      const LigmaCoords *coords,
                                      guint32           time,
                                      GdkModifierType   state,
                                      LigmaDisplay      *display)
{
  LigmaNPointDeformationTool *npd_tool  = LIGMA_N_POINT_DEFORMATION_TOOL (tool);
  LigmaDrawTool              *draw_tool = LIGMA_DRAW_TOOL (tool);

  ligma_draw_tool_pause (draw_tool);

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

  ligma_draw_tool_resume (draw_tool);
}

static gboolean
ligma_n_point_deformation_tool_canvas_update_timeout (LigmaNPointDeformationTool *npd_tool)
{
  if (! LIGMA_TOOL (npd_tool)->drawables->data)
    return FALSE;

  ligma_npd_debug (("canvas update thread\n"));

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL(npd_tool));
  ligma_draw_tool_resume (LIGMA_DRAW_TOOL(npd_tool));

  ligma_npd_debug (("canvas update thread stop\n"));

  return TRUE;
}

static gpointer
ligma_n_point_deformation_tool_deform_thread_func (gpointer data)
{
  LigmaNPointDeformationTool    *npd_tool = data;
  LigmaNPointDeformationOptions *npd_options;
  guint64                       start, duration;

  npd_options = LIGMA_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  npd_tool->deformation_active = TRUE;

  while (npd_tool->deformation_active)
    {
      start = g_get_monotonic_time ();

      ligma_n_point_deformation_tool_perform_deformation (npd_tool);

      if (npd_options->mesh_visible)
        ligma_n_point_deformation_tool_prepare_lattice (npd_tool);

      duration = g_get_monotonic_time () - start;
      if (duration < LIGMA_NPD_MAXIMUM_DEFORMATION_DELAY)
        {
          g_usleep (LIGMA_NPD_MAXIMUM_DEFORMATION_DELAY - duration);
        }
    }

  ligma_npd_debug (("deform thread exit\n"));

  return NULL;
}

static void
ligma_n_point_deformation_tool_perform_deformation (LigmaNPointDeformationTool *npd_tool)
{
  GObject *operation;

  g_object_get (npd_tool->npd_node,
                "gegl-operation", &operation,
                NULL);
  ligma_npd_debug (("gegl_operation_invalidate\n"));
  gegl_operation_invalidate (GEGL_OPERATION (operation), NULL, FALSE);
  g_object_unref (operation);

  ligma_npd_debug (("gegl_node_process\n"));
  gegl_node_process (npd_tool->sink);
}

static void
ligma_n_point_deformation_tool_halt_threads (LigmaNPointDeformationTool *npd_tool)
{
  if (! npd_tool->deformation_active)
    return;

  ligma_npd_debug (("waiting for deform thread to finish\n"));
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

  ligma_npd_debug (("finished\n"));
}

static void
ligma_n_point_deformation_tool_apply_deformation (LigmaNPointDeformationTool *npd_tool)
{
  LigmaTool                     *tool = LIGMA_TOOL (npd_tool);
  LigmaNPointDeformationOptions *npd_options;
  GeglBuffer                   *buffer;
  LigmaImage                    *image;
  gint                          width, height, prev;

  npd_options = LIGMA_N_POINT_DEFORMATION_TOOL_GET_OPTIONS (npd_tool);

  image  = ligma_display_get_image (tool->display);
  buffer = ligma_drawable_get_buffer (tool->drawables->data);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  prev = npd_options->rigidity;
  npd_options->rigidity = 0;
  ligma_n_point_deformation_tool_set_options (npd_tool, npd_options);
  npd_options->rigidity = prev;

  ligma_drawable_push_undo (tool->drawables->data, _("N-Point Deformation"), NULL,
                           0, 0, width, height);


  ligma_gegl_apply_operation (NULL, NULL, _("N-Point Deformation"),
                             npd_tool->npd_node,
                             ligma_drawable_get_buffer (tool->drawables->data),
                             NULL, FALSE);

  ligma_drawable_update (tool->drawables->data,
                        0, 0, width, height);

  ligma_projection_flush (ligma_image_get_projection (image));
}

#undef ligma_npd_debug
#ifdef LIGMA_NPD_DEBUG
#undef LIGMA_NPD_DEBUG
#endif
