/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprogress.c
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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

#include <glib-object.h>

#include "core-types.h"

#include "gimp.h"
#include "gimpmarshal.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


enum
{
  CANCEL,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_progress_iface_base_init (GimpProgressInterface *progress_iface);


static guint progress_signals[LAST_SIGNAL] = { 0 };


GType
gimp_progress_interface_get_type (void)
{
  static GType progress_iface_type = 0;

  if (! progress_iface_type)
    {
      static const GTypeInfo progress_iface_info =
      {
        sizeof (GimpProgressInterface),
        (GBaseInitFunc)     gimp_progress_iface_base_init,
        (GBaseFinalizeFunc) NULL,
      };

      progress_iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                                    "GimpProgressInterface",
                                                    &progress_iface_info,
                                                    0);

      g_type_interface_add_prerequisite (progress_iface_type, G_TYPE_OBJECT);
    }

  return progress_iface_type;
}

static void
gimp_progress_iface_base_init (GimpProgressInterface *progress_iface)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      progress_signals[CANCEL] =
        g_signal_new ("cancel",
                      G_TYPE_FROM_INTERFACE (progress_iface),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimpProgressInterface, cancel),
                      NULL, NULL,
                      gimp_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

      initialized = TRUE;
    }
}

GimpProgress *
gimp_progress_start (GimpProgress *progress,
                     const gchar  *message,
                     gboolean      cancelable)
{
  GimpProgressInterface *progress_iface;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), NULL);

  if (! message)
    message = _("Please wait");

  progress_iface = GIMP_PROGRESS_GET_INTERFACE (progress);

  if (progress_iface->start)
    return progress_iface->start (progress, message, cancelable);

  return NULL;
}

void
gimp_progress_end (GimpProgress *progress)
{
  GimpProgressInterface *progress_iface;

  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  progress_iface = GIMP_PROGRESS_GET_INTERFACE (progress);

  if (progress_iface->end)
    progress_iface->end (progress);
}

gboolean
gimp_progress_is_active (GimpProgress *progress)
{
  GimpProgressInterface *progress_iface;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), FALSE);

  progress_iface = GIMP_PROGRESS_GET_INTERFACE (progress);

  if (progress_iface->is_active)
    return progress_iface->is_active (progress);

  return FALSE;
}

void
gimp_progress_set_text (GimpProgress *progress,
                        const gchar  *message)
{
  GimpProgressInterface *progress_iface;

  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  if (! message || ! strlen (message))
    message = _("Please wait");

  progress_iface = GIMP_PROGRESS_GET_INTERFACE (progress);

  if (progress_iface->set_text)
    progress_iface->set_text (progress, message);
}

void
gimp_progress_set_value (GimpProgress *progress,
                         gdouble       percentage)
{
  GimpProgressInterface *progress_iface;

  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  percentage = CLAMP (percentage, 0.0, 1.0);

  progress_iface = GIMP_PROGRESS_GET_INTERFACE (progress);

  if (progress_iface->set_value)
    progress_iface->set_value (progress, percentage);
}

gdouble
gimp_progress_get_value (GimpProgress *progress)
{
  GimpProgressInterface *progress_iface;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), 0.0);

  progress_iface = GIMP_PROGRESS_GET_INTERFACE (progress);

  if (progress_iface->get_value)
    return progress_iface->get_value (progress);

  return 0.0;
}

void
gimp_progress_pulse (GimpProgress *progress)
{
  GimpProgressInterface *progress_iface;

  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  progress_iface = GIMP_PROGRESS_GET_INTERFACE (progress);

  if (progress_iface->pulse)
    progress_iface->pulse (progress);
}

guint32
gimp_progress_get_window (GimpProgress *progress)
{
  GimpProgressInterface *progress_iface;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), 0);

  progress_iface = GIMP_PROGRESS_GET_INTERFACE (progress);

  if (progress_iface->get_window)
    return progress_iface->get_window (progress);

  return 0;
}

gboolean
gimp_progress_message (GimpProgress        *progress,
                       Gimp                *gimp,
                       GimpMessageSeverity  severity,
                       const gchar         *domain,
                       const gchar         *message)
{
  GimpProgressInterface *progress_iface;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (domain != NULL, FALSE);
  g_return_val_if_fail (message != NULL, FALSE);

  progress_iface = GIMP_PROGRESS_GET_INTERFACE (progress);

  if (progress_iface->message)
    return progress_iface->message (progress, gimp, severity, domain, message);

  return FALSE;
}

void
gimp_progress_cancel (GimpProgress *progress)
{
  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  g_signal_emit (progress, progress_signals[CANCEL], 0);
}

void
gimp_progress_update_and_flush (gint     min,
                                gint     max,
                                gint     current,
                                gpointer data)
{
  gimp_progress_set_value (GIMP_PROGRESS (data),
                           (gdouble) (current - min) / (gdouble) (max - min));

  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, TRUE);
}
