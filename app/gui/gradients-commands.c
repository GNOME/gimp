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

#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimpgradient.h"
#include "core/gimpcontext.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "gradients-commands.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   gradients_save_as_pov_query       (GimpContainerEditor *editor);
static void   gradients_save_as_pov_ok_callback (GtkWidget           *widget,
						 GimpGradient        *gradient);


/*  public functions  */

void
gradients_save_as_pov_ray_cmd_callback (GtkWidget *widget,
					gpointer   data)
{
  GimpContainerEditor *editor;

  editor = (GimpContainerEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  gradients_save_as_pov_query (editor);
}

void
gradients_menu_update (GtkItemFactory *factory,
                       gpointer        data)
{
  GimpContainerEditor *editor;
  GimpGradient        *gradient;
  gboolean             internal = FALSE;

  editor = GIMP_CONTAINER_EDITOR (data);

  gradient = gimp_context_get_gradient (editor->view->context);

  if (gradient)
    internal = GIMP_DATA (gradient)->internal;

#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)

  SET_SENSITIVE ("/Duplicate Gradient",
		 gradient && GIMP_DATA_GET_CLASS (gradient)->duplicate);
  SET_SENSITIVE ("/Edit Gradient...",
		 gradient && GIMP_DATA_FACTORY_VIEW (editor)->data_edit_func);
  SET_SENSITIVE ("/Delete Gradient...",
		 gradient && ! internal);
  SET_SENSITIVE ("/Save as POV-Ray...",
		 gradient);

#undef SET_SENSITIVE
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

  title = g_strdup_printf (_("Save \"%s\" as POV-Ray"),
			   GIMP_OBJECT (gradient)->name);

  filesel = GTK_FILE_SELECTION (gtk_file_selection_new (title));

  g_free (title);

  gtk_window_set_wmclass (GTK_WINDOW (filesel), "save_gradient", "Gimp");
  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (filesel), 2);
  gtk_container_set_border_width (GTK_CONTAINER (filesel->button_area), 2);

  g_signal_connect (G_OBJECT (filesel->ok_button), "clicked",
                    G_CALLBACK (gradients_save_as_pov_ok_callback),
                    gradient);

  g_signal_connect_swapped (G_OBJECT (filesel->cancel_button), "clicked",
                            G_CALLBACK (gtk_widget_destroy),
                            filesel);

  g_signal_connect_swapped (G_OBJECT (filesel), "delete_event",
                            G_CALLBACK (gtk_widget_destroy),
                            filesel);

  g_object_ref (G_OBJECT (gradient));

  g_signal_connect_object (G_OBJECT (filesel), "destroy",
                           G_CALLBACK (g_object_unref),
                           G_OBJECT (gradient), 
                           G_CONNECT_SWAPPED);

  /*  Connect the "F1" help key  */
  gimp_help_connect (GTK_WIDGET (filesel), gimp_standard_help_func,
		     "dialogs/save_as_povray.html");

  gtk_widget_show (GTK_WIDGET (filesel));
}

static void
gradients_save_as_pov_ok_callback (GtkWidget    *widget,
				   GimpGradient *gradient)
{
  GtkFileSelection *filesel;
  const gchar      *filename;
  GError           *error = NULL;

  filesel  = GTK_FILE_SELECTION (gtk_widget_get_toplevel (widget));
  filename = gtk_file_selection_get_filename (filesel);

  if (! gimp_gradient_save_as_pov (gradient, filename, &error))
    {
      g_message (error->message);
      g_clear_error (&error);
    }

  gtk_widget_destroy (GTK_WIDGET (filesel));
}
