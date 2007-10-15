/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

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
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"

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

#define EPSILON      3   /* move distance after which we do a box-select */
#define MARKER_WIDTH 5   /* size (in pixels) of the square handles */

/*  local function prototypes  */

static GObject * gimp_align_tool_constructor (GType                  type,
                                              guint                  n_params,
                                              GObjectConstructParam *params);
static gboolean gimp_align_tool_initialize   (GimpTool              *tool,
                                              GimpDisplay           *display,
                                              GError               **error);
static void   gimp_align_tool_finalize       (GObject               *object);

static void   gimp_align_tool_control        (GimpTool              *tool,
                                              GimpToolAction         action,
                                              GimpDisplay           *display);
static void   gimp_align_tool_button_press   (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_align_tool_button_release (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonReleaseType  release_tyle,
                                              GimpDisplay           *display);
static void   gimp_align_tool_motion         (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);
static void   gimp_align_tool_oper_update    (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              GdkModifierType        state,
                                              gboolean               proximity,
                                              GimpDisplay           *display);
static void   gimp_align_tool_status_update  (GimpTool              *tool,
                                              GimpDisplay           *display,
                                              GdkModifierType        state,
                                              gboolean               proximity);
static void   gimp_align_tool_cursor_update  (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              GdkModifierType        state,
                                              GimpDisplay           *display);

static void   gimp_align_tool_draw           (GimpDrawTool          *draw_tool);

static GtkWidget *button_with_stock          (GimpAlignmentType      action,
                                              GimpAlignTool         *align_tool);
static GtkWidget *gimp_align_tool_controls   (GimpAlignTool         *align_tool);
static void   do_alignment                   (GtkWidget             *widget,
                                              gpointer               data);
static void   clear_selected_object          (GObject               *object,
                                              GimpAlignTool         *align_tool);
static void   clear_all_selected_objects     (GimpAlignTool         *align_tool);
static GimpLayer *select_layer_by_coords     (GimpImage             *image,
                                              gint                   x,
                                              gint                   y);
void          gimp_image_arrange_objects     (GimpImage             *image,
                                              GList                 *list,
                                              GimpAlignmentType      alignment,
                                              GObject               *reference,
                                              GimpAlignmentType      reference_alignment,
                                              gint                   offset);

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
                NULL, GIMP_HELP_TOOL_MOVE,
                GIMP_STOCK_TOOL_ALIGN,
                data);
}

static void
gimp_align_tool_class_init (GimpAlignToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = gimp_align_tool_finalize;
  object_class->constructor  = gimp_align_tool_constructor;

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

  align_tool->controls         = NULL;

  align_tool->function         = ALIGN_TOOL_IDLE;
  align_tool->selected_objects = NULL;

  align_tool->align_type  = GIMP_ALIGN_LEFT;

  align_tool->horz_offset = 0;
  align_tool->vert_offset = 0;

  gimp_tool_control_set_snap_to     (tool->control, FALSE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_MOVE);

}

static GObject *
gimp_align_tool_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GimpTool      *tool;
  GimpAlignTool *align_tool;
  GtkContainer  *container;
  GObject       *options;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tool       = GIMP_TOOL (object);
  align_tool = GIMP_ALIGN_TOOL (object);
  options    = G_OBJECT (gimp_tool_get_options (tool));

  container = GTK_CONTAINER (g_object_get_data (options,
                                                "controls-container"));

  align_tool->controls = gimp_align_tool_controls (align_tool);
  gtk_container_add (container, align_tool->controls);
  gtk_widget_show (align_tool->controls);

  return object;
}

static void
gimp_align_tool_finalize (GObject *object)
{
  GimpTool      *tool       = GIMP_TOOL (object);
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (object);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (object)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (object));

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);

  if (align_tool->controls)
    {
      gtk_widget_destroy (align_tool->controls);
      align_tool->controls = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
      clear_all_selected_objects (align_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_align_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpAlignTool    *align_tool  = GIMP_ALIGN_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  If the tool was being used in another image... reset it  */

  if (display != tool->display)
    {
      if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
        gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      gimp_tool_pop_status (tool, display);
      clear_all_selected_objects (align_tool);
    }

  if (! gimp_tool_control_is_active (tool->control))
    gimp_tool_control_activate (tool->control);

  tool->display = display;

  align_tool->x1 = align_tool->x0 = coords->x;
  align_tool->y1 = align_tool->y0 = coords->y;

  if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

/*
 * some rather complex logic here.  If the user clicks without modifiers,
 * then we start a new list, and use the first object in it as reference.
 * If the user clicks using Shift, or draws a rubber-band box, then
 * we add objects to the list, but do not specify which one should
 * be used as reference.
 */
static void
gimp_align_tool_button_release (GimpTool              *tool,
                                GimpCoords            *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpAlignTool    *align_tool = GIMP_ALIGN_TOOL (tool);
  GimpDisplayShell *shell      = GIMP_DISPLAY_SHELL (display->shell);
  GObject          *object     = NULL;
  GimpImage        *image      = display->image;
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
      gint         snap_distance;

      snap_distance =
        GIMP_DISPLAY_CONFIG (display->image->gimp->config)->snap_distance;

      if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), display,
                                     coords, snap_distance, snap_distance,
                                     NULL, NULL, NULL, NULL, NULL,
                                     &vectors))
        {
          object = G_OBJECT (vectors);
        }
      else if (gimp_display_shell_get_show_guides (shell) &&
               (guide = gimp_image_find_guide (display->image,
                                               coords->x, coords->y,
                                               FUNSCALEX (shell, snap_distance),
                                               FUNSCALEY (shell, snap_distance))))
        {
          object = G_OBJECT (guide);
        }
      else
        {
          if ((layer = select_layer_by_coords (display->image,
                                               coords->x, coords->y)))
            {
              object = G_OBJECT (layer);
            }
        }

      if (object)
        {
          if (! g_list_find (align_tool->selected_objects, object))
            {
              align_tool->selected_objects
                = g_list_append (align_tool->selected_objects, object);
              g_signal_connect (object, "removed",
                                G_CALLBACK (clear_selected_object),
                                (gpointer) align_tool);

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
      GList *list;

      for (list = GIMP_LIST (image->layers)->list;
           list;
           list = g_list_next (list))
        {
          GimpLayer *layer = list->data;
          gint       x0, y0, x1, y1;

          if (! gimp_item_get_visible (GIMP_ITEM (layer)))
            continue;

          gimp_item_offsets (GIMP_ITEM (layer), &x0, &y0);
          x1 = x0 + gimp_item_width (GIMP_ITEM (layer));
          y1 = y0 + gimp_item_height (GIMP_ITEM (layer));

          if (x0 < X0 || y0 < Y0 || x1 > X1 || y1 > Y1)
            continue;

          if (g_list_find (align_tool->selected_objects, layer))
            continue;

          align_tool->selected_objects
            = g_list_append (align_tool->selected_objects, layer);
          g_signal_connect (layer, "removed", G_CALLBACK (clear_selected_object),
                            (gpointer) align_tool);
        }
    }

  for (i = 0; i < ALIGN_TOOL_NUM_BUTTONS; i++)
    gtk_widget_set_sensitive (align_tool->button[i],
                              (align_tool->selected_objects != NULL));

  align_tool->x1 = align_tool->x0;
  align_tool->y1 = align_tool->y0;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_align_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *display)
{
  GimpAlignTool *align_tool = GIMP_ALIGN_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  align_tool->x1 = coords->x;
  align_tool->y1 = coords->y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_align_tool_oper_update (GimpTool        *tool,
                             GimpCoords      *coords,
                             GdkModifierType  state,
                             gboolean         proximity,
                             GimpDisplay     *display)
{
  GimpAlignTool      *align_tool  = GIMP_ALIGN_TOOL (tool);
  GimpDisplayShell   *shell       = GIMP_DISPLAY_SHELL (display->shell);
  gint                snap_distance;

  snap_distance =
    GIMP_DISPLAY_CONFIG (display->image->gimp->config)->snap_distance;

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
           (NULL != gimp_image_find_guide (display->image,
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
      GimpLayer *layer = select_layer_by_coords (display->image,
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
gimp_align_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *display)
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
      gchar    *status = NULL;
      gboolean  free_status = FALSE;

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
        gimp_tool_push_status (tool, display, status);

      if (free_status)
        g_free (status);
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

  gimp_draw_tool_draw_rectangle (draw_tool,
                                 FALSE,
                                 x, y,w, h,
                                 FALSE);

  for (list = g_list_first (align_tool->selected_objects); list;
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
              gimp_item_offsets (item, &x, &y);

              w = gimp_item_width (item);
              h = gimp_item_height (item);
            }

          gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                      x, y, MARKER_WIDTH, MARKER_WIDTH,
                                      GTK_ANCHOR_NORTH_WEST, FALSE);
          gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                      x + w, y, MARKER_WIDTH, MARKER_WIDTH,
                                      GTK_ANCHOR_NORTH_EAST, FALSE);
          gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                      x, y + h, MARKER_WIDTH, MARKER_WIDTH,
                                      GTK_ANCHOR_SOUTH_WEST, FALSE);
          gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                      x + w, y + h, MARKER_WIDTH, MARKER_WIDTH,
                                      GTK_ANCHOR_SOUTH_EAST, FALSE);
        }
      else if (GIMP_IS_GUIDE (list->data))
        {
          GimpGuide *guide = GIMP_GUIDE (list->data);
          GimpImage *image = GIMP_TOOL (draw_tool)->display->image;
          gint       x, y;
          gint       w, h;

          switch (gimp_guide_get_orientation (guide))
            {
            case GIMP_ORIENTATION_VERTICAL:
              x = gimp_guide_get_position (guide);
              h = gimp_image_get_height (image);
              gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                          x, h, MARKER_WIDTH, MARKER_WIDTH,
                                          GTK_ANCHOR_SOUTH, FALSE);
              gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                          x, 0, MARKER_WIDTH, MARKER_WIDTH,
                                          GTK_ANCHOR_NORTH, FALSE);
              break;

            case GIMP_ORIENTATION_HORIZONTAL:
              y = gimp_guide_get_position (guide);
              w = gimp_image_get_width (image);
              gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                          w, y, MARKER_WIDTH, MARKER_WIDTH,
                                          GTK_ANCHOR_EAST, FALSE);
              gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                          0, y, MARKER_WIDTH, MARKER_WIDTH,
                                          GTK_ANCHOR_WEST, FALSE);
              break;

            default:
              break;
            }
        }
    }
}

static GtkWidget *
gimp_align_tool_controls (GimpAlignTool *align_tool)
{
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *spinbutton;
  GtkWidget *combo;
  gint       n = 0;

  main_vbox = gtk_vbox_new (FALSE, 12);

  frame = gimp_frame_new (_("Align"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Relative to:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_enum_combo_box_new (GIMP_TYPE_ALIGN_REFERENCE);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              GIMP_ALIGN_REFERENCE_FIRST,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &align_tool->align_reference_type);
  gtk_widget_show (combo);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = button_with_stock (GIMP_ALIGN_LEFT, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button, _("Align left edge of target"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ALIGN_HCENTER, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button, _("Align center of target"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ALIGN_RIGHT, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button, _("Align right edge of target"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = button_with_stock (GIMP_ALIGN_TOP, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button, _("Align top edge of target"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ALIGN_VCENTER, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button, _("Align middle of target"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ALIGN_BOTTOM, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button, _("Align bottom of target"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  frame = gimp_frame_new (_("Distribute"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = button_with_stock (GIMP_ARRANGE_LEFT, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button, _("Distribute left edges of targets"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ARRANGE_HCENTER, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button,
                           _("Distribute horizontal centers of targets"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ARRANGE_RIGHT, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button,
                           _("Distribute right edges of targets"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = button_with_stock (GIMP_ARRANGE_TOP, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button, _("Distribute top edges of targets"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ARRANGE_VCENTER, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button,
                           _("Distribute vertical centers of targets"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = button_with_stock (GIMP_ARRANGE_BOTTOM, align_tool);
  align_tool->button[n++] = button;
  gimp_help_set_help_data (button, _("Distribute bottoms of targets"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Offset:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&align_tool->horz_offset_adjustment,
                                     0,
                                     -100000.,
                                     100000.,
                                     1., 20., 20., 1., 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  g_signal_connect (align_tool->horz_offset_adjustment, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &align_tool->horz_offset);
  gtk_widget_show (spinbutton);

  return main_vbox;
}


static void
do_alignment (GtkWidget *widget,
              gpointer   data)
{
  GimpAlignTool     *align_tool       = GIMP_ALIGN_TOOL (data);
  GimpAlignmentType  action;
  GimpImage         *image;
  GObject           *reference_object = NULL;
  GList             *list;
  gint               offset;

  image = GIMP_TOOL (align_tool)->display->image;
  action = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "action"));
  offset = align_tool->horz_offset;

  switch (action)
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
      offset = align_tool->horz_offset;
      break;
    }

  /* if nothing is selected, just return */
  if (g_list_length (align_tool->selected_objects) == 0)
    return;

  /* if only one object is selected, use the image as reference
   * if multiple objects are selected, use the first one as reference if
   * "set_reference" is TRUE, otherwise use NULL.
   */

  list = align_tool->selected_objects;

  switch (align_tool->align_reference_type)
    {
    case GIMP_ALIGN_REFERENCE_IMAGE:
      reference_object = G_OBJECT (image);
      break;

    case GIMP_ALIGN_REFERENCE_FIRST:
      if (g_list_length (align_tool->selected_objects) == 1)
        {
          reference_object = G_OBJECT (image);
        }
      else
        {
          if (align_tool->set_reference)
            {
              reference_object = G_OBJECT (align_tool->selected_objects->data);
              list = g_list_next (align_tool->selected_objects);
            }
          else
            {
              reference_object = NULL;
            }
        }
      break;

    case GIMP_ALIGN_REFERENCE_SELECTION:
      if (image->selection_mask)
        {
          reference_object = G_OBJECT (image->selection_mask);
        }
      else
        return;
      break;

    case GIMP_ALIGN_REFERENCE_ACTIVE_LAYER:
      if (image->active_layer)
        reference_object = G_OBJECT (image->active_layer);
      else
        return;
      break;

    case GIMP_ALIGN_REFERENCE_ACTIVE_CHANNEL:
      if (image->active_channel)
        reference_object = G_OBJECT (image->active_channel);
      else
        return;
      break;

    case GIMP_ALIGN_REFERENCE_ACTIVE_PATH:
      g_print ("reference = active path not yet handled.\n");
      return;
      break;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  gimp_image_arrange_objects (image, list,
                              action,
                              reference_object,
                              action,
                              offset);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));

  gimp_image_flush (image);
}

static GtkWidget *
button_with_stock (GimpAlignmentType  action,
                   GimpAlignTool     *align_tool)
{
  GtkWidget   *button;
  GtkWidget   *image;
  const gchar *stock_id = NULL;

  switch (action)
    {
    case GIMP_ALIGN_LEFT:
      stock_id = GIMP_STOCK_GRAVITY_WEST;
      break;
    case GIMP_ALIGN_HCENTER:
      stock_id = GIMP_STOCK_HCENTER;
      break;
    case GIMP_ALIGN_RIGHT:
      stock_id = GIMP_STOCK_GRAVITY_EAST;
      break;
    case GIMP_ALIGN_TOP:
      stock_id = GIMP_STOCK_GRAVITY_NORTH;
      break;
    case GIMP_ALIGN_VCENTER:
      stock_id = GIMP_STOCK_VCENTER;
      break;
    case GIMP_ALIGN_BOTTOM:
      stock_id = GIMP_STOCK_GRAVITY_SOUTH;
      break;
    case GIMP_ARRANGE_LEFT:
      stock_id = GIMP_STOCK_GRAVITY_WEST;
      break;
    case GIMP_ARRANGE_HCENTER:
      stock_id = GIMP_STOCK_HCENTER;
      break;
    case GIMP_ARRANGE_RIGHT:
      stock_id = GIMP_STOCK_GRAVITY_EAST;
      break;
    case GIMP_ARRANGE_TOP:
      stock_id = GIMP_STOCK_GRAVITY_NORTH;
      break;
    case GIMP_ARRANGE_VCENTER:
      stock_id = GIMP_STOCK_VCENTER;
      break;
    case GIMP_ARRANGE_BOTTOM:
      stock_id = GIMP_STOCK_GRAVITY_SOUTH;
      break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }

  button = gtk_button_new ();
  g_object_set_data (G_OBJECT (button), "action", GINT_TO_POINTER (action));

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
  gtk_misc_set_padding (GTK_MISC (image), 2, 2);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (do_alignment),
                    align_tool);

  gtk_widget_set_sensitive (button, FALSE);
  gtk_widget_show (button);

  return button;
}

static void
clear_selected_object (GObject       *object,
                       GimpAlignTool *align_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (align_tool));

  if (align_tool->selected_objects)
    g_signal_handlers_disconnect_by_func (object,
                                          G_CALLBACK (clear_selected_object),
                                          (gpointer) align_tool);

  align_tool->selected_objects = g_list_remove (align_tool->selected_objects,
                                                object);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (align_tool));
}

static void
clear_all_selected_objects (GimpAlignTool *align_tool)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (align_tool);

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_pause (draw_tool);

  while (align_tool->selected_objects)
    {
      GObject *object = G_OBJECT (g_list_first (align_tool->selected_objects)->data);

      g_signal_handlers_disconnect_by_func (object,
                                            G_CALLBACK (clear_selected_object),
                                            (gpointer) align_tool);

      align_tool->selected_objects = g_list_remove (align_tool->selected_objects,
                                                    object);
    }

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_resume (draw_tool);
}

static GimpLayer *
select_layer_by_coords (GimpImage *image,
                        gint       x,
                        gint       y)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  for (list = GIMP_LIST (image->layers)->list;
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;
      gint       off_x, off_y;
      gint       width, height;

      if (! gimp_item_get_visible (GIMP_ITEM (layer)))
        continue;

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);
      width = gimp_item_width (GIMP_ITEM (layer));
      height = gimp_item_height (GIMP_ITEM (layer));

      if (off_x <= x &&
          off_y <= y &&
          x < off_x + width &&
          y < off_y + height)
        {
          return layer;
        }
    }

  return NULL;
}
