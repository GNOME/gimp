/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaextensionlist.c
 * Copyright (C) 2018 Jehan <jehan@ligma.org>
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

#include "widgets-types.h"

#include "core/ligmaextension.h"
#include "core/ligmaextensionmanager.h"

#include "ligmaextensionlist.h"

#include "ligma-intl.h"

enum
{
  EXTENSION_ACTIVATED,
  LAST_SIGNAL
};

typedef enum
{
  LIGMA_EXT_LIST_USER,
  LIGMA_EXT_LIST_SYSTEM,
  LIGMA_EXT_LIST_SEARCH,
} LigmaExtensionListContents;

struct _LigmaExtensionListPrivate
{
  LigmaExtensionManager      *manager;

  LigmaExtensionListContents  contents;
};

static void ligma_extension_list_set            (LigmaExtensionList *list,
                                                const GList       *extensions,
                                                gboolean           is_system);

static void ligma_extension_list_ext_installed  (LigmaExtensionManager *manager,
                                                LigmaExtension        *extension,
                                                gboolean              is_system_ext,
                                                LigmaExtensionList    *list);
static void ligma_extension_list_ext_removed    (LigmaExtensionManager *manager,
                                                gchar                *extension_id,
                                                LigmaExtensionList    *list);

static void ligma_extension_list_delete_clicked (GtkButton            *delbutton,
                                                LigmaExtensionList    *list);
static void ligma_extension_switch_active       (GObject              *onoff,
                                                GParamSpec           *spec,
                                                gpointer              extension);
static void ligma_extension_row_activated       (GtkListBox           *box,
                                                GtkListBoxRow        *row,
                                                gpointer              user_data);

G_DEFINE_TYPE_WITH_PRIVATE (LigmaExtensionList, ligma_extension_list,
                            GTK_TYPE_LIST_BOX)

#define parent_class ligma_extension_list_parent_class

static guint signals[LAST_SIGNAL] = { 0, };

static void
ligma_extension_list_class_init (LigmaExtensionListClass *klass)
{
  signals[EXTENSION_ACTIVATED] =
    g_signal_new ("extension-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaExtensionListClass, extension_activated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_OBJECT);
}

static void
ligma_extension_list_init (LigmaExtensionList *list)
{
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list),
                                   GTK_SELECTION_SINGLE);
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (list),
                                             FALSE);
  list->p = ligma_extension_list_get_instance_private (list);
}

static void
ligma_extension_list_disable (GtkWidget *row,
                             gchar     *extension_id)
{
  LigmaExtension *extension;
  GtkWidget     *outframe = gtk_bin_get_child (GTK_BIN (row));

  extension = g_object_get_data (G_OBJECT (outframe), "extension");
  g_return_if_fail (extension);

  if (g_strcmp0 (ligma_object_get_name (extension), extension_id) == 0)
    {
      GtkWidget *button = gtk_frame_get_label_widget (GTK_FRAME (outframe));
      GtkWidget *image  = gtk_bin_get_child (GTK_BIN (button));

      gtk_widget_set_sensitive (gtk_bin_get_child (GTK_BIN (outframe)), FALSE);

      gtk_image_set_from_icon_name (GTK_IMAGE (image), LIGMA_ICON_EDIT_UNDO,
                                    GTK_ICON_SIZE_MENU);
      gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
    }
}

GtkWidget *
ligma_extension_list_new (LigmaExtensionManager *manager)
{
  LigmaExtensionList *list;

  g_return_val_if_fail (LIGMA_IS_EXTENSION_MANAGER (manager), NULL);

  list = g_object_new (LIGMA_TYPE_EXTENSION_LIST, NULL);
  list->p->manager  = manager;

  g_signal_connect (manager, "extension-installed",
                    G_CALLBACK (ligma_extension_list_ext_installed),
                    list);
  g_signal_connect (manager, "extension-removed",
                    G_CALLBACK (ligma_extension_list_ext_removed),
                    list);

  return GTK_WIDGET (list);
}

void
ligma_extension_list_show_system (LigmaExtensionList *list)
{
  list->p->contents = LIGMA_EXT_LIST_SYSTEM;
  ligma_extension_list_set (list,
                           ligma_extension_manager_get_system_extensions (list->p->manager),
                           TRUE);
}

void
ligma_extension_list_show_user (LigmaExtensionList *list)
{
  list->p->contents = LIGMA_EXT_LIST_USER;
  ligma_extension_list_set (list,
                           ligma_extension_manager_get_user_extensions (list->p->manager),
                           FALSE);
}

void
ligma_extension_list_show_search (LigmaExtensionList *list,
                                 const gchar       *search_terms)
{
  list->p->contents = LIGMA_EXT_LIST_SEARCH;
  /* TODO */
  ligma_extension_list_set (list, NULL, FALSE);
}

static void
ligma_extension_list_set (LigmaExtensionList *list,
                         const GList       *extensions,
                         gboolean           is_system)
{
  GList *iter = (GList *) extensions;

  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  for (; iter; iter = iter->next)
    {
      LigmaExtension *extension = iter->data;

      ligma_extension_list_ext_installed (list->p->manager, extension,
                                         is_system, list);
    }
  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback) gtk_list_box_row_set_activatable,
                         GUINT_TO_POINTER (TRUE));
  g_signal_connect (list, "row-activated",
                    G_CALLBACK (ligma_extension_row_activated), NULL);
}

static void
ligma_extension_list_ext_installed (LigmaExtensionManager *manager,
                                   LigmaExtension        *extension,
                                   gboolean              is_system_ext,
                                   LigmaExtensionList    *list)
{
  GList     *rows;
  GList     *iter;

  GtkWidget *outframe;
  GtkWidget *grid;
  GtkWidget *onoff;
  GtkWidget *image;

  if (list->p->contents == LIGMA_EXT_LIST_SEARCH                  ||
      (list->p->contents == LIGMA_EXT_LIST_USER && is_system_ext) ||
      (list->p->contents == LIGMA_EXT_LIST_SYSTEM && ! is_system_ext))
    return;

  /* Check if extension already listed (i.e. removed then reinstalled). */
  rows = gtk_container_get_children (GTK_CONTAINER (list));
  for (iter = rows; iter; iter = iter->next)
    {
      LigmaExtension *row_ext;
      GtkWidget     *outframe = gtk_bin_get_child (GTK_BIN (iter->data));

      row_ext = g_object_get_data (G_OBJECT (outframe), "extension");
      g_return_if_fail (row_ext);

      if (extension == row_ext)
        {
          GtkWidget *button = gtk_frame_get_label_widget (GTK_FRAME (outframe));

          image  = gtk_bin_get_child (GTK_BIN (button));
          gtk_widget_set_sensitive (gtk_bin_get_child (GTK_BIN (outframe)), TRUE);

          gtk_image_set_from_icon_name (GTK_IMAGE (image), LIGMA_ICON_EDIT_DELETE,
                                        GTK_ICON_SIZE_MENU);
          gtk_image_set_pixel_size (GTK_IMAGE (image), 12);

          g_list_free (rows);
          return;
        }
    }
  g_list_free (rows);

  outframe = gtk_frame_new (ligma_extension_get_name (extension));
  gtk_container_add (GTK_CONTAINER (list), outframe);
  g_object_set_data (G_OBJECT (outframe), "extension", extension);
  gtk_widget_show (outframe);

  grid = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), FALSE);
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), FALSE);
  gtk_container_add (GTK_CONTAINER (outframe), grid);
  gtk_widget_show (grid);

  /* On/Off switch. */
  onoff = gtk_switch_new ();
  gtk_widget_set_vexpand (onoff, FALSE);
  gtk_widget_set_hexpand (onoff, FALSE);
  gtk_widget_set_halign (onoff, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (onoff, GTK_ALIGN_CENTER);
  gtk_switch_set_active (GTK_SWITCH (onoff),
                         ligma_extension_manager_is_running (list->p->manager,
                                                            extension));
  gtk_widget_set_sensitive (onoff,
                            ligma_extension_manager_can_run (list->p->manager,
                                                            extension));
  g_signal_connect (onoff, "notify::active",
                    G_CALLBACK (ligma_extension_switch_active), extension);
  gtk_grid_attach (GTK_GRID (grid), onoff, 0, 0, 1, 1);
  gtk_widget_show (onoff);

  /* Short description. */
  if (ligma_extension_get_comment (extension))
    {
      GtkWidget     *desc = gtk_text_view_new ();
      GtkTextBuffer *buffer;

      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (desc));
      gtk_text_buffer_set_text (buffer,
                                ligma_extension_get_comment (extension),
                                -1);
      gtk_text_view_set_editable (GTK_TEXT_VIEW (desc), FALSE);
      gtk_widget_set_vexpand (desc, TRUE);
      gtk_widget_set_hexpand (desc, TRUE);
      gtk_grid_attach (GTK_GRID (grid), desc, 1, 0, 1, 1);
      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (desc),
                                   GTK_WRAP_WORD_CHAR);
      gtk_widget_show (desc);
    }

  /* Delete button. */
  if (! is_system_ext)
    {
      GtkWidget *delbutton;

      delbutton = gtk_button_new ();
      g_object_set_data (G_OBJECT (delbutton), "extension", extension);
      g_signal_connect (delbutton, "clicked",
                        G_CALLBACK (ligma_extension_list_delete_clicked),
                        list);
      gtk_button_set_relief (GTK_BUTTON (delbutton), GTK_RELIEF_NONE);
      image = gtk_image_new_from_icon_name (LIGMA_ICON_EDIT_DELETE,
                                            GTK_ICON_SIZE_MENU);
      gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
      gtk_widget_set_vexpand (delbutton, FALSE);
      gtk_widget_set_hexpand (delbutton, FALSE);
      gtk_widget_set_halign (delbutton, GTK_ALIGN_END);
      gtk_widget_set_valign (delbutton, GTK_ALIGN_START);
      gtk_container_add (GTK_CONTAINER (delbutton), image);
      gtk_widget_show (image);
      gtk_grid_attach (GTK_GRID (grid), delbutton, 2, 0, 1, 1);
      gtk_widget_show (delbutton);
    }
}

static void
ligma_extension_list_ext_removed (LigmaExtensionManager *manager,
                                 gchar                *extension_id,
                                 LigmaExtensionList    *list)
{
  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback) ligma_extension_list_disable,
                         extension_id);
}

static void
ligma_extension_list_delete_clicked (GtkButton         *delbutton,
                                    LigmaExtensionList *list)
{
  LigmaExtensionManager *manager = list->p->manager;
  LigmaExtension        *extension;
  GError               *error = NULL;

  extension = g_object_get_data (G_OBJECT (delbutton), "extension");
  g_return_if_fail (extension);

  if (ligma_extension_manager_is_removed (manager, extension))
    ligma_extension_manager_undo_remove (manager, extension, &error);
  else
    ligma_extension_manager_remove (manager, extension, &error);

  if (error)
    {
      g_warning ("%s: %s\n", G_STRFUNC, error->message);
      g_error_free (error);
    }
}

static void
ligma_extension_switch_active (GObject    *onoff,
                              GParamSpec *pspec,
                              gpointer    extension)
{
  LigmaExtension *ext = (LigmaExtension *) extension;

  if (gtk_switch_get_active (GTK_SWITCH (onoff)))
    {
      GError *error = NULL;

      ligma_extension_run (ext, &error);
      if (error)
        {
          g_signal_handlers_block_by_func (onoff,
                                           G_CALLBACK (ligma_extension_switch_active),
                                           extension);
          gtk_switch_set_active (GTK_SWITCH (onoff), FALSE);
          g_signal_handlers_unblock_by_func (onoff,
                                             G_CALLBACK (ligma_extension_switch_active),
                                             extension);
          g_printerr ("Extension '%s' failed to run: %s\n",
                      ligma_object_get_name (ext),
                      error->message);
          g_error_free (error);
        }
    }
  else
    ligma_extension_stop (ext);
}

static void
ligma_extension_row_activated (GtkListBox    *box,
                              GtkListBoxRow *row,
                              gpointer       user_data)
{
  GtkWidget *frame = gtk_bin_get_child (GTK_BIN (row));

  g_signal_emit (box, signals[EXTENSION_ACTIVATED], 0,
                 g_object_get_data (G_OBJECT (frame), "extension"));
}
