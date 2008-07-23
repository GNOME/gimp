/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo.c
 * Copyright (C) 2001-2008 Michael Natterer <mitch@gimp.org>
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
#include "gimpsessioninfo-aux.h"
#include "gimpsessioninfo-book.h"
#include "gimpsessioninfo-dock.h"


enum
{
  SESSION_INFO_POSITION,
  SESSION_INFO_SIZE,
  SESSION_INFO_OPEN,
  SESSION_INFO_AUX,
  SESSION_INFO_DOCK
};

#define DEFAULT_SCREEN  -1


static void     gimp_session_info_config_iface_init (GimpConfigInterface *iface);

static void     gimp_session_info_finalize          (GObject          *object);

static gint64   gimp_session_info_get_memsize       (GimpObject       *object,
                                                     gint64           *gui_size);

static gboolean gimp_session_info_serialize         (GimpConfig       *config,
                                                     GimpConfigWriter *writer,
                                                     gpointer          data);
static gboolean gimp_session_info_deserialize       (GimpConfig       *config,
                                                     GScanner         *scanner,
                                                     gint              nest_level,
                                                     gpointer          data);


G_DEFINE_TYPE_WITH_CODE (GimpSessionInfo, gimp_session_info, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_session_info_config_iface_init))

#define parent_class gimp_session_info_parent_class


static void
gimp_session_info_class_init (GimpSessionInfoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_session_info_finalize;

  gimp_object_class->get_memsize = gimp_session_info_get_memsize;
}

static void
gimp_session_info_init (GimpSessionInfo *info)
{
  info->screen = DEFAULT_SCREEN;
}

static void
gimp_session_info_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize   = gimp_session_info_serialize;
  iface->deserialize = gimp_session_info_deserialize;
}

static void
gimp_session_info_finalize (GObject *object)
{
  GimpSessionInfo *info = GIMP_SESSION_INFO (object);

  gimp_session_info_clear_info (info);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_session_info_get_memsize (GimpObject *object,
                               gint64     *gui_size)
{
#if 0
  GimpSessionInfo *info    = GIMP_SESSION_INFO (object);
#endif
  gint64           memsize = 0;

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_session_info_serialize (GimpConfig       *config,
                             GimpConfigWriter *writer,
                             gpointer          data)
{
  GimpSessionInfo *info = GIMP_SESSION_INFO (config);

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

  if (info->aux_info)
    gimp_session_info_aux_serialize (writer, info->aux_info);

  if (info->books)
    gimp_session_info_dock_serialize (writer, info->books);

  return TRUE;
}

/*
 * This function is just like gimp_scanner_parse_int(), but it is allows
 * to detect the special value '-0'. This is used as in X geometry strings.
 */
static gboolean
gimp_session_info_parse_offset (GScanner *scanner,
                                gint     *dest,
                                gboolean *negative)
{
  if (g_scanner_peek_next_token (scanner) == '-')
    {
      *negative = TRUE;
      g_scanner_get_next_token (scanner);
    }
  else
    {
      *negative = FALSE;
    }

  if (g_scanner_peek_next_token (scanner) != G_TOKEN_INT)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (*negative)
    *dest = -scanner->value.v_int64;
  else
    *dest = scanner->value.v_int64;

  return TRUE;
}

static gboolean
gimp_session_info_deserialize (GimpConfig *config,
                               GScanner   *scanner,
                               gint        nest_level,
                               gpointer    data)
{
  GimpSessionInfo *info = GIMP_SESSION_INFO (config);
  GTokenType       token;
  guint            scope_id;
  guint            old_scope_id;

  scope_id = g_type_qname (G_TYPE_FROM_INSTANCE (config));
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  g_scanner_scope_add_symbol (scanner, scope_id, "position",
                              GINT_TO_POINTER (SESSION_INFO_POSITION));
  g_scanner_scope_add_symbol (scanner, scope_id, "size",
                              GINT_TO_POINTER (SESSION_INFO_SIZE));
  g_scanner_scope_add_symbol (scanner, scope_id, "open-on-exit",
                              GINT_TO_POINTER (SESSION_INFO_OPEN));
  g_scanner_scope_add_symbol (scanner, scope_id, "aux-info",
                              GINT_TO_POINTER (SESSION_INFO_AUX));
  g_scanner_scope_add_symbol (scanner, scope_id, "dock",
                              GINT_TO_POINTER (SESSION_INFO_DOCK));

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
              if (! gimp_session_info_parse_offset (scanner,
                                                    &info->x,
                                                    &info->right_align))
                goto error;
              if (! gimp_session_info_parse_offset (scanner,
                                                    &info->y,
                                                    &info->bottom_align))
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

              g_scanner_set_scope (scanner, scope_id + 1);
              token = gimp_session_info_dock_deserialize (scanner, scope_id + 1,
                                                          info);

              if (token == G_TOKEN_LEFT_PAREN)
                g_scanner_set_scope (scanner, scope_id);
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

 error:

  g_scanner_scope_remove_symbol (scanner, scope_id, "position");
  g_scanner_scope_remove_symbol (scanner, scope_id, "size");
  g_scanner_scope_remove_symbol (scanner, scope_id, "open-on-exit");
  g_scanner_scope_remove_symbol (scanner, scope_id, "aux-info");
  g_scanner_scope_remove_symbol (scanner, scope_id, "dock");

  g_scanner_set_scope (scanner, old_scope_id);

  return gimp_config_deserialize_return (scanner, token, nest_level);
}


/*  public functions  */

GimpSessionInfo *
gimp_session_info_new (void)
{
  return g_object_new (GIMP_TYPE_SESSION_INFO, NULL);
}

void
gimp_session_info_restore (GimpSessionInfo   *info,
                           GimpDialogFactory *factory)
{
  GdkDisplay *display;
  GdkScreen  *screen = NULL;

  g_return_if_fail (GIMP_IS_SESSION_INFO (info));
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
        gimp_session_info_aux_set_list (dialog, info->aux_info);
    }
  else
    {
      gimp_session_info_dock_restore (info, factory, screen);
    }
}

/* This function mostly lifted from
 * gtk+/gdk/gdkscreen.c:gdk_screen_get_monitor_at_window()
 */
static gint
gimp_session_info_get_appropriate_monitor (GdkScreen *screen,
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
  GdkRectangle rect;
  gchar        geom[32];
  gint         monitor;
  gboolean     use_size;

  g_return_if_fail (GIMP_IS_SESSION_INFO (info));
  g_return_if_fail (GTK_IS_WINDOW (info->widget));

  screen = gtk_widget_get_screen (info->widget);

  use_size = ((! info->toplevel_entry || info->toplevel_entry->remember_size) &&
              (info->width > 0 && info->height > 0));

  if (use_size)
    {
      monitor = gimp_session_info_get_appropriate_monitor (screen,
                                                           info->x,
                                                           info->y,
                                                           info->width,
                                                           info->height);
    }
  else
    {
      monitor = gdk_screen_get_monitor_at_point (screen, info->x, info->y);
    }

  gdk_screen_get_monitor_geometry (screen, monitor, &rect);

  info->x = CLAMP (info->x,
                   rect.x,
                   rect.x + rect.width - (info->width > 0 ?
                                          info->width : 128));
  info->y = CLAMP (info->y,
                   rect.y,
                   rect.y + rect.height - (info->height > 0 ?
                                           info->height : 128));

  if (info->right_align && info->bottom_align)
    {
      g_strlcpy (geom, "-0-0", sizeof (geom));
    }
  else if (info->right_align)
    {
      g_snprintf (geom, sizeof (geom), "-0%+d", info->y);
    }
  else if (info->bottom_align)
    {
      g_snprintf (geom, sizeof (geom), "%+d-0", info->x);
    }
  else
    {
      g_snprintf (geom, sizeof (geom), "%+d%+d", info->x, info->y);
    }

  gtk_window_parse_geometry (GTK_WINDOW (info->widget), geom);

  if (use_size)
    gtk_window_set_default_size (GTK_WINDOW (info->widget),
                                 info->width, info->height);
}

void
gimp_session_info_get_geometry (GimpSessionInfo *info)
{
  g_return_if_fail (GIMP_IS_SESSION_INFO (info));
  g_return_if_fail (GTK_IS_WINDOW (info->widget));

  if (info->widget->window)
    {
      gint x, y;

      gdk_window_get_root_origin (info->widget->window, &x, &y);

      /* Don't write negative values to the sessionrc, they are
       * interpreted as relative to the right, respective bottom edge
       * of the screen.
       */
      info->x = MAX (0, x);
      info->y = MAX (0, y);

      if (! info->toplevel_entry || info->toplevel_entry->remember_size)
        {
          gdk_drawable_get_size (GDK_DRAWABLE (info->widget->window),
                                 &info->width, &info->height);
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

void
gimp_session_info_get_info (GimpSessionInfo *info)
{
  g_return_if_fail (GIMP_IS_SESSION_INFO (info));
  g_return_if_fail (GTK_IS_WIDGET (info->widget));

  gimp_session_info_get_geometry (info);

  info->aux_info = gimp_session_info_aux_get_list (info->widget);

  if (! info->toplevel_entry)
    info->books = gimp_session_info_dock_from_widget (GIMP_DOCK (info->widget));
}

void
gimp_session_info_clear_info (GimpSessionInfo *info)
{
  g_return_if_fail (GIMP_IS_SESSION_INFO (info));

  if (info->aux_info)
    {
      g_list_foreach (info->aux_info,
                      (GFunc) gimp_session_info_aux_free, NULL);
      g_list_free (info->aux_info);
      info->aux_info = NULL;
    }

   if (info->books)
     {
       g_list_foreach (info->books,
                       (GFunc) gimp_session_info_book_free, NULL);
       g_list_free (info->books);
       info->books = NULL;
     }
}
