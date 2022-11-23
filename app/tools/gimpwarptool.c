/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmawarptool.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gegl-plugin.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmadisplayconfig.h"
#include "config/ligmaguiconfig.h"

#include "gegl/ligma-gegl-apply-operation.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmadrawablefilter.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmaprogress.h"
#include "core/ligmaprojection.h"
#include "core/ligmasubprogress.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"

#include "ligmawarptool.h"
#include "ligmawarpoptions.h"
#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"

#include "ligma-intl.h"


#define STROKE_TIMER_MAX_FPS 20
#define PREVIEW_SAMPLER      GEGL_SAMPLER_NEAREST


static void            ligma_warp_tool_constructed               (GObject               *object);

static void            ligma_warp_tool_control                   (LigmaTool              *tool,
                                                                 LigmaToolAction         action,
                                                                 LigmaDisplay           *display);
static void            ligma_warp_tool_button_press              (LigmaTool              *tool,
                                                                 const LigmaCoords      *coords,
                                                                 guint32                time,
                                                                 GdkModifierType        state,
                                                                 LigmaButtonPressType    press_type,
                                                                 LigmaDisplay           *display);
static void            ligma_warp_tool_button_release            (LigmaTool              *tool,
                                                                 const LigmaCoords      *coords,
                                                                 guint32                time,
                                                                 GdkModifierType        state,
                                                                 LigmaButtonReleaseType  release_type,
                                                                 LigmaDisplay           *display);
static void            ligma_warp_tool_motion                    (LigmaTool              *tool,
                                                                 const LigmaCoords      *coords,
                                                                 guint32                time,
                                                                 GdkModifierType        state,
                                                                 LigmaDisplay           *display);
static gboolean        ligma_warp_tool_key_press                 (LigmaTool              *tool,
                                                                 GdkEventKey           *kevent,
                                                                 LigmaDisplay           *display);
static void            ligma_warp_tool_oper_update               (LigmaTool              *tool,
                                                                 const LigmaCoords      *coords,
                                                                 GdkModifierType        state,
                                                                 gboolean               proximity,
                                                                 LigmaDisplay           *display);
static void            ligma_warp_tool_cursor_update             (LigmaTool              *tool,
                                                                 const LigmaCoords      *coords,
                                                                 GdkModifierType        state,
                                                                 LigmaDisplay           *display);
const gchar          * ligma_warp_tool_can_undo                  (LigmaTool              *tool,
                                                                 LigmaDisplay           *display);
const gchar          * ligma_warp_tool_can_redo                  (LigmaTool              *tool,
                                                                 LigmaDisplay           *display);
static gboolean        ligma_warp_tool_undo                      (LigmaTool              *tool,
                                                                 LigmaDisplay           *display);
static gboolean        ligma_warp_tool_redo                      (LigmaTool              *tool,
                                                                 LigmaDisplay           *display);
static void            ligma_warp_tool_options_notify            (LigmaTool              *tool,
                                                                 LigmaToolOptions       *options,
                                                                 const GParamSpec      *pspec);

static void            ligma_warp_tool_draw                      (LigmaDrawTool          *draw_tool);

static void            ligma_warp_tool_cursor_notify             (LigmaDisplayConfig     *config,
                                                                 GParamSpec            *pspec,
                                                                 LigmaWarpTool          *wt);

static gboolean        ligma_warp_tool_can_stroke                (LigmaWarpTool          *wt,
                                                                 LigmaDisplay           *display,
                                                                 gboolean               show_message);

static gboolean        ligma_warp_tool_start                     (LigmaWarpTool          *wt,
                                                                 LigmaDisplay           *display);
static void            ligma_warp_tool_halt                      (LigmaWarpTool          *wt);
static void            ligma_warp_tool_commit                    (LigmaWarpTool          *wt);

static void            ligma_warp_tool_start_stroke_timer        (LigmaWarpTool          *wt);
static void            ligma_warp_tool_stop_stroke_timer         (LigmaWarpTool          *wt);
static gboolean        ligma_warp_tool_stroke_timer              (LigmaWarpTool          *wt);

static void            ligma_warp_tool_create_graph              (LigmaWarpTool          *wt);
static void            ligma_warp_tool_create_filter             (LigmaWarpTool          *wt,
                                                                 LigmaDrawable          *drawable);
static void            ligma_warp_tool_set_sampler               (LigmaWarpTool          *wt,
                                                                 gboolean               commit);
static GeglRectangle
                       ligma_warp_tool_get_stroke_bounds         (GeglNode              *node);
static GeglRectangle   ligma_warp_tool_get_node_bounds           (GeglNode              *node);
static void            ligma_warp_tool_clear_node_bounds         (GeglNode              *node);
static GeglRectangle   ligma_warp_tool_get_invalidated_by_change (LigmaWarpTool          *wt,
                                                                 const GeglRectangle   *area);
static void            ligma_warp_tool_update_bounds             (LigmaWarpTool          *wt);
static void            ligma_warp_tool_update_area               (LigmaWarpTool          *wt,
                                                                 const GeglRectangle   *area,
                                                                 gboolean               synchronous);
static void            ligma_warp_tool_update_stroke             (LigmaWarpTool          *wt,
                                                                 GeglNode              *node);
static void            ligma_warp_tool_stroke_append             (LigmaWarpTool          *wt,
                                                                 gchar                  type,
                                                                 gdouble                x,
                                                                 gdouble                y);
static void            ligma_warp_tool_filter_flush              (LigmaDrawableFilter    *filter,
                                                                 LigmaTool              *tool);
static void            ligma_warp_tool_add_op                    (LigmaWarpTool          *wt,
                                                                 GeglNode              *op);
static void            ligma_warp_tool_remove_op                 (LigmaWarpTool          *wt,
                                                                 GeglNode              *op);
static void            ligma_warp_tool_free_op                   (GeglNode              *op);

static void            ligma_warp_tool_animate                   (LigmaWarpTool          *wt);


G_DEFINE_TYPE (LigmaWarpTool, ligma_warp_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_warp_tool_parent_class


void
ligma_warp_tool_register (LigmaToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (LIGMA_TYPE_WARP_TOOL,
                LIGMA_TYPE_WARP_OPTIONS,
                ligma_warp_options_gui,
                0,
                "ligma-warp-tool",
                _("Warp Transform"),
                _("Warp Transform: Deform with different tools"),
                N_("_Warp Transform"), "W",
                NULL, LIGMA_HELP_TOOL_WARP,
                LIGMA_ICON_TOOL_WARP,
                data);
}

static void
ligma_warp_tool_class_init (LigmaWarpToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = ligma_warp_tool_constructed;

  tool_class->control        = ligma_warp_tool_control;
  tool_class->button_press   = ligma_warp_tool_button_press;
  tool_class->button_release = ligma_warp_tool_button_release;
  tool_class->motion         = ligma_warp_tool_motion;
  tool_class->key_press      = ligma_warp_tool_key_press;
  tool_class->oper_update    = ligma_warp_tool_oper_update;
  tool_class->cursor_update  = ligma_warp_tool_cursor_update;
  tool_class->can_undo       = ligma_warp_tool_can_undo;
  tool_class->can_redo       = ligma_warp_tool_can_redo;
  tool_class->undo           = ligma_warp_tool_undo;
  tool_class->redo           = ligma_warp_tool_redo;
  tool_class->options_notify = ligma_warp_tool_options_notify;

  draw_tool_class->draw      = ligma_warp_tool_draw;
}

static void
ligma_warp_tool_init (LigmaWarpTool *self)
{
  LigmaTool *tool = LIGMA_TOOL (self);

  ligma_tool_control_set_motion_mode     (tool->control, LIGMA_MOTION_MODE_EXACT);
  ligma_tool_control_set_scroll_lock     (tool->control, TRUE);
  ligma_tool_control_set_preserve        (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask      (tool->control,
                                         LIGMA_DIRTY_IMAGE           |
                                         LIGMA_DIRTY_DRAWABLE        |
                                         LIGMA_DIRTY_SELECTION       |
                                         LIGMA_DIRTY_ACTIVE_DRAWABLE);
  ligma_tool_control_set_dirty_action    (tool->control,
                                         LIGMA_TOOL_ACTION_COMMIT);
  ligma_tool_control_set_wants_click     (tool->control, TRUE);
  ligma_tool_control_set_precision       (tool->control,
                                         LIGMA_CURSOR_PRECISION_SUBPIXEL);
  ligma_tool_control_set_tool_cursor     (tool->control,
                                         LIGMA_TOOL_CURSOR_WARP);

  ligma_tool_control_set_action_pixel_size (tool->control,
                                           "tools/tools-warp-effect-pixel-size-set");
  ligma_tool_control_set_action_size       (tool->control,
                                           "tools/tools-warp-effect-size-set");
  ligma_tool_control_set_action_hardness   (tool->control,
                                           "tools/tools-warp-effect-hardness-set");

  self->show_cursor = TRUE;
  self->draw_brush  = TRUE;
  self->snap_brush  = FALSE;
}

static void
ligma_warp_tool_constructed (GObject *object)
{
  LigmaWarpTool      *wt   = LIGMA_WARP_TOOL (object);
  LigmaTool          *tool = LIGMA_TOOL (object);
  LigmaDisplayConfig *display_config;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  display_config = LIGMA_DISPLAY_CONFIG (tool->tool_info->ligma->config);

  wt->show_cursor = display_config->show_paint_tool_cursor;
  wt->draw_brush  = display_config->show_brush_outline;
  wt->snap_brush  = display_config->snap_brush_outline;

  g_signal_connect_object (display_config, "notify::show-paint-tool-cursor",
                           G_CALLBACK (ligma_warp_tool_cursor_notify),
                           wt, 0);
  g_signal_connect_object (display_config, "notify::show-brush-outline",
                           G_CALLBACK (ligma_warp_tool_cursor_notify),
                           wt, 0);
  g_signal_connect_object (display_config, "notify::snap-brush-outline",
                           G_CALLBACK (ligma_warp_tool_cursor_notify),
                           wt, 0);
}

static void
ligma_warp_tool_control (LigmaTool       *tool,
                        LigmaToolAction  action,
                        LigmaDisplay    *display)
{
  LigmaWarpTool *wt = LIGMA_WARP_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_warp_tool_halt (wt);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_warp_tool_commit (wt);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_warp_tool_button_press (LigmaTool            *tool,
                             const LigmaCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             LigmaButtonPressType  press_type,
                             LigmaDisplay         *display)
{
  LigmaWarpTool    *wt      = LIGMA_WARP_TOOL (tool);
  LigmaWarpOptions *options = LIGMA_WARP_TOOL_GET_OPTIONS (wt);
  GeglNode        *new_op;
  gint             off_x, off_y;

  if (tool->display && display != tool->display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      if (! ligma_warp_tool_start (wt, display))
        return;
    }

  if (! ligma_warp_tool_can_stroke (wt, display, TRUE))
    return;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  wt->current_stroke = gegl_path_new ();

  wt->last_pos.x = coords->x;
  wt->last_pos.y = coords->y;

  wt->total_dist = 0.0;

  new_op = gegl_node_new_child (NULL,
                                "operation", "gegl:warp",
                                "behavior",  options->behavior,
                                "size",      options->effect_size,
                                "hardness",  options->effect_hardness / 100.0,
                                "strength",  options->effect_strength,
                                /* we implement spacing manually.
                                 * anything > 1 will do.
                                 */
                                "spacing",   10.0,
                                "stroke",    wt->current_stroke,
                                NULL);

  ligma_warp_tool_add_op (wt, new_op);
  g_object_unref (new_op);

  ligma_item_get_offset (LIGMA_ITEM (tool->drawables->data), &off_x, &off_y);

  ligma_warp_tool_stroke_append (wt,
                                'M', wt->last_pos.x - off_x,
                                     wt->last_pos.y - off_y);

  ligma_warp_tool_start_stroke_timer (wt);

  ligma_tool_control_activate (tool->control);
}

void
ligma_warp_tool_button_release (LigmaTool              *tool,
                               const LigmaCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               LigmaButtonReleaseType  release_type,
                               LigmaDisplay           *display)
{
  LigmaWarpTool *wt = LIGMA_WARP_TOOL (tool);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (wt));

  ligma_tool_control_halt (tool->control);

  ligma_warp_tool_stop_stroke_timer (wt);

#ifdef WARP_DEBUG
  g_printerr ("%s\n", gegl_path_to_string (wt->current_stroke));
#endif

  g_clear_object (&wt->current_stroke);

  if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
    {
      ligma_warp_tool_undo (tool, display);

      /*  the just undone stroke has no business on the redo stack  */
      ligma_warp_tool_free_op (wt->redo_stack->data);
      wt->redo_stack = g_list_remove_link (wt->redo_stack, wt->redo_stack);
    }
  else
    {
      if (wt->redo_stack)
        {
          /*  the redo stack becomes invalid by actually doing a stroke  */
          g_list_free_full (wt->redo_stack,
                            (GDestroyNotify) ligma_warp_tool_free_op);
          wt->redo_stack = NULL;
        }

      ligma_tool_push_status (tool, tool->display,
                             _("Press ENTER to commit the transform"));
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

  /*  update the undo actions / menu items  */
  ligma_image_flush (ligma_display_get_image (LIGMA_TOOL (wt)->display));
}

static void
ligma_warp_tool_motion (LigmaTool         *tool,
                       const LigmaCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       LigmaDisplay      *display)
{
  LigmaWarpTool    *wt             = LIGMA_WARP_TOOL (tool);
  LigmaWarpOptions *options        = LIGMA_WARP_TOOL_GET_OPTIONS (wt);
  LigmaVector2      old_cursor_pos;
  LigmaVector2      delta;
  gdouble          dist;
  gdouble          step;
  gboolean         stroke_changed = FALSE;

  if (! wt->snap_brush)
    ligma_draw_tool_pause (LIGMA_DRAW_TOOL (wt));

  old_cursor_pos = wt->cursor_pos;

  wt->cursor_pos.x = coords->x;
  wt->cursor_pos.y = coords->y;

  ligma_vector2_sub (&delta, &wt->cursor_pos, &old_cursor_pos);
  dist = ligma_vector2_length (&delta);

  step = options->effect_size * options->stroke_spacing / 100.0;

  while (wt->total_dist + dist >= step)
    {
      gdouble diff = step - wt->total_dist;

      ligma_vector2_mul (&delta, diff / dist);
      ligma_vector2_add (&old_cursor_pos, &old_cursor_pos, &delta);

      ligma_vector2_sub (&delta, &wt->cursor_pos, &old_cursor_pos);
      dist -= diff;

      wt->last_pos   = old_cursor_pos;
      wt->total_dist = 0.0;

      if (options->stroke_during_motion)
        {
          gint off_x, off_y;

          if (! stroke_changed)
            {
              stroke_changed = TRUE;

              ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));
            }

          ligma_item_get_offset (LIGMA_ITEM (tool->drawables->data), &off_x, &off_y);

          ligma_warp_tool_stroke_append (wt,
                                        'L', wt->last_pos.x - off_x,
                                             wt->last_pos.y - off_y);
        }
    }

  wt->total_dist += dist;

  if (stroke_changed)
    {
      ligma_warp_tool_start_stroke_timer (wt);

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
    }

  if (! wt->snap_brush)
    ligma_draw_tool_resume (LIGMA_DRAW_TOOL (wt));
}

static gboolean
ligma_warp_tool_key_press (LigmaTool    *tool,
                          GdkEventKey *kevent,
                          LigmaDisplay *display)
{
  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      return TRUE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, display);
      return TRUE;

    case GDK_KEY_Escape:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
ligma_warp_tool_oper_update (LigmaTool         *tool,
                            const LigmaCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            LigmaDisplay      *display)
{
  LigmaWarpTool *wt        = LIGMA_WARP_TOOL (tool);
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (proximity)
    {
      ligma_draw_tool_pause (draw_tool);

      if (! tool->display || display == tool->display)
        {
          wt->cursor_pos.x = coords->x;
          wt->cursor_pos.y = coords->y;

          wt->last_pos = wt->cursor_pos;
        }

      if (! ligma_draw_tool_is_active (draw_tool))
        ligma_draw_tool_start (draw_tool, display);

      ligma_draw_tool_resume (draw_tool);
    }
  else if (ligma_draw_tool_is_active (draw_tool))
    {
      ligma_draw_tool_stop (draw_tool);
    }
}

static void
ligma_warp_tool_cursor_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              LigmaDisplay      *display)
{
  LigmaWarpTool       *wt       = LIGMA_WARP_TOOL (tool);
  LigmaWarpOptions    *options  = LIGMA_WARP_TOOL_GET_OPTIONS (tool);
  LigmaCursorModifier  modifier = LIGMA_CURSOR_MODIFIER_NONE;

  if (! ligma_warp_tool_can_stroke (wt, display, FALSE))
    {
      modifier = LIGMA_CURSOR_MODIFIER_BAD;
    }
  else if (display == tool->display)
    {
#if 0
      /* FIXME have better cursors  */

      switch (options->behavior)
        {
        case LIGMA_WARP_BEHAVIOR_MOVE:
        case LIGMA_WARP_BEHAVIOR_GROW:
        case LIGMA_WARP_BEHAVIOR_SHRINK:
        case LIGMA_WARP_BEHAVIOR_SWIRL_CW:
        case LIGMA_WARP_BEHAVIOR_SWIRL_CCW:
        case LIGMA_WARP_BEHAVIOR_ERASE:
        case LIGMA_WARP_BEHAVIOR_SMOOTH:
          modifier = LIGMA_CURSOR_MODIFIER_MOVE;
          break;
        }
#else
      (void) options;
#endif
    }

  if (! wt->show_cursor && modifier != LIGMA_CURSOR_MODIFIER_BAD)
    {
      ligma_tool_set_cursor (tool, display,
                            LIGMA_CURSOR_NONE,
                            LIGMA_TOOL_CURSOR_NONE,
                            LIGMA_CURSOR_MODIFIER_NONE);
      return;
    }

  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

const gchar *
ligma_warp_tool_can_undo (LigmaTool    *tool,
                         LigmaDisplay *display)
{
  LigmaWarpTool *wt = LIGMA_WARP_TOOL (tool);
  GeglNode     *to_delete;
  const gchar  *type;

  if (! wt->render_node)
    return NULL;

  to_delete = gegl_node_get_producer (wt->render_node, "aux", NULL);
  type = gegl_node_get_operation (to_delete);

  if (strcmp (type, "gegl:warp"))
    return NULL;

  return _("Warp Tool Stroke");
}

const gchar *
ligma_warp_tool_can_redo (LigmaTool    *tool,
                         LigmaDisplay *display)
{
  LigmaWarpTool *wt = LIGMA_WARP_TOOL (tool);

  if (! wt->render_node || ! wt->redo_stack)
    return NULL;

  return _("Warp Tool Stroke");
}

static gboolean
ligma_warp_tool_undo (LigmaTool    *tool,
                     LigmaDisplay *display)
{
  LigmaWarpTool *wt = LIGMA_WARP_TOOL (tool);
  GeglNode     *to_delete;
  GeglNode     *prev_node;

  to_delete = gegl_node_get_producer (wt->render_node, "aux", NULL);

  wt->redo_stack = g_list_prepend (wt->redo_stack, to_delete);

  /* we connect render_node to the previous node, but keep the current node
   * in the graph, connected to the previous node as well, so that it doesn't
   * get invalidated and maintains its cache.  this way, redoing it doesn't
   * require reprocessing.
   */
  prev_node = gegl_node_get_producer (to_delete, "input", NULL);

  gegl_node_connect_to (prev_node,       "output",
                        wt->render_node, "aux");

  ligma_warp_tool_update_bounds (wt);
  ligma_warp_tool_update_stroke (wt, to_delete);

  return TRUE;
}

static gboolean
ligma_warp_tool_redo (LigmaTool    *tool,
                     LigmaDisplay *display)
{
  LigmaWarpTool *wt = LIGMA_WARP_TOOL (tool);
  GeglNode     *to_add;

  to_add = wt->redo_stack->data;

  gegl_node_connect_to (to_add,          "output",
                        wt->render_node, "aux");

  wt->redo_stack = g_list_remove_link (wt->redo_stack, wt->redo_stack);

  ligma_warp_tool_update_bounds (wt);
  ligma_warp_tool_update_stroke (wt, to_add);

  return TRUE;
}

static void
ligma_warp_tool_options_notify (LigmaTool         *tool,
                               LigmaToolOptions  *options,
                               const GParamSpec *pspec)
{
  LigmaWarpTool    *wt         = LIGMA_WARP_TOOL (tool);
  LigmaWarpOptions *wt_options = LIGMA_WARP_OPTIONS (options);

  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "effect-size"))
    {
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));
      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
    }
  else if (! strcmp (pspec->name, "interpolation"))
    {
      ligma_warp_tool_set_sampler (wt, /* commit = */ FALSE);
    }
  else if (! strcmp (pspec->name, "abyss-policy"))
    {
      if (wt->render_node)
        {
          gegl_node_set (wt->render_node,
                         "abyss-policy", wt_options->abyss_policy,
                         NULL);

          ligma_warp_tool_update_stroke (wt, NULL);
        }
    }
  else if (! strcmp (pspec->name, "high-quality-preview"))
    {
      ligma_warp_tool_set_sampler (wt, /* commit = */ FALSE);
    }
}

static void
ligma_warp_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaWarpTool    *wt      = LIGMA_WARP_TOOL (draw_tool);
  LigmaWarpOptions *options = LIGMA_WARP_TOOL_GET_OPTIONS (wt);
  gdouble          x, y;

  if (wt->snap_brush)
    {
      x = wt->last_pos.x;
      y = wt->last_pos.y;
    }
  else
    {
      x = wt->cursor_pos.x;
      y = wt->cursor_pos.y;
    }

  if (wt->draw_brush)
    {
      ligma_draw_tool_add_arc (draw_tool,
                              FALSE,
                              x - options->effect_size * 0.5,
                              y - options->effect_size * 0.5,
                              options->effect_size,
                              options->effect_size,
                              0.0, 2.0 * G_PI);
    }
  else if (! wt->show_cursor)
    {
      /*  don't leave the user without any indication and draw
       *  a fallback crosshair
       */
      ligma_draw_tool_add_handle (draw_tool,
                                 LIGMA_HANDLE_CROSSHAIR,
                                 x, y,
                                 LIGMA_TOOL_HANDLE_SIZE_CROSSHAIR,
                                 LIGMA_TOOL_HANDLE_SIZE_CROSSHAIR,
                                 LIGMA_HANDLE_ANCHOR_CENTER);
    }
}

static void
ligma_warp_tool_cursor_notify (LigmaDisplayConfig *config,
                              GParamSpec        *pspec,
                              LigmaWarpTool      *wt)
{
  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (wt));

  wt->show_cursor = config->show_paint_tool_cursor;
  wt->draw_brush  = config->show_brush_outline;
  wt->snap_brush  = config->snap_brush_outline;

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (wt));
}

static gboolean
ligma_warp_tool_can_stroke (LigmaWarpTool *wt,
                           LigmaDisplay  *display,
                           gboolean      show_message)
{
  LigmaTool        *tool        = LIGMA_TOOL (wt);
  LigmaWarpOptions *options     = LIGMA_WARP_TOOL_GET_OPTIONS (wt);
  LigmaGuiConfig   *config      = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaImage       *image       = ligma_display_get_image (display);
  LigmaItem        *locked_item = NULL;
  GList           *drawables   = ligma_image_get_selected_drawables (image);
  LigmaDrawable    *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (show_message)
        {
          if (g_list_length (drawables) > 1)
            ligma_tool_message_literal (tool, display,
                                       _("Cannot warp multiple layers. Select only one layer."));
          else
            ligma_tool_message_literal (tool, display,
                                       _("No active drawables."));
        }

      g_list_free (drawables);

      return FALSE;
    }

  drawable = drawables->data;
  g_list_free (drawables);

  if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable)))
    {
      if (show_message)
        {
          ligma_tool_message_literal (tool, display,
                                     _("Cannot warp layer groups."));
        }

      return FALSE;
    }

  if (ligma_item_is_content_locked (LIGMA_ITEM (drawable), &locked_item))
    {
      if (show_message)
        {
          ligma_tool_message_literal (tool, display,
                                     _("The selected item's pixels are locked."));

          ligma_tools_blink_lock_box (display->ligma, locked_item);
        }

      return FALSE;
    }

  if (! ligma_item_is_visible (LIGMA_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      if (show_message)
        {
          ligma_tool_message_literal (tool, display,
                                     _("The selected item is not visible."));
        }

      return FALSE;
    }

  if (! options->stroke_during_motion &&
      ! options->stroke_periodically)
    {
      if (show_message)
        {
          ligma_tool_message_literal (tool, display,
                                     _("No stroke events selected."));

          ligma_tools_show_tool_options (display->ligma);
          ligma_widget_blink (options->stroke_frame);
        }

      return FALSE;
    }

  if (! wt->filter || ! ligma_tool_can_undo (tool, display))
    {
      const gchar *message = NULL;

      switch (options->behavior)
        {
        case LIGMA_WARP_BEHAVIOR_MOVE:
        case LIGMA_WARP_BEHAVIOR_GROW:
        case LIGMA_WARP_BEHAVIOR_SHRINK:
        case LIGMA_WARP_BEHAVIOR_SWIRL_CW:
        case LIGMA_WARP_BEHAVIOR_SWIRL_CCW:
          break;

        case LIGMA_WARP_BEHAVIOR_ERASE:
          message = _("No warp to erase.");
          break;

        case LIGMA_WARP_BEHAVIOR_SMOOTH:
          message = _("No warp to smooth.");
          break;
        }

      if (message)
        {
          if (show_message)
            {
              ligma_tool_message_literal (tool, display, message);

              ligma_tools_show_tool_options (display->ligma);
              ligma_widget_blink (options->behavior_combo);
            }

          return FALSE;
        }
    }

  return TRUE;
}

static gboolean
ligma_warp_tool_start (LigmaWarpTool *wt,
                      LigmaDisplay  *display)
{
  LigmaTool        *tool     = LIGMA_TOOL (wt);
  LigmaWarpOptions *options  = LIGMA_WARP_TOOL_GET_OPTIONS (wt);
  LigmaImage       *image    = ligma_display_get_image (display);
  LigmaDrawable    *drawable;
  const Babl      *format;
  GeglRectangle    bbox;

  if (! ligma_warp_tool_can_stroke (wt, display, TRUE))
    return FALSE;

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = ligma_image_get_selected_drawables (image);

  drawable = tool->drawables->data;

  /* Create the coords buffer, with the size of the selection */
  format = babl_format_n (babl_type ("float"), 2);

  ligma_item_mask_intersect (LIGMA_ITEM (drawable), &bbox.x, &bbox.y,
                            &bbox.width, &bbox.height);

#ifdef WARP_DEBUG
  g_printerr ("Initialize coordinate buffer (%d,%d) at %d,%d\n",
              bbox.width, bbox.height, bbox.x, bbox.y);
#endif

  wt->coords_buffer = gegl_buffer_new (&bbox, format);

  ligma_warp_tool_create_filter (wt, drawable);

  if (! ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (wt)))
    ligma_draw_tool_start (LIGMA_DRAW_TOOL (wt), display);

  if (options->animate_button)
    {
      g_signal_connect_swapped (options->animate_button, "clicked",
                                G_CALLBACK (ligma_warp_tool_animate),
                                wt);

      gtk_widget_set_sensitive (options->animate_button, TRUE);
    }

  return TRUE;
}

static void
ligma_warp_tool_halt (LigmaWarpTool *wt)
{
  LigmaTool        *tool    = LIGMA_TOOL (wt);
  LigmaWarpOptions *options = LIGMA_WARP_TOOL_GET_OPTIONS (wt);

  g_clear_object (&wt->coords_buffer);

  g_clear_object (&wt->graph);
  wt->render_node = NULL;

  if (wt->filter)
    {
      ligma_drawable_filter_abort (wt->filter);
      g_clear_object (&wt->filter);

      ligma_image_flush (ligma_display_get_image (tool->display));
    }

  if (wt->redo_stack)
    {
      g_list_free (wt->redo_stack);
      wt->redo_stack = NULL;
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (wt)))
    ligma_draw_tool_stop (LIGMA_DRAW_TOOL (wt));

  if (options->animate_button)
    {
      gtk_widget_set_sensitive (options->animate_button, FALSE);

      g_signal_handlers_disconnect_by_func (options->animate_button,
                                            ligma_warp_tool_animate,
                                            wt);
    }
}

static void
ligma_warp_tool_commit (LigmaWarpTool *wt)
{
  LigmaTool *tool = LIGMA_TOOL (wt);

  /* don't commit a nop */
  if (tool->display && ligma_tool_can_undo (tool, tool->display))
    {
      ligma_tool_control_push_preserve (tool->control, TRUE);

      ligma_warp_tool_set_sampler (wt, /* commit = */ TRUE);

      ligma_drawable_filter_commit (wt->filter, LIGMA_PROGRESS (tool), FALSE);
      g_clear_object (&wt->filter);

      ligma_tool_control_pop_preserve (tool->control);

      ligma_image_flush (ligma_display_get_image (tool->display));
    }
}

static void
ligma_warp_tool_start_stroke_timer (LigmaWarpTool *wt)
{
  LigmaWarpOptions *options = LIGMA_WARP_TOOL_GET_OPTIONS (wt);

  ligma_warp_tool_stop_stroke_timer (wt);

  if (options->stroke_periodically                    &&
      options->stroke_periodically_rate > 0.0         &&
      ! (options->behavior == LIGMA_WARP_BEHAVIOR_MOVE &&
         options->stroke_during_motion))
    {
      gdouble fps;

      fps = STROKE_TIMER_MAX_FPS * options->stroke_periodically_rate / 100.0;

      wt->stroke_timer = g_timeout_add (1000.0 / fps,
                                        (GSourceFunc) ligma_warp_tool_stroke_timer,
                                        wt);
    }
}

static void
ligma_warp_tool_stop_stroke_timer (LigmaWarpTool *wt)
{
  if (wt->stroke_timer)
    g_source_remove (wt->stroke_timer);

  wt->stroke_timer = 0;
}

static gboolean
ligma_warp_tool_stroke_timer (LigmaWarpTool *wt)
{
  LigmaTool *tool = LIGMA_TOOL (wt);
  gint      off_x, off_y;

  g_return_val_if_fail (g_list_length (tool->drawables) == 1, FALSE);

  ligma_item_get_offset (LIGMA_ITEM (tool->drawables->data), &off_x, &off_y);

  ligma_warp_tool_stroke_append (wt,
                                'L', wt->last_pos.x - off_x,
                                     wt->last_pos.y - off_y);

  return TRUE;
}

static void
ligma_warp_tool_create_graph (LigmaWarpTool *wt)
{
  LigmaWarpOptions *options = LIGMA_WARP_TOOL_GET_OPTIONS (wt);
  GeglNode        *graph;           /* Wrapper to be returned */
  GeglNode        *input, *output;  /* Proxy nodes */
  GeglNode        *coords, *render; /* Render nodes */

  /* render_node is not supposed to be recreated */
  g_return_if_fail (wt->graph == NULL);

  graph = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (graph, "input");
  output = gegl_node_get_output_proxy (graph, "output");

  coords = gegl_node_new_child (graph,
                               "operation", "gegl:buffer-source",
                               "buffer",    wt->coords_buffer,
                               NULL);

  render = gegl_node_new_child (graph,
                                "operation",    "gegl:map-relative",
                                "abyss-policy", options->abyss_policy,
                                NULL);

  gegl_node_connect_to (input,  "output",
                        render, "input");

  gegl_node_connect_to (coords, "output",
                        render, "aux");

  gegl_node_connect_to (render, "output",
                        output, "input");

  wt->graph       = graph;
  wt->render_node = render;
}

static void
ligma_warp_tool_create_filter (LigmaWarpTool *wt,
                              LigmaDrawable *drawable)
{
  if (! wt->graph)
    ligma_warp_tool_create_graph (wt);

  ligma_warp_tool_set_sampler (wt, /* commit = */ FALSE);

  wt->filter = ligma_drawable_filter_new (drawable,
                                         _("Warp transform"),
                                         wt->graph,
                                         LIGMA_ICON_TOOL_WARP);

  ligma_drawable_filter_set_region (wt->filter, LIGMA_FILTER_REGION_DRAWABLE);

#if 0
  g_object_set (wt->filter, "gegl-caching", TRUE, NULL);
#endif

  g_signal_connect (wt->filter, "flush",
                    G_CALLBACK (ligma_warp_tool_filter_flush),
                    wt);
}

static void
ligma_warp_tool_set_sampler (LigmaWarpTool *wt,
                            gboolean      commit)
{
  LigmaWarpOptions *options = LIGMA_WARP_TOOL_GET_OPTIONS (wt);
  GeglSamplerType  sampler;
  GeglSamplerType  old_sampler;

  if (! wt->render_node)
    return;

  if (commit || options->high_quality_preview)
    sampler = (GeglSamplerType) options->interpolation;
  else
    sampler = PREVIEW_SAMPLER;

  gegl_node_get (wt->render_node,
                 "sampler-type", &old_sampler,
                 NULL);

  if (sampler != old_sampler)
    {
      gegl_node_set (wt->render_node,
                     "sampler-type", sampler,
                     NULL);

      ligma_warp_tool_update_bounds (wt);
      ligma_warp_tool_update_stroke (wt, NULL);
    }
}

static GeglRectangle
ligma_warp_tool_get_stroke_bounds (GeglNode *node)
{
  GeglRectangle  bbox = {0, 0, 0, 0};
  GeglPath      *stroke;
  gdouble        size;

  gegl_node_get (node,
                 "stroke", &stroke,
                 "size",   &size,
                 NULL);

  if (stroke)
    {
      gdouble        min_x;
      gdouble        max_x;
      gdouble        min_y;
      gdouble        max_y;

      gegl_path_get_bounds (stroke, &min_x, &max_x, &min_y, &max_y);
      g_object_unref (stroke);

      bbox.x      = floor (min_x - size * 0.5);
      bbox.y      = floor (min_y - size * 0.5);
      bbox.width  = ceil (max_x + size * 0.5) - bbox.x;
      bbox.height = ceil (max_y + size * 0.5) - bbox.y;
    }

  return bbox;
}

static GeglRectangle
ligma_warp_tool_get_node_bounds (GeglNode *node)
{
  GeglRectangle *bounds;

  if (! node || strcmp (gegl_node_get_operation (node), "gegl:warp"))
    return *GEGL_RECTANGLE (0, 0, 0, 0);

  bounds = g_object_get_data (G_OBJECT (node), "ligma-warp-tool-bounds");

  if (! bounds)
    {
      GeglNode      *input_node;
      GeglRectangle  input_bounds;
      GeglRectangle  stroke_bounds;

      input_node   = gegl_node_get_producer (node, "input", NULL);
      input_bounds = ligma_warp_tool_get_node_bounds (input_node);

      stroke_bounds = ligma_warp_tool_get_stroke_bounds (node);

      gegl_rectangle_bounding_box (&input_bounds,
                                   &input_bounds, &stroke_bounds);

      bounds = gegl_rectangle_dup (&input_bounds);

      g_object_set_data_full (G_OBJECT (node), "ligma-warp-tool-bounds",
                              bounds, g_free);
    }

  return *bounds;
}

static void
ligma_warp_tool_clear_node_bounds (GeglNode *node)
{
  if (node && ! strcmp (gegl_node_get_operation (node), "gegl:warp"))
    g_object_set_data (G_OBJECT (node), "ligma-warp-tool-bounds", NULL);
}

static GeglRectangle
ligma_warp_tool_get_invalidated_by_change (LigmaWarpTool        *wt,
                                          const GeglRectangle *area)
{
  GeglRectangle result = *area;

  if (! wt->filter)
    return result;

  if (wt->render_node)
    {
      GeglOperation *operation = gegl_node_get_gegl_operation (wt->render_node);

      result = gegl_operation_get_invalidated_by_change (operation,
                                                         "aux", area);
    }

  return result;
}

static void
ligma_warp_tool_update_bounds (LigmaWarpTool *wt)
{
  GeglRectangle bounds = {0, 0, 0, 0};

  if (! wt->filter)
    return;

  if (wt->render_node)
    {
      GeglNode *node = gegl_node_get_producer (wt->render_node, "aux", NULL);

      bounds = ligma_warp_tool_get_node_bounds (node);

      bounds = ligma_warp_tool_get_invalidated_by_change (wt, &bounds);
    }

  ligma_drawable_filter_set_crop (wt->filter, &bounds, FALSE);
}

static void
ligma_warp_tool_update_area (LigmaWarpTool        *wt,
                            const GeglRectangle *area,
                            gboolean             synchronous)
{
  GeglRectangle rect;

  if (! wt->filter)
    return;

  rect = ligma_warp_tool_get_invalidated_by_change (wt, area);

  if (synchronous)
    {
      LigmaTool  *tool  = LIGMA_TOOL (wt);
      LigmaImage *image = ligma_display_get_image (tool->display);

      g_signal_handlers_block_by_func (wt->filter,
                                       ligma_warp_tool_filter_flush,
                                       wt);

      ligma_drawable_filter_apply (wt->filter, &rect);

      ligma_projection_flush_now (ligma_image_get_projection (image), TRUE);
      ligma_display_flush_now (tool->display);

      g_signal_handlers_unblock_by_func (wt->filter,
                                         ligma_warp_tool_filter_flush,
                                         wt);
    }
  else
    {
      ligma_drawable_filter_apply (wt->filter, &rect);
    }
}

static void
ligma_warp_tool_update_stroke (LigmaWarpTool *wt,
                              GeglNode     *node)
{
  GeglRectangle bounds = {0, 0, 0, 0};

  if (! wt->filter)
    return;

  if (node)
    {
      /* update just this stroke */
      bounds = ligma_warp_tool_get_stroke_bounds (node);
    }
  else if (wt->render_node)
    {
      node = gegl_node_get_producer (wt->render_node, "aux", NULL);

      bounds = ligma_warp_tool_get_node_bounds (node);
    }

  if (! gegl_rectangle_is_empty (&bounds))
    {
#ifdef WARP_DEBUG
  g_printerr ("update stroke: (%d,%d), %dx%d\n",
              bounds.x, bounds.y,
              bounds.width, bounds.height);
#endif

      ligma_warp_tool_update_area (wt, &bounds, FALSE);
    }
}

static void
ligma_warp_tool_stroke_append (LigmaWarpTool *wt,
                              gchar         type,
                              gdouble       x,
                              gdouble       y)
{
  LigmaWarpOptions *options = LIGMA_WARP_TOOL_GET_OPTIONS (wt);
  GeglRectangle    area;

  if (! wt->filter)
    return;

  gegl_path_append (wt->current_stroke, type, x, y);

  area.x      = floor (x - options->effect_size * 0.5);
  area.y      = floor (y - options->effect_size * 0.5);
  area.width  = ceil  (x + options->effect_size * 0.5) - area.x;
  area.height = ceil  (y + options->effect_size * 0.5) - area.y;

#ifdef WARP_DEBUG
  g_printerr ("update rect: (%d,%d), %dx%d\n",
              area.x, area.y,
              area.width, area.height);
#endif

  if (wt->render_node)
    {
      GeglNode *node = gegl_node_get_producer (wt->render_node, "aux", NULL);

      ligma_warp_tool_clear_node_bounds (node);

      ligma_warp_tool_update_bounds (wt);
    }

  ligma_warp_tool_update_area (wt, &area, options->real_time_preview);
}

static void
ligma_warp_tool_filter_flush (LigmaDrawableFilter *filter,
                             LigmaTool           *tool)
{
  LigmaImage *image = ligma_display_get_image (tool->display);

  ligma_projection_flush (ligma_image_get_projection (image));
}

static void
ligma_warp_tool_add_op (LigmaWarpTool *wt,
                       GeglNode     *op)
{
  GeglNode *last_op;

  g_return_if_fail (GEGL_IS_NODE (wt->render_node));

  gegl_node_add_child (wt->graph, op);

  last_op = gegl_node_get_producer (wt->render_node, "aux", NULL);

  gegl_node_disconnect (wt->render_node, "aux");
  gegl_node_connect_to (last_op,         "output",
                        op    ,          "input");
  gegl_node_connect_to (op,              "output",
                        wt->render_node, "aux");
}

static void
ligma_warp_tool_remove_op (LigmaWarpTool *wt,
                          GeglNode     *op)
{
  GeglNode *previous;

  g_return_if_fail (GEGL_IS_NODE (wt->render_node));

  previous = gegl_node_get_producer (op, "input", NULL);

  gegl_node_disconnect (op,              "input");
  gegl_node_connect_to (previous,        "output",
                        wt->render_node, "aux");

  gegl_node_remove_child (wt->graph, op);
}

static void
ligma_warp_tool_free_op (GeglNode *op)
{
  GeglNode *parent;

  parent = gegl_node_get_parent (op);

  ligma_assert (parent != NULL);

  gegl_node_remove_child (parent, op);
}

static void
ligma_warp_tool_animate (LigmaWarpTool *wt)
{
  LigmaTool        *tool    = LIGMA_TOOL (wt);
  LigmaWarpOptions *options = LIGMA_WARP_TOOL_GET_OPTIONS (wt);
  LigmaImage       *orig_image;
  LigmaImage       *image;
  LigmaLayer       *layer;
  LigmaLayer       *first_layer;
  GeglNode        *scale_node;
  LigmaProgress    *progress;
  GtkWidget       *widget;
  gint             i;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (! ligma_warp_tool_can_undo (tool, tool->display))
    {
      ligma_tool_message_literal (tool, tool->display,
                                 _("Please add some warp strokes first."));
      return;
    }

  /*  get rid of the image map so we can use wt->graph  */
  if (wt->filter)
    {
      ligma_drawable_filter_abort (wt->filter);
      g_clear_object (&wt->filter);
    }

  ligma_warp_tool_set_sampler (wt, /* commit = */ TRUE);

  ligma_progress_start (LIGMA_PROGRESS (tool), FALSE,
                       _("Rendering Frame %d"), 1);

  orig_image = ligma_item_get_image (LIGMA_ITEM (tool->drawables->data));

  image = ligma_create_image (orig_image->ligma,
                             ligma_item_get_width  (LIGMA_ITEM (tool->drawables->data)),
                             ligma_item_get_height (LIGMA_ITEM (tool->drawables->data)),
                             ligma_drawable_get_base_type (tool->drawables->data),
                             ligma_drawable_get_precision (tool->drawables->data),
                             TRUE);

  /*  the first frame is always the unwarped image  */
  layer = LIGMA_LAYER (ligma_item_convert (LIGMA_ITEM (tool->drawables->data), image,
                                         LIGMA_TYPE_LAYER));
  ligma_object_take_name (LIGMA_OBJECT (layer),
                         g_strdup_printf (_("Frame %d"), 1));

  ligma_item_set_offset (LIGMA_ITEM (layer), 0, 0);
  ligma_item_set_visible (LIGMA_ITEM (layer), TRUE, FALSE);
  ligma_layer_set_mode (layer, ligma_image_get_default_new_layer_mode (image),
                       FALSE);
  ligma_layer_set_opacity (layer, LIGMA_OPACITY_OPAQUE, FALSE);
  ligma_image_add_layer (image, layer, NULL, 0, FALSE);

  first_layer = layer;

  scale_node = gegl_node_new_child (NULL,
                                    "operation",    "ligma:scalar-multiply",
                                    "n-components", 2,
                                    NULL);
  ligma_warp_tool_add_op (wt, scale_node);

  progress = ligma_sub_progress_new (LIGMA_PROGRESS (tool));

  for (i = 1; i < options->n_animation_frames; i++)
    {
      ligma_progress_set_text (LIGMA_PROGRESS (tool),
                              _("Rendering Frame %d"), i + 1);

      ligma_sub_progress_set_step (LIGMA_SUB_PROGRESS (progress),
                                  i, options->n_animation_frames);

      layer = LIGMA_LAYER (ligma_item_duplicate (LIGMA_ITEM (first_layer),
                                               LIGMA_TYPE_LAYER));
      ligma_object_take_name (LIGMA_OBJECT (layer),
                             g_strdup_printf (_("Frame %d"), i + 1));

      gegl_node_set (scale_node,
                     "factor", (gdouble) i /
                               (gdouble) (options->n_animation_frames - 1),
                     NULL);

      ligma_gegl_apply_operation (ligma_drawable_get_buffer (LIGMA_DRAWABLE (first_layer)),
                                 progress,
                                 _("Frame"),
                                 wt->graph,
                                 ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer)),
                                 NULL, FALSE);

      ligma_image_add_layer (image, layer, NULL, 0, FALSE);
    }

  g_object_unref (progress);

  ligma_warp_tool_remove_op (wt, scale_node);

  ligma_progress_end (LIGMA_PROGRESS (tool));

  /*  recreate the image map  */
  ligma_warp_tool_create_filter (wt, tool->drawables->data);
  ligma_warp_tool_update_stroke (wt, NULL);

  widget = GTK_WIDGET (ligma_display_get_shell (tool->display));
  ligma_create_display (orig_image->ligma, image, LIGMA_UNIT_PIXEL, 1.0,
                       G_OBJECT (ligma_widget_get_monitor (widget)));
  g_object_unref (image);
}
