/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprogressdialog.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpprogress.h"

#include "gimpprogressbox.h"
#include "gimpprogressdialog.h"

#include "gimp-intl.h"


#define PROGRESS_DIALOG_WIDTH  400


static void    gimp_progress_dialog_progress_iface_init (GimpProgressInterface *iface);

static void     gimp_progress_dialog_response           (GtkDialog    *dialog,
                                                         gint          response_id);

static GimpProgress *
                gimp_progress_dialog_progress_start     (GimpProgress *progress,
                                                         gboolean      cancellable,
                                                         const gchar  *message);
static void     gimp_progress_dialog_progress_end       (GimpProgress *progress);
static gboolean gimp_progress_dialog_progress_is_active (GimpProgress *progress);
static void     gimp_progress_dialog_progress_set_text  (GimpProgress *progress,
                                                         const gchar  *message);
static void     gimp_progress_dialog_progress_set_value (GimpProgress *progress,
                                                         gdouble       percentage);
static gdouble  gimp_progress_dialog_progress_get_value (GimpProgress *progress);
static void     gimp_progress_dialog_progress_pulse     (GimpProgress *progress);


G_DEFINE_TYPE_WITH_CODE (GimpProgressDialog, gimp_progress_dialog,
                         GIMP_TYPE_DIALOG,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_progress_dialog_progress_iface_init))

#define parent_class gimp_progress_dialog_parent_class


static void
gimp_progress_dialog_class_init (GimpProgressDialogClass *klass)
{
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  dialog_class->response = gimp_progress_dialog_response;
}

static void
gimp_progress_dialog_init (GimpProgressDialog *dialog)
{
  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  dialog->box = gimp_progress_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (dialog->box), 12);
  gtk_box_pack_start (GTK_BOX (content_area), dialog->box, TRUE, TRUE, 0);
  gtk_widget_show (dialog->box);

  g_signal_connect (dialog->box, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &dialog->box);

  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         _("_Cancel"), GTK_RESPONSE_CANCEL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

  gtk_widget_set_size_request (GTK_WIDGET (dialog), PROGRESS_DIALOG_WIDTH, -1);
}

static void
gimp_progress_dialog_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start     = gimp_progress_dialog_progress_start;
  iface->end       = gimp_progress_dialog_progress_end;
  iface->is_active = gimp_progress_dialog_progress_is_active;
  iface->set_text  = gimp_progress_dialog_progress_set_text;
  iface->set_value = gimp_progress_dialog_progress_set_value;
  iface->get_value = gimp_progress_dialog_progress_get_value;
  iface->pulse     = gimp_progress_dialog_progress_pulse;
}

static void
gimp_progress_dialog_response (GtkDialog *dialog,
                               gint       response_id)
{
  GimpProgressDialog *progress_dialog = GIMP_PROGRESS_DIALOG (dialog);

  if (GIMP_PROGRESS_BOX (progress_dialog->box)->cancellable)
    gimp_progress_cancel (GIMP_PROGRESS (dialog));
}

static GimpProgress *
gimp_progress_dialog_progress_start (GimpProgress *progress,
                                     gboolean      cancellable,
                                     const gchar  *message)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return NULL;

  if (gimp_progress_start (GIMP_PROGRESS (dialog->box), cancellable,
                           "%s", message))
    {
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL, cancellable);

      gtk_window_present (GTK_WINDOW (dialog));

      return progress;
    }

  return NULL;
}

static void
gimp_progress_dialog_progress_end (GimpProgress *progress)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  if (GIMP_PROGRESS_BOX (dialog->box)->active)
    {
      gimp_progress_end (GIMP_PROGRESS (dialog->box));

      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL, FALSE);

      gtk_widget_hide (GTK_WIDGET (dialog));
    }
}

static gboolean
gimp_progress_dialog_progress_is_active (GimpProgress *progress)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return FALSE;

  return gimp_progress_is_active (GIMP_PROGRESS (dialog->box));
}

static void
gimp_progress_dialog_progress_set_text (GimpProgress *progress,
                                        const gchar  *message)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  gimp_progress_set_text_literal (GIMP_PROGRESS (dialog->box), message);
}

static void
gimp_progress_dialog_progress_set_value (GimpProgress *progress,
                                         gdouble       percentage)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  gimp_progress_set_value (GIMP_PROGRESS (dialog->box), percentage);
}

static gdouble
gimp_progress_dialog_progress_get_value (GimpProgress *progress)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return 0.0;

  return gimp_progress_get_value (GIMP_PROGRESS (dialog->box));
}

static void
gimp_progress_dialog_progress_pulse (GimpProgress *progress)
{
  GimpProgressDialog *dialog = GIMP_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  gimp_progress_pulse (GIMP_PROGRESS (dialog->box));
}

GtkWidget *
gimp_progress_dialog_new (void)
{
  gboolean use_header_bar;

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  return g_object_new (GIMP_TYPE_PROGRESS_DIALOG,
                       "title",             _("Progress"),
                       "role",              "progress",
                       "skip-taskbar-hint", TRUE,
                       "skip-pager-hint",   TRUE,
                       "resizable",         FALSE,
                       "focus-on-map",      FALSE,
                       "window-position",   GTK_WIN_POS_CENTER,
                       "use-header-bar",    use_header_bar,
                       NULL);
}
