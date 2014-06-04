/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

/*  This code is written so that it can also be compiled standalone.
 *  It shouldn't depend on libgimp.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#include "gimphelptypes.h"
#include "gimphelpprogress.h"
#include "gimphelpprogress-private.h"


struct _GimpHelpProgress
{
  GimpHelpProgressVTable  vtable;
  gpointer                user_data;

  GCancellable           *cancellable;
};


GimpHelpProgress *
gimp_help_progress_new (const GimpHelpProgressVTable *vtable,
                        gpointer                      user_data)
{
  GimpHelpProgress *progress;

  g_return_val_if_fail (vtable != NULL, NULL);

  progress = g_slice_new0 (GimpHelpProgress);

  progress->vtable.start     = vtable->start;
  progress->vtable.end       = vtable->end;
  progress->vtable.set_value = vtable->set_value;

  progress->user_data        = user_data;

  return progress;
}

void
gimp_help_progress_free (GimpHelpProgress *progress)
{
  g_return_if_fail (progress != NULL);

  if (progress->cancellable)
    {
      g_object_unref (progress->cancellable);
      progress->cancellable = NULL;
    }

  g_slice_free (GimpHelpProgress, progress);
}

void
gimp_help_progress_cancel (GimpHelpProgress *progress)
{
  g_return_if_fail (progress != NULL);

  if (progress->cancellable)
    g_cancellable_cancel (progress->cancellable);
}


/**
 * _gimp_help_progress_start:
 * @progress:    data model for the progress indicator
 * @cancellable: a thread-safe operation cancellation stack
 * @format:      format string for the output message
 * @...:         variable argument list
 *
 * Initialize the progress indicator.
 */
void
_gimp_help_progress_start (GimpHelpProgress *progress,
                           GCancellable     *cancellable,
                           const gchar      *format,
                           ...)
{
  gchar   *message;
  va_list  args;

  g_return_if_fail (progress != NULL);

  if (cancellable)
    g_object_ref (cancellable);

  if (progress->cancellable)
    g_object_unref (progress->cancellable);

  progress->cancellable = cancellable;

  va_start (args, format);
  message = g_strdup_vprintf (format, args);
  va_end (args);

  if (progress->vtable.start)
    progress->vtable.start (message, cancellable != NULL, progress->user_data);

  g_free (message);
}

/**
 * _gimp_help_progress_update:
 * @progress: the progress indicator
 * @percentage: the new percentage
 *
 * Update the progress indicator.
 */
void
_gimp_help_progress_update (GimpHelpProgress *progress,
                            gdouble           percentage)
{
  g_return_if_fail (progress != NULL);

  if (progress->vtable.set_value)
    progress->vtable.set_value (percentage, progress->user_data);
}

/**
 * _gimp_help_progress_pulse:
 * @progress: the progress indicator
 *
 * Change the progress indicator's percentage back to 0.
 */
void
_gimp_help_progress_pulse (GimpHelpProgress *progress)
{
  g_return_if_fail (progress != NULL);

  _gimp_help_progress_update (progress, -1.0);
}

/**
 * _gimp_help_progress_finish:
 * @progress: the progress indicator
 *
 * Finalize the progress indidicator.
 */
void
_gimp_help_progress_finish (GimpHelpProgress *progress)
{
  g_return_if_fail (progress != NULL);

  if (progress->vtable.end)
    progress->vtable.end (progress->user_data);

  if (progress->cancellable)
    {
      g_object_unref (progress->cancellable);
      progress->cancellable = NULL;
    }
}
