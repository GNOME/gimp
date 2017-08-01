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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "operations/gimp-operation-config.h"

#include "core/gimpdata.h"
#include "core/gimpgradient.h"
#include "core/gimp-gradients.h"

#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolline.h"

#include "gimpblendoptions.h"
#include "gimpblendtool.h"
#include "gimpblendtool-editor.h"

#include "gimp-intl.h"


#define EPSILON 2e-10


/*  local function prototypes  */

static gboolean              gimp_blend_tool_editor_line_can_add_slider           (GimpToolLine        *line,
                                                                                   gdouble              value,
                                                                                   GimpBlendTool       *blend_tool);
static gint                  gimp_blend_tool_editor_line_add_slider               (GimpToolLine        *line,
                                                                                   gdouble              value,
                                                                                   GimpBlendTool       *blend_tool);
static void                  gimp_blend_tool_editor_line_prepare_to_remove_slider (GimpToolLine        *line,
                                                                                   gint                 slider,
                                                                                   gboolean             remove,
                                                                                   GimpBlendTool       *blend_tool);
static void                  gimp_blend_tool_editor_line_remove_slider            (GimpToolLine        *line,
                                                                                   gint                 slider,
                                                                                   GimpBlendTool       *blend_tool);
static gboolean              gimp_blend_tool_editor_line_handle_clicked           (GimpToolLine        *line,
                                                                                   gint                 handle,
                                                                                   GdkModifierType      state,
                                                                                   GimpButtonPressType  press_type,
                                                                                   GimpBlendTool       *blend_tool);

static gboolean              gimp_blend_tool_editor_is_gradient_editable          (GimpBlendTool       *blend_tool);

static gboolean              gimp_blend_tool_editor_handle_is_endpoint            (GimpBlendTool       *blend_tool,
                                                                                   gint                 handle);
static gboolean              gimp_blend_tool_editor_handle_is_stop                (GimpBlendTool       *blend_tool,
                                                                                   gint                 handle);
static gboolean              gimp_blend_tool_editor_handle_is_midpoint            (GimpBlendTool       *blend_tool,
                                                                                   gint                 handle);
static GimpGradientSegment * gimp_blend_tool_editor_handle_get_segment            (GimpBlendTool       *blend_tool,
                                                                                   gint                 handle);

static void                  gimp_blend_tool_editor_block_handlers                (GimpBlendTool       *blend_tool);
static void                  gimp_blend_tool_editor_unblock_handlers              (GimpBlendTool       *blend_tool);
static gboolean              gimp_blend_tool_editor_are_handlers_blocked          (GimpBlendTool       *blend_tool);

static void                  gimp_blend_tool_editor_freeze_gradient               (GimpBlendTool       *blend_tool);
static void                  gimp_blend_tool_editor_thaw_gradient                 (GimpBlendTool       *blend_tool);

static gint                  gimp_blend_tool_editor_add_stop                      (GimpBlendTool       *blend_tool,
                                                                                   gdouble              value);

static void                  gimp_blend_tool_editor_update_sliders                (GimpBlendTool       *blend_tool);


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
  GimpGradientSegment *seg;

  gimp_blend_tool_editor_freeze_gradient (blend_tool);

  seg = gimp_blend_tool_editor_handle_get_segment (blend_tool, slider);

  gimp_gradient_segment_range_merge (blend_tool->gradient,
                                     seg, seg->next, NULL, NULL);

  gimp_blend_tool_editor_thaw_gradient (blend_tool);

  gimp_blend_tool_set_tentative_gradient (blend_tool, NULL);
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
          const GimpControllerSlider *sliders;
          gint                        stop;

          sliders = gimp_tool_line_get_sliders (line, NULL);

          if (sliders[handle].value > sliders[handle].min + EPSILON &&
              sliders[handle].value < sliders[handle].max - EPSILON)
            {
              stop = gimp_blend_tool_editor_add_stop (blend_tool,
                                                      sliders[handle].value);

              gimp_tool_line_set_selection (line, stop);
            }

          /* return FALSE, so that the new slider can be dragged immediately */
          return FALSE;
        }
    }

  return FALSE;
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
}

static void
gimp_blend_tool_editor_thaw_gradient(GimpBlendTool *blend_tool)
{
  gimp_data_thaw (GIMP_DATA (blend_tool->gradient));

  gimp_blend_tool_editor_update_sliders (blend_tool);

  gimp_blend_tool_editor_unblock_handlers (blend_tool);
}

static gint
gimp_blend_tool_editor_add_stop (GimpBlendTool *blend_tool,
                                 gdouble        value)
{
  GimpBlendOptions    *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpGradientSegment *seg;
  gint                 stop;

  gimp_blend_tool_editor_freeze_gradient (blend_tool);

  gimp_gradient_split_at (blend_tool->gradient,
                          GIMP_CONTEXT (options), NULL, value, &seg, NULL);

  stop =
    gimp_gradient_segment_range_get_n_segments (blend_tool->gradient,
                                                blend_tool->gradient->segments,
                                                seg) - 1;

  gimp_blend_tool_editor_thaw_gradient (blend_tool);

  return stop;
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


/*  public functions  */


void
gimp_blend_tool_editor_options_notify (GimpBlendTool    *blend_tool,
                                       GimpToolOptions  *options,
                                       const GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "modify-active"))
    {
      gimp_blend_tool_editor_update_sliders (blend_tool);
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
  g_signal_connect (blend_tool->widget, "handle-clicked",
                    G_CALLBACK (gimp_blend_tool_editor_line_handle_clicked),
                    blend_tool);
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
    gimp_blend_tool_editor_thaw_gradient (blend_tool);
}

void
gimp_blend_tool_editor_gradient_dirty (GimpBlendTool *blend_tool)
{
  if (gimp_blend_tool_editor_are_handlers_blocked (blend_tool))
    return;

  if (blend_tool->widget)
    {
      gimp_blend_tool_editor_update_sliders (blend_tool);

      gimp_tool_line_set_selection (GIMP_TOOL_LINE (blend_tool->widget),
                                    GIMP_TOOL_LINE_HANDLE_NONE);
    }
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

  if (blend_tool->widget)
    {
      gimp_blend_tool_editor_update_sliders (blend_tool);

      gimp_tool_line_set_selection (GIMP_TOOL_LINE (blend_tool->widget),
                                    GIMP_TOOL_LINE_HANDLE_NONE);
    }
}
