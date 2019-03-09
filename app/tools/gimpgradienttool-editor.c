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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "operations/gimp-operation-config.h"

#include "core/gimpdata.h"
#include "core/gimpgradient.h"
#include "core/gimp-gradients.h"
#include "core/gimpimage.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpeditor.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimptoolgui.h"
#include "display/gimptoolline.h"

#include "gimpgradientoptions.h"
#include "gimpgradienttool.h"
#include "gimpgradienttool-editor.h"

#include "gimp-intl.h"


#define EPSILON 2e-10


typedef enum
{
  DIRECTION_NONE,
  DIRECTION_LEFT,
  DIRECTION_RIGHT
} Direction;


typedef struct
{
  /* line endpoints at the beginning of the operation */
  gdouble       start_x;
  gdouble       start_y;
  gdouble       end_x;
  gdouble       end_y;

  /* copy of the gradient at the beginning of the operation, owned by the gradient
   * info, or NULL, if the gradient isn't affected
   */
  GimpGradient *gradient;

  /* handle added by the operation, or HANDLE_NONE */
  gint          added_handle;
  /* handle removed by the operation, or HANDLE_NONE */
  gint          removed_handle;
  /* selected handle at the end of the operation, or HANDLE_NONE */
  gint          selected_handle;
} GradientInfo;


/*  local function prototypes  */

static gboolean              gimp_gradient_tool_editor_line_can_add_slider           (GimpToolLine          *line,
                                                                                      gdouble                value,
                                                                                      GimpGradientTool      *gradient_tool);
static gint                  gimp_gradient_tool_editor_line_add_slider               (GimpToolLine          *line,
                                                                                      gdouble                value,
                                                                                      GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_line_prepare_to_remove_slider (GimpToolLine          *line,
                                                                                      gint                   slider,
                                                                                      gboolean               remove,
                                                                                      GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_line_remove_slider            (GimpToolLine          *line,
                                                                                      gint                   slider,
                                                                                      GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_line_selection_changed        (GimpToolLine          *line,
                                                                                      GimpGradientTool      *gradient_tool);
static gboolean              gimp_gradient_tool_editor_line_handle_clicked           (GimpToolLine          *line,
                                                                                      gint                   handle,
                                                                                      GdkModifierType        state,
                                                                                      GimpButtonPressType    press_type,
                                                                                      GimpGradientTool      *gradient_tool);

static void                  gimp_gradient_tool_editor_gui_response                  (GimpToolGui           *gui,
                                                                                      gint                   response_id,
                                                                                      GimpGradientTool      *gradient_tool);

static void                  gimp_gradient_tool_editor_color_entry_color_clicked     (GimpColorButton       *button,
                                                                                      GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_color_entry_color_changed     (GimpColorButton       *button,
                                                                                      GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_color_entry_color_response    (GimpColorButton       *button,
                                                                                      GimpColorDialogState   state,
                                                                                      GimpGradientTool      *gradient_tool);

static void                  gimp_gradient_tool_editor_color_entry_type_changed      (GtkComboBox           *combo,
                                                                                      GimpGradientTool      *gradient_tool);

static void                  gimp_gradient_tool_editor_endpoint_se_value_changed     (GimpSizeEntry         *se,
                                                                                      GimpGradientTool      *gradient_tool);

static void                  gimp_gradient_tool_editor_stop_se_value_changed         (GimpSizeEntry        *se,
                                                                                      GimpGradientTool     *gradient_tool);

static void                  gimp_gradient_tool_editor_stop_delete_clicked           (GtkWidget             *button,
                                                                                      GimpGradientTool      *gradient_tool);

static void                  gimp_gradient_tool_editor_midpoint_se_value_changed     (GimpSizeEntry        *se,
                                                                                      GimpGradientTool     *gradient_tool);

static void                  gimp_gradient_tool_editor_midpoint_type_changed         (GtkComboBox           *combo,
                                                                                      GimpGradientTool      *gradient_tool);

static void                  gimp_gradient_tool_editor_midpoint_color_changed        (GtkComboBox           *combo,
                                                                                      GimpGradientTool      *gradient_tool);

static void                  gimp_gradient_tool_editor_midpoint_new_stop_clicked     (GtkWidget             *button,
                                                                                      GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_midpoint_center_clicked       (GtkWidget             *button,
                                                                                      GimpGradientTool      *gradient_tool);

static gboolean              gimp_gradient_tool_editor_flush_idle                    (GimpGradientTool      *gradient_tool);

static gboolean              gimp_gradient_tool_editor_is_gradient_editable          (GimpGradientTool      *gradient_tool);

static gboolean              gimp_gradient_tool_editor_handle_is_endpoint            (GimpGradientTool      *gradient_tool,
                                                                                      gint                   handle);
static gboolean              gimp_gradient_tool_editor_handle_is_stop                (GimpGradientTool      *gradient_tool,
                                                                                      gint                   handle);
static gboolean              gimp_gradient_tool_editor_handle_is_midpoint            (GimpGradientTool      *gradient_tool,
                                                                                      gint                   handle);
static GimpGradientSegment * gimp_gradient_tool_editor_handle_get_segment            (GimpGradientTool      *gradient_tool,
                                                                                      gint                   handle);

static void                  gimp_gradient_tool_editor_block_handlers                (GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_unblock_handlers              (GimpGradientTool      *gradient_tool);
static gboolean              gimp_gradient_tool_editor_are_handlers_blocked          (GimpGradientTool      *gradient_tool);

static void                  gimp_gradient_tool_editor_freeze_gradient               (GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_thaw_gradient                 (GimpGradientTool      *gradient_tool);

static gint                  gimp_gradient_tool_editor_add_stop                      (GimpGradientTool      *gradient_tool,
                                                                                      gdouble                value);
static void                  gimp_gradient_tool_editor_delete_stop                   (GimpGradientTool      *gradient_tool,
                                                                                      gint                   slider);
static gint                  gimp_gradient_tool_editor_midpoint_to_stop              (GimpGradientTool      *gradient_tool,
                                                                                      gint                   slider);

static void                  gimp_gradient_tool_editor_update_sliders                (GimpGradientTool      *gradient_tool);

static void                  gimp_gradient_tool_editor_purge_gradient_history        (GSList               **stack);
static void                  gimp_gradient_tool_editor_purge_gradient                (GimpGradientTool      *gradient_tool);

static GtkWidget           * gimp_gradient_tool_editor_color_entry_new               (GimpGradientTool      *gradient_tool,
                                                                                      const gchar           *title,
                                                                                      Direction              direction,
                                                                                      GtkWidget             *chain_button,
                                                                                      GtkWidget            **color_panel,
                                                                                      GtkWidget            **type_combo);
static void                  gimp_gradient_tool_editor_init_endpoint_gui             (GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_init_stop_gui                 (GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_init_midpoint_gui             (GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_update_endpoint_gui           (GimpGradientTool      *gradient_tool,
                                                                                      gint                   selection);
static void                  gimp_gradient_tool_editor_update_stop_gui               (GimpGradientTool      *gradient_tool,
                                                                                      gint                   selection);
static void                  gimp_gradient_tool_editor_update_midpoint_gui           (GimpGradientTool      *gradient_tool,
                                                                                      gint                   selection);
static void                  gimp_gradient_tool_editor_update_gui                    (GimpGradientTool      *gradient_tool);

static GradientInfo        * gimp_gradient_tool_editor_gradient_info_new             (GimpGradientTool      *gradient_tool);
static void                  gimp_gradient_tool_editor_gradient_info_free            (GradientInfo          *info);
static void                  gimp_gradient_tool_editor_gradient_info_apply           (GimpGradientTool      *gradient_tool,
                                                                                      const GradientInfo    *info,
                                                                                      gboolean               set_selection);
static gboolean              gimp_gradient_tool_editor_gradient_info_is_trivial      (GimpGradientTool      *gradient_tool,
                                                                                      const GradientInfo    *info);


/*  private functions  */


static gboolean
gimp_gradient_tool_editor_line_can_add_slider (GimpToolLine  *line,
                                               gdouble        value,
                                               GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  gdouble              offset  = options->offset / 100.0;

  return gimp_gradient_tool_editor_is_gradient_editable (gradient_tool) &&
         value >= offset;
}

static gint
gimp_gradient_tool_editor_line_add_slider (GimpToolLine     *line,
                                           gdouble           value,
                                           GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options       = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpPaintOptions    *paint_options = GIMP_PAINT_OPTIONS (options);
  gdouble              offset        = options->offset / 100.0;

  /* adjust slider value according to the offset */
  value = (value - offset) / (1.0 - offset);

  /* flip the slider value, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    value = 1.0 - value;

  return gimp_gradient_tool_editor_add_stop (gradient_tool, value);
}

static void
gimp_gradient_tool_editor_line_prepare_to_remove_slider (GimpToolLine     *line,
                                                         gint              slider,
                                                         gboolean          remove,
                                                         GimpGradientTool *gradient_tool)
{
  if (remove)
    {
      GradientInfo *info;
      GimpGradient *tentative_gradient;

      /* show a tentative gradient, demonstrating the result of actually
       * removing the slider
       */

      info = gradient_tool->undo_stack->data;

      if (info->added_handle == slider)
        {
          /* see comment in gimp_gradient_tool_editor_delete_stop() */

          gimp_assert (info->gradient != NULL);

          tentative_gradient = g_object_ref (info->gradient);
        }
      else
        {
          GimpGradientSegment *seg;
          gint                 i;

          tentative_gradient =
            GIMP_GRADIENT (gimp_data_duplicate (GIMP_DATA (gradient_tool->gradient)));

          seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, slider);

          i = gimp_gradient_segment_range_get_n_segments (gradient_tool->gradient,
                                                          gradient_tool->gradient->segments,
                                                          seg) - 1;

          seg = gimp_gradient_segment_get_nth (tentative_gradient->segments, i);

          gimp_gradient_segment_range_merge (tentative_gradient,
                                             seg, seg->next, NULL, NULL);
        }

      gimp_gradient_tool_set_tentative_gradient (gradient_tool, tentative_gradient);

      g_object_unref (tentative_gradient);
    }
  else
    {
      gimp_gradient_tool_set_tentative_gradient (gradient_tool, NULL);
    }
}

static void
gimp_gradient_tool_editor_line_remove_slider (GimpToolLine     *line,
                                              gint              slider,
                                              GimpGradientTool *gradient_tool)
{
  gimp_gradient_tool_editor_delete_stop (gradient_tool, slider);
  gimp_gradient_tool_set_tentative_gradient (gradient_tool, NULL);
}

static void
gimp_gradient_tool_editor_line_selection_changed (GimpToolLine     *line,
                                                  GimpGradientTool *gradient_tool)
{
  gint selection;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  if (gradient_tool->gui)
    {
      /* hide all color dialogs */
      gimp_color_panel_dialog_response (
        GIMP_COLOR_PANEL (gradient_tool->endpoint_color_panel),
        GIMP_COLOR_DIALOG_OK);
      gimp_color_panel_dialog_response (
        GIMP_COLOR_PANEL (gradient_tool->stop_left_color_panel),
        GIMP_COLOR_DIALOG_OK);
      gimp_color_panel_dialog_response (
        GIMP_COLOR_PANEL (gradient_tool->stop_right_color_panel),
        GIMP_COLOR_DIALOG_OK);

      /* reset the stop colors chain button */
      if (gimp_gradient_tool_editor_handle_is_stop (gradient_tool, selection))
        {
          const GimpGradientSegment *seg;
          gboolean                   homogeneous;

          seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool,
                                                              selection);

          homogeneous = seg->right_color.r    == seg->next->left_color.r &&
                        seg->right_color.g    == seg->next->left_color.g &&
                        seg->right_color.b    == seg->next->left_color.b &&
                        seg->right_color.a    == seg->next->left_color.a &&
                        seg->right_color_type == seg->next->left_color_type;

          gimp_chain_button_set_active (
            GIMP_CHAIN_BUTTON (gradient_tool->stop_chain_button), homogeneous);
        }
    }

  gimp_gradient_tool_editor_update_gui (gradient_tool);
}

static gboolean
gimp_gradient_tool_editor_line_handle_clicked (GimpToolLine        *line,
                                               gint                 handle,
                                               GdkModifierType      state,
                                               GimpButtonPressType  press_type,
                                               GimpGradientTool    *gradient_tool)
{
  if (gimp_gradient_tool_editor_handle_is_midpoint (gradient_tool, handle))
    {
      if (press_type == GIMP_BUTTON_PRESS_DOUBLE &&
          gimp_gradient_tool_editor_is_gradient_editable (gradient_tool))
        {
          gint stop;

          stop = gimp_gradient_tool_editor_midpoint_to_stop (gradient_tool, handle);

          gimp_tool_line_set_selection (line, stop);

          /* return FALSE, so that the new slider can be dragged immediately */
          return FALSE;
        }
    }

  return FALSE;
}


static void
gimp_gradient_tool_editor_gui_response (GimpToolGui      *gui,
                                        gint              response_id,
                                        GimpGradientTool *gradient_tool)
{
  switch (response_id)
    {
    default:
      gimp_tool_line_set_selection (GIMP_TOOL_LINE (gradient_tool->widget),
                                    GIMP_TOOL_LINE_HANDLE_NONE);
      break;
    }
}

static void
gimp_gradient_tool_editor_color_entry_color_clicked (GimpColorButton  *button,
                                                     GimpGradientTool *gradient_tool)
{
  gimp_gradient_tool_editor_start_edit (gradient_tool);
}

static void
gimp_gradient_tool_editor_color_entry_color_changed (GimpColorButton  *button,
                                                     GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options       = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpPaintOptions    *paint_options = GIMP_PAINT_OPTIONS (options);
  gint                 selection;
  GimpRGB              color;
  Direction            direction;
  GtkWidget           *chain_button;
  GimpGradientSegment *seg;

  if (gimp_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  gimp_color_button_get_color (button, &color);

  direction =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                        "gimp-gradient-tool-editor-direction"));
  chain_button = g_object_get_data (G_OBJECT (button),
                                    "gimp-gradient-tool-editor-chain-button");

  gimp_gradient_tool_editor_start_edit (gradient_tool);
  gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

  /* swap the endpoint handles, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      switch (selection)
        {
        case GIMP_TOOL_LINE_HANDLE_START:
          selection = GIMP_TOOL_LINE_HANDLE_END;
          break;

        case GIMP_TOOL_LINE_HANDLE_END:
          selection = GIMP_TOOL_LINE_HANDLE_START;
          break;
        }
    }

  seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  switch (selection)
    {
    case GIMP_TOOL_LINE_HANDLE_START:
      seg->left_color      = color;
      seg->left_color_type = GIMP_GRADIENT_COLOR_FIXED;
      break;

    case GIMP_TOOL_LINE_HANDLE_END:
      seg->right_color      = color;
      seg->right_color_type = GIMP_GRADIENT_COLOR_FIXED;
      break;

    default:
      if (direction == DIRECTION_LEFT ||
          (chain_button               &&
           gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (chain_button))))
        {
          seg->right_color      = color;
          seg->right_color_type = GIMP_GRADIENT_COLOR_FIXED;
        }

      if (direction == DIRECTION_RIGHT ||
          (chain_button                &&
           gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (chain_button))))
        {
          seg->next->left_color      = color;
          seg->next->left_color_type = GIMP_GRADIENT_COLOR_FIXED;
        }
    }

  gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
gimp_gradient_tool_editor_color_entry_color_response (GimpColorButton      *button,
                                                      GimpColorDialogState  state,
                                                      GimpGradientTool     *gradient_tool)
{
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
gimp_gradient_tool_editor_color_entry_type_changed (GtkComboBox      *combo,
                                                    GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options       = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpPaintOptions    *paint_options = GIMP_PAINT_OPTIONS (options);
  gint                 selection;
  gint                 color_type;
  Direction            direction;
  GtkWidget           *chain_button;
  GimpGradientSegment *seg;

  if (gimp_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  if (! gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &color_type))
    return;

  direction =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo),
                                        "gimp-gradient-tool-editor-direction"));
  chain_button = g_object_get_data (G_OBJECT (combo),
                                    "gimp-gradient-tool-editor-chain-button");

  gimp_gradient_tool_editor_start_edit (gradient_tool);
  gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

  /* swap the endpoint handles, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      switch (selection)
        {
        case GIMP_TOOL_LINE_HANDLE_START:
          selection = GIMP_TOOL_LINE_HANDLE_END;
          break;

        case GIMP_TOOL_LINE_HANDLE_END:
          selection = GIMP_TOOL_LINE_HANDLE_START;
          break;
        }
    }

  seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  switch (selection)
    {
    case GIMP_TOOL_LINE_HANDLE_START:
      seg->left_color_type = color_type;
      break;

    case GIMP_TOOL_LINE_HANDLE_END:
      seg->right_color_type = color_type;
      break;

    default:
      if (direction == DIRECTION_LEFT ||
          (chain_button               &&
           gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (chain_button))))
        {
          seg->right_color_type = color_type;
        }

      if (direction == DIRECTION_RIGHT ||
          (chain_button                &&
           gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (chain_button))))
        {
          seg->next->left_color_type = color_type;
        }
    }

  gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
gimp_gradient_tool_editor_endpoint_se_value_changed (GimpSizeEntry    *se,
                                                     GimpGradientTool *gradient_tool)
{
  gint    selection;
  gdouble x;
  gdouble y;

  if (gimp_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  if (selection == GIMP_TOOL_LINE_HANDLE_NONE)
    return;

  x = gimp_size_entry_get_refval (se, 0);
  y = gimp_size_entry_get_refval (se, 1);

  gimp_gradient_tool_editor_start_edit (gradient_tool);
  gimp_gradient_tool_editor_block_handlers (gradient_tool);

  switch (selection)
    {
    case GIMP_TOOL_LINE_HANDLE_START:
      g_object_set (gradient_tool->widget,
                    "x1", x,
                    "y1", y,
                    NULL);
      break;

    case GIMP_TOOL_LINE_HANDLE_END:
      g_object_set (gradient_tool->widget,
                    "x2", x,
                    "y2", y,
                    NULL);
      break;

    default:
      gimp_assert_not_reached ();
    }

  gimp_gradient_tool_editor_unblock_handlers (gradient_tool);
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
gimp_gradient_tool_editor_stop_se_value_changed (GimpSizeEntry    *se,
                                                 GimpGradientTool *gradient_tool)
{
  gint                 selection;
  gdouble              value;
  GimpGradientSegment *seg;

  if (gimp_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  if (selection == GIMP_TOOL_LINE_HANDLE_NONE)
    return;

  value = gimp_size_entry_get_refval (se, 0) / 100.0;

  gimp_gradient_tool_editor_start_edit (gradient_tool);
  gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  gimp_gradient_segment_range_compress (gradient_tool->gradient,
                                        seg, seg,
                                        seg->left, value);
  gimp_gradient_segment_range_compress (gradient_tool->gradient,
                                        seg->next, seg->next,
                                        value, seg->next->right);

  gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
gimp_gradient_tool_editor_stop_delete_clicked (GtkWidget        *button,
                                               GimpGradientTool *gradient_tool)
{
  gint selection;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  gimp_gradient_tool_editor_delete_stop (gradient_tool, selection);
}

static void
gimp_gradient_tool_editor_midpoint_se_value_changed (GimpSizeEntry    *se,
                                                     GimpGradientTool *gradient_tool)
{
  gint                 selection;
  gdouble              value;
  GimpGradientSegment *seg;

  if (gimp_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  if (selection == GIMP_TOOL_LINE_HANDLE_NONE)
    return;

  value = gimp_size_entry_get_refval (se, 0) / 100.0;

  gimp_gradient_tool_editor_start_edit (gradient_tool);
  gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  seg->middle = value;

  gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
gimp_gradient_tool_editor_midpoint_type_changed (GtkComboBox      *combo,
                                                 GimpGradientTool *gradient_tool)
{
  gint                 selection;
  gint                 type;
  GimpGradientSegment *seg;

  if (gimp_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  if (! gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &type))
    return;

  gimp_gradient_tool_editor_start_edit (gradient_tool);
  gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  seg->type = type;

  gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
gimp_gradient_tool_editor_midpoint_color_changed (GtkComboBox      *combo,
                                                  GimpGradientTool *gradient_tool)
{
  gint                 selection;
  gint                 color;
  GimpGradientSegment *seg;

  if (gimp_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  if (! gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &color))
    return;

  gimp_gradient_tool_editor_start_edit (gradient_tool);
  gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  seg->color = color;

  gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
gimp_gradient_tool_editor_midpoint_new_stop_clicked (GtkWidget        *button,
                                                     GimpGradientTool *gradient_tool)
{
  gint selection;
  gint stop;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  stop = gimp_gradient_tool_editor_midpoint_to_stop (gradient_tool, selection);

  gimp_tool_line_set_selection (GIMP_TOOL_LINE (gradient_tool->widget), stop);
}

static void
gimp_gradient_tool_editor_midpoint_center_clicked (GtkWidget        *button,
                                                   GimpGradientTool *gradient_tool)
{
  gint                 selection;
  GimpGradientSegment *seg;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

  gimp_gradient_tool_editor_start_edit (gradient_tool);
  gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  gimp_gradient_segment_range_recenter_handles (gradient_tool->gradient, seg, seg);

  gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static gboolean
gimp_gradient_tool_editor_flush_idle (GimpGradientTool *gradient_tool)
{
  GimpDisplay *display = GIMP_TOOL (gradient_tool)->display;

  gimp_image_flush (gimp_display_get_image (display));

  gradient_tool->flush_idle_id = 0;

  return G_SOURCE_REMOVE;
}

static gboolean
gimp_gradient_tool_editor_is_gradient_editable (GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);

  return ! options->modify_active ||
         gimp_data_is_writable (GIMP_DATA (gradient_tool->gradient));
}

static gboolean
gimp_gradient_tool_editor_handle_is_endpoint (GimpGradientTool *gradient_tool,
                                              gint              handle)
{
  return handle == GIMP_TOOL_LINE_HANDLE_START ||
         handle == GIMP_TOOL_LINE_HANDLE_END;
}

static gboolean
gimp_gradient_tool_editor_handle_is_stop (GimpGradientTool *gradient_tool,
                                          gint              handle)
{
  gint n_sliders;

  gimp_tool_line_get_sliders (GIMP_TOOL_LINE (gradient_tool->widget), &n_sliders);

  return handle >= 0 && handle < n_sliders / 2;
}

static gboolean
gimp_gradient_tool_editor_handle_is_midpoint (GimpGradientTool *gradient_tool,
                                              gint              handle)
{
  gint n_sliders;

  gimp_tool_line_get_sliders (GIMP_TOOL_LINE (gradient_tool->widget), &n_sliders);

  return handle >= n_sliders / 2;
}

static GimpGradientSegment *
gimp_gradient_tool_editor_handle_get_segment (GimpGradientTool *gradient_tool,
                                              gint              handle)
{
  switch (handle)
    {
    case GIMP_TOOL_LINE_HANDLE_START:
      return gradient_tool->gradient->segments;

    case GIMP_TOOL_LINE_HANDLE_END:
      return gimp_gradient_segment_get_last (gradient_tool->gradient->segments);

    default:
      {
        const GimpControllerSlider *sliders;
        gint                        n_sliders;
        gint                        seg_i;

        sliders = gimp_tool_line_get_sliders (GIMP_TOOL_LINE (gradient_tool->widget),
                                              &n_sliders);

        gimp_assert (handle >= 0 && handle < n_sliders);

        seg_i = GPOINTER_TO_INT (sliders[handle].data);

        return gimp_gradient_segment_get_nth (gradient_tool->gradient->segments,
                                              seg_i);
      }
    }
}

static void
gimp_gradient_tool_editor_block_handlers (GimpGradientTool *gradient_tool)
{
  gradient_tool->block_handlers_count++;
}

static void
gimp_gradient_tool_editor_unblock_handlers (GimpGradientTool *gradient_tool)
{
  gimp_assert (gradient_tool->block_handlers_count > 0);

  gradient_tool->block_handlers_count--;
}

static gboolean
gimp_gradient_tool_editor_are_handlers_blocked (GimpGradientTool *gradient_tool)
{
  return gradient_tool->block_handlers_count > 0;
}

static void
gimp_gradient_tool_editor_freeze_gradient (GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpGradient        *custom;
  GradientInfo           *info;

  gimp_gradient_tool_editor_block_handlers (gradient_tool);

  custom = gimp_gradients_get_custom (GIMP_CONTEXT (options)->gimp);

  if (gradient_tool->gradient == custom || options->modify_active)
    {
      gimp_assert (gimp_gradient_tool_editor_is_gradient_editable (gradient_tool));

      gimp_data_freeze (GIMP_DATA (gradient_tool->gradient));
    }
  else
    {
      /* copy the active gradient to the custom gradient, and make the custom
       * gradient active.
       */
      gimp_data_freeze (GIMP_DATA (custom));

      gimp_data_copy (GIMP_DATA (custom), GIMP_DATA (gradient_tool->gradient));

      gimp_context_set_gradient (GIMP_CONTEXT (options), custom);

      gimp_assert (gradient_tool->gradient == custom);
      gimp_assert (gimp_gradient_tool_editor_is_gradient_editable (gradient_tool));
    }

  if (gradient_tool->edit_count > 0)
    {
      info = gradient_tool->undo_stack->data;

      if (! info->gradient)
        {
          info->gradient =
            GIMP_GRADIENT (gimp_data_duplicate (GIMP_DATA (gradient_tool->gradient)));
        }
    }
}

static void
gimp_gradient_tool_editor_thaw_gradient (GimpGradientTool *gradient_tool)
{
  gimp_data_thaw (GIMP_DATA (gradient_tool->gradient));

  gimp_gradient_tool_editor_update_sliders (gradient_tool);
  gimp_gradient_tool_editor_update_gui (gradient_tool);

  gimp_gradient_tool_editor_unblock_handlers (gradient_tool);
}

static gint
gimp_gradient_tool_editor_add_stop (GimpGradientTool *gradient_tool,
                                    gdouble           value)
{
  GimpGradientOptions *options       = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpPaintOptions    *paint_options = GIMP_PAINT_OPTIONS (options);
  GimpGradientSegment *seg;
  gint                 stop;
  GradientInfo           *info;

  gimp_gradient_tool_editor_start_edit (gradient_tool);
  gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

  gimp_gradient_split_at (gradient_tool->gradient,
                          GIMP_CONTEXT (options), NULL, value,
                          paint_options->gradient_options->gradient_blend_color_space,
                          &seg, NULL);

  stop =
    gimp_gradient_segment_range_get_n_segments (gradient_tool->gradient,
                                                gradient_tool->gradient->segments,
                                                seg) - 1;

  info               = gradient_tool->undo_stack->data;
  info->added_handle = stop;

  gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);

  return stop;
}

static void
gimp_gradient_tool_editor_delete_stop (GimpGradientTool *gradient_tool,
                                       gint              slider)
{
  GradientInfo *info;

  gimp_assert (gimp_gradient_tool_editor_handle_is_stop (gradient_tool, slider));

  gimp_gradient_tool_editor_start_edit (gradient_tool);
  gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

  info = gradient_tool->undo_stack->data;

  if (info->added_handle == slider)
    {
      /* when removing a stop that was added as part of the current action,
       * restore the original gradient at the beginning of the action, rather
       * than deleting the stop from the current gradient, so that the affected
       * midpoint returns to its state at the beginning of the action, instead
       * of being reset.
       *
       * note that this assumes that the gradient hasn't changed in any other
       * way during the action, which is ugly, but currently always true.
       */

      gimp_assert (info->gradient != NULL);

      gimp_data_copy (GIMP_DATA (gradient_tool->gradient),
                      GIMP_DATA (info->gradient));

      g_clear_object (&info->gradient);

      info->added_handle = GIMP_TOOL_LINE_HANDLE_NONE;
    }
  else
    {
      GimpGradientSegment *seg;

      seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, slider);

      gimp_gradient_segment_range_merge (gradient_tool->gradient,
                                         seg, seg->next, NULL, NULL);

      info->removed_handle = slider;
    }

  gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
  gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static gint
gimp_gradient_tool_editor_midpoint_to_stop (GimpGradientTool *gradient_tool,
                                            gint              slider)
{
  const GimpControllerSlider *sliders;

  gimp_assert (gimp_gradient_tool_editor_handle_is_midpoint (gradient_tool, slider));

  sliders = gimp_tool_line_get_sliders (GIMP_TOOL_LINE (gradient_tool->widget),
                                        NULL);

  if (sliders[slider].value > sliders[slider].min + EPSILON &&
      sliders[slider].value < sliders[slider].max - EPSILON)
    {
      const GimpGradientSegment *seg;
      gint                       stop;
      GradientInfo                 *info;

      seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, slider);

      stop = gimp_gradient_tool_editor_add_stop (gradient_tool, seg->middle);

      info                 = gradient_tool->undo_stack->data;
      info->removed_handle = slider;

      slider = stop;
    }

  return slider;
}

static void
gimp_gradient_tool_editor_update_sliders (GimpGradientTool *gradient_tool)
{
  GimpGradientOptions  *options       = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpPaintOptions     *paint_options = GIMP_PAINT_OPTIONS (options);
  gdouble               offset        = options->offset / 100.0;
  gboolean              editable;
  GimpControllerSlider *sliders;
  gint                  n_sliders;
  gint                  n_segments;
  GimpGradientSegment  *seg;
  GimpControllerSlider *slider;
  gint                  i;

  if (! gradient_tool->widget || options->instant)
    return;

  editable = gimp_gradient_tool_editor_is_gradient_editable (gradient_tool);

  n_segments = gimp_gradient_segment_range_get_n_segments (
    gradient_tool->gradient, gradient_tool->gradient->segments, NULL);

  n_sliders = (n_segments - 1) + /* gradient stops, between each adjacent
                                  * pair of segments */
              (n_segments);      /* midpoints, inside each segment */

  sliders = g_new (GimpControllerSlider, n_sliders);

  slider = sliders;

  /* initialize the gradient-stop sliders */
  for (seg = gradient_tool->gradient->segments, i = 0;
       seg->next;
       seg = seg->next, i++)
    {
      *slider = GIMP_CONTROLLER_SLIDER_DEFAULT;

      slider->value     = seg->right;
      slider->min       = seg->left;
      slider->max       = seg->next->right;

      slider->movable   = editable;
      slider->removable = editable;

      slider->data      = GINT_TO_POINTER (i);

      slider++;
    }

  /* initialize the midpoint sliders */
  for (seg = gradient_tool->gradient->segments, i = 0;
       seg;
       seg = seg->next, i++)
    {
      *slider = GIMP_CONTROLLER_SLIDER_DEFAULT;

      slider->value    = seg->middle;
      slider->min      = seg->left;
      slider->max      = seg->right;

      /* hide midpoints of zero-length segments, since they'd otherwise
       * prevent the segment's endpoints from being selected
       */
      slider->visible  = fabs (slider->max - slider->min) > EPSILON;
      slider->movable  = editable;

      slider->autohide = TRUE;
      slider->type     = GIMP_HANDLE_FILLED_CIRCLE;
      slider->size     = 0.6;

      slider->data     = GINT_TO_POINTER (i);

      slider++;
    }

  /* flip the slider limits and values, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      for (i = 0; i < n_sliders; i++)
        {
          gdouble temp;

          sliders[i].value = 1.0 - sliders[i].value;
          temp             = sliders[i].min;
          sliders[i].min   = 1.0 - sliders[i].max;
          sliders[i].max   = 1.0 - temp;
        }
    }

  /* adjust the sliders according to the offset */
  for (i = 0; i < n_sliders; i++)
    {
      sliders[i].value = (1.0 - offset) * sliders[i].value + offset;
      sliders[i].min   = (1.0 - offset) * sliders[i].min   + offset;
      sliders[i].max   = (1.0 - offset) * sliders[i].max   + offset;
    }

  /* avoid updating the gradient in gimp_gradient_tool_editor_line_changed() */
  gimp_gradient_tool_editor_block_handlers (gradient_tool);

  gimp_tool_line_set_sliders (GIMP_TOOL_LINE (gradient_tool->widget),
                              sliders, n_sliders);

  gimp_gradient_tool_editor_unblock_handlers (gradient_tool);

  g_free (sliders);
}

static void
gimp_gradient_tool_editor_purge_gradient_history (GSList **stack)
{
  GSList *link;

  /* eliminate all history steps that modify the gradient */
  while ((link = *stack))
    {
      GradientInfo *info = link->data;

      if (info->gradient)
        {
          gimp_gradient_tool_editor_gradient_info_free (info);

          *stack = g_slist_delete_link (*stack, link);
        }
      else
        {
          stack = &link->next;
        }
    }
}

static void
gimp_gradient_tool_editor_purge_gradient (GimpGradientTool *gradient_tool)
{
  if (gradient_tool->widget)
    {
      gimp_gradient_tool_editor_update_sliders (gradient_tool);

      gimp_tool_line_set_selection (GIMP_TOOL_LINE (gradient_tool->widget),
                                    GIMP_TOOL_LINE_HANDLE_NONE);
    }

  gimp_gradient_tool_editor_purge_gradient_history (&gradient_tool->undo_stack);
  gimp_gradient_tool_editor_purge_gradient_history (&gradient_tool->redo_stack);
}

static GtkWidget *
gimp_gradient_tool_editor_color_entry_new (GimpGradientTool  *gradient_tool,
                                           const gchar       *title,
                                           Direction          direction,
                                           GtkWidget         *chain_button,
                                           GtkWidget        **color_panel,
                                           GtkWidget        **type_combo)
{
  GimpContext *context = GIMP_CONTEXT (GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool));
  GtkWidget   *hbox;
  GtkWidget   *button;
  GtkWidget   *combo;
  GimpRGB      color   = {};

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  /* the color panel */
  *color_panel = button = gimp_color_panel_new (title, &color,
                                                GIMP_COLOR_AREA_SMALL_CHECKS,
                                                24, 24);
  gimp_color_button_set_update (GIMP_COLOR_BUTTON (button), TRUE);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (button), context);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button),
                     "gimp-gradient-tool-editor-direction",
                     GINT_TO_POINTER (direction));
  g_object_set_data (G_OBJECT (button),
                     "gimp-gradient-tool-editor-chain-button",
                     chain_button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_gradient_tool_editor_color_entry_color_clicked),
                    gradient_tool);
  g_signal_connect (button, "color-changed",
                    G_CALLBACK (gimp_gradient_tool_editor_color_entry_color_changed),
                    gradient_tool);
  g_signal_connect (button, "response",
                    G_CALLBACK (gimp_gradient_tool_editor_color_entry_color_response),
                    gradient_tool);

  /* the color type combo */
  *type_combo = combo = gimp_enum_combo_box_new (GIMP_TYPE_GRADIENT_COLOR);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
  gtk_widget_show (combo);

  g_object_set_data (G_OBJECT (combo),
                     "gimp-gradient-tool-editor-direction",
                     GINT_TO_POINTER (direction));
  g_object_set_data (G_OBJECT (combo),
                     "gimp-gradient-tool-editor-chain-button",
                     chain_button);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_gradient_tool_editor_color_entry_type_changed),
                    gradient_tool);

  return hbox;
}

static void
gimp_gradient_tool_editor_init_endpoint_gui (GimpGradientTool *gradient_tool)
{
  GimpDisplay      *display = GIMP_TOOL (gradient_tool)->display;
  GimpDisplayShell *shell   = gimp_display_get_shell (display);
  GimpImage        *image   = gimp_display_get_image (display);
  gdouble           xres;
  gdouble           yres;
  GtkWidget        *editor;
  GtkWidget        *table;
  GtkWidget        *label;
  GtkWidget        *spinbutton;
  GtkWidget        *se;
  GtkWidget        *hbox;
  gint              row     = 0;

  gimp_image_get_resolution (image, &xres, &yres);

  /* the endpoint editor */
  gradient_tool->endpoint_editor =
  editor                         = gimp_editor_new ();
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (gradient_tool->gui)),
                      editor, FALSE, TRUE, 0);

  /* the main table */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (editor), table, FALSE, TRUE, 0);
  gtk_widget_show (table);

  /* the position labels */
  label = gtk_label_new (_("X:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Y:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row + 1, row + 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /* the position size entry */
  spinbutton = gimp_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);

  gradient_tool->endpoint_se =
  se                         = gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
                                                    TRUE, TRUE, FALSE, 6,
                                                    GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_row_spacings (GTK_TABLE (se), 4);
  gtk_table_set_col_spacings (GTK_TABLE (se), 2);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (se), spinbutton, 1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gtk_table_attach (GTK_TABLE (table), se, 1, 2, row, row + 2,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (se), shell->unit);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (se), 0, xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (se), 1, yres, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (se), 0,
                                         -GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (se), 1,
                                         -GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (se), 0,
                            0, gimp_image_get_width (image));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (se), 1,
                            0, gimp_image_get_height (image));

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (gimp_gradient_tool_editor_endpoint_se_value_changed),
                    gradient_tool);

  row += 2;

  /* the color label */
  label = gtk_label_new (_("Color:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /* the color entry */
  hbox = gimp_gradient_tool_editor_color_entry_new (
    gradient_tool, _("Change Endpoint Color"), DIRECTION_NONE, NULL,
    &gradient_tool->endpoint_color_panel, &gradient_tool->endpoint_type_combo);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (hbox);

  row++;
}

static void
gimp_gradient_tool_editor_init_stop_gui (GimpGradientTool *gradient_tool)
{
  GtkWidget *editor;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *se;
  GtkWidget *table2;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *separator;
  gint       row = 0;

  /* the stop editor */
  gradient_tool->stop_editor =
  editor                     = gimp_editor_new ();
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (gradient_tool->gui)),
                      editor, FALSE, TRUE, 0);

  /* the main table */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (editor), table, FALSE, TRUE, 0);
  gtk_widget_show (table);

  /* the position label */
  label = gtk_label_new (_("Position:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /* the position size entry */
  gradient_tool->stop_se =
  se                     = gimp_size_entry_new (1, GIMP_UNIT_PERCENT, "%a",
                                                FALSE, TRUE, FALSE, 6,
                                                GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (se), FALSE);
  gtk_table_attach (GTK_TABLE (table), se, 1, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (se);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (gimp_gradient_tool_editor_stop_se_value_changed),
                    gradient_tool);

  row++;

  /* the color labels */
  label = gtk_label_new (_("Left color:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Right color:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row + 1, row + 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /* the color entries table */
  table2 = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 2);
  gtk_table_attach (GTK_TABLE (table), table2, 1, 2, row, row + 2,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (table2);

  /* the color entries chain button */
  gradient_tool->stop_chain_button =
  button                           = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gtk_table_attach (GTK_TABLE (table2), button, 1, 2, 0, 2,
                    GTK_SHRINK | GTK_FILL,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    0, 0);
  gtk_widget_show (button);

  /* the color entries */
  hbox = gimp_gradient_tool_editor_color_entry_new (
    gradient_tool, _("Change Stop Color"), DIRECTION_LEFT, button,
    &gradient_tool->stop_left_color_panel, &gradient_tool->stop_left_type_combo);
  gtk_table_attach (GTK_TABLE (table2), hbox, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (hbox);

  hbox = gimp_gradient_tool_editor_color_entry_new (
    gradient_tool, _("Change Stop Color"), DIRECTION_RIGHT, button,
    &gradient_tool->stop_right_color_panel, &gradient_tool->stop_right_type_combo);
  gtk_table_attach (GTK_TABLE (table2), hbox, 0, 1, 1, 2,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (hbox);

  row += 2;

  /* the action buttons separator */
  separator = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (table), separator, 0, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (separator);

  row++;

  /* the delete button */
  gimp_editor_add_button (GIMP_EDITOR (editor),
                          GIMP_ICON_EDIT_DELETE, _("Delete stop"),
                          NULL,
                          G_CALLBACK (gimp_gradient_tool_editor_stop_delete_clicked),
                          NULL, gradient_tool);
}

static void
gimp_gradient_tool_editor_init_midpoint_gui (GimpGradientTool *gradient_tool)
{
  GtkWidget *editor;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *se;
  GtkWidget *combo;
  GtkWidget *separator;
  gint       row = 0;

  /* the stop editor */
  gradient_tool->midpoint_editor =
  editor                         = gimp_editor_new ();
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (gradient_tool->gui)),
                      editor, FALSE, TRUE, 0);

  /* the main table */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (editor), table, FALSE, TRUE, 0);
  gtk_widget_show (table);

  /* the position label */
  label = gtk_label_new (_("Position:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /* the position size entry */
  gradient_tool->midpoint_se =
  se                         = gimp_size_entry_new (1, GIMP_UNIT_PERCENT, "%a",
                                                    FALSE, TRUE, FALSE, 6,
                                                    GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (se), FALSE);
  gtk_table_attach (GTK_TABLE (table), se, 1, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (se);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (gimp_gradient_tool_editor_midpoint_se_value_changed),
                    gradient_tool);

  row++;

  /* the type label */
  label = gtk_label_new (_("Blending:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /* the type combo */
  gradient_tool->midpoint_type_combo =
  combo                              = gimp_enum_combo_box_new (GIMP_TYPE_GRADIENT_SEGMENT_TYPE);
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_gradient_tool_editor_midpoint_type_changed),
                    gradient_tool);

  row++;

  /* the color label */
  label = gtk_label_new (_("Coloring:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /* the color combo */
  gradient_tool->midpoint_color_combo =
  combo                              = gimp_enum_combo_box_new (GIMP_TYPE_GRADIENT_SEGMENT_COLOR);
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_gradient_tool_editor_midpoint_color_changed),
                    gradient_tool);

  row++;

  /* the action buttons separator */
  separator = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (table), separator, 0, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (separator);

  row++;

  /* the new stop button */
  gradient_tool->midpoint_new_stop_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GIMP_ICON_DOCUMENT_NEW, _("New stop at midpoint"),
                            NULL,
                            G_CALLBACK (gimp_gradient_tool_editor_midpoint_new_stop_clicked),
                            NULL, gradient_tool);

  /* the center button */
  gradient_tool->midpoint_center_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GIMP_ICON_CENTER_HORIZONTAL, _("Center midpoint"),
                            NULL,
                            G_CALLBACK (gimp_gradient_tool_editor_midpoint_center_clicked),
                            NULL, gradient_tool);
}

static void
gimp_gradient_tool_editor_update_endpoint_gui (GimpGradientTool *gradient_tool,
                                               gint              selection)
{
  GimpGradientOptions *options       = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpPaintOptions    *paint_options = GIMP_PAINT_OPTIONS (options);
  GimpContext         *context       = GIMP_CONTEXT (options);
  gboolean             editable;
  GimpGradientSegment *seg;
  const gchar         *title;
  gdouble              x;
  gdouble              y;
  GimpRGB              color;
  GimpGradientColor    color_type;

  editable = gimp_gradient_tool_editor_is_gradient_editable (gradient_tool);

  switch (selection)
    {
    case GIMP_TOOL_LINE_HANDLE_START:
      g_object_get (gradient_tool->widget,
                    "x1", &x,
                    "y1", &y,
                    NULL);
      break;

    case GIMP_TOOL_LINE_HANDLE_END:
      g_object_get (gradient_tool->widget,
                    "x2", &x,
                    "y2", &y,
                    NULL);
      break;

    default:
      gimp_assert_not_reached ();
    }

  /* swap the endpoint handles, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      switch (selection)
        {
        case GIMP_TOOL_LINE_HANDLE_START:
          selection = GIMP_TOOL_LINE_HANDLE_END;
          break;

        case GIMP_TOOL_LINE_HANDLE_END:
          selection = GIMP_TOOL_LINE_HANDLE_START;
          break;
        }
    }

  seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  switch (selection)
    {
    case GIMP_TOOL_LINE_HANDLE_START:
      title = _("Start Endpoint");

      gimp_gradient_segment_get_left_flat_color (gradient_tool->gradient, context,
                                                 seg, &color);
      color_type = seg->left_color_type;
      break;

    case GIMP_TOOL_LINE_HANDLE_END:
      title = _("End Endpoint");

      gimp_gradient_segment_get_right_flat_color (gradient_tool->gradient, context,
                                                  seg, &color);
      color_type = seg->right_color_type;
      break;

    default:
      gimp_assert_not_reached ();
    }

  gimp_tool_gui_set_title (gradient_tool->gui, title);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (gradient_tool->endpoint_se), 0, x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (gradient_tool->endpoint_se), 1, y);

  gimp_color_button_set_color (
    GIMP_COLOR_BUTTON (gradient_tool->endpoint_color_panel), &color);
  gimp_int_combo_box_set_active (
    GIMP_INT_COMBO_BOX (gradient_tool->endpoint_type_combo), color_type);

  gtk_widget_set_sensitive (gradient_tool->endpoint_color_panel, editable);
  gtk_widget_set_sensitive (gradient_tool->endpoint_type_combo,  editable);

  gtk_widget_show (gradient_tool->endpoint_editor);
}

static void
gimp_gradient_tool_editor_update_stop_gui (GimpGradientTool *gradient_tool,
                                           gint              selection)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpContext         *context = GIMP_CONTEXT (options);
  gboolean             editable;
  GimpGradientSegment *seg;
  gint                 index;
  gchar               *title;
  gdouble              min;
  gdouble              max;
  gdouble              value;
  GimpRGB              left_color;
  GimpGradientColor    left_color_type;
  GimpRGB              right_color;
  GimpGradientColor    right_color_type;

  editable = gimp_gradient_tool_editor_is_gradient_editable (gradient_tool);

  seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  index = GPOINTER_TO_INT (
    gimp_tool_line_get_sliders (GIMP_TOOL_LINE (gradient_tool->widget),
                                NULL)[selection].data);

  title = g_strdup_printf (_("Stop %d"), index + 1);

  min   = seg->left;
  max   = seg->next->right;
  value = seg->right;

  gimp_gradient_segment_get_right_flat_color (gradient_tool->gradient, context,
                                              seg, &left_color);
  left_color_type = seg->right_color_type;

  gimp_gradient_segment_get_left_flat_color (gradient_tool->gradient, context,
                                             seg->next, &right_color);
  right_color_type = seg->next->left_color_type;

  gimp_tool_gui_set_title (gradient_tool->gui, title);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (gradient_tool->stop_se),
                                         0, 100.0 * min, 100.0 * max);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (gradient_tool->stop_se),
                              0, 100.0 * value);

  gimp_color_button_set_color (
    GIMP_COLOR_BUTTON (gradient_tool->stop_left_color_panel), &left_color);
  gimp_int_combo_box_set_active (
    GIMP_INT_COMBO_BOX (gradient_tool->stop_left_type_combo), left_color_type);

  gimp_color_button_set_color (
    GIMP_COLOR_BUTTON (gradient_tool->stop_right_color_panel), &right_color);
  gimp_int_combo_box_set_active (
    GIMP_INT_COMBO_BOX (gradient_tool->stop_right_type_combo), right_color_type);

  gtk_widget_set_sensitive (gradient_tool->stop_se,                editable);
  gtk_widget_set_sensitive (gradient_tool->stop_left_color_panel,  editable);
  gtk_widget_set_sensitive (gradient_tool->stop_left_type_combo,   editable);
  gtk_widget_set_sensitive (gradient_tool->stop_right_color_panel, editable);
  gtk_widget_set_sensitive (gradient_tool->stop_right_type_combo,  editable);
  gtk_widget_set_sensitive (gradient_tool->stop_chain_button,      editable);
  gtk_widget_set_sensitive (
    GTK_WIDGET (gimp_editor_get_button_box (GIMP_EDITOR (gradient_tool->stop_editor))),
    editable);

  g_free (title);

  gtk_widget_show (gradient_tool->stop_editor);
}

static void
gimp_gradient_tool_editor_update_midpoint_gui (GimpGradientTool *gradient_tool,
                                               gint              selection)
{
  gboolean                    editable;
  const GimpGradientSegment  *seg;
  gint                        index;
  gchar                      *title;
  gdouble                     min;
  gdouble                     max;
  gdouble                     value;
  GimpGradientSegmentType     type;
  GimpGradientSegmentColor    color;

  editable = gimp_gradient_tool_editor_is_gradient_editable (gradient_tool);

  seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  index = GPOINTER_TO_INT (
    gimp_tool_line_get_sliders (GIMP_TOOL_LINE (gradient_tool->widget),
                                NULL)[selection].data);

  title = g_strdup_printf (_("Midpoint %d"), index + 1);

  min   = seg->left;
  max   = seg->right;
  value = seg->middle;
  type  = seg->type;
  color = seg->color;

  gimp_tool_gui_set_title (gradient_tool->gui, title);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (gradient_tool->midpoint_se),
                                         0, 100.0 * min, 100.0 * max);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (gradient_tool->midpoint_se),
                              0, 100.0 * value);

  gimp_int_combo_box_set_active (
    GIMP_INT_COMBO_BOX (gradient_tool->midpoint_type_combo), type);

  gimp_int_combo_box_set_active (
    GIMP_INT_COMBO_BOX (gradient_tool->midpoint_color_combo), color);

  gtk_widget_set_sensitive (gradient_tool->midpoint_new_stop_button,
                            value > min + EPSILON && value < max - EPSILON);
  gtk_widget_set_sensitive (gradient_tool->midpoint_center_button,
                            fabs (value - (min + max) / 2.0) > EPSILON);

  gtk_widget_set_sensitive (gradient_tool->midpoint_se,          editable);
  gtk_widget_set_sensitive (gradient_tool->midpoint_type_combo,  editable);
  gtk_widget_set_sensitive (gradient_tool->midpoint_color_combo, editable);
  gtk_widget_set_sensitive (
    GTK_WIDGET (gimp_editor_get_button_box (GIMP_EDITOR (gradient_tool->midpoint_editor))),
    editable);

  g_free (title);

  gtk_widget_show (gradient_tool->midpoint_editor);
}

static void
gimp_gradient_tool_editor_update_gui (GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);

  if (gradient_tool->gradient && gradient_tool->widget && ! options->instant)
    {
      gint selection;

      selection =
        gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

      if (selection != GIMP_TOOL_LINE_HANDLE_NONE)
        {
          if (! gradient_tool->gui)
            {
              GimpDisplayShell *shell;

              shell = gimp_tool_widget_get_shell (gradient_tool->widget);

              gradient_tool->gui =
                gimp_tool_gui_new (GIMP_TOOL (gradient_tool)->tool_info,
                                   NULL, NULL, NULL, NULL,
                                   gtk_widget_get_screen (GTK_WIDGET (shell)),
                                   gimp_widget_get_monitor (GTK_WIDGET (shell)),
                                   TRUE,

                                   _("_Close"), GTK_RESPONSE_CLOSE,

                                   NULL);

              gimp_tool_gui_set_shell (gradient_tool->gui, shell);
              gimp_tool_gui_set_viewable (gradient_tool->gui,
                                          GIMP_VIEWABLE (gradient_tool->gradient));
              gimp_tool_gui_set_auto_overlay (gradient_tool->gui, TRUE);

              g_signal_connect (gradient_tool->gui, "response",
                                G_CALLBACK (gimp_gradient_tool_editor_gui_response),
                                gradient_tool);

              gimp_gradient_tool_editor_init_endpoint_gui (gradient_tool);
              gimp_gradient_tool_editor_init_stop_gui     (gradient_tool);
              gimp_gradient_tool_editor_init_midpoint_gui (gradient_tool);
            }

          gimp_gradient_tool_editor_block_handlers (gradient_tool);

          if (gimp_gradient_tool_editor_handle_is_endpoint (gradient_tool, selection))
            gimp_gradient_tool_editor_update_endpoint_gui (gradient_tool, selection);
          else
            gtk_widget_hide (gradient_tool->endpoint_editor);

          if (gimp_gradient_tool_editor_handle_is_stop (gradient_tool, selection))
            gimp_gradient_tool_editor_update_stop_gui (gradient_tool, selection);
          else
            gtk_widget_hide (gradient_tool->stop_editor);

          if (gimp_gradient_tool_editor_handle_is_midpoint (gradient_tool, selection))
            gimp_gradient_tool_editor_update_midpoint_gui (gradient_tool, selection);
          else
            gtk_widget_hide (gradient_tool->midpoint_editor);

          gimp_gradient_tool_editor_unblock_handlers (gradient_tool);

          gimp_tool_gui_show (gradient_tool->gui);

          return;
        }
    }

  if (gradient_tool->gui)
    gimp_tool_gui_hide (gradient_tool->gui);
}

static GradientInfo *
gimp_gradient_tool_editor_gradient_info_new (GimpGradientTool *gradient_tool)
{
  GradientInfo *info = g_slice_new (GradientInfo);

  info->start_x         = gradient_tool->start_x;
  info->start_y         = gradient_tool->start_y;
  info->end_x           = gradient_tool->end_x;
  info->end_y           = gradient_tool->end_y;

  info->gradient        = NULL;

  info->added_handle    = GIMP_TOOL_LINE_HANDLE_NONE;
  info->removed_handle  = GIMP_TOOL_LINE_HANDLE_NONE;
  info->selected_handle = GIMP_TOOL_LINE_HANDLE_NONE;

  return info;
}

static void
gimp_gradient_tool_editor_gradient_info_free (GradientInfo *info)
{
  if (info->gradient)
    g_object_unref (info->gradient);

  g_slice_free (GradientInfo, info);
}

static void
gimp_gradient_tool_editor_gradient_info_apply (GimpGradientTool   *gradient_tool,
                                               const GradientInfo *info,
                                               gboolean            set_selection)
{
  gint selection;

  gimp_assert (gradient_tool->widget   != NULL);
  gimp_assert (gradient_tool->gradient != NULL);

  /* pick the handle to select */
  if (info->gradient)
    {
      if (info->removed_handle != GIMP_TOOL_LINE_HANDLE_NONE)
        {
          /* we're undoing a stop-deletion or midpoint-to-stop operation;
           * select the removed handle
           */
          selection = info->removed_handle;
        }
      else if (info->added_handle != GIMP_TOOL_LINE_HANDLE_NONE)
        {
          /* we're undoing a stop addition operation */
          gimp_assert (gimp_gradient_tool_editor_handle_is_stop (gradient_tool,
                                                                 info->added_handle));

          selection =
            gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

          /* if the selected handle is a stop... */
          if (gimp_gradient_tool_editor_handle_is_stop (gradient_tool, selection))
            {
              /* if the added handle is selected, clear the selection */
              if (selection == info->added_handle)
                selection = GIMP_TOOL_LINE_HANDLE_NONE;
              /* otherwise, keep the currently selected stop, possibly
               * adjusting its handle index
               */
              else if (selection > info->added_handle)
                selection--;
            }
          /* otherwise, if the selected handle is a midpoint... */
          else if (gimp_gradient_tool_editor_handle_is_midpoint (gradient_tool, selection))
            {
              const GimpControllerSlider *sliders;
              gint                        seg_i;

              sliders =
                gimp_tool_line_get_sliders (GIMP_TOOL_LINE (gradient_tool->widget),
                                            NULL);

              seg_i = GPOINTER_TO_INT (sliders[selection].data);

              /* if the midpoint belongs to one of the two segments incident to
               * the added stop, clear the selection
               */
              if (seg_i == info->added_handle ||
                  seg_i == info->added_handle + 1)
                {
                  selection = GIMP_TOOL_LINE_HANDLE_NONE;
                }
              /* otherwise, keep the currently selected stop, adjusting its
               * handle index
               */
              else
                {
                  /* midpoint handles follow stop handles; since we removed a
                   * stop, we must decrement the handle index
                   */
                  selection--;

                  if (seg_i > info->added_handle)
                    selection--;
                }
            }
          /* otherwise, don't change the selection */
          else
            {
              set_selection = FALSE;
            }
        }
      else if (info->selected_handle != GIMP_TOOL_LINE_HANDLE_NONE)
        {
          /* we're undoing a property change operation; select the handle
           * corresponding to the affected object
           */
          selection = info->selected_handle;
        }
      else
        {
          /* something went wrong... */
          g_warn_if_reached ();

          set_selection = FALSE;
        }
    }
  else if ((info->start_x != gradient_tool->start_x  ||
            info->start_y != gradient_tool->start_y) &&
           (info->end_x   == gradient_tool->end_x    &&
            info->end_y   == gradient_tool->end_y))
    {
      /* we're undoing a start-endpoint move operation; select the start
       * endpoint
       */
      selection = GIMP_TOOL_LINE_HANDLE_START;
    }
  else if ((info->end_x   != gradient_tool->end_x    ||
            info->end_y   != gradient_tool->end_y)   &&
           (info->start_x == gradient_tool->start_x  &&
            info->start_y == gradient_tool->start_y))

    {
      /* we're undoing am end-endpoint move operation; select the end
       * endpoint
       */
      selection = GIMP_TOOL_LINE_HANDLE_END;
    }
  else
    {
      /* we're undoing a line move operation; don't change the selection */
      set_selection = FALSE;
    }

  gimp_gradient_tool_editor_block_handlers (gradient_tool);

  g_object_set (gradient_tool->widget,
                "x1", info->start_x,
                "y1", info->start_y,
                "x2", info->end_x,
                "y2", info->end_y,
                NULL);

  if (info->gradient)
    {
      gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

      gimp_data_copy (GIMP_DATA (gradient_tool->gradient),
                      GIMP_DATA (info->gradient));

      gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
    }

  if (set_selection)
    {
      gimp_tool_line_set_selection (GIMP_TOOL_LINE (gradient_tool->widget),
                                    selection);
    }

  gimp_gradient_tool_editor_update_gui (gradient_tool);

  gimp_gradient_tool_editor_unblock_handlers (gradient_tool);
}

static gboolean
gimp_gradient_tool_editor_gradient_info_is_trivial (GimpGradientTool   *gradient_tool,
                                                    const GradientInfo *info)
{
  const GimpGradientSegment *seg1;
  const GimpGradientSegment *seg2;

  if (info->start_x != gradient_tool->start_x ||
      info->start_y != gradient_tool->start_y ||
      info->end_x   != gradient_tool->end_x   ||
      info->end_y   != gradient_tool->end_y)
    {
      return FALSE;
    }

  if (info->gradient)
    {
      for (seg1 = info->gradient->segments, seg2 = gradient_tool->gradient->segments;
           seg1 && seg2;
           seg1 = seg1->next, seg2 = seg2->next)
        {
          if (memcmp (seg1, seg2, G_STRUCT_OFFSET (GimpGradientSegment, prev)))
            return FALSE;
        }

      if (seg1 || seg2)
        return FALSE;
    }

  return TRUE;
}


/*  public functions  */

void
gimp_gradient_tool_editor_options_notify (GimpGradientTool *gradient_tool,
                                          GimpToolOptions  *options,
                                          const GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "modify-active"))
    {
      gimp_gradient_tool_editor_update_sliders (gradient_tool);
      gimp_gradient_tool_editor_update_gui (gradient_tool);
    }
  else if (! strcmp (pspec->name, "gradient-reverse"))
    {
      gimp_gradient_tool_editor_update_sliders (gradient_tool);

      /* if an endpoint is selected, swap the selected endpoint */
      if (gradient_tool->widget)
        {
          gint selection;

          selection =
            gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

          switch (selection)
            {
            case GIMP_TOOL_LINE_HANDLE_START:
              gimp_tool_line_set_selection (GIMP_TOOL_LINE (gradient_tool->widget),
                                            GIMP_TOOL_LINE_HANDLE_END);
              break;

            case GIMP_TOOL_LINE_HANDLE_END:
              gimp_tool_line_set_selection (GIMP_TOOL_LINE (gradient_tool->widget),
                                            GIMP_TOOL_LINE_HANDLE_START);
              break;
            }
        }
    }
  else if (gradient_tool->render_node &&
           gegl_node_find_property (gradient_tool->render_node, pspec->name))
    {
      gimp_gradient_tool_editor_update_sliders (gradient_tool);
    }
}

void
gimp_gradient_tool_editor_start (GimpGradientTool *gradient_tool)
{
  g_signal_connect (gradient_tool->widget, "can-add-slider",
                    G_CALLBACK (gimp_gradient_tool_editor_line_can_add_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "add-slider",
                    G_CALLBACK (gimp_gradient_tool_editor_line_add_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "prepare-to-remove-slider",
                    G_CALLBACK (gimp_gradient_tool_editor_line_prepare_to_remove_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "remove-slider",
                    G_CALLBACK (gimp_gradient_tool_editor_line_remove_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "selection-changed",
                    G_CALLBACK (gimp_gradient_tool_editor_line_selection_changed),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "handle-clicked",
                    G_CALLBACK (gimp_gradient_tool_editor_line_handle_clicked),
                    gradient_tool);
}

void
gimp_gradient_tool_editor_halt (GimpGradientTool *gradient_tool)
{
  g_clear_object (&gradient_tool->gui);

  gradient_tool->edit_count = 0;

  if (gradient_tool->undo_stack)
    {
      g_slist_free_full (gradient_tool->undo_stack,
                         (GDestroyNotify) gimp_gradient_tool_editor_gradient_info_free);
      gradient_tool->undo_stack = NULL;
    }

  if (gradient_tool->redo_stack)
    {
      g_slist_free_full (gradient_tool->redo_stack,
                         (GDestroyNotify) gimp_gradient_tool_editor_gradient_info_free);
      gradient_tool->redo_stack = NULL;
    }

  if (gradient_tool->flush_idle_id)
    {
      g_source_remove (gradient_tool->flush_idle_id);
      gradient_tool->flush_idle_id = 0;
    }
}

gboolean
gimp_gradient_tool_editor_line_changed (GimpGradientTool *gradient_tool)
{
  GimpGradientOptions        *options       = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpPaintOptions           *paint_options = GIMP_PAINT_OPTIONS (options);
  gdouble                     offset        = options->offset / 100.0;
  const GimpControllerSlider *sliders;
  gint                        n_sliders;
  gint                        i;
  GimpGradientSegment        *seg;
  gboolean                    changed       = FALSE;

  if (gimp_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return FALSE;

  if (! gradient_tool->gradient || offset == 1.0)
    return FALSE;

  sliders = gimp_tool_line_get_sliders (GIMP_TOOL_LINE (gradient_tool->widget),
                                        &n_sliders);

  if (n_sliders == 0)
    return FALSE;

  /* update the midpoints first, since moving the gradient stops may change the
   * gradient's midpoints w.r.t. the sliders, but not the other way around.
   */
  for (seg = gradient_tool->gradient->segments, i = n_sliders / 2;
       seg;
       seg = seg->next, i++)
    {
      gdouble value;

      value = sliders[i].value;

      /* adjust slider value according to the offset */
      value = (value - offset) / (1.0 - offset);

      /* flip the slider value, if necessary */
      if (paint_options->gradient_options->gradient_reverse)
        value = 1.0 - value;

      if (fabs (value - seg->middle) > EPSILON)
        {
          if (! changed)
            {
              gimp_gradient_tool_editor_start_edit (gradient_tool);
              gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

              /* refetch the segment, since the gradient might have changed */
              seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, i);

              changed = TRUE;
            }

          seg->middle = value;
        }
    }

  /* update the gradient stops */
  for (seg = gradient_tool->gradient->segments, i = 0;
       seg->next;
       seg = seg->next, i++)
    {
      gdouble value;

      value = sliders[i].value;

      /* adjust slider value according to the offset */
      value = (value - offset) / (1.0 - offset);

      /* flip the slider value, if necessary */
      if (paint_options->gradient_options->gradient_reverse)
        value = 1.0 - value;

      if (fabs (value - seg->right) > EPSILON)
        {
          if (! changed)
            {
              gimp_gradient_tool_editor_start_edit (gradient_tool);
              gimp_gradient_tool_editor_freeze_gradient (gradient_tool);

              /* refetch the segment, since the gradient might have changed */
              seg = gimp_gradient_tool_editor_handle_get_segment (gradient_tool, i);

              changed = TRUE;
            }

          gimp_gradient_segment_range_compress (gradient_tool->gradient,
                                                seg, seg,
                                                seg->left, value);
          gimp_gradient_segment_range_compress (gradient_tool->gradient,
                                                seg->next, seg->next,
                                                value, seg->next->right);
        }
    }

  if (changed)
    {
      gimp_gradient_tool_editor_thaw_gradient (gradient_tool);
      gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);
    }

  gimp_gradient_tool_editor_update_gui (gradient_tool);

  return changed;
}

void
gimp_gradient_tool_editor_fg_bg_changed (GimpGradientTool *gradient_tool)
{
  gimp_gradient_tool_editor_update_gui (gradient_tool);
}

void
gimp_gradient_tool_editor_gradient_dirty (GimpGradientTool *gradient_tool)
{
  if (gimp_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  gimp_gradient_tool_editor_purge_gradient (gradient_tool);
}

void
gimp_gradient_tool_editor_gradient_changed (GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpContext         *context = GIMP_CONTEXT (options);

  if (options->modify_active_frame)
    {
      gtk_widget_set_sensitive (options->modify_active_frame,
                                gradient_tool->gradient !=
                                gimp_gradients_get_custom (context->gimp));
    }

  if (options->modify_active_hint)
    {
      gtk_widget_set_visible (options->modify_active_hint,
                              gradient_tool->gradient &&
                              ! gimp_data_is_writable (GIMP_DATA (gradient_tool->gradient)));
    }

  if (gimp_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  gimp_gradient_tool_editor_purge_gradient (gradient_tool);
}

const gchar *
gimp_gradient_tool_editor_can_undo (GimpGradientTool *gradient_tool)
{
  if (! gradient_tool->undo_stack || gradient_tool->edit_count > 0)
    return NULL;

  return _("Gradient Step");
}

const gchar *
gimp_gradient_tool_editor_can_redo (GimpGradientTool *gradient_tool)
{
  if (! gradient_tool->redo_stack || gradient_tool->edit_count > 0)
    return NULL;

  return _("Gradient Step");
}

gboolean
gimp_gradient_tool_editor_undo (GimpGradientTool *gradient_tool)
{
  GimpTool  *tool = GIMP_TOOL (gradient_tool);
  GradientInfo *info;
  GradientInfo *new_info;

  gimp_assert (gradient_tool->undo_stack != NULL);
  gimp_assert (gradient_tool->edit_count == 0);

  info = gradient_tool->undo_stack->data;

  new_info = gimp_gradient_tool_editor_gradient_info_new (gradient_tool);

  if (info->gradient)
    {
      new_info->gradient =
        GIMP_GRADIENT (gimp_data_duplicate (GIMP_DATA (gradient_tool->gradient)));

      /* swap the added and removed handles, so that gradient_info_apply() does
       * the right thing on redo
       */
      new_info->added_handle    = info->removed_handle;
      new_info->removed_handle  = info->added_handle;
      new_info->selected_handle = info->selected_handle;
    }

  gradient_tool->undo_stack = g_slist_remove (gradient_tool->undo_stack, info);
  gradient_tool->redo_stack = g_slist_prepend (gradient_tool->redo_stack, new_info);

  gimp_gradient_tool_editor_gradient_info_apply (gradient_tool, info, TRUE);
  gimp_gradient_tool_editor_gradient_info_free (info);

  /* the initial state of the gradient tool is not useful; we might as well halt */
  if (! gradient_tool->undo_stack)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  return TRUE;
}

gboolean
gimp_gradient_tool_editor_redo (GimpGradientTool *gradient_tool)
{
  GradientInfo *info;
  GradientInfo *new_info;

  gimp_assert (gradient_tool->redo_stack != NULL);
  gimp_assert (gradient_tool->edit_count == 0);

  info = gradient_tool->redo_stack->data;

  new_info = gimp_gradient_tool_editor_gradient_info_new (gradient_tool);

  if (info->gradient)
    {
      new_info->gradient =
        GIMP_GRADIENT (gimp_data_duplicate (GIMP_DATA (gradient_tool->gradient)));

      /* swap the added and removed handles, so that gradient_info_apply() does
       * the right thing on undo
       */
      new_info->added_handle    = info->removed_handle;
      new_info->removed_handle  = info->added_handle;
      new_info->selected_handle = info->selected_handle;
    }

  gradient_tool->redo_stack = g_slist_remove (gradient_tool->redo_stack, info);
  gradient_tool->undo_stack = g_slist_prepend (gradient_tool->undo_stack, new_info);

  gimp_gradient_tool_editor_gradient_info_apply (gradient_tool, info, TRUE);
  gimp_gradient_tool_editor_gradient_info_free (info);

  return TRUE;
}

void
gimp_gradient_tool_editor_start_edit (GimpGradientTool *gradient_tool)
{
  if (gradient_tool->edit_count++ == 0)
    {
      GradientInfo *info;

      info = gimp_gradient_tool_editor_gradient_info_new (gradient_tool);

      gradient_tool->undo_stack = g_slist_prepend (gradient_tool->undo_stack, info);

      /*  update the undo actions / menu items  */
      if (! gradient_tool->flush_idle_id)
        {
          gradient_tool->flush_idle_id =
            g_idle_add ((GSourceFunc) gimp_gradient_tool_editor_flush_idle,
                        gradient_tool);
        }
    }
}

void
gimp_gradient_tool_editor_end_edit (GimpGradientTool *gradient_tool,
                                    gboolean          cancel)
{
  /* can happen when halting using esc */
  if (gradient_tool->edit_count == 0)
    return;

  if (--gradient_tool->edit_count == 0)
    {
      GradientInfo *info = gradient_tool->undo_stack->data;

      info->selected_handle =
        gimp_tool_line_get_selection (GIMP_TOOL_LINE (gradient_tool->widget));

      if (cancel ||
          gimp_gradient_tool_editor_gradient_info_is_trivial (gradient_tool, info))
        {
          /* if the edit is canceled, or if nothing changed, undo the last
           * step
           */
          gimp_gradient_tool_editor_gradient_info_apply (gradient_tool, info, FALSE);

          gradient_tool->undo_stack = g_slist_remove (gradient_tool->undo_stack,
                                                   info);
          gimp_gradient_tool_editor_gradient_info_free (info);
        }
      else
        {
          /* otherwise, blow the redo stack */
          g_slist_free_full (gradient_tool->redo_stack,
                             (GDestroyNotify) gimp_gradient_tool_editor_gradient_info_free);
          gradient_tool->redo_stack = NULL;
        }

      /*  update the undo actions / menu items  */
      if (! gradient_tool->flush_idle_id)
        {
          gradient_tool->flush_idle_id =
            g_idle_add ((GSourceFunc) gimp_gradient_tool_editor_flush_idle,
                        gradient_tool);
        }
    }
}
