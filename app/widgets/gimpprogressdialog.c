/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaprogressdialog.c
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmaprogress.h"

#include "ligmaprogressbox.h"
#include "ligmaprogressdialog.h"

#include "ligma-intl.h"


#define PROGRESS_DIALOG_WIDTH  400


static void    ligma_progress_dialog_progress_iface_init (LigmaProgressInterface *iface);

static void     ligma_progress_dialog_response           (GtkDialog    *dialog,
                                                         gint          response_id);

static LigmaProgress *
                ligma_progress_dialog_progress_start     (LigmaProgress *progress,
                                                         gboolean      cancellable,
                                                         const gchar  *message);
static void     ligma_progress_dialog_progress_end       (LigmaProgress *progress);
static gboolean ligma_progress_dialog_progress_is_active (LigmaProgress *progress);
static void     ligma_progress_dialog_progress_set_text  (LigmaProgress *progress,
                                                         const gchar  *message);
static void     ligma_progress_dialog_progress_set_value (LigmaProgress *progress,
                                                         gdouble       percentage);
static gdouble  ligma_progress_dialog_progress_get_value (LigmaProgress *progress);
static void     ligma_progress_dialog_progress_pulse     (LigmaProgress *progress);


G_DEFINE_TYPE_WITH_CODE (LigmaProgressDialog, ligma_progress_dialog,
                         LIGMA_TYPE_DIALOG,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PROGRESS,
                                                ligma_progress_dialog_progress_iface_init))

#define parent_class ligma_progress_dialog_parent_class


static void
ligma_progress_dialog_class_init (LigmaProgressDialogClass *klass)
{
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  dialog_class->response = ligma_progress_dialog_response;
}

static void
ligma_progress_dialog_init (LigmaProgressDialog *dialog)
{
  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  dialog->box = ligma_progress_box_new ();
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
ligma_progress_dialog_progress_iface_init (LigmaProgressInterface *iface)
{
  iface->start     = ligma_progress_dialog_progress_start;
  iface->end       = ligma_progress_dialog_progress_end;
  iface->is_active = ligma_progress_dialog_progress_is_active;
  iface->set_text  = ligma_progress_dialog_progress_set_text;
  iface->set_value = ligma_progress_dialog_progress_set_value;
  iface->get_value = ligma_progress_dialog_progress_get_value;
  iface->pulse     = ligma_progress_dialog_progress_pulse;
}

static void
ligma_progress_dialog_response (GtkDialog *dialog,
                               gint       response_id)
{
  LigmaProgressDialog *progress_dialog = LIGMA_PROGRESS_DIALOG (dialog);

  if (LIGMA_PROGRESS_BOX (progress_dialog->box)->cancellable)
    ligma_progress_cancel (LIGMA_PROGRESS (dialog));
}

static LigmaProgress *
ligma_progress_dialog_progress_start (LigmaProgress *progress,
                                     gboolean      cancellable,
                                     const gchar  *message)
{
  LigmaProgressDialog *dialog = LIGMA_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return NULL;

  if (ligma_progress_start (LIGMA_PROGRESS (dialog->box), cancellable,
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
ligma_progress_dialog_progress_end (LigmaProgress *progress)
{
  LigmaProgressDialog *dialog = LIGMA_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  if (LIGMA_PROGRESS_BOX (dialog->box)->active)
    {
      ligma_progress_end (LIGMA_PROGRESS (dialog->box));

      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL, FALSE);

      gtk_widget_hide (GTK_WIDGET (dialog));
    }
}

static gboolean
ligma_progress_dialog_progress_is_active (LigmaProgress *progress)
{
  LigmaProgressDialog *dialog = LIGMA_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return FALSE;

  return ligma_progress_is_active (LIGMA_PROGRESS (dialog->box));
}

static void
ligma_progress_dialog_progress_set_text (LigmaProgress *progress,
                                        const gchar  *message)
{
  LigmaProgressDialog *dialog = LIGMA_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  ligma_progress_set_text_literal (LIGMA_PROGRESS (dialog->box), message);
}

static void
ligma_progress_dialog_progress_set_value (LigmaProgress *progress,
                                         gdouble       percentage)
{
  LigmaProgressDialog *dialog = LIGMA_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  ligma_progress_set_value (LIGMA_PROGRESS (dialog->box), percentage);
}

static gdouble
ligma_progress_dialog_progress_get_value (LigmaProgress *progress)
{
  LigmaProgressDialog *dialog = LIGMA_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return 0.0;

  return ligma_progress_get_value (LIGMA_PROGRESS (dialog->box));
}

static void
ligma_progress_dialog_progress_pulse (LigmaProgress *progress)
{
  LigmaProgressDialog *dialog = LIGMA_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  ligma_progress_pulse (LIGMA_PROGRESS (dialog->box));
}

GtkWidget *
ligma_progress_dialog_new (void)
{
  gboolean use_header_bar;

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  return g_object_new (LIGMA_TYPE_PROGRESS_DIALOG,
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
