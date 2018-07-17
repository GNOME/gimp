/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpextensionlist.c
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#include "widgets-types.h"

#include "core/gimpextension.h"
#include "core/gimpextensionmanager.h"
#include "core/gimpmarshal.h"

#include "gimpextensionlist.h"

#include "gimp-intl.h"

enum
{
  EXTENSION_ACTIVATED,
  LAST_SIGNAL
};

struct _GimpExtensionListPrivate
{
  GimpExtensionManager *manager;
};

static void gimp_extension_list_set      (GimpExtensionList *list,
                                          const GList       *extensions,
                                          gboolean           is_system);
static void gimp_extension_row_activated (GtkListBox        *box,
                                          GtkListBoxRow     *row,
                                          gpointer           user_data);

G_DEFINE_TYPE (GimpExtensionList, gimp_extension_list, GTK_TYPE_LIST_BOX)

#define parent_class gimp_extension_list_parent_class

static guint signals[LAST_SIGNAL] = { 0, };

static void
gimp_extension_list_class_init (GimpExtensionListClass *klass)
{
  signals[EXTENSION_ACTIVATED] =
    g_signal_new ("extension-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpExtensionListClass, extension_activated),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_OBJECT);

  g_type_class_add_private (klass, sizeof (GimpExtensionListPrivate));
}

static void
gimp_extension_list_init (GimpExtensionList *list)
{
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list),
                                   GTK_SELECTION_SINGLE);
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (list),
                                             FALSE);
  list->p = G_TYPE_INSTANCE_GET_PRIVATE (list,
                                         GIMP_TYPE_EXTENSION_LIST,
                                         GimpExtensionListPrivate);
}

GtkWidget *
gimp_extension_list_new (GimpExtensionManager *manager)
{
  GimpExtensionList *list;

  g_return_val_if_fail (GIMP_IS_EXTENSION_MANAGER (manager), NULL);

  list = g_object_new (GIMP_TYPE_EXTENSION_LIST, NULL);
  list->p->manager  = manager;

  return GTK_WIDGET (list);
}

void
gimp_extension_list_show_system (GimpExtensionList *list)
{
  gimp_extension_list_set (list,
                           gimp_extension_manager_get_system_extensions (list->p->manager),
                           TRUE);
}

void
gimp_extension_list_show_user (GimpExtensionList *list)
{
  gimp_extension_list_set (list,
                           gimp_extension_manager_get_user_extensions (list->p->manager),
                           FALSE);
}

void
gimp_extension_list_show_search (GimpExtensionList *list,
                                 const gchar       *search_terms)
{
  /* TODO */
  gimp_extension_list_set (list, NULL, FALSE);
}

static void
gimp_extension_list_set (GimpExtensionList *list,
                         const GList       *extensions,
                         gboolean           is_system)
{
  GList *iter = (GList *) extensions;

  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  for (; iter; iter = iter->next)
    {
      GimpExtension *extension = iter->data;
      GtkWidget     *frame;
      GtkWidget     *hbox;
      GtkWidget     *onoff;

      frame = gtk_frame_new (gimp_extension_get_name (extension));
      gtk_container_add (GTK_CONTAINER (list), frame);
      g_object_set_data (G_OBJECT (gtk_widget_get_parent (frame)),
                         "extension", extension);
      gtk_widget_show (frame);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
      gtk_container_add (GTK_CONTAINER (frame), hbox);
      gtk_widget_show (hbox);

      if (gimp_extension_get_comment (extension))
        {
          GtkWidget     *desc = gtk_text_view_new ();
          GtkTextBuffer *buffer;

          buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (desc));
          gtk_text_buffer_set_text (buffer,
                                    gimp_extension_get_comment (extension),
                                    -1);
          gtk_text_view_set_editable (GTK_TEXT_VIEW (desc), FALSE);
          gtk_box_pack_start (GTK_BOX (hbox), desc, TRUE, TRUE, 1);
          gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (desc),
                                       GTK_WRAP_WORD_CHAR);
          gtk_widget_show (desc);
        }

      onoff = gtk_switch_new ();
      gtk_switch_set_active (GTK_SWITCH (onoff),
                             gimp_extension_manager_is_running (list->p->manager,
                                                                extension));
      gtk_widget_set_sensitive (onoff,
                                gimp_extension_manager_can_run (list->p->manager,
                                                                extension));
      gtk_box_pack_end (GTK_BOX (hbox), onoff, FALSE, FALSE, 1);
      gtk_widget_show (onoff);
    }
  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback) gtk_list_box_row_set_activatable,
                         GUINT_TO_POINTER (TRUE));
  g_signal_connect (list, "row-activated",
                    G_CALLBACK (gimp_extension_row_activated), NULL);
}

static void
gimp_extension_row_activated (GtkListBox    *box,
                              GtkListBoxRow *row,
                              gpointer       user_data)
{
  g_signal_emit (box, signals[EXTENSION_ACTIVATED], 0,
                 g_object_get_data (G_OBJECT (row),
                                    "extension"));
}
