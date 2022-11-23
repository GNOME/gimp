/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "widgets/ligmacontainerview.h"
#include "widgets/ligmacontainerview-utils.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmadockable.h"
#include "widgets/ligmadockbook.h"
#include "widgets/ligmadocked.h"
#include "widgets/ligmasessioninfo.h"

#include "dockable-commands.h"


static LigmaDockable * dockable_get_current (LigmaDockbook *dockbook);


/*  public functions  */

void
dockable_add_tab_cmd_callback (LigmaAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (data);

  ligma_dockbook_add_from_dialog_factory (dockbook,
                                         g_variant_get_string (value, NULL));
}

void
dockable_close_tab_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (data);
  LigmaDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    gtk_container_remove (GTK_CONTAINER (dockbook),
                          GTK_WIDGET (dockable));
}

void
dockable_detach_tab_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (data);
  LigmaDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    ligma_dockable_detach (dockable);
}

void
dockable_lock_tab_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (data);
  LigmaDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    {
      gboolean lock = g_variant_get_boolean (value);

      ligma_dockable_set_locked (dockable, lock);
    }
}

void
dockable_toggle_view_cmd_callback (LigmaAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (data);
  LigmaDockable *dockable;
  LigmaViewType  view_type;
  gint          page_num;

  view_type = (LigmaViewType) g_variant_get_int32 (value);

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (LigmaDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable)
    {
      LigmaDialogFactoryEntry *entry;

      ligma_dialog_factory_from_widget (GTK_WIDGET (dockable), &entry);

      if (entry)
        {
          gchar *identifier;
          gchar *substring = NULL;

          identifier = g_strdup (entry->identifier);

          substring = strstr (identifier, "grid");

          if (substring && view_type == LIGMA_VIEW_TYPE_GRID)
            {
              g_free (identifier);
              return;
            }

          if (! substring)
            {
              substring = strstr (identifier, "list");

              if (substring && view_type == LIGMA_VIEW_TYPE_LIST)
                {
                  g_free (identifier);
                  return;
                }
            }

          if (substring)
            {
              LigmaContainerView *old_view;
              GtkWidget         *new_dockable;
              LigmaDock          *dock;
              gint               view_size = -1;

              if (view_type == LIGMA_VIEW_TYPE_LIST)
                memcpy (substring, "list", 4);
              else if (view_type == LIGMA_VIEW_TYPE_GRID)
                memcpy (substring, "grid", 4);

              old_view = ligma_container_view_get_by_dockable (dockable);

              if (old_view)
                view_size = ligma_container_view_get_view_size (old_view, NULL);

              dock         = ligma_dockbook_get_dock (dockbook);
              new_dockable = ligma_dialog_factory_dockable_new (ligma_dock_get_dialog_factory (dock),
                                                               dock,
                                                               identifier,
                                                               view_size);

              if (new_dockable)
                {
                  LigmaDocked *old;
                  LigmaDocked *new;
                  gboolean    show;

                  ligma_dockable_set_locked (LIGMA_DOCKABLE (new_dockable),
                                            ligma_dockable_get_locked (dockable));

                  old = LIGMA_DOCKED (gtk_bin_get_child (GTK_BIN (dockable)));
                  new = LIGMA_DOCKED (gtk_bin_get_child (GTK_BIN (new_dockable)));

                  show = ligma_docked_get_show_button_bar (old);
                  ligma_docked_set_show_button_bar (new, show);

                  /*  Maybe ligma_dialog_factory_dockable_new() returned
                   *  an already existing singleton dockable, so check
                   *  if it already is attached to a dockbook.
                   */
                  if (! ligma_dockable_get_dockbook (LIGMA_DOCKABLE (new_dockable)))
                    {
                      gtk_notebook_insert_page (GTK_NOTEBOOK (dockbook),
                                                new_dockable, NULL,
                                                page_num);
                      gtk_widget_show (new_dockable);

                      gtk_container_remove (GTK_CONTAINER (dockbook),
                                            GTK_WIDGET (dockable));

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
dockable_view_size_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (data);
  LigmaDockable *dockable = dockable_get_current (dockbook);
  gint          view_size;

  view_size = g_variant_get_int32 (value);

  if (dockable)
    {
      LigmaContainerView *view = ligma_container_view_get_by_dockable (dockable);

      if (view)
        {
          gint old_size;
          gint border_width;

          old_size = ligma_container_view_get_view_size (view, &border_width);

          if (old_size != view_size)
            ligma_container_view_set_view_size (view, view_size, border_width);
        }
    }
}

void
dockable_tab_style_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (data);
  LigmaDockable *dockable = dockable_get_current (dockbook);
  LigmaTabStyle  tab_style;

  tab_style = (LigmaTabStyle) g_variant_get_int32 (value);

  if (dockable && ligma_dockable_get_tab_style (dockable) != tab_style)
    {
      GtkWidget *tab_widget;

      ligma_dockable_set_tab_style (dockable, tab_style);

      tab_widget = ligma_dockbook_create_tab_widget (dockbook, dockable);

      gtk_notebook_set_tab_label (GTK_NOTEBOOK (dockbook),
                                  GTK_WIDGET (dockable),
                                  tab_widget);
    }
}

void
dockable_show_button_bar_cmd_callback (LigmaAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  LigmaDockbook *dockbook = LIGMA_DOCKBOOK (data);
  LigmaDockable *dockable = dockable_get_current (dockbook);

  if (dockable)
    {
      LigmaDocked *docked;
      gboolean    show;

      docked = LIGMA_DOCKED (gtk_bin_get_child (GTK_BIN (dockable)));
      show   = g_variant_get_boolean (value);

      ligma_docked_set_show_button_bar (docked, show);
    }
}


/*  private functions  */

static LigmaDockable *
dockable_get_current (LigmaDockbook *dockbook)
{
  gint page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  return (LigmaDockable *) gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook),
                                                     page_num);
}
