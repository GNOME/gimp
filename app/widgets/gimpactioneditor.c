/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactioneditor.c
 * Copyright (C) 2008  Michael Natterer <mitch@gimp.org>
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

#include "gimpactioneditor.h"
#include "gimpactionview.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_action_editor_filter_clear   (GtkButton        *button,
                                                 GtkEntry         *entry);
static void   gimp_action_editor_filter_changed (GtkEntry         *entry,
                                                 GimpActionEditor *editor);


G_DEFINE_TYPE (GimpActionEditor, gimp_action_editor, GTK_TYPE_VBOX)

#define parent_class gimp_action_editor_parent_class


static void
gimp_action_editor_class_init (GimpActionEditorClass *klass)
{
}

static void
gimp_action_editor_init (GimpActionEditor *editor)
{
  GtkWidget *hbox;
  GtkWidget *entrybox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *image;

  gtk_box_set_spacing (GTK_BOX (editor), 12);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Search:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entrybox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), entrybox, TRUE, TRUE, 0);
  gtk_widget_show (entrybox);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (entrybox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  button = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (entrybox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_action_editor_filter_clear),
                    entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_action_editor_filter_changed),
                    editor);
}

GtkWidget *
gimp_action_editor_new (GimpUIManager *manager,
                        const gchar   *select_action,
                        gboolean       show_shortcuts)
{
  GimpActionEditor *editor;
  GtkWidget        *scrolled_window;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);

  editor = g_object_new (GIMP_TYPE_ACTION_EDITOR, NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  editor->view = gimp_action_view_new (manager, select_action, show_shortcuts);
  gtk_widget_set_size_request (editor->view, 300, 400);
  gtk_container_add (GTK_CONTAINER (scrolled_window), editor->view);
  gtk_widget_show (editor->view);

  return GTK_WIDGET (editor);
}


/*  private functions  */

static void
gimp_action_editor_filter_clear (GtkButton *button,
                                 GtkEntry  *entry)
{
  gtk_entry_set_text (entry, "");
}

static void
gimp_action_editor_filter_changed (GtkEntry         *entry,
                                   GimpActionEditor *editor)
{
  gimp_action_view_set_filter (GIMP_ACTION_VIEW (editor->view),
                               gtk_entry_get_text (entry));
}

