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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimpgradient-save.h"
#include "core/gimpcontext.h"

#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimphelp-ids.h"

#include "gradients-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gradients_save_as_pov_ray_response (GtkWidget    *dialog,
                                                  gint          response_id,
                                                  GimpGradient *gradient);


/*  public functions  */

void
gradients_save_as_pov_ray_cmd_callback (GtkAction *action,
                                        gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContext         *context;
  GimpGradient        *gradient;
  GtkFileChooser      *chooser;
  gchar               *title;

  context = gimp_container_view_get_context (editor->view);

  gradient = gimp_context_get_gradient (context);

  if (! gradient)
    return;

  title = g_strdup_printf (_("Save '%s' as POV-Ray"),
                           GIMP_OBJECT (gradient)->name);

  chooser = GTK_FILE_CHOOSER
    (gtk_file_chooser_dialog_new (title, NULL,
                                  GTK_FILE_CHOOSER_ACTION_SAVE,

                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                  GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                  NULL));

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (chooser),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_free (title);

  gtk_window_set_screen (GTK_WINDOW (chooser),
                         gtk_widget_get_screen (GTK_WIDGET (editor)));

  gtk_window_set_role (GTK_WINDOW (chooser), "gimp-gradient-save-pov");
  gtk_window_set_position (GTK_WINDOW (chooser), GTK_WIN_POS_MOUSE);

  g_signal_connect (chooser, "response",
                    G_CALLBACK (gradients_save_as_pov_ray_response),
                    gradient);
  g_signal_connect (chooser, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  g_object_ref (gradient);

  g_signal_connect_object (chooser, "destroy",
                           G_CALLBACK (g_object_unref),
                           gradient,
                           G_CONNECT_SWAPPED);

  gimp_help_connect (GTK_WIDGET (chooser), gimp_standard_help_func,
                     GIMP_HELP_GRADIENT_SAVE_AS_POV, NULL);

  gtk_widget_show (GTK_WIDGET (chooser));
}


/*  private functions  */

static void
gradients_save_as_pov_ray_response (GtkWidget    *dialog,
                                    gint          response_id,
                                    GimpGradient *gradient)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *filename;
      GError      *error = NULL;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (! gimp_gradient_save_pov (gradient, filename, &error))
        {
          g_message (error->message);
          g_clear_error (&error);
        }
    }

  gtk_widget_destroy (dialog);
}
