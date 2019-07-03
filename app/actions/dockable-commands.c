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
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpcontainerview-utils.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimpsessioninfo.h"

#include "dockable-commands.h"


static GimpDockable * dockable_get_current (GimpDockbook *dockbook);


/*  public functions  */

void
dockable_add_tab_cmd_callback (GimpAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);

  gimp_dockbook_add_from_dialog_factory (dockbook,
                                         g_variant_get_string (value, NULL),
                                         -1);
}

void
dockable_close_tab_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    {
      g_object_ref (dockable);
      gimp_dockbook_remove (dockbook, dockable);
      gtk_widget_destroy (GTK_WIDGET (dockable));
      g_object_unref (dockable);
    }
}

void
dockable_detach_tab_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    gimp_dockable_detach (dockable);
}

void
dockable_lock_tab_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    {
      gboolean lock = g_variant_get_boolean (value);

      gimp_dockable_set_locked (dockable, lock);
    }
}

void
dockable_toggle_view_cmd_callback (GimpAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  GimpViewType  view_type;
  gint          page_num;

  view_type = (GimpViewType) g_variant_get_int32 (value);

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (GimpDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable)
    {
      GimpDialogFactoryEntry *entry;

      gimp_dialog_factory_from_widget (GTK_WIDGET (dockable), &entry);

      if (entry)
        {
          gchar *identifier;
          gchar *substring = NULL;

          identifier = g_strdup (entry->identifier);

          substring = strstr (identifier, "grid");

          if (substring && view_type == GIMP_VIEW_TYPE_GRID)
            {
              g_free (identifier);
              return;
            }

          if (! substring)
            {
              substring = strstr (identifier, "list");

              if (substring && view_type == GIMP_VIEW_TYPE_LIST)
                {
                  g_free (identifier);
                  return;
                }
            }

          if (substring)
            {
              GimpContainerView *old_view;
              GtkWidget         *new_dockable;
              GimpDock          *dock;
              gint               view_size = -1;

              if (view_type == GIMP_VIEW_TYPE_LIST)
                memcpy (substring, "list", 4);
              else if (view_type == GIMP_VIEW_TYPE_GRID)
                memcpy (substring, "grid", 4);

              old_view = gimp_container_view_get_by_dockable (dockable);

              if (old_view)
                view_size = gimp_container_view_get_view_size (old_view, NULL);

              dock         = gimp_dockbook_get_dock (dockbook);
              new_dockable = gimp_dialog_factory_dockable_new (gimp_dock_get_dialog_factory (dock),
                                                               dock,
                                                               identifier,
                                                               view_size);

              if (new_dockable)
                {
                  GimpDocked *old;
                  GimpDocked *new;
                  gboolean    show;

                  gimp_dockable_set_locked (GIMP_DOCKABLE (new_dockable),
                                            gimp_dockable_is_locked (dockable));

                  old = GIMP_DOCKED (gtk_bin_get_child (GTK_BIN (dockable)));
                  new = GIMP_DOCKED (gtk_bin_get_child (GTK_BIN (new_dockable)));

                  show = gimp_docked_get_show_button_bar (old);
                  gimp_docked_set_show_button_bar (new, show);

                  /*  Maybe gimp_dialog_factory_dockable_new() returned
                   *  an already existing singleton dockable, so check
                   *  if it already is attached to a dockbook.
                   */
                  if (! gimp_dockable_get_dockbook (GIMP_DOCKABLE (new_dockable)))
                    {
                      gimp_dockbook_add (dockbook, GIMP_DOCKABLE (new_dockable),
                                         page_num);

                      g_object_ref (dockable);
                      gimp_dockbook_remove (dockbook, dockable);
                      gtk_widget_destroy (GTK_WIDGET (dockable));
                      g_object_unref (dockable);

                      gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook),
                                                     page_num);
                    }
                }
            }

          g_free (identifier);
        }
    }
}

void
dockable_view_size_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable = dockable_get_current (dockbook);
  gint          view_size;

  view_size = g_variant_get_int32 (value);

  if (dockable)
    {
      GimpContainerView *view = gimp_container_view_get_by_dockable (dockable);

      if (view)
        {
          gint old_size;
          gint border_width;

          old_size = gimp_container_view_get_view_size (view, &border_width);

          if (old_size != view_size)
            gimp_container_view_set_view_size (view, view_size, border_width);
        }
    }
}

void
dockable_tab_style_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable = dockable_get_current (dockbook);
  GimpTabStyle  tab_style;

  tab_style = (GimpTabStyle) g_variant_get_int32 (value);

  if (dockable && gimp_dockable_get_tab_style (dockable) != tab_style)
    {
      GtkWidget *tab_widget;

      gimp_dockable_set_tab_style (dockable, tab_style);

      tab_widget = gimp_dockbook_create_tab_widget (dockbook, dockable);

      gtk_notebook_set_tab_label (GTK_NOTEBOOK (dockbook),
                                  GTK_WIDGET (dockable),
                                  tab_widget);
    }
}

void
dockable_show_button_bar_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    {
      GimpDocked *docked;
      gboolean    show;

      docked = GIMP_DOCKED (gtk_bin_get_child (GTK_BIN (dockable)));
      show   = g_variant_get_boolean (value);

      gimp_docked_set_show_button_bar (docked, show);
    }
}


/*  private functions  */

static GimpDockable *
dockable_get_current (GimpDockbook *dockbook)
{
  gint page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  return (GimpDockable *) gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook),
                                                     page_num);
}
