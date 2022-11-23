/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaDBusService
 * Copyright (C) 2007, 2008 Sven Neumann <sven@ligma.org>
 * Copyright (C) 2013       Michael Natterer <mitch@ligma.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "gui-types.h"

#include "core/ligma.h"
#include "core/ligma-batch.h"
#include "core/ligmacontainer.h"

#include "file/file-open.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "ligmadbusservice.h"


typedef struct
{
  GFile    *file;
  gboolean  as_new;

  gchar    *interpreter;
  gchar    *command;
} IdleData;

static void       ligma_dbus_service_ui_iface_init  (LigmaDBusServiceUIIface *iface);

static void       ligma_dbus_service_dispose        (GObject               *object);
static void       ligma_dbus_service_finalize       (GObject               *object);

static gboolean   ligma_dbus_service_activate       (LigmaDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation);
static gboolean   ligma_dbus_service_open           (LigmaDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation,
                                                    const gchar           *uri);

static gboolean   ligma_dbus_service_open_as_new    (LigmaDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation,
                                                    const gchar           *uri);

static gboolean   ligma_dbus_service_batch_run      (LigmaDBusServiceUI     *service,
                                                    GDBusMethodInvocation *invocation,
                                                    const gchar           *batch_interpreter,
                                                    const gchar           *batch_command);

static void       ligma_dbus_service_ligma_opened    (Ligma                  *ligma,
                                                    GFile                 *file,
                                                    LigmaDBusService       *service);

static gboolean   ligma_dbus_service_queue_open     (LigmaDBusService       *service,
                                                    const gchar           *uri,
                                                    gboolean               as_new);
static gboolean   ligma_dbus_service_queue_batch    (LigmaDBusService       *service,
                                                    const gchar           *interpreter,
                                                    const gchar           *command);

static gboolean   ligma_dbus_service_process_idle   (LigmaDBusService       *service);
static IdleData * ligma_dbus_service_open_data_new  (LigmaDBusService       *service,
                                                    const gchar           *uri,
                                                    gboolean               as_new);
static IdleData * ligma_dbus_service_batch_data_new (LigmaDBusService       *service,
                                                    const gchar           *interpreter,
                                                    const gchar           *command);
static void       ligma_dbus_service_idle_data_free (IdleData              *data);


G_DEFINE_TYPE_WITH_CODE (LigmaDBusService, ligma_dbus_service,
                         LIGMA_DBUS_SERVICE_TYPE_UI_SKELETON,
                         G_IMPLEMENT_INTERFACE (LIGMA_DBUS_SERVICE_TYPE_UI,
                                                ligma_dbus_service_ui_iface_init))

#define parent_class ligma_dbus_service_parent_class


static void
ligma_dbus_service_class_init (LigmaDBusServiceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = ligma_dbus_service_dispose;
  object_class->finalize = ligma_dbus_service_finalize;
}

static void
ligma_dbus_service_init (LigmaDBusService *service)
{
  service->queue = g_queue_new ();
}

static void
ligma_dbus_service_ui_iface_init (LigmaDBusServiceUIIface *iface)
{
  iface->handle_activate    = ligma_dbus_service_activate;
  iface->handle_open        = ligma_dbus_service_open;
  iface->handle_open_as_new = ligma_dbus_service_open_as_new;
  iface->handle_batch_run   = ligma_dbus_service_batch_run;
}

GObject *
ligma_dbus_service_new (Ligma *ligma)
{
  LigmaDBusService *service;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  service = g_object_new (LIGMA_TYPE_DBUS_SERVICE, NULL);

  service->ligma = ligma;

  g_signal_connect_object (ligma, "image-opened",
                           G_CALLBACK (ligma_dbus_service_ligma_opened),
                           service, 0);

  return G_OBJECT (service);
}

static void
ligma_dbus_service_dispose (GObject *object)
{
  LigmaDBusService *service = LIGMA_DBUS_SERVICE (object);

  if (service->source)
    {
      g_source_remove (g_source_get_id (service->source));
      service->source = NULL;
      service->timeout_source = FALSE;
    }

  while (! g_queue_is_empty (service->queue))
    {
      ligma_dbus_service_idle_data_free (g_queue_pop_head (service->queue));
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_dbus_service_finalize (GObject *object)
{
  LigmaDBusService *service = LIGMA_DBUS_SERVICE (object);

  if (service->queue)
    {
      g_queue_free (service->queue);
      service->queue = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

gboolean
ligma_dbus_service_activate (LigmaDBusServiceUI     *service,
                            GDBusMethodInvocation *invocation)
{
  Ligma *ligma = LIGMA_DBUS_SERVICE (service)->ligma;

  /*  We want to be called again later in case that LIGMA is not fully
   *  started yet.
   */
  if (ligma_is_restored (ligma))
    {
      LigmaObject *display;

      display = ligma_container_get_first_child (ligma->displays);

      if (display)
        ligma_display_shell_present (ligma_display_get_shell (LIGMA_DISPLAY (display)));
    }

  ligma_dbus_service_ui_complete_activate (service, invocation);

  return TRUE;
}

gboolean
ligma_dbus_service_open (LigmaDBusServiceUI     *service,
                        GDBusMethodInvocation *invocation,
                        const gchar           *uri)
{
  gboolean success;

  success = ligma_dbus_service_queue_open (LIGMA_DBUS_SERVICE (service),
                                          uri, FALSE);

  ligma_dbus_service_ui_complete_open (service, invocation, success);

  return TRUE;
}

gboolean
ligma_dbus_service_open_as_new (LigmaDBusServiceUI     *service,
                               GDBusMethodInvocation *invocation,
                               const gchar           *uri)
{
  gboolean success;

  success = ligma_dbus_service_queue_open (LIGMA_DBUS_SERVICE (service),
                                          uri, TRUE);

  ligma_dbus_service_ui_complete_open_as_new (service, invocation, success);

  return TRUE;
}

gboolean
ligma_dbus_service_batch_run (LigmaDBusServiceUI     *service,
                             GDBusMethodInvocation *invocation,
                             const gchar           *batch_interpreter,
                             const gchar           *batch_command)
{
  gboolean success;

  success = ligma_dbus_service_queue_batch (LIGMA_DBUS_SERVICE (service),
                                           batch_interpreter,
                                           batch_command);

  ligma_dbus_service_ui_complete_batch_run (service, invocation, success);

  return TRUE;
}

static void
ligma_dbus_service_ligma_opened (Ligma            *ligma,
                               GFile           *file,
                               LigmaDBusService *service)
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
ligma_dbus_service_queue_open (LigmaDBusService *service,
                              const gchar     *uri,
                              gboolean         as_new)
{
  g_queue_push_tail (service->queue,
                     ligma_dbus_service_open_data_new (service, uri, as_new));

  if (! service->source)
    {
      service->source = g_idle_source_new ();
      service->timeout_source = FALSE;

      g_source_set_priority (service->source, G_PRIORITY_LOW);
      g_source_set_callback (service->source,
                             (GSourceFunc) ligma_dbus_service_process_idle,
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
ligma_dbus_service_queue_batch (LigmaDBusService *service,
                               const gchar     *interpreter,
                               const gchar     *command)
{
  g_queue_push_tail (service->queue,
                     ligma_dbus_service_batch_data_new (service,
                                                       interpreter,
                                                       command));

  if (! service->source)
    {
      service->source = g_idle_source_new ();
      service->timeout_source = FALSE;

      g_source_set_priority (service->source, G_PRIORITY_LOW);
      g_source_set_callback (service->source,
                             (GSourceFunc) ligma_dbus_service_process_idle,
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
ligma_dbus_service_process_idle (LigmaDBusService *service)
{
  IdleData *data;

  if (! ligma_is_restored (service->ligma))
    {
      if (! service->timeout_source)
        {
          /* We are probably starting the program. No need to spam LIGMA with
           * an idle handler (which might make LIGMA slower to start even
           * with low priority).
           * Instead let's add a timeout of half a second.
           */
          service->source = g_timeout_source_new (500);
          service->timeout_source = TRUE;

          g_source_set_priority (service->source, G_PRIORITY_LOW);
          g_source_set_callback (service->source,
                                 (GSourceFunc) ligma_dbus_service_process_idle,
                                 service,
                                 NULL);
          g_source_attach (service->source, NULL);
          g_source_unref (service->source);

          return G_SOURCE_REMOVE;
        }
      else
        {
          return G_SOURCE_CONTINUE;
        }
    }

  /* Process data as a FIFO. */
  data = g_queue_pop_head (service->queue);

  if (data)
    {
      if (data->file)
        file_open_from_command_line (service->ligma, data->file, data->as_new,
                                     NULL /* FIXME monitor */);

      if (data->command)
        {
          const gchar *commands[2] = {data->command, 0};

          ligma_batch_run (service->ligma, data->interpreter,
                          commands);
        }

      ligma_dbus_service_idle_data_free (data);

      if (service->timeout_source)
        {
          /* Now LIGMA is fully functional and can respond quickly to
           * DBus calls. Switch to a usual idle source.
           */
          service->source = g_idle_source_new ();
          service->timeout_source = FALSE;

          g_source_set_priority (service->source, G_PRIORITY_LOW);
          g_source_set_callback (service->source,
                                 (GSourceFunc) ligma_dbus_service_process_idle,
                                 service,
                                 NULL);
          g_source_attach (service->source, NULL);
          g_source_unref (service->source);

          return G_SOURCE_REMOVE;
        }
      else
        {
          return G_SOURCE_CONTINUE;
        }
    }

  service->source = NULL;

  return G_SOURCE_REMOVE;
}

static IdleData *
ligma_dbus_service_open_data_new (LigmaDBusService *service,
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
ligma_dbus_service_batch_data_new (LigmaDBusService *service,
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
ligma_dbus_service_idle_data_free (IdleData *data)
{
  if (data->file)
    g_object_unref (data->file);

  if (data->command)
    g_free (data->command);
  if (data->interpreter)
    g_free (data->interpreter);

  g_slice_free (IdleData, data);
}
