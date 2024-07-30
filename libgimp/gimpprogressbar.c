/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprogressbar.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimpuitypes.h"

#include "gimp.h"

#include "gimpprogressbar.h"


/**
 * SECTION: gimpprogressbar
 * @title: GimpProgressBar
 * @short_description: A widget providing a progress bar.
 *
 * A widget providing a progress bar that automatically redirects any
 * progress calls to itself.
 **/


struct _GimpProgressBar
{
  GtkProgressBar  parent_instance;

  const gchar    *progress_callback;
  GBytes         *window_handle;
};


static void     gimp_progress_bar_dispose           (GObject     *object);

static void     gimp_progress_bar_start             (const gchar *message,
                                                     gboolean     cancelable,
                                                     gpointer     user_data);
static void     gimp_progress_bar_end               (gpointer     user_data);
static void     gimp_progress_bar_set_text          (const gchar *message,
                                                     gpointer     user_data);
static void     gimp_progress_bar_set_value         (gdouble      percentage,
                                                     gpointer     user_data);
static void     gimp_progress_bar_pulse             (gpointer     user_data);
static GBytes * gimp_progress_bar_get_window_handle (gpointer     user_data);


G_DEFINE_TYPE (GimpProgressBar, gimp_progress_bar, GTK_TYPE_PROGRESS_BAR)

#define parent_class gimp_progress_bar_parent_class


static void
gimp_progress_bar_class_init (GimpProgressBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gimp_progress_bar_dispose;
}

static void
gimp_progress_bar_init (GimpProgressBar *bar)
{
  GimpProgressVtable vtable = { 0, };

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), " ");
  gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (bar), PANGO_ELLIPSIZE_END);

  vtable.start             = gimp_progress_bar_start;
  vtable.end               = gimp_progress_bar_end;
  vtable.set_text          = gimp_progress_bar_set_text;
  vtable.set_value         = gimp_progress_bar_set_value;
  vtable.pulse             = gimp_progress_bar_pulse;
  vtable.get_window_handle = gimp_progress_bar_get_window_handle;

  bar->progress_callback = gimp_progress_install_vtable (&vtable, bar, NULL);

  gimp_widget_set_native_handle (GTK_WIDGET (bar), &bar->window_handle);
}

static void
gimp_progress_bar_dispose (GObject *object)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (object);

  if (bar->progress_callback)
    {
      gimp_progress_uninstall (bar->progress_callback);
      bar->progress_callback = NULL;
    }

  gimp_widget_free_native_handle (GTK_WIDGET (bar), &bar->window_handle);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_progress_bar_start (const gchar *message,
                         gboolean     cancelable,
                         gpointer     user_data)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (user_data);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), message ? message : " ");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);

  if (gtk_widget_is_drawable (GTK_WIDGET (bar)))
    while (gtk_events_pending ())
      gtk_main_iteration ();
}

static void
gimp_progress_bar_end (gpointer user_data)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (user_data);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), " ");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);

  if (gtk_widget_is_drawable (GTK_WIDGET (bar)))
    while (gtk_events_pending ())
      gtk_main_iteration ();
}

static void
gimp_progress_bar_set_text (const gchar *message,
                            gpointer     user_data)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (user_data);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), message ? message : " ");

  if (gtk_widget_is_drawable (GTK_WIDGET (bar)))
    while (gtk_events_pending ())
      gtk_main_iteration ();
}

static void
gimp_progress_bar_set_value (gdouble  percentage,
                             gpointer user_data)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (user_data);

  if (percentage >= 0.0)
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), percentage);
  else
    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar));

  if (gtk_widget_is_drawable (GTK_WIDGET (bar)))
    while (gtk_events_pending ())
      gtk_main_iteration ();
}

static void
gimp_progress_bar_pulse (gpointer user_data)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (user_data);

  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar));

  if (gtk_widget_is_drawable (GTK_WIDGET (bar)))
    while (gtk_events_pending ())
      gtk_main_iteration ();
}

static GBytes *
gimp_progress_bar_get_window_handle (gpointer user_data)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (user_data);

  return g_bytes_ref (bar->window_handle);
}

/**
 * gimp_progress_bar_new:
 *
 * Creates a new #GimpProgressBar widget.
 *
 * Returns: the new widget.
 *
 * Since: 2.2
 **/
GtkWidget *
gimp_progress_bar_new (void)
{
  return g_object_new (GIMP_TYPE_PROGRESS_BAR, NULL);
}
