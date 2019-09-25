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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpdrawable.h"

#include "paint/gimperaseroptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimperasertool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void        gimp_eraser_tool_modifier_key  (GimpTool         *tool,
                                                   GdkModifierType   key,
                                                   gboolean          press,
                                                   GdkModifierType   state,
                                                   GimpDisplay      *display);
static void        gimp_eraser_tool_cursor_update (GimpTool         *tool,
                                                   const GimpCoords *coords,
                                                   GdkModifierType   state,
                                                   GimpDisplay      *display);

static gboolean    gimp_eraser_tool_is_alpha_only (GimpPaintTool    *paint_tool,
                                                   GimpDrawable     *drawable);

static GtkWidget * gimp_eraser_options_gui        (GimpToolOptions  *tool_options);


G_DEFINE_TYPE (GimpEraserTool, gimp_eraser_tool, GIMP_TYPE_BRUSH_TOOL)

#define parent_class gimp_eraser_tool_parent_class


void
gimp_eraser_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_ERASER_TOOL,
                GIMP_TYPE_ERASER_OPTIONS,
                gimp_eraser_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK,
                "gimp-eraser-tool",
                _("Eraser"),
                _("Eraser Tool: Erase to background or transparency using a brush"),
                N_("_Eraser"), "<shift>E",
                NULL, GIMP_HELP_TOOL_ERASER,
                GIMP_ICON_TOOL_ERASER,
                data);
}

static void
gimp_eraser_tool_class_init (GimpEraserToolClass *klass)
{
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpPaintToolClass *paint_tool_class = GIMP_PAINT_TOOL_CLASS (klass);

  tool_class->modifier_key        = gimp_eraser_tool_modifier_key;
  tool_class->cursor_update       = gimp_eraser_tool_cursor_update;

  paint_tool_class->is_alpha_only = gimp_eraser_tool_is_alpha_only;
}

static void
gimp_eraser_tool_init (GimpEraserTool *eraser)
{
  GimpTool      *tool       = GIMP_TOOL (eraser);
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (eraser);

  gimp_tool_control_set_tool_cursor            (tool->control,
                                                GIMP_TOOL_CURSOR_ERASER);
  gimp_tool_control_set_toggle_cursor_modifier (tool->control,
                                                GIMP_CURSOR_MODIFIER_MINUS);

  gimp_paint_tool_enable_color_picker (paint_tool,
                                       GIMP_COLOR_PICK_TARGET_BACKGROUND);

  paint_tool->status      = _("Click to erase");
  paint_tool->status_line = _("Click to erase the line");
  paint_tool->status_ctrl = _("%s to pick a background color");
}

static void
gimp_eraser_tool_modifier_key (GimpTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  if (key == GDK_MOD1_MASK)
    {
      GimpEraserOptions *options = GIMP_ERASER_TOOL_GET_OPTIONS (tool);

      g_object_set (options,
                    "anti-erase", ! options->anti_erase,
                    NULL);
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state, display);
}

static void
gimp_eraser_tool_cursor_update (GimpTool         *tool,
                                const GimpCoords *coords,
                                GdkModifierType   state,
                                GimpDisplay      *display)
{
  GimpEraserOptions *options = GIMP_ERASER_TOOL_GET_OPTIONS (tool);

  gimp_tool_control_set_toggled (tool->control, options->anti_erase);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
gimp_eraser_tool_is_alpha_only (GimpPaintTool *paint_tool,
                                GimpDrawable  *drawable)
{
  GimpEraserOptions *options = GIMP_ERASER_TOOL_GET_OPTIONS (paint_tool);

  if (! options->anti_erase)
    return gimp_drawable_has_alpha (drawable);
  else
    return TRUE;
}


/*  tool options stuff  */

static GtkWidget *
gimp_eraser_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *button;
  gchar     *str;

  /* the anti_erase toggle */
  str = g_strdup_printf (_("Anti erase  (%s)"),
                         gimp_get_mod_string (GDK_MOD1_MASK));

  button = gimp_prop_check_button_new (config, "anti-erase", str);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  g_free (str);

  return vbox;
}
