/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptemplateview.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "config/gimpconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimpimage.h"
#include "core/gimptemplate.h"

#include "gimpcontainertreeview.h"
#include "gimptemplateview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimppreviewrenderer.h"

#include "gimp-intl.h"


static void   gimp_template_view_class_init (GimpTemplateViewClass *klass);
static void   gimp_template_view_init       (GimpTemplateView      *view);

static void   gimp_template_view_create_clicked    (GtkWidget        *widget,
                                                    GimpTemplateView *view);
static void   gimp_template_view_new_clicked       (GtkWidget        *widget,
                                                    GimpTemplateView *view);
static void   gimp_template_view_duplicate_clicked (GtkWidget        *widget,
                                                    GimpTemplateView *view);
static void   gimp_template_view_edit_clicked      (GtkWidget        *widget,
                                                    GimpTemplateView *view);
static void   gimp_template_view_delete_clicked    (GtkWidget        *widget,
                                                    GimpTemplateView *view);

static void   gimp_template_view_select_item    (GimpContainerEditor *editor,
                                                 GimpViewable        *viewable);
static void   gimp_template_view_activate_item  (GimpContainerEditor *editor,
                                                 GimpViewable        *viewable);

static void gimp_template_view_tree_name_edited (GtkCellRendererText *cell,
                                                 const gchar         *path_str,
                                                 const gchar         *new_name,
                                                 GimpTemplateView    *view);


static GimpContainerEditorClass *parent_class = NULL;


GType
gimp_template_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpTemplateViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_template_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpTemplateView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_template_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_CONTAINER_EDITOR,
                                          "GimpTemplateView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_template_view_class_init (GimpTemplateViewClass *klass)
{
  GimpContainerEditorClass *editor_class;

  editor_class = GIMP_CONTAINER_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_class->select_item   = gimp_template_view_select_item;
  editor_class->activate_item = gimp_template_view_activate_item;
}

static void
gimp_template_view_init (GimpTemplateView *view)
{
  view->create_image_func  = NULL;
  view->new_template_func  = NULL;
  view->edit_template_func = NULL;

  view->create_button     = NULL;
  view->new_button        = NULL;
  view->duplicate_button  = NULL;
  view->edit_button       = NULL;
  view->delete_button     = NULL;
}

GtkWidget *
gimp_template_view_new (GimpViewType     view_type,
                        GimpContainer   *container,
                        GimpContext     *context,
                        gint             preview_size,
                        gint             preview_border_width,
                        GimpMenuFactory *menu_factory)
{
  GimpTemplateView    *template_view;
  GimpContainerEditor *editor;

  template_view = g_object_new (GIMP_TYPE_TEMPLATE_VIEW, NULL);

  if (! gimp_container_editor_construct (GIMP_CONTAINER_EDITOR (template_view),
                                         view_type,
                                         container, context,
                                         preview_size, preview_border_width,
                                         TRUE, /* reorderable */
                                         menu_factory, "<Templates>",
                                         "/templates-popup"))
    {
      g_object_unref (template_view);
      return NULL;
    }

  editor = GIMP_CONTAINER_EDITOR (template_view);

  if (GIMP_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      GimpContainerTreeView *tree_view;

      tree_view = GIMP_CONTAINER_TREE_VIEW (editor->view);

      tree_view->name_cell->mode = GTK_CELL_RENDERER_MODE_EDITABLE;
      GTK_CELL_RENDERER_TEXT (tree_view->name_cell)->editable = TRUE;

      tree_view->editable_cells = g_list_prepend (tree_view->editable_cells,
                                                  tree_view->name_cell);

      g_signal_connect (tree_view->name_cell, "edited",
                        G_CALLBACK (gimp_template_view_tree_name_edited),
                        template_view);
    }

  template_view->create_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GIMP_STOCK_IMAGE,
                            _("Create a new image from the selected template"),
                            GIMP_HELP_TEMPLATE_IMAGE_NEW,
                            G_CALLBACK (gimp_template_view_create_clicked),
                            NULL,
                            editor);

  template_view->new_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_NEW,
                            _("Create a new template"),
                            GIMP_HELP_TEMPLATE_NEW,
                            G_CALLBACK (gimp_template_view_new_clicked),
                            NULL,
                            editor);

  template_view->duplicate_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GIMP_STOCK_DUPLICATE,
                            _("Duplicate the selected template"),
                            GIMP_HELP_TEMPLATE_DUPLICATE,
                            G_CALLBACK (gimp_template_view_duplicate_clicked),
                            NULL,
                            editor);

  template_view->edit_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GIMP_STOCK_EDIT,
                            _("Edit the selected template"),
                            GIMP_HELP_TEMPLATE_EDIT,
                            G_CALLBACK (gimp_template_view_edit_clicked),
                            NULL,
                            editor);

  template_view->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (editor->view),
                            GTK_STOCK_DELETE,
                            _("Delete the selected template"),
                            GIMP_HELP_TEMPLATE_DELETE,
                            G_CALLBACK (gimp_template_view_delete_clicked),
                            NULL,
                            editor);

  /*  set button sensitivity  */
  if (GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item)
    GIMP_CONTAINER_EDITOR_GET_CLASS (editor)->select_item
      (editor, (GimpViewable *) gimp_context_get_template (context));

  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (template_view->create_button),
				  GIMP_TYPE_TEMPLATE);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (template_view->duplicate_button),
				  GIMP_TYPE_TEMPLATE);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (template_view->edit_button),
				  GIMP_TYPE_TEMPLATE);
  gimp_container_view_enable_dnd (editor->view,
				  GTK_BUTTON (template_view->delete_button),
				  GIMP_TYPE_TEMPLATE);

  return GTK_WIDGET (template_view);
}

static void
gimp_template_view_create_clicked (GtkWidget        *widget,
                                   GimpTemplateView *view)
{
  GimpContainerEditor *editor;
  GimpTemplate        *template;

  editor = GIMP_CONTAINER_EDITOR (view);

  template = gimp_context_get_template (editor->view->context);

  if (template && gimp_container_have (editor->view->container,
                                        GIMP_OBJECT (template)))
    {
      if (view->create_image_func)
        view->create_image_func (editor->view->context->gimp, template,
                                 GTK_WIDGET (view));
    }
}

static void
gimp_template_view_new_clicked (GtkWidget        *widget,
                                GimpTemplateView *view)
{
  GimpContainerEditor *editor;
  GimpTemplate        *template;

  editor = GIMP_CONTAINER_EDITOR (view);

  template = gimp_context_get_template (editor->view->context);

  if (template && gimp_container_have (editor->view->container,
                                        GIMP_OBJECT (template)))
    {
    }

  if (view->new_template_func)
    view->new_template_func (editor->view->context->gimp, NULL,
                             GTK_WIDGET (view));
}

static void
gimp_template_view_duplicate_clicked (GtkWidget        *widget,
                                      GimpTemplateView *view)
{
  GimpContainerEditor *editor;
  GimpTemplate        *template;

  editor = GIMP_CONTAINER_EDITOR (view);

  template = gimp_context_get_template (editor->view->context);

  if (template && gimp_container_have (editor->view->container,
                                        GIMP_OBJECT (template)))
    {
      GimpTemplate *new_template;

      new_template = gimp_config_duplicate (GIMP_CONFIG (template));

      gimp_list_uniquefy_name (GIMP_LIST (editor->view->container),
                               GIMP_OBJECT (new_template), TRUE);
      gimp_container_add (editor->view->container, GIMP_OBJECT (new_template));

      gimp_context_set_by_type (editor->view->context,
                                editor->view->container->children_type,
                                GIMP_OBJECT (new_template));

      if (view->edit_template_func)
        view->edit_template_func (editor->view->context->gimp, new_template,
                                  GTK_WIDGET (view));

      g_object_unref (new_template);
    }
}

static void
gimp_template_view_edit_clicked (GtkWidget        *widget,
                                 GimpTemplateView *view)
{
  GimpContainerEditor *editor;
  GimpTemplate        *template;

  editor = GIMP_CONTAINER_EDITOR (view);

  template = gimp_context_get_template (editor->view->context);

  if (template && gimp_container_have (editor->view->container,
                                        GIMP_OBJECT (template)))
    {
      if (view->edit_template_func)
        view->edit_template_func (editor->view->context->gimp, template,
                                  GTK_WIDGET (view));
    }
}

typedef struct _GimpTemplateDeleteData GimpTemplateDeleteData;

struct _GimpTemplateDeleteData
{
  GimpContainer *container;
  GimpTemplate  *template;
};

static void
gimp_template_view_delete_callback (GtkWidget *widget,
                                    gboolean   delete,
                                    gpointer   data)
{
  GimpTemplateDeleteData *delete_data;

  delete_data = (GimpTemplateDeleteData *) data;

  if (! delete)
    return;

  if (gimp_container_have (delete_data->container,
			   GIMP_OBJECT (delete_data->template)))
    {
      gimp_container_remove (delete_data->container,
			     GIMP_OBJECT (delete_data->template));
    }
}

static void
gimp_template_view_delete_clicked (GtkWidget        *widget,
                                   GimpTemplateView *view)
{
  GimpContainerEditor *editor;
  GimpTemplate        *template;

  editor = GIMP_CONTAINER_EDITOR (view);

  template = gimp_context_get_template (editor->view->context);

  if (template && gimp_container_have (editor->view->container,
                                        GIMP_OBJECT (template)))
    {
      GimpTemplateDeleteData *delete_data;
      GtkWidget              *dialog;
      gchar                  *str;

      delete_data = g_new0 (GimpTemplateDeleteData, 1);

      delete_data->container = editor->view->container;
      delete_data->template  = template;

      str = g_strdup_printf (_("Are you sure you want to delete template '%s' "
                               "from the list and from disk?"),
			     GIMP_OBJECT (template)->name);

      dialog = gimp_query_boolean_box (_("Delete Template"),
                                       GTK_WIDGET (view),
				       gimp_standard_help_func, NULL,
				       GIMP_STOCK_QUESTION,
				       str,
				       GTK_STOCK_DELETE, GTK_STOCK_CANCEL,
				       G_OBJECT (template),
				       "disconnect",
				       gimp_template_view_delete_callback,
				       delete_data);

      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_free, delete_data);

      g_free (str);

      gtk_widget_show (dialog);
    }
}

static void
gimp_template_view_select_item (GimpContainerEditor *editor,
                                GimpViewable        *viewable)
{
  GimpTemplateView *view;
  gboolean          sensitive = FALSE;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  view = GIMP_TEMPLATE_VIEW (editor);

  if (viewable && gimp_container_have (editor->view->container,
                                       GIMP_OBJECT (viewable)))
    {
      sensitive = TRUE;
    }

  gtk_widget_set_sensitive (view->create_button,    sensitive);
  gtk_widget_set_sensitive (view->duplicate_button, sensitive);
  gtk_widget_set_sensitive (view->edit_button,      sensitive);
  gtk_widget_set_sensitive (view->delete_button,    sensitive);
}

static void
gimp_template_view_activate_item (GimpContainerEditor *editor,
                                  GimpViewable        *viewable)
{
  GimpTemplateView *view;

  if (GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    GIMP_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  view = GIMP_TEMPLATE_VIEW (editor);

  if (viewable && gimp_container_have (editor->view->container,
                                       GIMP_OBJECT (viewable)))
    {
      if (view->create_image_func)
        view->create_image_func (editor->view->context->gimp,
                                 GIMP_TEMPLATE (viewable),
                                 GTK_WIDGET (editor));
    }
}

static void
gimp_template_view_tree_name_edited (GtkCellRendererText *cell,
                                     const gchar         *path_str,
                                     const gchar         *new_name,
                                     GimpTemplateView    *view)
{
  GimpContainerTreeView *tree_view;
  GimpContainerView     *container_view;
  GtkTreePath           *path;
  GtkTreeIter            iter;

  tree_view = GIMP_CONTAINER_TREE_VIEW (GIMP_CONTAINER_EDITOR (view)->view);

  container_view = GIMP_CONTAINER_VIEW (tree_view);

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (tree_view->model, &iter, path))
    {
      GimpPreviewRenderer *renderer;
      GimpObject          *object;

      gtk_tree_model_get (tree_view->model, &iter,
                          tree_view->model_column_renderer, &renderer,
                          -1);

      object = GIMP_OBJECT (renderer->viewable);

      gimp_object_set_name (object, new_name);
      gimp_list_uniquefy_name (GIMP_LIST (container_view->context->gimp->templates),
                               object, TRUE);

      g_object_unref (renderer);
    }

  gtk_tree_path_free (path);
}
