/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmadisplayconfig.h"

#include "core/ligma.h"
#include "core/ligmaguide.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-arrange.h"
#include "core/ligmaimage-pick-item.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmalayer.h"

#include "vectors/ligmavectors.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvasitem.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-appearance.h"

#include "ligmaalignoptions.h"
#include "ligmaaligntool.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


#define EPSILON  3   /* move distance after which we do a box-select */


/*  local function prototypes  */

static void     ligma_align_tool_constructed    (GObject               *object);

static void     ligma_align_tool_control        (LigmaTool              *tool,
                                                LigmaToolAction         action,
                                                LigmaDisplay           *display);
static void     ligma_align_tool_button_press   (LigmaTool              *tool,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                LigmaButtonPressType    press_type,
                                                LigmaDisplay           *display);
static void     ligma_align_tool_button_release (LigmaTool              *tool,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                LigmaButtonReleaseType  release_type,
                                                LigmaDisplay           *display);
static void     ligma_align_tool_motion         (LigmaTool              *tool,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                LigmaDisplay           *display);
static gboolean ligma_align_tool_key_press      (LigmaTool              *tool,
                                                GdkEventKey           *kevent,
                                                LigmaDisplay           *display);
static void     ligma_align_tool_oper_update    (LigmaTool              *tool,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity,
                                                LigmaDisplay           *display);
static void     ligma_align_tool_status_update  (LigmaTool              *tool,
                                                LigmaDisplay           *display,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     ligma_align_tool_cursor_update  (LigmaTool              *tool,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state,
                                                LigmaDisplay           *display);
const gchar *   ligma_align_tool_can_undo       (LigmaTool              *tool,
                                                LigmaDisplay           *display);
static gboolean ligma_align_tool_undo           (LigmaTool              *tool,
                                                LigmaDisplay           *display);

static void     ligma_align_tool_draw           (LigmaDrawTool          *draw_tool);
static void     ligma_align_tool_redraw         (LigmaAlignTool         *align_tool);

static void     ligma_align_tool_align          (LigmaAlignTool         *align_tool,
                                                LigmaAlignmentType      align_type);

static gint     ligma_align_tool_undo_idle      (gpointer               data);
static void    ligma_align_tool_display_changed (LigmaContext           *context,
                                                LigmaDisplay           *display,
                                                LigmaAlignTool         *align_tool);


G_DEFINE_TYPE (LigmaAlignTool, ligma_align_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_align_tool_parent_class


void
ligma_align_tool_register (LigmaToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (LIGMA_TYPE_ALIGN_TOOL,
                LIGMA_TYPE_ALIGN_OPTIONS,
                ligma_align_options_gui,
                0,
                "ligma-align-tool",
                _("Align and Distribute"),
                _("Alignment Tool: Align or arrange layers and other objects"),
                N_("_Align and Distribute"), "Q",
                NULL, LIGMA_HELP_TOOL_ALIGN,
                LIGMA_ICON_TOOL_ALIGN,
                data);
}

static void
ligma_align_tool_class_init (LigmaAlignToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = ligma_align_tool_constructed;

  tool_class->control        = ligma_align_tool_control;
  tool_class->button_press   = ligma_align_tool_button_press;
  tool_class->button_release = ligma_align_tool_button_release;
  tool_class->motion         = ligma_align_tool_motion;
  tool_class->key_press      = ligma_align_tool_key_press;
  tool_class->oper_update    = ligma_align_tool_oper_update;
  tool_class->cursor_update  = ligma_align_tool_cursor_update;
  tool_class->can_undo       = ligma_align_tool_can_undo;
  tool_class->undo           = ligma_align_tool_undo;

  draw_tool_class->draw      = ligma_align_tool_draw;
}

static void
ligma_align_tool_init (LigmaAlignTool *align_tool)
{
  LigmaTool *tool = LIGMA_TOOL (align_tool);

  ligma_tool_control_set_snap_to     (tool->control, FALSE);
  ligma_tool_control_set_precision   (tool->control,
                                     LIGMA_CURSOR_PRECISION_PIXEL_BORDER);
  ligma_tool_control_set_tool_cursor (tool->control, LIGMA_TOOL_CURSOR_MOVE);

  align_tool->function = ALIGN_TOOL_REF_IDLE;
}

static void
ligma_align_tool_constructed (GObject *object)
{
  LigmaAlignTool    *align_tool = LIGMA_ALIGN_TOOL (object);
  LigmaAlignOptions *options;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  options = LIGMA_ALIGN_TOOL_GET_OPTIONS (align_tool);

  g_signal_connect_object (options, "align-button-clicked",
                           G_CALLBACK (ligma_align_tool_align),
                           align_tool, G_CONNECT_SWAPPED);
  g_signal_connect_object (options, "notify::align-reference",
                           G_CALLBACK (ligma_align_tool_redraw),
                           align_tool, G_CONNECT_SWAPPED);
  g_signal_connect_object (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma),
                           "display-changed",
                           G_CALLBACK (ligma_align_tool_display_changed),
                           align_tool, 0);
}

static void
ligma_align_tool_control (LigmaTool       *tool,
                         LigmaToolAction  action,
                         LigmaDisplay    *display)
{
  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_align_tool_button_press (LigmaTool            *tool,
                              const LigmaCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              LigmaButtonPressType  press_type,
                              LigmaDisplay         *display)
{
  LigmaAlignTool *align_tool  = LIGMA_ALIGN_TOOL (tool);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  /*  If the tool was being used in another image... reset it  */
  if (display != tool->display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);

  tool->display = display;

  ligma_tool_control_activate (tool->control);

  align_tool->x2 = align_tool->x1 = coords->x;
  align_tool->y2 = align_tool->y1 = coords->y;

  if (! ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tool)))
    ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

/* some rather complex logic here.  If the user clicks without modifiers,
 * then we start a new list, and use the first object in it as reference.
 * If the user clicks using Shift, or draws a rubber-band box, then
 * we add objects to the list, but do not specify which one should
 * be used as reference.
 */
static void
ligma_align_tool_button_release (LigmaTool              *tool,
                                const LigmaCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                LigmaButtonReleaseType  release_type,
                                LigmaDisplay           *display)
{
  LigmaAlignTool    *align_tool = LIGMA_ALIGN_TOOL (tool);
  LigmaAlignOptions *options    = LIGMA_ALIGN_TOOL_GET_OPTIONS (tool);
  LigmaDisplayShell *shell      = ligma_display_get_shell (display);
  LigmaImage        *image      = ligma_display_get_image (display);
  GdkModifierType   extend_mask;

  extend_mask = ligma_get_extend_selection_mask ();

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  ligma_tool_control_halt (tool->control);

  if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
    {
      align_tool->x2 = align_tool->x1;
      align_tool->y2 = align_tool->y1;

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
      return;
    }

  if (state & GDK_MOD1_MASK)
    {
      LigmaGuide *guide = NULL;

      if (ligma_display_shell_get_show_guides (shell))
        {
          gint snap_distance = display->config->snap_distance;

          guide = ligma_image_pick_guide (image,
                                          coords->x, coords->y,
                                          FUNSCALEX (shell, snap_distance),
                                          FUNSCALEY (shell, snap_distance));
        }

      ligma_align_options_pick_guide (options, guide, (gboolean) state & extend_mask);
    }
  else
    {
      GObject *object = NULL;

      /* Check if a layer is fully included in the rubber-band rectangle.
       * Don't verify for too small rectangles.
       */
      /* FIXME: look for vectors too */
      if (hypot (coords->x - align_tool->x1,
                 coords->y - align_tool->y1) > EPSILON)
        {
          gint   X0 = MIN (coords->x, align_tool->x1);
          gint   X1 = MAX (coords->x, align_tool->x1);
          gint   Y0 = MIN (coords->y, align_tool->y1);
          gint   Y1 = MAX (coords->y, align_tool->y1);
          GList *all_layers;
          GList *list;

          all_layers = ligma_image_get_layer_list (image);

          for (list = all_layers; list; list = g_list_next (list))
            {
              LigmaLayer *layer = list->data;
              gint       x0, y0, x1, y1;

              if (! ligma_item_get_visible (LIGMA_ITEM (layer)))
                continue;

              ligma_item_get_offset (LIGMA_ITEM (layer), &x0, &y0);
              x1 = x0 + ligma_item_get_width  (LIGMA_ITEM (layer));
              y1 = y0 + ligma_item_get_height (LIGMA_ITEM (layer));

              if (x0 < X0 || y0 < Y0 || x1 > X1 || y1 > Y1)
                continue;

              object = G_OBJECT (layer);
              break;
            }

          g_list_free (all_layers);
        }

      if (object == NULL)
        {
          LigmaVectors *vectors;
          LigmaGuide   *guide;
          LigmaLayer   *layer;
          GObject     *previously_picked;
          gint         snap_distance = display->config->snap_distance;

          previously_picked = ligma_align_options_get_reference (options, FALSE);


          if ((vectors = ligma_image_pick_vectors (image,
                                                  coords->x, coords->y,
                                                  FUNSCALEX (shell, snap_distance),
                                                  FUNSCALEY (shell, snap_distance))))
            {
              object = G_OBJECT (vectors);
            }
          else if (ligma_display_shell_get_show_guides (shell) &&
                   (guide = ligma_image_pick_guide (image,
                                                   coords->x, coords->y,
                                                   FUNSCALEX (shell, snap_distance),
                                                   FUNSCALEY (shell, snap_distance))))
            {
              object = G_OBJECT (guide);
            }
          else if ((layer = ligma_image_pick_layer (image, coords->x, coords->y,
                                                   previously_picked && LIGMA_IS_LAYER (previously_picked)? LIGMA_LAYER (previously_picked) : NULL)))
            {
              object = G_OBJECT (layer);
            }
        }

      if (object)
        ligma_align_options_pick_reference (options, object);
    }

  align_tool->x2 = align_tool->x1;
  align_tool->y2 = align_tool->y1;

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_align_tool_motion (LigmaTool         *tool,
                        const LigmaCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        LigmaDisplay      *display)
{
  LigmaAlignTool *align_tool = LIGMA_ALIGN_TOOL (tool);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  align_tool->x2 = coords->x;
  align_tool->y2 = coords->y;

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static gboolean
ligma_align_tool_key_press (LigmaTool    *tool,
                           GdkEventKey *kevent,
                           LigmaDisplay *display)
{
  if (display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Escape:
          ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
          return TRUE;

        default:
          break;
        }
    }

  return FALSE;
}

static void
ligma_align_tool_oper_update (LigmaTool         *tool,
                             const LigmaCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             LigmaDisplay      *display)
{
  LigmaAlignTool    *align_tool    = LIGMA_ALIGN_TOOL (tool);
  LigmaAlignOptions *options       = LIGMA_ALIGN_TOOL_GET_OPTIONS (align_tool);
  LigmaDisplayShell *shell         = ligma_display_get_shell (display);
  LigmaImage        *image         = ligma_display_get_image (display);
  gint              snap_distance = display->config->snap_distance;

  state &= ligma_get_all_modifiers_mask ();

  align_tool->function = ALIGN_TOOL_NO_ACTION;

  if (ligma_image_pick_vectors (image,
                               coords->x, coords->y,
                               FUNSCALEX (shell, snap_distance),
                               FUNSCALEY (shell, snap_distance)))
    {
      if (options->align_reference == LIGMA_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_PICK_PATH;
    }
  else if (ligma_display_shell_get_show_guides (shell) &&
           ligma_image_pick_guide (image,
                                  coords->x, coords->y,
                                  FUNSCALEX (shell, snap_distance),
                                  FUNSCALEY (shell, snap_distance)))
    {
      if (state == (ligma_get_extend_selection_mask () | GDK_MOD1_MASK))
        align_tool->function = ALIGN_TOOL_ALIGN_ADD_GUIDE;
      else if (state == GDK_MOD1_MASK)
        align_tool->function = ALIGN_TOOL_ALIGN_PICK_GUIDE;
      else if (options->align_reference == LIGMA_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_PICK_GUIDE;
    }
  else if (ligma_image_pick_layer_by_bounds (image, coords->x, coords->y))
    {
      if (options->align_reference == LIGMA_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_PICK_LAYER;
    }
  else
    {
      if (state & GDK_MOD1_MASK)
        align_tool->function = ALIGN_TOOL_ALIGN_IDLE;
      else if (options->align_reference == LIGMA_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_IDLE;
    }

  ligma_align_tool_status_update (tool, display, state, proximity);

  if (! ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tool)))
    ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);
}

static void
ligma_align_tool_cursor_update (LigmaTool         *tool,
                               const LigmaCoords *coords,
                               GdkModifierType   state,
                               LigmaDisplay      *display)
{
  LigmaAlignTool      *align_tool  = LIGMA_ALIGN_TOOL (tool);
  LigmaToolCursorType  tool_cursor = LIGMA_TOOL_CURSOR_NONE;
  LigmaCursorModifier  modifier    = LIGMA_CURSOR_MODIFIER_NONE;

  switch (align_tool->function)
    {
    case ALIGN_TOOL_REF_IDLE:
    case ALIGN_TOOL_ALIGN_IDLE:
      tool_cursor = LIGMA_TOOL_CURSOR_RECT_SELECT;
      break;

    case ALIGN_TOOL_REF_PICK_LAYER:
      tool_cursor = LIGMA_TOOL_CURSOR_HAND;
      break;

    case ALIGN_TOOL_ALIGN_ADD_GUIDE:
      modifier = LIGMA_CURSOR_MODIFIER_PLUS;
    case ALIGN_TOOL_REF_PICK_GUIDE:
    case ALIGN_TOOL_ALIGN_PICK_GUIDE:
      tool_cursor = LIGMA_TOOL_CURSOR_MOVE;
      break;

    case ALIGN_TOOL_REF_PICK_PATH:
      tool_cursor = LIGMA_TOOL_CURSOR_PATHS;
      break;

    case ALIGN_TOOL_REF_DRAG_BOX:
    case ALIGN_TOOL_NO_ACTION:
      break;
    }

  ligma_tool_control_set_cursor          (tool->control, LIGMA_CURSOR_MOUSE);
  ligma_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

const gchar *
ligma_align_tool_can_undo (LigmaTool    *tool,
                          LigmaDisplay *display)
{
  return _("Arrange Objects");
}

static gboolean
ligma_align_tool_undo (LigmaTool    *tool,
                      LigmaDisplay *display)
{
  g_idle_add ((GSourceFunc) ligma_align_tool_undo_idle, (gpointer) tool);

  return FALSE;
}

static void
ligma_align_tool_status_update (LigmaTool        *tool,
                               LigmaDisplay     *display,
                               GdkModifierType  state,
                               gboolean         proximity)
{
  LigmaAlignTool    *align_tool = LIGMA_ALIGN_TOOL (tool);
  gchar            *status     = NULL;
  GdkModifierType   extend_mask;

  extend_mask = ligma_get_extend_selection_mask ();

  ligma_tool_pop_status (tool, display);

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
          status = ligma_suggest_modifiers (_("Click to pick this guide as reference"),
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
          status = ligma_suggest_modifiers (_("Click to select this guide for alignment"),
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
      ligma_tool_push_status (tool, display, "%s", status);
      g_free (status);
    }
}

static void
ligma_align_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaAlignTool    *align_tool = LIGMA_ALIGN_TOOL (draw_tool);
  LigmaAlignOptions *options    = LIGMA_ALIGN_TOOL_GET_OPTIONS (align_tool);
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
    ligma_draw_tool_add_rectangle (draw_tool, FALSE, x, y, w, h);

  /* Draw handles on the reference object. */
  reference = ligma_align_options_get_reference (options, FALSE);
  if (LIGMA_IS_ITEM (reference))
    {
      LigmaItem *item = LIGMA_ITEM (reference);
      gint      off_x, off_y;

      ligma_item_bounds (item, &x, &y, &w, &h);

      ligma_item_get_offset (item, &off_x, &off_y);
      x += off_x;
      y += off_y;

      ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                 x, y,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_HANDLE_ANCHOR_NORTH_WEST);
      ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                 x + w, y,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_HANDLE_ANCHOR_NORTH_EAST);
      ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                 x, y + h,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_HANDLE_ANCHOR_SOUTH_WEST);
      ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                 x + w, y + h,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_HANDLE_ANCHOR_SOUTH_EAST);
    }
  else if (LIGMA_IS_GUIDE (reference))
    {
      LigmaGuide *guide = LIGMA_GUIDE (reference);
      LigmaImage *image = ligma_display_get_image (draw_tool->display);
      gint       x, y;
      gint       w, h;

      switch (ligma_guide_get_orientation (guide))
        {
        case LIGMA_ORIENTATION_VERTICAL:
          x = ligma_guide_get_position (guide);
          h = ligma_image_get_height (image);
          ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                     x, h,
                                     LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                     LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                     LIGMA_HANDLE_ANCHOR_SOUTH);
          ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                     x, 0,
                                     LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                     LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                     LIGMA_HANDLE_ANCHOR_NORTH);
          break;

        case LIGMA_ORIENTATION_HORIZONTAL:
          y = ligma_guide_get_position (guide);
          w = ligma_image_get_width (image);
          ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                     w, y,
                                     LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                     LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                     LIGMA_HANDLE_ANCHOR_EAST);
          ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                     0, y,
                                     LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                     LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                     LIGMA_HANDLE_ANCHOR_WEST);
          break;

        default:
          break;
        }
    }
  else if (LIGMA_IS_IMAGE (reference))
    {
      LigmaImage *image  = LIGMA_IMAGE (reference);
      gint       width  = ligma_image_get_width (image);
      gint       height = ligma_image_get_height (image);

      ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                 0, 0,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_HANDLE_ANCHOR_NORTH_WEST);
      ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                 width, 0,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_HANDLE_ANCHOR_NORTH_EAST);
      ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                 0, height,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_HANDLE_ANCHOR_SOUTH_WEST);
      ligma_draw_tool_add_handle (draw_tool, LIGMA_HANDLE_FILLED_SQUARE,
                                 width, height,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_TOOL_HANDLE_SIZE_SMALL,
                                 LIGMA_HANDLE_ANCHOR_SOUTH_EAST);
    }

  /* Highlight picked guides, similarly to how they are highlighted in Move
   * tool.
   */
  objects = ligma_align_options_get_objects (options);
  for (iter = objects; iter; iter = iter->next)
    {
      if (LIGMA_IS_GUIDE (iter->data))
        {
          LigmaGuide      *guide = iter->data;
          LigmaCanvasItem *item;
          LigmaGuideStyle  style;

          style = ligma_guide_get_style (guide);

          item = ligma_draw_tool_add_guide (draw_tool,
                                           ligma_guide_get_orientation (guide),
                                           ligma_guide_get_position (guide),
                                           style);
          ligma_canvas_item_set_highlight (item, TRUE);
        }
    }
}

static void
ligma_align_tool_redraw (LigmaAlignTool *align_tool)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (align_tool);

  ligma_draw_tool_pause (draw_tool);

  if (! ligma_draw_tool_is_active (draw_tool) && draw_tool->display)
    ligma_draw_tool_start (draw_tool, draw_tool->display);

  ligma_draw_tool_resume (draw_tool);
}

static void
ligma_align_tool_align (LigmaAlignTool     *align_tool,
                       LigmaAlignmentType  align_type)
{
  LigmaAlignOptions *options = LIGMA_ALIGN_TOOL_GET_OPTIONS (align_tool);
  LigmaImage        *image;
  GObject          *reference_object = NULL;
  GList            *objects;
  GList            *list;
  gdouble           align_x = 0.0;
  gdouble           align_y = 0.0;

  /* if nothing is selected, just return */
  objects = ligma_align_options_get_objects (options);
  if (! objects)
    return;

  image = ligma_context_get_image (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma));
  list  = objects;

  if (align_type < LIGMA_ARRANGE_HFILL)
    {
      reference_object = ligma_align_options_get_reference (options, TRUE);
      if (! reference_object)
        return;
    }

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (align_tool));

  ligma_align_options_get_pivot (options, &align_x, &align_y);
  ligma_image_arrange_objects (image, list,
                              align_x, align_y,
                              reference_object,
                              align_type,
                              ligma_align_options_align_contents (options));

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (align_tool));

  ligma_image_flush (image);
  g_list_free (objects);
}

static gint
ligma_align_tool_undo_idle (gpointer data)
{
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (data);
  LigmaDisplay  *display   = draw_tool->display;

  /* This makes sure that we properly undraw then redraw the various handles and
   * highlighted guides after undoing.
   */
  ligma_draw_tool_stop (draw_tool);
  ligma_draw_tool_start (draw_tool, display);

  return FALSE; /* remove idle */
}

static void
ligma_align_tool_display_changed (LigmaContext   *context,
                                 LigmaDisplay   *display,
                                 LigmaAlignTool *align_tool)
{
  LigmaTool     *tool      = LIGMA_TOOL (align_tool);
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (align_tool);

  ligma_draw_tool_pause (draw_tool);

  if (display != tool->display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);

  tool->display = display;

  if (! ligma_draw_tool_is_active (draw_tool) && display)
    ligma_draw_tool_start (draw_tool, display);

  ligma_draw_tool_resume (draw_tool);
}
