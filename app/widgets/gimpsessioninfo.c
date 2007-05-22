/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "gimpcontainerview.h"
#include "gimpcontainerview-utils.h"
#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpdocked.h"
#include "gimpsessioninfo.h"


enum
{
  SESSION_INFO_POSITION,
  SESSION_INFO_SIZE,
  SESSION_INFO_OPEN,
  SESSION_INFO_AUX,

  SESSION_INFO_DOCK,

  SESSION_INFO_BOOK,
  SESSION_INFO_BOOK_POSITION,
  SESSION_INFO_BOOK_CURRENT_PAGE,
  SESSION_INFO_DOCKABLE,

  SESSION_INFO_DOCKABLE_TAB_STYLE,
  SESSION_INFO_DOCKABLE_VIEW_SIZE,
  SESSION_INFO_DOCKABLE_AUX
};

#define DEFAULT_SCREEN  -1


/*  local function prototypes  */

static GTokenType  session_info_dock_deserialize     (GScanner        *scanner,
                                                      GimpSessionInfo *info);
static GTokenType  session_info_book_deserialize     (GScanner        *scanner,
                                                      GimpSessionInfo *info);
static GTokenType  session_info_dockable_deserialize (GScanner        *scanner,
                                                      GimpSessionInfoBook *book);
static GTokenType  session_info_aux_deserialize      (GScanner        *scanner,
                                                      GList          **aux_list);
static void        session_info_set_aux_info         (GtkWidget       *dialog,
                                                      GList           *aux_info);
static GList     * session_info_get_aux_info         (GtkWidget       *dialog);


/*  public functions  */

void
gimp_session_info_free (GimpSessionInfo *info)
{
  g_return_if_fail (info != NULL);

  if (info->aux_info)
    {
      g_list_foreach (info->aux_info,
                      (GFunc) gimp_session_info_aux_free, NULL);
      g_list_free (info->aux_info);
    }

   if (info->books)
     {
       g_list_foreach (info->books,
                       (GFunc) gimp_session_info_book_free, NULL);
       g_list_free (info->books);
     }

   g_free (info);
}

void
gimp_session_info_book_free (GimpSessionInfoBook *book)
{
  g_return_if_fail (book != NULL);

  if (book->dockables)
    {
      g_list_foreach (book->dockables, (GFunc) gimp_session_info_dockable_free,
                      NULL);
      g_list_free (book->dockables);
    }

  g_free (book);
}

void
gimp_session_info_dockable_free (GimpSessionInfoDockable *dockable)
{
  g_return_if_fail (dockable != NULL);

  if (dockable->identifier)
    g_free (dockable->identifier);

  if (dockable->aux_info)
    {
      g_list_foreach (dockable->aux_info, (GFunc) gimp_session_info_aux_free,
                      NULL);
      g_list_free (dockable->aux_info);
    }

  g_free (dockable);
}

GimpSessionInfoAux *
gimp_session_info_aux_new (const gchar *name,
                           const gchar *value)
{
  GimpSessionInfoAux *aux;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (value != NULL, NULL);

  aux = g_slice_new0 (GimpSessionInfoAux);

  aux->name  = g_strdup (name);
  aux->value = g_strdup (value);

  return aux;
}

void
gimp_session_info_aux_free (GimpSessionInfoAux *aux)
{
  g_return_if_fail (aux != NULL);

  g_free (aux->name);
  g_free (aux->value);
  g_slice_free (GimpSessionInfoAux, aux);
}

GList *
gimp_session_info_aux_new_from_props (GObject *object,
                                      ...)
{
  GList       *list = NULL;
  const gchar *prop_name;
  va_list      args;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  va_start (args, object);

  for (prop_name = va_arg (args, const gchar *);
       prop_name;
       prop_name = va_arg (args, const gchar *))
    {
      GObjectClass *class = G_OBJECT_GET_CLASS (object);
      GParamSpec   *pspec = g_object_class_find_property (class, prop_name);

      if (pspec)
        {
          GString *str   = g_string_new (NULL);
          GValue   value = { 0, };

          g_value_init (&value, pspec->value_type);
          g_object_get_property (object, pspec->name, &value);

          if (! g_param_value_defaults (pspec, &value) &&
              gimp_config_serialize_value (&value, str, TRUE))
            {
              list = g_list_prepend (list,
                                     gimp_session_info_aux_new (prop_name,
                                                                str->str));
            }

          g_value_unset (&value);
        }
      else
        {
          g_warning ("%s: no property named '%s' for %s",
                     G_STRFUNC,
                     prop_name, G_OBJECT_CLASS_NAME (class));
        }
    }

  va_end (args);

  return g_list_reverse (list);
}

void
gimp_session_info_aux_set_props (GObject *object,
                                 GList   *auxs,
                                 ...)
{
  const gchar *prop_name;
  va_list      args;

  g_return_if_fail (G_IS_OBJECT (object));

  va_start (args, auxs);

  for (prop_name = va_arg (args, const gchar *);
       prop_name;
       prop_name = va_arg (args, const gchar *))
    {
      GList *list;

      for (list = auxs; list; list = g_list_next (list))
        {
          GimpSessionInfoAux *aux = list->data;

          if (strcmp (aux->name, prop_name) == 0)
            {
              GObjectClass *class = G_OBJECT_GET_CLASS (object);
              GParamSpec   *pspec = g_object_class_find_property (class,
                                                                  prop_name);

              if (pspec)
                {
                  GValue  value = { 0, };

                  g_value_init (&value, pspec->value_type);

                  if (G_VALUE_HOLDS_ENUM (&value))
                    {
                      GEnumClass *enum_class;
                      GEnumValue *enum_value;

                      enum_class = g_type_class_peek (pspec->value_type);
                      enum_value = g_enum_get_value_by_nick (enum_class,
                                                             aux->value);

                      if (enum_value)
                        {
                          g_value_set_enum (&value, enum_value->value);
                          g_object_set_property (object, pspec->name, &value);
                        }
                      else
                        {
                        g_warning ("%s: unknown enum value in '%s' for %s",
                                   G_STRFUNC,
                                   prop_name, G_OBJECT_CLASS_NAME (class));
                        }
                    }
                  else
                    {
                      GValue  str_value = { 0, };

                      g_value_init (&str_value, G_TYPE_STRING);
                      g_value_set_static_string (&str_value, aux->value);

                      if (g_value_transform (&str_value, &value))
                        g_object_set_property (object, pspec->name, &value);
                      else
                        g_warning ("%s: cannot convert property '%s' for %s",
                                   G_STRFUNC,
                                   prop_name, G_OBJECT_CLASS_NAME (class));

                      g_value_unset (&str_value);
                    }

                  g_value_unset (&value);
                }
              else
                {
                  g_warning ("%s: no property named '%s' for %s",
                             G_STRFUNC,
                             prop_name, G_OBJECT_CLASS_NAME (class));
                }
            }
        }
    }

  va_end (args);
}

void
gimp_session_info_save (GimpSessionInfo  *info,
                        const gchar      *factory_name,
                        GimpConfigWriter *writer)
{
  GEnumClass  *enum_class;
  const gchar *dialog_name;

  g_return_if_fail (info != NULL);
  g_return_if_fail (factory_name != NULL);
  g_return_if_fail (writer != NULL);

  enum_class = g_type_class_ref (GIMP_TYPE_TAB_STYLE);

  if (info->widget)
    gimp_session_info_get_geometry (info);

  if (info->toplevel_entry)
    dialog_name = info->toplevel_entry->identifier;
  else
    dialog_name = "dock";

  gimp_config_writer_open (writer, "session-info");
  gimp_config_writer_string (writer, factory_name);
  gimp_config_writer_string (writer, dialog_name);

  gimp_config_writer_open (writer, "position");
  gimp_config_writer_printf (writer, "%d %d", info->x, info->y);
  gimp_config_writer_close (writer);

  if (info->width > 0 && info->height > 0)
    {
      gimp_config_writer_open (writer, "size");
      gimp_config_writer_printf (writer, "%d %d", info->width, info->height);
      gimp_config_writer_close (writer);
    }

  if (info->open)
    {
      gimp_config_writer_open (writer, "open-on-exit");

      if (info->screen != DEFAULT_SCREEN)
        gimp_config_writer_printf (writer, "%d", info->screen);

      gimp_config_writer_close (writer);
    }

  if (info->widget)
    {
      info->aux_info = session_info_get_aux_info (info->widget);

      if (info->aux_info)
        {
          GList *list;

          gimp_config_writer_open (writer, "aux-info");

          for (list = info->aux_info; list; list = g_list_next (list))
            {
              GimpSessionInfoAux *aux = list->data;

              gimp_config_writer_open (writer, aux->name);
              gimp_config_writer_string (writer, aux->value);
              gimp_config_writer_close (writer);
            }

          gimp_config_writer_close (writer);

          g_list_foreach (info->aux_info, (GFunc) gimp_session_info_aux_free,
                          NULL);
          g_list_free (info->aux_info);
          info->aux_info = NULL;
        }
    }

  if (! info->toplevel_entry && info->widget)
    {
      GimpDock *dock;
      GList    *books;

      dock = GIMP_DOCK (info->widget);

      gimp_config_writer_open (writer, "dock");

      for (books = dock->dockbooks; books; books = g_list_next (books))
        {
          GimpDockbook *dockbook = books->data;
          GList        *children;
          GList        *pages;
          gint          current_page;

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
            {
              GimpDockable           *dockable = pages->data;
              GimpDialogFactoryEntry *entry;

              gimp_dialog_factory_from_widget (GTK_WIDGET (dockable), &entry);

              if (entry)
                {
                  GimpContainerView *view;
                  GEnumValue        *enum_value;
                  const gchar       *tab_style = "icon";
                  gint               view_size = -1;
                  GList             *aux_info;

                  gimp_config_writer_open (writer, "dockable");
                  gimp_config_writer_string (writer, entry->identifier);

                  enum_value = g_enum_get_value (enum_class,
                                                 dockable->tab_style);

                  if (enum_value)
                    tab_style = enum_value->value_nick;

                  gimp_config_writer_open (writer, "tab-style");
                  gimp_config_writer_print (writer, tab_style, -1);
                  gimp_config_writer_close (writer);

                  view = gimp_container_view_get_by_dockable (dockable);

                  if (view)
                    view_size = gimp_container_view_get_view_size (view, NULL);

                  if (view_size > 0 &&
                      view_size != entry->view_size)
                    {
                      gimp_config_writer_open (writer, "preview-size");
                      gimp_config_writer_printf (writer, "%d", view_size);
                      gimp_config_writer_close (writer);
                    }

                  aux_info = session_info_get_aux_info (GTK_WIDGET (dockable));

                  if (aux_info)
                    {
                      GList *list;

                      gimp_config_writer_open (writer, "aux-info");

                      for (list = aux_info; list; list = g_list_next (list))
                        {
                          GimpSessionInfoAux *aux = list->data;

                          gimp_config_writer_open (writer, aux->name);
                          gimp_config_writer_string (writer, aux->value);
                          gimp_config_writer_close (writer);
                        }

                      gimp_config_writer_close (writer);

                      g_list_foreach (aux_info,
                                      (GFunc) gimp_session_info_aux_free,
                                      NULL);
                      g_list_free (aux_info);
                    }

                  gimp_config_writer_close (writer);  /* dockable */
                }
            }

          g_list_free (children);

          gimp_config_writer_close (writer);  /* book */
        }

      gimp_config_writer_close (writer);  /* dock */
    }

  gimp_config_writer_close (writer);  /* session-info */

  g_type_class_unref (enum_class);
}

GTokenType
gimp_session_info_deserialize (GScanner *scanner,
                               gint      scope)
{
  GimpDialogFactory *factory;
  GimpSessionInfo   *info = NULL;
  GTokenType         token;
  gboolean           skip = FALSE;
  gchar             *factory_name;
  gchar             *entry_name;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_LEFT_PAREN);

  g_scanner_scope_add_symbol (scanner, scope, "position",
                              GINT_TO_POINTER (SESSION_INFO_POSITION));
  g_scanner_scope_add_symbol (scanner, scope, "size",
                              GINT_TO_POINTER (SESSION_INFO_SIZE));
  g_scanner_scope_add_symbol (scanner, scope, "open-on-exit",
                              GINT_TO_POINTER (SESSION_INFO_OPEN));
  g_scanner_scope_add_symbol (scanner, scope, "aux-info",
                              GINT_TO_POINTER (SESSION_INFO_AUX));
  g_scanner_scope_add_symbol (scanner, scope, "dock",
                              GINT_TO_POINTER (SESSION_INFO_DOCK));

  g_scanner_scope_add_symbol (scanner, SESSION_INFO_DOCK, "book",
                              GINT_TO_POINTER (SESSION_INFO_BOOK));

  g_scanner_scope_add_symbol (scanner, SESSION_INFO_BOOK, "position",
                              GINT_TO_POINTER (SESSION_INFO_BOOK_POSITION));
  g_scanner_scope_add_symbol (scanner, SESSION_INFO_BOOK, "current-page",
                              GINT_TO_POINTER (SESSION_INFO_BOOK_CURRENT_PAGE));
  g_scanner_scope_add_symbol (scanner, SESSION_INFO_BOOK, "dockable",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE));

  g_scanner_scope_add_symbol (scanner, SESSION_INFO_DOCKABLE, "tab-style",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE_TAB_STYLE));
  g_scanner_scope_add_symbol (scanner, SESSION_INFO_DOCKABLE, "preview-size",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE_VIEW_SIZE));
  g_scanner_scope_add_symbol (scanner, SESSION_INFO_DOCKABLE, "aux-info",
                              GINT_TO_POINTER (SESSION_INFO_DOCKABLE_AUX));

  token = G_TOKEN_STRING;

  if (! gimp_scanner_parse_string (scanner, &factory_name))
    goto error;

  factory = gimp_dialog_factory_from_name (factory_name);
  g_free (factory_name);

  if (! factory)
    goto error;

  if (! gimp_scanner_parse_string (scanner, &entry_name))
    goto error;

  info = g_new0 (GimpSessionInfo, 1);

  info->screen = DEFAULT_SCREEN;

  if (strcmp (entry_name, "dock"))
    {
      info->toplevel_entry = gimp_dialog_factory_find_entry (factory,
                                                             entry_name);
      skip = (info->toplevel_entry == NULL);
    }

  g_free (entry_name);

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
            case SESSION_INFO_POSITION:
              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &info->x))
                goto error;
              if (! gimp_scanner_parse_int (scanner, &info->y))
                goto error;
              break;

            case SESSION_INFO_SIZE:
              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &info->width))
                goto error;
              if (! gimp_scanner_parse_int (scanner, &info->height))
                goto error;
              break;

            case SESSION_INFO_OPEN:
              info->open = TRUE;

              /*  the screen number is optional  */
              if (g_scanner_peek_next_token (scanner) == G_TOKEN_RIGHT_PAREN)
                break;

              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &info->screen))
                goto error;
              break;

            case SESSION_INFO_AUX:
              token = session_info_aux_deserialize (scanner, &info->aux_info);
              if (token != G_TOKEN_LEFT_PAREN)
                goto error;
              break;

            case SESSION_INFO_DOCK:
              if (info->toplevel_entry)
                goto error;

              g_scanner_set_scope (scanner, SESSION_INFO_DOCK);
              token = session_info_dock_deserialize (scanner, info);

              if (token == G_TOKEN_LEFT_PAREN)
                g_scanner_set_scope (scanner, scope);
              else
                goto error;

              break;

            default:
              break;
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

  if (token == G_TOKEN_LEFT_PAREN)
    {
      token = G_TOKEN_RIGHT_PAREN;

      if (!skip && g_scanner_peek_next_token (scanner) == token)
        factory->session_infos = g_list_append (factory->session_infos, info);
      else
        gimp_session_info_free (info);
    }
  else
    {
    error:
      if (info)
        gimp_session_info_free (info);
    }

  g_scanner_scope_remove_symbol (scanner, scope, "position");
  g_scanner_scope_remove_symbol (scanner, scope, "size");
  g_scanner_scope_remove_symbol (scanner, scope, "open-on-exit");
  g_scanner_scope_remove_symbol (scanner, scope, "aux-info");
  g_scanner_scope_remove_symbol (scanner, scope, "dock");

  g_scanner_scope_remove_symbol (scanner, SESSION_INFO_DOCK, "book");

  g_scanner_scope_remove_symbol (scanner, SESSION_INFO_BOOK, "position");
  g_scanner_scope_remove_symbol (scanner, SESSION_INFO_BOOK, "current-page");
  g_scanner_scope_remove_symbol (scanner, SESSION_INFO_BOOK, "dockable");

  g_scanner_scope_remove_symbol (scanner, SESSION_INFO_DOCKABLE, "tab-style");
  g_scanner_scope_remove_symbol (scanner, SESSION_INFO_DOCKABLE, "preview-size");
  g_scanner_scope_remove_symbol (scanner, SESSION_INFO_DOCKABLE, "aux-info");

  return token;
}

static void
gimp_session_info_paned_size_allocate (GtkWidget     *paned,
                                       GtkAllocation *allocation,
                                       gpointer       data)
{
  g_signal_handlers_disconnect_by_func (paned,
                                        gimp_session_info_paned_size_allocate,
                                        data);

  gtk_paned_set_position (GTK_PANED (paned), GPOINTER_TO_INT (data));
}

static void
gimp_session_info_paned_map (GtkWidget *paned,
                             gpointer   data)
{
  g_signal_handlers_disconnect_by_func (paned,
                                        gimp_session_info_paned_map,
                                        data);

  g_signal_connect_after (paned, "size-allocate",
                          G_CALLBACK (gimp_session_info_paned_size_allocate),
                          data);
}

void
gimp_session_info_restore (GimpSessionInfo   *info,
                           GimpDialogFactory *factory)
{
  GdkDisplay *display;
  GdkScreen  *screen = NULL;

  g_return_if_fail (info != NULL);
  g_return_if_fail (GIMP_IS_DIALOG_FACTORY (factory));

  display = gdk_display_get_default ();

  if (info->screen != DEFAULT_SCREEN)
    screen = gdk_display_get_screen (display, info->screen);

  if (! screen)
    screen = gdk_display_get_default_screen (display);

  info->open   = FALSE;
  info->screen = DEFAULT_SCREEN;

  if (info->toplevel_entry)
    {
      GtkWidget *dialog;

      dialog =
        gimp_dialog_factory_dialog_new (factory, screen,
                                        info->toplevel_entry->identifier,
                                        info->toplevel_entry->view_size,
                                        TRUE);

      if (dialog && info->aux_info)
        session_info_set_aux_info (dialog, info->aux_info);
    }
  else
    {
      GimpDock *dock;
      GList    *books;

      dock = GIMP_DOCK (gimp_dialog_factory_dock_new (factory, screen));

      if (dock && info->aux_info)
        session_info_set_aux_info (GTK_WIDGET (dock), info->aux_info);

      for (books = info->books; books; books = g_list_next (books))
        {
          GimpSessionInfoBook *book_info = books->data;
          GtkWidget           *dockbook;
          GList               *pages;

          dockbook = gimp_dockbook_new (dock->dialog_factory->menu_factory);

          gimp_dock_add_book (dock, GIMP_DOCKBOOK (dockbook), -1);

          book_info->widget = dockbook;

          for (pages = book_info->dockables; pages; pages = g_list_next (pages))
            {
              GimpSessionInfoDockable *dockable_info = pages->data;
              GtkWidget               *dockable;

              if (dockable_info->view_size < GIMP_VIEW_SIZE_TINY ||
                  dockable_info->view_size > GIMP_VIEW_SIZE_GIGANTIC)
                dockable_info->view_size = -1;

              /*  use the new dock's dialog factory to create dockables
               *  because it may be different from the dialog factory
               *  the dock was created from.
               */
              dockable =
                gimp_dialog_factory_dockable_new (dock->dialog_factory,
                                                  dock,
                                                  dockable_info->identifier,
                                                  dockable_info->view_size);

              if (! dockable)
                continue;

              gimp_dockable_set_tab_style (GIMP_DOCKABLE (dockable),
                                           dockable_info->tab_style);

              if (dockable_info->aux_info)
                session_info_set_aux_info (dockable, dockable_info->aux_info);

              gimp_dockbook_add (GIMP_DOCKBOOK (dockbook),
                                 GIMP_DOCKABLE (dockable), -1);
            }

          if (book_info->current_page <
              gtk_notebook_get_n_pages (GTK_NOTEBOOK (dockbook)))
            {
              gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook),
                                             book_info->current_page);
            }
          else
            {
              gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook), 0);
            }
        }

      for (books = info->books; books; books = g_list_next (books))
        {
          GimpSessionInfoBook *book_info = books->data;
          GtkWidget           *dockbook  = book_info->widget;

          if (GTK_IS_VPANED (dockbook->parent))
            {
              GtkPaned *paned = GTK_PANED (dockbook->parent);

              if (dockbook == paned->child2)
                g_signal_connect_after (paned, "map",
                                        G_CALLBACK (gimp_session_info_paned_map),
                                        GINT_TO_POINTER (book_info->position));
            }
        }

      g_list_foreach (info->books, (GFunc) gimp_session_info_book_free, NULL);
      g_list_free (info->books);
      info->books = NULL;

      gtk_widget_show (GTK_WIDGET (dock));
    }

  g_list_foreach (info->aux_info, (GFunc) gimp_session_info_aux_free, NULL);
  g_list_free (info->aux_info);
  info->aux_info = NULL;
}

/* This function mostly lifted from
 * gtk+/gdk/gdkscreen.c:gdk_screen_get_monitor_at_window()
 */
static gint
get_appropriate_monitor (GdkScreen *screen,
                         gint       x,
                         gint       y,
                         gint       w,
                         gint       h)
{
  GdkRectangle rect;
  gint         area    = 0;
  gint         monitor = -1;
  gint         num_monitors;
  gint         i;

  rect.x      = x;
  rect.y      = y;
  rect.width  = w;
  rect.height = h;

  num_monitors = gdk_screen_get_n_monitors (screen);

  for (i = 0; i < num_monitors; i++)
    {
      GdkRectangle geometry;

      gdk_screen_get_monitor_geometry (screen, i, &geometry);

      if (gdk_rectangle_intersect (&rect, &geometry, &geometry) &&
          geometry.width * geometry.height > area)
        {
          area = geometry.width * geometry.height;
          monitor = i;
        }
    }

  if (monitor >= 0)
    return monitor;
  else
    return gdk_screen_get_monitor_at_point (screen,
                                            rect.x + rect.width / 2,
                                            rect.y + rect.height / 2);
}

void
gimp_session_info_set_geometry (GimpSessionInfo *info)
{
  GdkScreen   *screen;
  GdkRectangle monitor;
  gchar        geom[32];

  g_return_if_fail (info != NULL);
  g_return_if_fail (GTK_IS_WINDOW (info->widget));

  screen = gtk_widget_get_screen (info->widget);

  if ((! info->toplevel_entry || info->toplevel_entry->remember_size) &&
      (info->width > 0 && info->height > 0))
    {
      gdk_screen_get_monitor_geometry (screen,
                                       get_appropriate_monitor (screen,
                                                                info->x,
                                                                info->y,
                                                                info->width,
                                                                info->height),
                                       &monitor);

      info->x = CLAMP (info->x,
                       monitor.x, monitor.x + monitor.width  - info->width);
      info->y = CLAMP (info->y,
                       monitor.y, monitor.y + monitor.height - info->height);
    }
  else
    {
      gdk_screen_get_monitor_geometry (screen,
                                       gdk_screen_get_monitor_at_point (screen,
                                                                        info->x,
                                                                        info->y),
                                       &monitor);

      info->x = CLAMP (info->x, monitor.x, monitor.x + monitor.width  - 128);
      info->y = CLAMP (info->y, monitor.y, monitor.y + monitor.height - 128);
    }

  g_snprintf (geom, sizeof (geom), "+%d+%d", info->x, info->y);

  gtk_window_parse_geometry (GTK_WINDOW (info->widget), geom);

  if (! info->toplevel_entry || info->toplevel_entry->remember_size)
    {
      if (info->width > 0 && info->height > 0)
        gtk_window_set_default_size (GTK_WINDOW (info->widget),
                                     info->width, info->height);
    }
}

void
gimp_session_info_get_geometry (GimpSessionInfo *info)
{
  g_return_if_fail (info != NULL);
  g_return_if_fail (GTK_IS_WINDOW (info->widget));

  if (info->widget->window)
    {
      gdk_window_get_root_origin (info->widget->window, &info->x, &info->y);

      if (! info->toplevel_entry || info->toplevel_entry->remember_size)
        {
          info->width  = info->widget->allocation.width;
          info->height = info->widget->allocation.height;
        }
      else
        {
          info->width  = 0;
          info->height = 0;
        }
    }

  info->open = FALSE;

  if (! info->toplevel_entry || info->toplevel_entry->remember_if_open)
    {
      GimpDialogVisibilityState visibility;

      visibility =
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info->widget),
                                            GIMP_DIALOG_VISIBILITY_KEY));

      switch (visibility)
        {
        case GIMP_DIALOG_VISIBILITY_UNKNOWN:
          info->open = GTK_WIDGET_VISIBLE (info->widget);
          break;

        case GIMP_DIALOG_VISIBILITY_INVISIBLE:
          info->open = FALSE;
          break;

        case GIMP_DIALOG_VISIBILITY_VISIBLE:
          info->open = TRUE;
          break;
        }
    }

  info->screen = DEFAULT_SCREEN;

  if (info->open)
    {
      GdkDisplay *display = gtk_widget_get_display (info->widget);
      GdkScreen  *screen  = gtk_widget_get_screen (info->widget);

      if (screen != gdk_display_get_default_screen (display))
        info->screen = gdk_screen_get_number (screen);
    }
}


/*  private functions  */

static GTokenType
session_info_dock_deserialize (GScanner        *scanner,
                               GimpSessionInfo *info)
{
  GTokenType token = G_TOKEN_LEFT_PAREN;

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
            case SESSION_INFO_BOOK:
              g_scanner_set_scope (scanner, SESSION_INFO_BOOK);
              token = session_info_book_deserialize (scanner, info);

              if (token == G_TOKEN_LEFT_PAREN)
                g_scanner_set_scope (scanner, SESSION_INFO_DOCK);
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

  return token;
}

static GTokenType
session_info_book_deserialize (GScanner        *scanner,
                               GimpSessionInfo *info)
{
  GimpSessionInfoBook *book;
  GTokenType           token;

  book = g_new0 (GimpSessionInfoBook, 1);

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

            case SESSION_INFO_DOCKABLE:
              g_scanner_set_scope (scanner, SESSION_INFO_DOCKABLE);
              token = session_info_dockable_deserialize (scanner, book);

              if (token == G_TOKEN_LEFT_PAREN)
                g_scanner_set_scope (scanner, SESSION_INFO_BOOK);
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

  return token;

 error:
  gimp_session_info_book_free (book);

  return token;
}

static GTokenType
session_info_dockable_deserialize (GScanner            *scanner,
                                   GimpSessionInfoBook *book)
{
  GimpSessionInfoDockable *dockable;
  GEnumClass              *enum_class;
  GEnumValue              *enum_value;
  GTokenType               token;

  dockable = g_new0 (GimpSessionInfoDockable, 1);

  enum_class = g_type_class_ref (GIMP_TYPE_TAB_STYLE);

  token = G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &dockable->identifier))
    goto error;

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
            case SESSION_INFO_DOCKABLE_TAB_STYLE:
              token = G_TOKEN_IDENTIFIER;
              if (g_scanner_peek_next_token (scanner) != token)
                goto error;

              g_scanner_get_next_token (scanner);

              enum_value = g_enum_get_value_by_nick (enum_class,
                                                     scanner->value.v_identifier);

              if (! enum_value)
                enum_value = g_enum_get_value_by_name (enum_class,
                                                       scanner->value.v_identifier);

              if (enum_value)
                dockable->tab_style = enum_value->value;
              break;

            case SESSION_INFO_DOCKABLE_VIEW_SIZE:
              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &dockable->view_size))
                goto error;
              break;

            case SESSION_INFO_DOCKABLE_AUX:
              token = session_info_aux_deserialize (scanner,
                                                    &dockable->aux_info);
              if (token != G_TOKEN_LEFT_PAREN)
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

  book->dockables = g_list_append (book->dockables, dockable);
  g_type_class_unref (enum_class);

  return token;

 error:
  gimp_session_info_dockable_free (dockable);
  g_type_class_unref (enum_class);

  return token;
}

static GTokenType
session_info_aux_deserialize (GScanner  *scanner,
                              GList    **aux_list)
{
  GimpSessionInfoAux *aux_info = NULL;
  GTokenType          token    = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_IDENTIFIER;
          break;

        case G_TOKEN_IDENTIFIER:
          {
            aux_info = g_slice_new0 (GimpSessionInfoAux);

            aux_info->name = g_strdup (scanner->value.v_identifier);

            token = G_TOKEN_STRING;
            if (g_scanner_peek_next_token (scanner) != token)
              goto error;

            if (! gimp_scanner_parse_string (scanner, &aux_info->value))
              goto error;

            *aux_list = g_list_append (*aux_list, aux_info);
            aux_info = NULL;
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

  return token;

 error:
  if (aux_info)
    gimp_session_info_aux_free (aux_info);

  return token;
}

static void
session_info_set_aux_info (GtkWidget *dialog,
                           GList     *aux_info)
{
  /* FIXME: make the aux-info stuff generic */

  if (GIMP_IS_DOCK (dialog))
    gimp_dock_set_aux_info (GIMP_DOCK (dialog), aux_info);
  else if (GIMP_IS_DOCKABLE (dialog))
    gimp_dockable_set_aux_info (GIMP_DOCKABLE (dialog), aux_info);
}

static GList *
session_info_get_aux_info (GtkWidget *dialog)
{
  /* FIXME: make the aux-info stuff generic */

  if (GIMP_IS_DOCK (dialog))
    return gimp_dock_get_aux_info (GIMP_DOCK (dialog));
  else if (GIMP_IS_DOCKABLE (dialog))
    return gimp_dockable_get_aux_info (GIMP_DOCKABLE (dialog));

  return NULL;
}
