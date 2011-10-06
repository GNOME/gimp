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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "paint/gimpconvolveoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpconvolvetool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_convolve_tool_modifier_key  (GimpTool         *tool,
                                                GdkModifierType   key,
                                                gboolean          press,
                                                GdkModifierType   state,
                                                GimpDisplay      *display);
static void   gimp_convolve_tool_cursor_update (GimpTool         *tool,
                                                const GimpCoords *coords,
                                                GdkModifierType   state,
                                                GimpDisplay      *display);
static void   gimp_convolve_tool_oper_update   (GimpTool         *tool,
                                                const GimpCoords *coords,
                                                GdkModifierType   state,
                                                gboolean          proximity,
                                                GimpDisplay      *display);
static void   gimp_convolve_tool_status_update (GimpTool         *tool,
                                                GimpConvolveType  type);

static GtkWidget * gimp_convolve_options_gui   (GimpToolOptions  *options);


G_DEFINE_TYPE (GimpConvolveTool, gimp_convolve_tool, GIMP_TYPE_BRUSH_TOOL)

#define parent_class gimp_convolve_tool_parent_class


void
gimp_convolve_tool_register (GimpToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (GIMP_TYPE_CONVOLVE_TOOL,
                GIMP_TYPE_CONVOLVE_OPTIONS,
                gimp_convolve_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK,
                "gimp-convolve-tool",
                _("Blur / Sharpen"),
                _("Blur / Sharpen Tool: Selective blurring or unblurring using a brush"),
                N_("Bl_ur / Sharpen"), "<shift>U",
                NULL, GIMP_HELP_TOOL_CONVOLVE,
                GIMP_STOCK_TOOL_BLUR,
                data);
}

static void
gimp_convolve_tool_class_init (GimpConvolveToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->modifier_key  = gimp_convolve_tool_modifier_key;
  tool_class->cursor_update = gimp_convolve_tool_cursor_update;
  tool_class->oper_update   = gimp_convolve_tool_oper_update;
}

static void
gimp_convolve_tool_init (GimpConvolveTool *convolve)
{
  GimpTool *tool = GIMP_TOOL (convolve);

  gimp_tool_control_set_tool_cursor            (tool->control,
                                                GIMP_TOOL_CURSOR_BLUR);
  gimp_tool_control_set_toggle_cursor_modifier (tool->control,
                                                GIMP_CURSOR_MODIFIER_MINUS);

  gimp_convolve_tool_status_update (tool, GIMP_BLUR_CONVOLVE);
}

static void
gimp_convolve_tool_modifier_key (GimpTool        *tool,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state,
                                 GimpDisplay     *display)
{
  GimpConvolveTool    *convolve = GIMP_CONVOLVE_TOOL (tool);
  GimpConvolveOptions *options  = GIMP_CONVOLVE_TOOL_GET_OPTIONS (tool);
  GdkModifierType      toggle_mask;

  toggle_mask = gimp_get_toggle_behavior_mask ();

  if (((key == toggle_mask)       &&
       ! (state & GDK_SHIFT_MASK) && /* leave stuff untouched in line draw mode */
       press != convolve->toggled)

      ||

      (key == GDK_SHIFT_MASK && /* toggle back after keypresses CTRL(hold)->  */
       ! press               && /* SHIFT(hold)->CTRL(release)->SHIFT(release) */
       convolve->toggled     &&
       ! (state & toggle_mask)))
    {
      convolve->toggled = press;

      switch (options->type)
        {
        case GIMP_BLUR_CONVOLVE:
          g_object_set (options, "type", GIMP_SHARPEN_CONVOLVE, NULL);
          break;

        case GIMP_SHARPEN_CONVOLVE:
          g_object_set (options, "type", GIMP_BLUR_CONVOLVE, NULL);
          break;

        default:
          break;
        }
    }
}

static void
gimp_convolve_tool_cursor_update (GimpTool         *tool,
                                  const GimpCoords *coords,
                                  GdkModifierType   state,
                                  GimpDisplay      *display)
{
  GimpConvolveOptions *options = GIMP_CONVOLVE_TOOL_GET_OPTIONS (tool);

  gimp_tool_control_set_toggled (tool->control,
                                 (options->type == GIMP_SHARPEN_CONVOLVE));

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_convolve_tool_oper_update (GimpTool         *tool,
                                const GimpCoords *coords,
                                GdkModifierType   state,
                                gboolean          proximity,
                                GimpDisplay      *display)
{
  GimpConvolveOptions *options = GIMP_CONVOLVE_TOOL_GET_OPTIONS (tool);

  gimp_convolve_tool_status_update (tool, options->type);

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
gimp_convolve_tool_status_update (GimpTool         *tool,
                                  GimpConvolveType  type)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);

  switch (type)
    {
    case GIMP_BLUR_CONVOLVE:
      paint_tool->status      = _("Click to blur");
      paint_tool->status_line = _("Click to blur the line");
      paint_tool->status_ctrl = _("%s to sharpen");
      break;

    case GIMP_SHARPEN_CONVOLVE:
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
gimp_convolve_options_gui (GimpToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget       *frame;
  GtkWidget       *scale;
  gchar           *str;
  GdkModifierType  toggle_mask;

  toggle_mask = gimp_get_toggle_behavior_mask ();

  /*  the type radio box  */
  str = g_strdup_printf (_("Convolve Type  (%s)"),
                         gimp_get_mod_string (toggle_mask));

  frame = gimp_prop_enum_radio_frame_new (config, "type",
                                          str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (str);

  /*  the rate scale  */
  scale = gimp_prop_spin_scale_new (config, "rate",
                                    _("Rate"),
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  return vbox;
}
