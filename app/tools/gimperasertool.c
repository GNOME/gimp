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

#include "core/ligmadrawable.h"

#include "paint/ligmaeraseroptions.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmaerasertool.h"
#include "ligmapaintoptions-gui.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


static void        ligma_eraser_tool_modifier_key  (LigmaTool         *tool,
                                                   GdkModifierType   key,
                                                   gboolean          press,
                                                   GdkModifierType   state,
                                                   LigmaDisplay      *display);
static void        ligma_eraser_tool_cursor_update (LigmaTool         *tool,
                                                   const LigmaCoords *coords,
                                                   GdkModifierType   state,
                                                   LigmaDisplay      *display);

static gboolean    ligma_eraser_tool_is_alpha_only (LigmaPaintTool    *paint_tool,
                                                   LigmaDrawable     *drawable);

static GtkWidget * ligma_eraser_options_gui        (LigmaToolOptions  *tool_options);


G_DEFINE_TYPE (LigmaEraserTool, ligma_eraser_tool, LIGMA_TYPE_BRUSH_TOOL)

#define parent_class ligma_eraser_tool_parent_class


void
ligma_eraser_tool_register (LigmaToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (LIGMA_TYPE_ERASER_TOOL,
                LIGMA_TYPE_ERASER_OPTIONS,
                ligma_eraser_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK,
                "ligma-eraser-tool",
                _("Eraser"),
                _("Eraser Tool: Erase to background or transparency using a brush"),
                N_("_Eraser"), "<shift>E",
                NULL, LIGMA_HELP_TOOL_ERASER,
                LIGMA_ICON_TOOL_ERASER,
                data);
}

static void
ligma_eraser_tool_class_init (LigmaEraserToolClass *klass)
{
  LigmaToolClass      *tool_class       = LIGMA_TOOL_CLASS (klass);
  LigmaPaintToolClass *paint_tool_class = LIGMA_PAINT_TOOL_CLASS (klass);

  tool_class->modifier_key        = ligma_eraser_tool_modifier_key;
  tool_class->cursor_update       = ligma_eraser_tool_cursor_update;

  paint_tool_class->is_alpha_only = ligma_eraser_tool_is_alpha_only;
}

static void
ligma_eraser_tool_init (LigmaEraserTool *eraser)
{
  LigmaTool      *tool       = LIGMA_TOOL (eraser);
  LigmaPaintTool *paint_tool = LIGMA_PAINT_TOOL (eraser);

  ligma_tool_control_set_tool_cursor            (tool->control,
                                                LIGMA_TOOL_CURSOR_ERASER);
  ligma_tool_control_set_toggle_cursor_modifier (tool->control,
                                                LIGMA_CURSOR_MODIFIER_MINUS);

  ligma_paint_tool_enable_color_picker (paint_tool,
                                       LIGMA_COLOR_PICK_TARGET_BACKGROUND);

  paint_tool->status      = _("Click to erase");
  paint_tool->status_line = _("Click to erase the line");
  paint_tool->status_ctrl = _("%s to pick a background color");
}

static void
ligma_eraser_tool_modifier_key (LigmaTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               LigmaDisplay     *display)
{
  if (key == GDK_MOD1_MASK)
    {
      LigmaEraserOptions *options = LIGMA_ERASER_TOOL_GET_OPTIONS (tool);

      g_object_set (options,
                    "anti-erase", ! options->anti_erase,
                    NULL);
    }

  LIGMA_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state, display);
}

static void
ligma_eraser_tool_cursor_update (LigmaTool         *tool,
                                const LigmaCoords *coords,
                                GdkModifierType   state,
                                LigmaDisplay      *display)
{
  LigmaEraserOptions *options = LIGMA_ERASER_TOOL_GET_OPTIONS (tool);

  ligma_tool_control_set_toggled (tool->control, options->anti_erase);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
ligma_eraser_tool_is_alpha_only (LigmaPaintTool *paint_tool,
                                LigmaDrawable  *drawable)
{
  LigmaEraserOptions *options = LIGMA_ERASER_TOOL_GET_OPTIONS (paint_tool);

  if (! options->anti_erase)
    return ligma_drawable_has_alpha (drawable);
  else
    return TRUE;
}


/*  tool options stuff  */

static GtkWidget *
ligma_eraser_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_paint_options_gui (tool_options);
  GtkWidget *button;
  gchar     *str;

  /* the anti_erase toggle */
  str = g_strdup_printf (_("Anti erase  (%s)"),
                         ligma_get_mod_string (GDK_MOD1_MASK));

  button = ligma_prop_check_button_new (config, "anti-erase", str);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  g_free (str);

  return vbox;
}
