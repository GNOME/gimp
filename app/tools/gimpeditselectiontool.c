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

#include <stdlib.h>
#include <stdarg.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpboundary.h"
#include "core/gimpgrouplayer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-item-list.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpprojection.h"
#include "core/gimpselection.h"
#include "core/gimpundostack.h"

#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-selection.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpdrawtool.h"
#include "gimpeditselectiontool.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"
#include "tool_manager.h"

#include "gimp-intl.h"


#define ARROW_VELOCITY 25


typedef struct _GimpEditSelectionTool      GimpEditSelectionTool;
typedef struct _GimpEditSelectionToolClass GimpEditSelectionToolClass;

struct _GimpEditSelectionTool
{
  GimpDrawTool        parent_instance;

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
  GimpBoundSeg       *segs_in;         /*  Pointer to the channel sel. segs  */
  GimpBoundSeg       *segs_out;        /*  Pointer to the channel sel. segs  */

  gdouble             center_x;        /*  Where to draw the mark of center  */
  gdouble             center_y;

  GimpTranslateMode   edit_mode;       /*  Translate the mask or layer?      */

  GList              *live_items;      /*  Items that are transformed live   */
  GList              *delayed_items;   /*  Items that are transformed later  */

  gboolean            first_move;      /*  Don't push undos after the first  */

  gboolean            propagate_release;

  gboolean            constrain;       /*  Constrain the movement            */

  gdouble             last_motion_x;   /*  Previous coords sent to _motion   */
  gdouble             last_motion_y;
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

static GList    * gimp_edit_selection_tool_get_selected_items  (GimpEditSelectionTool *edit_select,
                                                                GimpImage             *image);
static void       gimp_edit_selection_tool_calc_coords         (GimpEditSelectionTool *edit_select,
                                                                GimpImage             *image,
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

  edit_select->first_move = TRUE;

  gimp_tool_control_set_active_modifiers (tool->control,
                                          GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE);
}

static void
gimp_edit_selection_tool_finalize (GObject *object)
{
  GimpEditSelectionTool *edit_select = GIMP_EDIT_SELECTION_TOOL (object);

  g_clear_pointer (&edit_select->segs_in, g_free);
  edit_select->num_segs_in = 0;

  g_clear_pointer (&edit_select->segs_out, g_free);
  edit_select->num_segs_out = 0;

  g_clear_pointer (&edit_select->live_items,    g_list_free);
  g_clear_pointer (&edit_select->delayed_items, g_list_free);

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
  GList                 *selected_items;
  GList                 *iter;
  GList                 *list;
  gint                   off_x       = G_MAXINT;
  gint                   off_y       = G_MAXINT;

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
      GList *layers = gimp_image_get_selected_layers (image);

      if (layers && gimp_layer_is_floating_sel (layers->data))
        edit_mode = GIMP_TRANSLATE_MODE_FLOATING_SEL;
    }

  edit_select->edit_mode = edit_mode;

  gimp_edit_selection_tool_start_undo_group (edit_select, image);

  /* Remember starting point for use in constrained movement */
  edit_select->start_x = coords->x;
  edit_select->start_y = coords->y;

  selected_items = gimp_edit_selection_tool_get_selected_items (edit_select, image);
  g_return_if_fail (selected_items != NULL);

  for (iter = selected_items; iter; iter = iter->next)
    {
      gint item_off_x, item_off_y;

      gimp_item_get_offset (iter->data, &item_off_x, &item_off_y);
      off_x = MIN (off_x, item_off_x);
      off_y = MIN (off_y, item_off_y);
    }

  /* Manually set the last coords to the starting point */
  edit_select->last_x = coords->x - off_x;
  edit_select->last_y = coords->y - off_y;

  edit_select->constrain = FALSE;

  /* Find the active item's selection bounds */
  {
    GimpChannel        *channel;
    const GimpBoundSeg *segs_in;
    const GimpBoundSeg *segs_out;

    if (GIMP_IS_CHANNEL (selected_items->data))
      channel = GIMP_CHANNEL (selected_items->data);
    else
      channel = gimp_image_get_mask (image);

    gimp_channel_boundary (channel,
                           &segs_in, &segs_out,
                           &edit_select->num_segs_in,
                           &edit_select->num_segs_out,
                           0, 0, 0, 0);

    edit_select->segs_in = g_memdup2 (segs_in,
                                      edit_select->num_segs_in *
                                      sizeof (GimpBoundSeg));

    edit_select->segs_out = g_memdup2 (segs_out,
                                       edit_select->num_segs_out *
                                       sizeof (GimpBoundSeg));

    if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_VECTORS)
      {
        edit_select->sel_x      = 0;
        edit_select->sel_y      = 0;
        edit_select->sel_width  = gimp_image_get_width  (image);
        edit_select->sel_height = gimp_image_get_height (image);
      }
    else
      {
        /*  find the bounding box of the selection mask - this is used
         *  for the case of a GIMP_TRANSLATE_MODE_MASK_TO_LAYER, where
         *  the translation will result in floating the selection mask
         *  and translating the resulting layer
         */
        gimp_item_mask_intersect (selected_items->data,
                                  &edit_select->sel_x,
                                  &edit_select->sel_y,
                                  &edit_select->sel_width,
                                  &edit_select->sel_height);
      }
  }

  gimp_edit_selection_tool_calc_coords (edit_select, image,
                                        coords->x, coords->y);

  {
    gint x = 0;
    gint y = 0;
    gint w = 0;
    gint h = 0;

    switch (edit_select->edit_mode)
      {
      case GIMP_TRANSLATE_MODE_CHANNEL:
      case GIMP_TRANSLATE_MODE_MASK:
      case GIMP_TRANSLATE_MODE_LAYER_MASK:
        edit_select->delayed_items = gimp_image_item_list_filter (g_list_copy (selected_items));
        gimp_image_item_list_bounds (image, edit_select->delayed_items, &x, &y, &w, &h);
        x += off_x;
        y += off_y;

        break;

      case GIMP_TRANSLATE_MODE_MASK_TO_LAYER:
      case GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
        /* MASK_TO_LAYER and MASK_COPY_TO_LAYER create a live_item later */
        x = edit_select->sel_x + off_x;
        y = edit_select->sel_y + off_y;
        w = edit_select->sel_width;
        h = edit_select->sel_height;
        break;

      case GIMP_TRANSLATE_MODE_LAYER:
      case GIMP_TRANSLATE_MODE_FLOATING_SEL:
      case GIMP_TRANSLATE_MODE_VECTORS:
        edit_select->live_items = gimp_image_item_list_filter (g_list_copy (selected_items));
        gimp_image_item_list_bounds (image, edit_select->live_items, &x, &y, &w, &h);
        break;
      }

    gimp_tool_control_set_snap_offsets (tool->control,
                                        x - coords->x,
                                        y - coords->y,
                                        w, h);

    /* Save where to draw the mark of the center */
    edit_select->center_x = x + w / 2.0;
    edit_select->center_y = y + h / 2.0;
  }

  for (list = edit_select->live_items; list; list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_viewable_preview_freeze (GIMP_VIEWABLE (item));

      gimp_item_start_transform (item, TRUE);
    }

  g_list_free (selected_items);

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
  GList                 *list;

  /*  resume the current selection  */
  gimp_display_shell_selection_resume (shell);

  gimp_tool_pop_status (tool, display);

  gimp_tool_control_halt (tool->control);

  /*  Stop and free the selection core  */
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (edit_select));

  tool_manager_pop_tool (display->gimp);

  /* move the items -- whether there has been movement or not!
   * (to ensure that there's something on the undo stack)
   */
  gimp_image_item_list_translate (image,
                                  edit_select->delayed_items,
                                  edit_select->cuml_x,
                                  edit_select->cuml_y,
                                  TRUE);

  for (list = edit_select->live_items; list; list = g_list_next (list))
    {
      GimpItem *item = list->data;

      gimp_item_end_transform (item, TRUE);

      gimp_viewable_preview_thaw (GIMP_VIEWABLE (item));
    }

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

  shell->equidistance_side_horizontal = GIMP_ARRANGE_HFILL;
  shell->equidistance_side_vertical   = GIMP_ARRANGE_HFILL;
  shell->snapped_side_horizontal      = GIMP_ARRANGE_HFILL;
  shell->snapped_side_vertical        = GIMP_ARRANGE_HFILL;
}

static void
gimp_edit_selection_tool_update_motion (GimpEditSelectionTool *edit_select,
                                        gdouble                new_x,
                                        gdouble                new_y,
                                        GimpDisplay           *display)
{
  GimpDrawTool     *draw_tool = GIMP_DRAW_TOOL (edit_select);
  GimpTool         *tool      = GIMP_TOOL (edit_select);
  GimpDisplayShell *shell     = gimp_display_get_shell (display);
  GimpImage        *image     = gimp_display_get_image (display);
  gint              dx;
  gint              dy;

  gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (shell)));

  gimp_draw_tool_pause (draw_tool);

  if (edit_select->constrain)
    {
      gimp_constrain_line (edit_select->start_x, edit_select->start_y,
                           &new_x, &new_y,
                           GIMP_CONSTRAIN_LINE_45_DEGREES, 0.0, 1.0, 1.0);
    }

  gimp_edit_selection_tool_calc_coords (edit_select, image,
                                        new_x, new_y);

  dx = edit_select->current_x - edit_select->last_x;
  dy = edit_select->current_y - edit_select->last_y;

  /*  if there has been movement, move  */
  if (dx != 0 || dy != 0)
    {
      GList  *selected_items;
      GList  *list;
      GError *error = NULL;

      selected_items = gimp_edit_selection_tool_get_selected_items (edit_select, image);

      edit_select->cuml_x += dx;
      edit_select->cuml_y += dy;

      switch (edit_select->edit_mode)
        {
        case GIMP_TRANSLATE_MODE_LAYER_MASK:
        case GIMP_TRANSLATE_MODE_MASK:
        case GIMP_TRANSLATE_MODE_VECTORS:
        case GIMP_TRANSLATE_MODE_CHANNEL:
          edit_select->last_x = edit_select->current_x;
          edit_select->last_y = edit_select->current_y;

          /*  fallthru  */

        case GIMP_TRANSLATE_MODE_LAYER:
          gimp_image_item_list_translate (image,
                                          edit_select->live_items,
                                          dx, dy,
                                          edit_select->first_move);
          break;

        case GIMP_TRANSLATE_MODE_MASK_TO_LAYER:
        case GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
          if (! gimp_selection_float (GIMP_SELECTION (gimp_image_get_mask (image)),
                                      selected_items,
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

          edit_select->last_x -= edit_select->sel_x;
          edit_select->last_y -= edit_select->sel_y;
          edit_select->sel_x   = 0;
          edit_select->sel_y   = 0;

          edit_select->edit_mode = GIMP_TRANSLATE_MODE_FLOATING_SEL;

          /* Selected items should have changed. */
          edit_select->live_items = gimp_edit_selection_tool_get_selected_items (edit_select, image);

          for (list = edit_select->live_items; list; list = g_list_next (list))
            {
              GimpItem *item = list->data;

              gimp_viewable_preview_freeze (GIMP_VIEWABLE (item));

              gimp_item_start_transform (item, TRUE);
            }

          /*  fallthru  */

        case GIMP_TRANSLATE_MODE_FLOATING_SEL:
          gimp_image_item_list_translate (image,
                                          edit_select->live_items,
                                          dx, dy,
                                          edit_select->first_move);
          break;
        }

      edit_select->first_move = FALSE;

      g_list_free (selected_items);
    }

  gimp_projection_flush (gimp_image_get_projection (image));

  gimp_tool_pop_status (tool, display);
  gimp_tool_push_status_coords (tool, display,
                                gimp_tool_control_get_precision (tool->control),
                                _("Move: "),
                                edit_select->cuml_x,
                                ", ",
                                edit_select->cuml_y,
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

  edit_select->last_motion_x = coords->x;
  edit_select->last_motion_y = coords->y;

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
                                              edit_select->last_motion_x,
                                              edit_select->last_motion_y,
                                              display);
    }
}

static void
gimp_edit_selection_tool_draw (GimpDrawTool *draw_tool)
{
  GimpEditSelectionTool *edit_select  = GIMP_EDIT_SELECTION_TOOL (draw_tool);
  GimpDisplay           *display      = GIMP_TOOL (draw_tool)->display;
  GimpImage             *image        = gimp_display_get_image (display);
  GimpDisplayShell      *shell        = gimp_display_get_shell (display);
  GimpUnit              *unit         = gimp_display_shell_get_unit (shell);
  const gchar           *abbreviation = gimp_unit_get_abbreviation (unit);
  GList                 *selected_items;
  GList                 *iter;
  gdouble                xresolution, yresolution;
  gint                   off_x        = G_MAXINT;
  gint                   off_y        = G_MAXINT;

  gimp_image_get_resolution (image, &xresolution, &yresolution);
  selected_items = gimp_edit_selection_tool_get_selected_items (edit_select, image);
  g_return_if_fail (selected_items != NULL);

  for (iter = selected_items; iter; iter = iter->next)
    {
      gint item_off_x, item_off_y;

      gimp_item_get_offset (iter->data, &item_off_x, &item_off_y);
      off_x = MIN (off_x, item_off_x);
      off_y = MIN (off_y, item_off_y);
    }

  switch (edit_select->edit_mode)
    {
    case GIMP_TRANSLATE_MODE_CHANNEL:
    case GIMP_TRANSLATE_MODE_LAYER_MASK:
    case GIMP_TRANSLATE_MODE_MASK:
      {
        gboolean floating_sel = FALSE;

        if (edit_select->edit_mode == GIMP_TRANSLATE_MODE_MASK)
          {
            GList *layers = gimp_image_get_selected_layers (image);

            if (g_list_length (layers) == 1)
              floating_sel = gimp_layer_is_floating_sel (layers->data);
          }

        if (! floating_sel && edit_select->segs_in)
          {
            gimp_draw_tool_add_boundary (draw_tool,
                                         edit_select->segs_in,
                                         edit_select->num_segs_in,
                                         NULL,
                                         edit_select->cuml_x + off_x,
                                         edit_select->cuml_y + off_y);
          }

        if (edit_select->segs_out)
          {
            gimp_draw_tool_add_boundary (draw_tool,
                                         edit_select->segs_out,
                                         edit_select->num_segs_out,
                                         NULL,
                                         edit_select->cuml_x + off_x,
                                         edit_select->cuml_y + off_y);
          }
        else if (edit_select->edit_mode != GIMP_TRANSLATE_MODE_MASK)
          {
            gimp_draw_tool_add_rectangle (draw_tool,
                                          FALSE,
                                          edit_select->cuml_x + off_x,
                                          edit_select->cuml_y + off_y,
                                          gimp_item_get_width  (selected_items->data),
                                          gimp_item_get_height (selected_items->data));
          }
      }
      break;

    case GIMP_TRANSLATE_MODE_MASK_TO_LAYER:
    case GIMP_TRANSLATE_MODE_MASK_COPY_TO_LAYER:
      gimp_draw_tool_add_rectangle (draw_tool,
                                    FALSE,
                                    edit_select->sel_x + off_x,
                                    edit_select->sel_y + off_y,
                                    edit_select->sel_width,
                                    edit_select->sel_height);
      break;

    case GIMP_TRANSLATE_MODE_LAYER:
    case GIMP_TRANSLATE_MODE_VECTORS:
        {
          GList             *translate_items;
          GimpAlignmentType  snapped_side_horizontal      = shell->snapped_side_horizontal;
          GimpAlignmentType  snapped_side_vertical        = shell->snapped_side_vertical;
          GimpAlignmentType  equidistance_side_horizontal = shell->equidistance_side_horizontal;
          GimpAlignmentType  equidistance_side_vertical   = shell->equidistance_side_vertical;
          gint               x, y, w, h;

          translate_items = gimp_image_item_list_filter (g_list_copy (selected_items));
          gimp_image_item_list_bounds (image, translate_items, &x, &y, &w, &h);
          g_list_free (translate_items);

          gimp_draw_tool_add_rectangle (draw_tool, FALSE, x, y, w, h);

          if (snapped_side_horizontal != GIMP_ARRANGE_HFILL)
            {
              GimpLayer *snapped_layer_horizontal = shell->snapped_layer_horizontal;
              gint       gx, gy, gw, gh;

              gimp_item_bounds (GIMP_ITEM (snapped_layer_horizontal), &gx, &gy, &gw, &gh);
              gimp_item_get_offset (GIMP_ITEM (snapped_layer_horizontal), &gx, &gy);

              switch (snapped_side_horizontal)
                {
                case GIMP_ALIGN_LEFT:
                  gimp_draw_tool_add_line (draw_tool, x, y+h/2, x, gy+gh/2);
                  break;
                case GIMP_ALIGN_RIGHT:
                  gimp_draw_tool_add_line (draw_tool, x+w, y+h/2, x+w, gy+gh/2);
                  break;
                case GIMP_ALIGN_VCENTER:
                  gimp_draw_tool_add_line (draw_tool, x+w/2, y+h/2, x+w/2, gy+gh/2);
                  break;

                default:
                  break;
                }
            }

          if (snapped_side_vertical != GIMP_ARRANGE_HFILL)
            {
              GimpLayer *snapped_layer_vertical = shell->snapped_layer_vertical;
              gint       gx, gy, gw, gh;

              gimp_item_bounds (GIMP_ITEM (snapped_layer_vertical), &gx, &gy, &gw, &gh);
              gimp_item_get_offset (GIMP_ITEM (snapped_layer_vertical), &gx, &gy);

              switch (snapped_side_vertical)
                {
                case GIMP_ALIGN_TOP:
                  gimp_draw_tool_add_line (draw_tool, x+w/2, y, gx+gw/2, y);
                  break;
                case GIMP_ALIGN_BOTTOM:
                  gimp_draw_tool_add_line (draw_tool, x+w/2, y+h, gx+gw/2, y+h);
                  break;
                case GIMP_ALIGN_HCENTER:
                  gimp_draw_tool_add_line (draw_tool, x+w/2, y+h/2, gx+gw/2, y+h/2);
                  break;

                default:
                  break;
                }
            }

          if (equidistance_side_horizontal != GIMP_ARRANGE_HFILL)
            {
              GimpLayer       *near_layer1 = shell->near_layer_horizontal1;
              GimpLayer       *near_layer2 = shell->near_layer_horizontal2;
              GtkStyleContext *style       = gtk_widget_get_style_context (GTK_WIDGET (shell));
              gdouble          pixels_to_units;
              gdouble          font_size;
              gint             gx1 = 0;
              gint             gy1 = 0;
              gint             gw1 = 0;
              gint             gh1 = 0;
              gint             gx2 = 0;
              gint             gy2 = 0;
              gint             gw2 = 0;
              gint             gh2 = 0;
              gint             distance;
              gchar            distance_string[32];

              gtk_style_context_get (style, gtk_style_context_get_state (style),
                                     "font-size", &font_size, NULL);

              gimp_item_bounds (GIMP_ITEM (near_layer1), &gx1, &gy1, &gw1, &gh1);
              gimp_item_get_offset (GIMP_ITEM (near_layer1), &gx1, &gy1);

              gimp_item_bounds (GIMP_ITEM (near_layer2), &gx2, &gy2, &gw2, &gh2);
              gimp_item_get_offset (GIMP_ITEM (near_layer2), &gx2, &gy2);

              gimp_draw_tool_add_rectangle (draw_tool, FALSE, gx1, gy1, gw1, gh1);
              gimp_draw_tool_add_rectangle (draw_tool, FALSE, gx2, gy2, gw2, gh2);

              switch (equidistance_side_horizontal)
                {
                case GIMP_ALIGN_LEFT:
                  distance = x - (gx1+gw1);
                  pixels_to_units = gimp_pixels_to_units ((gdouble) distance, unit, xresolution);

                  gimp_draw_tool_add_line (draw_tool, x, y+h/2, gx1+gw1, y+h/2);
                  gimp_draw_tool_add_line (draw_tool, gx1, gy1+gh1/2, gx2+gw2, gy1+gh1/2);

                  g_sprintf (distance_string, "%g %s", pixels_to_units, abbreviation);
                  gimp_draw_tool_add_text (draw_tool, x - distance/2, y+h/2, font_size, distance_string);
                  gimp_draw_tool_add_text (draw_tool, gx1 - distance/2, gy1+gh1/2, font_size, distance_string);

                  break;

                case GIMP_ALIGN_RIGHT:
                  distance = gx1 - (x+w);
                  pixels_to_units = gimp_pixels_to_units ((gdouble) distance, unit, xresolution);

                  gimp_draw_tool_add_line (draw_tool, x+w, y+h/2, gx1, y+h/2);
                  gimp_draw_tool_add_line (draw_tool, gx1+gw1, gy1+gh1/2, gx2, gy1+gh1/2);

                  g_sprintf (distance_string, "%g %s", pixels_to_units, abbreviation);
                  gimp_draw_tool_add_text (draw_tool, (x+w) + distance/2, y+h/2, font_size, distance_string);
                  gimp_draw_tool_add_text (draw_tool, (gx1+gw1) + distance/2, gy1+gh1/2, font_size, distance_string);

                  break;

                default:
                  break;
                }
            }

          if (equidistance_side_vertical != GIMP_ARRANGE_HFILL)
            {
              GimpLayer       *near_layer1 = shell->near_layer_vertical1;
              GimpLayer       *near_layer2 = shell->near_layer_vertical2;
              GtkStyleContext *style       = gtk_widget_get_style_context (GTK_WIDGET (shell));
              gdouble          pixels_to_units;
              gdouble          font_size;
              gint             gx1 = 0;
              gint             gy1 = 0;
              gint             gw1 = 0;
              gint             gh1 = 0;
              gint             gx2 = 0;
              gint             gy2 = 0;
              gint             gw2 = 0;
              gint             gh2 = 0;
              gint             distance;
              gchar            distance_string[32];

              gtk_style_context_get (style, gtk_style_context_get_state (style),
                                     "font-size", &font_size, NULL);

              gimp_item_bounds (GIMP_ITEM (near_layer1), &gx1, &gy1, &gw1, &gh1);
              gimp_item_get_offset (GIMP_ITEM (near_layer1), &gx1, &gy1);

              gimp_item_bounds (GIMP_ITEM (near_layer2), &gx2, &gy2, &gw2, &gh2);
              gimp_item_get_offset (GIMP_ITEM (near_layer2), &gx2, &gy2);

              gimp_draw_tool_add_rectangle (draw_tool, FALSE, gx1, gy1, gw1, gh1);
              gimp_draw_tool_add_rectangle (draw_tool, FALSE, gx2, gy2, gw2, gh2);

              switch (equidistance_side_vertical)
                {
                case GIMP_ALIGN_TOP:
                  distance = y - (gy1+gh1);
                  pixels_to_units = gimp_pixels_to_units ((gdouble) distance, unit, yresolution);

                  gimp_draw_tool_add_line (draw_tool, x+w/2, y, x+w/2, gy1+gh1);
                  gimp_draw_tool_add_line (draw_tool, gx1+gw1/2, gy1, gx1+gw1/2, gy2+gh2);

                  g_sprintf (distance_string, "%g %s", pixels_to_units, abbreviation);
                  gimp_draw_tool_add_text (draw_tool, x+w/2, y - distance/2, font_size, distance_string);
                  gimp_draw_tool_add_text (draw_tool, gx1+gw1/2, gy1 - distance/2, font_size, distance_string);

                  break;

                case GIMP_ALIGN_BOTTOM:
                  distance = gy1 - (y+h);
                  pixels_to_units = gimp_pixels_to_units ((gdouble) distance, unit, yresolution);

                  gimp_draw_tool_add_line (draw_tool, x+w/2, y+h, x+w/2, gy1);
                  gimp_draw_tool_add_line (draw_tool, gx1+gw1/2, gy1+gh1, gx1+gw1/2, gy2);

                  g_sprintf (distance_string, "%g %s", pixels_to_units, abbreviation);
                  gimp_draw_tool_add_text (draw_tool, x+w/2, (y+h) + distance/2, font_size, distance_string);
                  gimp_draw_tool_add_text (draw_tool, gx1+gw1/2, (gy1+gh1) + distance/2, font_size, distance_string);

                  break;

                default:
                  break;
                }
            }
        }
      break;

    case GIMP_TRANSLATE_MODE_FLOATING_SEL:
      if (edit_select->segs_in)
        {
          gimp_draw_tool_add_boundary (draw_tool,
                                       edit_select->segs_in,
                                       edit_select->num_segs_in,
                                       NULL,
                                       edit_select->cuml_x,
                                       edit_select->cuml_y);
        }
      break;
    }

  /* Mark the center because we snap to it */
  gimp_draw_tool_add_handle (draw_tool,
                             GIMP_HANDLE_CROSS,
                             edit_select->center_x + edit_select->cuml_x,
                             edit_select->center_y + edit_select->cuml_y,
                             GIMP_TOOL_HANDLE_SIZE_SMALL,
                             GIMP_TOOL_HANDLE_SIZE_SMALL,
                             GIMP_HANDLE_ANCHOR_CENTER);

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  g_list_free (selected_items);
}

static GList *
gimp_edit_selection_tool_get_selected_items (GimpEditSelectionTool *edit_select,
                                             GimpImage             *image)
{
  GList *selected_items;

  switch (edit_select->edit_mode)
    {
    case GIMP_TRANSLATE_MODE_VECTORS:
      selected_items = g_list_copy (gimp_image_get_selected_paths (image));
      break;

    case GIMP_TRANSLATE_MODE_LAYER:
      selected_items = g_list_copy (gimp_image_get_selected_layers (image));
      break;

    case GIMP_TRANSLATE_MODE_MASK:
      selected_items = g_list_prepend (NULL, gimp_image_get_mask (image));
      break;

    default:
      selected_items = gimp_image_get_selected_drawables (image);
      break;
    }

  return selected_items;
}

static void
gimp_edit_selection_tool_calc_coords (GimpEditSelectionTool *edit_select,
                                      GimpImage             *image,
                                      gdouble                x,
                                      gdouble                y)
{
  GList    *selected_items;
  GList    *iter;
  gint      off_x = G_MAXINT;
  gint      off_y = G_MAXINT;
  gdouble   x1, y1;
  gdouble   dx, dy;

  selected_items = gimp_edit_selection_tool_get_selected_items (edit_select, image);
  g_return_if_fail (selected_items != NULL);

  for (iter = selected_items; iter; iter = iter->next)
    {
      gint item_off_x, item_off_y;

      gimp_item_get_offset (iter->data, &item_off_x, &item_off_y);
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
gimp_edit_selection_tool_start_undo_group (GimpEditSelectionTool *edit_select,
                                           GimpImage             *image)
{
  GList       *selected_items;
  const gchar *undo_desc = NULL;

  switch (edit_select->edit_mode)
    {
    case GIMP_TRANSLATE_MODE_VECTORS:
    case GIMP_TRANSLATE_MODE_CHANNEL:
    case GIMP_TRANSLATE_MODE_LAYER_MASK:
    case GIMP_TRANSLATE_MODE_MASK:
    case GIMP_TRANSLATE_MODE_LAYER:
      selected_items = gimp_edit_selection_tool_get_selected_items (edit_select, image);
      undo_desc = GIMP_ITEM_GET_CLASS (selected_items->data)->translate_desc;
      g_list_free (selected_items);
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
    {
      translate_type = GIMP_TRANSFORM_TYPE_SELECTION;
    }
  else if (kevent->state & gimp_get_toggle_behavior_mask ())
    {
      translate_type = GIMP_TRANSFORM_TYPE_PATH;
    }
  else
    {
      translate_type = GIMP_TRANSFORM_TYPE_LAYER;
    }

  return gimp_edit_selection_tool_translate (tool, kevent, translate_type,
                                             display, NULL);
}

gboolean
gimp_edit_selection_tool_translate (GimpTool          *tool,
                                    GdkEventKey       *kevent,
                                    GimpTransformType  translate_type,
                                    GimpDisplay       *display,
                                    GtkWidget        **type_box)
{
  gint               inc_x          = 0;
  gint               inc_y          = 0;
  GimpUndo          *undo;
  gboolean           push_undo      = TRUE;
  GimpImage         *image          = gimp_display_get_image (display);
  GimpItem          *active_item    = NULL;
  GList             *selected_items = NULL;
  GimpTranslateMode  edit_mode      = GIMP_TRANSLATE_MODE_MASK;
  GimpUndoType       undo_type      = GIMP_UNDO_GROUP_MASK;
  const gchar       *undo_desc      = NULL;
  GdkModifierType    extend_mask    = gimp_get_extend_selection_mask ();
  const gchar       *null_message   = NULL;
  const gchar       *locked_message = NULL;
  GimpItem          *locked_item    = NULL;
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
              gimp_zoom_model_get_factor (gimp_display_get_shell (display)->zoom));
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
    case GIMP_TRANSFORM_TYPE_SELECTION:
      active_item = GIMP_ITEM (gimp_image_get_mask (image));

      if (gimp_channel_is_empty (GIMP_CHANNEL (active_item)))
        active_item = NULL;

      edit_mode = GIMP_TRANSLATE_MODE_MASK;
      undo_type = GIMP_UNDO_GROUP_MASK;

      if (! active_item)
        {
          /* cannot happen, don't translate this message */
          null_message = "There is no selection to move.";
        }
      else if (gimp_item_is_position_locked (active_item,
                                             &locked_item))
        {
          /* cannot happen, don't translate this message */
          locked_message = "The selection's position is locked.";
        }
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      selected_items = gimp_image_get_selected_paths (image);
      selected_items = g_list_copy (selected_items);

      edit_mode = GIMP_TRANSLATE_MODE_VECTORS;
      undo_type = GIMP_UNDO_GROUP_ITEM_DISPLACE;

      if (selected_items == NULL)
        {
          null_message = _("There are no paths to move.");
        }
      else
        {
          for (iter = selected_items; iter; iter = iter->next)
            if (gimp_item_is_position_locked (iter->data, NULL))
              {
                locked_item    = iter->data;
                locked_message = _("A selected path's position is locked.");
                break;
              }
        }
      break;

    case GIMP_TRANSFORM_TYPE_LAYER:
      selected_items = gimp_image_get_selected_drawables (image);

      undo_type = GIMP_UNDO_GROUP_ITEM_DISPLACE;

      if (! selected_items)
        {
          null_message = _("There is no layer to move.");
        }
      else if (GIMP_IS_LAYER_MASK (selected_items->data))
        {
          edit_mode = GIMP_TRANSLATE_MODE_LAYER_MASK;

          if (gimp_item_is_position_locked (selected_items->data,
                                            &locked_item))
            {
              locked_message = _("The selected layer's position is locked.");
            }
          else if (gimp_item_is_content_locked (selected_items->data,
                                                &locked_item))
            {
              locked_message = _("The selected layer's pixels are locked.");
            }
        }
      else if (GIMP_IS_CHANNEL (selected_items->data))
        {
          edit_mode = GIMP_TRANSLATE_MODE_CHANNEL;

          for (iter = selected_items; iter; iter = iter->next)
            if (gimp_item_is_position_locked (iter->data, NULL))
              {
                locked_item    = iter->data;
                locked_message = _("A selected channel's position is locked.");
                break;
              }
        }
      else if (gimp_layer_is_floating_sel (selected_items->data))
        {
          edit_mode = GIMP_TRANSLATE_MODE_FLOATING_SEL;

          if (gimp_item_is_position_locked (selected_items->data,
                                            &locked_item))
            {
              locked_message = _("The selected layer's position is locked.");
            }
        }
      else
        {
          edit_mode = GIMP_TRANSLATE_MODE_LAYER;

          for (iter = selected_items; iter; iter = iter->next)
            if (gimp_item_is_position_locked (iter->data, NULL))
              {
                locked_item    = iter->data;
                locked_message = _("A selected layer's position is locked.");
                break;
              }
        }

      break;

    case GIMP_TRANSFORM_TYPE_IMAGE:
      g_return_val_if_reached (FALSE);
    }

  if (! active_item && ! selected_items)
    {
      gimp_tool_message_literal (tool, display, null_message);
      if (type_box)
        {
          gimp_tools_show_tool_options (display->gimp);
          if (*type_box)
            gimp_widget_blink (*type_box);
        }
      return TRUE;
    }
  else if (locked_message)
    {
      gimp_tool_message_literal (tool, display, locked_message);

      if (locked_item == NULL)
        locked_item = active_item ? active_item : selected_items->data;

      gimp_tools_blink_lock_box (display->gimp, locked_item);

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
    case GIMP_TRANSLATE_MODE_FLOATING_SEL:
      undo_desc = _("Move Floating Selection");
      break;

    default:
      undo_desc = GIMP_ITEM_GET_CLASS (selected_items->data)->translate_desc;
      break;
    }

  /* compress undo */
  undo = gimp_image_undo_can_compress (image, GIMP_TYPE_UNDO_STACK, undo_type);

  if (undo &&
      g_object_get_data (G_OBJECT (undo),
                         "edit-selection-tool") == (gpointer) tool &&
      g_object_get_data (G_OBJECT (undo),
                         "edit-selection-type") == GINT_TO_POINTER (edit_mode))
    {
      GList *undo_items;

      undo_items = g_object_get_data (G_OBJECT (undo), "edit-selection-items");

      if (gimp_image_equal_selected_drawables (image, undo_items))
        {
          push_undo = FALSE;
        }
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
              g_object_set_data_full (G_OBJECT (undo), "edit-selection-items",
                                      g_list_copy (selected_items),
                                      (GDestroyNotify) g_list_free);
              g_object_set_data (G_OBJECT (undo), "edit-selection-type",
                                 GINT_TO_POINTER (edit_mode));
            }
        }
    }

  gimp_image_item_list_translate (gimp_item_get_image (selected_items->data),
                                  selected_items, inc_x, inc_y, push_undo);

  if (push_undo)
    gimp_image_undo_group_end (image);
  else
    gimp_undo_refresh_preview (undo,
                               gimp_get_user_context (display->gimp));

  gimp_image_flush (image);
  g_list_free (selected_items);

  return TRUE;
}
