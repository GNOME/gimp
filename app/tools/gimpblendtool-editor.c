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


/*  local function prototypes  */

static gboolean   gimp_blend_tool_editor_is_gradient_editable (GimpBlendTool *blend_tool);

static void       gimp_blend_tool_editor_block_handlers       (GimpBlendTool *blend_tool);
static void       gimp_blend_tool_editor_unblock_handlers     (GimpBlendTool *blend_tool);
static gboolean   gimp_blend_tool_editor_are_handlers_blocked (GimpBlendTool *blend_tool);

static void       gimp_blend_tool_editor_freeze_gradient      (GimpBlendTool *blend_tool);
static void       gimp_blend_tool_editor_thaw_gradient        (GimpBlendTool *blend_tool);

static void       gimp_blend_tool_editor_update_sliders       (GimpBlendTool *blend_tool);


/*  private functions  */


static gboolean
gimp_blend_tool_editor_is_gradient_editable (GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);

  return ! options->modify_active ||
         gimp_data_is_writable (GIMP_DATA (blend_tool->gradient));
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

      slider->value   = seg->right;
      slider->min     = seg->left;
      slider->max     = seg->next->right;

      slider->movable = editable;

      slider->data    = GINT_TO_POINTER (i);

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

  gimp_tool_line_set_sliders (GIMP_TOOL_LINE (blend_tool->widget),
                              sliders, n_sliders);

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
