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

#include <stdlib.h>
#include <stdarg.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/boundary.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-item-list.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitem-linked.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpprojection.h"
#include "core/gimpselection.h"
#include "core/gimpundostack.h"

#include "vectors/gimpvectors.h"

#include "widgets/gimpwidgets-utils.h"

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


#define ARROW_VELOCITY 25


typedef struct _GimpEditSelectionTool      GimpEditSelectionTool;
typedef struct _GimpEditSelectionToolClass GimpEditSelectionToolClass;

struct _GimpEditSelectionTool
{
  GimpDrawTool        parent_instance;

  gint                origx, origy;    /*  Last x and y coords               */
  gint                cumlx, cumly;    /*  Cumulative changes to x and yed   */
  gint                x, y;            /*  Current x and y coords            */
  gint                num_segs_in;     /*  Num seg in selection boundary     */
  gint                num_segs_out;    /*  Num seg in selection boundary     */
  BoundSeg           *segs_in;         /*  Pointer to the channel sel. segs  */
  BoundSeg           *segs_out;        /*  Pointer to the channel sel. segs  */

  gint                x1, y1;          /*  Bounding box of selection mask    */
  gint                x2, y2;

  gdouble             center_x;        /*  Where to draw the mark of center  */
  gdouble             center_y;

  GimpTranslateMode   edit_mode;       /*  Translate the mask or layer?      */

  GList              *live_items;      /*  Items that are transformed live   */
  GList              *delayed_items;   /*  Items that are transformed later  */

  gboolean            first_move;      /*  Don't push undos after the first  */

  gboolean            propagate_release;

  gboolean            constrain;       /*  Constrain the movement            */
  gdouble             start_x, start_y;/*  Coords when button was pressed    */
  gdouble             last_x,  last_y; /*  Previous coords sent to _motion   */
};

struct _GimpEditSelectionToolClass
{
  GimpDrawToolClass   parent_class;
};


static void       gimp_edit_selection_tool_finalize            (GObject               *object);

static void       gimp_edit_selection_tool_button_release      (GimpTool              *tool,
                                                                const GimpCoords      *coords,
                                                                guint32                time,
                                                                GdkModifierType        state,
                                                                GimpButtonReleaseType  release_type,
                                                                GimpDisplay           *display);
static void       gimp_edit_selection_tool_motion              (GimpTool              *tool,
                                                                const GimpCoords      *coords,
                                                                guint32                time,
                                                                GdkModifierType        state,
                                                                GimpDisplay           *display);
static void       gimp_edit_selection_tool_active_modifier_key (GimpTool              *tool,
                                                                GdkModifierType        key,
                                                                gboolean               press,
                                                                GdkModifierType        state,
                                                                GimpDisplay           *display);
static void       gimp_edit_selection_tool_draw                (GimpDrawTool          *tool);

static GimpItem * gimp_edit_selection_tool_get_active_item     (GimpEditSelectionTool *edit_select,
                                                                GimpImage             *image);
static void       gimp_edit_selection_tool_calc_coords         (GimpEditSelectionTool *edit_select,
                                                                gdouble                x,
                                                                gdouble                y);
static void       gimp_edit_selection_tool_start_undo_group    (GimpEditSelectionTool *edit_select,
                                                                GimpImage             *image);


G_DEFINE_TYPE (GimpEditSelectionTool, gimp_edit_selection_tool,
               GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_edit_selection_tool_parent_class


static void
gimp_edit_selection_tool_class_init (GimpEditSelectionToolClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class   = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_class   = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize          = gimp_edit_selection_tool_finalize;

  tool_class->button_release      = gimp_edit_selection_tool_button_release;
  tool_class->motion              = gimp_edit_selection_tool_motion;
  tool_class->active_modifier_key = gimp_edit_selection_tool_active_modifier_key;

  draw_class->draw                = gimp_edit_selection_tool_draw;
}

static void
gimp_edit_selection_tool_init (GimpEditSelectionTool *edit_select)
{
  GimpTool *tool = GIMP_TOOL (edit_select);

  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_COMPRESS);

  edit_select->first_move = TRUE;
}

static void
gimp_edit_selection_tool_finalize (GObject *object)
{
  GimpEditSelectionTool *edit_select = GIMP_EDIT_SELECTION_TOOL (object);

  if (edit_select->segs_in)
    {
      g_free (edit_select->segs_in);
      edit_select->segs_in     = NULL;
      edit_select->num_segs_in = 0;
    }

  if (edit_select->segs_out)
    {
      g_free (edit_select->segs_out);
      edit_select->segs_out     = NULL;
      edit_select->num_segs_out = 0;
    }

  if (edit_select->live_items)
    {
      g_list_free (edit_select->live_items);
      edit_select->live_items = NULL;
    }

  if (edit_select->delayed_items)
    {
      g_list_free (edit_select->delayed_items);
      edit_select->delayed_items = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
gimp_edit_selection_tool_start (GimpTool          *parent_tool,
                                GimpDisplay       *display,
                                const GimpCoords  *coords,
                                GimpTranslateMode  edit_mode,
                                gboolean           propagate_release)
{
  GimpEditSelectionTool *edit_select;
  GimpTool              *tool;
  GimpDisplayShell      *shell;
  GimpImage             *image;
  GimpItem              *active_item;
  GimpChannel           *channel;
  gint                   off_x, off_y;
  const BoundSeg        *segs_in;
  const BoundSeg        *segs_out;

  edit_select = g_object_new (GIMP_TYPE_EDIT_SELECTION_TOOL,
                              "tool-info", parent_tool->tool_info,
                              NULL);

  edit_select->propagate_release = propagate_release;

  tool = GIMP_TOOL (edit_select);

  shell = gimp_display_get_shell (display);
  image = gimp_display_get_image (display);

  /*  Make a check to see if it should be a floating selection translation  */
  if ((edit_mode == GIMP_TRANSLATE_MODE_MASK_TO_LAYER ||
       edit_mode == GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER) &&
      gimp_image_get_floating_selection (image))
    {
      edit_mode = GIMP_TRANSLATE_MODE_FLOATING_SEL;
    }

  if (edit_mode == GIMP_TRANSLATE_MODE_LAYER)
    {
      GimpLayer *layer = gimp_image_get_active_layer (image);

      if (gimp_layer_is_floating_sel (layer))
        edit_mode = GIMP_TRANSLATE_MODE_FLOATING_SEL;
    }

  edit_select->edit_mode = edit_mode;

  active_item = gimp_edit_selection_tool_get_active_item (edit_select, image);

  gimp_edit_selection_tool_start_undo_group (edit_select, image);

  gimp_item_get_offset (active_item, &off_x, &off_y);

  edit_select->x = edit_select->origx = coords->x - off_x;
  edit_select->y = edit_select->origy = coords->y - off_y;

  /* Remember starting point for use in constrained movement */
  edit_select->start_x = coords->x;
  edit_select->start_y = coords->y;

  edit_select->constrain = FALSE;

  if (GIMP_IS_CHANNEL (active_item))
    channel = GIMP_CHANNEL (active_item);
  else
    channel = gimp_image_get_mask (image);

  gimp_channel_boundary (channel,
                         &segs_in, &segs_out,
                         &edit_select->num_segs_in, &edit_select->num_segs_out,
                         0, 0, 0, 0);

  edit_select->segs_in = g_memdup (segs_in,
                                   edit_select->num_segs_in *
                                   sizeof (BoundSeg));

  edit_select->segs_out = g_memdup (segs_out,
                                    edit_select->num_segs_out *
                                    sizeof (BoundSeg));

  if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_VECTORS)
    {
      edit_select->x1 = 0;
      edit_select->y1 = 0;
      edit_select->x2 = gimp_image_get_width  (image);
      edit_select->y2 = gimp_image_get_height (image);
    }
  else
    {
      /*  find the bounding box of the selection mask -
       *  this is used for the case of a GIMP_TRANSLATE_MODE_MASK_TO_LAYER,
       *  where the translation will result in floating the selection
       *  mask and translating the resulting layer
       */
      gimp_item_mask_bounds (active_item,
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
      case GIMP_TRANSLATE_MODE_MASK:
      case GIMP_TRANSLATE_MODE_LAYER_MASK:
        gimp_channel_bounds (GIMP_CHANNEL (active_item),
                             &x1, &y1, &x2, &y2);
        x1 += off_x;
        y1 += off_y;
        x2 += off_x;
        y2 += off_y;
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
        x2 = x1 + gimp_item_get_width  (active_item);
        y2 = y1 + gimp_item_get_height (active_item);

        if (gimp_item_get_linked (active_item))
          {
            /*  Expand the rectangle to include all linked layers as well  */

            GList *linked;
            GList *list;

            linked = gimp_image_item_list_get_list (image,
                                                    GIMP_ITEM_TYPE_LAYERS,
                                                    GIMP_ITEM_SET_LINKED);
            linked = gimp_image_item_list_filter (linked);

            for (list = linked; list; list = g_list_next (list))
              {
                GimpItem *item = list->data;
                gint      x3, y3;
                gint      x4, y4;

                gimp_item_get_offset (item, &x3, &y3);

                x4 = x3 + gimp_item_get_width  (item);
                y4 = y3 + gimp_item_get_height (item);

                x1 = MIN (x1, x3);
                y1 = MIN (y1, y3);
                x2 = MAX (x2, x4);
                y2 = MAX (y2, y4);
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
              /*  Expand the rectangle to include all linked vectors as well  */

              GList *linked;
              GList *list;

              linked = gimp_image_item_list_get_list (image,
                                                      GIMP_ITEM_TYPE_VECTORS,
                                                      GIMP_ITEM_SET_LINKED);
              linked = gimp_image_item_list_filter (linked);

              for (list = linked; list; list = g_list_next (list))
                {
                  GimpItem *item = list->data;
                  gdouble   x3, y3;
                  gdouble   x4, y4;

                  gimp_vectors_bounds (GIMP_VECTORS (item), &x3, &y3, &x4, &y4);

                  xd1 = MIN (xd1, x3);
                  yd1 = MIN (yd1, y3);
                  xd2 = MAX (xd2, x4);
                  yd2 = MAX (yd2, y4);
                }
            }

          x1 = ROUND (floor (xd1));
          y1 = ROUND (floor (yd1));
          x2 = ROUND (ceil (xd2));
          y2 = ROUND (ceil (yd2));
        }
        break;
      }

    gimp_tool_control_set_snap_offsets (tool->control,
                                        x1 - coords->x,
                                        y1 - coords->y,
                                        x2 - x1,
                                        y2 - y1);

    /* Save where to draw the mark of the center */
    edit_select->center_x = (x1 + x2) / 2.0;
    edit_select->center_y = (y1 + y2) / 2.0;
  }

  if (gimp_item_get_linked (active_item))
    {
      switch (edit_select->edit_mode)
        {
        case GIMP_TRANSLATE_MODE_CHANNEL:
        case GIMP_TRANSLATE_MODE_LAYER:
        case GIMP_TRANSLATE_MODE_VECTORS:
          edit_select->live_items =
            gimp_image_item_list_get_list (image,
                                           GIMP_ITEM_TYPE_LAYERS |
                                           GIMP_ITEM_TYPE_VECTORS,
                                           GIMP_ITEM_SET_LINKED);
          edit_select->live_items =
            gimp_image_item_list_filter (edit_select->live_items);

          edit_select->delayed_items =
            gimp_image_item_list_get_list (image,
                                           GIMP_ITEM_TYPE_CHANNELS,
                                           GIMP_ITEM_SET_LINKED);
          edit_select->delayed_items =
            gimp_image_item_list_filter (edit_select->delayed_items);
          break;

        default:
          /* other stuff can't be linked so don't bother */
          break;
        }
    }
  else
    {
      switch (edit_select->edit_mode)
        {
        case GIMP_TRANSLATE_MODE_VECTORS:
        case GIMP_TRANSLATE_MODE_LAYER:
        case GIMP_TRANSLATE_MODE_FLOATING_SEL:
          edit_select->live_items = g_list_append (NULL, active_item);
          break;

        case GIMP_TRANSLATE_MODE_CHANNEL:
        case GIMP_TRANSLATE_MODE_LAYER_MASK:
        case GIMP_TRANSLATE_MODE_MASK:
          edit_select->delayed_items = g_list_append (NULL, active_item);
          break;

        default:
          /* MASK_TO_LAYER and MASK_COPY_TO_LAYER create a live_item later */
          break;
        }
    }

  tool_manager_push_tool (display->gimp, tool);

  gimp_tool_control_activate (tool->control);
  tool->display = display;

  /*  pause the current selection  */
  gimp_display_shell_selection_pause (shell);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, display,
                                gimp_tool_control_get_precision (tool->control),
                                _("Move: "), 0, ", ", 0, NULL);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (edit_select), display);
}


static void
gimp_edit_selection_tool_button_release (GimpTool              *tool,
                                         const GimpCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonReleaseType  release_type,
                                         GimpDisplay           *display)
{
  GimpEditSelectionTool *edit_select = GIMP_EDIT_SELECTION_TOOL (tool);
  GimpDisplayShell      *shell       = gimp_display_get_shell (display);
  GimpImage             *image       = gimp_display_get_image (display);

  /*  resume the current selection  */
  gimp_display_shell_selection_resume (shell);

  gimp_tool_pop_status (tool, display);

  gimp_tool_control_halt (tool->control);

  /*  Stop and free the selection core  */
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (edit_select));

  tool_manager_pop_tool (display->gimp);

  gimp_edit_selection_tool_calc_coords (edit_select,
                                        coords->x,
                                        coords->y);

  /* move the items -- whether there has been movement or not!
   * (to ensure that there's something on the undo stack)
   */
  gimp_image_item_list_translate (image,
                                  edit_select->delayed_items,
                                  edit_select->cumlx,
                                  edit_select->cumly,
                                  TRUE);

  gimp_image_undo_group_end (image);

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      /* Operation cancelled - undo the undo-group! */
      gimp_image_undo (image);
    }

  gimp_image_flush (image);

  if (edit_select->propagate_release &&
      tool_manager_get_active (display->gimp))
    {
      tool_manager_button_release_active (display->gimp,
                                          coords, time, state,
                                          display);
    }

  g_object_unref (edit_select);
}

static void
gimp_edit_selection_tool_update_motion (GimpEditSelectionTool *edit_select,
                                        gdouble                new_x,
                                        gdouble                new_y,
                                        GimpDisplay           *display)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (edit_select);
  GimpTool     *tool      = GIMP_TOOL (edit_select);
  GimpImage    *image     = gimp_display_get_image (display);
  GimpItem     *active_item;
  gint          off_x, off_y;
  gdouble       motion_x, motion_y;
  gint          x, y;

  gdk_flush ();

  gimp_draw_tool_pause (draw_tool);

  active_item = gimp_edit_selection_tool_get_active_item (edit_select, image);

  gimp_item_get_offset (active_item, &off_x, &off_y);

  if (edit_select->constrain)
    {
      gimp_constrain_line (edit_select->start_x, edit_select->start_y,
                           &new_x, &new_y,
                           GIMP_CONSTRAIN_LINE_45_DEGREES);
    }

  motion_x = new_x - off_x;
  motion_y = new_y - off_y;

  /* now do the actual move. */

  gimp_edit_selection_tool_calc_coords (edit_select,
                                        motion_x,
                                        motion_y);
  x = edit_select->x;
  y = edit_select->y;

  /* if there has been movement, move the selection  */
  if (edit_select->origx != x || edit_select->origy != y)
    {
      gint    xoffset;
      gint    yoffset;
      GError *error = NULL;

      xoffset = x - edit_select->origx;
      yoffset = y - edit_select->origy;

      edit_select->cumlx += xoffset;
      edit_select->cumly += yoffset;

      switch (edit_select->edit_mode)
        {
        case GIMP_TRANSLATE_MODE_LAYER_MASK:
        case GIMP_TRANSLATE_MODE_MASK:
        case GIMP_TRANSLATE_MODE_VECTORS:
        case GIMP_TRANSLATE_MODE_CHANNEL:
          edit_select->origx = x;
          edit_select->origy = y;

          /*  fallthru  */

        case GIMP_TRANSLATE_MODE_LAYER:
          gimp_image_item_list_translate (image,
                                          edit_select->live_items,
                                          xoffset, yoffset,
                                          edit_select->first_move);
          break;

        case GIMP_TRANSLATE_MODE_MASK_TO_LAYER:
        case GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
          if (! gimp_selection_float (GIMP_SELECTION (gimp_image_get_mask (image)),
                                      GIMP_DRAWABLE (active_item),
                                      gimp_get_user_context (display->gimp),
                                      edit_select->edit_mode ==
                                      GIMP_TRANSLATE_MODE_MASK_TO_LAYER,
                                      0, 0, &error))
            {
              /* no region to float, abort safely */
              gimp_message_literal (display->gimp, G_OBJECT (display),
				    GIMP_MESSAGE_WARNING,
				    error->message);
              g_clear_error (&error);
              gimp_draw_tool_resume (draw_tool);

              return;
            }

          edit_select->origx -= edit_select->x1;
          edit_select->origy -= edit_select->y1;
          edit_select->x2    -= edit_select->x1;
          edit_select->y2    -= edit_select->y1;
          edit_select->x1     = 0;
          edit_select->y1     = 0;

          edit_select->edit_mode = GIMP_TRANSLATE_MODE_FLOATING_SEL;

          active_item = gimp_edit_selection_tool_get_active_item (edit_select,
                                                                  image);

          edit_select->live_items = g_list_prepend (NULL, active_item);

          /*  fallthru  */

        case GIMP_TRANSLATE_MODE_FLOATING_SEL:
          gimp_image_item_list_translate (image,
                                          edit_select->live_items,
                                          xoffset, yoffset,
                                          edit_select->first_move);
          break;
        }

      edit_select->first_move = FALSE;
    }

  gimp_projection_flush (gimp_image_get_projection (image));

  gimp_tool_pop_status (tool, display);
  gimp_tool_push_status_coords (tool, display,
                                gimp_tool_control_get_precision (tool->control),
                                _("Move: "),
                                edit_select->cumlx,
                                ", ",
                                edit_select->cumly,
                                NULL);

  gimp_draw_tool_resume (draw_tool);
}


static void
gimp_edit_selection_tool_motion (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 GimpDisplay      *display)
{
  GimpEditSelectionTool *edit_select = GIMP_EDIT_SELECTION_TOOL (tool);

  edit_select->last_x = coords->x;
  edit_select->last_y = coords->y;

  gimp_edit_selection_tool_update_motion (edit_select,
                                          coords->x, coords->y,
                                          display);
}

static void
gimp_edit_selection_tool_active_modifier_key (GimpTool        *tool,
                                              GdkModifierType  key,
                                              gboolean         press,
                                              GdkModifierType  state,
                                              GimpDisplay     *display)
{
  GimpEditSelectionTool *edit_select = GIMP_EDIT_SELECTION_TOOL (tool);

  edit_select->constrain = (state & gimp_get_constrain_behavior_mask () ?
                            TRUE : FALSE);

  /* If we didn't came here due to a mouse release, immediately update
   * the position of the thing we move.
   */
  if (state & GDK_BUTTON1_MASK)
    {
      gimp_edit_selection_tool_update_motion (edit_select,
                                              edit_select->last_x,
                                              edit_select->last_y,
                                              display);
    }
}

static void
gimp_edit_selection_tool_draw (GimpDrawTool *draw_tool)
{
  GimpEditSelectionTool *edit_select = GIMP_EDIT_SELECTION_TOOL (draw_tool);
  GimpDisplay           *display     = GIMP_TOOL (draw_tool)->display;
  GimpImage             *image       = gimp_display_get_image (display);
  GimpItem              *active_item;

  active_item = gimp_edit_selection_tool_get_active_item (edit_select, image);

  switch (edit_select->edit_mode)
    {
    case GIMP_TRANSLATE_MODE_CHANNEL:
    case GIMP_TRANSLATE_MODE_LAYER_MASK:
    case GIMP_TRANSLATE_MODE_MASK:
      {
        gboolean floating_sel = FALSE;
        gint     off_x;
        gint     off_y;

        gimp_item_get_offset (active_item, &off_x, &off_y);

        if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_MASK)
          {
            GimpLayer *layer = gimp_image_get_active_layer (image);

            if (layer)
              floating_sel = gimp_layer_is_floating_sel (layer);
          }

        if (! floating_sel && edit_select->segs_in)
          {
            gimp_draw_tool_add_boundary (draw_tool,
                                         edit_select->segs_in,
                                         edit_select->num_segs_in,
                                         NULL,
                                         edit_select->cumlx + off_x,
                                         edit_select->cumly + off_y);
          }

        if (edit_select->segs_out)
          {
            gimp_draw_tool_add_boundary (draw_tool,
                                         edit_select->segs_out,
                                         edit_select->num_segs_out,
                                         NULL,
                                         edit_select->cumlx + off_x,
                                         edit_select->cumly + off_y);
          }
        else if (edit_select->edit_mode != GIMP_TRANSLATE_MODE_MASK)
          {
            gimp_draw_tool_add_rectangle (draw_tool,
                                          FALSE,
                                          edit_select->cumlx + off_x,
                                          edit_select->cumly + off_y,
                                          gimp_item_get_width  (active_item),
                                          gimp_item_get_height (active_item));
          }
      }
      break;

    case GIMP_TRANSLATE_MODE_MASK_TO_LAYER:
    case GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
      {
        gint off_x;
        gint off_y;

        gimp_item_get_offset (active_item, &off_x, &off_y);

        gimp_draw_tool_add_rectangle (draw_tool,
                                      FALSE,
                                      edit_select->x1 + off_x,
                                      edit_select->y1 + off_y,
                                      edit_select->x2 - edit_select->x1,
                                      edit_select->y2 - edit_select->y1);
      }
      break;

    case GIMP_TRANSLATE_MODE_LAYER:
      {
        gint x1, y1, x2, y2;

        gimp_item_get_offset (active_item, &x1, &y1);

        x2 = x1 + gimp_item_get_width  (active_item);
        y2 = y1 + gimp_item_get_height (active_item);

        if (gimp_item_get_linked (active_item))
          {
            /*  Expand the rectangle to include all linked layers as well  */

            GList *linked;
            GList *list;

            linked = gimp_image_item_list_get_list (image,
                                                    GIMP_ITEM_TYPE_LAYERS,
                                                    GIMP_ITEM_SET_LINKED);
            linked = gimp_image_item_list_filter (linked);

            for (list = linked; list; list = g_list_next (list))
              {
                GimpItem *item = list->data;
                gint      x3, y3;
                gint      x4, y4;

                gimp_item_get_offset (item, &x3, &y3);

                x4 = x3 + gimp_item_get_width  (item);
                y4 = y3 + gimp_item_get_height (item);

                x1 = MIN (x1, x3);
                y1 = MIN (y1, y3);
                x2 = MAX (x2, x4);
                y2 = MAX (y2, y4);
              }

            g_list_free (linked);
          }

        gimp_draw_tool_add_rectangle (draw_tool, FALSE,
                                      x1, y1,
                                      x2 - x1, y2 - y1);
      }
      break;

    case GIMP_TRANSLATE_MODE_VECTORS:
      {
        gdouble x1, y1, x2, y2;

        gimp_vectors_bounds (GIMP_VECTORS (active_item), &x1, &y1, &x2, &y2);

        if (gimp_item_get_linked (active_item))
          {
            /*  Expand the rectangle to include all linked vectors as well  */

            GList *linked;
            GList *list;

            linked = gimp_image_item_list_get_list (image,
                                                    GIMP_ITEM_TYPE_VECTORS,
                                                    GIMP_ITEM_SET_LINKED);
            linked = gimp_image_item_list_filter (linked);

            for (list = linked; list; list = g_list_next (list))
              {
                GimpItem *item = list->data;
                gdouble   x3, y3;
                gdouble   x4, y4;

                gimp_vectors_bounds (GIMP_VECTORS (item), &x3, &y3, &x4, &y4);

                x1 = MIN (x1, x3);
                y1 = MIN (y1, y3);
                x2 = MAX (x2, x4);
                y2 = MAX (y2, y4);
              }

            g_list_free (linked);
          }

        x1 = floor (x1);
        y1 = floor (y1);
        x2 = ceil (x2);
        y2 = ceil (y2);

        gimp_draw_tool_add_rectangle (draw_tool, FALSE,
                                      x1, y1,
                                      x2 - x1, y2 - y1);
      }
      break;

    case GIMP_TRANSLATE_MODE_FLOATING_SEL:
      gimp_draw_tool_add_boundary (draw_tool,
                                   edit_select->segs_in,
                                   edit_select->num_segs_in,
                                   NULL,
                                   edit_select->cumlx,
                                   edit_select->cumly);
      break;
    }

  /* Mark the center because we snap to it */
  gimp_draw_tool_add_handle (draw_tool,
                             GIMP_HANDLE_CROSS,
                             edit_select->center_x + edit_select->cumlx,
                             edit_select->center_y + edit_select->cumly,
                             GIMP_TOOL_HANDLE_SIZE_SMALL,
                             GIMP_TOOL_HANDLE_SIZE_SMALL,
                             GIMP_HANDLE_ANCHOR_CENTER);

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static GimpItem *
gimp_edit_selection_tool_get_active_item (GimpEditSelectionTool *edit_select,
                                          GimpImage             *image)
{
  GimpItem *active_item;

  switch (edit_select->edit_mode)
    {
    case GIMP_TRANSLATE_MODE_VECTORS:
      active_item = GIMP_ITEM (gimp_image_get_active_vectors (image));
      break;

    case GIMP_TRANSLATE_MODE_LAYER:
      active_item = GIMP_ITEM (gimp_image_get_active_layer (image));
      break;

    case GIMP_TRANSLATE_MODE_MASK:
      active_item = GIMP_ITEM (gimp_image_get_mask (image));
      break;

    default:
      active_item = GIMP_ITEM (gimp_image_get_active_drawable (image));
      break;
    }

  return active_item;
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

static void
gimp_edit_selection_tool_start_undo_group (GimpEditSelectionTool *edit_select,
                                           GimpImage             *image)
{
  GimpItem    *active_item;
  const gchar *undo_desc = NULL;

  active_item = gimp_edit_selection_tool_get_active_item (edit_select, image);

  switch (edit_select->edit_mode)
    {
    case GIMP_TRANSLATE_MODE_VECTORS:
    case GIMP_TRANSLATE_MODE_CHANNEL:
    case GIMP_TRANSLATE_MODE_LAYER_MASK:
    case GIMP_TRANSLATE_MODE_MASK:
    case GIMP_TRANSLATE_MODE_LAYER:
      undo_desc = GIMP_ITEM_GET_CLASS (active_item)->translate_desc;
      break;

    case GIMP_TRANSLATE_MODE_MASK_TO_LAYER:
    case GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
    case GIMP_TRANSLATE_MODE_FLOATING_SEL:
      undo_desc = _("Move Floating Selection");
      break;

    default:
      g_return_if_reached ();
    }

  gimp_image_undo_group_start (image,
                               edit_select->edit_mode ==
                               GIMP_TRANSLATE_MODE_MASK ?
                               GIMP_UNDO_GROUP_MASK :
                               GIMP_UNDO_GROUP_ITEM_DISPLACE,
                               undo_desc);
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
  else if (kevent->state & gimp_get_toggle_behavior_mask ())
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
  GimpImage         *image     = gimp_display_get_image (display);
  GimpItem          *item      = NULL;
  GimpTranslateMode  edit_mode = GIMP_TRANSLATE_MODE_MASK;
  GimpUndoType       undo_type = GIMP_UNDO_GROUP_MASK;
  const gchar       *undo_desc = NULL;
  gint               velocity;

  /* bail out early if it is not an arrow key event */

  if (kevent->keyval != GDK_KEY_Left &&
      kevent->keyval != GDK_KEY_Right &&
      kevent->keyval != GDK_KEY_Up &&
      kevent->keyval != GDK_KEY_Down)
    return FALSE;

  /*  adapt arrow velocity to the zoom factor when holding <shift>  */
  velocity = (ARROW_VELOCITY /
              gimp_zoom_model_get_factor (gimp_display_get_shell (display)->zoom));
  velocity = MAX (1.0, velocity);

  /*  check the event queue for key events with the same modifier mask
   *  as the current event, allowing only GDK_SHIFT_MASK to vary between
   *  them.
   */
  inc_x = process_event_queue_keys (kevent,
                                    GDK_KEY_Left,
                                    kevent->state | GDK_SHIFT_MASK,
                                    -1 * velocity,

                                    GDK_KEY_Left,
                                    kevent->state & ~GDK_SHIFT_MASK,
                                    -1,

                                    GDK_KEY_Right,
                                    kevent->state | GDK_SHIFT_MASK,
                                    1 * velocity,

                                    GDK_KEY_Right,
                                    kevent->state & ~GDK_SHIFT_MASK,
                                    1,

                                    0);

  inc_y = process_event_queue_keys (kevent,
                                    GDK_KEY_Up,
                                    kevent->state | GDK_SHIFT_MASK,
                                    -1 * velocity,

                                    GDK_KEY_Up,
                                    kevent->state & ~GDK_SHIFT_MASK,
                                    -1,

                                    GDK_KEY_Down,
                                    kevent->state | GDK_SHIFT_MASK,
                                    1 * velocity,

                                    GDK_KEY_Down,
                                    kevent->state & ~GDK_SHIFT_MASK,
                                    1,

                                    0);

  if (inc_x != 0 || inc_y != 0)
    {
      switch (translate_type)
        {
        case GIMP_TRANSFORM_TYPE_SELECTION:
          item = GIMP_ITEM (gimp_image_get_mask (image));

          edit_mode = GIMP_TRANSLATE_MODE_MASK;
          undo_type = GIMP_UNDO_GROUP_MASK;
          break;

        case GIMP_TRANSFORM_TYPE_PATH:
          item = GIMP_ITEM (gimp_image_get_active_vectors (image));

          edit_mode = GIMP_TRANSLATE_MODE_VECTORS;
          undo_type = GIMP_UNDO_GROUP_ITEM_DISPLACE;
          break;

        case GIMP_TRANSFORM_TYPE_LAYER:
          item = GIMP_ITEM (gimp_image_get_active_drawable (image));

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
  undo = gimp_image_undo_can_compress (image, GIMP_TYPE_UNDO_STACK, undo_type);

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
      if (gimp_image_undo_group_start (image, undo_type, undo_desc))
        {
          undo = gimp_image_undo_can_compress (image,
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
      if (gimp_item_get_linked (item))
        {
          gimp_item_linked_translate (item, inc_x, inc_y, push_undo);
        }
      else
        {
          gimp_item_translate (item, inc_x, inc_y, push_undo);
        }
      break;

    case GIMP_TRANSLATE_MODE_FLOATING_SEL:
      gimp_item_translate (item, inc_x, inc_y, push_undo);
      break;
    }

  if (push_undo)
    gimp_image_undo_group_end (image);
  else
    gimp_undo_refresh_preview (undo,
                               gimp_get_user_context (display->gimp));

  gimp_image_flush (image);

  return TRUE;
}
