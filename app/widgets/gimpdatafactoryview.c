/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactoryview.c
 * Copyright (C) 2001 Michael Natterer
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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"
#include "core/gimpmarshal.h"

#include "gimpcontainerview.h"
#include "gimpdatafactoryview.h"
#include "gimpcontainergridview.h"
#include "gimpcontainerlistview.h"
#include "gimpdnd.h"
#include "gimpwidgets-utils.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/delete.xpm"
#include "pixmaps/duplicate.xpm"
#include "pixmaps/new.xpm"
#include "pixmaps/edit.xpm"
#include "pixmaps/refresh.xpm"


static void   gimp_data_factory_view_class_init (GimpDataFactoryViewClass *klass);
static void   gimp_data_factory_view_init       (GimpDataFactoryView      *view);
static void   gimp_data_factory_view_destroy    (GtkObject                *object);

static void   gimp_data_factory_view_new_clicked       (GtkWidget           *widget,
							GimpDataFactoryView *view);
static void   gimp_data_factory_view_duplicate_clicked (GtkWidget           *widget,
							GimpDataFactoryView *view);
static void   gimp_data_factory_view_edit_clicked      (GtkWidget           *widget,
							GimpDataFactoryView *view);
static void   gimp_data_factory_view_delete_clicked    (GtkWidget           *widget,
							GimpDataFactoryView *view);
static void   gimp_data_factory_view_refresh_clicked   (GtkWidget           *widget,
							GimpDataFactoryView *view);

static void   gimp_data_factory_view_select_item       (GimpContainerEditor *editor,
							GimpViewable        *viewable);
static void   gimp_data_factory_view_activate_item     (GimpContainerEditor *editor,
							GimpViewable        *viewable);


static GimpContainerEditorClass *parent_class = NULL;


GtkType
gimp_data_factory_view_get_type (void)
{
  static guint view_type = 0;

  if (! view_type)
    {
      GtkTypeInfo view_info =
      {
	"GimpDataFactoryView",
	sizeof (GimpDataFactoryView),
	sizeof (GimpDataFactoryViewClass),
	(GtkClassInitFunc) gimp_data_factory_view_class_init,
	(GtkObjectInitFunc) gimp_data_factory_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      view_type = gtk_type_unique (GIMP_TYPE_CONTAINER_EDITOR, &view_info);
    }

  return view_type;
}

static void
gimp_data_factory_view_class_init (GimpDataFactoryViewClass *klass)
{
  GtkObjectClass           *object_class;
  GimpContainerEditorClass *editor_class;

  object_class = (GtkObjectClass *) klass;
  editor_class = (GimpContainerEditorClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_CONTAINER_EDITOR);

  object_class->destroy       = gimp_data_factory_view_destroy;

  editor_class->select_item   = gimp_data_factory_view_select_item;
  editor_class->activate_item = gimp_data_factory_view_activate_item;
}

static void
gimp_data_factory_view_init (GimpDataFactoryView *view)
{
  GimpContainerEditor *editor;

  editor = GIMP_CONTAINER_EDITOR (view);

  view->new_button =
    gimp_container_editor_add_button (editor,
				      new_xpm,
				      _("New"), NULL,
				      G_CALLBACK (gimp_data_factory_view_new_clicked));

  view->duplicate_button =
    gimp_container_editor_add_button (editor,
				      duplicate_xpm,
				      _("Duplicate"), NULL,
				      G_CALLBACK (gimp_data_factory_view_duplicate_clicked));

  view->edit_button =
    gimp_container_editor_add_button (editor,
				      edit_xpm,
				      _("Edit"), NULL,
				      G_CALLBACK (gimp_data_factory_view_edit_clicked));

  view->delete_button =
    gimp_container_editor_add_button (editor,
				      delete_xpm,
				      _("Delete"), NULL,
				      G_CALLBACK (gimp_data_factory_view_delete_clicked));

  view->refresh_button =
    gimp_container_editor_add_button (editor,
				      refresh_xpm,
				      _("Refresh"), NULL,
				      G_CALLBACK (gimp_data_factory_view_refresh_clicked));

  gtk_widget_set_sensitive (view->duplicate_button, FALSE);
  gtk_widget_set_sensitive (view->edit_button, FALSE);
  gtk_widget_set_sensitive (view->delete_button, FALSE);
}

static void
gimp_data_factory_view_destroy (GtkObject *object)
{
  GimpDataFactoryView *view;

  view = GIMP_DATA_FACTORY_VIEW (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_data_factory_view_new (GimpViewType              view_type,
			    GimpDataFactory          *factory,
			    GimpDataEditFunc          edit_func,
			    GimpContext              *context,
			    gint                      preview_size,
			    gint                      min_items_x,
			    gint                      min_items_y,
			    GimpContainerContextFunc  context_func)
{
  GimpDataFactoryView *factory_view;

  factory_view = gtk_type_new (GIMP_TYPE_DATA_FACTORY_VIEW);

  if (! gimp_data_factory_view_construct (factory_view,
					  view_type,
					  factory,
					  edit_func,
					  context,
					  preview_size,
					  min_items_x,
					  min_items_y,
					  context_func))
    {
      g_object_unref (G_OBJECT (factory_view));
      return NULL;
    }

  return GTK_WIDGET (factory_view);
}

gboolean
gimp_data_factory_view_construct (GimpDataFactoryView      *factory_view,
				  GimpViewType              view_type,
				  GimpDataFactory          *factory,
				  GimpDataEditFunc          edit_func,
				  GimpContext              *context,
				  gint                      preview_size,
				  gint                      min_items_x,
				  gint                      min_items_y,
				  GimpContainerContextFunc  context_func)
{
  GimpContainerEditor *editor;

  g_return_val_if_fail (factory_view != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY_VIEW (factory_view), FALSE);
  g_return_val_if_fail (factory != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (preview_size > 0 && preview_size <= 64, FALSE);
  g_return_val_if_fail (min_items_x > 0 && min_items_x <= 64, FALSE);
  g_return_val_if_fail (min_items_y > 0 && min_items_y <= 64, FALSE);

  factory_view->factory        = factory;
  factory_view->data_edit_func = edit_func;

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (factory_view),
					 view_type,
					 factory->container,
					 context,
					 preview_size,
					 min_items_x,
					 min_items_y,
					 context_func))
    {
      return FALSE;
    }

  editor = GIMP_CONTAINER_EDITOR (factory_view);

  gimp_container_editor_enable_dnd (editor,
				    GTK_BUTTON (factory_view->duplicate_button));
  gimp_container_editor_enable_dnd (editor,
				    GTK_BUTTON (factory_view->edit_button));
  gimp_container_editor_enable_dnd (editor,
				    GTK_BUTTON (factory_view->delete_button));

  return TRUE;
}

static void
gimp_data_factory_view_new_clicked (GtkWidget           *widget,
				    GimpDataFactoryView *view)
{
  if (view->factory->data_new_func)
    {
      GimpData *data;

      data = gimp_data_factory_data_new (view->factory, _("Untitled"));

      if (data)
	{
	  gimp_context_set_by_type (GIMP_CONTAINER_EDITOR (view)->view->context,
				    view->factory->container->children_type,
				    GIMP_OBJECT (data));

	  gimp_data_factory_view_edit_clicked (NULL, view);
	}
    }
}

static void
gimp_data_factory_view_duplicate_clicked (GtkWidget           *widget,
					  GimpDataFactoryView *view)
{
  GimpData *data;

  data = (GimpData *)
    gimp_context_get_by_type (GIMP_CONTAINER_EDITOR (view)->view->context,
			      view->factory->container->children_type);

  if (data && gimp_container_have (view->factory->container,
				   GIMP_OBJECT (data)))
    {
      GimpData *new_data;

      new_data = gimp_data_duplicate (data);

      if (new_data)
	{
	  const gchar *name;
	  gchar       *ext;
	  gint         copy_len;
	  gint         number;
	  gchar       *new_name;

	  name = gimp_object_get_name (GIMP_OBJECT (data));

	  ext      = strrchr (name, '#');
	  copy_len = strlen (_("copy"));

	  if ((strlen (name) >= copy_len                                 &&
	       strcmp (&name[strlen (name) - copy_len], _("copy")) == 0) ||
	      (ext && (number = atoi (ext + 1)) > 0                      &&
	       ((gint) (log10 (number) + 1)) == strlen (ext + 1)))
	    {
	      /* don't have redundant "copy"s */
	      new_name = g_strdup (name);
	    }
	  else
	    {
	      new_name = g_strdup_printf (_("%s copy"), name);
	    }

	  gimp_object_set_name (GIMP_OBJECT (new_data), new_name);

	  g_free (new_name);

	  gimp_container_add (view->factory->container, GIMP_OBJECT (new_data));

	  gimp_context_set_by_type (GIMP_CONTAINER_EDITOR (view)->view->context,
				    view->factory->container->children_type,
				    GIMP_OBJECT (new_data));

	  gimp_data_factory_view_edit_clicked (NULL, view);
	}
    }
}

static void
gimp_data_factory_view_edit_clicked (GtkWidget           *widget,
				     GimpDataFactoryView *view)
{
  GimpData *data;

  data = (GimpData *)
    gimp_context_get_by_type (GIMP_CONTAINER_EDITOR (view)->view->context,
			      view->factory->container->children_type);

  if (view->data_edit_func                           &&
      data                                           &&
      gimp_container_have (view->factory->container,
			   GIMP_OBJECT (data)))
    {
      view->data_edit_func (data);
    }
}

typedef struct _GimpDataDeleteData GimpDataDeleteData;

struct _GimpDataDeleteData
{
  GimpDataFactory *factory;
  GimpData        *data;
};

static void
gimp_data_factory_view_delete_callback (GtkWidget *widget,
					gboolean   delete,
					gpointer   data)
{
  GimpDataDeleteData *delete_data;

  delete_data = (GimpDataDeleteData *) data;

  if (! delete)
    return;

  if (gimp_container_have (delete_data->factory->container,
			   GIMP_OBJECT (delete_data->data)))
    {
      if (delete_data->data->filename)
	gimp_data_delete_from_disk (delete_data->data);

      gimp_container_remove (delete_data->factory->container,
			     GIMP_OBJECT (delete_data->data));
    }
}

static void
gimp_data_factory_view_delete_clicked (GtkWidget           *widget,
				       GimpDataFactoryView *view)
{
  GimpData *data;

  data = (GimpData *)
    gimp_context_get_by_type (GIMP_CONTAINER_EDITOR (view)->view->context,
			      view->factory->container->children_type);

  if (data && gimp_container_have (view->factory->container,
				   GIMP_OBJECT (data)))
    {
      GimpDataDeleteData *delete_data;
      GtkWidget          *dialog;
      gchar              *str;

      delete_data = g_new0 (GimpDataDeleteData, 1);

      delete_data->factory = view->factory;
      delete_data->data    = data;

      str = g_strdup_printf (_("Are you sure you want to delete\n"
			       "\"%s\" from the list and from disk?"),
			     GIMP_OBJECT (data)->name);

      dialog =
	gimp_query_boolean_box (_("Delete Data Object"),
				gimp_standard_help_func, NULL,
				FALSE,
				str,
				_("Delete"), _("Cancel"),
				GTK_OBJECT (data),
				"destroy",
				gimp_data_factory_view_delete_callback,
				delete_data);

      g_signal_connect_swapped (G_OBJECT (dialog), "destroy",
				G_CALLBACK (g_free),
				delete_data);

      g_free (str);

      gtk_widget_show (dialog);
    }
}

static void
gimp_data_factory_view_refresh_clicked (GtkWidget           *widget,
					GimpDataFactoryView *view)
{
  gimp_data_factory_data_save (view->factory);
  gimp_data_factory_data_init (view->factory, FALSE);
}

static void
gimp_data_factory_view_select_item (GimpContainerEditor *editor,
				    GimpViewable        *viewable)
{
  GimpDataFactoryView *view;

  gboolean  duplicate_sensitive = FALSE;
  gboolean  edit_sensitive      = FALSE;
  gboolean  delete_sensitive    = FALSE;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  view = GIMP_DATA_FACTORY_VIEW (editor);

  if (viewable && gimp_container_have (view->factory->container,
				       GIMP_OBJECT (viewable)))
    {
      duplicate_sensitive = (GIMP_DATA_GET_CLASS (viewable)->duplicate != NULL);

      edit_sensitive   = (view->data_edit_func != NULL);
      delete_sensitive = TRUE;  /* TODO: check permissions */
    }

  gtk_widget_set_sensitive (view->duplicate_button, duplicate_sensitive);
  gtk_widget_set_sensitive (view->edit_button,      edit_sensitive);
  gtk_widget_set_sensitive (view->delete_button,    delete_sensitive);
}

static void
gimp_data_factory_view_activate_item (GimpContainerEditor *editor,
				      GimpViewable        *viewable)
{
  GimpDataFactoryView *view;
  GimpData            *data;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  view = GIMP_DATA_FACTORY_VIEW (editor);
  data = GIMP_DATA (viewable);

  if (data && gimp_container_have (view->factory->container,
				   GIMP_OBJECT (data)))
    {
      gimp_data_factory_view_edit_clicked (NULL, view);
    }
}
