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

#include "gui-types.h"

#include "core/gimpgradient.h"
#include "core/gimpcontext.h"

#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimphelp-ids.h"

#include "gradients-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gradients_save_as_pov_query    (GimpContainerEditor *editor);
static void   gradients_save_as_pov_response (GtkWidget           *dialog,
                                              gint                 response_id,
                                              GimpGradient        *gradient);


/*  public functions  */

void
gradients_save_as_pov_ray_cmd_callback (GtkWidget *widget,
					gpointer   data)
{
  GimpContainerEditor *editor;

  editor = GIMP_CONTAINER_EDITOR (data);

  gradients_save_as_pov_query (editor);
}


/*  private functions  */

static void
gradients_save_as_pov_query (GimpContainerEditor *editor)
{
  GimpGradient     *gradient;
  GtkFileSelection *filesel;
  gchar            *title;

  gradient = gimp_context_get_gradient (editor->view->context);

  if (! gradient)
    return;

  title = g_strdup_printf (_("Save '%s' as POV-Ray"),
			   GIMP_OBJECT (gradient)->name);

  filesel = GTK_FILE_SELECTION (gtk_file_selection_new (title));

  g_free (title);

  gtk_window_set_screen (GTK_WINDOW (filesel),
                         gtk_widget_get_screen (GTK_WIDGET (editor)));

  gtk_window_set_role (GTK_WINDOW (filesel), "gimp-gradient-save-pov");
  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (filesel), 6);
  gtk_container_set_border_width (GTK_CONTAINER (filesel->button_area), 4);

  g_signal_connect (filesel, "response",
                    G_CALLBACK (gradients_save_as_pov_response),
                    gradient);

  g_object_ref (gradient);

  g_signal_connect_object (filesel, "destroy",
                           G_CALLBACK (g_object_unref),
                           gradient,
                           G_CONNECT_SWAPPED);

  gimp_help_connect (GTK_WIDGET (filesel), gimp_standard_help_func,
		     GIMP_HELP_GRADIENT_SAVE_AS_POV, NULL);

  gtk_widget_show (GTK_WIDGET (filesel));
}

static void
gradients_save_as_pov_response (GtkWidget    *dialog,
                                gint          response_id,
                                GimpGradient *gradient)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *filename;
      GError      *error = NULL;

      filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog));

      if (! gimp_gradient_save_as_pov (gradient, filename, &error))
        {
          g_message (error->message);
          g_clear_error (&error);
        }
    }

  gtk_widget_destroy (dialog);
}
