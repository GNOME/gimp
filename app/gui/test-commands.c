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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"

#include "config/gimpconfig.h"

#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimpcontainergridview.h"
#include "widgets/gimpcontainermenuimpl.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpimagedock.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdrawablelistview.h"
#include "widgets/gimppreview.h"

#include "tools/tool_manager.h"

#include "dialogs.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


/*****  Container View Test Dialogs  *****/

static void
container_view_scale_callback (GtkAdjustment     *adj,
			       GimpContainerView *view)
{
  gimp_container_view_set_preview_size (view, ROUND (adj->value));
}

static void
brushes_callback (GtkWidget         *widget,
		  GimpContainerView *view)
{
  gimp_container_view_set_container (view, view->context->gimp->brush_factory->container);
}

static void
patterns_callback (GtkWidget         *widget,
		   GimpContainerView *view)
{
  gimp_container_view_set_container (view, view->context->gimp->pattern_factory->container);
}

static void
gradients_callback (GtkWidget         *widget,
		    GimpContainerView *view)
{
  gimp_container_view_set_container (view, view->context->gimp->gradient_factory->container);
}

static void
palettes_callback (GtkWidget         *widget,
		   GimpContainerView *view)
{
  gimp_container_view_set_container (view, view->context->gimp->palette_factory->container);
}

static void
images_callback (GtkWidget         *widget,
		 GimpContainerView *view)
{
  gimp_container_view_set_container (view, view->context->gimp->images);
}

/*
static void
null_callback (GtkWidget         *widget,
	       GimpContainerView *view)
{
  gimp_container_view_set_container (view, NULL);
}
*/

static void
container_multi_view_new (gboolean       list,
			  gchar         *title,
			  GimpContainer *container,
			  GimpContext   *context,
			  gint           preview_size)
{
  GtkWidget *dialog;
  GtkWidget *view;
  GtkObject *adjustment;
  GtkWidget *scale;
  GtkWidget *preview;

  if (list)
    {
      view = gimp_container_list_view_new (container,
					   context,
					   preview_size,
                                           FALSE,
					   5, 5);
    }
  else
    {
      view = gimp_container_grid_view_new (container,
					   context,
					   preview_size,
                                           FALSE,
					   5, 5);
    }

  dialog = gimp_dialog_new (title, "test",
			    gimp_standard_help_func,
			    NULL,
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, TRUE,

			    "Images", images_callback,
			    view, NULL, NULL, FALSE, FALSE,

				/*
				  "NULL", null_callback,
				  view, NULL, NULL, FALSE, FALSE,
				*/

			    "_delete_event_", gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_brush (context)),
			   32, 32, 1,
			   FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), preview,
		      FALSE, FALSE, 0);
  gtk_widget_show (preview);

  g_signal_connect (G_OBJECT (preview), "clicked",
		    G_CALLBACK (brushes_callback),
		    view);

  g_signal_connect_object (G_OBJECT (context), "brush_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_pattern (context)),
			   32, 32, 1,
			   FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), preview,
		      FALSE, FALSE, 0);
  gtk_widget_show (preview);

  g_signal_connect (G_OBJECT (preview), "clicked",
		    G_CALLBACK (patterns_callback),
		    view);

  g_signal_connect_object (G_OBJECT (context), "pattern_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_gradient (context)),
			   32, 32, 1,
			   FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), preview,
		      FALSE, FALSE, 0);
  gtk_widget_show (preview);

  g_signal_connect (G_OBJECT (preview), "clicked",
		    G_CALLBACK (gradients_callback),
		    view);

  g_signal_connect_object (G_OBJECT (context), "gradient_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);

  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_palette (context)),
			   32, 32, 1,
			   FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), preview,
		      FALSE, FALSE, 0);
  gtk_widget_show (preview);

  g_signal_connect (G_OBJECT (preview), "clicked",
		    G_CALLBACK (palettes_callback),
		    view);

  g_signal_connect_object (G_OBJECT (context), "palette_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);

#if 0
  preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_image (context)),
			   32, 32, 1,
			   FALSE, TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), preview,
		      FALSE, FALSE, 0);
  gtk_widget_show (preview);

  g_signal_connect (G_OBJECT (preview), "clicked",
		    G_CALLBACK (images_callback),
		    view);

  g_signal_connect_object (G_OBJECT (context), "pattern_changed",
			   G_CALLBACK (gimp_preview_set_viewable),
			   G_OBJECT (preview),
			   G_CONNECT_SWAPPED);
#endif

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), view);
  gtk_widget_show (view);

  adjustment = gtk_adjustment_new (preview_size, 16, 64, 4, 4, 0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scale,
		      FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (container_view_scale_callback),
		    view);

  gtk_widget_show (dialog);
}

void
test_multi_container_list_view_cmd_callback (GtkWidget *widget,
					     gpointer   data)
{
  Gimp *gimp;

  gimp = GIMP (data);

  container_multi_view_new (TRUE, "Multi List",
			    gimp->brush_factory->container,
			    gimp_get_user_context (gimp),
			    24);
}

void
test_multi_container_grid_view_cmd_callback (GtkWidget *widget,
					     gpointer   data)
{
  Gimp *gimp;

  gimp = GIMP (data);

  container_multi_view_new (FALSE, "Multi Grid",
			    gimp->brush_factory->container,
			    gimp_get_user_context (gimp),
			    32);
}

void
test_serialize_context_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  Gimp   *gimp;
  gchar  *filename;
  GError *error = NULL;

  gimp = GIMP (data);

  filename = gimp_personal_rc_file ("test-context");

  if (! gimp_config_serialize (G_OBJECT (gimp_get_user_context (gimp)),
                               filename,
                               "# test-context\n",
                               "# end of test-context\n",
                               NULL,
                               &error))
    {
      g_message ("Serializing Context failed:\n%s", error->message);
      g_clear_error (&error);
    }

  g_free (filename);
}

void
test_deserialize_context_cmd_callback (GtkWidget *widget,
                                       gpointer   data)
{
  Gimp   *gimp;
  gchar  *filename;
  GError *error = NULL;

  gimp = GIMP (data);

  filename = gimp_personal_rc_file ("test-context");

  if (! gimp_config_deserialize (G_OBJECT (gimp_get_user_context (gimp)),
                                 filename,
                                 NULL,
                                 &error))
    {
      g_message ("Deserializing Context failed:\n%s", error->message);
      g_clear_error (&error);
    }

  g_free (filename);
}

void
test_serialize_devicerc_cmd_callback (GtkWidget *widget,
                                      gpointer   data)
{
  Gimp *gimp;

  gimp = GIMP (data);

  gimp_devices_save_test (gimp);
}

void
test_deserialize_devicerc_cmd_callback (GtkWidget *widget,
                                        gpointer   data)
{
  Gimp *gimp;

  gimp = GIMP (data);

  gimp_devices_restore_test (gimp);
}
