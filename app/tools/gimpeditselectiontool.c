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

#include <stdlib.h>
#include <stdarg.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "base/boundary.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-item-list.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitem-linked.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimpprojection.h"
#include "core/gimpselection.h"
#include "core/gimpundostack.h"

#include "vectors/gimpvectors.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-selection.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpdrawtool.h"
#include "gimpeditselectiontool.h"
#include "gimptoolcontrol.h"
#include "tool_manager.h"

#include "gimp-intl.h"


#define EDIT_SELECT_SCROLL_LOCK FALSE
#define ARROW_VELOCITY          25


static void   gimp_edit_selection_tool_button_release (GimpTool              *tool,
                                                       GimpCoords            *coords,
                                                       guint32                time,
                                                       GdkModifierType        state,
                                                       GimpButtonReleaseType  release_type,
                                                       GimpDisplay           *display);
static void   gimp_edit_selection_tool_motion         (GimpTool              *tool,
                                                       GimpCoords            *coords,
                                                       guint32                time,
                                                       GdkModifierType        state,
                                                       GimpDisplay           *display);

static void   gimp_edit_selection_tool_draw           (GimpDrawTool          *tool);


G_DEFINE_TYPE (GimpEditSelectionTool, gimp_edit_selection_tool,
               GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_edit_selection_tool_parent_class


static void
gimp_edit_selection_tool_class_init (GimpEditSelectionToolClass *klass)
{
  GimpToolClass     *tool_class = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->button_release = gimp_edit_selection_tool_button_release;
  tool_class->motion         = gimp_edit_selection_tool_motion;

  draw_class->draw           = gimp_edit_selection_tool_draw;
}

static void
gimp_edit_selection_tool_init (GimpEditSelectionTool *edit_selection_tool)
{
  GimpTool *tool = GIMP_TOOL (edit_selection_tool);

  gimp_tool_control_set_scroll_lock (tool->control, EDIT_SELECT_SCROLL_LOCK);
  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_COMPRESS);

  edit_selection_tool->origx      = 0;
  edit_selection_tool->origy      = 0;

  edit_selection_tool->cumlx      = 0;
  edit_selection_tool->cumly      = 0;

  edit_selection_tool->first_move = TRUE;
}

static void
gimp_edit_selection_tool_calc_coords (GimpEditSelectionTool *edit_select,
                                      gdouble                x,
                                      gdouble                y)
{
  gdouble x1, y1;
  gdouble dx, dy;

  dx = x - edit_select->origx;
  dy = y - edit_select->origy;

  x1 = edit_select->x1 + dx;
  y1 = edit_select->y1 + dy;

  edit_select->x = (gint) floor (x1) - (edit_select->x1 - edit_select->origx);
  edit_select->y = (gint) floor (y1) - (edit_select->y1 - edit_select->origy);
}

void
gimp_edit_selection_tool_start (GimpTool          *parent_tool,
                                GimpDisplay       *display,
                                GimpCoords        *coords,
                                GimpTranslateMode  edit_mode,
                                gboolean           propagate_release)
{
  GimpEditSelectionTool *edit_select;
  GimpDisplayShell      *shell;
  GimpItem              *active_item;
  GimpChannel           *channel;
  gint                   off_x, off_y;
  const BoundSeg        *segs_in;
  const BoundSeg        *segs_out;
  gint                   num_groups;
  const gchar           *undo_desc;

  edit_select = g_object_new (GIMP_TYPE_EDIT_SELECTION_TOOL, NULL);

  edit_select->propagate_release = propagate_release;

  shell = GIMP_DISPLAY_SHELL (display->shell);

  /*  Make a check to see if it should be a floating selection translation  */
  if ((edit_mode == GIMP_TRANSLATE_MODE_MASK_TO_LAYER ||
       edit_mode == GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER) &&
      gimp_image_floating_sel (display->image))
    {
      edit_mode = GIMP_TRANSLATE_MODE_FLOATING_SEL;
    }

  if (edit_mode == GIMP_TRANSLATE_MODE_LAYER)
    {
      GimpLayer *layer = gimp_image_get_active_layer (display->image);

      if (gimp_layer_is_floating_sel (layer))
        edit_mode = GIMP_TRANSLATE_MODE_FLOATING_SEL;
    }

  edit_select->edit_mode = edit_mode;

  if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_VECTORS)
    active_item = GIMP_ITEM (gimp_image_get_active_vectors (display->image));
  else
    active_item = GIMP_ITEM (gimp_image_get_active_drawable (display->image));

  switch (edit_select->edit_mode)
    {
    case GIMP_TRANSLATE_MODE_VECTORS:
    case GIMP_TRANSLATE_MODE_CHANNEL:
    case GIMP_TRANSLATE_MODE_LAYER_MASK:
    case GIMP_TRANSLATE_MODE_LAYER:
      undo_desc = GIMP_ITEM_GET_CLASS (active_item)->translate_desc;
      break;

    case GIMP_TRANSLATE_MODE_MASK:
      undo_desc = _("Move Selection");
      break;

    default:
      undo_desc = _("Move Floating Selection");
      break;
    }

  gimp_image_undo_group_start (display->image,
                               edit_select->edit_mode ==
                               GIMP_TRANSLATE_MODE_MASK ?
                               GIMP_UNDO_GROUP_MASK :
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               undo_desc);

  gimp_item_offsets (active_item, &off_x, &off_y);

  edit_select->x = edit_select->origx = coords->x - off_x;
  edit_select->y = edit_select->origy = coords->y - off_y;

  switch (edit_select->edit_mode)
    {
    case GIMP_TRANSLATE_MODE_CHANNEL:
    case GIMP_TRANSLATE_MODE_LAYER_MASK:
      channel = GIMP_CHANNEL (active_item);
     break;

    default:
      channel = gimp_image_get_mask (display->image);
      break;
    }

  gimp_channel_boundary (channel,
                         &segs_in, &segs_out,
                         &edit_select->num_segs_in, &edit_select->num_segs_out,
                         0, 0, 0, 0);

  edit_select->segs_in = boundary_sort (segs_in, edit_select->num_segs_in,
                                        &num_groups);
  edit_select->num_segs_in += num_groups;

  edit_select->segs_out = boundary_sort (segs_out, edit_select->num_segs_out,
                                         &num_groups);
  edit_select->num_segs_out += num_groups;

  if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_VECTORS)
    {
      edit_select->x1 = 0;
      edit_select->y1 = 0;
      edit_select->x2 = display->image->width;
      edit_select->y2 = display->image->height;
    }
  else
    {
      /*  find the bounding box of the selection mask -
       *  this is used for the case of a GIMP_TRANSLATE_MODE_MASK_TO_LAYER,
       *  where the translation will result in floating the selection
       *  mask and translating the resulting layer
       */
      gimp_drawable_mask_bounds (GIMP_DRAWABLE (active_item),
                                 &edit_select->x1, &edit_select->y1,
                                 &edit_select->x2, &edit_select->y2);
    }

  gimp_edit_selection_tool_calc_coords (edit_select,
                                        edit_select->origx,
                                        edit_select->origy);

  {
    gint x1, y1, x2, y2;

    switch (edit_select->edit_mode)
      {
      case GIMP_TRANSLATE_MODE_CHANNEL:
        gimp_channel_bounds (GIMP_CHANNEL (active_item),
                             &x1, &y1, &x2, &y2);
        break;

      case GIMP_TRANSLATE_MODE_LAYER_MASK:
        gimp_channel_bounds (GIMP_CHANNEL (active_item),
                             &x1, &y1, &x2, &y2);
        x1 += off_x;
        y1 += off_y;
        x2 += off_x;
        y2 += off_y;
        break;

      case GIMP_TRANSLATE_MODE_MASK:
        gimp_channel_bounds (gimp_image_get_mask (display->image),
                             &x1, &y1, &x2, &y2);
        break;

      case GIMP_TRANSLATE_MODE_MASK_TO_LAYER:
      case GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
        x1 = edit_select->x1 + off_x;
        y1 = edit_select->y1 + off_y;
        x2 = edit_select->x2 + off_x;
        y2 = edit_select->y2 + off_y;
        break;

      case GIMP_TRANSLATE_MODE_LAYER:
      case GIMP_TRANSLATE_MODE_FLOATING_SEL:
        x1 = off_x;
        y1 = off_y;
        x2 = x1 + gimp_item_width  (active_item);
        y2 = y1 + gimp_item_height (active_item);

        if (gimp_item_get_linked (active_item))
          {
            GList *linked;
            GList *list;

            linked = gimp_image_item_list_get_list (display->image,
                                                    active_item,
                                                    GIMP_ITEM_TYPE_LAYERS,
                                                    GIMP_ITEM_SET_LINKED);

            /*  Expand the rectangle to include all linked layers as well  */
            for (list = linked; list; list = g_list_next (list))
              {
                GimpItem *item = list->data;
                gint      x3, y3;
                gint      x4, y4;

                gimp_item_offsets (item, &x3, &y3);

                x4 = x3 + gimp_item_width  (item);
                y4 = y3 + gimp_item_height (item);

                if (x3 < x1)
                  x1 = x3;
                if (y3 < y1)
                  y1 = y3;
                if (x4 > x2)
                  x2 = x4;
                if (y4 > y2)
                  y2 = y4;
              }

            g_list_free (linked);
          }
        break;

      case GIMP_TRANSLATE_MODE_VECTORS:
        {
          gdouble  xd1, yd1, xd2, yd2;

          gimp_vectors_bounds (GIMP_VECTORS (active_item),
                               &xd1, &yd1, &xd2, &yd2);

          if (gimp_item_get_linked (active_item))
            {
              /*  Expand the rectangle to include all linked layers as well  */

              GList *linked;
              GList *list;

              linked = gimp_image_item_list_get_list (display->image,
                                                      active_item,
                                                      GIMP_ITEM_TYPE_VECTORS,
                                                      GIMP_ITEM_SET_LINKED);

              for (list = linked; list; list = g_list_next (list))
                {
                  GimpItem *item = list->data;
                  gdouble   x3, y3;
                  gdouble   x4, y4;

                  gimp_vectors_bounds (GIMP_VECTORS (item), &x3, &y3, &x4, &y4);

                  if (x3 < xd1)
                    xd1 = x3;
                  if (y3 < yd1)
                    yd1 = y3;
                  if (x4 > xd2)
                    xd2 = x4;
                  if (y4 > yd2)
                    yd2 = y4;
                }
            }

          x1 = ROUND (xd1);
          y1 = ROUND (yd1);
          x2 = ROUND (xd2);
          y2 = ROUND (yd2);
        }
        break;
      }

    gimp_tool_control_set_snap_offsets (GIMP_TOOL (edit_select)->control,
                                        x1 - coords->x,
                                        y1 - coords->y,
                                        x2 - x1,
                                        y2 - y1);
  }

  gimp_tool_control_activate (GIMP_TOOL (edit_select)->control);
  GIMP_TOOL (edit_select)->display = display;

  tool_manager_push_tool (display->image->gimp, GIMP_TOOL (edit_select));

  /*  pause the current selection  */
  gimp_display_shell_selection_control (shell, GIMP_SELECTION_PAUSE);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (GIMP_TOOL (edit_select), display,
                                _("Move: "), 0, ", ", 0, NULL);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (edit_select), display);
}


static void
gimp_edit_selection_tool_button_release (GimpTool              *tool,
                                         GimpCoords            *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonReleaseType  release_type,
                                         GimpDisplay           *display)
{
  GimpEditSelectionTool *edit_select = GIMP_EDIT_SELECTION_TOOL (tool);
  GimpDisplayShell      *shell       = GIMP_DISPLAY_SHELL (display->shell);
  GimpItem              *active_item;

  /*  resume the current selection  */
  gimp_display_shell_selection_control (shell, GIMP_SELECTION_RESUME);

  gimp_tool_pop_status (tool, display);

  /*  Stop and free the selection core  */
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (edit_select));

  tool_manager_pop_tool (display->image->gimp);

  if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_VECTORS)
    active_item = GIMP_ITEM (gimp_image_get_active_vectors (display->image));
  else
    active_item = GIMP_ITEM (gimp_image_get_active_drawable (display->image));

  gimp_edit_selection_tool_calc_coords (edit_select,
                                        coords->x,
                                        coords->y);

  /* GIMP_TRANSLATE_MODE_MASK is performed here at movement end, not 'live' like
   *  the other translation types.
   */
  if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_MASK)
    {
      /* move the selection -- whether there has been movement or not!
       * (to ensure that there's something on the undo stack)
       */
      gimp_item_translate (GIMP_ITEM (gimp_image_get_mask (display->image)),
                           edit_select->cumlx,
                           edit_select->cumly,
                           TRUE);
    }

  /*  GIMP_TRANSLATE_MODE_CHANNEL and GIMP_TRANSLATE_MODE_LAYER_MASK
   *  need to be preformed after thawing the undo.
   */
  if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_CHANNEL ||
      edit_select->edit_mode == GIMP_TRANSLATE_MODE_LAYER_MASK)
    {
      /* move the channel -- whether there has been movement or not!
       * (to ensure that there's something on the undo stack)
       */
      gimp_item_translate (active_item,
                           edit_select->cumlx,
                           edit_select->cumly,
                           TRUE);
    }

  if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_VECTORS ||
      edit_select->edit_mode == GIMP_TRANSLATE_MODE_CHANNEL ||
      edit_select->edit_mode == GIMP_TRANSLATE_MODE_LAYER)
    {
      if ((release_type != GIMP_BUTTON_RELEASE_CANCEL) &&
          (edit_select->cumlx != 0 ||
           edit_select->cumly != 0))
        {
          if (gimp_item_get_linked (active_item))
            {
              /*  translate all linked channels as well  */

              GList *linked;

              linked = gimp_image_item_list_get_list (display->image,
                                                      active_item,
                                                      GIMP_ITEM_TYPE_CHANNELS,
                                                      GIMP_ITEM_SET_LINKED);

              gimp_image_item_list_translate (display->image,
                                              linked,
                                              edit_select->cumlx,
                                              edit_select->cumly,
                                              TRUE);

              g_list_free (linked);
            }
        }
    }

  gimp_image_undo_group_end (display->image);

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      /* Operation cancelled - undo the undo-group! */
      gimp_image_undo (display->image);
    }

  gimp_image_flush (display->image);

  g_free (edit_select->segs_in);
  g_free (edit_select->segs_out);

  edit_select->segs_in      = NULL;
  edit_select->segs_out     = NULL;
  edit_select->num_segs_in  = 0;
  edit_select->num_segs_out = 0;

  if (edit_select->propagate_release &&
      tool_manager_get_active (display->image->gimp))
    {
      tool_manager_button_release_active (display->image->gimp,
                                          coords, time, state,
                                          display);
    }

  g_object_unref (edit_select);
}


static void
gimp_edit_selection_tool_motion (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 guint32          time,
                                 GdkModifierType  state,
                                 GimpDisplay     *display)
{
  GimpEditSelectionTool *edit_select = GIMP_EDIT_SELECTION_TOOL (tool);
  GimpItem              *active_item;
  gint                   off_x, off_y;
  gdouble                motion_x, motion_y;

  gdk_flush ();

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_VECTORS)
    active_item = GIMP_ITEM (gimp_image_get_active_vectors (display->image));
  else
    active_item = GIMP_ITEM (gimp_image_get_active_drawable (display->image));

  gimp_item_offsets (active_item, &off_x, &off_y);

  motion_x = coords->x - off_x;
  motion_y = coords->y - off_y;

  /* now do the actual move. */

  gimp_edit_selection_tool_calc_coords (edit_select,
                                        motion_x,
                                        motion_y);

  /******************************************* adam's live move *******/
  /********************************************************************/
  {
    gint x, y;

    x = edit_select->x;
    y = edit_select->y;

    /* if there has been movement, move the selection  */
    if (edit_select->origx != x || edit_select->origy != y)
      {
        gint xoffset, yoffset;

        xoffset = x - edit_select->origx;
        yoffset = y - edit_select->origy;

        edit_select->cumlx += xoffset;
        edit_select->cumly += yoffset;

        switch (edit_select->edit_mode)
          {
          case GIMP_TRANSLATE_MODE_LAYER_MASK:
          case GIMP_TRANSLATE_MODE_MASK:
            /*  we don't do the actual edit selection move here.  */
            edit_select->origx = x;
            edit_select->origy = y;
            break;

          case GIMP_TRANSLATE_MODE_VECTORS:
          case GIMP_TRANSLATE_MODE_CHANNEL:
            edit_select->origx = x;
            edit_select->origy = y;

            /*  fallthru  */

          case GIMP_TRANSLATE_MODE_LAYER:
            /*  for CHANNEL_TRANSLATE, only translate the linked layers
             *  and vectors on-the-fly, the channel is translated
             *  on button_release.
             */
            if (edit_select->edit_mode != GIMP_TRANSLATE_MODE_CHANNEL)
              gimp_item_translate (active_item, xoffset, yoffset,
                                   edit_select->first_move);

            if (gimp_item_get_linked (active_item))
              {
                /*  translate all linked layers & vectors as well  */

                GList *linked;

                linked = gimp_image_item_list_get_list (display->image,
                                                        active_item,
                                                        GIMP_ITEM_TYPE_LAYERS |
                                                        GIMP_ITEM_TYPE_VECTORS,
                                                        GIMP_ITEM_SET_LINKED);

                gimp_image_item_list_translate (display->image,
                                                linked,
                                                xoffset, yoffset,
                                                edit_select->first_move);

                g_list_free (linked);
              }
            break;

          case GIMP_TRANSLATE_MODE_MASK_TO_LAYER:
          case GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
            if (! gimp_selection_float (gimp_image_get_mask (display->image),
                                        GIMP_DRAWABLE (active_item),
                                        gimp_get_user_context (display->image->gimp),
                                        edit_select->edit_mode ==
                                        GIMP_TRANSLATE_MODE_MASK_TO_LAYER,
                                        0, 0))
              {
                /* no region to float, abort safely */
                gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

                return;
              }

            edit_select->origx -= edit_select->x1;
            edit_select->origy -= edit_select->y1;
            edit_select->x2    -= edit_select->x1;
            edit_select->y2    -= edit_select->y1;
            edit_select->x1     = 0;
            edit_select->y1     = 0;

            edit_select->edit_mode = GIMP_TRANSLATE_MODE_FLOATING_SEL;

            active_item =
              GIMP_ITEM (gimp_image_get_active_drawable (display->image));

            /* fall through */

          case GIMP_TRANSLATE_MODE_FLOATING_SEL:
            gimp_item_translate (active_item, xoffset, yoffset,
                                 edit_select->first_move);
            break;
          }

        edit_select->first_move = FALSE;
      }

    gimp_projection_flush (display->image->projection);
  }
  /********************************************************************/
  /********************************************************************/

  gimp_tool_pop_status (tool, display);
  gimp_tool_push_status_coords (tool, display,
                                _("Move: "),
                                edit_select->cumlx,
                                ", ",
                                edit_select->cumly,
                                NULL);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_edit_selection_tool_draw (GimpDrawTool *draw_tool)
{
  GimpEditSelectionTool *edit_select = GIMP_EDIT_SELECTION_TOOL (draw_tool);
  GimpDisplay           *display       = GIMP_TOOL (draw_tool)->display;
  GimpItem              *active_item;

  if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_VECTORS)
    active_item = GIMP_ITEM (gimp_image_get_active_vectors (display->image));
  else
    active_item = GIMP_ITEM (gimp_image_get_active_drawable (display->image));

  switch (edit_select->edit_mode)
    {
    case GIMP_TRANSLATE_MODE_CHANNEL:
    case GIMP_TRANSLATE_MODE_LAYER_MASK:
    case GIMP_TRANSLATE_MODE_MASK:
      {
        gboolean floating_sel = FALSE;
        gint     off_x        = 0;
        gint     off_y        = 0;

        if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_MASK)
          {
            GimpLayer *layer = gimp_image_get_active_layer (display->image);

            if (layer)
              floating_sel = gimp_layer_is_floating_sel (layer);
          }
        else
          {
            gimp_item_offsets (active_item, &off_x, &off_y);
          }

        if (! floating_sel && edit_select->segs_in)
          {
            gimp_draw_tool_draw_boundary (draw_tool,
                                          edit_select->segs_in,
                                          edit_select->num_segs_in,
                                          edit_select->cumlx + off_x,
                                          edit_select->cumly + off_y,
                                          FALSE);
          }

        if (edit_select->segs_out)
          {
            gimp_draw_tool_draw_boundary (draw_tool,
                                          edit_select->segs_out,
                                          edit_select->num_segs_out,
                                          edit_select->cumlx + off_x,
                                          edit_select->cumly + off_y,
                                          FALSE);
          }
        else if (edit_select->edit_mode != GIMP_TRANSLATE_MODE_MASK)
          {
            gimp_draw_tool_draw_rectangle (draw_tool,
                                           FALSE,
                                           edit_select->cumlx + off_x,
                                           edit_select->cumly + off_y,
                                           active_item->width,
                                           active_item->height,
                                           FALSE);
          }
      }
      break;

    case GIMP_TRANSLATE_MODE_MASK_TO_LAYER:
    case GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
      gimp_draw_tool_draw_rectangle (draw_tool,
                                     FALSE,
                                     edit_select->x1,
                                     edit_select->y1,
                                     edit_select->x2 - edit_select->x1,
                                     edit_select->y2 - edit_select->y1,
                                     TRUE);
      break;

    case GIMP_TRANSLATE_MODE_LAYER:
      {
        GimpItem *active_item;
        gint      x1, y1, x2, y2;

        active_item = GIMP_ITEM (gimp_image_get_active_layer (display->image));

        gimp_item_offsets (active_item, &x1, &y1);

        x2 = x1 + gimp_item_width  (active_item);
        y2 = y1 + gimp_item_height (active_item);

        if (gimp_item_get_linked (active_item))
          {
            /*  Expand the rectangle to include all linked layers as well  */

            GList *linked;
            GList *list;

            linked = gimp_image_item_list_get_list (display->image,
                                                    active_item,
                                                    GIMP_ITEM_TYPE_LAYERS,
                                                    GIMP_ITEM_SET_LINKED);

            for (list = linked; list; list = g_list_next (list))
              {
                GimpItem *item = list->data;
                gint      x3, y3;
                gint      x4, y4;

                gimp_item_offsets (item, &x3, &y3);

                x4 = x3 + gimp_item_width  (item);
                y4 = y3 + gimp_item_height (item);

                if (x3 < x1)
                  x1 = x3;
                if (y3 < y1)
                  y1 = y3;
                if (x4 > x2)
                  x2 = x4;
                if (y4 > y2)
                  y2 = y4;
              }

            g_list_free (linked);
          }

        gimp_draw_tool_draw_rectangle (draw_tool,
                                       FALSE,
                                       x1, y1,
                                       x2 - x1, y2 - y1,
                                       FALSE);
      }
      break;

    case GIMP_TRANSLATE_MODE_VECTORS:
      {
        GimpItem *active_item;
        gdouble   x1, y1, x2, y2;

        active_item = GIMP_ITEM (gimp_image_get_active_vectors (display->image));

        gimp_vectors_bounds (GIMP_VECTORS (active_item), &x1, &y1, &x2, &y2);

        if (gimp_item_get_linked (active_item))
          {
            /*  Expand the rectangle to include all linked vectors as well  */

            GList *linked;
            GList *list;

            linked = gimp_image_item_list_get_list (display->image,
                                                    active_item,
                                                    GIMP_ITEM_TYPE_VECTORS,
                                                    GIMP_ITEM_SET_LINKED);

            for (list = linked; list; list = g_list_next (list))
              {
                GimpItem *item = list->data;
                gdouble   x3, y3;
                gdouble   x4, y4;

                gimp_vectors_bounds (GIMP_VECTORS (item), &x3, &y3, &x4, &y4);

                if (x3 < x1)
                  x1 = x3;
                if (y3 < y1)
                  y1 = y3;
                if (x4 > x2)
                  x2 = x4;
                if (y4 > y2)
                  y2 = y4;
              }

            g_list_free (linked);
          }

        gimp_draw_tool_draw_rectangle (draw_tool,
                                       FALSE,
                                       ROUND (x1), ROUND (y1),
                                       ROUND (x2 - x1), ROUND (y2 - y1),
                                       FALSE);
      }
      break;

    case GIMP_TRANSLATE_MODE_FLOATING_SEL:
      gimp_draw_tool_draw_boundary (draw_tool,
                                    edit_select->segs_in,
                                    edit_select->num_segs_in,
                                    edit_select->cumlx,
                                    edit_select->cumly,
                                    FALSE);
      break;
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static gint
process_event_queue_keys (GdkEventKey *kevent,
                          ... /* GdkKeyType, GdkModifierType, value ... 0 */)
{

#define FILTER_MAX_KEYS 50

  va_list          argp;
  GdkEvent        *event;
  GList           *event_list = NULL;
  GList           *list;
  guint            keys[FILTER_MAX_KEYS];
  GdkModifierType  modifiers[FILTER_MAX_KEYS];
  gint             values[FILTER_MAX_KEYS];
  gint             i      = 0;
  gint             n_keys = 0;
  gint             value  = 0;
  gboolean         done   = FALSE;
  GtkWidget       *orig_widget;

  va_start (argp, kevent);

  while (n_keys < FILTER_MAX_KEYS &&
         (keys[n_keys] = va_arg (argp, guint)) != 0)
    {
      modifiers[n_keys] = va_arg (argp, GdkModifierType);
      values[n_keys]    = va_arg (argp, gint);
      n_keys++;
    }

  va_end (argp);

  for (i = 0; i < n_keys; i++)
    if (kevent->keyval                 == keys[i] &&
        (kevent->state & modifiers[i]) == modifiers[i])
      value += values[i];

  orig_widget = gtk_get_event_widget ((GdkEvent *) kevent);

  while (gdk_events_pending () > 0 && ! done)
    {
      gboolean discard_event = FALSE;

      event = gdk_event_get ();

      if (! event || orig_widget != gtk_get_event_widget (event))
        {
          done = TRUE;
        }
      else
        {
          if (event->any.type == GDK_KEY_PRESS)
            {
              for (i = 0; i < n_keys; i++)
                if (event->key.keyval                 == keys[i] &&
                    (event->key.state & modifiers[i]) == modifiers[i])
                  {
                    discard_event = TRUE;
                    value += values[i];
                  }

              if (! discard_event)
                done = TRUE;
            }
          /* should there be more types here? */
          else if (event->any.type != GDK_KEY_RELEASE &&
                   event->any.type != GDK_MOTION_NOTIFY &&
                   event->any.type != GDK_EXPOSE)
            done = FALSE;
        }

      if (! event)
        ; /* Do nothing */
      else if (! discard_event)
        event_list = g_list_prepend (event_list, event);
      else
        gdk_event_free (event);
    }

  event_list = g_list_reverse (event_list);

  /* unget the unused events and free the list */
  for (list = event_list; list; list = g_list_next (list))
    {
      gdk_event_put ((GdkEvent *) list->data);
      gdk_event_free ((GdkEvent *) list->data);
    }

  g_list_free (event_list);

  return value;

#undef FILTER_MAX_KEYS
}

gboolean
gimp_edit_selection_tool_key_press (GimpTool    *tool,
                                    GdkEventKey *kevent,
                                    GimpDisplay *display)
{
  GimpTransformType translate_type;

  if (kevent->state & GDK_MOD1_MASK)
    translate_type = GIMP_TRANSFORM_TYPE_SELECTION;
  else if (kevent->state & GDK_CONTROL_MASK)
    translate_type = GIMP_TRANSFORM_TYPE_PATH;
  else
    translate_type = GIMP_TRANSFORM_TYPE_LAYER;

  return gimp_edit_selection_tool_translate (tool, kevent, translate_type,
                                             display);
}

gboolean
gimp_edit_selection_tool_translate (GimpTool          *tool,
                                    GdkEventKey       *kevent,
                                    GimpTransformType  translate_type,
                                    GimpDisplay       *display)
{
  gint               inc_x     = 0;
  gint               inc_y     = 0;
  GimpUndo          *undo;
  gboolean           push_undo = TRUE;
  GimpItem          *item      = NULL;
  GimpTranslateMode  edit_mode = GIMP_TRANSLATE_MODE_MASK;
  GimpUndoType       undo_type = GIMP_UNDO_GROUP_MASK;
  const gchar       *undo_desc = NULL;
  gint               velocity;

  /* bail out early if it is not an arrow key event */

  if (kevent->keyval != GDK_Left &&
      kevent->keyval != GDK_Right &&
      kevent->keyval != GDK_Up &&
      kevent->keyval != GDK_Down)
    return FALSE;

  /*  adapt arrow velocity to the zoom factor when holding <shift>  */
  velocity = (ARROW_VELOCITY /
              gimp_zoom_model_get_factor (GIMP_DISPLAY_SHELL (display->shell)->zoom));
  velocity = MAX (1.0, velocity);

  /*  check the event queue for key events with the same modifier mask
   *  as the current event, allowing only GDK_SHIFT_MASK to vary between
   *  them.
   */
  inc_x = process_event_queue_keys (kevent,
                                    GDK_Left,
                                    kevent->state | GDK_SHIFT_MASK,
                                    -1 * velocity,

                                    GDK_Left,
                                    kevent->state & ~GDK_SHIFT_MASK,
                                    -1,

                                    GDK_Right,
                                    kevent->state | GDK_SHIFT_MASK,
                                    1 * velocity,

                                    GDK_Right,
                                    kevent->state & ~GDK_SHIFT_MASK,
                                    1,

                                    0);

  inc_y = process_event_queue_keys (kevent,
                                    GDK_Up,
                                    kevent->state | GDK_SHIFT_MASK,
                                    -1 * velocity,

                                    GDK_Up,
                                    kevent->state & ~GDK_SHIFT_MASK,
                                    -1,

                                    GDK_Down,
                                    kevent->state | GDK_SHIFT_MASK,
                                    1 * velocity,

                                    GDK_Down,
                                    kevent->state & ~GDK_SHIFT_MASK,
                                    1,

                                    0);

  if (inc_x != 0 || inc_y != 0)
    {
      switch (translate_type)
        {
        case GIMP_TRANSFORM_TYPE_SELECTION:
          item = GIMP_ITEM (gimp_image_get_mask (display->image));

          edit_mode = GIMP_TRANSLATE_MODE_MASK;
          undo_type = GIMP_UNDO_GROUP_MASK;
          break;

        case GIMP_TRANSFORM_TYPE_PATH:
          item = GIMP_ITEM (gimp_image_get_active_vectors (display->image));

          edit_mode = GIMP_TRANSLATE_MODE_VECTORS;
          undo_type = GIMP_UNDO_GROUP_ITEM_DISPLACE;
          break;

        case GIMP_TRANSFORM_TYPE_LAYER:
          item = GIMP_ITEM (gimp_image_get_active_drawable (display->image));

          if (item)
            {
              if (GIMP_IS_LAYER_MASK (item))
                {
                  edit_mode = GIMP_TRANSLATE_MODE_LAYER_MASK;
                }
              else if (GIMP_IS_CHANNEL (item))
                {
                  edit_mode = GIMP_TRANSLATE_MODE_CHANNEL;
                }
              else if (gimp_layer_is_floating_sel (GIMP_LAYER (item)))
                {
                  edit_mode = GIMP_TRANSLATE_MODE_FLOATING_SEL;
                }
              else
                {
                  edit_mode = GIMP_TRANSLATE_MODE_LAYER;
                }

              undo_type = GIMP_UNDO_GROUP_ITEM_DISPLACE;
            }
          break;
        }
    }

  if (! item)
    return TRUE;

  switch (edit_mode)
    {
    case GIMP_TRANSLATE_MODE_FLOATING_SEL:
      undo_desc = _("Move Floating Selection");
      break;

    default:
      undo_desc = GIMP_ITEM_GET_CLASS (item)->translate_desc;
      break;
    }

  /* compress undo */
  undo = gimp_image_undo_can_compress (display->image, GIMP_TYPE_UNDO_STACK,
                                       undo_type);

  if (undo                                                         &&
      g_object_get_data (G_OBJECT (undo),
                         "edit-selection-tool") == (gpointer) tool &&
      g_object_get_data (G_OBJECT (undo),
                         "edit-selection-item") == (gpointer) item &&
      g_object_get_data (G_OBJECT (undo),
                         "edit-selection-type") == GINT_TO_POINTER (edit_mode))
    {
      push_undo = FALSE;
    }

  if (push_undo)
    {
      if (gimp_image_undo_group_start (display->image, undo_type, undo_desc))
        {
          undo = gimp_image_undo_can_compress (display->image,
                                               GIMP_TYPE_UNDO_STACK,
                                               undo_type);

          if (undo)
            {
              g_object_set_data (G_OBJECT (undo), "edit-selection-tool",
                                 tool);
              g_object_set_data (G_OBJECT (undo), "edit-selection-item",
                                 item);
              g_object_set_data (G_OBJECT (undo), "edit-selection-type",
                                 GINT_TO_POINTER (edit_mode));
            }
        }
    }

  switch (edit_mode)
    {
    case GIMP_TRANSLATE_MODE_LAYER_MASK:
    case GIMP_TRANSLATE_MODE_MASK:
      gimp_item_translate (item, inc_x, inc_y, push_undo);
      break;

    case GIMP_TRANSLATE_MODE_MASK_TO_LAYER:
    case GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
      /*  this won't happen  */
      break;

    case GIMP_TRANSLATE_MODE_VECTORS:
    case GIMP_TRANSLATE_MODE_CHANNEL:
    case GIMP_TRANSLATE_MODE_LAYER:
      gimp_item_translate (item, inc_x, inc_y, push_undo);

      /*  translate all linked items as well  */
      if (gimp_item_get_linked (item))
        gimp_item_linked_translate (item, inc_x, inc_y, push_undo);
      break;

    case GIMP_TRANSLATE_MODE_FLOATING_SEL:
      gimp_item_translate (item, inc_x, inc_y, push_undo);
      break;
    }

  if (push_undo)
    gimp_image_undo_group_end (display->image);
  else
    gimp_undo_refresh_preview (undo,
                               gimp_get_user_context (display->image->gimp));

  gimp_image_flush (display->image);

  return TRUE;
}
