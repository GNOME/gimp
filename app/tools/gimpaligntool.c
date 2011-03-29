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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-arrange.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-pick-layer.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"

#include "vectors/gimpvectors.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"

#include "gimpalignoptions.h"
#include "gimpaligntool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define EPSILON  3   /* move distance after which we do a box-select */


/*  local function prototypes  */

static void     gimp_align_tool_constructed  (GObject               *object);
static void     gimp_align_tool_dispose      (GObject               *object);

static gboolean gimp_align_tool_initialize   (GimpTool              *tool,
                                              GimpDisplay           *display,
                                              GError               **error);
static void   gimp_align_tool_control        (GimpTool              *tool,
                                              GimpToolAction         action,
                                              GimpDisplay           *display);
static void   gimp_align_tool_button_press   (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonPressType    press_type,
                                              GimpDisplay           *display);
static void   gimp_align_tool_button_release (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonReleaseType  release_type,
                                              GimpDisplay           *display);
static void   gimp_align_tool_motion         (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_align_tool_oper_update    (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              GdkModifierType        state,
                                              gboolean               proximity,
                                              GimpDisplay           *display);
static void   gimp_align_tool_status_update  (GimpTool              *tool,
                                              GimpDisplay           *display,
                                              GdkModifierType        state,
                                              gboolean               proximity);
static void   gimp_align_tool_cursor_update  (GimpTool              *tool,
                                              const GimpCoords      *coords,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);

static void   gimp_align_tool_draw           (GimpDrawTool          *draw_tool);

static void   gimp_align_tool_halt           (GimpAlignTool         *align_tool);
static void   gimp_align_tool_align          (GimpAlignTool         *align_tool,
                                              GimpAlignmentType      align_type);

static void   clear_selected_object          (GObject               *object,
                                              GimpAlignTool         *align_tool);
static void   clear_all_selected_objects     (GimpAlignTool         *align_tool);


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
                _("Align"),
                _("Alignment Tool: Align or arrange layers and other objects"),
                N_("_Align"), "Q",
                NULL, GIMP_HELP_TOOL_ALIGN,
                GIMP_STOCK_TOOL_ALIGN,
                data);
}

static void
gimp_align_tool_class_init (GimpAlignToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = gimp_align_tool_constructed;
  object_class->dispose      = gimp_align_tool_dispose;

  tool_class->initialize     = gimp_align_tool_initialize;
  tool_class->control        = gimp_align_tool_control;
  tool_class->button_press   = gimp_align_tool_button_press;
  tool_class->button_release = gimp_align_tool_button_release;
  tool_class->motion         = gimp_align_tool_motion;
  tool_class->oper_update    = gimp_align_tool_oper_update;
  tool_class->cursor_update  = gimp_align_tool_cursor_update;

  draw_tool_class->draw      = gimp_align_tool_draw;
}

static void
gimp_align_tool_init (GimpAlignTool *align_tool)
{
  GimpTool *tool = GIMP_TOOL (align_tool);

  align_tool->function         = ALIGN_TOOL_IDLE;
  align_tool->selected_objects = NULL;

  align_tool->align_type  = GIMP_ALIGN_LEFT;

  gimp_tool_control_set_snap_to     (tool->control, FALSE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_MOVE);
}

static void
gimp_align_tool_constructed (GObject *object)
{
  GimpAlignTool    *align_tool = GIMP_ALIGN_TOOL (object);
  GimpAlignOptions *options;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  options = GIMP_ALIGN_TOOL_GET_OPTIONS (align_tool);

  g_signal_connect_object (options, "align-button-clicked",
                           G_CALLBACK (gimp_align_tool_align),
                           align_tool, G_CONNECT_SWAPPED);
}

static void
gimp_align_tool_dispose (GObject *object)
{
  gimp_align_tool_halt (GIMP_ALIGN_TOOL (object));

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
gimp_align_tool_initialize (GimpTool     *tool,
                            GimpDisplay  *display,
                            GError      **error)
{
  if (tool->display != display)
    {
    }

  return TRUE;
}

static void
gimp_align_tool_control (GimpTool       *tool,
                         GimpToolAction  action,
                         GimpDisplay    *display)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_align_tool_halt (align_tool);
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
    gimp_align_tool_halt (align_tool);

  tool->display = display;

  gimp_tool_control_activate (tool->control);

  align_tool->x1 = align_tool->x0 = coords->x;
  align_tool->y1 = align_tool->y0 = coords->y;

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
  GObject          *object     = NULL;
  GimpImage        *image      = gimp_display_get_image (display);
  gint              i;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      align_tool->x1 = align_tool->x0;
      align_tool->y1 = align_tool->y0;

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
      return;
    }

  if (! (state & GDK_SHIFT_MASK)) /* start a new list */
    {
      clear_all_selected_objects (align_tool);
      align_tool->set_reference = FALSE;
    }

  /* if mouse has moved less than EPSILON pixels since button press, select
     the nearest thing, otherwise make a rubber-band rectangle */
  if (hypot (coords->x - align_tool->x0, coords->y - align_tool->y0) < EPSILON)
    {
      GimpVectors *vectors;
      GimpGuide   *guide;
      GimpLayer   *layer;
      gint         snap_distance = display->config->snap_distance;

      if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), display,
                                     coords, snap_distance, snap_distance,
                                     NULL, NULL, NULL, NULL, NULL,
                                     &vectors))
        {
          object = G_OBJECT (vectors);
        }
      else if (gimp_display_shell_get_show_guides (shell) &&
               (guide = gimp_image_find_guide (image,
                                               coords->x, coords->y,
                                               FUNSCALEX (shell, snap_distance),
                                               FUNSCALEY (shell, snap_distance))))
        {
          object = G_OBJECT (guide);
        }
      else
        {
          if ((layer = gimp_image_pick_layer_by_bounds (image,
                                                        coords->x, coords->y)))
            {
              object = G_OBJECT (layer);
            }
        }

      if (object)
        {
          if (! g_list_find (align_tool->selected_objects, object))
            {
              align_tool->selected_objects =
                g_list_append (align_tool->selected_objects, object);

              g_signal_connect (object, "removed",
                                G_CALLBACK (clear_selected_object),
                                align_tool);

              /* if an object has been selected using unmodified click,
               * it should be used as the reference
               */
              if (! (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)))
                align_tool->set_reference = TRUE;
            }
        }
    }
  else  /* FIXME: look for vectors too */
    {
      gint   X0    = MIN (coords->x, align_tool->x0);
      gint   X1    = MAX (coords->x, align_tool->x0);
      gint   Y0    = MIN (coords->y, align_tool->y0);
      gint   Y1    = MAX (coords->y, align_tool->y0);
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

          if (g_list_find (align_tool->selected_objects, layer))
            continue;

          align_tool->selected_objects =
            g_list_append (align_tool->selected_objects, layer);

          g_signal_connect (layer, "removed",
                            G_CALLBACK (clear_selected_object),
                            align_tool);
        }

      g_list_free (all_layers);
    }

  for (i = 0; i < ALIGN_OPTIONS_N_BUTTONS; i++)
    {
      if (options->button[i])
        gtk_widget_set_sensitive (options->button[i],
                                  align_tool->selected_objects != NULL);
    }

  align_tool->x1 = align_tool->x0;
  align_tool->y1 = align_tool->y0;

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

  align_tool->x1 = coords->x;
  align_tool->y1 = coords->y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_align_tool_oper_update (GimpTool         *tool,
                             const GimpCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             GimpDisplay      *display)
{
  GimpAlignTool    *align_tool    = GIMP_ALIGN_TOOL (tool);
  GimpDisplayShell *shell         = gimp_display_get_shell (display);
  GimpImage        *image         = gimp_display_get_image (display);
  gint              snap_distance = display->config->snap_distance;

  if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), display,
                                 coords, snap_distance, snap_distance,
                                 NULL, NULL, NULL, NULL, NULL, NULL))
    {
      if ((state & GDK_SHIFT_MASK) && align_tool->selected_objects)
        align_tool->function = ALIGN_TOOL_ADD_PATH;
      else
        align_tool->function = ALIGN_TOOL_PICK_PATH;
    }
  else if (gimp_display_shell_get_show_guides (shell) &&
           (NULL != gimp_image_find_guide (image,
                                           coords->x, coords->y,
                                           FUNSCALEX (shell, snap_distance),
                                           FUNSCALEY (shell, snap_distance))))
    {
      if ((state & GDK_SHIFT_MASK) && align_tool->selected_objects)
        align_tool->function = ALIGN_TOOL_ADD_GUIDE;
      else
        align_tool->function = ALIGN_TOOL_PICK_GUIDE;
    }
  else
    {
      GimpLayer *layer = gimp_image_pick_layer_by_bounds (image,
                                                          coords->x, coords->y);

      if (layer)
        {
          if ((state & GDK_SHIFT_MASK) && align_tool->selected_objects)
            align_tool->function = ALIGN_TOOL_ADD_LAYER;
          else
            align_tool->function = ALIGN_TOOL_PICK_LAYER;
        }
      else
        {
          align_tool->function = ALIGN_TOOL_IDLE;
        }
    }

  gimp_align_tool_status_update (tool, display, state, proximity);
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

  if (state & GDK_SHIFT_MASK)
    {
      /* always add '+' when Shift is pressed, even if nothing is selected */
      modifier = GIMP_CURSOR_MODIFIER_PLUS;
    }

  switch (align_tool->function)
    {
    case ALIGN_TOOL_IDLE:
      tool_cursor = GIMP_TOOL_CURSOR_RECT_SELECT;
      break;

    case ALIGN_TOOL_PICK_LAYER:
    case ALIGN_TOOL_ADD_LAYER:
      tool_cursor = GIMP_TOOL_CURSOR_HAND;
      break;

    case ALIGN_TOOL_PICK_GUIDE:
    case ALIGN_TOOL_ADD_GUIDE:
      tool_cursor = GIMP_TOOL_CURSOR_MOVE;
      break;

    case ALIGN_TOOL_PICK_PATH:
    case ALIGN_TOOL_ADD_PATH:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS;
      break;

    case ALIGN_TOOL_DRAG_BOX:
      /* FIXME: it would be nice to add something here, but we cannot easily
         detect when the tool is in this mode (the draw tool is always active)
         so this state is not used for the moment.
      */
      break;
    }

  gimp_tool_control_set_cursor          (tool->control, GIMP_CURSOR_MOUSE);
  gimp_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_align_tool_status_update (GimpTool        *tool,
                               GimpDisplay     *display,
                               GdkModifierType  state,
                               gboolean         proximity)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (tool);

  gimp_tool_pop_status (tool, display);

  if (proximity)
    {
      const gchar *status      = NULL;
      gboolean     free_status = FALSE;

      if (! align_tool->selected_objects)
        {
          /* no need to suggest Shift if nothing is selected */
          state |= GDK_SHIFT_MASK;
        }

      switch (align_tool->function)
        {
        case ALIGN_TOOL_IDLE:
          status = gimp_suggest_modifiers (_("Click on a layer, path or guide, "
                                             "or Click-Drag to pick several "
                                             "layers"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case ALIGN_TOOL_PICK_LAYER:
          status = gimp_suggest_modifiers (_("Click to pick this layer as "
                                             "first item"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case ALIGN_TOOL_ADD_LAYER:
          status = _("Click to add this layer to the list");
          break;

        case ALIGN_TOOL_PICK_GUIDE:
          status = gimp_suggest_modifiers (_("Click to pick this guide as "
                                             "first item"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case ALIGN_TOOL_ADD_GUIDE:
          status = _("Click to add this guide to the list");
          break;

        case ALIGN_TOOL_PICK_PATH:
          status = gimp_suggest_modifiers (_("Click to pick this path as "
                                             "first item"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case ALIGN_TOOL_ADD_PATH:
          status = _("Click to add this path to the list");
          break;

        case ALIGN_TOOL_DRAG_BOX:
          break;
        }

      if (status)
        gimp_tool_push_status (tool, display, "%s", status);

      if (free_status)
        g_free ((gchar *) status);
    }
}

static void
gimp_align_tool_draw (GimpDrawTool *draw_tool)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (draw_tool);
  GList         *list;
  gint           x, y, w, h;

  /* draw rubber-band rectangle */
  x = MIN (align_tool->x1, align_tool->x0);
  y = MIN (align_tool->y1, align_tool->y0);
  w = MAX (align_tool->x1, align_tool->x0) - x;
  h = MAX (align_tool->y1, align_tool->y0) - y;

  gimp_draw_tool_add_rectangle (draw_tool, FALSE, x, y, w, h);

  for (list = align_tool->selected_objects;
       list;
       list = g_list_next (list))
    {
      if (GIMP_IS_ITEM (list->data))
        {
          GimpItem *item = GIMP_ITEM (list->data);

          if (GIMP_IS_VECTORS (list->data))
            {
              gdouble x1_f, y1_f, x2_f, y2_f;

              gimp_vectors_bounds (GIMP_VECTORS (item),
                                   &x1_f, &y1_f,
                                   &x2_f, &y2_f);
              x = ROUND (x1_f);
              y = ROUND (y1_f);
              w = ROUND (x2_f - x1_f);
              h = ROUND (y2_f - y1_f);
            }
          else
            {
              gimp_item_get_offset (item, &x, &y);

              w = gimp_item_get_width  (item);
              h = gimp_item_get_height (item);
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
      else if (GIMP_IS_GUIDE (list->data))
        {
          GimpGuide *guide = GIMP_GUIDE (list->data);
          GimpImage *image = gimp_display_get_image (GIMP_TOOL (draw_tool)->display);
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
    }
}

static void
gimp_align_tool_halt (GimpAlignTool *align_tool)
{
  GimpTool *tool = GIMP_TOOL (align_tool);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (align_tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (align_tool));

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);

  clear_all_selected_objects (align_tool);

  if (tool->display)
    gimp_tool_pop_status (tool, tool->display);

  tool->display = NULL;
}

static void
gimp_align_tool_align (GimpAlignTool     *align_tool,
                       GimpAlignmentType  align_type)
{
  GimpAlignOptions *options = GIMP_ALIGN_TOOL_GET_OPTIONS (align_tool);
  GimpImage        *image;
  GObject          *reference_object = NULL;
  GList            *list;
  gint              offset = 0;

  /* if nothing is selected, just return */
  if (! align_tool->selected_objects)
    return;

  image  = gimp_display_get_image (GIMP_TOOL (align_tool)->display);

  switch (align_type)
    {
    case GIMP_ALIGN_LEFT:
    case GIMP_ALIGN_HCENTER:
    case GIMP_ALIGN_RIGHT:
    case GIMP_ALIGN_TOP:
    case GIMP_ALIGN_VCENTER:
    case GIMP_ALIGN_BOTTOM:
      offset = 0;
      break;

    case GIMP_ARRANGE_LEFT:
    case GIMP_ARRANGE_HCENTER:
    case GIMP_ARRANGE_RIGHT:
    case GIMP_ARRANGE_TOP:
    case GIMP_ARRANGE_VCENTER:
    case GIMP_ARRANGE_BOTTOM:
      offset = options->offset_x;
      break;
    }

  /* if only one object is selected, use the image as reference
   * if multiple objects are selected, use the first one as reference if
   * "set_reference" is TRUE, otherwise use NULL.
   */

  list = align_tool->selected_objects;

  switch (options->align_reference)
    {
    case GIMP_ALIGN_REFERENCE_IMAGE:
      reference_object = G_OBJECT (image);
      break;

    case GIMP_ALIGN_REFERENCE_FIRST:
      if (g_list_length (list) == 1)
        {
          reference_object = G_OBJECT (image);
        }
      else
        {
          if (align_tool->set_reference)
            {
              reference_object = G_OBJECT (list->data);
              list = g_list_next (list);
            }
          else
            {
              reference_object = NULL;
            }
        }
      break;

    case GIMP_ALIGN_REFERENCE_SELECTION:
      reference_object = G_OBJECT (gimp_image_get_mask (image));
      break;

    case GIMP_ALIGN_REFERENCE_ACTIVE_LAYER:
      reference_object = G_OBJECT (gimp_image_get_active_layer (image));
      break;

    case GIMP_ALIGN_REFERENCE_ACTIVE_CHANNEL:
      reference_object = G_OBJECT (gimp_image_get_active_channel (image));
      break;

    case GIMP_ALIGN_REFERENCE_ACTIVE_PATH:
      g_print ("reference = active path not yet handled.\n");
      break;
    }

  if (! reference_object)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  gimp_image_arrange_objects (image, list,
                              align_type,
                              reference_object,
                              align_type,
                              offset);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));

  gimp_image_flush (image);
}

static void
clear_selected_object (GObject       *object,
                       GimpAlignTool *align_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  if (align_tool->selected_objects)
    g_signal_handlers_disconnect_by_func (object,
                                          G_CALLBACK (clear_selected_object),
                                          align_tool);

  align_tool->selected_objects = g_list_remove (align_tool->selected_objects,
                                                object);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));
}

static void
clear_all_selected_objects (GimpAlignTool *align_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  while (align_tool->selected_objects)
    {
      GObject *object = align_tool->selected_objects->data;

      g_signal_handlers_disconnect_by_func (object,
                                            clear_selected_object,
                                            align_tool);

      align_tool->selected_objects = g_list_remove (align_tool->selected_objects,
                                                    object);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));
}
