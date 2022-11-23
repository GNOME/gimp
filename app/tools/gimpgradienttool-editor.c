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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "operations/ligma-operation-config.h"

#include "core/ligmadata.h"
#include "core/ligmagradient.h"
#include "core/ligma-gradients.h"
#include "core/ligmaimage.h"

#include "widgets/ligmacolorpanel.h"
#include "widgets/ligmaeditor.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmatoolgui.h"
#include "display/ligmatoolline.h"

#include "ligmagradientoptions.h"
#include "ligmagradienttool.h"
#include "ligmagradienttool-editor.h"

#include "ligma-intl.h"


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
  LigmaGradient *gradient;

  /* handle added by the operation, or HANDLE_NONE */
  gint          added_handle;
  /* handle removed by the operation, or HANDLE_NONE */
  gint          removed_handle;
  /* selected handle at the end of the operation, or HANDLE_NONE */
  gint          selected_handle;
} GradientInfo;


/*  local function prototypes  */

static gboolean              ligma_gradient_tool_editor_line_can_add_slider           (LigmaToolLine          *line,
                                                                                      gdouble                value,
                                                                                      LigmaGradientTool      *gradient_tool);
static gint                  ligma_gradient_tool_editor_line_add_slider               (LigmaToolLine          *line,
                                                                                      gdouble                value,
                                                                                      LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_line_prepare_to_remove_slider (LigmaToolLine          *line,
                                                                                      gint                   slider,
                                                                                      gboolean               remove,
                                                                                      LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_line_remove_slider            (LigmaToolLine          *line,
                                                                                      gint                   slider,
                                                                                      LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_line_selection_changed        (LigmaToolLine          *line,
                                                                                      LigmaGradientTool      *gradient_tool);
static gboolean              ligma_gradient_tool_editor_line_handle_clicked           (LigmaToolLine          *line,
                                                                                      gint                   handle,
                                                                                      GdkModifierType        state,
                                                                                      LigmaButtonPressType    press_type,
                                                                                      LigmaGradientTool      *gradient_tool);

static void                  ligma_gradient_tool_editor_gui_response                  (LigmaToolGui           *gui,
                                                                                      gint                   response_id,
                                                                                      LigmaGradientTool      *gradient_tool);

static void                  ligma_gradient_tool_editor_color_entry_color_clicked     (LigmaColorButton       *button,
                                                                                      LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_color_entry_color_changed     (LigmaColorButton       *button,
                                                                                      LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_color_entry_color_response    (LigmaColorButton       *button,
                                                                                      LigmaColorDialogState   state,
                                                                                      LigmaGradientTool      *gradient_tool);

static void                  ligma_gradient_tool_editor_color_entry_type_changed      (GtkComboBox           *combo,
                                                                                      LigmaGradientTool      *gradient_tool);

static void                  ligma_gradient_tool_editor_endpoint_se_value_changed     (LigmaSizeEntry         *se,
                                                                                      LigmaGradientTool      *gradient_tool);

static void                  ligma_gradient_tool_editor_stop_se_value_changed         (LigmaSizeEntry        *se,
                                                                                      LigmaGradientTool     *gradient_tool);

static void                  ligma_gradient_tool_editor_stop_delete_clicked           (GtkWidget             *button,
                                                                                      LigmaGradientTool      *gradient_tool);

static void                  ligma_gradient_tool_editor_midpoint_se_value_changed     (LigmaSizeEntry        *se,
                                                                                      LigmaGradientTool     *gradient_tool);

static void                  ligma_gradient_tool_editor_midpoint_type_changed         (GtkComboBox           *combo,
                                                                                      LigmaGradientTool      *gradient_tool);

static void                  ligma_gradient_tool_editor_midpoint_color_changed        (GtkComboBox           *combo,
                                                                                      LigmaGradientTool      *gradient_tool);

static void                  ligma_gradient_tool_editor_midpoint_new_stop_clicked     (GtkWidget             *button,
                                                                                      LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_midpoint_center_clicked       (GtkWidget             *button,
                                                                                      LigmaGradientTool      *gradient_tool);

static gboolean              ligma_gradient_tool_editor_flush_idle                    (LigmaGradientTool      *gradient_tool);

static gboolean              ligma_gradient_tool_editor_is_gradient_editable          (LigmaGradientTool      *gradient_tool);

static gboolean              ligma_gradient_tool_editor_handle_is_endpoint            (LigmaGradientTool      *gradient_tool,
                                                                                      gint                   handle);
static gboolean              ligma_gradient_tool_editor_handle_is_stop                (LigmaGradientTool      *gradient_tool,
                                                                                      gint                   handle);
static gboolean              ligma_gradient_tool_editor_handle_is_midpoint            (LigmaGradientTool      *gradient_tool,
                                                                                      gint                   handle);
static LigmaGradientSegment * ligma_gradient_tool_editor_handle_get_segment            (LigmaGradientTool      *gradient_tool,
                                                                                      gint                   handle);

static void                  ligma_gradient_tool_editor_block_handlers                (LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_unblock_handlers              (LigmaGradientTool      *gradient_tool);
static gboolean              ligma_gradient_tool_editor_are_handlers_blocked          (LigmaGradientTool      *gradient_tool);

static void                  ligma_gradient_tool_editor_freeze_gradient               (LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_thaw_gradient                 (LigmaGradientTool      *gradient_tool);

static gint                  ligma_gradient_tool_editor_add_stop                      (LigmaGradientTool      *gradient_tool,
                                                                                      gdouble                value);
static void                  ligma_gradient_tool_editor_delete_stop                   (LigmaGradientTool      *gradient_tool,
                                                                                      gint                   slider);
static gint                  ligma_gradient_tool_editor_midpoint_to_stop              (LigmaGradientTool      *gradient_tool,
                                                                                      gint                   slider);

static void                  ligma_gradient_tool_editor_update_sliders                (LigmaGradientTool      *gradient_tool);

static void                  ligma_gradient_tool_editor_purge_gradient_history        (GSList               **stack);
static void                  ligma_gradient_tool_editor_purge_gradient                (LigmaGradientTool      *gradient_tool);

static GtkWidget           * ligma_gradient_tool_editor_color_entry_new               (LigmaGradientTool      *gradient_tool,
                                                                                      const gchar           *title,
                                                                                      Direction              direction,
                                                                                      GtkWidget             *chain_button,
                                                                                      GtkWidget            **color_panel,
                                                                                      GtkWidget            **type_combo);
static void                  ligma_gradient_tool_editor_init_endpoint_gui             (LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_init_stop_gui                 (LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_init_midpoint_gui             (LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_update_endpoint_gui           (LigmaGradientTool      *gradient_tool,
                                                                                      gint                   selection);
static void                  ligma_gradient_tool_editor_update_stop_gui               (LigmaGradientTool      *gradient_tool,
                                                                                      gint                   selection);
static void                  ligma_gradient_tool_editor_update_midpoint_gui           (LigmaGradientTool      *gradient_tool,
                                                                                      gint                   selection);
static void                  ligma_gradient_tool_editor_update_gui                    (LigmaGradientTool      *gradient_tool);

static GradientInfo        * ligma_gradient_tool_editor_gradient_info_new             (LigmaGradientTool      *gradient_tool);
static void                  ligma_gradient_tool_editor_gradient_info_free            (GradientInfo          *info);
static void                  ligma_gradient_tool_editor_gradient_info_apply           (LigmaGradientTool      *gradient_tool,
                                                                                      const GradientInfo    *info,
                                                                                      gboolean               set_selection);
static gboolean              ligma_gradient_tool_editor_gradient_info_is_trivial      (LigmaGradientTool      *gradient_tool,
                                                                                      const GradientInfo    *info);


/*  private functions  */


static gboolean
ligma_gradient_tool_editor_line_can_add_slider (LigmaToolLine  *line,
                                               gdouble        value,
                                               LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  gdouble              offset  = options->offset / 100.0;

  return ligma_gradient_tool_editor_is_gradient_editable (gradient_tool) &&
         value >= offset;
}

static gint
ligma_gradient_tool_editor_line_add_slider (LigmaToolLine     *line,
                                           gdouble           value,
                                           LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options       = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaPaintOptions    *paint_options = LIGMA_PAINT_OPTIONS (options);
  gdouble              offset        = options->offset / 100.0;

  /* adjust slider value according to the offset */
  value = (value - offset) / (1.0 - offset);

  /* flip the slider value, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    value = 1.0 - value;

  return ligma_gradient_tool_editor_add_stop (gradient_tool, value);
}

static void
ligma_gradient_tool_editor_line_prepare_to_remove_slider (LigmaToolLine     *line,
                                                         gint              slider,
                                                         gboolean          remove,
                                                         LigmaGradientTool *gradient_tool)
{
  if (remove)
    {
      GradientInfo *info;
      LigmaGradient *tentative_gradient;

      /* show a tentative gradient, demonstrating the result of actually
       * removing the slider
       */

      info = gradient_tool->undo_stack->data;

      if (info->added_handle == slider)
        {
          /* see comment in ligma_gradient_tool_editor_delete_stop() */

          ligma_assert (info->gradient != NULL);

          tentative_gradient = g_object_ref (info->gradient);
        }
      else
        {
          LigmaGradientSegment *seg;
          gint                 i;

          tentative_gradient =
            LIGMA_GRADIENT (ligma_data_duplicate (LIGMA_DATA (gradient_tool->gradient)));

          seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, slider);

          i = ligma_gradient_segment_range_get_n_segments (gradient_tool->gradient,
                                                          gradient_tool->gradient->segments,
                                                          seg) - 1;

          seg = ligma_gradient_segment_get_nth (tentative_gradient->segments, i);

          ligma_gradient_segment_range_merge (tentative_gradient,
                                             seg, seg->next, NULL, NULL);
        }

      ligma_gradient_tool_set_tentative_gradient (gradient_tool, tentative_gradient);

      g_object_unref (tentative_gradient);
    }
  else
    {
      ligma_gradient_tool_set_tentative_gradient (gradient_tool, NULL);
    }
}

static void
ligma_gradient_tool_editor_line_remove_slider (LigmaToolLine     *line,
                                              gint              slider,
                                              LigmaGradientTool *gradient_tool)
{
  ligma_gradient_tool_editor_delete_stop (gradient_tool, slider);
  ligma_gradient_tool_set_tentative_gradient (gradient_tool, NULL);
}

static void
ligma_gradient_tool_editor_line_selection_changed (LigmaToolLine     *line,
                                                  LigmaGradientTool *gradient_tool)
{
  gint selection;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  if (gradient_tool->gui)
    {
      /* hide all color dialogs */
      ligma_color_panel_dialog_response (
        LIGMA_COLOR_PANEL (gradient_tool->endpoint_color_panel),
        LIGMA_COLOR_DIALOG_OK);
      ligma_color_panel_dialog_response (
        LIGMA_COLOR_PANEL (gradient_tool->stop_left_color_panel),
        LIGMA_COLOR_DIALOG_OK);
      ligma_color_panel_dialog_response (
        LIGMA_COLOR_PANEL (gradient_tool->stop_right_color_panel),
        LIGMA_COLOR_DIALOG_OK);

      /* reset the stop colors chain button */
      if (ligma_gradient_tool_editor_handle_is_stop (gradient_tool, selection))
        {
          const LigmaGradientSegment *seg;
          gboolean                   homogeneous;

          seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool,
                                                              selection);

          homogeneous = seg->right_color.r    == seg->next->left_color.r &&
                        seg->right_color.g    == seg->next->left_color.g &&
                        seg->right_color.b    == seg->next->left_color.b &&
                        seg->right_color.a    == seg->next->left_color.a &&
                        seg->right_color_type == seg->next->left_color_type;

          ligma_chain_button_set_active (
            LIGMA_CHAIN_BUTTON (gradient_tool->stop_chain_button), homogeneous);
        }
    }

  ligma_gradient_tool_editor_update_gui (gradient_tool);
}

static gboolean
ligma_gradient_tool_editor_line_handle_clicked (LigmaToolLine        *line,
                                               gint                 handle,
                                               GdkModifierType      state,
                                               LigmaButtonPressType  press_type,
                                               LigmaGradientTool    *gradient_tool)
{
  if (ligma_gradient_tool_editor_handle_is_midpoint (gradient_tool, handle))
    {
      if (press_type == LIGMA_BUTTON_PRESS_DOUBLE &&
          ligma_gradient_tool_editor_is_gradient_editable (gradient_tool))
        {
          gint stop;

          stop = ligma_gradient_tool_editor_midpoint_to_stop (gradient_tool, handle);

          ligma_tool_line_set_selection (line, stop);

          /* return FALSE, so that the new slider can be dragged immediately */
          return FALSE;
        }
    }

  return FALSE;
}


static void
ligma_gradient_tool_editor_gui_response (LigmaToolGui      *gui,
                                        gint              response_id,
                                        LigmaGradientTool *gradient_tool)
{
  switch (response_id)
    {
    default:
      ligma_tool_line_set_selection (LIGMA_TOOL_LINE (gradient_tool->widget),
                                    LIGMA_TOOL_LINE_HANDLE_NONE);
      break;
    }
}

static void
ligma_gradient_tool_editor_color_entry_color_clicked (LigmaColorButton  *button,
                                                     LigmaGradientTool *gradient_tool)
{
  ligma_gradient_tool_editor_start_edit (gradient_tool);
}

static void
ligma_gradient_tool_editor_color_entry_color_changed (LigmaColorButton  *button,
                                                     LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options       = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaPaintOptions    *paint_options = LIGMA_PAINT_OPTIONS (options);
  gint                 selection;
  LigmaRGB              color;
  Direction            direction;
  GtkWidget           *chain_button;
  LigmaGradientSegment *seg;

  if (ligma_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  ligma_color_button_get_color (button, &color);

  direction =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                        "ligma-gradient-tool-editor-direction"));
  chain_button = g_object_get_data (G_OBJECT (button),
                                    "ligma-gradient-tool-editor-chain-button");

  ligma_gradient_tool_editor_start_edit (gradient_tool);
  ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

  /* swap the endpoint handles, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      switch (selection)
        {
        case LIGMA_TOOL_LINE_HANDLE_START:
          selection = LIGMA_TOOL_LINE_HANDLE_END;
          break;

        case LIGMA_TOOL_LINE_HANDLE_END:
          selection = LIGMA_TOOL_LINE_HANDLE_START;
          break;
        }
    }

  seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  switch (selection)
    {
    case LIGMA_TOOL_LINE_HANDLE_START:
      seg->left_color      = color;
      seg->left_color_type = LIGMA_GRADIENT_COLOR_FIXED;
      break;

    case LIGMA_TOOL_LINE_HANDLE_END:
      seg->right_color      = color;
      seg->right_color_type = LIGMA_GRADIENT_COLOR_FIXED;
      break;

    default:
      if (direction == DIRECTION_LEFT ||
          (chain_button               &&
           ligma_chain_button_get_active (LIGMA_CHAIN_BUTTON (chain_button))))
        {
          seg->right_color      = color;
          seg->right_color_type = LIGMA_GRADIENT_COLOR_FIXED;
        }

      if (direction == DIRECTION_RIGHT ||
          (chain_button                &&
           ligma_chain_button_get_active (LIGMA_CHAIN_BUTTON (chain_button))))
        {
          seg->next->left_color      = color;
          seg->next->left_color_type = LIGMA_GRADIENT_COLOR_FIXED;
        }
    }

  ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
ligma_gradient_tool_editor_color_entry_color_response (LigmaColorButton      *button,
                                                      LigmaColorDialogState  state,
                                                      LigmaGradientTool     *gradient_tool)
{
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
ligma_gradient_tool_editor_color_entry_type_changed (GtkComboBox      *combo,
                                                    LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options       = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaPaintOptions    *paint_options = LIGMA_PAINT_OPTIONS (options);
  gint                 selection;
  gint                 color_type;
  Direction            direction;
  GtkWidget           *chain_button;
  LigmaGradientSegment *seg;

  if (ligma_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  if (! ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (combo), &color_type))
    return;

  direction =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo),
                                        "ligma-gradient-tool-editor-direction"));
  chain_button = g_object_get_data (G_OBJECT (combo),
                                    "ligma-gradient-tool-editor-chain-button");

  ligma_gradient_tool_editor_start_edit (gradient_tool);
  ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

  /* swap the endpoint handles, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      switch (selection)
        {
        case LIGMA_TOOL_LINE_HANDLE_START:
          selection = LIGMA_TOOL_LINE_HANDLE_END;
          break;

        case LIGMA_TOOL_LINE_HANDLE_END:
          selection = LIGMA_TOOL_LINE_HANDLE_START;
          break;
        }
    }

  seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  switch (selection)
    {
    case LIGMA_TOOL_LINE_HANDLE_START:
      seg->left_color_type = color_type;
      break;

    case LIGMA_TOOL_LINE_HANDLE_END:
      seg->right_color_type = color_type;
      break;

    default:
      if (direction == DIRECTION_LEFT ||
          (chain_button               &&
           ligma_chain_button_get_active (LIGMA_CHAIN_BUTTON (chain_button))))
        {
          seg->right_color_type = color_type;
        }

      if (direction == DIRECTION_RIGHT ||
          (chain_button                &&
           ligma_chain_button_get_active (LIGMA_CHAIN_BUTTON (chain_button))))
        {
          seg->next->left_color_type = color_type;
        }
    }

  ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
ligma_gradient_tool_editor_endpoint_se_value_changed (LigmaSizeEntry    *se,
                                                     LigmaGradientTool *gradient_tool)
{
  gint    selection;
  gdouble x;
  gdouble y;

  if (ligma_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  if (selection == LIGMA_TOOL_LINE_HANDLE_NONE)
    return;

  x = ligma_size_entry_get_refval (se, 0);
  y = ligma_size_entry_get_refval (se, 1);

  ligma_gradient_tool_editor_start_edit (gradient_tool);
  ligma_gradient_tool_editor_block_handlers (gradient_tool);

  switch (selection)
    {
    case LIGMA_TOOL_LINE_HANDLE_START:
      g_object_set (gradient_tool->widget,
                    "x1", x,
                    "y1", y,
                    NULL);
      break;

    case LIGMA_TOOL_LINE_HANDLE_END:
      g_object_set (gradient_tool->widget,
                    "x2", x,
                    "y2", y,
                    NULL);
      break;

    default:
      ligma_assert_not_reached ();
    }

  ligma_gradient_tool_editor_unblock_handlers (gradient_tool);
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
ligma_gradient_tool_editor_stop_se_value_changed (LigmaSizeEntry    *se,
                                                 LigmaGradientTool *gradient_tool)
{
  gint                 selection;
  gdouble              value;
  LigmaGradientSegment *seg;

  if (ligma_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  if (selection == LIGMA_TOOL_LINE_HANDLE_NONE)
    return;

  value = ligma_size_entry_get_refval (se, 0) / 100.0;

  ligma_gradient_tool_editor_start_edit (gradient_tool);
  ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  ligma_gradient_segment_range_compress (gradient_tool->gradient,
                                        seg, seg,
                                        seg->left, value);
  ligma_gradient_segment_range_compress (gradient_tool->gradient,
                                        seg->next, seg->next,
                                        value, seg->next->right);

  ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
ligma_gradient_tool_editor_stop_delete_clicked (GtkWidget        *button,
                                               LigmaGradientTool *gradient_tool)
{
  gint selection;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  ligma_gradient_tool_editor_delete_stop (gradient_tool, selection);
}

static void
ligma_gradient_tool_editor_midpoint_se_value_changed (LigmaSizeEntry    *se,
                                                     LigmaGradientTool *gradient_tool)
{
  gint                 selection;
  gdouble              value;
  LigmaGradientSegment *seg;

  if (ligma_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  if (selection == LIGMA_TOOL_LINE_HANDLE_NONE)
    return;

  value = ligma_size_entry_get_refval (se, 0) / 100.0;

  ligma_gradient_tool_editor_start_edit (gradient_tool);
  ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  seg->middle = value;

  ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
ligma_gradient_tool_editor_midpoint_type_changed (GtkComboBox      *combo,
                                                 LigmaGradientTool *gradient_tool)
{
  gint                 selection;
  gint                 type;
  LigmaGradientSegment *seg;

  if (ligma_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  if (! ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (combo), &type))
    return;

  ligma_gradient_tool_editor_start_edit (gradient_tool);
  ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  seg->type = type;

  ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
ligma_gradient_tool_editor_midpoint_color_changed (GtkComboBox      *combo,
                                                  LigmaGradientTool *gradient_tool)
{
  gint                 selection;
  gint                 color;
  LigmaGradientSegment *seg;

  if (ligma_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  if (! ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (combo), &color))
    return;

  ligma_gradient_tool_editor_start_edit (gradient_tool);
  ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  seg->color = color;

  ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
ligma_gradient_tool_editor_midpoint_new_stop_clicked (GtkWidget        *button,
                                                     LigmaGradientTool *gradient_tool)
{
  gint selection;
  gint stop;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  stop = ligma_gradient_tool_editor_midpoint_to_stop (gradient_tool, selection);

  ligma_tool_line_set_selection (LIGMA_TOOL_LINE (gradient_tool->widget), stop);
}

static void
ligma_gradient_tool_editor_midpoint_center_clicked (GtkWidget        *button,
                                                   LigmaGradientTool *gradient_tool)
{
  gint                 selection;
  LigmaGradientSegment *seg;

  selection =
    ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

  ligma_gradient_tool_editor_start_edit (gradient_tool);
  ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  ligma_gradient_segment_range_recenter_handles (gradient_tool->gradient, seg, seg);

  ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static gboolean
ligma_gradient_tool_editor_flush_idle (LigmaGradientTool *gradient_tool)
{
  LigmaDisplay *display = LIGMA_TOOL (gradient_tool)->display;

  ligma_image_flush (ligma_display_get_image (display));

  gradient_tool->flush_idle_id = 0;

  return G_SOURCE_REMOVE;
}

static gboolean
ligma_gradient_tool_editor_is_gradient_editable (LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);

  return ! options->modify_active ||
         ligma_data_is_writable (LIGMA_DATA (gradient_tool->gradient));
}

static gboolean
ligma_gradient_tool_editor_handle_is_endpoint (LigmaGradientTool *gradient_tool,
                                              gint              handle)
{
  return handle == LIGMA_TOOL_LINE_HANDLE_START ||
         handle == LIGMA_TOOL_LINE_HANDLE_END;
}

static gboolean
ligma_gradient_tool_editor_handle_is_stop (LigmaGradientTool *gradient_tool,
                                          gint              handle)
{
  gint n_sliders;

  ligma_tool_line_get_sliders (LIGMA_TOOL_LINE (gradient_tool->widget), &n_sliders);

  return handle >= 0 && handle < n_sliders / 2;
}

static gboolean
ligma_gradient_tool_editor_handle_is_midpoint (LigmaGradientTool *gradient_tool,
                                              gint              handle)
{
  gint n_sliders;

  ligma_tool_line_get_sliders (LIGMA_TOOL_LINE (gradient_tool->widget), &n_sliders);

  return handle >= n_sliders / 2;
}

static LigmaGradientSegment *
ligma_gradient_tool_editor_handle_get_segment (LigmaGradientTool *gradient_tool,
                                              gint              handle)
{
  switch (handle)
    {
    case LIGMA_TOOL_LINE_HANDLE_START:
      return gradient_tool->gradient->segments;

    case LIGMA_TOOL_LINE_HANDLE_END:
      return ligma_gradient_segment_get_last (gradient_tool->gradient->segments);

    default:
      {
        const LigmaControllerSlider *sliders;
        gint                        n_sliders;
        gint                        seg_i;

        sliders = ligma_tool_line_get_sliders (LIGMA_TOOL_LINE (gradient_tool->widget),
                                              &n_sliders);

        ligma_assert (handle >= 0 && handle < n_sliders);

        seg_i = GPOINTER_TO_INT (sliders[handle].data);

        return ligma_gradient_segment_get_nth (gradient_tool->gradient->segments,
                                              seg_i);
      }
    }
}

static void
ligma_gradient_tool_editor_block_handlers (LigmaGradientTool *gradient_tool)
{
  gradient_tool->block_handlers_count++;
}

static void
ligma_gradient_tool_editor_unblock_handlers (LigmaGradientTool *gradient_tool)
{
  ligma_assert (gradient_tool->block_handlers_count > 0);

  gradient_tool->block_handlers_count--;
}

static gboolean
ligma_gradient_tool_editor_are_handlers_blocked (LigmaGradientTool *gradient_tool)
{
  return gradient_tool->block_handlers_count > 0;
}

static void
ligma_gradient_tool_editor_freeze_gradient (LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaGradient        *custom;
  GradientInfo           *info;

  ligma_gradient_tool_editor_block_handlers (gradient_tool);

  custom = ligma_gradients_get_custom (LIGMA_CONTEXT (options)->ligma);

  if (gradient_tool->gradient == custom || options->modify_active)
    {
      ligma_assert (ligma_gradient_tool_editor_is_gradient_editable (gradient_tool));

      ligma_data_freeze (LIGMA_DATA (gradient_tool->gradient));
    }
  else
    {
      /* copy the active gradient to the custom gradient, and make the custom
       * gradient active.
       */
      ligma_data_freeze (LIGMA_DATA (custom));

      ligma_data_copy (LIGMA_DATA (custom), LIGMA_DATA (gradient_tool->gradient));

      ligma_context_set_gradient (LIGMA_CONTEXT (options), custom);

      ligma_assert (gradient_tool->gradient == custom);
      ligma_assert (ligma_gradient_tool_editor_is_gradient_editable (gradient_tool));
    }

  if (gradient_tool->edit_count > 0)
    {
      info = gradient_tool->undo_stack->data;

      if (! info->gradient)
        {
          info->gradient =
            LIGMA_GRADIENT (ligma_data_duplicate (LIGMA_DATA (gradient_tool->gradient)));
        }
    }
}

static void
ligma_gradient_tool_editor_thaw_gradient (LigmaGradientTool *gradient_tool)
{
  ligma_data_thaw (LIGMA_DATA (gradient_tool->gradient));

  ligma_gradient_tool_editor_update_sliders (gradient_tool);
  ligma_gradient_tool_editor_update_gui (gradient_tool);

  ligma_gradient_tool_editor_unblock_handlers (gradient_tool);
}

static gint
ligma_gradient_tool_editor_add_stop (LigmaGradientTool *gradient_tool,
                                    gdouble           value)
{
  LigmaGradientOptions *options       = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaPaintOptions    *paint_options = LIGMA_PAINT_OPTIONS (options);
  LigmaGradientSegment *seg;
  gint                 stop;
  GradientInfo           *info;

  ligma_gradient_tool_editor_start_edit (gradient_tool);
  ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

  ligma_gradient_split_at (gradient_tool->gradient,
                          LIGMA_CONTEXT (options), NULL, value,
                          paint_options->gradient_options->gradient_blend_color_space,
                          &seg, NULL);

  stop =
    ligma_gradient_segment_range_get_n_segments (gradient_tool->gradient,
                                                gradient_tool->gradient->segments,
                                                seg) - 1;

  info               = gradient_tool->undo_stack->data;
  info->added_handle = stop;

  ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);

  return stop;
}

static void
ligma_gradient_tool_editor_delete_stop (LigmaGradientTool *gradient_tool,
                                       gint              slider)
{
  GradientInfo *info;

  ligma_assert (ligma_gradient_tool_editor_handle_is_stop (gradient_tool, slider));

  ligma_gradient_tool_editor_start_edit (gradient_tool);
  ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

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

      ligma_assert (info->gradient != NULL);

      ligma_data_copy (LIGMA_DATA (gradient_tool->gradient),
                      LIGMA_DATA (info->gradient));

      g_clear_object (&info->gradient);

      info->added_handle = LIGMA_TOOL_LINE_HANDLE_NONE;
    }
  else
    {
      LigmaGradientSegment *seg;

      seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, slider);

      ligma_gradient_segment_range_merge (gradient_tool->gradient,
                                         seg, seg->next, NULL, NULL);

      info->removed_handle = slider;
    }

  ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
  ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static gint
ligma_gradient_tool_editor_midpoint_to_stop (LigmaGradientTool *gradient_tool,
                                            gint              slider)
{
  const LigmaControllerSlider *sliders;

  ligma_assert (ligma_gradient_tool_editor_handle_is_midpoint (gradient_tool, slider));

  sliders = ligma_tool_line_get_sliders (LIGMA_TOOL_LINE (gradient_tool->widget),
                                        NULL);

  if (sliders[slider].value > sliders[slider].min + EPSILON &&
      sliders[slider].value < sliders[slider].max - EPSILON)
    {
      const LigmaGradientSegment *seg;
      gint                       stop;
      GradientInfo                 *info;

      seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, slider);

      stop = ligma_gradient_tool_editor_add_stop (gradient_tool, seg->middle);

      info                 = gradient_tool->undo_stack->data;
      info->removed_handle = slider;

      slider = stop;
    }

  return slider;
}

static void
ligma_gradient_tool_editor_update_sliders (LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions  *options       = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaPaintOptions     *paint_options = LIGMA_PAINT_OPTIONS (options);
  gdouble               offset        = options->offset / 100.0;
  gboolean              editable;
  LigmaControllerSlider *sliders;
  gint                  n_sliders;
  gint                  n_segments;
  LigmaGradientSegment  *seg;
  LigmaControllerSlider *slider;
  gint                  i;

  if (! gradient_tool->widget || options->instant)
    return;

  editable = ligma_gradient_tool_editor_is_gradient_editable (gradient_tool);

  n_segments = ligma_gradient_segment_range_get_n_segments (
    gradient_tool->gradient, gradient_tool->gradient->segments, NULL);

  n_sliders = (n_segments - 1) + /* gradient stops, between each adjacent
                                  * pair of segments */
              (n_segments);      /* midpoints, inside each segment */

  sliders = g_new (LigmaControllerSlider, n_sliders);

  slider = sliders;

  /* initialize the gradient-stop sliders */
  for (seg = gradient_tool->gradient->segments, i = 0;
       seg->next;
       seg = seg->next, i++)
    {
      *slider = LIGMA_CONTROLLER_SLIDER_DEFAULT;

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
      *slider = LIGMA_CONTROLLER_SLIDER_DEFAULT;

      slider->value    = seg->middle;
      slider->min      = seg->left;
      slider->max      = seg->right;

      /* hide midpoints of zero-length segments, since they'd otherwise
       * prevent the segment's endpoints from being selected
       */
      slider->visible  = fabs (slider->max - slider->min) > EPSILON;
      slider->movable  = editable;

      slider->autohide = TRUE;
      slider->type     = LIGMA_HANDLE_FILLED_CIRCLE;
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

  /* avoid updating the gradient in ligma_gradient_tool_editor_line_changed() */
  ligma_gradient_tool_editor_block_handlers (gradient_tool);

  ligma_tool_line_set_sliders (LIGMA_TOOL_LINE (gradient_tool->widget),
                              sliders, n_sliders);

  ligma_gradient_tool_editor_unblock_handlers (gradient_tool);

  g_free (sliders);
}

static void
ligma_gradient_tool_editor_purge_gradient_history (GSList **stack)
{
  GSList *link;

  /* eliminate all history steps that modify the gradient */
  while ((link = *stack))
    {
      GradientInfo *info = link->data;

      if (info->gradient)
        {
          ligma_gradient_tool_editor_gradient_info_free (info);

          *stack = g_slist_delete_link (*stack, link);
        }
      else
        {
          stack = &link->next;
        }
    }
}

static void
ligma_gradient_tool_editor_purge_gradient (LigmaGradientTool *gradient_tool)
{
  if (gradient_tool->widget)
    {
      ligma_gradient_tool_editor_update_sliders (gradient_tool);

      ligma_tool_line_set_selection (LIGMA_TOOL_LINE (gradient_tool->widget),
                                    LIGMA_TOOL_LINE_HANDLE_NONE);
    }

  ligma_gradient_tool_editor_purge_gradient_history (&gradient_tool->undo_stack);
  ligma_gradient_tool_editor_purge_gradient_history (&gradient_tool->redo_stack);
}

static GtkWidget *
ligma_gradient_tool_editor_color_entry_new (LigmaGradientTool  *gradient_tool,
                                           const gchar       *title,
                                           Direction          direction,
                                           GtkWidget         *chain_button,
                                           GtkWidget        **color_panel,
                                           GtkWidget        **type_combo)
{
  LigmaContext *context = LIGMA_CONTEXT (LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool));
  GtkWidget   *hbox;
  GtkWidget   *button;
  GtkWidget   *combo;
  LigmaRGB      color   = {};

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  /* the color panel */
  *color_panel = button = ligma_color_panel_new (title, &color,
                                                LIGMA_COLOR_AREA_SMALL_CHECKS,
                                                24, 24);
  ligma_color_button_set_update (LIGMA_COLOR_BUTTON (button), TRUE);
  ligma_color_panel_set_context (LIGMA_COLOR_PANEL (button), context);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button),
                     "ligma-gradient-tool-editor-direction",
                     GINT_TO_POINTER (direction));
  g_object_set_data (G_OBJECT (button),
                     "ligma-gradient-tool-editor-chain-button",
                     chain_button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_gradient_tool_editor_color_entry_color_clicked),
                    gradient_tool);
  g_signal_connect (button, "color-changed",
                    G_CALLBACK (ligma_gradient_tool_editor_color_entry_color_changed),
                    gradient_tool);
  g_signal_connect (button, "response",
                    G_CALLBACK (ligma_gradient_tool_editor_color_entry_color_response),
                    gradient_tool);

  /* the color type combo */
  *type_combo = combo = ligma_enum_combo_box_new (LIGMA_TYPE_GRADIENT_COLOR);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
  gtk_widget_show (combo);

  g_object_set_data (G_OBJECT (combo),
                     "ligma-gradient-tool-editor-direction",
                     GINT_TO_POINTER (direction));
  g_object_set_data (G_OBJECT (combo),
                     "ligma-gradient-tool-editor-chain-button",
                     chain_button);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (ligma_gradient_tool_editor_color_entry_type_changed),
                    gradient_tool);

  return hbox;
}

static void
ligma_gradient_tool_editor_init_endpoint_gui (LigmaGradientTool *gradient_tool)
{
  LigmaDisplay      *display = LIGMA_TOOL (gradient_tool)->display;
  LigmaDisplayShell *shell   = ligma_display_get_shell (display);
  LigmaImage        *image   = ligma_display_get_image (display);
  gdouble           xres;
  gdouble           yres;
  GtkWidget        *editor;
  GtkWidget        *grid;
  GtkWidget        *label;
  GtkWidget        *spinbutton;
  GtkWidget        *se;
  GtkWidget        *hbox;
  gint              row     = 0;

  ligma_image_get_resolution (image, &xres, &yres);

  /* the endpoint editor */
  gradient_tool->endpoint_editor =
  editor                         = ligma_editor_new ();
  gtk_box_pack_start (GTK_BOX (ligma_tool_gui_get_vbox (gradient_tool->gui)),
                      editor, FALSE, TRUE, 0);

  /* the main grid */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (editor), grid, FALSE, TRUE, 0);
  gtk_widget_show (grid);

  /* the position labels */
  label = gtk_label_new (_("X:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new (_("Y:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row + 1, 1, 1);
  gtk_widget_show (label);

  /* the position size entry */
  spinbutton = ligma_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);

  gradient_tool->endpoint_se =
  se                         = ligma_size_entry_new (1, LIGMA_UNIT_PIXEL, "%a",
                                                    TRUE, TRUE, FALSE, 6,
                                                    LIGMA_SIZE_ENTRY_UPDATE_SIZE);
  gtk_grid_set_row_spacing (GTK_GRID (se), 4);
  gtk_grid_set_column_spacing (GTK_GRID (se), 2);

  ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (se), spinbutton, 1, 0, 1, 1);
  gtk_widget_show (spinbutton);

  gtk_grid_attach (GTK_GRID (grid), se, 1, row, 1, 2);
  gtk_widget_show (se);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (se), shell->unit);

  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (se), 0, xres, FALSE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (se), 1, yres, FALSE);

  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (se), 0,
                                         -LIGMA_MAX_IMAGE_SIZE,
                                         LIGMA_MAX_IMAGE_SIZE);
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (se), 1,
                                         -LIGMA_MAX_IMAGE_SIZE,
                                         LIGMA_MAX_IMAGE_SIZE);

  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (se), 0,
                            0, ligma_image_get_width (image));
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (se), 1,
                            0, ligma_image_get_height (image));

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (ligma_gradient_tool_editor_endpoint_se_value_changed),
                    gradient_tool);

  row += 2;

  /* the color label */
  label = gtk_label_new (_("Color:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  /* the color entry */
  hbox = ligma_gradient_tool_editor_color_entry_new (
    gradient_tool, _("Change Endpoint Color"), DIRECTION_NONE, NULL,
    &gradient_tool->endpoint_color_panel, &gradient_tool->endpoint_type_combo);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, row, 1, 1);
  gtk_widget_show (hbox);

  row++;
}

static void
ligma_gradient_tool_editor_init_stop_gui (LigmaGradientTool *gradient_tool)
{
  GtkWidget *editor;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *se;
  GtkWidget *grid2;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *separator;
  gint       row = 0;

  /* the stop editor */
  gradient_tool->stop_editor =
  editor                     = ligma_editor_new ();
  gtk_box_pack_start (GTK_BOX (ligma_tool_gui_get_vbox (gradient_tool->gui)),
                      editor, FALSE, TRUE, 0);

  /* the main grid */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (editor), grid, FALSE, TRUE, 0);
  gtk_widget_show (grid);

  /* the position label */
  label = gtk_label_new (_("Position:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  /* the position size entry */
  gradient_tool->stop_se =
  se                     = ligma_size_entry_new (1, LIGMA_UNIT_PERCENT, "%a",
                                                FALSE, TRUE, FALSE, 6,
                                                LIGMA_SIZE_ENTRY_UPDATE_SIZE);
  ligma_size_entry_show_unit_menu (LIGMA_SIZE_ENTRY (se), FALSE);
  gtk_grid_attach (GTK_GRID (grid), se, 1, row, 1, 1);
  gtk_widget_show (se);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (ligma_gradient_tool_editor_stop_se_value_changed),
                    gradient_tool);

  row++;

  /* the color labels */
  label = gtk_label_new (_("Left color:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new (_("Right color:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row + 1, 1, 1);
  gtk_widget_show (label);

  /* the color entries grid */
  grid2 = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid2), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid2), 2);
  gtk_grid_attach (GTK_GRID (grid), grid2, 1, row, 1, 2);
  gtk_widget_show (grid2);

  /* the color entries chain button */
  gradient_tool->stop_chain_button =
  button                           = ligma_chain_button_new (LIGMA_CHAIN_RIGHT);
  gtk_grid_attach (GTK_GRID (grid2), button, 1, 0, 1, 2);
  gtk_widget_show (button);

  /* the color entries */
  hbox = ligma_gradient_tool_editor_color_entry_new (
    gradient_tool, _("Change Stop Color"), DIRECTION_LEFT, button,
    &gradient_tool->stop_left_color_panel, &gradient_tool->stop_left_type_combo);
  gtk_grid_attach (GTK_GRID (grid2), hbox, 0, 0, 1, 1);
  gtk_widget_show (hbox);

  hbox = ligma_gradient_tool_editor_color_entry_new (
    gradient_tool, _("Change Stop Color"), DIRECTION_RIGHT, button,
    &gradient_tool->stop_right_color_panel, &gradient_tool->stop_right_type_combo);
  gtk_grid_attach (GTK_GRID (grid2), hbox, 0, 1, 1, 1);
  gtk_widget_show (hbox);

  row += 2;

  /* the action buttons separator */
  separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach (GTK_GRID (grid), separator, 0, row, 2, 1);
  gtk_widget_show (separator);

  row++;

  /* the delete button */
  ligma_editor_add_button (LIGMA_EDITOR (editor),
                          LIGMA_ICON_EDIT_DELETE, _("Delete stop"),
                          NULL,
                          G_CALLBACK (ligma_gradient_tool_editor_stop_delete_clicked),
                          NULL, gradient_tool);
}

static void
ligma_gradient_tool_editor_init_midpoint_gui (LigmaGradientTool *gradient_tool)
{
  GtkWidget *editor;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *se;
  GtkWidget *combo;
  GtkWidget *separator;
  gint       row = 0;

  /* the stop editor */
  gradient_tool->midpoint_editor =
  editor                         = ligma_editor_new ();
  gtk_box_pack_start (GTK_BOX (ligma_tool_gui_get_vbox (gradient_tool->gui)),
                      editor, FALSE, TRUE, 0);

  /* the main grid */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (editor), grid, FALSE, TRUE, 0);
  gtk_widget_show (grid);

  /* the position label */
  label = gtk_label_new (_("Position:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  /* the position size entry */
  gradient_tool->midpoint_se =
  se                         = ligma_size_entry_new (1, LIGMA_UNIT_PERCENT, "%a",
                                                    FALSE, TRUE, FALSE, 6,
                                                    LIGMA_SIZE_ENTRY_UPDATE_SIZE);
  ligma_size_entry_show_unit_menu (LIGMA_SIZE_ENTRY (se), FALSE);
  gtk_grid_attach (GTK_GRID (grid), se, 1, row, 1, 1);
  gtk_widget_show (se);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (ligma_gradient_tool_editor_midpoint_se_value_changed),
                    gradient_tool);

  row++;

  /* the type label */
  label = gtk_label_new (_("Blending:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  /* the type combo */
  gradient_tool->midpoint_type_combo =
  combo                              = ligma_enum_combo_box_new (LIGMA_TYPE_GRADIENT_SEGMENT_TYPE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (ligma_gradient_tool_editor_midpoint_type_changed),
                    gradient_tool);

  row++;

  /* the color label */
  label = gtk_label_new (_("Coloring:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  /* the color combo */
  gradient_tool->midpoint_color_combo =
  combo                              = ligma_enum_combo_box_new (LIGMA_TYPE_GRADIENT_SEGMENT_COLOR);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (ligma_gradient_tool_editor_midpoint_color_changed),
                    gradient_tool);

  row++;

  /* the action buttons separator */
  separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach (GTK_GRID (grid), separator, 0, row, 2, 1);
  gtk_widget_show (separator);

  row++;

  /* the new stop button */
  gradient_tool->midpoint_new_stop_button =
    ligma_editor_add_button (LIGMA_EDITOR (editor),
                            LIGMA_ICON_DOCUMENT_NEW, _("New stop at midpoint"),
                            NULL,
                            G_CALLBACK (ligma_gradient_tool_editor_midpoint_new_stop_clicked),
                            NULL, gradient_tool);

  /* the center button */
  gradient_tool->midpoint_center_button =
    ligma_editor_add_button (LIGMA_EDITOR (editor),
                            LIGMA_ICON_CENTER_HORIZONTAL, _("Center midpoint"),
                            NULL,
                            G_CALLBACK (ligma_gradient_tool_editor_midpoint_center_clicked),
                            NULL, gradient_tool);
}

static void
ligma_gradient_tool_editor_update_endpoint_gui (LigmaGradientTool *gradient_tool,
                                               gint              selection)
{
  LigmaGradientOptions *options       = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaPaintOptions    *paint_options = LIGMA_PAINT_OPTIONS (options);
  LigmaContext         *context       = LIGMA_CONTEXT (options);
  gboolean             editable;
  LigmaGradientSegment *seg;
  const gchar         *title;
  gdouble              x;
  gdouble              y;
  LigmaRGB              color;
  LigmaGradientColor    color_type;

  editable = ligma_gradient_tool_editor_is_gradient_editable (gradient_tool);

  switch (selection)
    {
    case LIGMA_TOOL_LINE_HANDLE_START:
      g_object_get (gradient_tool->widget,
                    "x1", &x,
                    "y1", &y,
                    NULL);
      break;

    case LIGMA_TOOL_LINE_HANDLE_END:
      g_object_get (gradient_tool->widget,
                    "x2", &x,
                    "y2", &y,
                    NULL);
      break;

    default:
      ligma_assert_not_reached ();
    }

  /* swap the endpoint handles, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      switch (selection)
        {
        case LIGMA_TOOL_LINE_HANDLE_START:
          selection = LIGMA_TOOL_LINE_HANDLE_END;
          break;

        case LIGMA_TOOL_LINE_HANDLE_END:
          selection = LIGMA_TOOL_LINE_HANDLE_START;
          break;
        }
    }

  seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  switch (selection)
    {
    case LIGMA_TOOL_LINE_HANDLE_START:
      title = _("Start Endpoint");

      ligma_gradient_segment_get_left_flat_color (gradient_tool->gradient, context,
                                                 seg, &color);
      color_type = seg->left_color_type;
      break;

    case LIGMA_TOOL_LINE_HANDLE_END:
      title = _("End Endpoint");

      ligma_gradient_segment_get_right_flat_color (gradient_tool->gradient, context,
                                                  seg, &color);
      color_type = seg->right_color_type;
      break;

    default:
      ligma_assert_not_reached ();
    }

  ligma_tool_gui_set_title (gradient_tool->gui, title);

  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (gradient_tool->endpoint_se), 0, x);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (gradient_tool->endpoint_se), 1, y);

  ligma_color_button_set_color (
    LIGMA_COLOR_BUTTON (gradient_tool->endpoint_color_panel), &color);
  ligma_int_combo_box_set_active (
    LIGMA_INT_COMBO_BOX (gradient_tool->endpoint_type_combo), color_type);

  gtk_widget_set_sensitive (gradient_tool->endpoint_color_panel, editable);
  gtk_widget_set_sensitive (gradient_tool->endpoint_type_combo,  editable);

  gtk_widget_show (gradient_tool->endpoint_editor);
}

static void
ligma_gradient_tool_editor_update_stop_gui (LigmaGradientTool *gradient_tool,
                                           gint              selection)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaContext         *context = LIGMA_CONTEXT (options);
  gboolean             editable;
  LigmaGradientSegment *seg;
  gint                 index;
  gchar               *title;
  gdouble              min;
  gdouble              max;
  gdouble              value;
  LigmaRGB              left_color;
  LigmaGradientColor    left_color_type;
  LigmaRGB              right_color;
  LigmaGradientColor    right_color_type;

  editable = ligma_gradient_tool_editor_is_gradient_editable (gradient_tool);

  seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  index = GPOINTER_TO_INT (
    ligma_tool_line_get_sliders (LIGMA_TOOL_LINE (gradient_tool->widget),
                                NULL)[selection].data);

  title = g_strdup_printf (_("Stop %d"), index + 1);

  min   = seg->left;
  max   = seg->next->right;
  value = seg->right;

  ligma_gradient_segment_get_right_flat_color (gradient_tool->gradient, context,
                                              seg, &left_color);
  left_color_type = seg->right_color_type;

  ligma_gradient_segment_get_left_flat_color (gradient_tool->gradient, context,
                                             seg->next, &right_color);
  right_color_type = seg->next->left_color_type;

  ligma_tool_gui_set_title (gradient_tool->gui, title);

  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (gradient_tool->stop_se),
                                         0, 100.0 * min, 100.0 * max);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (gradient_tool->stop_se),
                              0, 100.0 * value);

  ligma_color_button_set_color (
    LIGMA_COLOR_BUTTON (gradient_tool->stop_left_color_panel), &left_color);
  ligma_int_combo_box_set_active (
    LIGMA_INT_COMBO_BOX (gradient_tool->stop_left_type_combo), left_color_type);

  ligma_color_button_set_color (
    LIGMA_COLOR_BUTTON (gradient_tool->stop_right_color_panel), &right_color);
  ligma_int_combo_box_set_active (
    LIGMA_INT_COMBO_BOX (gradient_tool->stop_right_type_combo), right_color_type);

  gtk_widget_set_sensitive (gradient_tool->stop_se,                editable);
  gtk_widget_set_sensitive (gradient_tool->stop_left_color_panel,  editable);
  gtk_widget_set_sensitive (gradient_tool->stop_left_type_combo,   editable);
  gtk_widget_set_sensitive (gradient_tool->stop_right_color_panel, editable);
  gtk_widget_set_sensitive (gradient_tool->stop_right_type_combo,  editable);
  gtk_widget_set_sensitive (gradient_tool->stop_chain_button,      editable);
  gtk_widget_set_sensitive (
    GTK_WIDGET (ligma_editor_get_button_box (LIGMA_EDITOR (gradient_tool->stop_editor))),
    editable);

  g_free (title);

  gtk_widget_show (gradient_tool->stop_editor);
}

static void
ligma_gradient_tool_editor_update_midpoint_gui (LigmaGradientTool *gradient_tool,
                                               gint              selection)
{
  gboolean                    editable;
  const LigmaGradientSegment  *seg;
  gint                        index;
  gchar                      *title;
  gdouble                     min;
  gdouble                     max;
  gdouble                     value;
  LigmaGradientSegmentType     type;
  LigmaGradientSegmentColor    color;

  editable = ligma_gradient_tool_editor_is_gradient_editable (gradient_tool);

  seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  index = GPOINTER_TO_INT (
    ligma_tool_line_get_sliders (LIGMA_TOOL_LINE (gradient_tool->widget),
                                NULL)[selection].data);

  title = g_strdup_printf (_("Midpoint %d"), index + 1);

  min   = seg->left;
  max   = seg->right;
  value = seg->middle;
  type  = seg->type;
  color = seg->color;

  ligma_tool_gui_set_title (gradient_tool->gui, title);

  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (gradient_tool->midpoint_se),
                                         0, 100.0 * min, 100.0 * max);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (gradient_tool->midpoint_se),
                              0, 100.0 * value);

  ligma_int_combo_box_set_active (
    LIGMA_INT_COMBO_BOX (gradient_tool->midpoint_type_combo), type);

  ligma_int_combo_box_set_active (
    LIGMA_INT_COMBO_BOX (gradient_tool->midpoint_color_combo), color);

  gtk_widget_set_sensitive (gradient_tool->midpoint_new_stop_button,
                            value > min + EPSILON && value < max - EPSILON);
  gtk_widget_set_sensitive (gradient_tool->midpoint_center_button,
                            fabs (value - (min + max) / 2.0) > EPSILON);

  gtk_widget_set_sensitive (gradient_tool->midpoint_se,          editable);
  gtk_widget_set_sensitive (gradient_tool->midpoint_type_combo,  editable);
  gtk_widget_set_sensitive (gradient_tool->midpoint_color_combo, editable);
  gtk_widget_set_sensitive (
    GTK_WIDGET (ligma_editor_get_button_box (LIGMA_EDITOR (gradient_tool->midpoint_editor))),
    editable);

  g_free (title);

  gtk_widget_show (gradient_tool->midpoint_editor);
}

static void
ligma_gradient_tool_editor_update_gui (LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);

  if (gradient_tool->gradient && gradient_tool->widget && ! options->instant)
    {
      gint selection;

      selection =
        ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

      if (selection != LIGMA_TOOL_LINE_HANDLE_NONE)
        {
          if (! gradient_tool->gui)
            {
              LigmaDisplayShell *shell;

              shell = ligma_tool_widget_get_shell (gradient_tool->widget);

              gradient_tool->gui =
                ligma_tool_gui_new (LIGMA_TOOL (gradient_tool)->tool_info,
                                   NULL, NULL, NULL, NULL,
                                   ligma_widget_get_monitor (GTK_WIDGET (shell)),
                                   TRUE,

                                   _("_Close"), GTK_RESPONSE_CLOSE,

                                   NULL);

              ligma_tool_gui_set_shell (gradient_tool->gui, shell);
              ligma_tool_gui_set_viewable (gradient_tool->gui,
                                          LIGMA_VIEWABLE (gradient_tool->gradient));
              ligma_tool_gui_set_auto_overlay (gradient_tool->gui, TRUE);

              g_signal_connect (gradient_tool->gui, "response",
                                G_CALLBACK (ligma_gradient_tool_editor_gui_response),
                                gradient_tool);

              ligma_gradient_tool_editor_init_endpoint_gui (gradient_tool);
              ligma_gradient_tool_editor_init_stop_gui     (gradient_tool);
              ligma_gradient_tool_editor_init_midpoint_gui (gradient_tool);
            }

          ligma_gradient_tool_editor_block_handlers (gradient_tool);

          if (ligma_gradient_tool_editor_handle_is_endpoint (gradient_tool, selection))
            ligma_gradient_tool_editor_update_endpoint_gui (gradient_tool, selection);
          else
            gtk_widget_hide (gradient_tool->endpoint_editor);

          if (ligma_gradient_tool_editor_handle_is_stop (gradient_tool, selection))
            ligma_gradient_tool_editor_update_stop_gui (gradient_tool, selection);
          else
            gtk_widget_hide (gradient_tool->stop_editor);

          if (ligma_gradient_tool_editor_handle_is_midpoint (gradient_tool, selection))
            ligma_gradient_tool_editor_update_midpoint_gui (gradient_tool, selection);
          else
            gtk_widget_hide (gradient_tool->midpoint_editor);

          ligma_gradient_tool_editor_unblock_handlers (gradient_tool);

          ligma_tool_gui_show (gradient_tool->gui);

          return;
        }
    }

  if (gradient_tool->gui)
    ligma_tool_gui_hide (gradient_tool->gui);
}

static GradientInfo *
ligma_gradient_tool_editor_gradient_info_new (LigmaGradientTool *gradient_tool)
{
  GradientInfo *info = g_slice_new (GradientInfo);

  info->start_x         = gradient_tool->start_x;
  info->start_y         = gradient_tool->start_y;
  info->end_x           = gradient_tool->end_x;
  info->end_y           = gradient_tool->end_y;

  info->gradient        = NULL;

  info->added_handle    = LIGMA_TOOL_LINE_HANDLE_NONE;
  info->removed_handle  = LIGMA_TOOL_LINE_HANDLE_NONE;
  info->selected_handle = LIGMA_TOOL_LINE_HANDLE_NONE;

  return info;
}

static void
ligma_gradient_tool_editor_gradient_info_free (GradientInfo *info)
{
  if (info->gradient)
    g_object_unref (info->gradient);

  g_slice_free (GradientInfo, info);
}

static void
ligma_gradient_tool_editor_gradient_info_apply (LigmaGradientTool   *gradient_tool,
                                               const GradientInfo *info,
                                               gboolean            set_selection)
{
  gint selection;

  ligma_assert (gradient_tool->widget   != NULL);
  ligma_assert (gradient_tool->gradient != NULL);

  /* pick the handle to select */
  if (info->gradient)
    {
      if (info->removed_handle != LIGMA_TOOL_LINE_HANDLE_NONE)
        {
          /* we're undoing a stop-deletion or midpoint-to-stop operation;
           * select the removed handle
           */
          selection = info->removed_handle;
        }
      else if (info->added_handle != LIGMA_TOOL_LINE_HANDLE_NONE)
        {
          /* we're undoing a stop addition operation */
          ligma_assert (ligma_gradient_tool_editor_handle_is_stop (gradient_tool,
                                                                 info->added_handle));

          selection =
            ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

          /* if the selected handle is a stop... */
          if (ligma_gradient_tool_editor_handle_is_stop (gradient_tool, selection))
            {
              /* if the added handle is selected, clear the selection */
              if (selection == info->added_handle)
                selection = LIGMA_TOOL_LINE_HANDLE_NONE;
              /* otherwise, keep the currently selected stop, possibly
               * adjusting its handle index
               */
              else if (selection > info->added_handle)
                selection--;
            }
          /* otherwise, if the selected handle is a midpoint... */
          else if (ligma_gradient_tool_editor_handle_is_midpoint (gradient_tool, selection))
            {
              const LigmaControllerSlider *sliders;
              gint                        seg_i;

              sliders =
                ligma_tool_line_get_sliders (LIGMA_TOOL_LINE (gradient_tool->widget),
                                            NULL);

              seg_i = GPOINTER_TO_INT (sliders[selection].data);

              /* if the midpoint belongs to one of the two segments incident to
               * the added stop, clear the selection
               */
              if (seg_i == info->added_handle ||
                  seg_i == info->added_handle + 1)
                {
                  selection = LIGMA_TOOL_LINE_HANDLE_NONE;
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
      else if (info->selected_handle != LIGMA_TOOL_LINE_HANDLE_NONE)
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
      selection = LIGMA_TOOL_LINE_HANDLE_START;
    }
  else if ((info->end_x   != gradient_tool->end_x    ||
            info->end_y   != gradient_tool->end_y)   &&
           (info->start_x == gradient_tool->start_x  &&
            info->start_y == gradient_tool->start_y))

    {
      /* we're undoing am end-endpoint move operation; select the end
       * endpoint
       */
      selection = LIGMA_TOOL_LINE_HANDLE_END;
    }
  else
    {
      /* we're undoing a line move operation; don't change the selection */
      set_selection = FALSE;
    }

  ligma_gradient_tool_editor_block_handlers (gradient_tool);

  g_object_set (gradient_tool->widget,
                "x1", info->start_x,
                "y1", info->start_y,
                "x2", info->end_x,
                "y2", info->end_y,
                NULL);

  if (info->gradient)
    {
      ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

      ligma_data_copy (LIGMA_DATA (gradient_tool->gradient),
                      LIGMA_DATA (info->gradient));

      ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
    }

  if (set_selection)
    {
      ligma_tool_line_set_selection (LIGMA_TOOL_LINE (gradient_tool->widget),
                                    selection);
    }

  ligma_gradient_tool_editor_update_gui (gradient_tool);

  ligma_gradient_tool_editor_unblock_handlers (gradient_tool);
}

static gboolean
ligma_gradient_tool_editor_gradient_info_is_trivial (LigmaGradientTool   *gradient_tool,
                                                    const GradientInfo *info)
{
  const LigmaGradientSegment *seg1;
  const LigmaGradientSegment *seg2;

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
          if (memcmp (seg1, seg2, G_STRUCT_OFFSET (LigmaGradientSegment, prev)))
            return FALSE;
        }

      if (seg1 || seg2)
        return FALSE;
    }

  return TRUE;
}


/*  public functions  */

void
ligma_gradient_tool_editor_options_notify (LigmaGradientTool *gradient_tool,
                                          LigmaToolOptions  *options,
                                          const GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "modify-active"))
    {
      ligma_gradient_tool_editor_update_sliders (gradient_tool);
      ligma_gradient_tool_editor_update_gui (gradient_tool);
    }
  else if (! strcmp (pspec->name, "gradient-reverse"))
    {
      ligma_gradient_tool_editor_update_sliders (gradient_tool);

      /* if an endpoint is selected, swap the selected endpoint */
      if (gradient_tool->widget)
        {
          gint selection;

          selection =
            ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

          switch (selection)
            {
            case LIGMA_TOOL_LINE_HANDLE_START:
              ligma_tool_line_set_selection (LIGMA_TOOL_LINE (gradient_tool->widget),
                                            LIGMA_TOOL_LINE_HANDLE_END);
              break;

            case LIGMA_TOOL_LINE_HANDLE_END:
              ligma_tool_line_set_selection (LIGMA_TOOL_LINE (gradient_tool->widget),
                                            LIGMA_TOOL_LINE_HANDLE_START);
              break;
            }
        }
    }
  else if (gradient_tool->render_node &&
           gegl_node_find_property (gradient_tool->render_node, pspec->name))
    {
      ligma_gradient_tool_editor_update_sliders (gradient_tool);
    }
}

void
ligma_gradient_tool_editor_start (LigmaGradientTool *gradient_tool)
{
  g_signal_connect (gradient_tool->widget, "can-add-slider",
                    G_CALLBACK (ligma_gradient_tool_editor_line_can_add_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "add-slider",
                    G_CALLBACK (ligma_gradient_tool_editor_line_add_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "prepare-to-remove-slider",
                    G_CALLBACK (ligma_gradient_tool_editor_line_prepare_to_remove_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "remove-slider",
                    G_CALLBACK (ligma_gradient_tool_editor_line_remove_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "selection-changed",
                    G_CALLBACK (ligma_gradient_tool_editor_line_selection_changed),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "handle-clicked",
                    G_CALLBACK (ligma_gradient_tool_editor_line_handle_clicked),
                    gradient_tool);
}

void
ligma_gradient_tool_editor_halt (LigmaGradientTool *gradient_tool)
{
  g_clear_object (&gradient_tool->gui);

  gradient_tool->edit_count = 0;

  if (gradient_tool->undo_stack)
    {
      g_slist_free_full (gradient_tool->undo_stack,
                         (GDestroyNotify) ligma_gradient_tool_editor_gradient_info_free);
      gradient_tool->undo_stack = NULL;
    }

  if (gradient_tool->redo_stack)
    {
      g_slist_free_full (gradient_tool->redo_stack,
                         (GDestroyNotify) ligma_gradient_tool_editor_gradient_info_free);
      gradient_tool->redo_stack = NULL;
    }

  if (gradient_tool->flush_idle_id)
    {
      g_source_remove (gradient_tool->flush_idle_id);
      gradient_tool->flush_idle_id = 0;
    }
}

gboolean
ligma_gradient_tool_editor_line_changed (LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions        *options       = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaPaintOptions           *paint_options = LIGMA_PAINT_OPTIONS (options);
  gdouble                     offset        = options->offset / 100.0;
  const LigmaControllerSlider *sliders;
  gint                        n_sliders;
  gint                        i;
  LigmaGradientSegment        *seg;
  gboolean                    changed       = FALSE;

  if (ligma_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return FALSE;

  if (! gradient_tool->gradient || offset == 1.0)
    return FALSE;

  sliders = ligma_tool_line_get_sliders (LIGMA_TOOL_LINE (gradient_tool->widget),
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
              ligma_gradient_tool_editor_start_edit (gradient_tool);
              ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

              /* refetch the segment, since the gradient might have changed */
              seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, i);

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
              ligma_gradient_tool_editor_start_edit (gradient_tool);
              ligma_gradient_tool_editor_freeze_gradient (gradient_tool);

              /* refetch the segment, since the gradient might have changed */
              seg = ligma_gradient_tool_editor_handle_get_segment (gradient_tool, i);

              changed = TRUE;
            }

          ligma_gradient_segment_range_compress (gradient_tool->gradient,
                                                seg, seg,
                                                seg->left, value);
          ligma_gradient_segment_range_compress (gradient_tool->gradient,
                                                seg->next, seg->next,
                                                value, seg->next->right);
        }
    }

  if (changed)
    {
      ligma_gradient_tool_editor_thaw_gradient (gradient_tool);
      ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);
    }

  ligma_gradient_tool_editor_update_gui (gradient_tool);

  return changed;
}

void
ligma_gradient_tool_editor_fg_bg_changed (LigmaGradientTool *gradient_tool)
{
  ligma_gradient_tool_editor_update_gui (gradient_tool);
}

void
ligma_gradient_tool_editor_gradient_dirty (LigmaGradientTool *gradient_tool)
{
  if (ligma_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  ligma_gradient_tool_editor_purge_gradient (gradient_tool);
}

void
ligma_gradient_tool_editor_gradient_changed (LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaContext         *context = LIGMA_CONTEXT (options);

  if (options->modify_active_frame)
    {
      gtk_widget_set_sensitive (options->modify_active_frame,
                                gradient_tool->gradient !=
                                ligma_gradients_get_custom (context->ligma));
    }

  if (options->modify_active_hint)
    {
      gtk_widget_set_visible (options->modify_active_hint,
                              gradient_tool->gradient &&
                              ! ligma_data_is_writable (LIGMA_DATA (gradient_tool->gradient)));
    }

  if (ligma_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  ligma_gradient_tool_editor_purge_gradient (gradient_tool);
}

const gchar *
ligma_gradient_tool_editor_can_undo (LigmaGradientTool *gradient_tool)
{
  if (! gradient_tool->undo_stack || gradient_tool->edit_count > 0)
    return NULL;

  return _("Gradient Step");
}

const gchar *
ligma_gradient_tool_editor_can_redo (LigmaGradientTool *gradient_tool)
{
  if (! gradient_tool->redo_stack || gradient_tool->edit_count > 0)
    return NULL;

  return _("Gradient Step");
}

gboolean
ligma_gradient_tool_editor_undo (LigmaGradientTool *gradient_tool)
{
  LigmaTool  *tool = LIGMA_TOOL (gradient_tool);
  GradientInfo *info;
  GradientInfo *new_info;

  ligma_assert (gradient_tool->undo_stack != NULL);
  ligma_assert (gradient_tool->edit_count == 0);

  info = gradient_tool->undo_stack->data;

  new_info = ligma_gradient_tool_editor_gradient_info_new (gradient_tool);

  if (info->gradient)
    {
      new_info->gradient =
        LIGMA_GRADIENT (ligma_data_duplicate (LIGMA_DATA (gradient_tool->gradient)));

      /* swap the added and removed handles, so that gradient_info_apply() does
       * the right thing on redo
       */
      new_info->added_handle    = info->removed_handle;
      new_info->removed_handle  = info->added_handle;
      new_info->selected_handle = info->selected_handle;
    }

  gradient_tool->undo_stack = g_slist_remove (gradient_tool->undo_stack, info);
  gradient_tool->redo_stack = g_slist_prepend (gradient_tool->redo_stack, new_info);

  ligma_gradient_tool_editor_gradient_info_apply (gradient_tool, info, TRUE);
  ligma_gradient_tool_editor_gradient_info_free (info);

  /* the initial state of the gradient tool is not useful; we might as well halt */
  if (! gradient_tool->undo_stack)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);

  return TRUE;
}

gboolean
ligma_gradient_tool_editor_redo (LigmaGradientTool *gradient_tool)
{
  GradientInfo *info;
  GradientInfo *new_info;

  ligma_assert (gradient_tool->redo_stack != NULL);
  ligma_assert (gradient_tool->edit_count == 0);

  info = gradient_tool->redo_stack->data;

  new_info = ligma_gradient_tool_editor_gradient_info_new (gradient_tool);

  if (info->gradient)
    {
      new_info->gradient =
        LIGMA_GRADIENT (ligma_data_duplicate (LIGMA_DATA (gradient_tool->gradient)));

      /* swap the added and removed handles, so that gradient_info_apply() does
       * the right thing on undo
       */
      new_info->added_handle    = info->removed_handle;
      new_info->removed_handle  = info->added_handle;
      new_info->selected_handle = info->selected_handle;
    }

  gradient_tool->redo_stack = g_slist_remove (gradient_tool->redo_stack, info);
  gradient_tool->undo_stack = g_slist_prepend (gradient_tool->undo_stack, new_info);

  ligma_gradient_tool_editor_gradient_info_apply (gradient_tool, info, TRUE);
  ligma_gradient_tool_editor_gradient_info_free (info);

  return TRUE;
}

void
ligma_gradient_tool_editor_start_edit (LigmaGradientTool *gradient_tool)
{
  if (gradient_tool->edit_count++ == 0)
    {
      GradientInfo *info;

      info = ligma_gradient_tool_editor_gradient_info_new (gradient_tool);

      gradient_tool->undo_stack = g_slist_prepend (gradient_tool->undo_stack, info);

      /*  update the undo actions / menu items  */
      if (! gradient_tool->flush_idle_id)
        {
          gradient_tool->flush_idle_id =
            g_idle_add ((GSourceFunc) ligma_gradient_tool_editor_flush_idle,
                        gradient_tool);
        }
    }
}

void
ligma_gradient_tool_editor_end_edit (LigmaGradientTool *gradient_tool,
                                    gboolean          cancel)
{
  /* can happen when halting using esc */
  if (gradient_tool->edit_count == 0)
    return;

  if (--gradient_tool->edit_count == 0)
    {
      GradientInfo *info = gradient_tool->undo_stack->data;

      info->selected_handle =
        ligma_tool_line_get_selection (LIGMA_TOOL_LINE (gradient_tool->widget));

      if (cancel ||
          ligma_gradient_tool_editor_gradient_info_is_trivial (gradient_tool, info))
        {
          /* if the edit is canceled, or if nothing changed, undo the last
           * step
           */
          ligma_gradient_tool_editor_gradient_info_apply (gradient_tool, info, FALSE);

          gradient_tool->undo_stack = g_slist_remove (gradient_tool->undo_stack,
                                                   info);
          ligma_gradient_tool_editor_gradient_info_free (info);
        }
      else
        {
          /* otherwise, blow the redo stack */
          g_slist_free_full (gradient_tool->redo_stack,
                             (GDestroyNotify) ligma_gradient_tool_editor_gradient_info_free);
          gradient_tool->redo_stack = NULL;
        }

      /*  update the undo actions / menu items  */
      if (! gradient_tool->flush_idle_id)
        {
          gradient_tool->flush_idle_id =
            g_idle_add ((GSourceFunc) ligma_gradient_tool_editor_flush_idle,
                        gradient_tool);
        }
    }
}
