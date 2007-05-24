/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo.c
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpsessioninfo.h"
#include "gimpsessioninfoaux.h"
#include "gimpsessioninfobook.h"
#include "gimpsessioninfodock.h"


enum
{
  SESSION_INFO_POSITION,
  SESSION_INFO_SIZE,
  SESSION_INFO_OPEN,
  SESSION_INFO_AUX,
  SESSION_INFO_DOCK
};

#define DEFAULT_SCREEN  -1


/*  public functions  */

GimpSessionInfo *
gimp_session_info_new (void)
{
  return g_slice_new0 (GimpSessionInfo);
}

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

   g_slice_free (GimpSessionInfo, info);
}

void
gimp_session_info_serialize (GimpConfigWriter *writer,
                             GimpSessionInfo  *info,
                             const gchar      *factory_name)
{
  const gchar *dialog_name;

  g_return_if_fail (info != NULL);
  g_return_if_fail (factory_name != NULL);
  g_return_if_fail (writer != NULL);

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
      gimp_session_info_aux_serialize (writer, info->widget);

      if (! info->toplevel_entry)
        gimp_session_info_dock_serialize (writer, GIMP_DOCK (info->widget));
    }

  gimp_config_writer_close (writer);  /* session-info */
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

  token = G_TOKEN_STRING;

  if (! gimp_scanner_parse_string (scanner, &factory_name))
    goto error;

  factory = gimp_dialog_factory_from_name (factory_name);
  g_free (factory_name);

  if (! factory)
    goto error;

  if (! gimp_scanner_parse_string (scanner, &entry_name))
    goto error;

  info = gimp_session_info_new ();

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
              token = gimp_session_info_aux_deserialize (scanner,
                                                         &info->aux_info);
              if (token != G_TOKEN_LEFT_PAREN)
                goto error;
              break;

            case SESSION_INFO_DOCK:
              if (info->toplevel_entry)
                goto error;

              g_scanner_set_scope (scanner, scope + 1);
              token = gimp_session_info_dock_deserialize (scanner, scope + 1,
                                                          info);

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

  return token;
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
        gimp_session_info_set_aux_info (dialog, info->aux_info);
    }
  else
    {
      gimp_session_info_dock_restore (info, factory, screen);
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
