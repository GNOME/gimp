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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "operations/gimpcolorbalanceconfig.h"

#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"

#include "display/gimpdisplay.h"

#include "gimpcolorbalancetool.h"
#include "gimpfilteroptions.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static gboolean   gimp_color_balance_tool_initialize    (GimpTool        *tool,
                                                         GimpDisplay     *display,
                                                         GError         **error);

static gchar    * gimp_color_balance_tool_get_operation (GimpFilterTool  *filter_tool,
                                                         gchar          **title,
                                                         gchar          **description,
                                                         gchar          **undo_desc,
                                                         gchar          **icon_name,
                                                         gchar          **help_id,
                                                         gboolean        *has_settings,
                                                         gchar          **settings_folder,
                                                         gchar          **import_dialog_title,
                                                         gchar          **export_dialog_title);
static void       gimp_color_balance_tool_dialog        (GimpFilterTool  *filter_tool);
static void       gimp_color_balance_tool_reset         (GimpFilterTool  *filter_tool);

static void       color_balance_range_reset_callback    (GtkWidget       *widget,
                                                         GimpFilterTool  *filter_tool);


G_DEFINE_TYPE (GimpColorBalanceTool, gimp_color_balance_tool,
               GIMP_TYPE_FILTER_TOOL)

#define parent_class gimp_color_balance_tool_parent_class


void
gimp_color_balance_tool_register (GimpToolRegisterCallback  callback,
                                  gpointer                  data)
{
  (* callback) (GIMP_TYPE_COLOR_BALANCE_TOOL,
                GIMP_TYPE_FILTER_OPTIONS, NULL,
                0,
                "gimp-color-balance-tool",
                _("Color Balance"),
                _("Color Balance Tool: Adjust color distribution"),
                N_("Color _Balance..."), NULL,
                NULL, GIMP_HELP_TOOL_COLOR_BALANCE,
                GIMP_ICON_TOOL_COLOR_BALANCE,
                data);
}

static void
gimp_color_balance_tool_class_init (GimpColorBalanceToolClass *klass)
{
  GimpToolClass       *tool_class        = GIMP_TOOL_CLASS (klass);
  GimpFilterToolClass *filter_tool_class = GIMP_FILTER_TOOL_CLASS (klass);

  tool_class->initialize           = gimp_color_balance_tool_initialize;

  filter_tool_class->get_operation = gimp_color_balance_tool_get_operation;
  filter_tool_class->dialog        = gimp_color_balance_tool_dialog;
  filter_tool_class->reset         = gimp_color_balance_tool_reset;
}

static void
gimp_color_balance_tool_init (GimpColorBalanceTool *cb_tool)
{
}

static gboolean
gimp_color_balance_tool_initialize (GimpTool     *tool,
                                    GimpDisplay  *display,
                                    GError      **error)
{
  GimpImage    *image    = gimp_display_get_image (display);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    return FALSE;

  if (! gimp_drawable_is_rgb (drawable))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Color Balance operates only on RGB color layers."));
      return FALSE;
    }

  return GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);
}

static gchar *
gimp_color_balance_tool_get_operation (GimpFilterTool  *filter_tool,
                                       gchar          **title,
                                       gchar          **description,
                                       gchar          **undo_desc,
                                       gchar          **icon_name,
                                       gchar          **help_id,
                                       gboolean        *has_settings,
                                       gchar          **settings_folder,
                                       gchar          **import_dialog_title,
                                       gchar          **export_dialog_title)
{
  *description         = g_strdup (_("Adjust Color Balance"));
  *has_settings        = TRUE;
  *settings_folder     = g_strdup ("color-balance");
  *import_dialog_title = g_strdup (_("Import Color Balance Settings"));
  *export_dialog_title = g_strdup (_("Export Color Balance Settings"));

  return g_strdup ("gimp:color-balance");
}


/**************************/
/*  Color Balance dialog  */
/**************************/

static void
create_levels_scale (GObject     *config,
                     const gchar *property_name,
                     const gchar *left,
                     const gchar *right,
                     GtkWidget   *table,
                     gint         col)
{
  GtkWidget *label;
  GtkWidget *scale;

  label = gtk_label_new (left);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, col, col + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  scale = gimp_prop_spin_scale_new (config, property_name,
                                    NULL, 0.01, 0.1, 0);
  gimp_spin_scale_set_label (GIMP_SPIN_SCALE (scale), NULL);
  gimp_prop_widget_set_factor (scale, 100.0, 0.0, 0.0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), scale, 1, 2, col, col + 1);
  gtk_widget_show (scale);

  label = gtk_label_new (right);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, col, col + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);
}

static void
gimp_color_balance_tool_dialog (GimpFilterTool *filter_tool)
{
  GimpColorBalanceTool *cb_tool = GIMP_COLOR_BALANCE_TOOL (filter_tool);
  GtkWidget            *main_vbox;
  GtkWidget            *vbox;
  GtkWidget            *hbox;
  GtkWidget            *table;
  GtkWidget            *button;
  GtkWidget            *frame;

  main_vbox = gimp_filter_tool_dialog_get_vbox (filter_tool);

  frame = gimp_prop_enum_radio_frame_new (filter_tool->config, "range",
                                          _("Select Range to Adjust"),
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gimp_frame_new (_("Adjust Color Levels"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  The table containing sliders  */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  create_levels_scale (filter_tool->config, "cyan-red",
                       _("Cyan"), _("Red"),
                       table, 0);

  create_levels_scale (filter_tool->config, "magenta-green",
                       _("Magenta"), _("Green"),
                       table, 1);

  create_levels_scale (filter_tool->config, "yellow-blue",
                       _("Yellow"), _("Blue"),
                       table, 2);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("R_eset Range"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (color_balance_range_reset_callback),
                    cb_tool);

  button = gimp_prop_check_button_new (filter_tool->config,
                                       "preserve-luminosity",
                                       _("Preserve _luminosity"));
  gtk_box_pack_end (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
}

static void
gimp_color_balance_tool_reset (GimpFilterTool *filter_tool)
{
  GimpTransferMode range;

  g_object_get (filter_tool->config,
                "range", &range,
                NULL);

  GIMP_FILTER_TOOL_CLASS (parent_class)->reset (filter_tool);

  g_object_set (filter_tool->config,
                "range", range,
                NULL);
}

static void
color_balance_range_reset_callback (GtkWidget      *widget,
                                    GimpFilterTool *filter_tool)
{
  gimp_color_balance_config_reset_range (GIMP_COLOR_BALANCE_CONFIG (filter_tool->config));
}
