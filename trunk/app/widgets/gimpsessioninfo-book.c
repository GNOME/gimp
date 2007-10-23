/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo-book.c
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockbook.h"
#include "gimpsessioninfo.h"
#include "gimpsessioninfo-book.h"
#include "gimpsessioninfo-dockable.h"


enum
{
  SESSION_INFO_BOOK_POSITION,
  SESSION_INFO_BOOK_CURRENT_PAGE,
  SESSION_INFO_BOOK_DOCKABLE
};


/*  public functions  */

GimpSessionInfoBook *
gimp_session_info_book_new (void)
{
  return g_slice_new0 (GimpSessionInfoBook);
}

void
gimp_session_info_book_free (GimpSessionInfoBook *info)
{
  g_return_if_fail (info != NULL);

  if (info->dockables)
    {
      g_list_foreach (info->dockables, (GFunc) gimp_session_info_dockable_free,
                      NULL);
      g_list_free (info->dockables);
    }

  g_slice_free (GimpSessionInfoBook, info);
}

void
gimp_session_info_book_serialize (GimpConfigWriter *writer,
                                  GimpDockbook     *dockbook)
{
  GList *children;
  GList *pages;
  gint   current_page;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));

  gimp_config_writer_open (writer, "book");

  if (GTK_IS_VPANED (GTK_WIDGET (dockbook)->parent))
    {
      GtkPaned *paned = GTK_PANED (GTK_WIDGET (dockbook)->parent);

      if (GTK_WIDGET (dockbook) == paned->child2)
        {
          gimp_config_writer_open (writer, "position");
          gimp_config_writer_printf (writer, "%d",
                                     gtk_paned_get_position (paned));
          gimp_config_writer_close (writer);
        }
    }

  current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  gimp_config_writer_open (writer, "current-page");
  gimp_config_writer_printf (writer, "%d", current_page);
  gimp_config_writer_close (writer);

  children = gtk_container_get_children (GTK_CONTAINER (dockbook));

  for (pages = children; pages; pages = g_list_next (pages))
    gimp_session_info_dockable_serialize (writer, pages->data);

  g_list_free (children);

  gimp_config_writer_close (writer);
}

GTokenType
gimp_session_info_book_deserialize (GScanner        *scanner,
                                    gint             scope,
                                    GimpSessionInfo *info)
{
  GimpSessionInfoBook *book;
  GTokenType           token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_LEFT_PAREN);
  g_return_val_if_fail (info != NULL, G_TOKEN_LEFT_PAREN);

  g_scanner_scope_add_symbol (scanner, scope, "position",
                              GINT_TO_POINTER (SESSION_INFO_BOOK_POSITION));
  g_scanner_scope_add_symbol (scanner, scope, "current-page",
                              GINT_TO_POINTER (SESSION_INFO_BOOK_CURRENT_PAGE));
  g_scanner_scope_add_symbol (scanner, scope, "dockable",
                              GINT_TO_POINTER (SESSION_INFO_BOOK_DOCKABLE));

  book = gimp_session_info_book_new ();

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
            case SESSION_INFO_BOOK_POSITION:
              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &book->position))
                goto error;
              break;

            case SESSION_INFO_BOOK_CURRENT_PAGE:
              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &book->current_page))
                goto error;
              break;

            case SESSION_INFO_BOOK_DOCKABLE:
              g_scanner_set_scope (scanner, scope + 1);
              token = gimp_session_info_dockable_deserialize (scanner,
                                                              scope + 1,
                                                              book);

              if (token == G_TOKEN_LEFT_PAREN)
                g_scanner_set_scope (scanner, scope);
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

  info->books = g_list_append (info->books, book);

  g_scanner_scope_remove_symbol (scanner, scope, "position");
  g_scanner_scope_remove_symbol (scanner, scope, "current-page");
  g_scanner_scope_remove_symbol (scanner, scope, "dockable");

  return token;

 error:
  gimp_session_info_book_free (book);

  return token;
}

void
gimp_session_info_book_restore (GimpSessionInfoBook *info,
                                GimpDock            *dock)
{
  GtkWidget *dockbook;
  GList     *pages;

  g_return_if_fail (info != NULL);
  g_return_if_fail (GIMP_IS_DOCK (dock));

  dockbook = gimp_dockbook_new (dock->dialog_factory->menu_factory);

  gimp_dock_add_book (dock, GIMP_DOCKBOOK (dockbook), -1);

  info->widget = dockbook;

  for (pages = info->dockables; pages; pages = g_list_next (pages))
    {
      GimpSessionInfoDockable *dockable_info = pages->data;
      GimpDockable            *dockable;

      dockable = gimp_session_info_dockable_restore (dockable_info, dock);

      if (dockable)
        gimp_dockbook_add (GIMP_DOCKBOOK (dockbook), dockable, -1);
    }

  if (info->current_page <
      gtk_notebook_get_n_pages (GTK_NOTEBOOK (dockbook)))
    {
      gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook),
                                     info->current_page);
    }
  else
    {
      gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook), 0);
    }
 }
