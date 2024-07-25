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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gegl-plugin.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"
#include "config/gimpguiconfig.h"

#include "gegl/gimp-gegl-apply-operation.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable-filters.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"
#include "core/gimpsubprogress.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpwarptool.h"
#include "gimpwarpoptions.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"

#include "gimp-intl.h"


#define STROKE_TIMER_MAX_FPS 20
#define PREVIEW_SAMPLER      GEGL_SAMPLER_NEAREST


static void            gimp_warp_tool_constructed               (GObject               *object);

static void            gimp_warp_tool_control                   (GimpTool              *tool,
                                                                 GimpToolAction         action,
                                                                 GimpDisplay           *display);
static void            gimp_warp_tool_button_press              (GimpTool              *tool,
                                                                 const GimpCoords      *coords,
                                                                 guint32                time,
                                                                 GdkModifierType        state,
                                                                 GimpButtonPressType    press_type,
                                                                 GimpDisplay           *display);
static void            gimp_warp_tool_button_release            (GimpTool              *tool,
                                                                 const GimpCoords      *coords,
                                                                 guint32                time,
                                                                 GdkModifierType        state,
                                                                 GimpButtonReleaseType  release_type,
                                                                 GimpDisplay           *display);
static void            gimp_warp_tool_motion                    (GimpTool              *tool,
                                                                 const GimpCoords      *coords,
                                                                 guint32                time,
                                                                 GdkModifierType        state,
                                                                 GimpDisplay           *display);
static gboolean        gimp_warp_tool_key_press                 (GimpTool              *tool,
                                                                 GdkEventKey           *kevent,
                                                                 GimpDisplay           *display);
static void            gimp_warp_tool_oper_update               (GimpTool              *tool,
                                                                 const GimpCoords      *coords,
                                                                 GdkModifierType        state,
                                                                 gboolean               proximity,
                                                                 GimpDisplay           *display);
static void            gimp_warp_tool_cursor_update             (GimpTool              *tool,
                                                                 const GimpCoords      *coords,
                                                                 GdkModifierType        state,
                                                                 GimpDisplay           *display);
const gchar          * gimp_warp_tool_can_undo                  (GimpTool              *tool,
                                                                 GimpDisplay           *display);
const gchar          * gimp_warp_tool_can_redo                  (GimpTool              *tool,
                                                                 GimpDisplay           *display);
static gboolean        gimp_warp_tool_undo                      (GimpTool              *tool,
                                                                 GimpDisplay           *display);
static gboolean        gimp_warp_tool_redo                      (GimpTool              *tool,
                                                                 GimpDisplay           *display);
static void            gimp_warp_tool_options_notify            (GimpTool              *tool,
                                                                 GimpToolOptions       *options,
                                                                 const GParamSpec      *pspec);

static void            gimp_warp_tool_draw                      (GimpDrawTool          *draw_tool);

static void            gimp_warp_tool_cursor_notify             (GimpDisplayConfig     *config,
                                                                 GParamSpec            *pspec,
                                                                 GimpWarpTool          *wt);

static gboolean        gimp_warp_tool_can_stroke                (GimpWarpTool          *wt,
                                                                 GimpDisplay           *display,
                                                                 gboolean               show_message);

static gboolean        gimp_warp_tool_start                     (GimpWarpTool          *wt,
                                                                 GimpDisplay           *display);
static void            gimp_warp_tool_halt                      (GimpWarpTool          *wt);
static void            gimp_warp_tool_commit                    (GimpWarpTool          *wt);

static void            gimp_warp_tool_start_stroke_timer        (GimpWarpTool          *wt);
static void            gimp_warp_tool_stop_stroke_timer         (GimpWarpTool          *wt);
static gboolean        gimp_warp_tool_stroke_timer              (GimpWarpTool          *wt);

static void            gimp_warp_tool_create_graph              (GimpWarpTool          *wt);
static void            gimp_warp_tool_create_filter             (GimpWarpTool          *wt,
                                                                 GimpDrawable          *drawable);
static void            gimp_warp_tool_set_sampler               (GimpWarpTool          *wt,
                                                                 gboolean               commit);
static GeglRectangle
                       gimp_warp_tool_get_stroke_bounds         (GeglNode              *node);
static GeglRectangle   gimp_warp_tool_get_node_bounds           (GeglNode              *node);
static void            gimp_warp_tool_clear_node_bounds         (GeglNode              *node);
static GeglRectangle   gimp_warp_tool_get_invalidated_by_change (GimpWarpTool          *wt,
                                                                 const GeglRectangle   *area);
static void            gimp_warp_tool_update_bounds             (GimpWarpTool          *wt);
static void            gimp_warp_tool_update_area               (GimpWarpTool          *wt,
                                                                 const GeglRectangle   *area,
                                                                 gboolean               synchronous);
static void            gimp_warp_tool_update_stroke             (GimpWarpTool          *wt,
                                                                 GeglNode              *node);
static void            gimp_warp_tool_stroke_append             (GimpWarpTool          *wt,
                                                                 gchar                  type,
                                                                 gdouble                x,
                                                                 gdouble                y);
static void            gimp_warp_tool_filter_flush              (GimpDrawableFilter    *filter,
                                                                 GimpTool              *tool);
static void            gimp_warp_tool_add_op                    (GimpWarpTool          *wt,
                                                                 GeglNode              *op);
static void            gimp_warp_tool_remove_op                 (GimpWarpTool          *wt,
                                                                 GeglNode              *op);
static void            gimp_warp_tool_free_op                   (GeglNode              *op);

static void            gimp_warp_tool_animate                   (GimpWarpTool          *wt);


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
                GIMP_ICON_TOOL_WARP,
                data);
}

static void
gimp_warp_tool_class_init (GimpWarpToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = gimp_warp_tool_constructed;

  tool_class->control        = gimp_warp_tool_control;
  tool_class->button_press   = gimp_warp_tool_button_press;
  tool_class->button_release = gimp_warp_tool_button_release;
  tool_class->motion         = gimp_warp_tool_motion;
  tool_class->key_press      = gimp_warp_tool_key_press;
  tool_class->oper_update    = gimp_warp_tool_oper_update;
  tool_class->cursor_update  = gimp_warp_tool_cursor_update;
  tool_class->can_undo       = gimp_warp_tool_can_undo;
  tool_class->can_redo       = gimp_warp_tool_can_redo;
  tool_class->undo           = gimp_warp_tool_undo;
  tool_class->redo           = gimp_warp_tool_redo;
  tool_class->options_notify = gimp_warp_tool_options_notify;

  draw_tool_class->draw      = gimp_warp_tool_draw;
}

static void
gimp_warp_tool_init (GimpWarpTool *self)
{
  GimpTool *tool = GIMP_TOOL (self);

  gimp_tool_control_set_motion_mode     (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_scroll_lock     (tool->control, TRUE);
  gimp_tool_control_set_preserve        (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask      (tool->control,
                                         GIMP_DIRTY_IMAGE           |
                                         GIMP_DIRTY_DRAWABLE        |
                                         GIMP_DIRTY_SELECTION       |
                                         GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_dirty_action    (tool->control,
                                         GIMP_TOOL_ACTION_COMMIT);
  gimp_tool_control_set_wants_click     (tool->control, TRUE);
  gimp_tool_control_set_precision       (tool->control,
                                         GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_WARP);

  gimp_tool_control_set_action_pixel_size (tool->control,
                                           "tools-warp-effect-pixel-size-set");
  gimp_tool_control_set_action_size       (tool->control,
                                           "tools-warp-effect-size-set");
  gimp_tool_control_set_action_hardness   (tool->control,
                                           "tools-warp-effect-hardness-set");

  self->show_cursor = TRUE;
  self->draw_brush  = TRUE;
  self->snap_brush  = FALSE;
}

static void
gimp_warp_tool_constructed (GObject *object)
{
  GimpWarpTool      *wt   = GIMP_WARP_TOOL (object);
  GimpTool          *tool = GIMP_TOOL (object);
  GimpDisplayConfig *display_config;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  display_config = GIMP_DISPLAY_CONFIG (tool->tool_info->gimp->config);

  wt->show_cursor = display_config->show_paint_tool_cursor;
  wt->draw_brush  = display_config->show_brush_outline;
  wt->snap_brush  = display_config->snap_brush_outline;

  g_signal_connect_object (display_config, "notify::show-paint-tool-cursor",
                           G_CALLBACK (gimp_warp_tool_cursor_notify),
                           wt, 0);
  g_signal_connect_object (display_config, "notify::show-brush-outline",
                           G_CALLBACK (gimp_warp_tool_cursor_notify),
                           wt, 0);
  g_signal_connect_object (display_config, "notify::snap-brush-outline",
                           G_CALLBACK (gimp_warp_tool_cursor_notify),
                           wt, 0);
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
      gimp_warp_tool_halt (wt);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_warp_tool_commit (wt);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_warp_tool_button_press (GimpTool            *tool,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type,
                             GimpDisplay         *display)
{
  GimpWarpTool    *wt      = GIMP_WARP_TOOL (tool);
  GimpWarpOptions *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);
  GeglNode        *new_op;
  gint             off_x, off_y;

  if (tool->display && display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      if (! gimp_warp_tool_start (wt, display))
        return;
    }

  if (! gimp_warp_tool_can_stroke (wt, display, TRUE))
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

  gimp_warp_tool_add_op (wt, new_op);
  g_object_unref (new_op);

  gimp_item_get_offset (GIMP_ITEM (tool->drawables->data), &off_x, &off_y);

  gimp_warp_tool_stroke_append (wt,
                                'M', wt->last_pos.x - off_x,
                                     wt->last_pos.y - off_y);

  gimp_warp_tool_start_stroke_timer (wt);

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
  GimpWarpTool *wt = GIMP_WARP_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (wt));

  gimp_tool_control_halt (tool->control);

  gimp_warp_tool_stop_stroke_timer (wt);

#ifdef WARP_DEBUG
  g_printerr ("%s\n", gegl_path_to_string (wt->current_stroke));
#endif

  g_clear_object (&wt->current_stroke);

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      gimp_warp_tool_undo (tool, display);

      /*  the just undone stroke has no business on the redo stack  */
      gimp_warp_tool_free_op (wt->redo_stack->data);
      wt->redo_stack = g_list_remove_link (wt->redo_stack, wt->redo_stack);
    }
  else
    {
      if (wt->redo_stack)
        {
          /*  the redo stack becomes invalid by actually doing a stroke  */
          g_list_free_full (wt->redo_stack,
                            (GDestroyNotify) gimp_warp_tool_free_op);
          wt->redo_stack = NULL;
        }

      gimp_tool_push_status (tool, tool->display,
                             _("Press ENTER to commit the transform"));
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  /*  update the undo actions / menu items  */
  gimp_image_flush (gimp_display_get_image (GIMP_TOOL (wt)->display));
}

static void
gimp_warp_tool_motion (GimpTool         *tool,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       GimpDisplay      *display)
{
  GimpWarpTool    *wt             = GIMP_WARP_TOOL (tool);
  GimpWarpOptions *options        = GIMP_WARP_TOOL_GET_OPTIONS (wt);
  GimpVector2      old_cursor_pos;
  GimpVector2      delta;
  gdouble          dist;
  gdouble          step;
  gboolean         stroke_changed = FALSE;

  if (! wt->snap_brush)
    gimp_draw_tool_pause (GIMP_DRAW_TOOL (wt));

  old_cursor_pos = wt->cursor_pos;

  wt->cursor_pos.x = coords->x;
  wt->cursor_pos.y = coords->y;

  gimp_vector2_sub (&delta, &wt->cursor_pos, &old_cursor_pos);
  dist = gimp_vector2_length (&delta);

  step = options->effect_size * options->stroke_spacing / 100.0;

  while (wt->total_dist + dist >= step)
    {
      gdouble diff = step - wt->total_dist;

      gimp_vector2_mul (&delta, diff / dist);
      gimp_vector2_add (&old_cursor_pos, &old_cursor_pos, &delta);

      gimp_vector2_sub (&delta, &wt->cursor_pos, &old_cursor_pos);
      dist -= diff;

      wt->last_pos   = old_cursor_pos;
      wt->total_dist = 0.0;

      if (options->stroke_during_motion)
        {
          gint off_x, off_y;

          if (! stroke_changed)
            {
              stroke_changed = TRUE;

              gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));
            }

          gimp_item_get_offset (GIMP_ITEM (tool->drawables->data), &off_x, &off_y);

          gimp_warp_tool_stroke_append (wt,
                                        'L', wt->last_pos.x - off_x,
                                             wt->last_pos.y - off_y);
        }
    }

  wt->total_dist += dist;

  if (stroke_changed)
    {
      gimp_warp_tool_start_stroke_timer (wt);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  if (! wt->snap_brush)
    gimp_draw_tool_resume (GIMP_DRAW_TOOL (wt));
}

static gboolean
gimp_warp_tool_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      return TRUE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
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
gimp_warp_tool_oper_update (GimpTool         *tool,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            GimpDisplay      *display)
{
  GimpWarpTool *wt        = GIMP_WARP_TOOL (tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (proximity)
    {
      gimp_draw_tool_pause (draw_tool);

      if (! tool->display || display == tool->display)
        {
          wt->cursor_pos.x = coords->x;
          wt->cursor_pos.y = coords->y;

          wt->last_pos = wt->cursor_pos;
        }

      if (! gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_start (draw_tool, display);

      gimp_draw_tool_resume (draw_tool);
    }
  else if (gimp_draw_tool_is_active (draw_tool))
    {
      gimp_draw_tool_stop (draw_tool);
    }
}

static void
gimp_warp_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpWarpTool       *wt       = GIMP_WARP_TOOL (tool);
  GimpWarpOptions    *options  = GIMP_WARP_TOOL_GET_OPTIONS (tool);
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_NONE;

  if (! gimp_warp_tool_can_stroke (wt, display, FALSE))
    {
      modifier = GIMP_CURSOR_MODIFIER_BAD;
    }
  else if (display == tool->display)
    {
#if 0
      /* FIXME have better cursors  */

      switch (options->behavior)
        {
        case GIMP_WARP_BEHAVIOR_MOVE:
        case GIMP_WARP_BEHAVIOR_GROW:
        case GIMP_WARP_BEHAVIOR_SHRINK:
        case GIMP_WARP_BEHAVIOR_SWIRL_CW:
        case GIMP_WARP_BEHAVIOR_SWIRL_CCW:
        case GIMP_WARP_BEHAVIOR_ERASE:
        case GIMP_WARP_BEHAVIOR_SMOOTH:
          modifier = GIMP_CURSOR_MODIFIER_MOVE;
          break;
        }
#else
      (void) options;
#endif
    }

  if (! wt->show_cursor && modifier != GIMP_CURSOR_MODIFIER_BAD)
    {
      gimp_tool_set_cursor (tool, display,
                            GIMP_CURSOR_NONE,
                            GIMP_TOOL_CURSOR_NONE,
                            GIMP_CURSOR_MODIFIER_NONE);
      return;
    }

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

const gchar *
gimp_warp_tool_can_undo (GimpTool    *tool,
                         GimpDisplay *display)
{
  GimpWarpTool *wt = GIMP_WARP_TOOL (tool);
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
gimp_warp_tool_can_redo (GimpTool    *tool,
                         GimpDisplay *display)
{
  GimpWarpTool *wt = GIMP_WARP_TOOL (tool);

  if (! wt->render_node || ! wt->redo_stack)
    return NULL;

  return _("Warp Tool Stroke");
}

static gboolean
gimp_warp_tool_undo (GimpTool    *tool,
                     GimpDisplay *display)
{
  GimpWarpTool *wt = GIMP_WARP_TOOL (tool);
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

  gegl_node_connect (prev_node,       "output",
                     wt->render_node, "aux");

  gimp_warp_tool_update_bounds (wt);
  gimp_warp_tool_update_stroke (wt, to_delete);

  return TRUE;
}

static gboolean
gimp_warp_tool_redo (GimpTool    *tool,
                     GimpDisplay *display)
{
  GimpWarpTool *wt = GIMP_WARP_TOOL (tool);
  GeglNode     *to_add;

  to_add = wt->redo_stack->data;

  gegl_node_connect (to_add,          "output",
                     wt->render_node, "aux");

  wt->redo_stack = g_list_remove_link (wt->redo_stack, wt->redo_stack);

  gimp_warp_tool_update_bounds (wt);
  gimp_warp_tool_update_stroke (wt, to_add);

  return TRUE;
}

static void
gimp_warp_tool_options_notify (GimpTool         *tool,
                               GimpToolOptions  *options,
                               const GParamSpec *pspec)
{
  GimpWarpTool    *wt         = GIMP_WARP_TOOL (tool);
  GimpWarpOptions *wt_options = GIMP_WARP_OPTIONS (options);

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "effect-size"))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));
      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
  else if (! strcmp (pspec->name, "interpolation"))
    {
      gimp_warp_tool_set_sampler (wt, /* commit = */ FALSE);
    }
  else if (! strcmp (pspec->name, "abyss-policy"))
    {
      if (wt->render_node)
        {
          gegl_node_set (wt->render_node,
                         "abyss-policy", wt_options->abyss_policy,
                         NULL);

          gimp_warp_tool_update_stroke (wt, NULL);
        }
    }
  else if (! strcmp (pspec->name, "high-quality-preview"))
    {
      gimp_warp_tool_set_sampler (wt, /* commit = */ FALSE);
    }
}

static void
gimp_warp_tool_draw (GimpDrawTool *draw_tool)
{
  GimpWarpTool    *wt      = GIMP_WARP_TOOL (draw_tool);
  GimpWarpOptions *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);
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
      gimp_draw_tool_add_arc (draw_tool,
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
      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_CROSSHAIR,
                                 x, y,
                                 GIMP_TOOL_HANDLE_SIZE_CROSSHAIR,
                                 GIMP_TOOL_HANDLE_SIZE_CROSSHAIR,
                                 GIMP_HANDLE_ANCHOR_CENTER);
    }
}

static void
gimp_warp_tool_cursor_notify (GimpDisplayConfig *config,
                              GParamSpec        *pspec,
                              GimpWarpTool      *wt)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (wt));

  wt->show_cursor = config->show_paint_tool_cursor;
  wt->draw_brush  = config->show_brush_outline;
  wt->snap_brush  = config->snap_brush_outline;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (wt));
}

static gboolean
gimp_warp_tool_can_stroke (GimpWarpTool *wt,
                           GimpDisplay  *display,
                           gboolean      show_message)
{
  GimpTool        *tool        = GIMP_TOOL (wt);
  GimpWarpOptions *options     = GIMP_WARP_TOOL_GET_OPTIONS (wt);
  GimpGuiConfig   *config      = GIMP_GUI_CONFIG (display->gimp->config);
  GimpImage       *image       = gimp_display_get_image (display);
  GimpItem        *locked_item = NULL;
  GList           *drawables   = gimp_image_get_selected_drawables (image);
  GimpDrawable    *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (show_message)
        {
          if (g_list_length (drawables) > 1)
            gimp_tool_message_literal (tool, display,
                                       _("Cannot warp multiple layers. Select only one layer."));
          else
            gimp_tool_message_literal (tool, display,
                                       _("No active drawables."));
        }

      g_list_free (drawables);

      return FALSE;
    }

  drawable = drawables->data;
  g_list_free (drawables);

  if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
    {
      if (show_message)
        {
          gimp_tool_message_literal (tool, display,
                                     _("Cannot warp layer groups."));
        }

      return FALSE;
    }

  if (gimp_item_is_content_locked (GIMP_ITEM (drawable), &locked_item))
    {
      if (show_message)
        {
          gimp_tool_message_literal (tool, display,
                                     _("The selected item's pixels are locked."));

          gimp_tools_blink_lock_box (display->gimp, locked_item);
        }

      return FALSE;
    }

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      if (show_message)
        {
          gimp_tool_message_literal (tool, display,
                                     _("The selected item is not visible."));
        }

      return FALSE;
    }

  if (! options->stroke_during_motion &&
      ! options->stroke_periodically)
    {
      if (show_message)
        {
          gimp_tool_message_literal (tool, display,
                                     _("No stroke events selected."));

          gimp_tools_show_tool_options (display->gimp);
          gimp_widget_blink (options->stroke_frame);
        }

      return FALSE;
    }

  if (! wt->filter || ! gimp_tool_can_undo (tool, display))
    {
      const gchar *message = NULL;

      switch (options->behavior)
        {
        case GIMP_WARP_BEHAVIOR_MOVE:
        case GIMP_WARP_BEHAVIOR_GROW:
        case GIMP_WARP_BEHAVIOR_SHRINK:
        case GIMP_WARP_BEHAVIOR_SWIRL_CW:
        case GIMP_WARP_BEHAVIOR_SWIRL_CCW:
          break;

        case GIMP_WARP_BEHAVIOR_ERASE:
          message = _("No warp to erase.");
          break;

        case GIMP_WARP_BEHAVIOR_SMOOTH:
          message = _("No warp to smooth.");
          break;
        }

      if (message)
        {
          if (show_message)
            {
              gimp_tool_message_literal (tool, display, message);

              gimp_tools_show_tool_options (display->gimp);
              gimp_widget_blink (options->behavior_combo);
            }

          return FALSE;
        }
    }

  return TRUE;
}

static gboolean
gimp_warp_tool_start (GimpWarpTool *wt,
                      GimpDisplay  *display)
{
  GimpTool        *tool     = GIMP_TOOL (wt);
  GimpWarpOptions *options  = GIMP_WARP_TOOL_GET_OPTIONS (wt);
  GimpImage       *image    = gimp_display_get_image (display);
  GimpDrawable    *drawable;
  const Babl      *format;
  GeglRectangle    bbox;

  if (! gimp_warp_tool_can_stroke (wt, display, TRUE))
    return FALSE;

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = gimp_image_get_selected_drawables (image);

  drawable = tool->drawables->data;

  /* Create the coords buffer, with the size of the selection */
  format = babl_format_n (babl_type ("float"), 2);

  gimp_item_mask_intersect (GIMP_ITEM (drawable), &bbox.x, &bbox.y,
                            &bbox.width, &bbox.height);

#ifdef WARP_DEBUG
  g_printerr ("Initialize coordinate buffer (%d,%d) at %d,%d\n",
              bbox.width, bbox.height, bbox.x, bbox.y);
#endif

  wt->coords_buffer = gegl_buffer_new (&bbox, format);

  gimp_warp_tool_create_filter (wt, drawable);

  if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (wt)))
    gimp_draw_tool_start (GIMP_DRAW_TOOL (wt), display);

  if (options->animate_button)
    {
      g_signal_connect_swapped (options->animate_button, "clicked",
                                G_CALLBACK (gimp_warp_tool_animate),
                                wt);

      gtk_widget_set_sensitive (options->animate_button, TRUE);
    }

  return TRUE;
}

static void
gimp_warp_tool_halt (GimpWarpTool *wt)
{
  GimpTool        *tool    = GIMP_TOOL (wt);
  GimpWarpOptions *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);

  g_clear_object (&wt->coords_buffer);

  g_clear_object (&wt->graph);
  wt->render_node = NULL;

  if (wt->filter)
    {
      gimp_drawable_filter_abort (wt->filter);
      g_clear_object (&wt->filter);

      gimp_image_flush (gimp_display_get_image (tool->display));
    }

  if (wt->redo_stack)
    {
      g_list_free (wt->redo_stack);
      wt->redo_stack = NULL;
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (wt)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (wt));

  if (options->animate_button)
    {
      gtk_widget_set_sensitive (options->animate_button, FALSE);

      g_signal_handlers_disconnect_by_func (options->animate_button,
                                            gimp_warp_tool_animate,
                                            wt);
    }
}

static void
gimp_warp_tool_commit (GimpWarpTool *wt)
{
  GimpTool *tool = GIMP_TOOL (wt);

  /* don't commit a nop */
  if (tool->display && gimp_tool_can_undo (tool, tool->display))
    {
      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_warp_tool_set_sampler (wt, /* commit = */ TRUE);

      gimp_drawable_filter_commit (wt->filter, FALSE,
                                   GIMP_PROGRESS (tool), FALSE);
      g_clear_object (&wt->filter);

      gimp_tool_control_pop_preserve (tool->control);

      gimp_image_flush (gimp_display_get_image (tool->display));
    }
}

static void
gimp_warp_tool_start_stroke_timer (GimpWarpTool *wt)
{
  GimpWarpOptions *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);

  gimp_warp_tool_stop_stroke_timer (wt);

  if (options->stroke_periodically                    &&
      options->stroke_periodically_rate > 0.0         &&
      ! (options->behavior == GIMP_WARP_BEHAVIOR_MOVE &&
         options->stroke_during_motion))
    {
      gdouble fps;

      fps = STROKE_TIMER_MAX_FPS * options->stroke_periodically_rate / 100.0;

      wt->stroke_timer = g_timeout_add (1000.0 / fps,
                                        (GSourceFunc) gimp_warp_tool_stroke_timer,
                                        wt);
    }
}

static void
gimp_warp_tool_stop_stroke_timer (GimpWarpTool *wt)
{
  if (wt->stroke_timer)
    g_source_remove (wt->stroke_timer);

  wt->stroke_timer = 0;
}

static gboolean
gimp_warp_tool_stroke_timer (GimpWarpTool *wt)
{
  GimpTool *tool = GIMP_TOOL (wt);
  gint      off_x, off_y;

  g_return_val_if_fail (g_list_length (tool->drawables) == 1, FALSE);

  gimp_item_get_offset (GIMP_ITEM (tool->drawables->data), &off_x, &off_y);

  gimp_warp_tool_stroke_append (wt,
                                'L', wt->last_pos.x - off_x,
                                     wt->last_pos.y - off_y);

  return TRUE;
}

static void
gimp_warp_tool_create_graph (GimpWarpTool *wt)
{
  GimpWarpOptions *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);
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

  gegl_node_link_many (input, render, output, NULL);
  gegl_node_connect (coords, "output",
                     render, "aux");


  wt->graph       = graph;
  wt->render_node = render;
}

static void
gimp_warp_tool_create_filter (GimpWarpTool *wt,
                              GimpDrawable *drawable)
{
  if (! wt->graph)
    gimp_warp_tool_create_graph (wt);

  gimp_warp_tool_set_sampler (wt, /* commit = */ FALSE);

  wt->filter = gimp_drawable_filter_new (drawable,
                                         _("Warp transform"),
                                         wt->graph,
                                         GIMP_ICON_TOOL_WARP);

  gimp_drawable_filter_set_region (wt->filter, GIMP_FILTER_REGION_DRAWABLE);

#if 0
  g_object_set (wt->filter, "gegl-caching", TRUE, NULL);
#endif

  g_signal_connect (wt->filter, "flush",
                    G_CALLBACK (gimp_warp_tool_filter_flush),
                    wt);
}

static void
gimp_warp_tool_set_sampler (GimpWarpTool *wt,
                            gboolean      commit)
{
  GimpWarpOptions *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);
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

      gimp_warp_tool_update_bounds (wt);
      gimp_warp_tool_update_stroke (wt, NULL);
    }
}

static GeglRectangle
gimp_warp_tool_get_stroke_bounds (GeglNode *node)
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
gimp_warp_tool_get_node_bounds (GeglNode *node)
{
  GeglRectangle *bounds;

  if (! node || strcmp (gegl_node_get_operation (node), "gegl:warp"))
    return *GEGL_RECTANGLE (0, 0, 0, 0);

  bounds = g_object_get_data (G_OBJECT (node), "gimp-warp-tool-bounds");

  if (! bounds)
    {
      GeglNode      *input_node;
      GeglRectangle  input_bounds;
      GeglRectangle  stroke_bounds;

      input_node   = gegl_node_get_producer (node, "input", NULL);
      input_bounds = gimp_warp_tool_get_node_bounds (input_node);

      stroke_bounds = gimp_warp_tool_get_stroke_bounds (node);

      gegl_rectangle_bounding_box (&input_bounds,
                                   &input_bounds, &stroke_bounds);

      bounds = gegl_rectangle_dup (&input_bounds);

      g_object_set_data_full (G_OBJECT (node), "gimp-warp-tool-bounds",
                              bounds, g_free);
    }

  return *bounds;
}

static void
gimp_warp_tool_clear_node_bounds (GeglNode *node)
{
  if (node && ! strcmp (gegl_node_get_operation (node), "gegl:warp"))
    g_object_set_data (G_OBJECT (node), "gimp-warp-tool-bounds", NULL);
}

static GeglRectangle
gimp_warp_tool_get_invalidated_by_change (GimpWarpTool        *wt,
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
gimp_warp_tool_update_bounds (GimpWarpTool *wt)
{
  GeglRectangle bounds = {0, 0, 0, 0};

  if (! wt->filter)
    return;

  if (wt->render_node)
    {
      GeglNode *node = gegl_node_get_producer (wt->render_node, "aux", NULL);

      bounds = gimp_warp_tool_get_node_bounds (node);

      bounds = gimp_warp_tool_get_invalidated_by_change (wt, &bounds);
    }

  gimp_drawable_filter_set_crop (wt->filter, &bounds, FALSE);
}

static void
gimp_warp_tool_update_area (GimpWarpTool        *wt,
                            const GeglRectangle *area,
                            gboolean             synchronous)
{
  GeglRectangle  rect;
  GimpContainer *filters;

  if (! wt->filter)
    return;

  rect = gimp_warp_tool_get_invalidated_by_change (wt, area);

  /* Move this operation below any non-destructive filters that
   * may be active, so that it's directly affect the raw pixels. */
  filters =
    gimp_drawable_get_filters (gimp_drawable_filter_get_drawable (wt->filter));

  if (gimp_container_have (filters, GIMP_OBJECT (wt->filter)))
  {
    gint end_index = gimp_container_get_n_children (filters) - 1;
    gint index     = gimp_container_get_child_index (filters,
                                                     GIMP_OBJECT (wt->filter));

    if (end_index > 0 && index != end_index)
      gimp_container_reorder (filters, GIMP_OBJECT (wt->filter),
                              end_index);
  }

  if (synchronous)
    {
      GimpTool  *tool  = GIMP_TOOL (wt);
      GimpImage *image = gimp_display_get_image (tool->display);

      g_signal_handlers_block_by_func (wt->filter,
                                       gimp_warp_tool_filter_flush,
                                       wt);

      gimp_drawable_filter_apply (wt->filter, &rect);

      gimp_projection_flush_now (gimp_image_get_projection (image), TRUE);
      gimp_display_flush_now (tool->display);

      g_signal_handlers_unblock_by_func (wt->filter,
                                         gimp_warp_tool_filter_flush,
                                         wt);
    }
  else
    {
      gimp_drawable_filter_apply (wt->filter, &rect);
    }
}

static void
gimp_warp_tool_update_stroke (GimpWarpTool *wt,
                              GeglNode     *node)
{
  GeglRectangle bounds = {0, 0, 0, 0};

  if (! wt->filter)
    return;

  if (node)
    {
      /* update just this stroke */
      bounds = gimp_warp_tool_get_stroke_bounds (node);
    }
  else if (wt->render_node)
    {
      node = gegl_node_get_producer (wt->render_node, "aux", NULL);

      bounds = gimp_warp_tool_get_node_bounds (node);
    }

  if (! gegl_rectangle_is_empty (&bounds))
    {
#ifdef WARP_DEBUG
  g_printerr ("update stroke: (%d,%d), %dx%d\n",
              bounds.x, bounds.y,
              bounds.width, bounds.height);
#endif

      gimp_warp_tool_update_area (wt, &bounds, FALSE);
    }
}

static void
gimp_warp_tool_stroke_append (GimpWarpTool *wt,
                              gchar         type,
                              gdouble       x,
                              gdouble       y)
{
  GimpWarpOptions *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);
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

      gimp_warp_tool_clear_node_bounds (node);

      gimp_warp_tool_update_bounds (wt);
    }

  gimp_warp_tool_update_area (wt, &area, options->real_time_preview);
}

static void
gimp_warp_tool_filter_flush (GimpDrawableFilter *filter,
                             GimpTool           *tool)
{
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush (gimp_image_get_projection (image));
}

static void
gimp_warp_tool_add_op (GimpWarpTool *wt,
                       GeglNode     *op)
{
  GeglNode *last_op;

  g_return_if_fail (GEGL_IS_NODE (wt->render_node));

  gegl_node_add_child (wt->graph, op);

  last_op = gegl_node_get_producer (wt->render_node, "aux", NULL);

  gegl_node_disconnect (wt->render_node, "aux");
  gegl_node_link (last_op, op);
  gegl_node_connect (op,              "output",
                     wt->render_node, "aux");
}

static void
gimp_warp_tool_remove_op (GimpWarpTool *wt,
                          GeglNode     *op)
{
  GeglNode *previous;

  g_return_if_fail (GEGL_IS_NODE (wt->render_node));

  previous = gegl_node_get_producer (op, "input", NULL);

  gegl_node_disconnect (op,              "input");
  gegl_node_connect (previous,        "output",
                     wt->render_node, "aux");

  gegl_node_remove_child (wt->graph, op);
}

static void
gimp_warp_tool_free_op (GeglNode *op)
{
  GeglNode *parent;

  parent = gegl_node_get_parent (op);

  gimp_assert (parent != NULL);

  gegl_node_remove_child (parent, op);
}

static void
gimp_warp_tool_animate (GimpWarpTool *wt)
{
  GimpTool        *tool    = GIMP_TOOL (wt);
  GimpWarpOptions *options = GIMP_WARP_TOOL_GET_OPTIONS (wt);
  GimpImage       *orig_image;
  GimpImage       *image;
  GimpLayer       *layer;
  GimpLayer       *first_layer;
  GeglNode        *scale_node;
  GimpProgress    *progress;
  GtkWidget       *widget;
  gint             i;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (! gimp_warp_tool_can_undo (tool, tool->display))
    {
      gimp_tool_message_literal (tool, tool->display,
                                 _("Please add some warp strokes first."));
      return;
    }

  /*  get rid of the image map so we can use wt->graph  */
  if (wt->filter)
    {
      gimp_drawable_filter_abort (wt->filter);
      g_clear_object (&wt->filter);
    }

  gimp_warp_tool_set_sampler (wt, /* commit = */ TRUE);

  gimp_progress_start (GIMP_PROGRESS (tool), FALSE,
                       _("Rendering Frame %d"), 1);

  orig_image = gimp_item_get_image (GIMP_ITEM (tool->drawables->data));

  image = gimp_create_image (orig_image->gimp,
                             gimp_item_get_width  (GIMP_ITEM (tool->drawables->data)),
                             gimp_item_get_height (GIMP_ITEM (tool->drawables->data)),
                             gimp_drawable_get_base_type (tool->drawables->data),
                             gimp_drawable_get_precision (tool->drawables->data),
                             TRUE);

  /*  the first frame is always the unwarped image  */
  layer = GIMP_LAYER (gimp_item_convert (GIMP_ITEM (tool->drawables->data), image,
                                         GIMP_TYPE_LAYER));
  gimp_object_take_name (GIMP_OBJECT (layer),
                         g_strdup_printf (_("Frame %d"), 1));

  gimp_item_set_offset (GIMP_ITEM (layer), 0, 0);
  gimp_item_set_visible (GIMP_ITEM (layer), TRUE, FALSE);
  gimp_layer_set_mode (layer, gimp_image_get_default_new_layer_mode (image),
                       FALSE);
  gimp_layer_set_opacity (layer, GIMP_OPACITY_OPAQUE, FALSE);
  gimp_image_add_layer (image, layer, NULL, 0, FALSE);

  first_layer = layer;

  scale_node = gegl_node_new_child (NULL,
                                    "operation",    "gimp:scalar-multiply",
                                    "n-components", 2,
                                    NULL);
  gimp_warp_tool_add_op (wt, scale_node);

  progress = gimp_sub_progress_new (GIMP_PROGRESS (tool));

  for (i = 1; i < options->n_animation_frames; i++)
    {
      gimp_progress_set_text (GIMP_PROGRESS (tool),
                              _("Rendering Frame %d"), i + 1);

      gimp_sub_progress_set_step (GIMP_SUB_PROGRESS (progress),
                                  i, options->n_animation_frames);

      layer = GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (first_layer),
                                               GIMP_TYPE_LAYER));
      gimp_object_take_name (GIMP_OBJECT (layer),
                             g_strdup_printf (_("Frame %d"), i + 1));

      gegl_node_set (scale_node,
                     "factor", (gdouble) i /
                               (gdouble) (options->n_animation_frames - 1),
                     NULL);

      gimp_gegl_apply_operation (gimp_drawable_get_buffer (GIMP_DRAWABLE (first_layer)),
                                 progress,
                                 _("Frame"),
                                 wt->graph,
                                 gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                 NULL, FALSE);

      gimp_image_add_layer (image, layer, NULL, 0, FALSE);
    }

  g_object_unref (progress);

  gimp_warp_tool_remove_op (wt, scale_node);

  gimp_progress_end (GIMP_PROGRESS (tool));

  /*  recreate the image map  */
  gimp_warp_tool_create_filter (wt, tool->drawables->data);
  gimp_warp_tool_update_stroke (wt, NULL);

  widget = GTK_WIDGET (gimp_display_get_shell (tool->display));
  gimp_create_display (orig_image->gimp, image, gimp_unit_pixel (), 1.0,
                       G_OBJECT (gimp_widget_get_monitor (widget)));
  g_object_unref (image);
}
