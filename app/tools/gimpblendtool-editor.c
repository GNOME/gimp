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

#include "gimpblendoptions.h"
#include "gimpblendtool.h"
#include "gimpblendtool-editor.h"

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

  /* copy of the gradient at the beginning of the operation, owned by the blend
   * info, or NULL, if the gradient isn't affected
   */
  GimpGradient *gradient;

  /* handle added by the operation, or HANDLE_NONE */
  gint          added_handle;
  /* handle removed by the operation, or HANDLE_NONE */
  gint          removed_handle;
  /* selected handle at the end of the operation, or HANDLE_NONE */
  gint          selected_handle;
} BlendInfo;


/*  local function prototypes  */

static gboolean              gimp_blend_tool_editor_line_can_add_slider           (GimpToolLine          *line,
                                                                                   gdouble                value,
                                                                                   GimpBlendTool         *blend_tool);
static gint                  gimp_blend_tool_editor_line_add_slider               (GimpToolLine          *line,
                                                                                   gdouble                value,
                                                                                   GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_line_prepare_to_remove_slider (GimpToolLine        *line,
                                                                                   gint                 slider,
                                                                                   gboolean             remove,
                                                                                   GimpBlendTool       *blend_tool);
static void                  gimp_blend_tool_editor_line_remove_slider            (GimpToolLine          *line,
                                                                                   gint                   slider,
                                                                                   GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_line_selection_changed        (GimpToolLine          *line,
                                                                                   GimpBlendTool         *blend_tool);
static gboolean              gimp_blend_tool_editor_line_handle_clicked           (GimpToolLine          *line,
                                                                                   gint                   handle,
                                                                                   GdkModifierType        state,
                                                                                   GimpButtonPressType    press_type,
                                                                                   GimpBlendTool         *blend_tool);

static void                  gimp_blend_tool_editor_gui_response                  (GimpToolGui           *gui,
                                                                                   gint                   response_id,
                                                                                   GimpBlendTool         *blend_tool);

static void                  gimp_blend_tool_editor_color_entry_color_clicked     (GimpColorButton       *button,
                                                                                   GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_color_entry_color_changed     (GimpColorButton       *button,
                                                                                   GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_color_entry_color_response    (GimpColorButton       *button,
                                                                                   GimpColorDialogState   state,
                                                                                   GimpBlendTool         *blend_tool);

static void                  gimp_blend_tool_editor_color_entry_type_changed      (GtkComboBox           *combo,
                                                                                   GimpBlendTool         *blend_tool);

static void                  gimp_blend_tool_editor_endpoint_se_value_changed     (GimpSizeEntry         *se,
                                                                                   GimpBlendTool         *blend_tool);

static void                  gimp_blend_tool_editor_stop_se_value_changed         (GimpSizeEntry        *se,
                                                                                   GimpBlendTool        *blend_tool);

static void                  gimp_blend_tool_editor_stop_delete_clicked           (GtkWidget             *button,
                                                                                   GimpBlendTool         *blend_tool);

static void                  gimp_blend_tool_editor_midpoint_se_value_changed     (GimpSizeEntry        *se,
                                                                                   GimpBlendTool        *blend_tool);

static void                  gimp_blend_tool_editor_midpoint_type_changed         (GtkComboBox           *combo,
                                                                                   GimpBlendTool         *blend_tool);

static void                  gimp_blend_tool_editor_midpoint_color_changed        (GtkComboBox           *combo,
                                                                                   GimpBlendTool         *blend_tool);

static void                  gimp_blend_tool_editor_midpoint_new_stop_clicked     (GtkWidget             *button,
                                                                                   GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_midpoint_center_clicked       (GtkWidget             *button,
                                                                                   GimpBlendTool         *blend_tool);

static gboolean              gimp_blend_tool_editor_flush_idle                    (GimpBlendTool         *blend_tool);

static gboolean              gimp_blend_tool_editor_is_gradient_editable          (GimpBlendTool         *blend_tool);

static gboolean              gimp_blend_tool_editor_handle_is_endpoint            (GimpBlendTool         *blend_tool,
                                                                                   gint                   handle);
static gboolean              gimp_blend_tool_editor_handle_is_stop                (GimpBlendTool         *blend_tool,
                                                                                   gint                   handle);
static gboolean              gimp_blend_tool_editor_handle_is_midpoint            (GimpBlendTool         *blend_tool,
                                                                                   gint                   handle);
static GimpGradientSegment * gimp_blend_tool_editor_handle_get_segment            (GimpBlendTool         *blend_tool,
                                                                                   gint                   handle);

static void                  gimp_blend_tool_editor_block_handlers                (GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_unblock_handlers              (GimpBlendTool         *blend_tool);
static gboolean              gimp_blend_tool_editor_are_handlers_blocked          (GimpBlendTool         *blend_tool);

static void                  gimp_blend_tool_editor_freeze_gradient               (GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_thaw_gradient                 (GimpBlendTool         *blend_tool);

static gint                  gimp_blend_tool_editor_add_stop                      (GimpBlendTool         *blend_tool,
                                                                                   gdouble                value);
static void                  gimp_blend_tool_editor_delete_stop                   (GimpBlendTool         *blend_tool,
                                                                                   gint                   slider);
static gint                  gimp_blend_tool_editor_midpoint_to_stop              (GimpBlendTool         *blend_tool,
                                                                                   gint                   slider);

static void                  gimp_blend_tool_editor_update_sliders                (GimpBlendTool         *blend_tool);

static void                  gimp_blend_tool_editor_purge_gradient_history        (GSList               **stack);
static void                  gimp_blend_tool_editor_purge_gradient                (GimpBlendTool         *blend_tool);

static GtkWidget           * gimp_blend_tool_editor_color_entry_new               (GimpBlendTool         *blend_tool,
                                                                                   const gchar           *title,
                                                                                   Direction              direction,
                                                                                   GtkWidget             *chain_button,
                                                                                   GtkWidget            **color_panel,
                                                                                   GtkWidget            **type_combo);
static void                  gimp_blend_tool_editor_init_endpoint_gui             (GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_init_stop_gui                 (GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_init_midpoint_gui             (GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_update_endpoint_gui           (GimpBlendTool         *blend_tool,
                                                                                   gint                   selection);
static void                  gimp_blend_tool_editor_update_stop_gui               (GimpBlendTool         *blend_tool,
                                                                                   gint                   selection);
static void                  gimp_blend_tool_editor_update_midpoint_gui           (GimpBlendTool         *blend_tool,
                                                                                   gint                   selection);
static void                  gimp_blend_tool_editor_update_gui                    (GimpBlendTool         *blend_tool);

static BlendInfo           * gimp_blend_tool_editor_blend_info_new                (GimpBlendTool         *blend_tool);
static void                  gimp_blend_tool_editor_blend_info_free               (BlendInfo             *info);
static void                  gimp_blend_tool_editor_blend_info_apply              (GimpBlendTool         *blend_tool,
                                                                                   const BlendInfo       *info,
                                                                                   gboolean               set_selection);


/*  private functions  */


static gboolean
gimp_blend_tool_editor_line_can_add_slider (GimpToolLine  *line,
                                            gdouble        value,
                                            GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  gdouble           offset  = options->offset / 100.0;

  return gimp_blend_tool_editor_is_gradient_editable (blend_tool) &&
         value >= offset;
}

static gint
gimp_blend_tool_editor_line_add_slider (GimpToolLine  *line,
                                        gdouble        value,
                                        GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options       = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_OPTIONS (options);
  gdouble           offset        = options->offset / 100.0;

  /* adjust slider value according to the offset */
  value = (value - offset) / (1.0 - offset);

  /* flip the slider value, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    value = 1.0 - value;

  return gimp_blend_tool_editor_add_stop (blend_tool, value);
}

static void
gimp_blend_tool_editor_line_prepare_to_remove_slider (GimpToolLine  *line,
                                                      gint           slider,
                                                      gboolean       remove,
                                                      GimpBlendTool *blend_tool)
{
  if (remove)
    {
      GimpGradient        *tentative_gradient;
      GimpGradientSegment *seg;
      gint                 i;

      tentative_gradient =
        GIMP_GRADIENT (gimp_data_duplicate (GIMP_DATA (blend_tool->gradient)));

      seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, slider);

      i = gimp_gradient_segment_range_get_n_segments (blend_tool->gradient,
                                                      blend_tool->gradient->segments,
                                                      seg) - 1;

      seg = gimp_gradient_segment_get_nth (tentative_gradient->segments, i);

      gimp_gradient_segment_range_merge (tentative_gradient,
                                         seg, seg->next, NULL, NULL);

      gimp_blend_tool_set_tentative_gradient (blend_tool, tentative_gradient);
    }
  else
    {
      gimp_blend_tool_set_tentative_gradient (blend_tool, NULL);
    }
}

static void
gimp_blend_tool_editor_line_remove_slider (GimpToolLine  *line,
                                           gint           slider,
                                           GimpBlendTool *blend_tool)
{
  gimp_blend_tool_editor_delete_stop (blend_tool, slider);
  gimp_blend_tool_set_tentative_gradient (blend_tool, NULL);
}

static void
gimp_blend_tool_editor_line_selection_changed (GimpToolLine  *line,
                                               GimpBlendTool *blend_tool)
{
  gint selection;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  if (blend_tool->gui)
    {
      /* hide all color dialogs */
      gimp_color_panel_dialog_response (
        GIMP_COLOR_PANEL (blend_tool->endpoint_color_panel),
        GIMP_COLOR_DIALOG_OK);
      gimp_color_panel_dialog_response (
        GIMP_COLOR_PANEL (blend_tool->stop_left_color_panel),
        GIMP_COLOR_DIALOG_OK);
      gimp_color_panel_dialog_response (
        GIMP_COLOR_PANEL (blend_tool->stop_right_color_panel),
        GIMP_COLOR_DIALOG_OK);

      /* reset the stop colors chain button */
      if (gimp_blend_tool_editor_handle_is_stop (blend_tool, selection))
        {
          const GimpGradientSegment *seg;
          gboolean                   homogeneous;

          seg = gimp_blend_tool_editor_handle_get_segment (blend_tool,
                                                           selection);

          homogeneous = seg->right_color.r    == seg->next->left_color.r &&
                        seg->right_color.g    == seg->next->left_color.g &&
                        seg->right_color.b    == seg->next->left_color.b &&
                        seg->right_color.a    == seg->next->left_color.a &&
                        seg->right_color_type == seg->next->left_color_type;

          gimp_chain_button_set_active (
            GIMP_CHAIN_BUTTON (blend_tool->stop_chain_button), homogeneous);
        }
    }

  gimp_blend_tool_editor_update_gui (blend_tool);
}

static gboolean
gimp_blend_tool_editor_line_handle_clicked (GimpToolLine        *line,
                                            gint                 handle,
                                            GdkModifierType      state,
                                            GimpButtonPressType  press_type,
                                            GimpBlendTool       *blend_tool)
{
  if (gimp_blend_tool_editor_handle_is_midpoint (blend_tool, handle))
    {
      if (press_type == GIMP_BUTTON_PRESS_DOUBLE &&
          gimp_blend_tool_editor_is_gradient_editable (blend_tool))
        {
          gint stop;

          stop = gimp_blend_tool_editor_midpoint_to_stop (blend_tool, handle);

          gimp_tool_line_set_selection (line, stop);

          /* return FALSE, so that the new slider can be dragged immediately */
          return FALSE;
        }
    }

  return FALSE;
}


static void
gimp_blend_tool_editor_gui_response (GimpToolGui   *gui,
                                     gint           response_id,
                                     GimpBlendTool *blend_tool)
{
  switch (response_id)
    {
    default:
      gimp_tool_line_set_selection (GIMP_TOOL_LINE (blend_tool->widget),
                                    GIMP_TOOL_LINE_HANDLE_NONE);
      break;
    }
}

static void
gimp_blend_tool_editor_color_entry_color_clicked (GimpColorButton *button,
                                                  GimpBlendTool   *blend_tool)
{
  gimp_blend_tool_editor_start_edit (blend_tool);
}

static void
gimp_blend_tool_editor_color_entry_color_changed (GimpColorButton *button,
                                                  GimpBlendTool   *blend_tool)
{
  GimpBlendOptions    *options       = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpPaintOptions    *paint_options = GIMP_PAINT_OPTIONS (options);
  gint                 selection;
  GimpRGB              color;
  Direction            direction;
  GtkWidget           *chain_button;
  GimpGradientSegment *seg;

  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  gimp_color_button_get_color (button, &color);

  direction =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                        "gimp-blend-tool-editor-direction"));
  chain_button = g_object_get_data (G_OBJECT (button),
                                    "gimp-blend-tool-editor-chain-button");

  gimp_blend_tool_editor_start_edit (blend_tool);
  gimp_blend_tool_editor_freeze_gradient (blend_tool);

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

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, selection);

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

  gimp_blend_tool_editor_thaw_gradient (blend_tool);
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
}

static void
gimp_blend_tool_editor_color_entry_color_response (GimpColorButton      *button,
                                                   GimpColorDialogState  state,
                                                   GimpBlendTool        *blend_tool)
{
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
}

static void
gimp_blend_tool_editor_color_entry_type_changed (GtkComboBox   *combo,
                                                 GimpBlendTool *blend_tool)
{
  GimpBlendOptions    *options       = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpPaintOptions    *paint_options = GIMP_PAINT_OPTIONS (options);
  gint                 selection;
  gint                 color_type;
  Direction            direction;
  GtkWidget           *chain_button;
  GimpGradientSegment *seg;

  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  if (! gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &color_type))
    return;

  direction =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo),
                                        "gimp-blend-tool-editor-direction"));
  chain_button = g_object_get_data (G_OBJECT (combo),
                                    "gimp-blend-tool-editor-chain-button");

  gimp_blend_tool_editor_start_edit (blend_tool);
  gimp_blend_tool_editor_freeze_gradient (blend_tool);

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

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, selection);

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

  gimp_blend_tool_editor_thaw_gradient (blend_tool);
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
}

static void
gimp_blend_tool_editor_endpoint_se_value_changed (GimpSizeEntry *se,
                                                  GimpBlendTool *blend_tool)
{
  gint    selection;
  gdouble x;
  gdouble y;

  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  x = gimp_size_entry_get_refval (se, 0);
  y = gimp_size_entry_get_refval (se, 1);

  gimp_blend_tool_editor_start_edit (blend_tool);
  gimp_blend_tool_editor_block_handlers (blend_tool);

  switch (selection)
    {
    case GIMP_TOOL_LINE_HANDLE_START:
      g_object_set (blend_tool->widget,
                    "x1", x,
                    "y1", y,
                    NULL);
      break;

    case GIMP_TOOL_LINE_HANDLE_END:
      g_object_set (blend_tool->widget,
                    "x2", x,
                    "y2", y,
                    NULL);
      break;

    default:
      g_assert_not_reached ();
    }

  gimp_blend_tool_editor_unblock_handlers (blend_tool);
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
}

static void
gimp_blend_tool_editor_stop_se_value_changed (GimpSizeEntry *se,
                                              GimpBlendTool *blend_tool)
{
  gint                 selection;
  gdouble              value;
  GimpGradientSegment *seg;

  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  if (selection == GIMP_TOOL_LINE_HANDLE_NONE)
    return;

  value = gimp_size_entry_get_refval (se, 0) / 100.0;

  gimp_blend_tool_editor_start_edit (blend_tool);
  gimp_blend_tool_editor_freeze_gradient (blend_tool);

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, selection);

  gimp_gradient_segment_range_compress (blend_tool->gradient,
                                        seg, seg,
                                        seg->left, value);
  gimp_gradient_segment_range_compress (blend_tool->gradient,
                                        seg->next, seg->next,
                                        value, seg->next->right);

  gimp_blend_tool_editor_thaw_gradient (blend_tool);
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
}

static void
gimp_blend_tool_editor_stop_delete_clicked (GtkWidget     *button,
                                            GimpBlendTool *blend_tool)
{
  gint selection;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  gimp_blend_tool_editor_delete_stop (blend_tool, selection);
}

static void
gimp_blend_tool_editor_midpoint_se_value_changed (GimpSizeEntry *se,
                                                  GimpBlendTool *blend_tool)
{
  gint                 selection;
  gdouble              value;
  GimpGradientSegment *seg;

  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  if (selection == GIMP_TOOL_LINE_HANDLE_NONE)
    return;

  value = gimp_size_entry_get_refval (se, 0) / 100.0;

  gimp_blend_tool_editor_start_edit (blend_tool);
  gimp_blend_tool_editor_freeze_gradient (blend_tool);

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, selection);

  seg->middle = value;

  gimp_blend_tool_editor_thaw_gradient (blend_tool);
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
}

static void
gimp_blend_tool_editor_midpoint_type_changed (GtkComboBox   *combo,
                                              GimpBlendTool *blend_tool)
{
  gint                 selection;
  gint                 type;
  GimpGradientSegment *seg;

  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  if (! gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &type))
    return;

  gimp_blend_tool_editor_start_edit (blend_tool);
  gimp_blend_tool_editor_freeze_gradient (blend_tool);

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, selection);

  seg->type = type;

  gimp_blend_tool_editor_thaw_gradient (blend_tool);
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
}

static void
gimp_blend_tool_editor_midpoint_color_changed (GtkComboBox   *combo,
                                               GimpBlendTool *blend_tool)
{
  gint                 selection;
  gint                 color;
  GimpGradientSegment *seg;

  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  if (! gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &color))
    return;

  gimp_blend_tool_editor_start_edit (blend_tool);
  gimp_blend_tool_editor_freeze_gradient (blend_tool);

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, selection);

  seg->color = color;

  gimp_blend_tool_editor_thaw_gradient (blend_tool);
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
}

static void
gimp_blend_tool_editor_midpoint_new_stop_clicked (GtkWidget     *button,
                                                  GimpBlendTool *blend_tool)
{
  gint selection;
  gint stop;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  stop = gimp_blend_tool_editor_midpoint_to_stop (blend_tool, selection);

  gimp_tool_line_set_selection (GIMP_TOOL_LINE (blend_tool->widget), stop);
}

static void
gimp_blend_tool_editor_midpoint_center_clicked (GtkWidget     *button,
                                                GimpBlendTool *blend_tool)
{
  gint                 selection;
  GimpGradientSegment *seg;

  selection =
    gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

  gimp_blend_tool_editor_start_edit (blend_tool);
  gimp_blend_tool_editor_freeze_gradient (blend_tool);

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, selection);

  gimp_gradient_segment_range_recenter_handles (blend_tool->gradient, seg, seg);

  gimp_blend_tool_editor_thaw_gradient (blend_tool);
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
}

static gboolean
gimp_blend_tool_editor_flush_idle (GimpBlendTool *blend_tool)
{
  GimpDisplay *display = GIMP_TOOL (blend_tool)->display;

  gimp_image_flush (gimp_display_get_image (display));

  blend_tool->flush_idle_id = 0;

  return G_SOURCE_REMOVE;
}

static gboolean
gimp_blend_tool_editor_is_gradient_editable (GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);

  return ! options->modify_active ||
         gimp_data_is_writable (GIMP_DATA (blend_tool->gradient));
}

static gboolean
gimp_blend_tool_editor_handle_is_endpoint (GimpBlendTool *blend_tool,
                                           gint           handle)
{
  return handle == GIMP_TOOL_LINE_HANDLE_START ||
         handle == GIMP_TOOL_LINE_HANDLE_END;
}

static gboolean
gimp_blend_tool_editor_handle_is_stop (GimpBlendTool *blend_tool,
                                       gint           handle)
{
  gint n_sliders;

  gimp_tool_line_get_sliders (GIMP_TOOL_LINE (blend_tool->widget), &n_sliders);

  return handle >= 0 && handle < n_sliders / 2;
}

static gboolean
gimp_blend_tool_editor_handle_is_midpoint (GimpBlendTool *blend_tool,
                                           gint           handle)
{
  gint n_sliders;

  gimp_tool_line_get_sliders (GIMP_TOOL_LINE (blend_tool->widget), &n_sliders);

  return handle >= n_sliders / 2;
}

static GimpGradientSegment *
gimp_blend_tool_editor_handle_get_segment (GimpBlendTool *blend_tool,
                                           gint           handle)
{
  switch (handle)
    {
    case GIMP_TOOL_LINE_HANDLE_START:
      return blend_tool->gradient->segments;

    case GIMP_TOOL_LINE_HANDLE_END:
      return gimp_gradient_segment_get_last (blend_tool->gradient->segments);

    default:
      {
        const GimpControllerSlider *sliders;
        gint                        n_sliders;
        gint                        seg_i;

        sliders = gimp_tool_line_get_sliders (GIMP_TOOL_LINE (blend_tool->widget),
                                              &n_sliders);

        g_assert (handle >= 0 && handle < n_sliders);

        seg_i = GPOINTER_TO_INT (sliders[handle].data);

        return gimp_gradient_segment_get_nth (blend_tool->gradient->segments,
                                              seg_i);
      }
    }
}

static void
gimp_blend_tool_editor_block_handlers (GimpBlendTool *blend_tool)
{
  blend_tool->block_handlers_count++;
}

static void
gimp_blend_tool_editor_unblock_handlers (GimpBlendTool *blend_tool)
{
  g_assert (blend_tool->block_handlers_count > 0);

  blend_tool->block_handlers_count--;
}

static gboolean
gimp_blend_tool_editor_are_handlers_blocked (GimpBlendTool *blend_tool)
{
  return blend_tool->block_handlers_count > 0;
}

static void
gimp_blend_tool_editor_freeze_gradient (GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpGradient     *custom;
  BlendInfo        *info;

  gimp_blend_tool_editor_block_handlers (blend_tool);

  custom = gimp_gradients_get_custom (GIMP_CONTEXT (options)->gimp);

  if (blend_tool->gradient == custom || options->modify_active)
    {
      g_assert (gimp_blend_tool_editor_is_gradient_editable (blend_tool));

      gimp_data_freeze (GIMP_DATA (blend_tool->gradient));
    }
  else
    {
      /* copy the active gradient to the custom gradient, and make the custom
       * gradient active.
       */
      gimp_data_freeze (GIMP_DATA (custom));

      gimp_data_copy (GIMP_DATA (custom), GIMP_DATA (blend_tool->gradient));

      gimp_context_set_gradient (GIMP_CONTEXT (options), custom);

      g_assert (blend_tool->gradient == custom);
      g_assert (gimp_blend_tool_editor_is_gradient_editable (blend_tool));
    }

  if (blend_tool->edit_count > 0)
    {
      info = blend_tool->undo_stack->data;

      if (! info->gradient)
        {
          info->gradient =
            GIMP_GRADIENT (gimp_data_duplicate (GIMP_DATA (blend_tool->gradient)));
        }
    }
}

static void
gimp_blend_tool_editor_thaw_gradient(GimpBlendTool *blend_tool)
{
  gimp_data_thaw (GIMP_DATA (blend_tool->gradient));

  gimp_blend_tool_editor_update_sliders (blend_tool);
  gimp_blend_tool_editor_update_gui (blend_tool);

  gimp_blend_tool_editor_unblock_handlers (blend_tool);
}

static gint
gimp_blend_tool_editor_add_stop (GimpBlendTool *blend_tool,
                                 gdouble        value)
{
  GimpBlendOptions    *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpGradientSegment *seg;
  gint                 stop;
  BlendInfo           *info;

  gimp_blend_tool_editor_start_edit (blend_tool);
  gimp_blend_tool_editor_freeze_gradient (blend_tool);

  gimp_gradient_split_at (blend_tool->gradient,
                          GIMP_CONTEXT (options), NULL, value, &seg, NULL);

  stop =
    gimp_gradient_segment_range_get_n_segments (blend_tool->gradient,
                                                blend_tool->gradient->segments,
                                                seg) - 1;

  info               = blend_tool->undo_stack->data;
  info->added_handle = stop;

  gimp_blend_tool_editor_thaw_gradient (blend_tool);
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);

  return stop;
}

static void
gimp_blend_tool_editor_delete_stop (GimpBlendTool *blend_tool,
                                    gint           slider)
{
  GimpGradientSegment *seg;
  BlendInfo           *info;

  gimp_blend_tool_editor_start_edit (blend_tool);
  gimp_blend_tool_editor_freeze_gradient (blend_tool);

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, slider);

  gimp_gradient_segment_range_merge (blend_tool->gradient,
                                     seg, seg->next, NULL, NULL);

  info = blend_tool->undo_stack->data;

  if (info->added_handle == slider)
    info->added_handle = GIMP_TOOL_LINE_HANDLE_NONE;
  else
    info->removed_handle = slider;

  gimp_blend_tool_editor_thaw_gradient (blend_tool);
  gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
}

static gint
gimp_blend_tool_editor_midpoint_to_stop (GimpBlendTool *blend_tool,
                                         gint           slider)
{
  const GimpControllerSlider *sliders;

  sliders = gimp_tool_line_get_sliders (GIMP_TOOL_LINE (blend_tool->widget),
                                        NULL);

  if (sliders[slider].value > sliders[slider].min + EPSILON &&
      sliders[slider].value < sliders[slider].max - EPSILON)
    {
      gint       stop;
      BlendInfo *info;

      stop = gimp_blend_tool_editor_add_stop (blend_tool,
                                              sliders[slider].value);

      info                 = blend_tool->undo_stack->data;
      info->removed_handle = slider;

      slider = stop;
    }

  return slider;
}

static void
gimp_blend_tool_editor_update_sliders (GimpBlendTool *blend_tool)
{
  GimpBlendOptions     *options       = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpPaintOptions     *paint_options = GIMP_PAINT_OPTIONS (options);
  gdouble               offset        = options->offset / 100.0;
  gboolean              editable;
  GimpControllerSlider *sliders;
  gint                  n_sliders;
  gint                  n_segments;
  GimpGradientSegment  *seg;
  GimpControllerSlider *slider;
  gint                  i;

  if (! blend_tool->widget || options->instant)
    return;

  editable = gimp_blend_tool_editor_is_gradient_editable (blend_tool);

  n_segments = gimp_gradient_segment_range_get_n_segments (
    blend_tool->gradient, blend_tool->gradient->segments, NULL);

  n_sliders = (n_segments - 1) + /* gradient stops, between each adjacent
                                  * pair of segments */
              (n_segments);      /* midpoints, inside each segment */

  sliders = g_new (GimpControllerSlider, n_sliders);

  slider = sliders;

  /* initialize the gradient-stop sliders */
  for (seg = blend_tool->gradient->segments, i = 0;
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
  for (seg = blend_tool->gradient->segments, i = 0;
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

  /* avoid updating the gradient in gimp_blend_tool_editor_line_changed() */
  gimp_blend_tool_editor_block_handlers (blend_tool);

  gimp_tool_line_set_sliders (GIMP_TOOL_LINE (blend_tool->widget),
                              sliders, n_sliders);

  gimp_blend_tool_editor_unblock_handlers (blend_tool);

  g_free (sliders);
}

static void
gimp_blend_tool_editor_purge_gradient_history (GSList **stack)
{
  GSList *link;

  /* eliminate all history steps that modify the gradient */
  while ((link = *stack))
    {
      BlendInfo *info = link->data;

      if (info->gradient)
        {
          gimp_blend_tool_editor_blend_info_free (info);

          *stack = g_slist_delete_link (*stack, link);
        }
      else
        {
          stack = &link->next;
        }
    }
}

static void
gimp_blend_tool_editor_purge_gradient (GimpBlendTool *blend_tool)
{
  if (blend_tool->widget)
    {
      gimp_blend_tool_editor_update_sliders (blend_tool);

      gimp_tool_line_set_selection (GIMP_TOOL_LINE (blend_tool->widget),
                                    GIMP_TOOL_LINE_HANDLE_NONE);
    }

  gimp_blend_tool_editor_purge_gradient_history (&blend_tool->undo_stack);
  gimp_blend_tool_editor_purge_gradient_history (&blend_tool->redo_stack);
}

static GtkWidget *
gimp_blend_tool_editor_color_entry_new (GimpBlendTool  *blend_tool,
                                        const gchar    *title,
                                        Direction       direction,
                                        GtkWidget      *chain_button,
                                        GtkWidget     **color_panel,
                                        GtkWidget     **type_combo)
{
  GimpContext *context = GIMP_CONTEXT (GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool));
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
                     "gimp-blend-tool-editor-direction",
                     GINT_TO_POINTER (direction));
  g_object_set_data (G_OBJECT (button),
                     "gimp-blend-tool-editor-chain-button",
                     chain_button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_blend_tool_editor_color_entry_color_clicked),
                    blend_tool);
  g_signal_connect (button, "color-changed",
                    G_CALLBACK (gimp_blend_tool_editor_color_entry_color_changed),
                    blend_tool);
  g_signal_connect (button, "response",
                    G_CALLBACK (gimp_blend_tool_editor_color_entry_color_response),
                    blend_tool);

  /* the color type combo */
  *type_combo = combo = gimp_enum_combo_box_new (GIMP_TYPE_GRADIENT_COLOR);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
  gtk_widget_show (combo);

  g_object_set_data (G_OBJECT (combo),
                     "gimp-blend-tool-editor-direction",
                     GINT_TO_POINTER (direction));
  g_object_set_data (G_OBJECT (combo),
                     "gimp-blend-tool-editor-chain-button",
                     chain_button);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_blend_tool_editor_color_entry_type_changed),
                    blend_tool);

  return hbox;
}

static void
gimp_blend_tool_editor_init_endpoint_gui (GimpBlendTool *blend_tool)
{
  GimpDisplay      *display = GIMP_TOOL (blend_tool)->display;
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
  blend_tool->endpoint_editor =
  editor                      = gimp_editor_new ();
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (blend_tool->gui)),
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
  spinbutton = gtk_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);

  blend_tool->endpoint_se =
  se                      = gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
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
                    G_CALLBACK (gimp_blend_tool_editor_endpoint_se_value_changed),
                    blend_tool);

  row += 2;

  /* the color label */
  label = gtk_label_new (_("Color:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /* the color entry */
  hbox = gimp_blend_tool_editor_color_entry_new (
    blend_tool, _("Change Endpoint Color"), DIRECTION_NONE, NULL,
    &blend_tool->endpoint_color_panel, &blend_tool->endpoint_type_combo);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (hbox);

  row++;
}

static void
gimp_blend_tool_editor_init_stop_gui (GimpBlendTool *blend_tool)
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
  blend_tool->stop_editor =
  editor                  = gimp_editor_new ();
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (blend_tool->gui)),
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
  blend_tool->stop_se =
  se                  = gimp_size_entry_new (1, GIMP_UNIT_PERCENT, "%a",
                                             FALSE, TRUE, FALSE, 6,
                                             GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (se), FALSE);
  gtk_table_attach (GTK_TABLE (table), se, 1, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (se);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (gimp_blend_tool_editor_stop_se_value_changed),
                    blend_tool);

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
  blend_tool->stop_chain_button =
  button                        = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gtk_table_attach (GTK_TABLE (table2), button, 1, 2, 0, 2,
                    GTK_SHRINK | GTK_FILL,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    0, 0);
  gtk_widget_show (button);

  /* the color entries */
  hbox = gimp_blend_tool_editor_color_entry_new (
    blend_tool, _("Change Stop Color"), DIRECTION_LEFT, button,
    &blend_tool->stop_left_color_panel, &blend_tool->stop_left_type_combo);
  gtk_table_attach (GTK_TABLE (table2), hbox, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (hbox);

  hbox = gimp_blend_tool_editor_color_entry_new (
    blend_tool, _("Change Stop Color"), DIRECTION_RIGHT, button,
    &blend_tool->stop_right_color_panel, &blend_tool->stop_right_type_combo);
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
                          G_CALLBACK (gimp_blend_tool_editor_stop_delete_clicked),
                          NULL, blend_tool);
}

static void
gimp_blend_tool_editor_init_midpoint_gui (GimpBlendTool *blend_tool)
{
  GtkWidget *editor;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *se;
  GtkWidget *combo;
  GtkWidget *separator;
  gint       row = 0;

  /* the stop editor */
  blend_tool->midpoint_editor =
  editor                      = gimp_editor_new ();
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (blend_tool->gui)),
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
  blend_tool->midpoint_se =
  se                      = gimp_size_entry_new (1, GIMP_UNIT_PERCENT, "%a",
                                                 FALSE, TRUE, FALSE, 6,
                                                 GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (se), FALSE);
  gtk_table_attach (GTK_TABLE (table), se, 1, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (se);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (gimp_blend_tool_editor_midpoint_se_value_changed),
                    blend_tool);

  row++;

  /* the type label */
  label = gtk_label_new (_("Blending:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /* the type combo */
  blend_tool->midpoint_type_combo =
  combo                           = gimp_enum_combo_box_new (GIMP_TYPE_GRADIENT_SEGMENT_TYPE);
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_blend_tool_editor_midpoint_type_changed),
                    blend_tool);

  row++;

  /* the color label */
  label = gtk_label_new (_("Coloring:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /* the color combo */
  blend_tool->midpoint_color_combo =
  combo                           = gimp_enum_combo_box_new (GIMP_TYPE_GRADIENT_SEGMENT_COLOR);
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL | GTK_EXPAND,
                    GTK_SHRINK | GTK_FILL,
                    0, 0);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_blend_tool_editor_midpoint_color_changed),
                    blend_tool);

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
  blend_tool->midpoint_new_stop_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GIMP_ICON_DOCUMENT_NEW, _("New stop at midpoint"),
                            NULL,
                            G_CALLBACK (gimp_blend_tool_editor_midpoint_new_stop_clicked),
                            NULL, blend_tool);

  /* the center button */
  blend_tool->midpoint_center_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GIMP_ICON_CENTER_HORIZONTAL, _("Center midpoint"),
                            NULL,
                            G_CALLBACK (gimp_blend_tool_editor_midpoint_center_clicked),
                            NULL, blend_tool);
}

static void
gimp_blend_tool_editor_update_endpoint_gui (GimpBlendTool *blend_tool,
                                            gint           selection)
{
  GimpBlendOptions    *options       = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpPaintOptions    *paint_options = GIMP_PAINT_OPTIONS (options);
  GimpContext         *context       = GIMP_CONTEXT (options);
  gboolean             editable;
  GimpGradientSegment *seg;
  const gchar         *title;
  gdouble              x;
  gdouble              y;
  GimpRGB              color;
  GimpGradientColor    color_type;

  editable = gimp_blend_tool_editor_is_gradient_editable (blend_tool);

  switch (selection)
    {
    case GIMP_TOOL_LINE_HANDLE_START:
      g_object_get (blend_tool->widget,
                    "x1", &x,
                    "y1", &y,
                    NULL);
      break;

    case GIMP_TOOL_LINE_HANDLE_END:
      g_object_get (blend_tool->widget,
                    "x2", &x,
                    "y2", &y,
                    NULL);
      break;

    default:
      g_assert_not_reached ();
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

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, selection);

  switch (selection)
    {
    case GIMP_TOOL_LINE_HANDLE_START:
      title = _("Start Endpoint");

      gimp_gradient_segment_get_left_flat_color (blend_tool->gradient, context,
                                                 seg, &color);
      color_type = seg->left_color_type;
      break;

    case GIMP_TOOL_LINE_HANDLE_END:
      title = _("End Endpoint");

      gimp_gradient_segment_get_right_flat_color (blend_tool->gradient, context,
                                                  seg, &color);
      color_type = seg->right_color_type;
      break;

    default:
      g_assert_not_reached ();
    }

  gimp_tool_gui_set_title (blend_tool->gui, title);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (blend_tool->endpoint_se), 0, x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (blend_tool->endpoint_se), 1, y);

  gimp_color_button_set_color (
    GIMP_COLOR_BUTTON (blend_tool->endpoint_color_panel), &color);
  gimp_int_combo_box_set_active (
    GIMP_INT_COMBO_BOX (blend_tool->endpoint_type_combo), color_type);

  gtk_widget_set_sensitive (blend_tool->endpoint_color_panel, editable);
  gtk_widget_set_sensitive (blend_tool->endpoint_type_combo,  editable);

  gtk_widget_show (blend_tool->endpoint_editor);
}

static void
gimp_blend_tool_editor_update_stop_gui (GimpBlendTool *blend_tool,
                                        gint           selection)
{
  GimpBlendOptions    *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
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

  editable = gimp_blend_tool_editor_is_gradient_editable (blend_tool);

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, selection);

  index = GPOINTER_TO_INT (
    gimp_tool_line_get_sliders (GIMP_TOOL_LINE (blend_tool->widget),
                                NULL)[selection].data);

  title = g_strdup_printf (_("Stop %d"), index + 1);

  min   = seg->left;
  max   = seg->next->right;
  value = seg->right;

  gimp_gradient_segment_get_right_flat_color (blend_tool->gradient, context,
                                              seg, &left_color);
  left_color_type = seg->right_color_type;

  gimp_gradient_segment_get_left_flat_color (blend_tool->gradient, context,
                                             seg->next, &right_color);
  right_color_type = seg->next->left_color_type;

  gimp_tool_gui_set_title (blend_tool->gui, title);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (blend_tool->stop_se),
                                         0, 100.0 * min, 100.0 * max);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (blend_tool->stop_se),
                              0, 100.0 * value);

  gimp_color_button_set_color (
    GIMP_COLOR_BUTTON (blend_tool->stop_left_color_panel), &left_color);
  gimp_int_combo_box_set_active (
    GIMP_INT_COMBO_BOX (blend_tool->stop_left_type_combo), left_color_type);

  gimp_color_button_set_color (
    GIMP_COLOR_BUTTON (blend_tool->stop_right_color_panel), &right_color);
  gimp_int_combo_box_set_active (
    GIMP_INT_COMBO_BOX (blend_tool->stop_right_type_combo), right_color_type);

  gtk_widget_set_sensitive (blend_tool->stop_se,                editable);
  gtk_widget_set_sensitive (blend_tool->stop_left_color_panel,  editable);
  gtk_widget_set_sensitive (blend_tool->stop_left_type_combo,   editable);
  gtk_widget_set_sensitive (blend_tool->stop_right_color_panel, editable);
  gtk_widget_set_sensitive (blend_tool->stop_right_type_combo,  editable);
  gtk_widget_set_sensitive (blend_tool->stop_chain_button,      editable);
  gtk_widget_set_sensitive (
    GTK_WIDGET (gimp_editor_get_button_box (GIMP_EDITOR (blend_tool->stop_editor))),
    editable);

  g_free (title);

  gtk_widget_show (blend_tool->stop_editor);
}

static void
gimp_blend_tool_editor_update_midpoint_gui (GimpBlendTool *blend_tool,
                                            gint           selection)
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

  editable = gimp_blend_tool_editor_is_gradient_editable (blend_tool);

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, selection);

  index = GPOINTER_TO_INT (
    gimp_tool_line_get_sliders (GIMP_TOOL_LINE (blend_tool->widget),
                                NULL)[selection].data);

  title = g_strdup_printf (_("Midpoint %d"), index + 1);

  min   = seg->left;
  max   = seg->right;
  value = seg->middle;
  type  = seg->type;
  color = seg->color;

  gimp_tool_gui_set_title (blend_tool->gui, title);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (blend_tool->midpoint_se),
                                         0, 100.0 * min, 100.0 * max);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (blend_tool->midpoint_se),
                              0, 100.0 * value);

  gimp_int_combo_box_set_active (
    GIMP_INT_COMBO_BOX (blend_tool->midpoint_type_combo), type);

  gimp_int_combo_box_set_active (
    GIMP_INT_COMBO_BOX (blend_tool->midpoint_color_combo), color);

  gtk_widget_set_sensitive (blend_tool->midpoint_new_stop_button,
                            value > min + EPSILON && value < max - EPSILON);
  gtk_widget_set_sensitive (blend_tool->midpoint_center_button,
                            fabs (value - (min + max) / 2.0) > EPSILON);

  gtk_widget_set_sensitive (blend_tool->midpoint_se,          editable);
  gtk_widget_set_sensitive (blend_tool->midpoint_type_combo,  editable);
  gtk_widget_set_sensitive (blend_tool->midpoint_color_combo, editable);
  gtk_widget_set_sensitive (
    GTK_WIDGET (gimp_editor_get_button_box (GIMP_EDITOR (blend_tool->midpoint_editor))),
    editable);

  g_free (title);

  gtk_widget_show (blend_tool->midpoint_editor);
}

static void
gimp_blend_tool_editor_update_gui (GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);

  if (blend_tool->gradient && blend_tool->widget && ! options->instant)
    {
      gint selection;

      selection =
        gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

      if (selection != GIMP_TOOL_LINE_HANDLE_NONE)
        {
          if (! blend_tool->gui)
            {
              GimpDisplayShell *shell;

              shell = gimp_tool_widget_get_shell (blend_tool->widget);

              blend_tool->gui =
                gimp_tool_gui_new (GIMP_TOOL (blend_tool)->tool_info,
                                   NULL, NULL, NULL, NULL,
                                   gtk_widget_get_screen (GTK_WIDGET (shell)),
                                   gimp_widget_get_monitor (GTK_WIDGET (shell)),
                                   TRUE,

                                   _("_Close"), GTK_RESPONSE_CLOSE,

                                   NULL);

              gimp_tool_gui_set_shell (blend_tool->gui, shell);
              gimp_tool_gui_set_viewable (blend_tool->gui,
                                          GIMP_VIEWABLE (blend_tool->gradient));
              gimp_tool_gui_set_auto_overlay (blend_tool->gui, TRUE);

              g_signal_connect (blend_tool->gui, "response",
                                G_CALLBACK (gimp_blend_tool_editor_gui_response),
                                blend_tool);

              gimp_blend_tool_editor_init_endpoint_gui (blend_tool);
              gimp_blend_tool_editor_init_stop_gui     (blend_tool);
              gimp_blend_tool_editor_init_midpoint_gui (blend_tool);
            }

          gimp_blend_tool_editor_block_handlers (blend_tool);

          if (gimp_blend_tool_editor_handle_is_endpoint (blend_tool, selection))
            gimp_blend_tool_editor_update_endpoint_gui (blend_tool, selection);
          else
            gtk_widget_hide (blend_tool->endpoint_editor);

          if (gimp_blend_tool_editor_handle_is_stop (blend_tool, selection))
            gimp_blend_tool_editor_update_stop_gui (blend_tool, selection);
          else
            gtk_widget_hide (blend_tool->stop_editor);

          if (gimp_blend_tool_editor_handle_is_midpoint (blend_tool, selection))
            gimp_blend_tool_editor_update_midpoint_gui (blend_tool, selection);
          else
            gtk_widget_hide (blend_tool->midpoint_editor);

          gimp_blend_tool_editor_unblock_handlers (blend_tool);

          gimp_tool_gui_show (blend_tool->gui);

          return;
        }
    }

  if (blend_tool->gui)
    gimp_tool_gui_hide (blend_tool->gui);
}

static BlendInfo *
gimp_blend_tool_editor_blend_info_new (GimpBlendTool *blend_tool)
{
  BlendInfo *info = g_slice_new (BlendInfo);

  info->start_x         = blend_tool->start_x;
  info->start_y         = blend_tool->start_y;
  info->end_x           = blend_tool->end_x;
  info->end_y           = blend_tool->end_y;

  info->gradient        = NULL;

  info->added_handle    = GIMP_TOOL_LINE_HANDLE_NONE;
  info->removed_handle  = GIMP_TOOL_LINE_HANDLE_NONE;
  info->selected_handle = GIMP_TOOL_LINE_HANDLE_NONE;

  return info;
}

static void
gimp_blend_tool_editor_blend_info_free (BlendInfo *info)
{
  if (info->gradient)
    g_object_unref (info->gradient);

  g_slice_free (BlendInfo, info);
}

static void
gimp_blend_tool_editor_blend_info_apply (GimpBlendTool   *blend_tool,
                                         const BlendInfo *info,
                                         gboolean         set_selection)
{
  gint selection;

  g_assert (blend_tool->widget   != NULL);
  g_assert (blend_tool->gradient != NULL);

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
          g_assert (gimp_blend_tool_editor_handle_is_stop (blend_tool,
                                                           info->added_handle));

          selection =
            gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

          /* if the selected handle is a stop... */
          if (gimp_blend_tool_editor_handle_is_stop (blend_tool, selection))
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
          else if (gimp_blend_tool_editor_handle_is_midpoint (blend_tool, selection))
            {
              const GimpControllerSlider *sliders;
              gint                        seg_i;

              sliders =
                gimp_tool_line_get_sliders (GIMP_TOOL_LINE (blend_tool->widget),
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
          /* we're undoing an operation in which the same handle was added and
           * then removed; don't change the selection
           */
          set_selection = FALSE;
        }
    }
  else if ((info->start_x != blend_tool->start_x  ||
            info->start_y != blend_tool->start_y) &&
           (info->end_x   == blend_tool->end_x    &&
            info->end_y   == blend_tool->end_y))
    {
      /* we're undoing a start-endpoint move operation; select the start
       * endpoint
       */
      selection = GIMP_TOOL_LINE_HANDLE_START;
    }
  else if ((info->end_x   != blend_tool->end_x    ||
            info->end_y   != blend_tool->end_y)   &&
           (info->start_x == blend_tool->start_x  &&
            info->start_y == blend_tool->start_y))

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

  gimp_blend_tool_editor_block_handlers (blend_tool);

  g_object_set (blend_tool->widget,
                "x1", info->start_x,
                "y1", info->start_y,
                "x2", info->end_x,
                "y2", info->end_y,
                NULL);

  if (info->gradient)
    {
      gimp_blend_tool_editor_freeze_gradient (blend_tool);

      gimp_data_copy (GIMP_DATA (blend_tool->gradient),
                      GIMP_DATA (info->gradient));

      gimp_blend_tool_editor_thaw_gradient (blend_tool);
    }

  if (set_selection)
    {
      gimp_tool_line_set_selection (GIMP_TOOL_LINE (blend_tool->widget),
                                    selection);
    }

  gimp_blend_tool_editor_update_gui (blend_tool);

  gimp_blend_tool_editor_unblock_handlers (blend_tool);
}


/*  public functions  */


void
gimp_blend_tool_editor_options_notify (GimpBlendTool    *blend_tool,
                                       GimpToolOptions  *options,
                                       const GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "modify-active"))
    {
      gimp_blend_tool_editor_update_sliders (blend_tool);
      gimp_blend_tool_editor_update_gui (blend_tool);
    }
  else if (! strcmp (pspec->name, "gradient-reverse"))
    {
      gimp_blend_tool_editor_update_sliders (blend_tool);

      /* if an endpoint is selected, swap the selected endpoint */
      if (blend_tool->widget)
        {
          gint selection;

          selection =
            gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

          switch (selection)
            {
            case GIMP_TOOL_LINE_HANDLE_START:
              gimp_tool_line_set_selection (GIMP_TOOL_LINE (blend_tool->widget),
                                            GIMP_TOOL_LINE_HANDLE_END);
              break;

            case GIMP_TOOL_LINE_HANDLE_END:
              gimp_tool_line_set_selection (GIMP_TOOL_LINE (blend_tool->widget),
                                            GIMP_TOOL_LINE_HANDLE_START);
              break;
            }
        }
    }
  else if (blend_tool->render_node &&
           gegl_node_find_property (blend_tool->render_node, pspec->name))
    {
      gimp_blend_tool_editor_update_sliders (blend_tool);
    }
}

void
gimp_blend_tool_editor_start (GimpBlendTool *blend_tool)
{
  g_signal_connect (blend_tool->widget, "can-add-slider",
                    G_CALLBACK (gimp_blend_tool_editor_line_can_add_slider),
                    blend_tool);
  g_signal_connect (blend_tool->widget, "add-slider",
                    G_CALLBACK (gimp_blend_tool_editor_line_add_slider),
                    blend_tool);
  g_signal_connect (blend_tool->widget, "prepare-to-remove-slider",
                    G_CALLBACK (gimp_blend_tool_editor_line_prepare_to_remove_slider),
                    blend_tool);
  g_signal_connect (blend_tool->widget, "remove-slider",
                    G_CALLBACK (gimp_blend_tool_editor_line_remove_slider),
                    blend_tool);
  g_signal_connect (blend_tool->widget, "selection-changed",
                    G_CALLBACK (gimp_blend_tool_editor_line_selection_changed),
                    blend_tool);
  g_signal_connect (blend_tool->widget, "handle-clicked",
                    G_CALLBACK (gimp_blend_tool_editor_line_handle_clicked),
                    blend_tool);
}

void
gimp_blend_tool_editor_halt (GimpBlendTool *blend_tool)
{
  g_clear_object (&blend_tool->gui);

  blend_tool->edit_count = 0;

  if (blend_tool->undo_stack)
    {
      g_slist_free_full (blend_tool->undo_stack,
                         (GDestroyNotify) gimp_blend_tool_editor_blend_info_free);
      blend_tool->undo_stack = NULL;
    }

  if (blend_tool->redo_stack)
    {
      g_slist_free_full (blend_tool->redo_stack,
                         (GDestroyNotify) gimp_blend_tool_editor_blend_info_free);
      blend_tool->redo_stack = NULL;
    }

  if (blend_tool->flush_idle_id)
    {
      g_source_remove (blend_tool->flush_idle_id);
      blend_tool->flush_idle_id = 0;
    }
}

void
gimp_blend_tool_editor_line_changed (GimpBlendTool *blend_tool)
{
  GimpBlendOptions           *options       = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpPaintOptions           *paint_options = GIMP_PAINT_OPTIONS (options);
  gdouble                     offset        = options->offset / 100.0;
  const GimpControllerSlider *sliders;
  gint                        n_sliders;
  gint                        i;
  GimpGradientSegment        *seg;
  gboolean                    changed       = FALSE;

  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  if (! blend_tool->gradient || offset == 1.0)
    return;

  sliders = gimp_tool_line_get_sliders (GIMP_TOOL_LINE (blend_tool->widget),
                                        &n_sliders);

  if (n_sliders == 0)
    return;

  /* update the midpoints first, since moving the gradient stops may change the
   * gradient's midpoints w.r.t. the sliders, but not the other way around.
   */
  for (seg = blend_tool->gradient->segments, i = n_sliders / 2;
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
              gimp_blend_tool_editor_start_edit (blend_tool);
              gimp_blend_tool_editor_freeze_gradient (blend_tool);

              /* refetch the segment, since the gradient might have changed */
              seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, i);

              changed = TRUE;
            }

          seg->middle = value;
        }
    }

  /* update the gradient stops */
  for (seg = blend_tool->gradient->segments, i = 0;
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
              gimp_blend_tool_editor_start_edit (blend_tool);
              gimp_blend_tool_editor_freeze_gradient (blend_tool);

              /* refetch the segment, since the gradient might have changed */
              seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, i);

              changed = TRUE;
            }

          gimp_gradient_segment_range_compress (blend_tool->gradient,
                                                seg, seg,
                                                seg->left, value);
          gimp_gradient_segment_range_compress (blend_tool->gradient,
                                                seg->next, seg->next,
                                                value, seg->next->right);
        }
    }

  if (changed)
    {
      gimp_blend_tool_editor_thaw_gradient (blend_tool);
      gimp_blend_tool_editor_end_edit (blend_tool, FALSE);
    }

  gimp_blend_tool_editor_update_gui (blend_tool);
}

void
gimp_blend_tool_editor_gradient_dirty (GimpBlendTool *blend_tool)
{
  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  gimp_blend_tool_editor_purge_gradient (blend_tool);
}

void
gimp_blend_tool_editor_gradient_changed (GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpContext      *context = GIMP_CONTEXT (options);

  if (options->modify_active_frame)
    {
      gtk_widget_set_sensitive (options->modify_active_frame,
                                blend_tool->gradient !=
                                gimp_gradients_get_custom (context->gimp));
    }

  if (options->modify_active_hint)
    {
      gtk_widget_set_visible (options->modify_active_hint,
                              blend_tool->gradient &&
                              ! gimp_data_is_writable (GIMP_DATA (blend_tool->gradient)));
    }

  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  gimp_blend_tool_editor_purge_gradient (blend_tool);
}

const gchar *
gimp_blend_tool_editor_can_undo (GimpBlendTool *blend_tool)
{
  if (! blend_tool->undo_stack || blend_tool->edit_count > 0)
    return NULL;

  return _("Blend Step");
}

const gchar *
gimp_blend_tool_editor_can_redo (GimpBlendTool *blend_tool)
{
  if (! blend_tool->redo_stack || blend_tool->edit_count > 0)
    return NULL;

  return _("Blend Step");
}

gboolean
gimp_blend_tool_editor_undo (GimpBlendTool *blend_tool)
{
  GimpTool  *tool = GIMP_TOOL (blend_tool);
  BlendInfo *info;
  BlendInfo *new_info;

  g_assert (blend_tool->undo_stack != NULL);
  g_assert (blend_tool->edit_count == 0);

  info = blend_tool->undo_stack->data;

  new_info = gimp_blend_tool_editor_blend_info_new (blend_tool);

  if (info->gradient)
    {
      new_info->gradient =
        GIMP_GRADIENT (gimp_data_duplicate (GIMP_DATA (blend_tool->gradient)));

      /* swap the added and removed handles, so that blend_info_apply() does
       * the right thing on redo
       */
      new_info->added_handle    = info->removed_handle;
      new_info->removed_handle  = info->added_handle;
      new_info->selected_handle = info->selected_handle;
    }

  blend_tool->undo_stack = g_slist_remove (blend_tool->undo_stack, info);
  blend_tool->redo_stack = g_slist_prepend (blend_tool->redo_stack, new_info);

  gimp_blend_tool_editor_blend_info_apply (blend_tool, info, TRUE);
  gimp_blend_tool_editor_blend_info_free (info);

  /* the initial state of the blend tool is not useful; we might as well halt */
  if (! blend_tool->undo_stack)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  return TRUE;
}

gboolean
gimp_blend_tool_editor_redo (GimpBlendTool *blend_tool)
{
  BlendInfo *info;
  BlendInfo *new_info;

  g_assert (blend_tool->redo_stack != NULL);
  g_assert (blend_tool->edit_count == 0);

  info = blend_tool->redo_stack->data;

  new_info = gimp_blend_tool_editor_blend_info_new (blend_tool);

  if (info->gradient)
    {
      new_info->gradient =
        GIMP_GRADIENT (gimp_data_duplicate (GIMP_DATA (blend_tool->gradient)));

      /* swap the added and removed handles, so that blend_info_apply() does
       * the right thing on undo
       */
      new_info->added_handle    = info->removed_handle;
      new_info->removed_handle  = info->added_handle;
      new_info->selected_handle = info->selected_handle;
    }

  blend_tool->redo_stack = g_slist_remove (blend_tool->redo_stack, info);
  blend_tool->undo_stack = g_slist_prepend (blend_tool->undo_stack, new_info);

  gimp_blend_tool_editor_blend_info_apply (blend_tool, info, TRUE);
  gimp_blend_tool_editor_blend_info_free (info);

  return TRUE;
}

void
gimp_blend_tool_editor_start_edit (GimpBlendTool *blend_tool)
{
  if (blend_tool->edit_count++ == 0)
    {
      BlendInfo *info;

      info = gimp_blend_tool_editor_blend_info_new (blend_tool);

      blend_tool->undo_stack = g_slist_prepend (blend_tool->undo_stack, info);
    }
}

void
gimp_blend_tool_editor_end_edit (GimpBlendTool *blend_tool,
                                 gboolean       cancel)
{
  /* can happen when halting using esc */
  if (blend_tool->edit_count == 0)
    return;

  if (--blend_tool->edit_count == 0)
    {
      BlendInfo *info = blend_tool->undo_stack->data;

      info->selected_handle =
        gimp_tool_line_get_selection (GIMP_TOOL_LINE (blend_tool->widget));

      if (cancel                                ||
          (info->start_x == blend_tool->start_x &&
           info->start_y == blend_tool->start_y &&
           info->end_x   == blend_tool->end_x   &&
           info->end_y   == blend_tool->end_y   &&
           ! info->gradient))
        {
          /* if the edit is canceled, or if nothing changed, undo the last
           * step
           */
          gimp_blend_tool_editor_blend_info_apply (blend_tool, info, FALSE);

          blend_tool->undo_stack = g_slist_remove (blend_tool->undo_stack,
                                                   info);
          gimp_blend_tool_editor_blend_info_free (info);
        }
      else
        {
          /* otherwise, blow the redo stack */
          g_slist_free_full (blend_tool->redo_stack,
                             (GDestroyNotify) gimp_blend_tool_editor_blend_info_free);
          blend_tool->redo_stack = NULL;
        }

      /*  update the undo actions / menu items  */
      if (! blend_tool->flush_idle_id)
        {
          blend_tool->flush_idle_id =
            g_idle_add ((GSourceFunc) gimp_blend_tool_editor_flush_idle,
                        blend_tool);
        }
    }
}
