/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDBusService
 * Copyright (C) 2007, 2008 Sven Neumann <sven@gimp.org>
 * Copyright (C) 2013       Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimp-batch.h"
#include "core/gimpcontainer.h"

#include "file/file-open.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpdbusservice.h"


typedef struct
{
  GFile    *file;
  gboolean  as_new;

  gchar    *interpreter;
  gchar    *command;
} IdleData;

static void       gimp_dbus_service_ui_iface_init  (GimpDBusServiceUIIface *iface);

static void       gimp_dbus_service_dispose        (GObject               *object);
static void       gimp_dbus_service_finalize       (GObject               *object);

static gboolean   gimp_dbus_service_activate       (GimpDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation);
static gboolean   gimp_dbus_service_open           (GimpDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation,
                                                    const gchar           *uri);

static gboolean   gimp_dbus_service_open_as_new    (GimpDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation,
                                                    const gchar           *uri);

static gboolean   gimp_dbus_service_batch_run      (GimpDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation,
                                                    const gchar           *batch_interpreter,
                                                    const gchar           *batch_command);

static void       gimp_dbus_service_gimp_opened    (Gimp                  *gimp,
                                                    GFile                 *file,
                                                    GimpDBusService       *service);

static gboolean   gimp_dbus_service_queue_open     (GimpDBusService       *service,
                                                    const gchar           *uri,
                                                    gboolean               as_new);
static gboolean   gimp_dbus_service_queue_batch    (GimpDBusService       *service,
                                                    const gchar           *interpreter,
                                                    const gchar           *command);

static gboolean   gimp_dbus_service_process_idle   (GimpDBusService       *service);
static IdleData * gimp_dbus_service_open_data_new  (GimpDBusService       *service,
                                                    const gchar           *uri,
                                                    gboolean               as_new);
static IdleData * gimp_dbus_service_batch_data_new (GimpDBusService       *service,
                                                    const gchar           *interpreter,
                                                    const gchar           *command);
static void       gimp_dbus_service_idle_data_free (IdleData              *data);


G_DEFINE_TYPE_WITH_CODE (GimpDBusService, gimp_dbus_service,
                         GIMP_DBUS_SERVICE_TYPE_UI_SKELETON,
                         G_IMPLEMENT_INTERFACE (GIMP_DBUS_SERVICE_TYPE_UI,
                                                gimp_dbus_service_ui_iface_init))

#define parent_class gimp_dbus_service_parent_class


static void
gimp_dbus_service_class_init (GimpDBusServiceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = gimp_dbus_service_dispose;
  object_class->finalize = gimp_dbus_service_finalize;
}

static void
gimp_dbus_service_init (GimpDBusService *service)
{
  service->queue = g_queue_new ();
}

static void
gimp_dbus_service_ui_iface_init (GimpDBusServiceUIIface *iface)
{
  iface->handle_activate    = gimp_dbus_service_activate;
  iface->handle_open        = gimp_dbus_service_open;
  iface->handle_open_as_new = gimp_dbus_service_open_as_new;
  iface->handle_batch_run   = gimp_dbus_service_batch_run;
}

GObject *
gimp_dbus_service_new (Gimp *gimp)
{
  GimpDBusService *service;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  service = g_object_new (GIMP_TYPE_DBUS_SERVICE, NULL);

  service->gimp = gimp;

  g_signal_connect_object (gimp, "image-opened",
                           G_CALLBACK (gimp_dbus_service_gimp_opened),
                           service, 0);

  return G_OBJECT (service);
}

static void
gimp_dbus_service_dispose (GObject *object)
{
  GimpDBusService *service = GIMP_DBUS_SERVICE (object);

  if (service->source)
    {
      g_source_remove (g_source_get_id (service->source));
      service->source = NULL;
    }

  while (! g_queue_is_empty (service->queue))
    {
      gimp_dbus_service_idle_data_free (g_queue_pop_head (service->queue));
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_dbus_service_finalize (GObject *object)
{
  GimpDBusService *service = GIMP_DBUS_SERVICE (object);

  if (service->queue)
    {
      g_queue_free (service->queue);
      service->queue = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

gboolean
gimp_dbus_service_activate (GimpDBusServiceUI     *service,
                            GDBusMethodInvocation *invocation)
{
  Gimp *gimp = GIMP_DBUS_SERVICE (service)->gimp;

  /*  We want to be called again later in case that GIMP is not fully
   *  started yet.
   */
  if (gimp_is_restored (gimp))
    {
      GimpObject *display;

      display = gimp_container_get_first_child (gimp->displays);

      if (display)
        gimp_display_shell_present (gimp_display_get_shell (GIMP_DISPLAY (display)));
    }

  gimp_dbus_service_ui_complete_activate (service, invocation);

  return TRUE;
}

gboolean
gimp_dbus_service_open (GimpDBusServiceUI     *service,
                        GDBusMethodInvocation *invocation,
                        const gchar           *uri)
{
  gboolean success;

  success = gimp_dbus_service_queue_open (GIMP_DBUS_SERVICE (service),
                                          uri, FALSE);

  gimp_dbus_service_ui_complete_open (service, invocation, success);

  return TRUE;
}

gboolean
gimp_dbus_service_open_as_new (GimpDBusServiceUI     *service,
                               GDBusMethodInvocation *invocation,
                               const gchar           *uri)
{
  gboolean success;

  success = gimp_dbus_service_queue_open (GIMP_DBUS_SERVICE (service),
                                          uri, TRUE);

  gimp_dbus_service_ui_complete_open_as_new (service, invocation, success);

  return TRUE;
}

gboolean
gimp_dbus_service_batch_run (GimpDBusServiceUI     *service,
                             GDBusMethodInvocation *invocation,
                             const gchar           *batch_interpreter,
                             const gchar           *batch_command)
{
  gboolean success;

  success = gimp_dbus_service_queue_batch (GIMP_DBUS_SERVICE (service),
                                           batch_interpreter,
                                           batch_command);

  gimp_dbus_service_ui_complete_batch_run (service, invocation, success);

  return TRUE;
}

static void
gimp_dbus_service_gimp_opened (Gimp            *gimp,
                               GFile           *file,
                               GimpDBusService *service)
{
  gchar *uri = g_file_get_uri (file);

  g_signal_emit_by_name (service, "opened", uri);

  g_free (uri);
}

/*
 * Adds a request to open a file to the end of the queue and
 * starts an idle source if it is not already running.
 */
static gboolean
gimp_dbus_service_queue_open (GimpDBusService *service,
                              const gchar     *uri,
                              gboolean         as_new)
{
  g_queue_push_tail (service->queue,
                     gimp_dbus_service_open_data_new (service, uri, as_new));

  if (! service->source)
    {
      service->source = g_idle_source_new ();

      g_source_set_priority (service->source, G_PRIORITY_LOW);
      g_source_set_callback (service->source,
                             (GSourceFunc) gimp_dbus_service_process_idle,
                             service,
                             NULL);
      g_source_attach (service->source, NULL);
      g_source_unref (service->source);
    }

  /*  The call always succeeds as it is handled in one way or another.
   *  Even presenting an error message is considered success ;-)
   */
  return TRUE;
}

/*
 * Adds a request to run a batch command to the end of the queue and
 * starts an idle source if it is not already running.
 */
static gboolean
gimp_dbus_service_queue_batch (GimpDBusService *service,
                               const gchar     *interpreter,
                               const gchar     *command)
{
  g_queue_push_tail (service->queue,
                     gimp_dbus_service_batch_data_new (service,
                                                       interpreter,
                                                       command));

  if (! service->source)
    {
      service->source = g_idle_source_new ();

      g_source_set_priority (service->source, G_PRIORITY_LOW);
      g_source_set_callback (service->source,
                             (GSourceFunc) gimp_dbus_service_process_idle,
                             service,
                             NULL);
      g_source_attach (service->source, NULL);
      g_source_unref (service->source);
    }

  /*  The call always succeeds as it is handled in one way or another.
   *  Even presenting an error message is considered success ;-)
   */
  return TRUE;
}

/*
 * Idle callback that removes the first request from the queue and
 * handles it. If there are no more requests, the idle source is
 * removed.
 */
static gboolean
gimp_dbus_service_process_idle (GimpDBusService *service)
{
  IdleData *data;

  if (! service->gimp->restored)
    return TRUE;

  data = g_queue_pop_tail (service->queue);

  if (data)
    {

      if (data->file)
        file_open_from_command_line (service->gimp, data->file, data->as_new,
                                     NULL /* FIXME monitor */);

      if (data->command)
        {
          const gchar *commands[2] = {data->command, 0};

          gimp_batch_run (service->gimp, data->interpreter,
                          commands);
        }

      gimp_dbus_service_idle_data_free (data);

      return TRUE;
    }

  service->source = NULL;

  return FALSE;
}

static IdleData *
gimp_dbus_service_open_data_new (GimpDBusService *service,
                                 const gchar     *uri,
                                 gboolean         as_new)
{
  IdleData *data    = g_slice_new (IdleData);

  data->file        = g_file_new_for_uri (uri);
  data->as_new      = as_new;
  data->interpreter = NULL;
  data->command     = NULL;

  return data;
}

static IdleData *
gimp_dbus_service_batch_data_new (GimpDBusService *service,
                                  const gchar     *interpreter,
                                  const gchar     *command)
{
  IdleData *data = g_slice_new (IdleData);

  data->file     = NULL;
  data->as_new   = FALSE;

  data->command  = g_strdup (command);

  if (g_strcmp0 (interpreter, "") == 0)
    data->interpreter = NULL;
  else
    data->interpreter = g_strdup (interpreter);

  return data;
}

static void
gimp_dbus_service_idle_data_free (IdleData *data)
{
  if (data->file)
    g_object_unref (data->file);

  if (data->command)
    g_free (data->command);
  if (data->interpreter)
    g_free (data->interpreter);

  g_slice_free (IdleData, data);
}
