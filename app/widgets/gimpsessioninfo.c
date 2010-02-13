/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo.c
 * Copyright (C) 2001-2008 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "dialogs/dialogs.h"

#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockwindow.h"
#include "gimpsessioninfo.h"
#include "gimpsessioninfo-aux.h"
#include "gimpsessioninfo-book.h"
#include "gimpsessioninfo-dock.h"
#include "gimpsessioninfo-private.h"

#include "gimp-log.h"


enum
{
  SESSION_INFO_POSITION,
  SESSION_INFO_SIZE,
  SESSION_INFO_OPEN,
  SESSION_INFO_AUX,
  SESSION_INFO_DOCK,
  SESSION_INFO_GIMP_DOCK,
  SESSION_INFO_GIMP_TOOLBOX
};

#define DEFAULT_SCREEN  -1


static void      gimp_session_info_config_iface_init  (GimpConfigInterface *iface);
static void      gimp_session_info_finalize           (GObject             *object);
static gint64    gimp_session_info_get_memsize        (GimpObject          *object,
                                                       gint64              *gui_size);
static gboolean  gimp_session_info_serialize          (GimpConfig          *config,
                                                       GimpConfigWriter    *writer,
                                                       gpointer             data);
static gboolean  gimp_session_info_deserialize        (GimpConfig          *config,
                                                       GScanner            *scanner,
                                                       gint                 nest_level,
                                                       gpointer             data);
static gboolean  gimp_session_info_is_for_dock_window (GimpSessionInfo     *info);


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

  g_type_class_add_private (klass, sizeof (GimpSessionInfoPrivate));
}

static void
gimp_session_info_init (GimpSessionInfo *info)
{
  info->p = G_TYPE_INSTANCE_GET_PRIVATE (info,
                                         GIMP_TYPE_SESSION_INFO,
                                         GimpSessionInfoPrivate);
  info->p->screen = DEFAULT_SCREEN;
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
  GList           *iter = NULL;

  gimp_config_writer_open (writer, "position");
  gimp_config_writer_printf (writer, "%d %d", info->p->x, info->p->y);
  gimp_config_writer_close (writer);

  if (info->p->width > 0 && info->p->height > 0)
    {
      gimp_config_writer_open (writer, "size");
      gimp_config_writer_printf (writer, "%d %d", info->p->width, info->p->height);
      gimp_config_writer_close (writer);
    }

  if (info->p->open)
    {
      gimp_config_writer_open (writer, "open-on-exit");

      if (info->p->screen != DEFAULT_SCREEN)
        gimp_config_writer_printf (writer, "%d", info->p->screen);

      gimp_config_writer_close (writer);
    }

  if (info->p->aux_info)
    gimp_session_info_aux_serialize (writer, info->p->aux_info);

  for (iter = info->p->docks; iter; iter = g_list_next (iter))
    gimp_session_info_dock_serialize (writer, iter->data);

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
  g_scanner_scope_add_symbol (scanner, scope_id, "gimp-dock",
                              GINT_TO_POINTER (SESSION_INFO_GIMP_DOCK));
  g_scanner_scope_add_symbol (scanner, scope_id, "gimp-toolbox",
                              GINT_TO_POINTER (SESSION_INFO_GIMP_TOOLBOX));

  /* For sessionrc files from version <= GIMP 2.6 */
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
                                                    &info->p->x,
                                                    &info->p->right_align))
                goto error;
              if (! gimp_session_info_parse_offset (scanner,
                                                    &info->p->y,
                                                    &info->p->bottom_align))
                goto error;
              break;

            case SESSION_INFO_SIZE:
              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &info->p->width))
                goto error;
              if (! gimp_scanner_parse_int (scanner, &info->p->height))
                goto error;
              break;

            case SESSION_INFO_OPEN:
              info->p->open = TRUE;

              /*  the screen number is optional  */
              if (g_scanner_peek_next_token (scanner) == G_TOKEN_RIGHT_PAREN)
                break;

              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &info->p->screen))
                goto error;
              break;

            case SESSION_INFO_AUX:
              token = gimp_session_info_aux_deserialize (scanner,
                                                         &info->p->aux_info);
              if (token != G_TOKEN_LEFT_PAREN)
                goto error;
              break;

            case SESSION_INFO_GIMP_TOOLBOX:
            case SESSION_INFO_GIMP_DOCK:
            case SESSION_INFO_DOCK:
              {
                GimpSessionInfoDock *dock_info  = NULL;
                const gchar         *identifier = NULL;

                /* Handle old sessionrc:s from versions <= GIMP 2.6 */
                if (GPOINTER_TO_INT (scanner->value.v_symbol) == SESSION_INFO_DOCK &&
                    info->p->factory_entry &&
                    info->p->factory_entry->identifier &&
                    strcmp ("gimp-toolbox-window", info->p->factory_entry->identifier) == 0)
                  {
                    identifier = "gimp-toolbox";
                  }
                else
                  {
                    identifier = ((GPOINTER_TO_INT (scanner->value.v_symbol) ==
                                   SESSION_INFO_GIMP_TOOLBOX) ?
                                  "gimp-toolbox" :
                                  "gimp-dock");
                  }

                g_scanner_set_scope (scanner, scope_id + 1);
                token = gimp_session_info_dock_deserialize (scanner, scope_id + 1,
                                                            &dock_info,
                                                            identifier);

                if (token == G_TOKEN_LEFT_PAREN)
                  {
                    g_scanner_set_scope (scanner, scope_id);
                    info->p->docks = g_list_append (info->p->docks, dock_info);
                  }
                else
                  goto error;
              }
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

  /* If we don't have docks, assume it is a toolbox dock window from a
   * sessionrc file from GIMP <= 2.6 and add a toolbox dock manually
   */
  if (! info->p->docks &&
      strcmp ("gimp-toolbox-window",
              info->p->factory_entry->identifier) == 0)
    {
      info->p->docks =
        g_list_append (info->p->docks,
                       gimp_session_info_dock_new ("gimp-toolbox"));
    }

  g_scanner_scope_remove_symbol (scanner, scope_id, "position");
  g_scanner_scope_remove_symbol (scanner, scope_id, "size");
  g_scanner_scope_remove_symbol (scanner, scope_id, "open-on-exit");
  g_scanner_scope_remove_symbol (scanner, scope_id, "aux-info");
  g_scanner_scope_remove_symbol (scanner, scope_id, "gimp-dock");
  g_scanner_scope_remove_symbol (scanner, scope_id, "gimp-toolbox");
  g_scanner_scope_remove_symbol (scanner, scope_id, "dock");

  g_scanner_set_scope (scanner, old_scope_id);

  return gimp_config_deserialize_return (scanner, token, nest_level);
}

/**
 * gimp_session_info_is_for_dock_window:
 * @info:
 *
 * Helper function to determine if the session info is for a dock. It
 * uses the dialog factory entry state and the associated widget state
 * if any to determine that.
 *
 * Returns: %TRUE if session info is for a dock, %FALSE otherwise.
 **/
static gboolean
gimp_session_info_is_for_dock_window (GimpSessionInfo *info)
{
  gboolean entry_state_for_dock  =  info->p->factory_entry == NULL;
  gboolean widget_state_for_dock = (info->p->widget == NULL ||
                                    GIMP_IS_DOCK_WINDOW (info->p->widget));

  return entry_state_for_dock && widget_state_for_dock;
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
  GtkWidget  *dialog  = NULL;
  GdkDisplay *display = NULL;
  GdkScreen  *screen  = NULL;
  GList      *iter    = NULL;

  g_return_if_fail (GIMP_IS_SESSION_INFO (info));
  g_return_if_fail (GIMP_IS_DIALOG_FACTORY (factory));

  display = gdk_display_get_default ();

  if (info->p->screen != DEFAULT_SCREEN)
    screen = gdk_display_get_screen (display, info->p->screen);

  if (! screen)
    screen = gdk_display_get_default_screen (display);

  info->p->open   = FALSE;
  info->p->screen = DEFAULT_SCREEN;

  if (info->p->factory_entry &&
      ! info->p->factory_entry->dockable)
    {
      GIMP_LOG (DIALOG_FACTORY, "restoring toplevel \"%s\" (info %p)",
                info->p->factory_entry->identifier,
                info);

      dialog =
        gimp_dialog_factory_dialog_new (factory, screen,
                                        info->p->factory_entry->identifier,
                                        info->p->factory_entry->view_size,
                                        TRUE);

      if (dialog && info->p->aux_info)
        gimp_session_info_aux_set_list (dialog, info->p->aux_info);
    }

  /* We expect expect there to always be docks. In sessionrc files
   * from <= 2.6 not all dock window entries had dock entries, but we
   * take care of that during sessionrc parsing
   */
  for (iter = info->p->docks; iter; iter = g_list_next (iter))
    gimp_session_info_dock_restore ((GimpSessionInfoDock *)iter->data,
                                    factory,
                                    screen,
                                    GIMP_DOCK_WINDOW (dialog));

  gtk_widget_show (dialog);
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

/**
 * gimp_session_info_apply_geometry:
 * @info:
 *
 * Apply the geometry stored in the session info object to the
 * associated widget.
 **/
void
gimp_session_info_apply_geometry (GimpSessionInfo *info)
{
  GdkScreen   *screen;
  GdkRectangle rect;
  gchar        geom[32];
  gint         monitor;
  gboolean     use_size;

  g_return_if_fail (GIMP_IS_SESSION_INFO (info));
  g_return_if_fail (GTK_IS_WINDOW (info->p->widget));

  screen = gtk_widget_get_screen (info->p->widget);

  use_size = (gimp_session_info_get_remember_size (info) &&
              info->p->width  > 0 &&
              info->p->height > 0);

  if (use_size)
    {
      monitor = gimp_session_info_get_appropriate_monitor (screen,
                                                           info->p->x,
                                                           info->p->y,
                                                           info->p->width,
                                                           info->p->height);
    }
  else
    {
      monitor = gdk_screen_get_monitor_at_point (screen, info->p->x, info->p->y);
    }

  gdk_screen_get_monitor_geometry (screen, monitor, &rect);

  info->p->x = CLAMP (info->p->x,
                   rect.x,
                   rect.x + rect.width - (info->p->width > 0 ?
                                          info->p->width : 128));
  info->p->y = CLAMP (info->p->y,
                   rect.y,
                   rect.y + rect.height - (info->p->height > 0 ?
                                           info->p->height : 128));

  if (info->p->right_align && info->p->bottom_align)
    {
      g_strlcpy (geom, "-0-0", sizeof (geom));
    }
  else if (info->p->right_align)
    {
      g_snprintf (geom, sizeof (geom), "-0%+d", info->p->y);
    }
  else if (info->p->bottom_align)
    {
      g_snprintf (geom, sizeof (geom), "%+d-0", info->p->x);
    }
  else
    {
      g_snprintf (geom, sizeof (geom), "%+d%+d", info->p->x, info->p->y);
    }

  gtk_window_parse_geometry (GTK_WINDOW (info->p->widget), geom);

  if (use_size)
    gtk_window_set_default_size (GTK_WINDOW (info->p->widget),
                                 info->p->width, info->p->height);
}

/**
 * gimp_session_info_read_geometry:
 * @info:
 *
 * Read geometry related information from the associated widget.
 **/
void
gimp_session_info_read_geometry (GimpSessionInfo *info)
{
  GdkWindow *window;

  g_return_if_fail (GIMP_IS_SESSION_INFO (info));
  g_return_if_fail (GTK_IS_WINDOW (info->p->widget));

  window = gtk_widget_get_window (info->p->widget);

  if (window)
    {
      gint x, y;

      gdk_window_get_root_origin (window, &x, &y);

      /* Don't write negative values to the sessionrc, they are
       * interpreted as relative to the right, respective bottom edge
       * of the screen.
       */
      info->p->x = MAX (0, x);
      info->p->y = MAX (0, y);

      if (gimp_session_info_get_remember_size (info))
        {
          gdk_drawable_get_size (GDK_DRAWABLE (window),
                                 &info->p->width, &info->p->height);
        }
      else
        {
          info->p->width  = 0;
          info->p->height = 0;
        }
    }

  info->p->open = FALSE;

  if (gimp_session_info_get_remember_if_open (info))
    {
      GimpDialogVisibilityState visibility;

      visibility =
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info->p->widget),
                                            GIMP_DIALOG_VISIBILITY_KEY));

      switch (visibility)
        {
        case GIMP_DIALOG_VISIBILITY_UNKNOWN:
          info->p->open = gtk_widget_get_visible (info->p->widget);
          break;

        case GIMP_DIALOG_VISIBILITY_INVISIBLE:
          info->p->open = FALSE;
          break;

        case GIMP_DIALOG_VISIBILITY_VISIBLE:
          info->p->open = TRUE;
          break;
        }
    }

  info->p->screen = DEFAULT_SCREEN;

  if (info->p->open)
    {
      GdkDisplay *display = gtk_widget_get_display (info->p->widget);
      GdkScreen  *screen  = gtk_widget_get_screen (info->p->widget);

      if (screen != gdk_display_get_default_screen (display))
        info->p->screen = gdk_screen_get_number (screen);
    }
}

void
gimp_session_info_get_info (GimpSessionInfo *info)
{
  g_return_if_fail (GIMP_IS_SESSION_INFO (info));
  g_return_if_fail (GTK_IS_WIDGET (info->p->widget));

  gimp_session_info_read_geometry (info);

  info->p->aux_info = gimp_session_info_aux_get_list (info->p->widget);

  if (GIMP_IS_DOCK_WINDOW (info->p->widget))
    {
      GList *iter = NULL;

      for (iter = gimp_dock_window_get_docks (GIMP_DOCK_WINDOW (info->p->widget));
           iter;
           iter = g_list_next (iter))
        {
          GimpDock *dock = GIMP_DOCK (iter->data);

          info->p->docks =
            g_list_append (info->p->docks,
                           gimp_session_info_dock_from_widget (dock));
        }
    }
}

void
gimp_session_info_clear_info (GimpSessionInfo *info)
{
  g_return_if_fail (GIMP_IS_SESSION_INFO (info));

  if (info->p->aux_info)
    {
      g_list_foreach (info->p->aux_info,
                      (GFunc) gimp_session_info_aux_free, NULL);
      g_list_free (info->p->aux_info);
      info->p->aux_info = NULL;
    }

   if (info->p->docks)
     {
       g_list_foreach (info->p->docks,
                       (GFunc) gimp_session_info_dock_free, NULL);
       g_list_free (info->p->docks);
       info->p->docks = NULL;
     }
}

gboolean
gimp_session_info_is_singleton (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), FALSE);

  return (! gimp_session_info_is_for_dock_window (info) &&
          info->p->factory_entry &&
          info->p->factory_entry->singleton);
}

gboolean
gimp_session_info_is_session_managed (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), FALSE);

  return (gimp_session_info_is_for_dock_window (info) ||
          (info->p->factory_entry &&
           info->p->factory_entry->session_managed));
}


gboolean
gimp_session_info_get_remember_size (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), FALSE);

  return (gimp_session_info_is_for_dock_window (info) ||
          (info->p->factory_entry &&
           info->p->factory_entry->remember_size));
}

gboolean
gimp_session_info_get_remember_if_open (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), FALSE);

  return (gimp_session_info_is_for_dock_window (info) ||
          (info->p->factory_entry &&
           info->p->factory_entry->remember_if_open));
}

GtkWidget *
gimp_session_info_get_widget (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), FALSE);

  return info->p->widget;
}

void
gimp_session_info_set_widget (GimpSessionInfo *info,
                              GtkWidget       *widget)
{
  g_return_if_fail (GIMP_IS_SESSION_INFO (info));

  info->p->widget = widget;
}

GimpDialogFactoryEntry *
gimp_session_info_get_factory_entry (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), FALSE);

  return info->p->factory_entry;
}

void
gimp_session_info_set_factory_entry (GimpSessionInfo        *info,
                                     GimpDialogFactoryEntry *entry)
{
  g_return_if_fail (GIMP_IS_SESSION_INFO (info));

  info->p->factory_entry = entry;
}

gboolean
gimp_session_info_get_open (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), FALSE);

  return info->p->open;
}

gint
gimp_session_info_get_x (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), 0);

  return info->p->x;
}

gint
gimp_session_info_get_y (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), 0);

  return info->p->y;
}

gint
gimp_session_info_get_width (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), 0);

  return info->p->width;
}

gint
gimp_session_info_get_height (GimpSessionInfo *info)
{
  g_return_val_if_fail (GIMP_IS_SESSION_INFO (info), 0);

  return info->p->height;
}
