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

#include "widgets/widgets-types.h"

#include "core/gimpgradient.h"
#include "core/gimpcontext.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpwidgets-utils.h"

#include "gradients-commands.h"
#include "menus.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   gradients_menu_set_sensitivity    (GimpContainerEditor *editor);
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
gradients_show_context_menu (GimpContainerEditor *editor)
{
  GtkItemFactory *item_factory;
  gint            x, y;

  gradients_menu_set_sensitivity (editor);

  item_factory = menus_get_gradients_factory ();

  gimp_menu_position (GTK_MENU (item_factory->widget), &x, &y);

  gtk_item_factory_popup_with_data (item_factory,
				    editor,
				    NULL,
				    x, y,
				    3, 0);
}


/*  private functions  */

static void
gradients_menu_set_sensitivity (GimpContainerEditor *editor)
{
  GimpGradient *gradient;

  gradient = gimp_context_get_gradient (editor->view->context);

#define SET_SENSITIVE(menu,condition) \
        menus_set_sensitive ("<Gradients>/" menu, (condition) != 0)

  SET_SENSITIVE ("Duplicate Gradient",
		 gradient && GIMP_DATA_GET_CLASS (gradient)->duplicate);
  SET_SENSITIVE ("Edit Gradient...",
		 gradient && GIMP_DATA_FACTORY_VIEW (editor)->data_edit_func);
  SET_SENSITIVE ("Delete Gradient...",
		 gradient);
  SET_SENSITIVE ("Save as POV-Ray...",
		 gradient);

#undef SET_SENSITIVE
}

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
                            GTK_SIGNAL_FUNC (gtk_widget_destroy),
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
  gimp_help_connect_help_accel (GTK_WIDGET (filesel), gimp_standard_help_func,
				"dialogs/save_as_povray.html");

  gtk_widget_show (GTK_WIDGET (filesel));
}

static void
gradients_save_as_pov_ok_callback (GtkWidget    *widget,
				   GimpGradient *gradient)
{
  GtkFileSelection    *filesel;
  const gchar         *filename;
  FILE                *file;
  GimpGradientSegment *seg;

  filesel  = GTK_FILE_SELECTION (gtk_widget_get_toplevel (widget));
  filename = gtk_file_selection_get_filename (filesel);

  file = fopen (filename, "wb");

  if (!file)
    {
      g_message ("Could not open \"%s\"", filename);
    }
  else
    {
      fprintf (file, "/* color_map file created by the GIMP */\n");
      fprintf (file, "/* http://www.gimp.org/               */\n");

      fprintf (file, "color_map {\n");

      for (seg = gradient->segments; seg; seg = seg->next)
	{
	  /* Left */
	  fprintf (file, "\t[%f color rgbt <%f, %f, %f, %f>]\n",
		   seg->left,
		   seg->left_color.r,
		   seg->left_color.g,
		   seg->left_color.b,
		   1.0 - seg->left_color.a);

	  /* Middle */
	  fprintf (file, "\t[%f color rgbt <%f, %f, %f, %f>]\n",
		   seg->middle,
		   (seg->left_color.r + seg->right_color.r) / 2.0,
		   (seg->left_color.g + seg->right_color.g) / 2.0,
		   (seg->left_color.b + seg->right_color.b) / 2.0,
		   1.0 - (seg->left_color.a + seg->right_color.a) / 2.0);

	  /* Right */
	  fprintf (file, "\t[%f color rgbt <%f, %f, %f, %f>]\n",
		   seg->right,
		   seg->right_color.r,
		   seg->right_color.g,
		   seg->right_color.b,
		   1.0 - seg->right_color.a);
	}

      fprintf (file, "} /* color_map */\n");
      fclose (file);
    }

  gtk_widget_destroy (GTK_WIDGET (filesel));
}
