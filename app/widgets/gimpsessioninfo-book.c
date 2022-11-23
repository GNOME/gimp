/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasessioninfo-book.c
 * Copyright (C) 2001-2007 Michael Natterer <mitch@ligma.org>
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

#include <gtk/gtk.h>

#include "libligmaconfig/ligmaconfig.h"

#include "widgets-types.h"

#include "ligmadialogfactory.h"
#include "ligmadock.h"
#include "ligmadockbook.h"
#include "ligmasessioninfo.h"
#include "ligmasessioninfo-book.h"
#include "ligmasessioninfo-dockable.h"


enum
{
  SESSION_INFO_BOOK_POSITION,
  SESSION_INFO_BOOK_CURRENT_PAGE,
  SESSION_INFO_BOOK_DOCKABLE
};


/*  public functions  */

LigmaSessionInfoBook *
ligma_session_info_book_new (void)
{
  return g_slice_new0 (LigmaSessionInfoBook);
}

void
ligma_session_info_book_free (LigmaSessionInfoBook *info)
{
  g_return_if_fail (info != NULL);

  if (info->dockables)
    {
      g_list_free_full (info->dockables,
                        (GDestroyNotify) ligma_session_info_dockable_free);
      info->dockables = NULL;
    }

  g_slice_free (LigmaSessionInfoBook, info);
}

void
ligma_session_info_book_serialize (LigmaConfigWriter    *writer,
                                  LigmaSessionInfoBook *info)
{
  GList *pages;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (info != NULL);

  ligma_config_writer_open (writer, "book");

  if (info->position != 0)
    {
      gint position;

      position = ligma_session_info_apply_position_accuracy (info->position);

      ligma_config_writer_open (writer, "position");
      ligma_config_writer_printf (writer, "%d", position);
      ligma_config_writer_close (writer);
    }

  ligma_config_writer_open (writer, "current-page");
  ligma_config_writer_printf (writer, "%d", info->current_page);
  ligma_config_writer_close (writer);

  for (pages = info->dockables; pages; pages = g_list_next (pages))
    ligma_session_info_dockable_serialize (writer, pages->data);

  ligma_config_writer_close (writer);
}

GTokenType
ligma_session_info_book_deserialize (GScanner             *scanner,
                                    gint                  scope,
                                    LigmaSessionInfoBook **book)
{
  LigmaSessionInfoBook *info;
  GTokenType           token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_LEFT_PAREN);
  g_return_val_if_fail (book != NULL, G_TOKEN_LEFT_PAREN);

  g_scanner_scope_add_symbol (scanner, scope, "position",
                              GINT_TO_POINTER (SESSION_INFO_BOOK_POSITION));
  g_scanner_scope_add_symbol (scanner, scope, "current-page",
                              GINT_TO_POINTER (SESSION_INFO_BOOK_CURRENT_PAGE));
  g_scanner_scope_add_symbol (scanner, scope, "dockable",
                              GINT_TO_POINTER (SESSION_INFO_BOOK_DOCKABLE));

  info = ligma_session_info_book_new ();

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
              LigmaSessionInfoDockable *dockable;

            case SESSION_INFO_BOOK_POSITION:
              token = G_TOKEN_INT;
              if (! ligma_scanner_parse_int (scanner, &info->position))
                goto error;
              break;

            case SESSION_INFO_BOOK_CURRENT_PAGE:
              token = G_TOKEN_INT;
              if (! ligma_scanner_parse_int (scanner, &info->current_page))
                goto error;
              break;

            case SESSION_INFO_BOOK_DOCKABLE:
              g_scanner_set_scope (scanner, scope + 1);
              token = ligma_session_info_dockable_deserialize (scanner,
                                                              scope + 1,
                                                              &dockable);

              if (token == G_TOKEN_LEFT_PAREN)
                {
                  info->dockables = g_list_append (info->dockables, dockable);
                  g_scanner_set_scope (scanner, scope);
                }
              else
                goto error;

              break;

            default:
              goto error;
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  *book = info;

  g_scanner_scope_remove_symbol (scanner, scope, "position");
  g_scanner_scope_remove_symbol (scanner, scope, "current-page");
  g_scanner_scope_remove_symbol (scanner, scope, "dockable");

  return token;

 error:
  *book = NULL;

  ligma_session_info_book_free (info);

  return token;
}

LigmaSessionInfoBook *
ligma_session_info_book_from_widget (LigmaDockbook *dockbook)
{
  LigmaSessionInfoBook *info;
  GtkWidget           *parent;
  GList               *children;
  GList               *list;

  g_return_val_if_fail (LIGMA_IS_DOCKBOOK (dockbook), NULL);

  info = ligma_session_info_book_new ();

  parent = gtk_widget_get_parent (GTK_WIDGET (dockbook));

  if (GTK_IS_PANED (parent))
    {
      GtkPaned *paned = GTK_PANED (parent);

      if (GTK_WIDGET (dockbook) == gtk_paned_get_child2 (paned))
        info->position = gtk_paned_get_position (paned);
    }

  info->current_page =
    gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  children = gtk_container_get_children (GTK_CONTAINER (dockbook));

  for (list = children; list; list = g_list_next (list))
    {
      LigmaSessionInfoDockable *dockable;

      dockable = ligma_session_info_dockable_from_widget (list->data);

      info->dockables = g_list_prepend (info->dockables, dockable);
    }

  info->dockables = g_list_reverse (info->dockables);

  g_list_free (children);

  return info;
}

LigmaDockbook *
ligma_session_info_book_restore (LigmaSessionInfoBook *info,
                                LigmaDock            *dock)
{
  LigmaDialogFactory *dialog_factory;
  LigmaMenuFactory   *menu_factory;
  GtkWidget         *dockbook;
  GList             *pages;
  gint               n_dockables = 0;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  dialog_factory = ligma_dock_get_dialog_factory (dock);
  menu_factory   = ligma_dialog_factory_get_menu_factory (dialog_factory);

  dockbook = ligma_dockbook_new (menu_factory);

  ligma_dock_add_book (dock, LIGMA_DOCKBOOK (dockbook), -1);

  for (pages = info->dockables; pages; pages = g_list_next (pages))
    {
      LigmaSessionInfoDockable *dockable_info = pages->data;
      LigmaDockable            *dockable;

      dockable = ligma_session_info_dockable_restore (dockable_info, dock);

      if (dockable)
        {
          gtk_notebook_append_page (GTK_NOTEBOOK (dockbook),
                                    GTK_WIDGET (dockable), NULL);
          n_dockables++;
        }
    }

  if (info->current_page <
      gtk_notebook_get_n_pages (GTK_NOTEBOOK (dockbook)))
    {
      gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook),
                                     info->current_page);
    }
  else if (n_dockables > 1)
    {
      gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook), 0);
    }

  /*  Return the dockbook even if no dockable could be restored
   *  (n_dockables == 0) because otherwise we would have to remove it
   *  from the dock right here, which could implicitly destroy the
   *  dock and make catching restore errors much harder on higher
   *  levels. Instead, we check for restored empty dockbooks in our
   *  caller.
   */
  return LIGMA_DOCKBOOK (dockbook);
}
