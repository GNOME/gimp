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

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpundostack.h"

#include "gimpcontainerlistview.h"
#include "gimpundoeditor.h"

#include "libgimp/gimpintl.h"


static void   gimp_undo_editor_class_init   (GimpUndoEditorClass *klass);
static void   gimp_undo_editor_init         (GimpUndoEditor      *undo_editor);

static void   gimp_undo_editor_destroy      (GtkObject           *object);

static void   gimp_undo_editor_undo_clicked (GtkWidget           *widget,
                                             GimpUndoEditor      *editor);
static void   gimp_undo_editor_redo_clicked (GtkWidget           *widget,
                                             GimpUndoEditor      *editor);

static void   gimp_undo_editor_undo_event   (GimpImage           *gimage,
                                             GimpUndoEvent        event,
                                             GimpUndo            *undo,
                                             GimpUndoEditor      *editor);

static void   gimp_undo_editor_select_item  (GimpContainerView   *view,
                                             GimpUndo            *undo,
                                             gpointer             insert_data,
                                             GimpUndoEditor      *editor);


static GimpEditorClass *parent_class = NULL;


GType
gimp_undo_editor_get_type (void)
{
  static GType editor_type = 0;

  if (! editor_type)
    {
      static const GTypeInfo editor_info =
      {
        sizeof (GimpUndoEditorClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_undo_editor_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpUndoEditor),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_undo_editor_init,
      };

      editor_type = g_type_register_static (GIMP_TYPE_EDITOR,
                                            "GimpUndoEditor",
                                            &editor_info, 0);
    }

  return editor_type;
}

static void
gimp_undo_editor_class_init (GimpUndoEditorClass *klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = gimp_undo_editor_destroy;
}

static void
gimp_undo_editor_init (GimpUndoEditor *undo_editor)
{
  undo_editor->gimage    = NULL;
  undo_editor->container = NULL;

  undo_editor->view = gimp_container_list_view_new (NULL,
                                                    NULL,
                                                    GIMP_PREVIEW_SIZE_MEDIUM,
                                                    FALSE, 3, 3);
  gtk_box_pack_start (GTK_BOX (undo_editor), undo_editor->view, TRUE, TRUE, 0);
  gtk_widget_show (undo_editor->view);

  g_signal_connect (undo_editor->view, "select_item",
                    G_CALLBACK (gimp_undo_editor_select_item),
                    undo_editor);

  undo_editor->undo_button =
    gimp_editor_add_button (GIMP_EDITOR (undo_editor),
                            GTK_STOCK_UNDO,
                            _("Undo"), NULL,
                            G_CALLBACK (gimp_undo_editor_undo_clicked),
                            NULL,
                            undo_editor);

  undo_editor->redo_button =
    gimp_editor_add_button (GIMP_EDITOR (undo_editor),
                            GTK_STOCK_REDO,
                            _("Redo"), NULL,
                            G_CALLBACK (gimp_undo_editor_redo_clicked),
                            NULL,
                            undo_editor);

  gtk_widget_set_sensitive (GTK_WIDGET (undo_editor), FALSE);
}

static void
gimp_undo_editor_destroy (GtkObject *object)
{
  GimpUndoEditor *editor;

  editor = GIMP_UNDO_EDITOR (object);

  if (editor->gimage)
    gimp_undo_editor_set_image (editor, NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


/*  public functions  */

GtkWidget *
gimp_undo_editor_new (GimpImage *gimage)
{
  GimpUndoEditor *editor;

  g_return_val_if_fail (! gimage || GIMP_IS_IMAGE (gimage), NULL);

  editor = g_object_new (GIMP_TYPE_UNDO_EDITOR, NULL);

  gimp_undo_editor_set_image (editor, gimage);

  return GTK_WIDGET (editor);
}

void
gimp_undo_editor_set_image (GimpUndoEditor *editor,
                            GimpImage      *gimage)
{
  g_return_if_fail (GIMP_IS_UNDO_EDITOR (editor));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  if (gimage == editor->gimage)
    return;

  if (editor->gimage)
    {
      gimp_container_view_set_container (GIMP_CONTAINER_VIEW (editor->view),
                                         NULL);
      g_object_unref (editor->container);
      editor->container = NULL;

      g_object_unref (editor->base_item);
      editor->base_item = NULL;

      g_signal_handlers_disconnect_by_func (editor->gimage,
					    gimp_undo_editor_undo_event,
					    editor);
    }
  else if (gimage)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
    }

  editor->gimage = gimage;

  if (gimage)
    {
      GimpUndo *top_undo_item;
      GimpUndo *top_redo_item;
      GList    *list;

      /*  create a container as model for the undo history list  */
      editor->container = gimp_list_new (GIMP_TYPE_UNDO,
                                         GIMP_CONTAINER_POLICY_STRONG);
      editor->base_item = gimp_undo_new (gimage,
                                         GIMP_UNDO_GROUP_NONE,
                                         _("[ Base Image ]"),
                                         NULL, 0, FALSE, NULL, NULL);

      /*  the list prepends its items, so first add the redo items...  */
      for (list = GIMP_LIST (gimage->redo_stack->undos)->list;
           list;
           list = g_list_next (list))
        {
          gimp_container_add (editor->container, GIMP_OBJECT (list->data));
        }

      /*  ...reverse the list so the redo items are in ascending order...  */
      gimp_list_reverse (GIMP_LIST (editor->container));

      /*  ...then add the undo items in descending order...  */
      for (list = GIMP_LIST (gimage->undo_stack->undos)->list;
           list;
           list = g_list_next (list))
        {
          gimp_container_add (editor->container, GIMP_OBJECT (list->data));
        }

      /*  ...finally, the first item is the special "base_item" which stands
       *  for the image with no more undos available to pop
       */
      gimp_container_add (editor->container, GIMP_OBJECT (editor->base_item));

      /*  display the container  */
      gimp_container_view_set_container (GIMP_CONTAINER_VIEW (editor->view),
                                         editor->container);

      /*  get the top item of both stacks  */
      top_undo_item = gimp_undo_stack_peek (gimage->undo_stack);
      top_redo_item = gimp_undo_stack_peek (gimage->redo_stack);

      gtk_widget_set_sensitive (editor->undo_button, top_undo_item != NULL);
      gtk_widget_set_sensitive (editor->redo_button, top_redo_item != NULL);

      g_signal_handlers_block_by_func (editor->view,
                                       gimp_undo_editor_select_item,
                                       editor);

      /*  select the current state of the image  */
      if (top_undo_item)
        {
          gimp_container_view_select_item (GIMP_CONTAINER_VIEW (editor->view),
                                           GIMP_VIEWABLE (top_undo_item));
          gimp_undo_create_preview (top_undo_item, FALSE);
        }
      else
        {
          gimp_container_view_select_item (GIMP_CONTAINER_VIEW (editor->view),
                                           GIMP_VIEWABLE (editor->base_item));
          gimp_undo_create_preview (editor->base_item, TRUE);
        }

      g_signal_handlers_unblock_by_func (editor->view,
                                         gimp_undo_editor_select_item,
                                         editor);

      g_signal_connect (gimage, "undo_event",
			G_CALLBACK (gimp_undo_editor_undo_event),
			editor);
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
    }
}

static void
gimp_undo_editor_undo_clicked (GtkWidget      *widget,
                               GimpUndoEditor *editor)
{
  if (editor->gimage)
    {
      if (gimp_image_undo (editor->gimage))
        gimp_image_flush (editor->gimage);
    }
}

static void
gimp_undo_editor_redo_clicked (GtkWidget      *widget,
                               GimpUndoEditor *editor)
{
  if (editor->gimage)
    {
      if (gimp_image_redo (editor->gimage))
        gimp_image_flush (editor->gimage);
    }
}

static void
gimp_undo_editor_undo_event (GimpImage      *gimage,
                             GimpUndoEvent   event,
                             GimpUndo       *undo,
                             GimpUndoEditor *editor)
{
  GimpUndo *top_undo_item;
  GimpUndo *top_redo_item;

  top_undo_item = gimp_undo_stack_peek (gimage->undo_stack);
  top_redo_item = gimp_undo_stack_peek (gimage->redo_stack);

  g_signal_handlers_block_by_func (editor->view,
                                   gimp_undo_editor_select_item,
                                   editor);

  switch (event)
    {
    case GIMP_UNDO_EVENT_UNDO_PUSHED:
      gimp_container_insert (editor->container, GIMP_OBJECT (undo), -1);
      gimp_container_view_select_item (GIMP_CONTAINER_VIEW (editor->view),
                                       GIMP_VIEWABLE (undo));
      gimp_undo_create_preview (undo, FALSE);
      break;

    case GIMP_UNDO_EVENT_UNDO_EXPIRED:
    case GIMP_UNDO_EVENT_REDO_EXPIRED:
      gimp_container_remove (editor->container, GIMP_OBJECT (undo));
      break;

    case GIMP_UNDO_EVENT_UNDO:
      if (top_undo_item)
        {
          gimp_container_view_select_item (GIMP_CONTAINER_VIEW (editor->view),
                                           GIMP_VIEWABLE (top_undo_item));
          gimp_undo_create_preview (top_undo_item, FALSE);
        }
      else
        {
          gimp_container_view_select_item (GIMP_CONTAINER_VIEW (editor->view),
                                           GIMP_VIEWABLE (editor->base_item));
          gimp_undo_create_preview (editor->base_item, TRUE);
        }
      break;

    case GIMP_UNDO_EVENT_REDO:
      gimp_container_view_select_item (GIMP_CONTAINER_VIEW (editor->view),
                                       GIMP_VIEWABLE (top_undo_item));
      gimp_undo_create_preview (top_undo_item, FALSE);
      break;

    case GIMP_UNDO_EVENT_UNDO_FREE:
      gimp_undo_editor_set_image (editor, NULL);
      break;
    }

  g_signal_handlers_unblock_by_func (editor->view,
                                     gimp_undo_editor_select_item,
                                     editor);

  gtk_widget_set_sensitive (editor->undo_button, top_undo_item != NULL);
  gtk_widget_set_sensitive (editor->redo_button, top_redo_item != NULL);
}

static void
gimp_undo_editor_select_item (GimpContainerView *view,
                              GimpUndo          *undo,
                              gpointer           insert_data,
                              GimpUndoEditor    *editor)
{
  GimpUndo *top_undo_item;
  GimpUndo *top_redo_item;

  top_undo_item = gimp_undo_stack_peek (editor->gimage->undo_stack);

  if (undo == editor->base_item)
    {
      /*  the base_image was selected, pop all available undo items
       */
      while (top_undo_item != NULL)
        {
          gimp_image_undo (editor->gimage);

          top_undo_item = gimp_undo_stack_peek (editor->gimage->undo_stack);
        }
    }
  else if (gimp_container_have (editor->gimage->undo_stack->undos,
                                GIMP_OBJECT (undo)))
    {
      /*  the selected item is on the undo stack, pop undos until it
       *  is on the of the undo stack
       */
      while (top_undo_item != undo)
        {
          gimp_image_undo (editor->gimage);

          top_undo_item = gimp_undo_stack_peek (editor->gimage->undo_stack);
        }
    }
  else if (gimp_container_have (editor->gimage->redo_stack->undos,
                                GIMP_OBJECT (undo)))
    {
      /*  the selected item is on the redo stack, pop redos until it
       *  is on top of the undo stack
       */
      while (top_undo_item != undo)
        {
          gimp_image_redo (editor->gimage);

          top_undo_item = gimp_undo_stack_peek (editor->gimage->undo_stack);
        }
    }

  gimp_image_flush (editor->gimage);

  top_redo_item = gimp_undo_stack_peek (editor->gimage->redo_stack);

  gtk_widget_set_sensitive (editor->undo_button, top_undo_item != NULL);
  gtk_widget_set_sensitive (editor->redo_button, top_redo_item != NULL);
}
