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

#include <string.h>

#include <gegl.h>
#include <gegl-plugin.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimp-filter.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpfilteroptions.h"
#include "gimpgegltool.h"

#include "gimp-intl.h"


enum
{
  COLUMN_NAME,
  COLUMN_LABEL,
  COLUMN_ICON_NAME,
  N_COLUMNS
};


/*  local function prototypes  */

static void    gimp_gegl_tool_control           (GimpTool       *tool,
                                                 GimpToolAction  action,
                                                 GimpDisplay    *display);

static void    gimp_gegl_tool_dialog            (GimpFilterTool *filter_tool);

static void    gimp_gegl_tool_halt              (GimpGeglTool   *gegl_tool);

static void    gimp_gegl_tool_operation_changed (GtkWidget      *widget,
                                                 GimpGeglTool   *gegl_tool);


G_DEFINE_TYPE (GimpGeglTool, gimp_gegl_tool, GIMP_TYPE_OPERATION_TOOL)

#define parent_class gimp_gegl_tool_parent_class


void
gimp_gegl_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  Gimp  *gimp = GIMP (data);
  GList *opclasses;
  GList *iter;

  (* callback) (GIMP_TYPE_GEGL_TOOL,
                GIMP_TYPE_FILTER_OPTIONS,
                gimp_color_options_gui,
                0,
                "gimp-gegl-tool",
                _("GEGL Operation"),
                _("Run an arbitrary GEGL operation"),
                N_("_GEGL Operation..."), NULL,
                NULL, GIMP_HELP_TOOL_GEGL,
                GIMP_ICON_GEGL,
                data);

  /* We generate hidden tools for every non-ignored GEGL operation, allowing
   * people to easily access their custom GEGL operations.
   */
  opclasses = gimp_gegl_get_op_classes ();

  for (iter = opclasses; iter; iter = iter->next)
    {
      GeglOperationClass *opclass   = GEGL_OPERATION_CLASS (iter->data);
      const gchar        *icon_name = NULL;
      const gchar        *op_name   = opclass->name;
      const gchar        *title;
      const gchar        *desc;
      gchar              *formatted_op_name;
      gchar              *action_name;
      gchar              *identifier;
      gchar              *label;
      gint                i = 2;

      formatted_op_name = g_strdup (opclass->name);
      gimp_make_valid_action_name (formatted_op_name);
      action_name = g_strdup_printf ("tools-gegl-op-%s", formatted_op_name);

      while (g_action_group_has_action (G_ACTION_GROUP (gimp->app), action_name))
        {
          /* In the off-chance that after formatting to a valid action name, 2
           * operations end up generating the same action name.
           */
          g_free (action_name);
          action_name = g_strdup_printf ("tools-gegl-op-%s-%d", formatted_op_name, i++);
        }
      g_free (formatted_op_name);

      /* gimp_tool_info_get_action_name() expects a name starting with "gimp-"
       * and ending with "-tool".
       */
      identifier = g_strdup_printf ("gimp-%s-tool", action_name + strlen ("tools-"));
      gimp_filter_gegl_ops_add (gimp, action_name, op_name);
      g_free (action_name);

      if (g_str_has_prefix (op_name, "gegl:"))
        icon_name = GIMP_ICON_GEGL;

      if (g_str_has_prefix (op_name, "gegl:"))
        op_name += strlen ("gegl:");

      title = gegl_operation_class_get_key (opclass, "title");
      desc  = gegl_operation_class_get_key (opclass, "description");

      if (title)
        label = g_strdup_printf ("%s (%s)", title, op_name);
      else
        label = g_strdup (op_name);

      (* callback) (GIMP_TYPE_GEGL_TOOL,
                    GIMP_TYPE_FILTER_OPTIONS,
                    gimp_color_options_gui,
                    0,
                    identifier,
                    label, desc ? desc : label,
                    label, NULL,
                    NULL, GIMP_HELP_TOOL_GEGL,
                    icon_name,
                    data);

      g_free (label);
      g_free (identifier);
    }

  g_list_free (opclasses);
}

static void
gimp_gegl_tool_class_init (GimpGeglToolClass *klass)
{
  GimpToolClass       *tool_class        = GIMP_TOOL_CLASS (klass);
  GimpFilterToolClass *filter_tool_class = GIMP_FILTER_TOOL_CLASS (klass);

  tool_class->control       = gimp_gegl_tool_control;

  filter_tool_class->dialog = gimp_gegl_tool_dialog;
}

static void
gimp_gegl_tool_init (GimpGeglTool *tool)
{
}

static void
gimp_gegl_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  GimpGeglTool *gegl_tool = GIMP_GEGL_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_gegl_tool_halt (gegl_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}


/*****************/
/*  Gegl dialog  */
/*****************/

static void
gimp_gegl_tool_dialog (GimpFilterTool *filter_tool)
{
  GimpGeglTool      *tool      = GIMP_GEGL_TOOL (filter_tool);
  GimpOperationTool *o_tool    = GIMP_OPERATION_TOOL (filter_tool);
  GimpToolInfo      *tool_info = GIMP_TOOL (tool)->tool_info;
  GtkListStore      *store     = NULL;
  GtkCellRenderer   *cell;
  GtkWidget         *main_vbox;
  GtkWidget         *hbox;
  GtkWidget         *combo;
  GtkWidget         *options_gui;
  GtkWidget         *options_box;
  GList             *opclasses;
  GList             *iter;
  gchar             *action_name;
  const gchar       *show_op_name;

  action_name  = gimp_tool_info_get_action_name (tool_info);
  show_op_name = gimp_filter_gegl_ops_get (tool_info->gimp, action_name);
  g_free (action_name);

  GIMP_FILTER_TOOL_CLASS (parent_class)->dialog (filter_tool);

  options_box = g_weak_ref_get (&o_tool->options_box_ref);
  g_return_if_fail (options_box);

  main_vbox = gimp_filter_tool_dialog_get_vbox (filter_tool);

  /*  The operation combo box  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_vbox), hbox, 0);
  gtk_widget_show (hbox);

  if (show_op_name == NULL)
    store = gtk_list_store_new (N_COLUMNS,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  opclasses = gimp_gegl_get_op_classes ();

  for (iter = opclasses; iter; iter = iter->next)
    {
      GeglOperationClass *opclass   = GEGL_OPERATION_CLASS (iter->data);
      const gchar        *icon_name = NULL;
      const gchar        *op_name   = opclass->name;
      const gchar        *title;
      gchar              *label;

      if (show_op_name != NULL && g_strcmp0 (show_op_name, op_name) != 0)
        continue;

      if (g_str_has_prefix (opclass->name, "gegl:"))
        icon_name = GIMP_ICON_GEGL;

      if (g_str_has_prefix (op_name, "gegl:"))
        op_name += strlen ("gegl:");

      title = gegl_operation_class_get_key (opclass, "title");

      if (title)
        label = g_strdup_printf ("%s (%s)", title, op_name);
      else
        label = g_strdup (op_name);

      if (store != NULL)
        gtk_list_store_insert_with_values (store, NULL, -1,
                                           COLUMN_NAME,      opclass->name,
                                           COLUMN_LABEL,     label,
                                           COLUMN_ICON_NAME, icon_name,
                                           -1);
      else
        gimp_operation_tool_set_operation (GIMP_OPERATION_TOOL (tool),
                                           opclass->name,
                                           label,
                                           gegl_operation_class_get_key (opclass, "description"),
                                           NULL,
                                           icon_name,
                                           GIMP_HELP_TOOL_GEGL);

      g_free (label);

      if (show_op_name != NULL)
          break;
    }
  g_list_free (opclasses);

  /* We either show the full combo or we found a specific operation. */
  g_return_if_fail (store != NULL || iter != NULL);

  if (store != NULL)
    {
      combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
      g_object_unref (store);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      cell = gtk_cell_renderer_pixbuf_new ();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, FALSE);
      gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell,
                                     "icon-name", COLUMN_ICON_NAME);

      cell = gtk_cell_renderer_text_new ();
      g_object_set (cell,
                    "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                    NULL);
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
      gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell,
                                     "text", COLUMN_LABEL);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_gegl_tool_operation_changed),
                        tool);

      tool->operation_combo = combo;
    }

  tool->description_label = gtk_label_new ("");
  gtk_label_set_line_wrap (GTK_LABEL (tool->description_label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (tool->description_label), 0.0);
  gtk_box_pack_start (GTK_BOX (main_vbox), tool->description_label,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_vbox), tool->description_label, 1);

  /*  The options vbox  */
  options_gui = gtk_label_new (_("Select an operation from the list above"));
  gimp_label_set_attributes (GTK_LABEL (options_gui),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  g_object_set (options_gui,
                "margin-top",    4,
                "margin-bottom", 4,
                NULL);
  gtk_container_add (GTK_CONTAINER (options_box), options_gui);
  g_object_unref (options_box);
  g_weak_ref_set (&o_tool->options_gui_ref, options_gui);
  gtk_widget_show (options_gui);
}

static void
gimp_gegl_tool_halt (GimpGeglTool *gegl_tool)
{
  GimpOperationTool *op_tool = GIMP_OPERATION_TOOL (gegl_tool);

  gimp_operation_tool_set_operation (op_tool, NULL,
                                     NULL, NULL, NULL, NULL, NULL);
}

static void
gimp_gegl_tool_operation_changed (GtkWidget    *widget,
                                  GimpGeglTool *tool)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *operation;

  if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    return;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

  gtk_tree_model_get (model, &iter,
                      COLUMN_NAME, &operation,
                      -1);

  if (operation)
    {
      const gchar *description;

      description = gegl_operation_get_key (operation, "description");

      if (description)
        {
          gtk_label_set_text (GTK_LABEL (tool->description_label), description);
          gtk_widget_show (tool->description_label);
        }
      else
        {
          gtk_widget_hide (tool->description_label);
        }

      gimp_operation_tool_set_operation (GIMP_OPERATION_TOOL (tool),
                                         operation,
                                         _("GEGL Operation"),
                                         _("GEGL Operation"),
                                         NULL,
                                         GIMP_ICON_GEGL,
                                         GIMP_HELP_TOOL_GEGL);
      g_free (operation);
    }
}
