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

#include "paint/gimpclone.h"
#include "paint/gimpcloneoptions.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpclonetool.h"
#include "gimppaintoptions-gui.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static GtkWidget * gimp_clone_options_gui (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpCloneTool, gimp_clone_tool, GIMP_TYPE_SOURCE_TOOL)

#define parent_class gimp_clone_tool_parent_class


void
gimp_clone_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_CLONE_TOOL,
                GIMP_TYPE_CLONE_OPTIONS,
                gimp_clone_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PATTERN_MASK,
                "gimp-clone-tool",
                _("Clone"),
                _("Clone Tool: Selectively copy from an image or pattern, using a brush"),
                N_("_Clone"), "C",
                NULL, GIMP_HELP_TOOL_CLONE,
                GIMP_STOCK_TOOL_CLONE,
                data);
}

static void
gimp_clone_tool_class_init (GimpCloneToolClass *klass)
{
}

static void
gimp_clone_tool_init (GimpCloneTool *clone)
{
  GimpTool       *tool        = GIMP_TOOL (clone);
  GimpPaintTool  *paint_tool  = GIMP_PAINT_TOOL (tool);
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (tool);

  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_CLONE);
  gimp_tool_control_set_action_object_2 (tool->control,
                                         "context/context-pattern-select-set");

  paint_tool->status      = _("Click to clone");
  paint_tool->status_ctrl = _("%s to set a new clone source");

  source_tool->status_paint           = _("Click to clone");
  /* Translators: the translation of "Click" must be the first word */
  source_tool->status_set_source      = _("Click to set a new clone source");
  source_tool->status_set_source_ctrl = _("%s to set a new clone source");
}


/*  tool options stuff  */

static GtkWidget *
gimp_clone_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *combo;
  GtkWidget *label;

  frame = gimp_prop_enum_radio_frame_new (config, "clone-type",
                                          _("Source"), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gimp_prop_check_button_new (config, "sample-merged",
                                       _("Sample merged"));
  gimp_enum_radio_frame_add (GTK_FRAME (frame), button,
                             GIMP_IMAGE_CLONE, TRUE);

  hbox = gimp_prop_pattern_box_new (NULL, GIMP_CONTEXT (tool_options),
                                    NULL, 2,
                                    "pattern-view-type", "pattern-view-size");
  gimp_enum_radio_frame_add (GTK_FRAME (frame), hbox,
                             GIMP_PATTERN_CLONE, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Alignment:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_prop_enum_combo_box_new (config, "align-mode", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  return vbox;
}
