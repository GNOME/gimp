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

#include <stdlib.h>
#include <stdarg.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligma-utils.h"
#include "core/ligmaboundary.h"
#include "core/ligmagrouplayer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-guides.h"
#include "core/ligmaimage-item-list.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"
#include "core/ligmaprojection.h"
#include "core/ligmaselection.h"
#include "core/ligmaundostack.h"

#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-appearance.h"
#include "display/ligmadisplayshell-selection.h"
#include "display/ligmadisplayshell-transform.h"

#include "ligmadrawtool.h"
#include "ligmaeditselectiontool.h"
#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"
#include "tool_manager.h"

#include "ligma-intl.h"


#define ARROW_VELOCITY 25


typedef struct _LigmaEditSelectionTool      LigmaEditSelectionTool;
typedef struct _LigmaEditSelectionToolClass LigmaEditSelectionToolClass;

struct _LigmaEditSelectionTool
{
  LigmaDrawTool        parent_instance;

  gdouble             start_x;         /*  Coords where button was pressed   */
  gdouble             start_y;

  gint                last_x;          /*  Last x and y coords               */
  gint                last_y;

  gint                current_x;       /*  Current x and y coords            */
  gint                current_y;

  gint                cuml_x;          /*  Cumulative changes to x and y     */
  gint                cuml_y;

  gint                sel_x;           /*  Bounding box of selection mask    */
  gint                sel_y;           /*  Bounding box of selection mask    */
  gint                sel_width;
  gint                sel_height;

  gint                num_segs_in;     /*  Num seg in selection boundary     */
  gint                num_segs_out;    /*  Num seg in selection boundary     */
  LigmaBoundSeg       *segs_in;         /*  Pointer to the channel sel. segs  */
  LigmaBoundSeg       *segs_out;        /*  Pointer to the channel sel. segs  */

  gdouble             center_x;        /*  Where to draw the mark of center  */
  gdouble             center_y;

  LigmaTranslateMode   edit_mode;       /*  Translate the mask or layer?      */

  GList              *live_items;      /*  Items that are transformed live   */
  GList              *delayed_items;   /*  Items that are transformed later  */

  gboolean            first_move;      /*  Don't push undos after the first  */

  gboolean            propagate_release;

  gboolean            constrain;       /*  Constrain the movement            */

  gdouble             last_motion_x;   /*  Previous coords sent to _motion   */
  gdouble             last_motion_y;
};

struct _LigmaEditSelectionToolClass
{
  LigmaDrawToolClass   parent_class;
};


static void       ligma_edit_selection_tool_finalize            (GObject               *object);

static void       ligma_edit_selection_tool_button_release      (LigmaTool              *tool,
                                                                const LigmaCoords      *coords,
                                                                guint32                time,
                                                                GdkModifierType        state,
                                                                LigmaButtonReleaseType  release_type,
                                                                LigmaDisplay           *display);
static void       ligma_edit_selection_tool_motion              (LigmaTool              *tool,
                                                                const LigmaCoords      *coords,
                                                                guint32                time,
                                                                GdkModifierType        state,
                                                                LigmaDisplay           *display);
static void       ligma_edit_selection_tool_active_modifier_key (LigmaTool              *tool,
                                                                GdkModifierType        key,
                                                                gboolean               press,
                                                                GdkModifierType        state,
                                                                LigmaDisplay           *display);
static void       ligma_edit_selection_tool_draw                (LigmaDrawTool          *tool);

static GList    * ligma_edit_selection_tool_get_selected_items  (LigmaEditSelectionTool *edit_select,
                                                                LigmaImage             *image);
static void       ligma_edit_selection_tool_calc_coords         (LigmaEditSelectionTool *edit_select,
                                                                LigmaImage             *image,
                                                                gdouble                x,
                                                                gdouble                y);
static void       ligma_edit_selection_tool_start_undo_group    (LigmaEditSelectionTool *edit_select,
                                                                LigmaImage             *image);


G_DEFINE_TYPE (LigmaEditSelectionTool, ligma_edit_selection_tool,
               LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_edit_selection_tool_parent_class


static void
ligma_edit_selection_tool_class_init (LigmaEditSelectionToolClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class   = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_class   = LIGMA_DRAW_TOOL_CLASS (klass);

  object_class->finalize          = ligma_edit_selection_tool_finalize;

  tool_class->button_release      = ligma_edit_selection_tool_button_release;
  tool_class->motion              = ligma_edit_selection_tool_motion;
  tool_class->active_modifier_key = ligma_edit_selection_tool_active_modifier_key;

  draw_class->draw                = ligma_edit_selection_tool_draw;
}

static void
ligma_edit_selection_tool_init (LigmaEditSelectionTool *edit_select)
{
  LigmaTool *tool = LIGMA_TOOL (edit_select);

  edit_select->first_move = TRUE;

  ligma_tool_control_set_active_modifiers (tool->control,
                                          LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE);
}

static void
ligma_edit_selection_tool_finalize (GObject *object)
{
  LigmaEditSelectionTool *edit_select = LIGMA_EDIT_SELECTION_TOOL (object);

  g_clear_pointer (&edit_select->segs_in, g_free);
  edit_select->num_segs_in = 0;

  g_clear_pointer (&edit_select->segs_out, g_free);
  edit_select->num_segs_out = 0;

  g_clear_pointer (&edit_select->live_items,    g_list_free);
  g_clear_pointer (&edit_select->delayed_items, g_list_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
ligma_edit_selection_tool_start (LigmaTool          *parent_tool,
                                LigmaDisplay       *display,
                                const LigmaCoords  *coords,
                                LigmaTranslateMode  edit_mode,
                                gboolean           propagate_release)
{
  LigmaEditSelectionTool *edit_select;
  LigmaTool              *tool;
  LigmaDisplayShell      *shell;
  LigmaImage             *image;
  GList                 *selected_items;
  GList                 *iter;
  GList                 *list;
  gint                   off_x       = G_MAXINT;
  gint                   off_y       = G_MAXINT;

  edit_select = g_object_new (LIGMA_TYPE_EDIT_SELECTION_TOOL,
                              "tool-info", parent_tool->tool_info,
                              NULL);

  edit_select->propagate_release = propagate_release;

  tool = LIGMA_TOOL (edit_select);

  shell = ligma_display_get_shell (display);
  image = ligma_display_get_image (display);

  /*  Make a check to see if it should be a floating selection translation  */
  if ((edit_mode == LIGMA_TRANSLATE_MODE_MASK_TO_LAYER ||
       edit_mode == LIGMA_TRANSLATE_MODE_MASK_COPY_TO_LAYER) &&
      ligma_image_get_floating_selection (image))
    {
      edit_mode = LIGMA_TRANSLATE_MODE_FLOATING_SEL;
    }

  if (edit_mode == LIGMA_TRANSLATE_MODE_LAYER)
    {
      GList *layers = ligma_image_get_selected_layers (image);

      if (layers && ligma_layer_is_floating_sel (layers->data))
        edit_mode = LIGMA_TRANSLATE_MODE_FLOATING_SEL;
    }

  edit_select->edit_mode = edit_mode;

  ligma_edit_selection_tool_start_undo_group (edit_select, image);

  /* Remember starting point for use in constrained movement */
  edit_select->start_x = coords->x;
  edit_select->start_y = coords->y;

  selected_items = ligma_edit_selection_tool_get_selected_items (edit_select, image);
  g_return_if_fail (selected_items != NULL);

  for (iter = selected_items; iter; iter = iter->next)
    {
      gint item_off_x, item_off_y;

      ligma_item_get_offset (iter->data, &item_off_x, &item_off_y);
      off_x = MIN (off_x, item_off_x);
      off_y = MIN (off_y, item_off_y);
    }

  /* Manually set the last coords to the starting point */
  edit_select->last_x = coords->x - off_x;
  edit_select->last_y = coords->y - off_y;

  edit_select->constrain = FALSE;

  /* Find the active item's selection bounds */
  {
    LigmaChannel        *channel;
    const LigmaBoundSeg *segs_in;
    const LigmaBoundSeg *segs_out;

    if (LIGMA_IS_CHANNEL (selected_items->data))
      channel = LIGMA_CHANNEL (selected_items->data);
    else
      channel = ligma_image_get_mask (image);

    ligma_channel_boundary (channel,
                           &segs_in, &segs_out,
                           &edit_select->num_segs_in,
                           &edit_select->num_segs_out,
                           0, 0, 0, 0);

    edit_select->segs_in = g_memdup2 (segs_in,
                                      edit_select->num_segs_in *
                                      sizeof (LigmaBoundSeg));

    edit_select->segs_out = g_memdup2 (segs_out,
                                       edit_select->num_segs_out *
                                       sizeof (LigmaBoundSeg));

    if (edit_select->edit_mode == LIGMA_TRANSLATE_MODE_VECTORS)
      {
        edit_select->sel_x      = 0;
        edit_select->sel_y      = 0;
        edit_select->sel_width  = ligma_image_get_width  (image);
        edit_select->sel_height = ligma_image_get_height (image);
      }
    else
      {
        /*  find the bounding box of the selection mask - this is used
         *  for the case of a LIGMA_TRANSLATE_MODE_MASK_TO_LAYER, where
         *  the translation will result in floating the selection mask
         *  and translating the resulting layer
         */
        ligma_item_mask_intersect (selected_items->data,
                                  &edit_select->sel_x,
                                  &edit_select->sel_y,
                                  &edit_select->sel_width,
                                  &edit_select->sel_height);
      }
  }

  ligma_edit_selection_tool_calc_coords (edit_select, image,
                                        coords->x, coords->y);

  {
    gint x = 0;
    gint y = 0;
    gint w = 0;
    gint h = 0;

    switch (edit_select->edit_mode)
      {
      case LIGMA_TRANSLATE_MODE_CHANNEL:
      case LIGMA_TRANSLATE_MODE_MASK:
      case LIGMA_TRANSLATE_MODE_LAYER_MASK:
        edit_select->delayed_items = ligma_image_item_list_filter (g_list_copy (selected_items));
        ligma_image_item_list_bounds (image, edit_select->delayed_items, &x, &y, &w, &h);
        x += off_x;
        y += off_y;

        break;

      case LIGMA_TRANSLATE_MODE_MASK_TO_LAYER:
      case LIGMA_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
        /* MASK_TO_LAYER and MASK_COPY_TO_LAYER create a live_item later */
        x = edit_select->sel_x + off_x;
        y = edit_select->sel_y + off_y;
        w = edit_select->sel_width;
        h = edit_select->sel_height;
        break;

      case LIGMA_TRANSLATE_MODE_LAYER:
      case LIGMA_TRANSLATE_MODE_FLOATING_SEL:
      case LIGMA_TRANSLATE_MODE_VECTORS:
        edit_select->live_items = ligma_image_item_list_filter (g_list_copy (selected_items));
        ligma_image_item_list_bounds (image, edit_select->live_items, &x, &y, &w, &h);
        break;
      }

    ligma_tool_control_set_snap_offsets (tool->control,
                                        x - coords->x,
                                        y - coords->y,
                                        w, h);

    /* Save where to draw the mark of the center */
    edit_select->center_x = x + w / 2.0;
    edit_select->center_y = y + h / 2.0;
  }

  for (list = edit_select->live_items; list; list = g_list_next (list))
    {
      LigmaItem *item = list->data;

      ligma_viewable_preview_freeze (LIGMA_VIEWABLE (item));

      ligma_item_start_transform (item, TRUE);
    }

  g_list_free (selected_items);

  tool_manager_push_tool (display->ligma, tool);

  ligma_tool_control_activate (tool->control);
  tool->display = display;

  /*  pause the current selection  */
  ligma_display_shell_selection_pause (shell);

  /* initialize the statusbar display */
  ligma_tool_push_status_coords (tool, display,
                                ligma_tool_control_get_precision (tool->control),
                                _("Move: "), 0, ", ", 0, NULL);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (edit_select), display);
}


static void
ligma_edit_selection_tool_button_release (LigmaTool              *tool,
                                         const LigmaCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         LigmaButtonReleaseType  release_type,
                                         LigmaDisplay           *display)
{
  LigmaEditSelectionTool *edit_select = LIGMA_EDIT_SELECTION_TOOL (tool);
  LigmaDisplayShell      *shell       = ligma_display_get_shell (display);
  LigmaImage             *image       = ligma_display_get_image (display);
  GList                 *list;

  /*  resume the current selection  */
  ligma_display_shell_selection_resume (shell);

  ligma_tool_pop_status (tool, display);

  ligma_tool_control_halt (tool->control);

  /*  Stop and free the selection core  */
  ligma_draw_tool_stop (LIGMA_DRAW_TOOL (edit_select));

  tool_manager_pop_tool (display->ligma);

  /* move the items -- whether there has been movement or not!
   * (to ensure that there's something on the undo stack)
   */
  ligma_image_item_list_translate (image,
                                  edit_select->delayed_items,
                                  edit_select->cuml_x,
                                  edit_select->cuml_y,
                                  TRUE);

  for (list = edit_select->live_items; list; list = g_list_next (list))
    {
      LigmaItem *item = list->data;

      ligma_item_end_transform (item, TRUE);

      ligma_viewable_preview_thaw (LIGMA_VIEWABLE (item));
    }

  ligma_image_undo_group_end (image);

  if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
    {
      /* Operation cancelled - undo the undo-group! */
      ligma_image_undo (image);
    }

  ligma_image_flush (image);

  if (edit_select->propagate_release &&
      tool_manager_get_active (display->ligma))
    {
      tool_manager_button_release_active (display->ligma,
                                          coords, time, state,
                                          display);
    }

  g_object_unref (edit_select);
}

static void
ligma_edit_selection_tool_update_motion (LigmaEditSelectionTool *edit_select,
                                        gdouble                new_x,
                                        gdouble                new_y,
                                        LigmaDisplay           *display)
{
  LigmaDrawTool     *draw_tool = LIGMA_DRAW_TOOL (edit_select);
  LigmaTool         *tool      = LIGMA_TOOL (edit_select);
  LigmaDisplayShell *shell     = ligma_display_get_shell (display);
  LigmaImage        *image     = ligma_display_get_image (display);
  gint              dx;
  gint              dy;

  gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (shell)));

  ligma_draw_tool_pause (draw_tool);

  if (edit_select->constrain)
    {
      ligma_constrain_line (edit_select->start_x, edit_select->start_y,
                           &new_x, &new_y,
                           LIGMA_CONSTRAIN_LINE_45_DEGREES, 0.0, 1.0, 1.0);
    }

  ligma_edit_selection_tool_calc_coords (edit_select, image,
                                        new_x, new_y);

  dx = edit_select->current_x - edit_select->last_x;
  dy = edit_select->current_y - edit_select->last_y;

  /*  if there has been movement, move  */
  if (dx != 0 || dy != 0)
    {
      GList  *selected_items;
      GList  *list;
      GError *error = NULL;

      selected_items = ligma_edit_selection_tool_get_selected_items (edit_select, image);

      edit_select->cuml_x += dx;
      edit_select->cuml_y += dy;

      switch (edit_select->edit_mode)
        {
        case LIGMA_TRANSLATE_MODE_LAYER_MASK:
        case LIGMA_TRANSLATE_MODE_MASK:
        case LIGMA_TRANSLATE_MODE_VECTORS:
        case LIGMA_TRANSLATE_MODE_CHANNEL:
          edit_select->last_x = edit_select->current_x;
          edit_select->last_y = edit_select->current_y;

          /*  fallthru  */

        case LIGMA_TRANSLATE_MODE_LAYER:
          ligma_image_item_list_translate (image,
                                          edit_select->live_items,
                                          dx, dy,
                                          edit_select->first_move);
          break;

        case LIGMA_TRANSLATE_MODE_MASK_TO_LAYER:
        case LIGMA_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
          if (! ligma_selection_float (LIGMA_SELECTION (ligma_image_get_mask (image)),
                                      selected_items,
                                      ligma_get_user_context (display->ligma),
                                      edit_select->edit_mode ==
                                      LIGMA_TRANSLATE_MODE_MASK_TO_LAYER,
                                      0, 0, &error))
            {
              /* no region to float, abort safely */
              ligma_message_literal (display->ligma, G_OBJECT (display),
                                    LIGMA_MESSAGE_WARNING,
                                    error->message);
              g_clear_error (&error);
              ligma_draw_tool_resume (draw_tool);

              return;
            }

          edit_select->last_x -= edit_select->sel_x;
          edit_select->last_y -= edit_select->sel_y;
          edit_select->sel_x   = 0;
          edit_select->sel_y   = 0;

          edit_select->edit_mode = LIGMA_TRANSLATE_MODE_FLOATING_SEL;

          /* Selected items should have changed. */
          edit_select->live_items = ligma_edit_selection_tool_get_selected_items (edit_select, image);

          for (list = edit_select->live_items; list; list = g_list_next (list))
            {
              LigmaItem *item = list->data;

              ligma_viewable_preview_freeze (LIGMA_VIEWABLE (item));

              ligma_item_start_transform (item, TRUE);
            }

          /*  fallthru  */

        case LIGMA_TRANSLATE_MODE_FLOATING_SEL:
          ligma_image_item_list_translate (image,
                                          edit_select->live_items,
                                          dx, dy,
                                          edit_select->first_move);
          break;
        }

      edit_select->first_move = FALSE;

      g_list_free (selected_items);
    }

  ligma_projection_flush (ligma_image_get_projection (image));

  ligma_tool_pop_status (tool, display);
  ligma_tool_push_status_coords (tool, display,
                                ligma_tool_control_get_precision (tool->control),
                                _("Move: "),
                                edit_select->cuml_x,
                                ", ",
                                edit_select->cuml_y,
                                NULL);

  ligma_draw_tool_resume (draw_tool);
}


static void
ligma_edit_selection_tool_motion (LigmaTool         *tool,
                                 const LigmaCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 LigmaDisplay      *display)
{
  LigmaEditSelectionTool *edit_select = LIGMA_EDIT_SELECTION_TOOL (tool);

  edit_select->last_motion_x = coords->x;
  edit_select->last_motion_y = coords->y;

  ligma_edit_selection_tool_update_motion (edit_select,
                                          coords->x, coords->y,
                                          display);
}

static void
ligma_edit_selection_tool_active_modifier_key (LigmaTool        *tool,
                                              GdkModifierType  key,
                                              gboolean         press,
                                              GdkModifierType  state,
                                              LigmaDisplay     *display)
{
  LigmaEditSelectionTool *edit_select = LIGMA_EDIT_SELECTION_TOOL (tool);

  edit_select->constrain = (state & ligma_get_constrain_behavior_mask () ?
                            TRUE : FALSE);

  /* If we didn't came here due to a mouse release, immediately update
   * the position of the thing we move.
   */
  if (state & GDK_BUTTON1_MASK)
    {
      ligma_edit_selection_tool_update_motion (edit_select,
                                              edit_select->last_motion_x,
                                              edit_select->last_motion_y,
                                              display);
    }
}

static void
ligma_edit_selection_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaEditSelectionTool *edit_select = LIGMA_EDIT_SELECTION_TOOL (draw_tool);
  LigmaDisplay           *display     = LIGMA_TOOL (draw_tool)->display;
  LigmaImage             *image       = ligma_display_get_image (display);
  GList                 *selected_items;
  GList                 *iter;
  gint                   off_x       = G_MAXINT;
  gint                   off_y       = G_MAXINT;

  selected_items = ligma_edit_selection_tool_get_selected_items (edit_select, image);
  g_return_if_fail (selected_items != NULL);

  for (iter = selected_items; iter; iter = iter->next)
    {
      gint item_off_x, item_off_y;

      ligma_item_get_offset (iter->data, &item_off_x, &item_off_y);
      off_x = MIN (off_x, item_off_x);
      off_y = MIN (off_y, item_off_y);
    }

  switch (edit_select->edit_mode)
    {
    case LIGMA_TRANSLATE_MODE_CHANNEL:
    case LIGMA_TRANSLATE_MODE_LAYER_MASK:
    case LIGMA_TRANSLATE_MODE_MASK:
      {
        gboolean floating_sel = FALSE;

        if (edit_select->edit_mode == LIGMA_TRANSLATE_MODE_MASK)
          {
            GList *layers = ligma_image_get_selected_layers (image);

            if (g_list_length (layers) == 1)
              floating_sel = ligma_layer_is_floating_sel (layers->data);
          }

        if (! floating_sel && edit_select->segs_in)
          {
            ligma_draw_tool_add_boundary (draw_tool,
                                         edit_select->segs_in,
                                         edit_select->num_segs_in,
                                         NULL,
                                         edit_select->cuml_x + off_x,
                                         edit_select->cuml_y + off_y);
          }

        if (edit_select->segs_out)
          {
            ligma_draw_tool_add_boundary (draw_tool,
                                         edit_select->segs_out,
                                         edit_select->num_segs_out,
                                         NULL,
                                         edit_select->cuml_x + off_x,
                                         edit_select->cuml_y + off_y);
          }
        else if (edit_select->edit_mode != LIGMA_TRANSLATE_MODE_MASK)
          {
            ligma_draw_tool_add_rectangle (draw_tool,
                                          FALSE,
                                          edit_select->cuml_x + off_x,
                                          edit_select->cuml_y + off_y,
                                          ligma_item_get_width  (selected_items->data),
                                          ligma_item_get_height (selected_items->data));
          }
      }
      break;

    case LIGMA_TRANSLATE_MODE_MASK_TO_LAYER:
    case LIGMA_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
      ligma_draw_tool_add_rectangle (draw_tool,
                                    FALSE,
                                    edit_select->sel_x + off_x,
                                    edit_select->sel_y + off_y,
                                    edit_select->sel_width,
                                    edit_select->sel_height);
      break;

    case LIGMA_TRANSLATE_MODE_LAYER:
    case LIGMA_TRANSLATE_MODE_VECTORS:
        {
          GList *translate_items;
          gint   x, y, w, h;

          translate_items = ligma_image_item_list_filter (g_list_copy (selected_items));
          ligma_image_item_list_bounds (image, translate_items, &x, &y, &w, &h);
          g_list_free (translate_items);

          ligma_draw_tool_add_rectangle (draw_tool, FALSE, x, y, w, h);
        }
      break;

    case LIGMA_TRANSLATE_MODE_FLOATING_SEL:
      if (edit_select->segs_in)
        {
          ligma_draw_tool_add_boundary (draw_tool,
                                       edit_select->segs_in,
                                       edit_select->num_segs_in,
                                       NULL,
                                       edit_select->cuml_x,
                                       edit_select->cuml_y);
        }
      break;
    }

  /* Mark the center because we snap to it */
  ligma_draw_tool_add_handle (draw_tool,
                             LIGMA_HANDLE_CROSS,
                             edit_select->center_x + edit_select->cuml_x,
                             edit_select->center_y + edit_select->cuml_y,
                             LIGMA_TOOL_HANDLE_SIZE_SMALL,
                             LIGMA_TOOL_HANDLE_SIZE_SMALL,
                             LIGMA_HANDLE_ANCHOR_CENTER);

  LIGMA_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  g_list_free (selected_items);
}

static GList *
ligma_edit_selection_tool_get_selected_items (LigmaEditSelectionTool *edit_select,
                                             LigmaImage             *image)
{
  GList *selected_items;

  switch (edit_select->edit_mode)
    {
    case LIGMA_TRANSLATE_MODE_VECTORS:
      selected_items = g_list_copy (ligma_image_get_selected_vectors (image));
      break;

    case LIGMA_TRANSLATE_MODE_LAYER:
      selected_items = g_list_copy (ligma_image_get_selected_layers (image));
      break;

    case LIGMA_TRANSLATE_MODE_MASK:
      selected_items = g_list_prepend (NULL, ligma_image_get_mask (image));
      break;

    default:
      selected_items = ligma_image_get_selected_drawables (image);
      break;
    }

  return selected_items;
}

static void
ligma_edit_selection_tool_calc_coords (LigmaEditSelectionTool *edit_select,
                                      LigmaImage             *image,
                                      gdouble                x,
                                      gdouble                y)
{
  GList    *selected_items;
  GList    *iter;
  gint      off_x = G_MAXINT;
  gint      off_y = G_MAXINT;
  gdouble   x1, y1;
  gdouble   dx, dy;

  selected_items = ligma_edit_selection_tool_get_selected_items (edit_select, image);
  g_return_if_fail (selected_items != NULL);

  for (iter = selected_items; iter; iter = iter->next)
    {
      gint item_off_x, item_off_y;

      ligma_item_get_offset (iter->data, &item_off_x, &item_off_y);
      off_x = MIN (off_x, item_off_x);
      off_y = MIN (off_y, item_off_y);
    }

  g_list_free (selected_items);

  dx = (x - off_x) - edit_select->last_x;
  dy = (y - off_y) - edit_select->last_y;

  x1 = edit_select->sel_x + dx;
  y1 = edit_select->sel_y + dy;

  edit_select->current_x = ((gint) floor (x1) -
                            (edit_select->sel_x - edit_select->last_x));
  edit_select->current_y = ((gint) floor (y1) -
                            (edit_select->sel_y - edit_select->last_y));
}

static void
ligma_edit_selection_tool_start_undo_group (LigmaEditSelectionTool *edit_select,
                                           LigmaImage             *image)
{
  GList       *selected_items;
  const gchar *undo_desc = NULL;

  switch (edit_select->edit_mode)
    {
    case LIGMA_TRANSLATE_MODE_VECTORS:
    case LIGMA_TRANSLATE_MODE_CHANNEL:
    case LIGMA_TRANSLATE_MODE_LAYER_MASK:
    case LIGMA_TRANSLATE_MODE_MASK:
    case LIGMA_TRANSLATE_MODE_LAYER:
      selected_items = ligma_edit_selection_tool_get_selected_items (edit_select, image);
      undo_desc = LIGMA_ITEM_GET_CLASS (selected_items->data)->translate_desc;
      g_list_free (selected_items);
      break;

    case LIGMA_TRANSLATE_MODE_MASK_TO_LAYER:
    case LIGMA_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
    case LIGMA_TRANSLATE_MODE_FLOATING_SEL:
      undo_desc = _("Move Floating Selection");
      break;

    default:
      g_return_if_reached ();
    }

  ligma_image_undo_group_start (image,
                               edit_select->edit_mode ==
                               LIGMA_TRANSLATE_MODE_MASK ?
                               LIGMA_UNDO_GROUP_MASK :
                               LIGMA_UNDO_GROUP_ITEM_DISPLACE,
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
ligma_edit_selection_tool_key_press (LigmaTool    *tool,
                                    GdkEventKey *kevent,
                                    LigmaDisplay *display)
{
  LigmaTransformType translate_type;

  if (kevent->state & GDK_MOD1_MASK)
    {
      translate_type = LIGMA_TRANSFORM_TYPE_SELECTION;
    }
  else if (kevent->state & ligma_get_toggle_behavior_mask ())
    {
      translate_type = LIGMA_TRANSFORM_TYPE_PATH;
    }
  else
    {
      translate_type = LIGMA_TRANSFORM_TYPE_LAYER;
    }

  return ligma_edit_selection_tool_translate (tool, kevent, translate_type,
                                             display, NULL);
}

gboolean
ligma_edit_selection_tool_translate (LigmaTool          *tool,
                                    GdkEventKey       *kevent,
                                    LigmaTransformType  translate_type,
                                    LigmaDisplay       *display,
                                    GtkWidget        **type_box)
{
  gint               inc_x          = 0;
  gint               inc_y          = 0;
  LigmaUndo          *undo;
  gboolean           push_undo      = TRUE;
  LigmaImage         *image          = ligma_display_get_image (display);
  LigmaItem          *active_item    = NULL;
  GList             *selected_items = NULL;
  LigmaTranslateMode  edit_mode      = LIGMA_TRANSLATE_MODE_MASK;
  LigmaUndoType       undo_type      = LIGMA_UNDO_GROUP_MASK;
  const gchar       *undo_desc      = NULL;
  GdkModifierType    extend_mask    = ligma_get_extend_selection_mask ();
  const gchar       *null_message   = NULL;
  const gchar       *locked_message = NULL;
  LigmaItem          *locked_item    = NULL;
  GList             *iter;
  gint               velocity;

  /* bail out early if it is not an arrow key event */

  if (kevent->keyval != GDK_KEY_Left &&
      kevent->keyval != GDK_KEY_Right &&
      kevent->keyval != GDK_KEY_Up &&
      kevent->keyval != GDK_KEY_Down)
    return FALSE;

  /*  adapt arrow velocity to the zoom factor when holding <shift>  */
  velocity = (ARROW_VELOCITY /
              ligma_zoom_model_get_factor (ligma_display_get_shell (display)->zoom));
  velocity = MAX (1.0, velocity);

  /*  check the event queue for key events with the same modifier mask
   *  as the current event, allowing only extend_mask to vary between
   *  them.
   */
  inc_x = process_event_queue_keys (kevent,
                                    GDK_KEY_Left,
                                    kevent->state | extend_mask,
                                    -1 * velocity,

                                    GDK_KEY_Left,
                                    kevent->state & ~extend_mask,
                                    -1,

                                    GDK_KEY_Right,
                                    kevent->state | extend_mask,
                                    1 * velocity,

                                    GDK_KEY_Right,
                                    kevent->state & ~extend_mask,
                                    1,

                                    0);

  inc_y = process_event_queue_keys (kevent,
                                    GDK_KEY_Up,
                                    kevent->state | extend_mask,
                                    -1 * velocity,

                                    GDK_KEY_Up,
                                    kevent->state & ~extend_mask,
                                    -1,

                                    GDK_KEY_Down,
                                    kevent->state | extend_mask,
                                    1 * velocity,

                                    GDK_KEY_Down,
                                    kevent->state & ~extend_mask,
                                    1,

                                    0);

  switch (translate_type)
    {
    case LIGMA_TRANSFORM_TYPE_SELECTION:
      active_item = LIGMA_ITEM (ligma_image_get_mask (image));

      if (ligma_channel_is_empty (LIGMA_CHANNEL (active_item)))
        active_item = NULL;

      edit_mode = LIGMA_TRANSLATE_MODE_MASK;
      undo_type = LIGMA_UNDO_GROUP_MASK;

      if (! active_item)
        {
          /* cannot happen, don't translate this message */
          null_message = "There is no selection to move.";
        }
      else if (ligma_item_is_position_locked (active_item,
                                             &locked_item))
        {
          /* cannot happen, don't translate this message */
          locked_message = "The selection's position is locked.";
        }
      break;

    case LIGMA_TRANSFORM_TYPE_PATH:
      selected_items = ligma_image_get_selected_vectors (image);
      selected_items = g_list_copy (selected_items);

      edit_mode = LIGMA_TRANSLATE_MODE_VECTORS;
      undo_type = LIGMA_UNDO_GROUP_ITEM_DISPLACE;

      if (selected_items == NULL)
        {
          null_message = _("There are no paths to move.");
        }
      else
        {
          for (iter = selected_items; iter; iter = iter->next)
            if (ligma_item_is_position_locked (iter->data, NULL))
              {
                locked_item    = iter->data;
                locked_message = _("A selected path's position is locked.");
                break;
              }
        }
      break;

    case LIGMA_TRANSFORM_TYPE_LAYER:
      selected_items = ligma_image_get_selected_drawables (image);

      undo_type = LIGMA_UNDO_GROUP_ITEM_DISPLACE;

      if (! selected_items)
        {
          null_message = _("There is no layer to move.");
        }
      else if (LIGMA_IS_LAYER_MASK (selected_items->data))
        {
          edit_mode = LIGMA_TRANSLATE_MODE_LAYER_MASK;

          if (ligma_item_is_position_locked (selected_items->data,
                                            &locked_item))
            {
              locked_message = _("The selected layer's position is locked.");
            }
          else if (ligma_item_is_content_locked (selected_items->data,
                                                &locked_item))
            {
              locked_message = _("The selected layer's pixels are locked.");
            }
        }
      else if (LIGMA_IS_CHANNEL (selected_items->data))
        {
          edit_mode = LIGMA_TRANSLATE_MODE_CHANNEL;

          for (iter = selected_items; iter; iter = iter->next)
            if (ligma_item_is_position_locked (iter->data, NULL))
              {
                locked_item    = iter->data;
                locked_message = _("A selected channel's position is locked.");
                break;
              }
        }
      else if (ligma_layer_is_floating_sel (selected_items->data))
        {
          edit_mode = LIGMA_TRANSLATE_MODE_FLOATING_SEL;

          if (ligma_item_is_position_locked (selected_items->data,
                                            &locked_item))
            {
              locked_message = _("The selected layer's position is locked.");
            }
        }
      else
        {
          edit_mode = LIGMA_TRANSLATE_MODE_LAYER;

          for (iter = selected_items; iter; iter = iter->next)
            if (ligma_item_is_position_locked (iter->data, NULL))
              {
                locked_item    = iter->data;
                locked_message = _("A selected layer's position is locked.");
                break;
              }
        }

      break;

    case LIGMA_TRANSFORM_TYPE_IMAGE:
      g_return_val_if_reached (FALSE);
    }

  if (! active_item && ! selected_items)
    {
      ligma_tool_message_literal (tool, display, null_message);
      if (type_box)
        {
          ligma_tools_show_tool_options (display->ligma);
          if (*type_box)
            ligma_widget_blink (*type_box);
        }
      return TRUE;
    }
  else if (locked_message)
    {
      ligma_tool_message_literal (tool, display, locked_message);

      if (locked_item == NULL)
        locked_item = active_item ? active_item : selected_items->data;

      ligma_tools_blink_lock_box (display->ligma, locked_item);

      g_list_free (selected_items);
      return TRUE;
    }

  if (inc_x == 0 && inc_y == 0)
    {
      g_list_free (selected_items);
      return TRUE;
    }
  else if (! selected_items)
    {
      selected_items = g_list_prepend (NULL, active_item);
    }

  switch (edit_mode)
    {
    case LIGMA_TRANSLATE_MODE_FLOATING_SEL:
      undo_desc = _("Move Floating Selection");
      break;

    default:
      undo_desc = LIGMA_ITEM_GET_CLASS (selected_items->data)->translate_desc;
      break;
    }

  /* compress undo */
  undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_UNDO_STACK, undo_type);

  if (undo &&
      g_object_get_data (G_OBJECT (undo),
                         "edit-selection-tool") == (gpointer) tool &&
      g_object_get_data (G_OBJECT (undo),
                         "edit-selection-type") == GINT_TO_POINTER (edit_mode))
    {
      GList *undo_items;

      undo_items = g_object_get_data (G_OBJECT (undo), "edit-selection-items");

      if (ligma_image_equal_selected_drawables (image, undo_items))
        {
          push_undo = FALSE;
        }
    }

  if (push_undo)
    {
      if (ligma_image_undo_group_start (image, undo_type, undo_desc))
        {
          undo = ligma_image_undo_can_compress (image,
                                               LIGMA_TYPE_UNDO_STACK,
                                               undo_type);

          if (undo)
            {
              g_object_set_data (G_OBJECT (undo), "edit-selection-tool",
                                 tool);
              g_object_set_data_full (G_OBJECT (undo), "edit-selection-items",
                                      g_list_copy (selected_items),
                                      (GDestroyNotify) g_list_free);
              g_object_set_data (G_OBJECT (undo), "edit-selection-type",
                                 GINT_TO_POINTER (edit_mode));
            }
        }
    }

  ligma_image_item_list_translate (ligma_item_get_image (selected_items->data),
                                  selected_items, inc_x, inc_y, push_undo);

  if (push_undo)
    ligma_image_undo_group_end (image);
  else
    ligma_undo_refresh_preview (undo,
                               ligma_get_user_context (display->ligma));

  ligma_image_flush (image);
  g_list_free (selected_items);

  return TRUE;
}
