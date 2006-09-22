/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpprogress.h"

#include "plug-in/gimpplugin.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimperrorconsole.h"
#include "widgets/gimperrordialog.h"
#include "widgets/gimpprogressdialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "dialogs/dialogs.h"

#include "gimp-intl.h"


static gboolean  gui_message_error_console (const gchar  *domain,
                                            const gchar  *message);
static gboolean  gui_message_error_dialog  (GimpProgress *progress,
                                            const gchar  *domain,
                                            const gchar  *message);
static void      gui_message_console       (const gchar  *domain,
                                            const gchar  *message);


void
gui_message (Gimp         *gimp,
             GimpProgress *progress,
             const gchar  *domain,
             const gchar  *message)
{
  switch (gimp->message_handler)
    {
    case GIMP_ERROR_CONSOLE:
      if (gui_message_error_console (domain, message))
        return;

      gimp->message_handler = GIMP_MESSAGE_BOX;
      /*  fallthru  */

    case GIMP_MESSAGE_BOX:
      if (gui_message_error_dialog (progress, domain, message))
        return;

      gimp->message_handler = GIMP_CONSOLE;
      /*  fallthru  */

    case GIMP_CONSOLE:
      gui_message_console (domain, message);
      break;
    }
}

static gboolean
gui_message_error_console (const gchar *domain,
                           const gchar *message)
{
  GtkWidget *dockable;

  dockable = gimp_dialog_factory_dialog_raise (global_dock_factory,
                                               gdk_screen_get_default (),
                                               "gimp-error-console", -1);

  if (dockable)
    {
      gimp_error_console_add (GIMP_ERROR_CONSOLE (GTK_BIN (dockable)->child),
                              GIMP_STOCK_WARNING, domain, message);

      return TRUE;
    }

  return FALSE;
}

static void
progress_error_dialog_unset (GimpProgress *progress)
{
  g_object_set_data (G_OBJECT (progress), "gimp-error-dialog", NULL);
}

static GtkWidget *
progress_error_dialog (GimpProgress *progress)
{
  GtkWidget *dialog;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), NULL);

  dialog = g_object_get_data (G_OBJECT (progress), "gimp-error-dialog");

  if (! dialog)
    {
      dialog = gimp_error_dialog_new (_("GIMP Message"));

      g_object_set_data (G_OBJECT (progress), "gimp-error-dialog", dialog);

      g_signal_connect_object (dialog, "destroy",
                               G_CALLBACK (progress_error_dialog_unset),
                               progress, G_CONNECT_SWAPPED);

      if (GTK_IS_WIDGET (progress))
        {
          GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (progress));

          if (GTK_IS_WINDOW (toplevel))
            gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                          GTK_WINDOW (toplevel));
        }
      else
        {
          guint32 window = gimp_progress_get_window (progress);

          if (window)
            gimp_window_set_transient_for (GTK_WINDOW (dialog), window);
        }
    }

  return dialog;
}

static GtkWidget *
global_error_dialog (void)
{
  return gimp_dialog_factory_dialog_new (global_dialog_factory,
                                         gdk_screen_get_default (),
                                         "gimp-error-dialog", -1,
                                         FALSE);
}

static gboolean
gui_message_error_dialog (GimpProgress *progress,
                          const gchar  *domain,
                          const gchar  *message)
{
  GtkWidget *dialog;

  if (progress && ! GIMP_IS_PROGRESS_DIALOG (progress))
    dialog = progress_error_dialog (progress);
  else
    dialog = global_error_dialog ();

  if (dialog)
    {
      gimp_error_dialog_add (GIMP_ERROR_DIALOG (dialog),
                             GIMP_STOCK_WARNING, domain, message);
      gtk_window_present (GTK_WINDOW (dialog));

      return TRUE;
    }

  return FALSE;
}

static void
gui_message_console (const gchar *domain,
                     const gchar *message)
{
  g_printerr ("%s: %s\n\n", domain, message);
}
