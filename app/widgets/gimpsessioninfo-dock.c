/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo-dock.c
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockwindow.h"
#include "gimpsessioninfo.h"
#include "gimpsessioninfo-aux.h"
#include "gimpsessioninfo-book.h"
#include "gimpsessioninfo-dock.h"
#include "gimpsessioninfo-private.h"
#include "gimptoolbox.h"


enum
{
  SESSION_INFO_BOOK
};


/*  public functions  */

GimpSessionInfoDock *
gimp_session_info_dock_new (const gchar *identifier)
{
  GimpSessionInfoDock *dock_info = NULL;

  dock_info = g_slice_new0 (GimpSessionInfoDock);
  dock_info->identifier = g_strdup (identifier);

  return dock_info;
}

void
gimp_session_info_dock_free (GimpSessionInfoDock *dock_info)
{
  g_return_if_fail (dock_info != NULL);

  if (dock_info->identifier)
    {
      g_free (dock_info->identifier);
      dock_info->identifier = NULL;
    }

  if (dock_info->books)
    {
      g_list_foreach (dock_info->books,
                      (GFunc) gimp_session_info_book_free,
                      NULL);
      g_list_free (dock_info->books);
      dock_info->books = NULL;
    }

  g_slice_free (GimpSessionInfoDock, dock_info);
}

void
gimp_session_info_dock_serialize (GimpConfigWriter    *writer,
                                  GimpSessionInfoDock *dock_info)
{
  GList *list;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (dock_info != NULL);

  gimp_config_writer_open (writer, dock_info->identifier);

  for (list = dock_info->books; list; list = g_list_next (list))
    gimp_session_info_book_serialize (writer, list->data);

  gimp_config_writer_close (writer);
}

GTokenType
gimp_session_info_dock_deserialize (GScanner             *scanner,
                                    gint                  scope,
                                    GimpSessionInfoDock **dock_info,
                                    const gchar          *identifier)
{
  GTokenType token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_LEFT_PAREN);
  g_return_val_if_fail (dock_info != NULL, G_TOKEN_LEFT_PAREN);

  g_scanner_scope_add_symbol (scanner, scope, "book",
                              GINT_TO_POINTER (SESSION_INFO_BOOK));

  *dock_info = gimp_session_info_dock_new (identifier);

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
              GimpSessionInfoBook *book;

            case SESSION_INFO_BOOK:
              g_scanner_set_scope (scanner, scope + 1);
              token = gimp_session_info_book_deserialize (scanner, scope + 1,
                                                          &book);

              if (token == G_TOKEN_LEFT_PAREN)
                {
                  (*dock_info)->books = g_list_append ((*dock_info)->books, book);
                  g_scanner_set_scope (scanner, scope);
                }
              else
                return token;

              break;

            default:
              return token;
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

  g_scanner_scope_remove_symbol (scanner, scope, "book");

  return token;
}

GimpSessionInfoDock *
gimp_session_info_dock_from_widget (GimpDock *dock)
{
  GimpSessionInfoDock *dock_info = NULL;
  GList               *list      = NULL;

  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  dock_info = gimp_session_info_dock_new (GIMP_IS_TOOLBOX (dock) ?
                                          "gimp-toolbox" :
                                          "gimp-dock");

  for (list = gimp_dock_get_dockbooks (dock); list; list = g_list_next (list))
    {
      GimpSessionInfoBook *book;

      book = gimp_session_info_book_from_widget (list->data);

      dock_info->books = g_list_prepend (dock_info->books, book);
    }

  dock_info->books = g_list_reverse (dock_info->books);
  
  return dock_info;
}

static void
gimp_session_info_dock_paned_size_allocate (GtkWidget     *paned,
                                            GtkAllocation *allocation,
                                            gpointer       data)
{
  g_signal_handlers_disconnect_by_func (paned,
                                        gimp_session_info_dock_paned_size_allocate,
                                        data);

  gtk_paned_set_position (GTK_PANED (paned), GPOINTER_TO_INT (data));
}

static void
gimp_session_info_dock_paned_map (GtkWidget *paned,
                                  gpointer   data)
{
  g_signal_handlers_disconnect_by_func (paned,
                                        gimp_session_info_dock_paned_map,
                                        data);

  g_signal_connect_after (paned, "size-allocate",
                          G_CALLBACK (gimp_session_info_dock_paned_size_allocate),
                          data);
}

void
gimp_session_info_dock_restore (GimpSessionInfoDock *dock_info,
                                GimpDialogFactory   *factory,
                                GdkScreen           *screen,
                                GimpDockWindow      *dock_window)
{
  GtkWidget     *dock       = NULL;
  GList         *iter       = NULL;
  GimpUIManager *ui_manager = NULL;

  g_return_if_fail (GIMP_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (GDK_IS_SCREEN (screen));

  ui_manager = gimp_dock_window_get_ui_manager (GIMP_DOCK_WINDOW (dock_window));
  dock       = gimp_dialog_factory_dialog_new (factory,
                                               screen,
                                               ui_manager,
                                               dock_info->identifier,
                                               -1 /*view_size*/,
                                               FALSE /*present*/);

  g_return_if_fail (GIMP_IS_DOCK (dock));

  /* Add the dock to the dock window immediately so the stuff in the
   * dock has access to e.g. a dialog factory
   */
  gimp_dock_window_add_dock (GIMP_DOCK_WINDOW (dock_window),
                             GIMP_DOCK (dock),
                             -1);

  /* Note that if it is a toolbox, we will get here even though we
   * don't have any books
   */
  for (iter = dock_info ? dock_info->books : NULL;
       iter;
       iter = g_list_next (iter))
    {
      GimpSessionInfoBook *book_info = iter->data;
      GtkWidget           *dockbook;
      GtkWidget           *parent;

      dockbook = GTK_WIDGET (gimp_session_info_book_restore (book_info,
                                                             GIMP_DOCK (dock)));
      parent   = gtk_widget_get_parent (dockbook);

      if (GTK_IS_VPANED (parent))
        {
          GtkPaned *paned = GTK_PANED (parent);

          if (dockbook == gtk_paned_get_child2 (paned))
            g_signal_connect_after (paned, "map",
                                    G_CALLBACK (gimp_session_info_dock_paned_map),
                                    GINT_TO_POINTER (book_info->position));
        }
    }

  gtk_widget_show (dock);
}
