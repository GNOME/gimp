/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprogressbox.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpprogress.h"

#include "gimpprogressbox.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void     gimp_progress_box_progress_iface_init (GimpProgressInterface *iface);

static void     gimp_progress_box_dispose            (GObject      *object);

static GimpProgress *
                gimp_progress_box_progress_start     (GimpProgress *progress,
                                                      gboolean      cancellable,
                                                      const gchar  *message);
static void     gimp_progress_box_progress_end       (GimpProgress *progress);
static gboolean gimp_progress_box_progress_is_active (GimpProgress *progress);
static void     gimp_progress_box_progress_set_text  (GimpProgress *progress,
                                                      const gchar  *message);
static void     gimp_progress_box_progress_set_value (GimpProgress *progress,
                                                      gdouble       percentage);
static gdouble  gimp_progress_box_progress_get_value (GimpProgress *progress);
static void     gimp_progress_box_progress_pulse     (GimpProgress *progress);


G_DEFINE_TYPE_WITH_CODE (GimpProgressBox, gimp_progress_box, GTK_TYPE_BOX,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_progress_box_progress_iface_init))

#define parent_class gimp_progress_box_parent_class


static void
gimp_progress_box_class_init (GimpProgressBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gimp_progress_box_dispose;
}

static void
gimp_progress_box_init (GimpProgressBox *box)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (box), 6);

  box->progress = gtk_progress_bar_new ();
  gtk_widget_set_size_request (box->progress, 250, 20);
  gtk_box_pack_start (GTK_BOX (box), box->progress, FALSE, FALSE, 0);
  gtk_widget_show (box->progress);

  box->label = gtk_label_new ("");
  gtk_label_set_ellipsize (GTK_LABEL (box->label), PANGO_ELLIPSIZE_MIDDLE);
  gtk_label_set_xalign (GTK_LABEL (box->label), 0.0);
  gimp_label_set_attributes (GTK_LABEL (box->label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (box), box->label, FALSE, FALSE, 0);
  gtk_widget_show (box->label);
}

static void
gimp_progress_box_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start     = gimp_progress_box_progress_start;
  iface->end       = gimp_progress_box_progress_end;
  iface->is_active = gimp_progress_box_progress_is_active;
  iface->set_text  = gimp_progress_box_progress_set_text;
  iface->set_value = gimp_progress_box_progress_set_value;
  iface->get_value = gimp_progress_box_progress_get_value;
  iface->pulse     = gimp_progress_box_progress_pulse;
}

static void
gimp_progress_box_dispose (GObject *object)
{
  GimpProgressBox *box = GIMP_PROGRESS_BOX (object);

  G_OBJECT_CLASS (parent_class)->dispose (object);

  box->progress = NULL;
}

static GimpProgress *
gimp_progress_box_progress_start (GimpProgress *progress,
                                  gboolean      cancellable,
                                  const gchar  *message)
{
  GimpProgressBox *box = GIMP_PROGRESS_BOX (progress);

  if (! box->progress)
    return NULL;

  if (! box->active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_label_set_text (GTK_LABEL (box->label), message);
      gtk_progress_bar_set_fraction (bar, 0.0);

      box->active      = TRUE;
      box->cancellable = cancellable;
      box->value       = 0.0;

      if (gtk_widget_is_drawable (box->progress))
        gdk_window_process_updates (gtk_widget_get_window (box->progress),
                                    TRUE);

      return progress;
    }

  return NULL;
}

static void
gimp_progress_box_progress_end (GimpProgress *progress)
{
  if (gimp_progress_box_progress_is_active (progress))
    {
      GimpProgressBox *box = GIMP_PROGRESS_BOX (progress);
      GtkProgressBar  *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_label_set_text (GTK_LABEL (box->label), "");
      gtk_progress_bar_set_fraction (bar, 0.0);

      box->active      = FALSE;
      box->cancellable = FALSE;
      box->value       = 0.0;
    }
}

static gboolean
gimp_progress_box_progress_is_active (GimpProgress *progress)
{
  GimpProgressBox *box = GIMP_PROGRESS_BOX (progress);

  return (box->progress && box->active);
}

static void
gimp_progress_box_progress_set_text (GimpProgress *progress,
                                     const gchar  *message)
{
  if (gimp_progress_box_progress_is_active (progress))
    {
      GimpProgressBox *box = GIMP_PROGRESS_BOX (progress);

      gtk_label_set_text (GTK_LABEL (box->label), message);

      if (gtk_widget_is_drawable (box->progress))
        gdk_window_process_updates (gtk_widget_get_window (box->progress),
                                    TRUE);
    }
}

static void
gimp_progress_box_progress_set_value (GimpProgress *progress,
                                      gdouble       percentage)
{
  if (gimp_progress_box_progress_is_active (progress))
    {
      GimpProgressBox *box = GIMP_PROGRESS_BOX (progress);
      GtkProgressBar  *bar = GTK_PROGRESS_BAR (box->progress);
      GtkAllocation    allocation;

      gtk_widget_get_allocation (GTK_WIDGET (bar), &allocation);

      box->value = percentage;

      /* only update the progress bar if this causes a visible change */
      if (fabs (allocation.width *
                (percentage - gtk_progress_bar_get_fraction (bar))) > 1.0)
        {
          gtk_progress_bar_set_fraction (bar, box->value);

          gimp_widget_flush_expose (box->progress);
        }
    }
}

static gdouble
gimp_progress_box_progress_get_value (GimpProgress *progress)
{
  if (gimp_progress_box_progress_is_active (progress))
    {
      return GIMP_PROGRESS_BOX (progress)->value;
    }

  return 0.0;
}

static void
gimp_progress_box_progress_pulse (GimpProgress *progress)
{
  if (gimp_progress_box_progress_is_active (progress))
    {
      GimpProgressBox *box = GIMP_PROGRESS_BOX (progress);
      GtkProgressBar  *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_pulse (bar);

      if (gtk_widget_is_drawable (box->progress))
        gdk_window_process_updates (gtk_widget_get_window (box->progress),
                                    TRUE);
    }
}

GtkWidget *
gimp_progress_box_new (void)
{
  return g_object_new (GIMP_TYPE_PROGRESS_BOX, NULL);
}
