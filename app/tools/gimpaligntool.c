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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-arrange.h"
#include "core/gimpimage-pick-item.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimppickable.h"
#include "core/gimppickable-auto-shrink.h"

#include "path/gimppath.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasitem.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"

#include "gimpalignoptions.h"
#include "gimpaligntool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define EPSILON  3   /* move distance after which we do a box-select */


/*  local function prototypes  */

static void     gimp_align_tool_constructed    (GObject               *object);

static void     gimp_align_tool_control        (GimpTool              *tool,
                                                GimpToolAction         action,
                                                GimpDisplay           *display);
static void     gimp_align_tool_button_press   (GimpTool              *tool,
                                                const GimpCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                GimpButtonPressType    press_type,
                                                GimpDisplay           *display);
static void     gimp_align_tool_button_release (GimpTool              *tool,
                                                const GimpCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                GimpButtonReleaseType  release_type,
                                                GimpDisplay           *display);
static void     gimp_align_tool_motion         (GimpTool              *tool,
                                                const GimpCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                GimpDisplay           *display);
static gboolean gimp_align_tool_key_press      (GimpTool              *tool,
                                                GdkEventKey           *kevent,
                                                GimpDisplay           *display);
static void     gimp_align_tool_oper_update    (GimpTool              *tool,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity,
                                                GimpDisplay           *display);
static void     gimp_align_tool_status_update  (GimpTool              *tool,
                                                GimpDisplay           *display,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     gimp_align_tool_cursor_update  (GimpTool              *tool,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state,
                                                GimpDisplay           *display);
const gchar *   gimp_align_tool_can_undo       (GimpTool              *tool,
                                                GimpDisplay           *display);
static gboolean gimp_align_tool_undo           (GimpTool              *tool,
                                                GimpDisplay           *display);

static void     gimp_align_tool_draw           (GimpDrawTool          *draw_tool);
static void     gimp_align_tool_redraw         (GimpAlignTool         *align_tool);

static void     gimp_align_tool_align          (GimpAlignTool         *align_tool,
                                                GimpAlignmentType      align_type);

static gint     gimp_align_tool_undo_idle      (gpointer               data);
static void    gimp_align_tool_display_changed (GimpContext           *context,
                                                GimpDisplay           *display,
                                                GimpAlignTool         *align_tool);


G_DEFINE_TYPE (GimpAlignTool, gimp_align_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_align_tool_parent_class


void
gimp_align_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_ALIGN_TOOL,
                GIMP_TYPE_ALIGN_OPTIONS,
                gimp_align_options_gui,
                0,
                "gimp-align-tool",
                _("Align and Distribute"),
                _("Alignment Tool: Align or arrange layers and other objects"),
                N_("_Align and Distribute"), "Q",
                NULL, GIMP_HELP_TOOL_ALIGN,
                GIMP_ICON_TOOL_ALIGN,
                data);
}

static void
gimp_align_tool_class_init (GimpAlignToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = gimp_align_tool_constructed;

  tool_class->control        = gimp_align_tool_control;
  tool_class->button_press   = gimp_align_tool_button_press;
  tool_class->button_release = gimp_align_tool_button_release;
  tool_class->motion         = gimp_align_tool_motion;
  tool_class->key_press      = gimp_align_tool_key_press;
  tool_class->oper_update    = gimp_align_tool_oper_update;
  tool_class->cursor_update  = gimp_align_tool_cursor_update;
  tool_class->can_undo       = gimp_align_tool_can_undo;
  tool_class->undo           = gimp_align_tool_undo;
  tool_class->is_destructive = FALSE;

  draw_tool_class->draw      = gimp_align_tool_draw;
}

static void
gimp_align_tool_init (GimpAlignTool *align_tool)
{
  GimpTool *tool = GIMP_TOOL (align_tool);

  gimp_tool_control_set_snap_to     (tool->control, FALSE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_MOVE);

  align_tool->function = ALIGN_TOOL_REF_IDLE;
}

static void
gimp_align_tool_constructed (GObject *object)
{
  GimpAlignTool    *align_tool = GIMP_ALIGN_TOOL (object);
  GimpAlignOptions *options;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  options = GIMP_ALIGN_TOOL_GET_OPTIONS (align_tool);

  g_signal_connect_object (options, "align-button-clicked",
                           G_CALLBACK (gimp_align_tool_align),
                           align_tool, G_CONNECT_SWAPPED);
  g_signal_connect_object (options, "notify::align-reference",
                           G_CALLBACK (gimp_align_tool_redraw),
                           align_tool, G_CONNECT_SWAPPED);
  g_signal_connect_object (options, "notify::align-contents",
                           G_CALLBACK (gimp_align_tool_redraw),
                           align_tool, G_CONNECT_SWAPPED);
  g_signal_connect_object (gimp_get_user_context (GIMP_CONTEXT (options)->gimp),
                           "display-changed",
                           G_CALLBACK (gimp_align_tool_display_changed),
                           align_tool, 0);
}

static void
gimp_align_tool_control (GimpTool       *tool,
                         GimpToolAction  action,
                         GimpDisplay    *display)
{
  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_align_tool_button_press (GimpTool            *tool,
                              const GimpCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              GimpButtonPressType  press_type,
                              GimpDisplay         *display)
{
  GimpAlignTool *align_tool  = GIMP_ALIGN_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  If the tool was being used in another image... reset it  */
  if (display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

  tool->display = display;

  gimp_tool_control_activate (tool->control);

  align_tool->x2 = align_tool->x1 = coords->x;
  align_tool->y2 = align_tool->y1 = coords->y;

  if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

/* some rather complex logic here.  If the user clicks without modifiers,
 * then we start a new list, and use the first object in it as reference.
 * If the user clicks using Shift, or draws a rubber-band box, then
 * we add objects to the list, but do not specify which one should
 * be used as reference.
 */
static void
gimp_align_tool_button_release (GimpTool              *tool,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpAlignTool    *align_tool = GIMP_ALIGN_TOOL (tool);
  GimpAlignOptions *options    = GIMP_ALIGN_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell *shell      = gimp_display_get_shell (display);
  GimpImage        *image      = gimp_display_get_image (display);
  GdkModifierType   extend_mask;

  extend_mask = gimp_get_extend_selection_mask ();

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      align_tool->x2 = align_tool->x1;
      align_tool->y2 = align_tool->y1;

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
      return;
    }

  if (state & GDK_MOD1_MASK)
    {
      GimpGuide *guide = NULL;

      if (gimp_display_shell_get_show_guides (shell))
        {
          gint snap_distance = display->config->snap_distance;

          guide = gimp_image_pick_guide (image,
                                          coords->x, coords->y,
                                          FUNSCALEX (shell, snap_distance),
                                          FUNSCALEY (shell, snap_distance));
        }

      gimp_align_options_pick_guide (options, guide, (gboolean) state & extend_mask);
    }
  else
    {
      GObject *object = NULL;

      /* Check if a layer is fully included in the rubber-band rectangle.
       * Don't verify for too small rectangles.
       */
      /* FIXME: look for paths too */
      if (hypot (coords->x - align_tool->x1,
                 coords->y - align_tool->y1) > EPSILON)
        {
          gint   X0 = MIN (coords->x, align_tool->x1);
          gint   X1 = MAX (coords->x, align_tool->x1);
          gint   Y0 = MIN (coords->y, align_tool->y1);
          gint   Y1 = MAX (coords->y, align_tool->y1);
          GList *all_layers;
          GList *list;

          all_layers = gimp_image_get_layer_list (image);

          for (list = all_layers; list; list = g_list_next (list))
            {
              GimpLayer *layer = list->data;
              gint       x0, y0, x1, y1;

              if (! gimp_item_get_visible (GIMP_ITEM (layer)))
                continue;

              gimp_item_get_offset (GIMP_ITEM (layer), &x0, &y0);
              x1 = x0 + gimp_item_get_width  (GIMP_ITEM (layer));
              y1 = y0 + gimp_item_get_height (GIMP_ITEM (layer));

              if (x0 < X0 || y0 < Y0 || x1 > X1 || y1 > Y1)
                continue;

              object = G_OBJECT (layer);
              break;
            }

          g_list_free (all_layers);
        }

      if (object == NULL)
        {
          GimpPath    *path;
          GimpGuide   *guide;
          GimpLayer   *layer;
          GObject     *previously_picked;
          gint         snap_distance = display->config->snap_distance;

          previously_picked = gimp_align_options_get_reference (options, FALSE);


          if ((path = gimp_image_pick_path (image,
                                               coords->x, coords->y,
                                               FUNSCALEX (shell, snap_distance),
                                               FUNSCALEY (shell, snap_distance))))
            {
              object = G_OBJECT (path);
            }
          else if (gimp_display_shell_get_show_guides (shell) &&
                   (guide = gimp_image_pick_guide (image,
                                                   coords->x, coords->y,
                                                   FUNSCALEX (shell, snap_distance),
                                                   FUNSCALEY (shell, snap_distance))))
            {
              object = G_OBJECT (guide);
            }
          else if ((layer = gimp_image_pick_layer (image, coords->x, coords->y,
                                                   previously_picked && GIMP_IS_LAYER (previously_picked)? GIMP_LAYER (previously_picked) : NULL)))
            {
              object = G_OBJECT (layer);
            }
        }

      if (object)
        gimp_align_options_pick_reference (options, object);
    }

  align_tool->x2 = align_tool->x1;
  align_tool->y2 = align_tool->y1;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_align_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  align_tool->x2 = coords->x;
  align_tool->y2 = coords->y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static gboolean
gimp_align_tool_key_press (GimpTool    *tool,
                           GdkEventKey *kevent,
                           GimpDisplay *display)
{
  if (display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Escape:
          gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
          return TRUE;

        default:
          break;
        }
    }

  return FALSE;
}

static void
gimp_align_tool_oper_update (GimpTool         *tool,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             GimpDisplay      *display)
{
  GimpAlignTool    *align_tool    = GIMP_ALIGN_TOOL (tool);
  GimpAlignOptions *options       = GIMP_ALIGN_TOOL_GET_OPTIONS (align_tool);
  GimpDisplayShell *shell         = gimp_display_get_shell (display);
  GimpImage        *image         = gimp_display_get_image (display);
  gint              snap_distance = display->config->snap_distance;

  state &= gimp_get_all_modifiers_mask ();

  align_tool->function = ALIGN_TOOL_NO_ACTION;

  if (gimp_image_pick_path (image,
                            coords->x, coords->y,
                            FUNSCALEX (shell, snap_distance),
                            FUNSCALEY (shell, snap_distance)))
    {
      if (options->align_reference == GIMP_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_PICK_PATH;
    }
  else if (gimp_display_shell_get_show_guides (shell) &&
           gimp_image_pick_guide (image,
                                  coords->x, coords->y,
                                  FUNSCALEX (shell, snap_distance),
                                  FUNSCALEY (shell, snap_distance)))
    {
      if (state == (gimp_get_extend_selection_mask () | GDK_MOD1_MASK))
        align_tool->function = ALIGN_TOOL_ALIGN_ADD_GUIDE;
      else if (state == GDK_MOD1_MASK)
        align_tool->function = ALIGN_TOOL_ALIGN_PICK_GUIDE;
      else if (options->align_reference == GIMP_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_PICK_GUIDE;
    }
  else if (gimp_image_pick_layer_by_bounds (image, coords->x, coords->y))
    {
      if (options->align_reference == GIMP_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_PICK_LAYER;
    }
  else
    {
      if (state & GDK_MOD1_MASK)
        align_tool->function = ALIGN_TOOL_ALIGN_IDLE;
      else if (options->align_reference == GIMP_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_IDLE;
    }

  gimp_align_tool_status_update (tool, display, state, proximity);

  if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_align_tool_cursor_update (GimpTool         *tool,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               GimpDisplay      *display)
{
  GimpAlignTool      *align_tool  = GIMP_ALIGN_TOOL (tool);
  GimpToolCursorType  tool_cursor = GIMP_TOOL_CURSOR_NONE;
  GimpCursorModifier  modifier    = GIMP_CURSOR_MODIFIER_NONE;

  switch (align_tool->function)
    {
    case ALIGN_TOOL_REF_IDLE:
    case ALIGN_TOOL_ALIGN_IDLE:
      tool_cursor = GIMP_TOOL_CURSOR_RECT_SELECT;
      break;

    case ALIGN_TOOL_REF_PICK_LAYER:
      tool_cursor = GIMP_TOOL_CURSOR_HAND;
      break;

    case ALIGN_TOOL_ALIGN_ADD_GUIDE:
      modifier = GIMP_CURSOR_MODIFIER_PLUS;
    case ALIGN_TOOL_REF_PICK_GUIDE:
    case ALIGN_TOOL_ALIGN_PICK_GUIDE:
      tool_cursor = GIMP_TOOL_CURSOR_MOVE;
      break;

    case ALIGN_TOOL_REF_PICK_PATH:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS;
      break;

    case ALIGN_TOOL_REF_DRAG_BOX:
    case ALIGN_TOOL_NO_ACTION:
      break;
    }

  gimp_tool_control_set_cursor          (tool->control, GIMP_CURSOR_MOUSE);
  gimp_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

const gchar *
gimp_align_tool_can_undo (GimpTool    *tool,
                          GimpDisplay *display)
{
  return _("Arrange Objects");
}

static gboolean
gimp_align_tool_undo (GimpTool    *tool,
                      GimpDisplay *display)
{
  g_idle_add ((GSourceFunc) gimp_align_tool_undo_idle, (gpointer) tool);

  return FALSE;
}

static void
gimp_align_tool_status_update (GimpTool        *tool,
                               GimpDisplay     *display,
                               GdkModifierType  state,
                               gboolean         proximity)
{
  GimpAlignTool    *align_tool = GIMP_ALIGN_TOOL (tool);
  gchar            *status     = NULL;
  GdkModifierType   extend_mask;

  extend_mask = gimp_get_extend_selection_mask ();

  gimp_tool_pop_status (tool, display);

  if (proximity)
    {
      switch (align_tool->function)
        {
        case ALIGN_TOOL_REF_IDLE:
          status = g_strdup (_("Click on a layer, path or guide, "
                               "or Click-Drag to pick a reference"));
          break;
        case ALIGN_TOOL_REF_PICK_LAYER:
          status = g_strdup (_("Click to pick this layer as reference"));
          break;
        case ALIGN_TOOL_REF_PICK_GUIDE:
          status = gimp_suggest_modifiers (_("Click to pick this guide as reference"),
                                           GDK_MOD1_MASK & ~state,
                                           NULL, NULL, NULL);
          break;
        case ALIGN_TOOL_REF_PICK_PATH:
          status = g_strdup (_("Click to pick this path as reference"));
          break;

        case ALIGN_TOOL_REF_DRAG_BOX:
          break;

        case ALIGN_TOOL_ALIGN_IDLE:
          status = g_strdup (_("Click on a guide to add it to objects to align, "
                               "click anywhere else to unselect all guides"));
          break;
        case ALIGN_TOOL_ALIGN_PICK_GUIDE:
          status = gimp_suggest_modifiers (_("Click to select this guide for alignment"),
                                           extend_mask & ~state,
                                           NULL, NULL, NULL);
          break;
        case ALIGN_TOOL_ALIGN_ADD_GUIDE:
          status = g_strdup (_("Click to add this guide to the list of objects to align"));
          break;

        case ALIGN_TOOL_NO_ACTION:
          break;
        }
    }

  if (status)
    {
      gimp_tool_push_status (tool, display, "%s", status);
      g_free (status);
    }
}

static void
gimp_align_tool_draw (GimpDrawTool *draw_tool)
{
  GimpAlignTool    *align_tool = GIMP_ALIGN_TOOL (draw_tool);
  GimpAlignOptions *options    = GIMP_ALIGN_TOOL_GET_OPTIONS (align_tool);
  GObject          *reference;
  GList            *objects;
  GList            *iter;
  gint              x, y, w, h;

  /* draw rubber-band rectangle */
  x = MIN (align_tool->x2, align_tool->x1);
  y = MIN (align_tool->y2, align_tool->y1);
  w = MAX (align_tool->x2, align_tool->x1) - x;
  h = MAX (align_tool->y2, align_tool->y1) - y;

  if (w != 0 && h != 0)
    gimp_draw_tool_add_rectangle (draw_tool, FALSE, x, y, w, h);

  /* Draw handles on the reference object. */
  reference = gimp_align_options_get_reference (options, FALSE);
  if (GIMP_IS_ITEM (reference))
    {
      GimpItem *item = GIMP_ITEM (reference);
      gint      off_x, off_y;

      gimp_item_bounds (item, &x, &y, &w, &h);

      gimp_item_get_offset (item, &off_x, &off_y);
      x += off_x;
      y += off_y;

      if (gimp_align_options_align_contents (options) &&
          GIMP_IS_PICKABLE (reference))
        {
          gint shrunk_x;
          gint shrunk_y;

          if (gimp_pickable_auto_shrink (GIMP_PICKABLE (reference),
                                         0, 0,
                                         gimp_item_get_width  (GIMP_ITEM (reference)),
                                         gimp_item_get_height (GIMP_ITEM (reference)),
                                         &shrunk_x, &shrunk_y, &w, &h) == GIMP_AUTO_SHRINK_SHRINK)
            {
              x += shrunk_x;
              y += shrunk_y;
            }
        }

      gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                 x, y,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_HANDLE_ANCHOR_NORTH_WEST);
      gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                 x + w, y,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_HANDLE_ANCHOR_NORTH_EAST);
      gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                 x, y + h,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_HANDLE_ANCHOR_SOUTH_WEST);
      gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                 x + w, y + h,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_HANDLE_ANCHOR_SOUTH_EAST);
    }
  else if (GIMP_IS_GUIDE (reference))
    {
      GimpGuide *guide = GIMP_GUIDE (reference);
      GimpImage *image = gimp_display_get_image (draw_tool->display);
      gint       x, y;
      gint       w, h;

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_VERTICAL:
          x = gimp_guide_get_position (guide);
          h = gimp_image_get_height (image);
          gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                     x, h,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_HANDLE_ANCHOR_SOUTH);
          gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                     x, 0,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_HANDLE_ANCHOR_NORTH);
          break;

        case GIMP_ORIENTATION_HORIZONTAL:
          y = gimp_guide_get_position (guide);
          w = gimp_image_get_width (image);
          gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                     w, y,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_HANDLE_ANCHOR_EAST);
          gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                     0, y,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_HANDLE_ANCHOR_WEST);
          break;

        default:
          break;
        }
    }
  else if (GIMP_IS_IMAGE (reference))
    {
      GimpImage *image  = GIMP_IMAGE (reference);
      gint       width  = gimp_image_get_width (image);
      gint       height = gimp_image_get_height (image);

      gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                 0, 0,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_HANDLE_ANCHOR_NORTH_WEST);
      gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                 width, 0,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_HANDLE_ANCHOR_NORTH_EAST);
      gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                 0, height,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_HANDLE_ANCHOR_SOUTH_WEST);
      gimp_draw_tool_add_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                 width, height,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_TOOL_HANDLE_SIZE_SMALL,
                                 GIMP_HANDLE_ANCHOR_SOUTH_EAST);
    }

  /* Highlight picked guides, similarly to how they are highlighted in Move
   * tool.
   */
  objects = gimp_align_options_get_objects (options);
  for (iter = objects; iter; iter = iter->next)
    {
      if (GIMP_IS_GUIDE (iter->data))
        {
          GimpGuide      *guide = iter->data;
          GimpCanvasItem *item;
          GimpGuideStyle  style;

          style = gimp_guide_get_style (guide);

          item = gimp_draw_tool_add_guide (draw_tool,
                                           gimp_guide_get_orientation (guide),
                                           gimp_guide_get_position (guide),
                                           style);
          gimp_canvas_item_set_highlight (item, TRUE);
        }
    }
}

static void
gimp_align_tool_redraw (GimpAlignTool *align_tool)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (align_tool);

  gimp_draw_tool_pause (draw_tool);

  if (! gimp_draw_tool_is_active (draw_tool) && draw_tool->display)
    gimp_draw_tool_start (draw_tool, draw_tool->display);

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_align_tool_align (GimpAlignTool     *align_tool,
                       GimpAlignmentType  align_type)
{
  GimpAlignOptions *options = GIMP_ALIGN_TOOL_GET_OPTIONS (align_tool);
  GimpImage        *image;
  GObject          *reference_object = NULL;
  GList            *objects;
  GList            *list;
  gdouble           align_x = 0.0;
  gdouble           align_y = 0.0;

  /* if nothing is selected, just return */
  objects = gimp_align_options_get_objects (options);
  if (! objects)
    return;

  image = gimp_context_get_image (gimp_get_user_context (GIMP_CONTEXT (options)->gimp));
  list  = objects;

  if (align_type < GIMP_ARRANGE_HFILL)
    {
      reference_object = gimp_align_options_get_reference (options, TRUE);
      if (! reference_object)
        return;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  gimp_align_options_get_pivot (options, &align_x, &align_y);
  gimp_image_arrange_objects (image, list,
                              align_x, align_y,
                              reference_object,
                              align_type,
                              gimp_align_options_align_contents (options));

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));

  gimp_image_flush (image);
  g_list_free (objects);
}

static gint
gimp_align_tool_undo_idle (gpointer data)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (data);
  GimpDisplay  *display   = draw_tool->display;

  /* This makes sure that we properly undraw then redraw the various handles and
   * highlighted guides after undoing.
   */
  gimp_draw_tool_stop (draw_tool);
  gimp_draw_tool_start (draw_tool, display);

  return FALSE; /* remove idle */
}

static void
gimp_align_tool_display_changed (GimpContext   *context,
                                 GimpDisplay   *display,
                                 GimpAlignTool *align_tool)
{
  GimpTool     *tool      = GIMP_TOOL (align_tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (align_tool);

  gimp_draw_tool_pause (draw_tool);

  if (display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

  tool->display = display;

  if (! gimp_draw_tool_is_active (draw_tool) && display)
    gimp_draw_tool_start (draw_tool, display);

  gimp_draw_tool_resume (draw_tool);
}
