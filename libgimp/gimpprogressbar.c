/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprogressbar.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpuitypes.h"

#include "gimp.h"

#include "gimpprogressbar.h"


static void   gimp_progress_bar_class_init (GimpProgressBarClass *klass);
static void   gimp_progress_bar_init       (GimpProgressBar      *bar);

static void   gimp_progress_bar_destroy    (GtkObject            *object);

static void   gimp_progress_bar_start      (const gchar          *message,
                                            gboolean              cancelable,
                                            gpointer              user_data);
static void   gimp_progress_bar_end        (gpointer              user_data);
static void   gimp_progress_bar_set_text   (const gchar          *message,
                                            gpointer              user_data);
static void   gimp_progress_bar_set_value  (gdouble               percentage,
                                            gpointer              user_data);


static GtkProgressBarClass *parent_class = NULL;


GType
gimp_progress_bar_get_type (void)
{
  static GType bar_type = 0;

  if (! bar_type)
    {
      static const GTypeInfo bar_info =
      {
        sizeof (GimpProgressBarClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_progress_bar_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpProgressBar),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_progress_bar_init,
      };

      bar_type = g_type_register_static (GTK_TYPE_PROGRESS_BAR,
                                         "GimpProgressBar",
                                         &bar_info, 0);
    }

  return bar_type;
}

static void
gimp_progress_bar_class_init (GimpProgressBarClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = gimp_progress_bar_destroy;
}

static void
gimp_progress_bar_init (GimpProgressBar *bar)
{
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), " ");

  bar->progress_callback = gimp_progress_install (gimp_progress_bar_start,
                                                  gimp_progress_bar_end,
                                                  gimp_progress_bar_set_text,
                                                  gimp_progress_bar_set_value,
                                                  bar);
}

static void
gimp_progress_bar_destroy (GtkObject *object)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (object);

  if (bar->progress_callback)
    {
      gimp_progress_uninstall (bar->progress_callback);
      bar->progress_callback = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_progress_bar_start (const gchar *message,
                         gboolean     cancelable,
                         gpointer     user_data)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (user_data);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), message ? message : " ");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);

  if (GTK_WIDGET_DRAWABLE (bar))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static void
gimp_progress_bar_end (gpointer user_data)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (user_data);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), " ");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);

  if (GTK_WIDGET_DRAWABLE (bar))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static void
gimp_progress_bar_set_text (const gchar *message,
                            gpointer     user_data)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (user_data);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), message ? message : " ");

  if (GTK_WIDGET_DRAWABLE (bar))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static void
gimp_progress_bar_set_value (gdouble  percentage,
                             gpointer user_data)
{
  GimpProgressBar *bar = GIMP_PROGRESS_BAR (user_data);

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), percentage);

  if (GTK_WIDGET_DRAWABLE (bar))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

/**
 * gimp_progress_bar_new:
 *
 * Creates a new #GimpProgressBar widget.
 *
 * Return value: the new widget.
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_progress_bar_new (void)
{
  return g_object_new (GIMP_TYPE_PROGRESS_BAR, NULL);
}
