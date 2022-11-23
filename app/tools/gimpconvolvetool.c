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

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "paint/ligmaconvolveoptions.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmaconvolvetool.h"
#include "ligmapaintoptions-gui.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static void   ligma_convolve_tool_modifier_key  (LigmaTool         *tool,
                                                GdkModifierType   key,
                                                gboolean          press,
                                                GdkModifierType   state,
                                                LigmaDisplay      *display);
static void   ligma_convolve_tool_cursor_update (LigmaTool         *tool,
                                                const LigmaCoords *coords,
                                                GdkModifierType   state,
                                                LigmaDisplay      *display);
static void   ligma_convolve_tool_oper_update   (LigmaTool         *tool,
                                                const LigmaCoords *coords,
                                                GdkModifierType   state,
                                                gboolean          proximity,
                                                LigmaDisplay      *display);
static void   ligma_convolve_tool_status_update (LigmaTool         *tool,
                                                LigmaConvolveType  type);

static GtkWidget * ligma_convolve_options_gui   (LigmaToolOptions  *options);


G_DEFINE_TYPE (LigmaConvolveTool, ligma_convolve_tool, LIGMA_TYPE_BRUSH_TOOL)

#define parent_class ligma_convolve_tool_parent_class


void
ligma_convolve_tool_register (LigmaToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (LIGMA_TYPE_CONVOLVE_TOOL,
                LIGMA_TYPE_CONVOLVE_OPTIONS,
                ligma_convolve_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK,
                "ligma-convolve-tool",
                _("Blur / Sharpen"),
                _("Blur / Sharpen Tool: Selective blurring or unblurring using a brush"),
                N_("Bl_ur / Sharpen"), "<shift>U",
                NULL, LIGMA_HELP_TOOL_CONVOLVE,
                LIGMA_ICON_TOOL_BLUR,
                data);
}

static void
ligma_convolve_tool_class_init (LigmaConvolveToolClass *klass)
{
  LigmaToolClass *tool_class = LIGMA_TOOL_CLASS (klass);

  tool_class->modifier_key  = ligma_convolve_tool_modifier_key;
  tool_class->cursor_update = ligma_convolve_tool_cursor_update;
  tool_class->oper_update   = ligma_convolve_tool_oper_update;
}

static void
ligma_convolve_tool_init (LigmaConvolveTool *convolve)
{
  LigmaTool *tool = LIGMA_TOOL (convolve);

  ligma_tool_control_set_tool_cursor            (tool->control,
                                                LIGMA_TOOL_CURSOR_BLUR);
  ligma_tool_control_set_toggle_cursor_modifier (tool->control,
                                                LIGMA_CURSOR_MODIFIER_MINUS);

  ligma_convolve_tool_status_update (tool, LIGMA_CONVOLVE_BLUR);
}

static void
ligma_convolve_tool_modifier_key (LigmaTool        *tool,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state,
                                 LigmaDisplay     *display)
{
  LigmaConvolveTool    *convolve    = LIGMA_CONVOLVE_TOOL (tool);
  LigmaConvolveOptions *options     = LIGMA_CONVOLVE_TOOL_GET_OPTIONS (tool);
  GdkModifierType      line_mask   = LIGMA_PAINT_TOOL_LINE_MASK;
  GdkModifierType      toggle_mask = ligma_get_toggle_behavior_mask ();

  if (((key == toggle_mask)  &&
       ! (state & line_mask) && /* leave stuff untouched in line draw mode */
       press != convolve->toggled)

      ||

      (key == line_mask  && /* toggle back after keypresses CTRL(hold)->  */
       ! press           && /* SHIFT(hold)->CTRL(release)->SHIFT(release) */
       convolve->toggled &&
       ! (state & toggle_mask)))
    {
      convolve->toggled = press;

      switch (options->type)
        {
        case LIGMA_CONVOLVE_BLUR:
          g_object_set (options, "type", LIGMA_CONVOLVE_SHARPEN, NULL);
          break;

        case LIGMA_CONVOLVE_SHARPEN:
          g_object_set (options, "type", LIGMA_CONVOLVE_BLUR, NULL);
          break;
        }
    }
}

static void
ligma_convolve_tool_cursor_update (LigmaTool         *tool,
                                  const LigmaCoords *coords,
                                  GdkModifierType   state,
                                  LigmaDisplay      *display)
{
  LigmaConvolveOptions *options = LIGMA_CONVOLVE_TOOL_GET_OPTIONS (tool);

  ligma_tool_control_set_toggled (tool->control,
                                 options->type == LIGMA_CONVOLVE_SHARPEN);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_convolve_tool_oper_update (LigmaTool         *tool,
                                const LigmaCoords *coords,
                                GdkModifierType   state,
                                gboolean          proximity,
                                LigmaDisplay      *display)
{
  LigmaConvolveOptions *options = LIGMA_CONVOLVE_TOOL_GET_OPTIONS (tool);

  ligma_convolve_tool_status_update (tool, options->type);

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
ligma_convolve_tool_status_update (LigmaTool         *tool,
                                  LigmaConvolveType  type)
{
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (tool);

  switch (type)
    {
    case LIGMA_CONVOLVE_BLUR:
      paint_tool->status      = _("Click to blur");
      paint_tool->status_line = _("Click to blur the line");
      paint_tool->status_ctrl = _("%s to sharpen");
      break;

    case LIGMA_CONVOLVE_SHARPEN:
      paint_tool->status      = _("Click to sharpen");
      paint_tool->status_line = _("Click to sharpen the line");
      paint_tool->status_ctrl = _("%s to blur");
      break;

    default:
      break;
    }
}


/*  tool options stuff  */

static GtkWidget *
ligma_convolve_options_gui (LigmaToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = ligma_paint_options_gui (tool_options);
  GtkWidget       *frame;
  GtkWidget       *scale;
  gchar           *str;
  GdkModifierType  toggle_mask;

  toggle_mask = ligma_get_toggle_behavior_mask ();

  /*  the type radio box  */
  str = g_strdup_printf (_("Convolve Type  (%s)"),
                         ligma_get_mod_string (toggle_mask));

  frame = ligma_prop_enum_radio_frame_new (config, "type",
                                          str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  g_free (str);

  /*  the rate scale  */
  scale = ligma_prop_spin_scale_new (config, "rate",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
