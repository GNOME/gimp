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
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimphelp-ids.h"
#include "gimpitemfactory.h"
#include "gimpmenufactory.h"
#include "gimppreview.h"
#include "gimppreviewrenderer.h"
#include "gimppropwidgets.h"
#include "gimptooloptionseditor.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_tool_options_editor_class_init        (GimpToolOptionsEditorClass *klass);
static void   gimp_tool_options_editor_init              (GimpToolOptionsEditor      *editor);
static void   gimp_tool_options_editor_docked_iface_init (GimpDockedInterface        *docked_iface);

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
static void   gimp_tool_options_editor_reset_clicked   (GtkWidget             *widget,
                                                        GimpToolOptionsEditor *editor);
static void gimp_tool_options_editor_reset_ext_clicked (GtkWidget             *widget,
                                                        GdkModifierType        state,
                                                        GimpToolOptionsEditor *editor);

static void   gimp_tool_options_editor_drop_tool       (GtkWidget             *widget,
                                                        GimpViewable          *viewable,
                                                        gpointer               data);

static void   gimp_tool_options_editor_tool_changed    (GimpContext           *context,
                                                        GimpToolInfo          *tool_info,
                                                        GimpToolOptionsEditor *editor);

static void   gimp_tool_options_editor_presets_changed (GimpContainer         *container,
                                                        GimpToolOptions       *options,
                                                        GimpToolOptionsEditor *editor);


static GimpEditorClass *parent_class = NULL;


GType
gimp_tool_options_editor_get_type (void)
{
  static GType type = 0;

  if (! type)
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
      static const GInterfaceInfo docked_iface_info =
      {
        (GInterfaceInitFunc) gimp_tool_options_editor_docked_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      type = g_type_register_static (GIMP_TYPE_EDITOR,
                                     "GimpToolOptionsEditor",
                                     &editor_info, 0);

      g_type_add_interface_static (type, GIMP_TYPE_DOCKED,
                                   &docked_iface_info);
    }

  return type;
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
  gchar     *str;

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

  str = g_strdup_printf (_("Reset to default values\n"
                           "%s  Reset all Tool Options"),
                         gimp_get_mod_name_shift ());
  editor->reset_button =
    gimp_editor_add_button (GIMP_EDITOR (editor), GIMP_STOCK_RESET,
                            str,
                            GIMP_HELP_TOOL_OPTIONS_RESET,
                            G_CALLBACK (gimp_tool_options_editor_reset_clicked),
                            G_CALLBACK (gimp_tool_options_editor_reset_ext_clicked),
                            editor);
  g_free (str);

  editor->scrolled_window = scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
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
  gimp_dnd_viewable_dest_add (GTK_WIDGET (editor),
                              GIMP_TYPE_TOOL_INFO,
                              gimp_tool_options_editor_drop_tool,
                              editor);
}

static void
gimp_tool_options_editor_docked_iface_init (GimpDockedInterface *docked_iface)
{
  docked_iface->get_preview = gimp_tool_options_editor_get_preview;
  docked_iface->get_title   = gimp_tool_options_editor_get_title;
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

static GtkWidget *
gimp_tool_options_editor_get_preview (GimpDocked   *docked,
                                      GimpContext  *context,
                                      GtkIconSize   size)
{
  GdkScreen *screen;
  GtkWidget *preview;
  gint       width;
  gint       height;

  screen = gtk_widget_get_screen (GTK_WIDGET (docked));
  gtk_icon_size_lookup_for_settings (gtk_settings_get_for_screen (screen),
                                     size, &width, &height);

  preview = gimp_prop_preview_new (G_OBJECT (context), "tool", height);
  GIMP_PREVIEW (preview)->renderer->size = -1;
  gimp_preview_renderer_set_size_full (GIMP_PREVIEW (preview)->renderer,
                                       width, height, 0);

  return preview;
}

static gchar *
gimp_tool_options_editor_get_title (GimpDocked *docked)
{
  GimpToolOptionsEditor *editor = GIMP_TOOL_OPTIONS_EDITOR (docked);
  GimpContext           *context;
  GimpToolInfo          *tool_info;

  context = gimp_get_user_context (editor->gimp);

  tool_info = gimp_context_get_tool (context);

  return g_strdup_printf (_("%s Options"), tool_info->blurb);
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

  gimp_editor_create_menu (GIMP_EDITOR (editor), menu_factory,
                           "<ToolOptions>", "/tool-options-popup",
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
gimp_tool_options_editor_menu_pos (GtkMenu  *menu,
                                   gint     *x,
                                   gint     *y,
                                   gboolean *push_in,
                                   gpointer  func_data)
{
  gimp_button_menu_position (GTK_WIDGET (func_data), menu, GTK_POS_RIGHT, x, y);
}

static void
gimp_tool_options_editor_menu_popup (GimpToolOptionsEditor *editor,
                                     GtkWidget             *button,
                                     const gchar           *path)
{
  GtkItemFactory *item_factory;
  GtkWidget      *menu;

  item_factory = GTK_ITEM_FACTORY (GIMP_EDITOR (editor)->item_factory);

  gimp_item_factory_update (GIMP_EDITOR (editor)->item_factory,
                            GIMP_EDITOR (editor)->item_factory_data);

  menu = gtk_item_factory_get_widget (item_factory, path);

  if (menu)
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
                    gimp_tool_options_editor_menu_pos, button,
                    0, GDK_CURRENT_TIME);
}

static void
gimp_tool_options_editor_save_clicked (GtkWidget             *widget,
                                       GimpToolOptionsEditor *editor)
{
  if (GTK_WIDGET_SENSITIVE (editor->restore_button) /* evil but correct */)
    {
      gimp_tool_options_editor_menu_popup (editor, widget, "/Save Options to");
    }
  else
    {
      GtkItemFactory *item_factory;
      GtkWidget      *item;

      item_factory = GTK_ITEM_FACTORY (GIMP_EDITOR (editor)->item_factory);

      item = gtk_item_factory_get_widget (item_factory,
                                          "/Save Options to/New Entry...");

      if (item)
        gtk_widget_activate (item);
    }
}

static void
gimp_tool_options_editor_restore_clicked (GtkWidget             *widget,
                                          GimpToolOptionsEditor *editor)
{
  gimp_tool_options_editor_menu_popup (editor, widget, "/Restore Options from");
}

static void
gimp_tool_options_editor_delete_clicked (GtkWidget             *widget,
                                         GimpToolOptionsEditor *editor)
{
  gimp_tool_options_editor_menu_popup (editor, widget, "/Delete Saved Options");
}

static void
gimp_tool_options_editor_reset_clicked (GtkWidget             *widget,
                                        GimpToolOptionsEditor *editor)
{
  gimp_tool_options_editor_reset_ext_clicked (widget, 0, editor);
}

static void
gimp_tool_options_editor_reset_all_callback (GtkWidget *widget,
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

static void
gimp_tool_options_editor_reset_ext_clicked (GtkWidget             *widget,
                                            GdkModifierType        state,
                                            GimpToolOptionsEditor *editor)
{
  if (state & GDK_SHIFT_MASK)
    {
      GtkWidget *qbox;

      qbox = gimp_query_boolean_box (_("Reset Tool Options"),
                                     GTK_WIDGET (editor),
                                     gimp_standard_help_func,
                                     GIMP_HELP_TOOL_OPTIONS_RESET,
                                     GTK_STOCK_DIALOG_QUESTION,
                                     _("Do you really want to reset all "
                                       "tool options to default values?"),
                                     GIMP_STOCK_RESET, GTK_STOCK_CANCEL,
                                     G_OBJECT (editor), "unmap",
                                     gimp_tool_options_editor_reset_all_callback,
                                     editor->gimp);
      gtk_widget_show (qbox);
    }
  else
    {
      GimpToolInfo *tool_info;

      tool_info = gimp_context_get_tool (gimp_get_user_context (editor->gimp));

      if (tool_info)
        gimp_tool_options_reset (tool_info->tool_options);
    }
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
  GimpContainer *presets;
  GtkWidget     *options_gui;

  if (tool_info && tool_info->tool_options == editor->visible_tool_options)
    return;

  if (editor->visible_tool_options)
    {
      presets = editor->visible_tool_options->tool_info->options_presets;

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
      presets = tool_info->options_presets;

      if (presets)
        {
          g_signal_connect_object (presets, "add",
                                   G_CALLBACK (gimp_tool_options_editor_presets_changed),
                                   G_OBJECT (editor), 0);
          g_signal_connect_object (presets, "remove",
                                   G_CALLBACK (gimp_tool_options_editor_presets_changed),
                                   G_OBJECT (editor), 0);
        }

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

  gimp_tool_options_editor_presets_changed (presets, NULL, editor);

  gimp_docked_title_changed (GIMP_DOCKED (editor));
}

static void
gimp_tool_options_editor_presets_changed (GimpContainer         *container,
                                          GimpToolOptions       *options,
                                          GimpToolOptionsEditor *editor)
{
  gboolean save_sensitive    = FALSE;
  gboolean restore_sensitive = FALSE;
  gboolean delete_sensitive  = FALSE;
  gboolean reset_sensitive   = FALSE;

  if (container)
    {
      save_sensitive  = TRUE;
      reset_sensitive = TRUE;

      if (gimp_container_num_children (container))
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
