/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactoryview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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


static void   gimp_data_factory_view_class_init (GimpDataFactoryViewClass *klass);
static void   gimp_data_factory_view_init       (GimpDataFactoryView      *view);

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


GType
gimp_data_factory_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpDataFactoryViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_data_factory_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpDataFactoryView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_data_factory_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_EDITOR,
                                          "GimpDataFactoryView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_data_factory_view_class_init (GimpDataFactoryViewClass *klass)
{
  GimpContainerEditorClass *editor_class;

  editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_class->select_item   = gimp_data_factory_view_select_item;
  editor_class->activate_item = gimp_data_factory_view_activate_item;
}

static void
gimp_data_factory_view_init (GimpDataFactoryView *view)
{
  view->new_button       = NULL;
  view->duplicate_button = NULL;
  view->edit_button      = NULL;
  view->delete_button    = NULL;
  view->refresh_button   = NULL;
}

GtkWidget *
gimp_data_factory_view_new (GimpViewType      view_type,
			    GimpDataFactory  *factory,
			    GimpDataEditFunc  edit_func,
			    GimpContext      *context,
			    gint              preview_size,
			    gint              min_items_x,
			    gint              min_items_y,
			    const gchar      *item_factory)
{
  GimpDataFactoryView *factory_view;

  factory_view = g_object_new (GIMP_TYPE_DATA_FACTORY_VIEW, NULL);

  if (! gimp_data_factory_view_construct (factory_view,
					  view_type,
					  factory,
					  edit_func,
					  context,
					  preview_size,
					  min_items_x,
					  min_items_y,
					  item_factory))
    {
      g_object_unref (G_OBJECT (factory_view));
      return NULL;
    }

  return GTK_WIDGET (factory_view);
}

gboolean
gimp_data_factory_view_construct (GimpDataFactoryView *factory_view,
				  GimpViewType         view_type,
				  GimpDataFactory     *factory,
				  GimpDataEditFunc     edit_func,
				  GimpContext         *context,
				  gint                 preview_size,
				  gint                 min_items_x,
				  gint                 min_items_y,
				  const gchar         *item_factory)
{
  GimpContainerEditor *editor;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY_VIEW (factory_view), FALSE);
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
                                         FALSE, /* reorderable */
					 min_items_x,
					 min_items_y,
					 item_factory))
    {
      return FALSE;
    }

  editor = GIMP_CONTAINER_EDITOR (factory_view);

  factory_view->new_button =
    gimp_container_view_add_button (editor->view,
				    GTK_STOCK_NEW,
				    _("New"), NULL,
				    G_CALLBACK (gimp_data_factory_view_new_clicked),
				    NULL,
				    editor);

  factory_view->duplicate_button =
    gimp_container_view_add_button (editor->view,
				    GIMP_STOCK_DUPLICATE,
				    _("Duplicate"), NULL,
				    G_CALLBACK (gimp_data_factory_view_duplicate_clicked),
				    NULL,
				    editor);

  factory_view->edit_button =
    gimp_container_view_add_button (editor->view,
				    GIMP_STOCK_EDIT,
				    _("Edit"), NULL,
				    G_CALLBACK (gimp_data_factory_view_edit_clicked),
				    NULL,
				    editor);

  factory_view->delete_button =
    gimp_container_view_add_button (editor->view,
				    GTK_STOCK_DELETE,
				    _("Delete"), NULL,
				    G_CALLBACK (gimp_data_factory_view_delete_clicked),
				    NULL,
				    editor);

  factory_view->refresh_button =
    gimp_container_view_add_button (editor->view,
				    GTK_STOCK_REFRESH,
				    _("Refresh"), NULL,
				    G_CALLBACK (gimp_data_factory_view_refresh_clicked),
				    NULL,
				    editor);

  /*  set button sensitivity  */
  if (GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item)
    GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item
      (editor,
       (GimpViewable *) gimp_context_get_by_type (context,
						  factory->container->children_type));

  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (factory_view->duplicate_button),
				  factory->container->children_type);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (factory_view->edit_button),
				  factory->container->children_type);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (factory_view->delete_button),
				  factory->container->children_type);

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

      dialog = gimp_query_boolean_box (_("Delete Data Object"),
				       gimp_standard_help_func, NULL,
				       GTK_STOCK_DIALOG_QUESTION,
				       str,
				       GTK_STOCK_DELETE, GTK_STOCK_CANCEL,
				       G_OBJECT (data),
				       "disconnect",
				       gimp_data_factory_view_delete_callback,
				       delete_data);

      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_free, delete_data);

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
