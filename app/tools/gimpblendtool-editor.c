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

  gimp_blend_tool_editor_unblock_handlers (blend_tool);
}


/*  public functions  */


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
}
