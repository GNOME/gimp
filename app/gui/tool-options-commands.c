/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "widgets/gimpeditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "tool-options-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   tool_options_save_callback      (GtkWidget   *widget,
                                               const gchar *name,
                                               gpointer     data);
static void   tool_options_reset_all_callback (GtkWidget   *widget,
                                               gboolean     reset_all,
                                               gpointer     data);


/*  public functions  */

void
tool_options_save_to_cmd_callback (GtkWidget *widget,
                                   gpointer   data,
                                   guint      action)
{
  GimpToolOptions *options = GIMP_TOOL_OPTIONS (data);

  gimp_config_copy_properties (G_OBJECT (options->tool_info->tool_options),
                               G_OBJECT (options));
}

void
tool_options_save_new_cmd_callback (GtkWidget *widget,
                                    gpointer   data,
                                    guint      action)
{
  GimpEditor   *editor;
  GimpContext  *context;
  GimpToolInfo *tool_info;
  GtkWidget    *qbox;

  editor = GIMP_EDITOR (data);

  context   = gimp_get_user_context (editor->item_factory->gimp);
  tool_info = gimp_context_get_tool (context);

  qbox = gimp_query_string_box (_("Save Tool Options"),
				gimp_standard_help_func,
				GIMP_HELP_TOOL_OPTIONS_DIALOG,
				_("Enter a name for the saved options"),
				_("Saved Options"),
				NULL, NULL,
				tool_options_save_callback, tool_info);
  gtk_widget_show (qbox);
}

void
tool_options_restore_from_cmd_callback (GtkWidget *widget,
                                        gpointer   data,
                                        guint      action)
{
  GimpToolOptions *options = GIMP_TOOL_OPTIONS (data);

  gimp_config_copy_properties (G_OBJECT (options),
                               G_OBJECT (options->tool_info->tool_options));
}

void
tool_options_delete_saved_cmd_callback (GtkWidget *widget,
                                        gpointer   data,
                                        guint      action)
{
  GimpToolOptions *options = GIMP_TOOL_OPTIONS (data);

  gimp_container_remove (options->tool_info->options_presets,
                         GIMP_OBJECT (options));
}

void
tool_options_reset_cmd_callback (GtkWidget *widget,
                                 gpointer   data,
                                 guint      action)
{
  GimpEditor   *editor;
  GimpContext  *context;
  GimpToolInfo *tool_info;

  editor = GIMP_EDITOR (data);

  context   = gimp_get_user_context (editor->item_factory->gimp);
  tool_info = gimp_context_get_tool (context);

  gimp_tool_options_reset (tool_info->tool_options);
}

void
tool_options_reset_all_cmd_callback (GtkWidget *widget,
                                     gpointer   data,
                                     guint      action)
{
  GimpEditor *editor;
  GtkWidget  *qbox;

  editor = GIMP_EDITOR (data);

  qbox = gimp_query_boolean_box (_("Reset Tool Options"),
                                 gimp_standard_help_func,
                                 GIMP_HELP_TOOL_OPTIONS_RESET,
                                 GTK_STOCK_DIALOG_QUESTION,
                                 _("Do you really want to reset all\n"
                                   "tool options to default values?"),
                                 GIMP_STOCK_RESET, GTK_STOCK_CANCEL,
                                 G_OBJECT (editor), "unmap",
                                 tool_options_reset_all_callback,
                                 editor->item_factory->gimp);
  gtk_widget_show (qbox);
}


/*  private functions  */

static void
tool_options_save_callback (GtkWidget   *widget,
                            const gchar *name,
                            gpointer     data)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (data);
  GObject      *copy;

  if (! name || ! strlen (name))
    name = _("Saved Options");

  copy = gimp_config_duplicate (G_OBJECT (tool_info->tool_options));

  gimp_object_set_name (GIMP_OBJECT (copy), name);
  gimp_list_uniquefy_name (GIMP_LIST (tool_info->options_presets),
                           GIMP_OBJECT (copy), TRUE);

  gimp_container_insert (tool_info->options_presets, GIMP_OBJECT (copy), -1);
  g_object_unref (copy);
}

static void
tool_options_reset_all_callback (GtkWidget *widget,
                                 gboolean   reset_all,
                                 gpointer   data)
{
  Gimp *gimp = GIMP (data);

  if (reset_all)
    {
      GList *list;

      for (list = GIMP_LIST (gimp->tool_info_list)->list;
           list;
           list = g_list_next (list))
        {
          GimpToolInfo *tool_info = list->data;

          gimp_tool_options_reset (tool_info->tool_options);
        }
    }
}
