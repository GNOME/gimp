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

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"
#include "core/gimptooloptions.h"

#include "vectors/gimpvectors.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"

#include "gimplinktool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"

#define EPSILON      3   /* move distance after which we do a box-select */
#define MARKER_WIDTH 5   /* size (in pixels) of the square handles */

/*  local function prototypes  */

static void   gimp_link_tool_finalize       (GObject               *object);

static void   gimp_link_tool_button_press   (GimpTool              *tool,
                                             GimpCoords            *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             GimpDisplay           *display);
static void   gimp_link_tool_button_release (GimpTool              *tool,
                                             GimpCoords            *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             GimpButtonReleaseType  release_tyle,
                                              GimpDisplay           *display);
static void   gimp_link_tool_motion         (GimpTool              *tool,
                                             GimpCoords            *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             GimpDisplay           *display);
static void   gimp_link_tool_oper_update    (GimpTool              *tool,
                                             GimpCoords            *coords,
                                             GdkModifierType        state,
                                             gboolean               proximity,
                                             GimpDisplay           *display);
static void   gimp_link_tool_status_update  (GimpTool              *tool,
                                             GimpDisplay           *display,
                                             GdkModifierType        state,
                                             gboolean               proximity);
static void   gimp_link_tool_cursor_update  (GimpTool              *tool,
                                             GimpCoords            *coords,
                                             GdkModifierType        state,
                                             GimpDisplay           *display);

static void   gimp_link_tool_draw           (GimpDrawTool          *draw_tool);

static GimpLayer *select_layer_by_coords     (GimpImage             *image,
                                              gint                   x,
                                              gint                   y);

G_DEFINE_TYPE (GimpLinkTool, gimp_link_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_link_tool_parent_class


void
gimp_link_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_LINK_TOOL,
                GIMP_TYPE_TOOL_OPTIONS,
                NULL,
                0,
                "gimp-link-tool",
                _("Link"),
                _("Linking Tool: Link layers and other objects"),
                N_("_Link"), "Q",
                NULL, GIMP_HELP_TOOL_LINK,
                GIMP_STOCK_CURSOR,
                data);
}

static void
gimp_link_tool_class_init (GimpLinkToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = gimp_link_tool_finalize;

  tool_class->button_press   = gimp_link_tool_button_press;
  tool_class->button_release = gimp_link_tool_button_release;
  tool_class->motion         = gimp_link_tool_motion;
  tool_class->oper_update    = gimp_link_tool_oper_update;
  tool_class->cursor_update  = gimp_link_tool_cursor_update;

  draw_tool_class->draw      = gimp_link_tool_draw;
}

static void
gimp_link_tool_init (GimpLinkTool *link_tool)
{
  GimpTool *tool = GIMP_TOOL (link_tool);

  link_tool->function         = LINK_TOOL_IDLE;

  gimp_tool_control_set_snap_to     (tool->control, FALSE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_MOVE);

}

static void
gimp_link_tool_finalize (GObject *object)
{
  GimpTool      *tool       = GIMP_TOOL (object);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (object)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (object));

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_link_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpLinkTool    *link_tool  = GIMP_LINK_TOOL (tool);

  /*  If the tool was being used in another image... reset it  */

  if (display != tool->display)
    {
      if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
        gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

      gimp_tool_pop_status (tool, display);
    }

  if (! gimp_tool_control_is_active (tool->control))
    gimp_tool_control_activate (tool->control);

  tool->display = display;

  link_tool->x1 = link_tool->x0 = coords->x;
  link_tool->y1 = link_tool->y0 = coords->y;

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

/*
 * some rather complex logic here.  If no modifiers are pressed,
 * we clear all existing links and set the selected item or items to be
 * linked.  If Shift is pressed, we add the selected items to the
 * set of linked items.  If Ctrl is pressed, we remove the selected
 * items from the set of linked items.  In all cases, we only act
 * on items that are visible.
 */
static void
gimp_link_tool_button_release (GimpTool              *tool,
                               GimpCoords            *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  GimpLinkTool *link_tool = GIMP_LINK_TOOL (tool);
  GimpImage    *image     = display->image;

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      link_tool->x1 = link_tool->x0;
      link_tool->y1 = link_tool->y0;
      return;
    }

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_LINKED,
                                   _("Link Items"));

  if (! (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))) /* start a new list */
    gimp_image_unlink_all_items (image);

  /* if mouse has moved less than EPSILON pixels since button press, select
     the nearest item, otherwise make a rubber-band rectangle */
  if (hypot (coords->x - link_tool->x0, coords->y - link_tool->y0) < EPSILON)
    {
      GimpVectors *vectors;
      GimpItem    *item            = NULL;
      gint         snap_distance;

      snap_distance =
        GIMP_DISPLAY_CONFIG (display->image->gimp->config)->snap_distance;

      if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), display,
                                     coords, snap_distance, snap_distance,
                                     NULL, NULL, NULL, NULL, NULL,
                                     &vectors))
        {
          item = GIMP_ITEM (vectors);
        }
      else
        {
          GimpLayer   *layer;

          if ((layer = select_layer_by_coords (image,
                                               coords->x, coords->y)))
            {
              item = GIMP_ITEM (layer);
            }
        }

      if (item)
        {
          if (state & GDK_CONTROL_MASK)
            gimp_item_set_linked (item, FALSE, TRUE);
          else
            gimp_item_set_linked (item, TRUE, TRUE);
        }
    }
  else  /* FIXME: look for vectors too */
    {
      gint   X0    = MIN (coords->x, link_tool->x0);
      gint   X1    = MAX (coords->x, link_tool->x0);
      gint   Y0    = MIN (coords->y, link_tool->y0);
      gint   Y1    = MAX (coords->y, link_tool->y0);
      GList *list;

      for (list = GIMP_LIST (image->layers)->list;
           list;
           list = g_list_next (list))
        {
          GimpLayer *layer = list->data;
          GimpItem  *item  = GIMP_ITEM (layer);
          gint       x0, y0, x1, y1;

          if (! gimp_item_get_visible (item))
            continue;

          gimp_item_offsets (item, &x0, &y0);
          x1 = x0 + gimp_item_width (item);
          y1 = y0 + gimp_item_height (item);

          if (x0 < X0 || y0 < Y0 || x1 > X1 || y1 > Y1)
            continue;

          if (state & GDK_CONTROL_MASK)
            gimp_item_set_linked (item, FALSE, TRUE);
          else
            gimp_item_set_linked (item, TRUE, TRUE);
        }
    }

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);

  link_tool->x1 = link_tool->x0;
  link_tool->y1 = link_tool->y0;
}

static void
gimp_link_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *display)
{
  GimpLinkTool *link_tool = GIMP_LINK_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  link_tool->x1 = coords->x;
  link_tool->y1 = coords->y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_link_tool_oper_update (GimpTool        *tool,
                             GimpCoords      *coords,
                             GdkModifierType  state,
                             gboolean         proximity,
                             GimpDisplay     *display)
{
  GimpLinkTool *link_tool      = GIMP_LINK_TOOL (tool);
  gint          snap_distance;

  snap_distance =
    GIMP_DISPLAY_CONFIG (display->image->gimp->config)->snap_distance;

  if (gimp_draw_tool_on_vectors (GIMP_DRAW_TOOL (tool), display,
                                 coords, snap_distance, snap_distance,
                                 NULL, NULL, NULL, NULL, NULL, NULL))
    {
      if (state & GDK_SHIFT_MASK)
        link_tool->function = LINK_TOOL_ADD_PATH;
      else
        link_tool->function = LINK_TOOL_PICK_PATH;
    }
  else
    {
      GimpLayer *layer = select_layer_by_coords (display->image,
                                                 coords->x, coords->y);

      if (layer)
        {
          if (state & GDK_SHIFT_MASK)
            link_tool->function = LINK_TOOL_ADD_LAYER;
          else
            link_tool->function = LINK_TOOL_PICK_LAYER;
        }
      else
        {
          link_tool->function = LINK_TOOL_IDLE;
        }
    }

  gimp_link_tool_status_update (tool, display, state, proximity);
}

static void
gimp_link_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  GimpLinkTool      *link_tool  = GIMP_LINK_TOOL (tool);
  GimpToolCursorType  tool_cursor = GIMP_TOOL_CURSOR_NONE;
  GimpCursorModifier  modifier    = GIMP_CURSOR_MODIFIER_NONE;

  if (state & GDK_SHIFT_MASK)
    {
      /* always add '+' when Shift is pressed, even if nothing is selected */
      modifier = GIMP_CURSOR_MODIFIER_PLUS;
    }
  else if (state & GDK_CONTROL_MASK)
    {
      /* always add '-' when Ctrl is pressed, even if nothing is selected */
      modifier = GIMP_CURSOR_MODIFIER_MINUS;
    }

  switch (link_tool->function)
    {
    case LINK_TOOL_IDLE:
      tool_cursor = GIMP_TOOL_CURSOR_RECT_SELECT;
      break;

    case LINK_TOOL_PICK_LAYER:
    case LINK_TOOL_ADD_LAYER:
      tool_cursor = GIMP_TOOL_CURSOR_HAND;
      break;

    case LINK_TOOL_PICK_PATH:
    case LINK_TOOL_ADD_PATH:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS;
      break;

    case LINK_TOOL_DRAG_BOX:
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
gimp_link_tool_status_update (GimpTool        *tool,
                               GimpDisplay     *display,
                               GdkModifierType  state,
                               gboolean         proximity)
{
  GimpLinkTool *link_tool = GIMP_LINK_TOOL (tool);

  gimp_tool_pop_status (tool, display);

  if (proximity)
    {
      gchar    *status = NULL;
      gboolean  free_status = FALSE;

      switch (link_tool->function)
        {
        case LINK_TOOL_IDLE:
          status = gimp_suggest_modifiers (_("Click on a layer, path or guide, "
                                             "or Click-Drag to pick several "
                                             "layers"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case LINK_TOOL_PICK_LAYER:
          status = gimp_suggest_modifiers (_("Click to link this layer"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case LINK_TOOL_ADD_LAYER:
          status = _("Click to add this layer to the linked set");
          break;

        case LINK_TOOL_PICK_PATH:
          status = gimp_suggest_modifiers (_("Click to pick this path as "
                                             "first item"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case LINK_TOOL_ADD_PATH:
          status = _("Click to add this path to the list");
          break;

        case LINK_TOOL_DRAG_BOX:
          break;
        }

      if (status)
        gimp_tool_push_status (tool, display, status);

      if (free_status)
        g_free (status);
    }
}

static void
gimp_link_tool_draw (GimpDrawTool *draw_tool)
{
  GimpLinkTool *link_tool    = GIMP_LINK_TOOL (draw_tool);
  gint          x, y, w, h;

  /* draw rubber-band rectangle */
  x = MIN (link_tool->x1, link_tool->x0);
  y = MIN (link_tool->y1, link_tool->y0);
  w = MAX (link_tool->x1, link_tool->x0) - x;
  h = MAX (link_tool->y1, link_tool->y0) - y;

  gimp_draw_tool_draw_rectangle (draw_tool,
                                 FALSE,
                                 x, y,w, h,
                                 FALSE);
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
