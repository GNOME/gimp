/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptooloptionseditor.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpmenufactory.h"
#include "gimptooloptionseditor.h"

#include "gimp-intl.h"


static void   gimp_tool_options_editor_class_init (GimpToolOptionsEditorClass *klass);
static void   gimp_tool_options_editor_init       (GimpToolOptionsEditor      *editor);

static void   gimp_tool_options_editor_destroy         (GtkObject             *object);

static void   gimp_tool_options_editor_save_clicked    (GtkWidget             *widget,
                                                        GimpToolOptionsEditor *editor);
static void   gimp_tool_options_editor_restore_clicked (GtkWidget             *widget,
                                                        GimpToolOptionsEditor *editor);
static void   gimp_tool_options_editor_reset_clicked   (GtkWidget             *widget,
                                                        GimpToolOptionsEditor *editor);

static void   gimp_tool_options_editor_drop_tool       (GtkWidget             *widget,
                                                        GimpViewable          *viewable,
                                                        gpointer               data);

static void   gimp_tool_options_editor_tool_changed    (GimpContext           *context,
                                                        GimpToolInfo          *tool_info,
                                                        GimpToolOptionsEditor *editor);


static GimpEditorClass *parent_class = NULL;


GType
gimp_tool_options_editor_get_type (void)
{
  static GType editor_type = 0;

  if (! editor_type)
    {
      static const GTypeInfo editor_info =
      {
        sizeof (GimpToolOptionsEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_tool_options_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_tool */
        sizeof (GimpToolOptionsEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_tool_options_editor_init,
      };

      editor_type = g_type_register_static (GIMP_TYPE_EDITOR,
                                            "GimpToolOptionsEditor",
                                            &editor_info, 0);
    }

  return editor_type;
}

static void
gimp_tool_options_editor_class_init (GimpToolOptionsEditorClass *klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = gimp_tool_options_editor_destroy;
}

static void
gimp_tool_options_editor_init (GimpToolOptionsEditor *editor)
{
  GtkWidget *scrolled_win;

  editor->save_button =
    gimp_editor_add_button (GIMP_EDITOR (editor), GTK_STOCK_SAVE,
                            _("Save current settings as default values"),
                            GIMP_HELP_TOOL_OPTIONS_SAVE,
                            G_CALLBACK (gimp_tool_options_editor_save_clicked),
                            NULL,
                            editor);

  editor->restore_button =
    gimp_editor_add_button (GIMP_EDITOR (editor), GTK_STOCK_REVERT_TO_SAVED,
                            _("Restore saved default values"),
                            GIMP_HELP_TOOL_OPTIONS_RESTORE,
                            G_CALLBACK (gimp_tool_options_editor_restore_clicked),
                            NULL,
                            editor);

  editor->reset_button =
    gimp_editor_add_button (GIMP_EDITOR (editor), GIMP_STOCK_RESET,
                            _("Reset to factory defaults"),
                            GIMP_HELP_TOOL_OPTIONS_RESET,
                            G_CALLBACK (gimp_tool_options_editor_reset_clicked),
                            NULL,
                            editor);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (editor), scrolled_win);
  gtk_widget_show (scrolled_win);

  /*  The vbox containing the tool options  */
  editor->options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (editor->options_vbox), 2);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
                                         editor->options_vbox);
  gtk_widget_show (editor->options_vbox);

  /*  dnd stuff  */
  gtk_drag_dest_set (GTK_WIDGET (editor),
                     GTK_DEST_DEFAULT_ALL,
                     NULL, 0,
                     GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_add (GTK_WIDGET (editor),
                              GIMP_TYPE_TOOL_INFO,
                              gimp_tool_options_editor_drop_tool,
                              editor);
}

static void
gimp_tool_options_editor_destroy (GtkObject *object)
{
  GimpToolOptionsEditor *editor = GIMP_TOOL_OPTIONS_EDITOR (object);

  if (editor->options_vbox)
    {
      GList *options;
      GList *list;

      options =
        gtk_container_get_children (GTK_CONTAINER (editor->options_vbox));

      for (list = options; list; list = g_list_next (list))
        {
          g_object_ref (list->data);
          gtk_container_remove (GTK_CONTAINER (editor->options_vbox),
                                GTK_WIDGET (list->data));
        }

      g_list_free (options);
      editor->options_vbox = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_tool_options_editor_new (Gimp            *gimp,
                              GimpMenuFactory *menu_factory)
{
  GimpToolOptionsEditor *editor;
  GimpContext           *user_context;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  user_context = gimp_get_user_context (gimp);

  editor = g_object_new (GIMP_TYPE_TOOL_OPTIONS_EDITOR, NULL);

  editor->gimp = gimp;

  gtk_widget_set_size_request (GTK_WIDGET (editor), -1, 200);

  gimp_editor_create_menu (GIMP_EDITOR (editor),
                           menu_factory, "<ToolOptions>",
                           editor);

  g_signal_connect_object (user_context, "tool_changed",
                           G_CALLBACK (gimp_tool_options_editor_tool_changed),
                           editor,
                           0);

  gimp_tool_options_editor_tool_changed (user_context,
                                         gimp_context_get_tool (user_context),
                                         editor);

  return GTK_WIDGET (editor);
}

static void
gimp_tool_options_editor_save_clicked (GtkWidget             *widget,
                                       GimpToolOptionsEditor *editor)
{
  GimpToolInfo *tool_info;

  tool_info = gimp_context_get_tool (gimp_get_user_context (editor->gimp));

  if (tool_info)
    {
      GError *error = NULL;

      if (! gimp_tool_options_serialize (tool_info->tool_options, "user",
                                         &error))
        {
          g_message ("EEK: %s\n", error->message);
          g_clear_error (&error);
        }
    }
}

static void
gimp_tool_options_editor_restore_clicked (GtkWidget             *widget,
                                          GimpToolOptionsEditor *editor)
{
  GimpToolInfo *tool_info;

  tool_info = gimp_context_get_tool (gimp_get_user_context (editor->gimp));

  if (tool_info)
    {
      /*  Need to reset the tool-options since only the changes
       *  from the default values are written to disk.
       */
      g_object_freeze_notify (G_OBJECT (tool_info->tool_options));

      gimp_tool_options_reset (tool_info->tool_options);
      gimp_tool_options_deserialize (tool_info->tool_options, "user", NULL);

      g_object_thaw_notify (G_OBJECT (tool_info->tool_options));
    }
}

static void
gimp_tool_options_editor_reset_clicked (GtkWidget             *widget,
                                        GimpToolOptionsEditor *editor)
{
  GimpToolInfo *tool_info;

  tool_info = gimp_context_get_tool (gimp_get_user_context (editor->gimp));

  if (tool_info)
    gimp_tool_options_reset (tool_info->tool_options);
}

static void
gimp_tool_options_editor_drop_tool (GtkWidget    *widget,
                                    GimpViewable *viewable,
                                    gpointer      data)
{
  GimpToolOptionsEditor *editor = GIMP_TOOL_OPTIONS_EDITOR (data);
  GimpContext           *context;

  context = gimp_get_user_context (editor->gimp);

  gimp_context_set_tool (context, GIMP_TOOL_INFO (viewable));
}

static void
gimp_tool_options_editor_tool_changed (GimpContext           *context,
                                       GimpToolInfo          *tool_info,
                                       GimpToolOptionsEditor *editor)
{
  GtkWidget *options_gui;
  gboolean   sensitive;

  if (editor->visible_tool_options &&
      (! tool_info || tool_info->tool_options != editor->visible_tool_options))
    {
      options_gui = g_object_get_data (G_OBJECT (editor->visible_tool_options),
                                       "gimp-tool-options-gui");

      if (options_gui)
        gtk_widget_hide (options_gui);

      editor->visible_tool_options = NULL;
    }

  if (tool_info && tool_info->tool_options)
    {
      options_gui = g_object_get_data (G_OBJECT (tool_info->tool_options),
                                       "gimp-tool-options-gui");

      if (! options_gui->parent)
        gtk_box_pack_start (GTK_BOX (editor->options_vbox), options_gui,
                            FALSE, FALSE, 0);

      gtk_widget_show (options_gui);

      editor->visible_tool_options = tool_info->tool_options;
    }

  sensitive = (editor->visible_tool_options &&
               G_TYPE_FROM_INSTANCE (editor->visible_tool_options) !=
               GIMP_TYPE_TOOL_OPTIONS);

  gtk_widget_set_sensitive (editor->save_button,    sensitive);
  gtk_widget_set_sensitive (editor->restore_button, sensitive);
  gtk_widget_set_sensitive (editor->reset_button,   sensitive);
}
