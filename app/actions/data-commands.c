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

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdataeditor.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpdialogfactory.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "data-commands.h"

#include "gimp-intl.h"


void
data_new_data_cmd_callback (GtkAction *action,
			    gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);

  if (view->factory->data_new_func)
    {
      GimpContext *context;
      GimpData    *data;

      context =
        gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

      data = gimp_data_factory_data_new (view->factory, _("Untitled"));

      if (data)
	{
	  gimp_context_set_by_type (context,
				    view->factory->container->children_type,
				    GIMP_OBJECT (data));

	  gtk_button_clicked (GTK_BUTTON (view->edit_button));
	}
    }
}

void
data_duplicate_data_cmd_callback (GtkAction *action,
				  gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
			      view->factory->container->children_type);

  if (data && gimp_container_have (view->factory->container,
				   GIMP_OBJECT (data)))
    {
      GimpData *new_data;

      new_data = gimp_data_factory_data_duplicate (view->factory, data);

      if (new_data)
	{
	  gimp_context_set_by_type (context,
				    view->factory->container->children_type,
				    GIMP_OBJECT (new_data));

	  gtk_button_clicked (GTK_BUTTON (view->edit_button));
	}
    }
}

typedef struct _GimpDataDeleteData GimpDataDeleteData;

struct _GimpDataDeleteData
{
  GimpDataFactory *factory;
  GimpData        *data;
};

static void
data_delete_callback (GtkWidget *widget,
                      gboolean   delete,
                      gpointer   data)
{
  GimpDataDeleteData *delete_data = data;

  if (delete)
    {
      GError *error = NULL;

      if (! gimp_data_factory_data_delete (delete_data->factory,
                                           delete_data->data,
                                           TRUE, &error))
        {
          g_message (error->message);
          g_clear_error (&error);
        }
    }
}

void
data_delete_data_cmd_callback (GtkAction *action,
			       gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
			      view->factory->container->children_type);

  if (data && data->deletable && gimp_container_have (view->factory->container,
                                                      GIMP_OBJECT (data)))
    {
      GimpDataDeleteData *delete_data;
      GtkWidget          *dialog;
      gchar              *str;

      delete_data = g_new0 (GimpDataDeleteData, 1);

      delete_data->factory = view->factory;
      delete_data->data    = data;

      str = g_strdup_printf (_("Are you sure you want to delete '%s' "
			       "from the list and from disk?"),
			     GIMP_OBJECT (data)->name);

      dialog = gimp_query_boolean_box (_("Delete Data Object"),
                                       GTK_WIDGET (view),
				       gimp_standard_help_func, NULL,
				       GIMP_STOCK_QUESTION,
				       str,
				       GTK_STOCK_DELETE, GTK_STOCK_CANCEL,
				       G_OBJECT (data),
				       "disconnect",
				       data_delete_callback,
				       delete_data);

      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_free, delete_data);

      g_free (str);

      gtk_widget_show (dialog);
    }
}

void
data_refresh_data_cmd_callback (GtkAction *action,
				gpointer   user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);

  gimp_data_factory_data_save (view->factory);
  gimp_data_factory_data_init (view->factory, FALSE);
}

void
data_edit_data_cmd_callback (GtkAction   *action,
                             const gchar *value,
			     gpointer     user_data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (user_data);
  GimpContext         *context;
  GimpData            *data;

  context = gimp_container_view_get_context (GIMP_CONTAINER_EDITOR (view)->view);

  data = (GimpData *)
    gimp_context_get_by_type (context,
			      view->factory->container->children_type);

  if (data && gimp_container_have (view->factory->container,
                                   GIMP_OBJECT (data)))
    {
      GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (view));
      GtkWidget *dockable;

      dockable = gimp_dialog_factory_dialog_raise (global_dock_factory, screen,
                                                   value, -1);

      gimp_data_editor_set_data (GIMP_DATA_EDITOR (GTK_BIN (dockable)->child),
                                 data);
    }
}
