/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprogress.c
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gio/gio.h>
#include <gegl.h>

#include "core-types.h"

#include "gimp.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


enum
{
  CANCEL,
  LAST_SIGNAL
};


G_DEFINE_INTERFACE (GimpProgress, gimp_progress, G_TYPE_OBJECT)


static guint progress_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */


static void
gimp_progress_default_init (GimpProgressInterface *progress_iface)
{
  progress_signals[CANCEL] =
    g_signal_new ("cancel",
                  G_TYPE_FROM_INTERFACE (progress_iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpProgressInterface, cancel),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}


/*  public functions  */


GimpProgress *
gimp_progress_start (GimpProgress *progress,
                     gboolean      cancellable,
                     const gchar  *format,
                     ...)
{
  GimpProgressInterface *progress_iface;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (format != NULL, NULL);

  progress_iface = GIMP_PROGRESS_GET_IFACE (progress);

  if (progress_iface->start)
    {
      GimpProgress *ret;
      va_list       args;
      gchar        *text;

      va_start (args, format);
      text = g_strdup_vprintf (format, args);
      va_end (args);

      ret = progress_iface->start (progress, cancellable, text);

      g_free (text);

      return ret;
    }

  return NULL;
}

void
gimp_progress_end (GimpProgress *progress)
{
  GimpProgressInterface *progress_iface;

  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  progress_iface = GIMP_PROGRESS_GET_IFACE (progress);

  if (progress_iface->end)
    progress_iface->end (progress);
}

gboolean
gimp_progress_is_active (GimpProgress *progress)
{
  GimpProgressInterface *progress_iface;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), FALSE);

  progress_iface = GIMP_PROGRESS_GET_IFACE (progress);

  if (progress_iface->is_active)
    return progress_iface->is_active (progress);

  return FALSE;
}

void
gimp_progress_set_text (GimpProgress *progress,
                        const gchar  *format,
                        ...)
{
  va_list  args;
  gchar   *message;

  g_return_if_fail (GIMP_IS_PROGRESS (progress));
  g_return_if_fail (format != NULL);

  va_start (args, format);
  message = g_strdup_vprintf (format, args);
  va_end (args);

  gimp_progress_set_text_literal (progress, message);

  g_free (message);
}

void
gimp_progress_set_text_literal (GimpProgress *progress,
                                const gchar  *message)
{
  GimpProgressInterface *progress_iface;

  g_return_if_fail (GIMP_IS_PROGRESS (progress));
  g_return_if_fail (message != NULL);

  progress_iface = GIMP_PROGRESS_GET_IFACE (progress);

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

  progress_iface = GIMP_PROGRESS_GET_IFACE (progress);

  if (progress_iface->set_value)
    progress_iface->set_value (progress, percentage);
}

gdouble
gimp_progress_get_value (GimpProgress *progress)
{
  GimpProgressInterface *progress_iface;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), 0.0);

  progress_iface = GIMP_PROGRESS_GET_IFACE (progress);

  if (progress_iface->get_value)
    return progress_iface->get_value (progress);

  return 0.0;
}

void
gimp_progress_pulse (GimpProgress *progress)
{
  GimpProgressInterface *progress_iface;

  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  progress_iface = GIMP_PROGRESS_GET_IFACE (progress);

  if (progress_iface->pulse)
    progress_iface->pulse (progress);
}

GBytes *
gimp_progress_get_window_id (GimpProgress *progress)
{
  GimpProgressInterface *progress_iface;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), 0);

  progress_iface = GIMP_PROGRESS_GET_IFACE (progress);

  if (progress_iface->get_window_id)
    return progress_iface->get_window_id (progress);

  return NULL;
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

  progress_iface = GIMP_PROGRESS_GET_IFACE (progress);

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
