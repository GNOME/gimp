/* GIMP - The GNU Image Manipulation Program
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
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"
#include "core/gimptoolpresets.h"

#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimphelp-ids.h"
#include "gimpmenufactory.h"
#include "gimppropwidgets.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"
#include "gimptooloptionseditor.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_tool_options_editor_docked_iface_init (GimpDockedInterface *iface);

static GObject * gimp_tool_options_editor_constructor  (GType                  type,
                                                        guint                  n_params,
                                                        GObjectConstructParam *params);

static void   gimp_tool_options_editor_destroy         (GtkObject             *object);

static GtkWidget *gimp_tool_options_editor_get_preview (GimpDocked            *docked,
                                                        GimpContext           *context,
                                                        GtkIconSize            size);
static gchar *gimp_tool_options_editor_get_title       (GimpDocked            *docked);

static void   gimp_tool_options_editor_save_clicked    (GtkWidget             *widget,
                                                        GimpToolOptionsEditor *editor);
static void   gimp_tool_options_editor_restore_clicked (GtkWidget             *widget,
                                                        GimpToolOptionsEditor *editor);
static void   gimp_tool_options_editor_delete_clicked  (GtkWidget             *widget,
                                                        GimpToolOptionsEditor *editor);
static void   gimp_tool_options_editor_drop_tool       (GtkWidget             *widget,
                                                        gint                   x,
                                                        gint                   y,
                                                        GimpViewable          *viewable,
                                                        gpointer               data);

static void   gimp_tool_options_editor_tool_changed    (GimpContext           *context,
                                                        GimpToolInfo          *tool_info,
                                                        GimpToolOptionsEditor *editor);

static void   gimp_tool_options_editor_presets_changed (GimpToolPresets       *presets,
                                                        GimpToolOptionsEditor *editor);
static void   gimp_tool_options_editor_presets_update  (GimpToolOptionsEditor *editor,
                                                        GimpToolPresets       *presets);
static void   gimp_tool_options_editor_save_presets    (GimpToolOptionsEditor *editor);


G_DEFINE_TYPE_WITH_CODE (GimpToolOptionsEditor, gimp_tool_options_editor,
                         GIMP_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_tool_options_editor_docked_iface_init))

#define parent_class gimp_tool_options_editor_parent_class


static void
gimp_tool_options_editor_class_init (GimpToolOptionsEditorClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);

  object_class->constructor = gimp_tool_options_editor_constructor;

  gtk_object_class->destroy = gimp_tool_options_editor_destroy;
}

static void
gimp_tool_options_editor_init (GimpToolOptionsEditor *editor)
{
  GtkWidget *sw;

  gtk_widget_set_size_request (GTK_WIDGET (editor), -1, 200);

  gimp_dnd_viewable_dest_add (GTK_WIDGET (editor),
                              GIMP_TYPE_TOOL_INFO,
                              gimp_tool_options_editor_drop_tool,
                              editor);

  editor->scrolled_window = sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (editor), sw);
  gtk_widget_show (sw);

  /*  The vbox containing the tool options  */
  editor->options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (editor->options_vbox), 2);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw),
                                         editor->options_vbox);
  gtk_widget_show (editor->options_vbox);

  editor->save_queue   = NULL;
  editor->save_idle_id = 0;
}

static void
gimp_tool_options_editor_docked_iface_init (GimpDockedInterface *docked_iface)
{
  docked_iface->get_preview = gimp_tool_options_editor_get_preview;
  docked_iface->get_title   = gimp_tool_options_editor_get_title;
}

static GObject *
gimp_tool_options_editor_constructor (GType                  type,
                                      guint                  n_params,
                                      GObjectConstructParam *params)
{
  GObject               *object;
  GimpToolOptionsEditor *editor;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_TOOL_OPTIONS_EDITOR (object);

  editor->save_button =
    gimp_editor_add_button (GIMP_EDITOR (editor), GTK_STOCK_SAVE,
                            _("Save options to..."),
                            GIMP_HELP_TOOL_OPTIONS_SAVE,
                            G_CALLBACK (gimp_tool_options_editor_save_clicked),
                            NULL,
                            editor);

  editor->restore_button =
    gimp_editor_add_button (GIMP_EDITOR (editor), GTK_STOCK_REVERT_TO_SAVED,
                            _("Restore options from..."),
                            GIMP_HELP_TOOL_OPTIONS_RESTORE,
                            G_CALLBACK (gimp_tool_options_editor_restore_clicked),
                            NULL,
                            editor);

  editor->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (editor), GTK_STOCK_DELETE,
                            _("Delete saved options..."),
                            GIMP_HELP_TOOL_OPTIONS_DELETE,
                            G_CALLBACK (gimp_tool_options_editor_delete_clicked),
                            NULL,
                            editor);

  editor->reset_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "tool-options",
                                   "tool-options-reset",
                                   "tool-options-reset-all",
                                   GDK_SHIFT_MASK,
                                   NULL);

  return object;
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

  gimp_tool_options_editor_save_presets (editor);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static GtkWidget *
gimp_tool_options_editor_get_preview (GimpDocked   *docked,
                                      GimpContext  *context,
                                      GtkIconSize   size)
{
  GtkSettings *settings = gtk_widget_get_settings (GTK_WIDGET (docked));
  GtkWidget   *view;
  gint         width;
  gint         height;

  gtk_icon_size_lookup_for_settings (settings, size, &width, &height);

  view = gimp_prop_view_new (G_OBJECT (context), "tool", context, height);
  GIMP_VIEW (view)->renderer->size = -1;
  gimp_view_renderer_set_size_full (GIMP_VIEW (view)->renderer,
                                    width, height, 0);

  return view;
}

static gchar *
gimp_tool_options_editor_get_title (GimpDocked *docked)
{
  GimpToolOptionsEditor *editor = GIMP_TOOL_OPTIONS_EDITOR (docked);
  GimpContext           *context;
  GimpToolInfo          *tool_info;

  context = gimp_get_user_context (editor->gimp);

  tool_info = gimp_context_get_tool (context);

  return tool_info ? g_strdup (tool_info->blurb) : NULL;
}


/*  public functions  */

GtkWidget *
gimp_tool_options_editor_new (Gimp            *gimp,
                              GimpMenuFactory *menu_factory)
{
  GimpToolOptionsEditor *editor;
  GimpContext           *user_context;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  editor = g_object_new (GIMP_TYPE_TOOL_OPTIONS_EDITOR,
                         "menu-factory",    menu_factory,
                         "menu-identifier", "<ToolOptions>",
                         "ui-path",         "/tool-options-popup",
                         NULL);

  editor->gimp = gimp;

  user_context = gimp_get_user_context (gimp);

  g_signal_connect_object (user_context, "tool-changed",
                           G_CALLBACK (gimp_tool_options_editor_tool_changed),
                           editor,
                           0);

  gimp_tool_options_editor_tool_changed (user_context,
                                         gimp_context_get_tool (user_context),
                                         editor);

  return GTK_WIDGET (editor);
}


/*  private functions  */

static void
gimp_tool_options_editor_menu_pos (GtkMenu  *menu,
                                   gint     *x,
                                   gint     *y,
                                   gpointer  data)
{
  gimp_button_menu_position (GTK_WIDGET (data), menu, GTK_POS_RIGHT, x, y);
}

static void
gimp_tool_options_editor_menu_popup (GimpToolOptionsEditor *editor,
                                     GtkWidget             *button,
                                     const gchar           *path)
{
  GimpEditor *gimp_editor = GIMP_EDITOR (editor);

  gtk_ui_manager_get_widget (GTK_UI_MANAGER (gimp_editor->ui_manager),
                             gimp_editor->ui_path);
  gimp_ui_manager_update (gimp_editor->ui_manager, gimp_editor->popup_data);

  gimp_ui_manager_ui_popup (gimp_editor->ui_manager, path,
                            button,
                            gimp_tool_options_editor_menu_pos, button,
                            NULL, NULL);
}

static void
gimp_tool_options_editor_save_clicked (GtkWidget             *widget,
                                       GimpToolOptionsEditor *editor)
{
  if (GTK_WIDGET_SENSITIVE (editor->restore_button) /* evil but correct */)
    {
      gimp_tool_options_editor_menu_popup (editor, widget,
                                           "/tool-options-popup/Save");
    }
  else
    {
      gimp_ui_manager_activate_action (GIMP_EDITOR (editor)->ui_manager,
                                       "tool-options",
                                       "tool-options-save-new");
    }
}

static void
gimp_tool_options_editor_restore_clicked (GtkWidget             *widget,
                                          GimpToolOptionsEditor *editor)
{
  gimp_tool_options_editor_menu_popup (editor, widget,
                                       "/tool-options-popup/Restore");
}

static void
gimp_tool_options_editor_delete_clicked (GtkWidget             *widget,
                                         GimpToolOptionsEditor *editor)
{
  gimp_tool_options_editor_menu_popup (editor, widget,
                                       "/tool-options-popup/Delete");
}

static void
gimp_tool_options_editor_drop_tool (GtkWidget    *widget,
                                    gint          x,
                                    gint          y,
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
  GimpToolPresets *presets;
  GtkWidget       *options_gui;

  if (tool_info && tool_info->tool_options == editor->visible_tool_options)
    return;

  if (editor->visible_tool_options)
    {
      presets = editor->visible_tool_options->tool_info->presets;

      if (presets)
        g_signal_handlers_disconnect_by_func (presets,
                                              gimp_tool_options_editor_presets_changed,
                                              editor);

      options_gui = g_object_get_data (G_OBJECT (editor->visible_tool_options),
                                       "gimp-tool-options-gui");

      if (options_gui)
        gtk_widget_hide (options_gui);

      editor->visible_tool_options = NULL;
    }

  if (tool_info && tool_info->tool_options)
    {
      presets = tool_info->presets;

      if (presets)
        g_signal_connect_object (presets, "changed",
                                 G_CALLBACK (gimp_tool_options_editor_presets_changed),
                                 G_OBJECT (editor), 0);

      options_gui = g_object_get_data (G_OBJECT (tool_info->tool_options),
                                       "gimp-tool-options-gui");

      if (! options_gui->parent)
        gtk_box_pack_start (GTK_BOX (editor->options_vbox), options_gui,
                            FALSE, FALSE, 0);

      gtk_widget_show (options_gui);

      editor->visible_tool_options = tool_info->tool_options;

      gimp_help_set_help_data (editor->scrolled_window, NULL,
                               tool_info->help_id);
    }
  else
    {
      presets = NULL;
    }

  gimp_tool_options_editor_presets_update (editor, presets);

  gimp_docked_title_changed (GIMP_DOCKED (editor));
}

static gboolean
gimp_tool_options_editor_save_presets_idle (GimpToolOptionsEditor *editor)
{
  editor->save_idle_id = 0;

  gimp_tool_options_editor_save_presets (editor);

  return FALSE;
}

static void
gimp_tool_options_editor_queue_save_presets (GimpToolOptionsEditor *editor,
                                             GimpToolPresets       *presets)
{
  if (g_list_find (editor->save_queue, presets))
    return;

  editor->save_queue = g_list_append (editor->save_queue, presets);

  if (! editor->save_idle_id)
    {
      editor->save_idle_id =
        g_idle_add ((GSourceFunc) gimp_tool_options_editor_save_presets_idle,
                    editor);
    }
}

static void
gimp_tool_options_editor_save_presets (GimpToolOptionsEditor *editor)
{
  GList *list;

  if (editor->save_idle_id)
    {
      g_source_remove (editor->save_idle_id);
      editor->save_idle_id = 0;
    }

  for (list = editor->save_queue; list; list = list->next)
    {
      GimpToolPresets *presets = list->data;
      GError          *error   = NULL;

      if (! gimp_tool_presets_save (presets, &error))
        {
          gimp_message (editor->gimp, G_OBJECT (editor), GIMP_MESSAGE_ERROR,
                        _("Error saving tool options presets: %s"),
                        error->message);
          g_error_free (error);
        }
    }

  g_list_free (editor->save_queue);

  editor->save_queue = NULL;
}

static void
gimp_tool_options_editor_presets_changed (GimpToolPresets       *presets,
                                          GimpToolOptionsEditor *editor)
{
  gimp_tool_options_editor_queue_save_presets (editor, presets);

  gimp_tool_options_editor_presets_update (editor, presets);
}

static void
gimp_tool_options_editor_presets_update (GimpToolOptionsEditor *editor,
                                         GimpToolPresets       *presets)
{
  gboolean save_sensitive    = FALSE;
  gboolean restore_sensitive = FALSE;
  gboolean delete_sensitive  = FALSE;
  gboolean reset_sensitive   = FALSE;

  if (presets)
    {
      save_sensitive  = TRUE;
      reset_sensitive = TRUE;

      if (! gimp_container_is_empty (GIMP_CONTAINER (presets)))
        {
          restore_sensitive = TRUE;
          delete_sensitive  = TRUE;
        }
    }

  gtk_widget_set_sensitive (editor->save_button,    save_sensitive);
  gtk_widget_set_sensitive (editor->restore_button, restore_sensitive);
  gtk_widget_set_sensitive (editor->delete_button,  delete_sensitive);
  gtk_widget_set_sensitive (editor->reset_button,   reset_sensitive);
}
