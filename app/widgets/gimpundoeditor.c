/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmalist.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaundostack.h"

#include "ligmacontainertreeview.h"
#include "ligmacontainerview.h"
#include "ligmadocked.h"
#include "ligmahelp-ids.h"
#include "ligmamenufactory.h"
#include "ligmaundoeditor.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_VIEW_SIZE
};


static void   ligma_undo_editor_docked_iface_init (LigmaDockedInterface *iface);

static void   ligma_undo_editor_constructed    (GObject           *object);
static void   ligma_undo_editor_set_property   (GObject           *object,
                                               guint              property_id,
                                               const GValue      *value,
                                               GParamSpec        *pspec);

static void   ligma_undo_editor_set_image      (LigmaImageEditor   *editor,
                                               LigmaImage         *image);

static void   ligma_undo_editor_set_context    (LigmaDocked        *docked,
                                               LigmaContext       *context);

static void   ligma_undo_editor_fill           (LigmaUndoEditor    *editor);
static void   ligma_undo_editor_clear          (LigmaUndoEditor    *editor);

static void   ligma_undo_editor_undo_event     (LigmaImage         *image,
                                               LigmaUndoEvent      event,
                                               LigmaUndo          *undo,
                                               LigmaUndoEditor    *editor);

static gboolean ligma_undo_editor_select_items (LigmaContainerView *view,
                                               GList             *undos,
                                               GList             *paths,
                                               LigmaUndoEditor    *editor);


G_DEFINE_TYPE_WITH_CODE (LigmaUndoEditor, ligma_undo_editor,
                         LIGMA_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_undo_editor_docked_iface_init))

#define parent_class ligma_undo_editor_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_undo_editor_class_init (LigmaUndoEditorClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  LigmaImageEditorClass *image_editor_class = LIGMA_IMAGE_EDITOR_CLASS (klass);

  object_class->constructed     = ligma_undo_editor_constructed;
  object_class->set_property    = ligma_undo_editor_set_property;

  image_editor_class->set_image = ligma_undo_editor_set_image;

  g_object_class_install_property (object_class, PROP_VIEW_SIZE,
                                   g_param_spec_enum ("view-size",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_VIEW_SIZE,
                                                      LIGMA_VIEW_SIZE_LARGE,
                                                      LIGMA_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_undo_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_context = ligma_undo_editor_set_context;
}

static void
ligma_undo_editor_init (LigmaUndoEditor *undo_editor)
{
}

static void
ligma_undo_editor_constructed (GObject *object)
{
  LigmaUndoEditor *undo_editor = LIGMA_UNDO_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  undo_editor->view = ligma_container_tree_view_new (NULL, NULL,
                                                    undo_editor->view_size,
                                                    1);

  gtk_box_pack_start (GTK_BOX (undo_editor), undo_editor->view, TRUE, TRUE, 0);
  gtk_widget_show (undo_editor->view);

  g_signal_connect (undo_editor->view, "select-items",
                    G_CALLBACK (ligma_undo_editor_select_items),
                    undo_editor);

  undo_editor->undo_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (undo_editor), "edit",
                                   "edit-undo", NULL);

  undo_editor->redo_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (undo_editor), "edit",
                                   "edit-redo", NULL);

  undo_editor->clear_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (undo_editor), "edit",
                                   "edit-undo-clear", NULL);
}

static void
ligma_undo_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaUndoEditor *undo_editor = LIGMA_UNDO_EDITOR (object);

  switch (property_id)
    {
    case PROP_VIEW_SIZE:
      undo_editor->view_size = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_undo_editor_set_image (LigmaImageEditor *image_editor,
                            LigmaImage       *image)
{
  LigmaUndoEditor *editor = LIGMA_UNDO_EDITOR (image_editor);

  if (image_editor->image)
    {
      ligma_undo_editor_clear (editor);

      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            ligma_undo_editor_undo_event,
                                            editor);
    }

  LIGMA_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image_editor->image)
    {
      if (ligma_image_undo_is_enabled (image_editor->image))
        ligma_undo_editor_fill (editor);

      g_signal_connect (image_editor->image, "undo-event",
                        G_CALLBACK (ligma_undo_editor_undo_event),
                        editor);
    }
}

static void
ligma_undo_editor_set_context (LigmaDocked  *docked,
                              LigmaContext *context)
{
  LigmaUndoEditor *editor = LIGMA_UNDO_EDITOR (docked);

  if (editor->context)
    g_object_unref (editor->context);

  editor->context = context;

  if (editor->context)
    g_object_ref (editor->context);

  /* This calls ligma_undo_editor_set_image(), so make sure that it
   * isn't called before editor->context has been initialized.
   */
  parent_docked_iface->set_context (docked, context);

  ligma_container_view_set_context (LIGMA_CONTAINER_VIEW (editor->view),
                                   context);
}


/*  public functions  */

GtkWidget *
ligma_undo_editor_new (LigmaCoreConfig  *config,
                      LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_CORE_CONFIG (config), NULL);
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (LIGMA_TYPE_UNDO_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<Undo>",
                       "ui-path",         "/undo-popup",
                       "view-size",       config->undo_preview_size,
                       NULL);
}


/*  private functions  */

static void
ligma_undo_editor_fill (LigmaUndoEditor *editor)
{
  LigmaImage     *image      = LIGMA_IMAGE_EDITOR (editor)->image;
  LigmaUndoStack *undo_stack = ligma_image_get_undo_stack (image);
  LigmaUndoStack *redo_stack = ligma_image_get_redo_stack (image);
  LigmaUndo      *top_undo_item;
  GList         *list;

  /*  create a container as model for the undo history list  */
  editor->container = ligma_list_new (LIGMA_TYPE_UNDO, FALSE);
  editor->base_item = g_object_new (LIGMA_TYPE_UNDO,
                                    "image", image,
                                    "name",  _("[ Base Image ]"),
                                    NULL);

  /*  the list prepends its items, so first add the redo items in
   *  reverse (ascending) order...
   */
  for (list = LIGMA_LIST (redo_stack->undos)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      ligma_container_add (editor->container, LIGMA_OBJECT (list->data));
    }

  /*  ...then add the undo items in descending order...  */
  for (list = LIGMA_LIST (undo_stack->undos)->queue->head;
       list;
       list = g_list_next (list))
    {
      /*  Don't add the topmost item if it is an open undo group,
       *  it will be added upon closing of the group.
       */
      if (list->prev || ! LIGMA_IS_UNDO_STACK (list->data) ||
          ligma_image_get_undo_group_count (image) == 0)
        {
          ligma_container_add (editor->container, LIGMA_OBJECT (list->data));
        }
    }

  /*  ...finally, the first item is the special "base_item" which stands
   *  for the image with no more undos available to pop
   */
  ligma_container_add (editor->container, LIGMA_OBJECT (editor->base_item));

  /*  display the container  */
  ligma_container_view_set_container (LIGMA_CONTAINER_VIEW (editor->view),
                                     editor->container);

  top_undo_item = ligma_undo_stack_peek (undo_stack);

  g_signal_handlers_block_by_func (editor->view,
                                   ligma_undo_editor_select_items,
                                   editor);

  /*  select the current state of the image  */
  if (top_undo_item)
    {
      ligma_container_view_select_item (LIGMA_CONTAINER_VIEW (editor->view),
                                       LIGMA_VIEWABLE (top_undo_item));
      ligma_undo_create_preview (top_undo_item, editor->context, FALSE);
    }
  else
    {
      ligma_container_view_select_item (LIGMA_CONTAINER_VIEW (editor->view),
                                       LIGMA_VIEWABLE (editor->base_item));
      ligma_undo_create_preview (editor->base_item, editor->context, TRUE);
    }

  g_signal_handlers_unblock_by_func (editor->view,
                                     ligma_undo_editor_select_items,
                                     editor);
}

static void
ligma_undo_editor_clear (LigmaUndoEditor *editor)
{
  if (editor->container)
    {
      ligma_container_view_set_container (LIGMA_CONTAINER_VIEW (editor->view),
                                         NULL);
      g_clear_object (&editor->container);
    }

  g_clear_object (&editor->base_item);
}

static void
ligma_undo_editor_undo_event (LigmaImage      *image,
                             LigmaUndoEvent   event,
                             LigmaUndo       *undo,
                             LigmaUndoEditor *editor)
{
  LigmaUndoStack *undo_stack    = ligma_image_get_undo_stack (image);
  LigmaUndo      *top_undo_item = ligma_undo_stack_peek (undo_stack);

  switch (event)
    {
    case LIGMA_UNDO_EVENT_UNDO_PUSHED:
      g_signal_handlers_block_by_func (editor->view,
                                       ligma_undo_editor_select_items,
                                       editor);

      ligma_container_insert (editor->container, LIGMA_OBJECT (undo), -1);
      ligma_container_view_select_item (LIGMA_CONTAINER_VIEW (editor->view),
                                       LIGMA_VIEWABLE (undo));
      ligma_undo_create_preview (undo, editor->context, FALSE);

      g_signal_handlers_unblock_by_func (editor->view,
                                         ligma_undo_editor_select_items,
                                         editor);
      break;

    case LIGMA_UNDO_EVENT_UNDO_EXPIRED:
    case LIGMA_UNDO_EVENT_REDO_EXPIRED:
      ligma_container_remove (editor->container, LIGMA_OBJECT (undo));
      break;

    case LIGMA_UNDO_EVENT_UNDO:
    case LIGMA_UNDO_EVENT_REDO:
      g_signal_handlers_block_by_func (editor->view,
                                       ligma_undo_editor_select_items,
                                       editor);

      if (top_undo_item)
        {
          ligma_container_view_select_item (LIGMA_CONTAINER_VIEW (editor->view),
                                           LIGMA_VIEWABLE (top_undo_item));
          ligma_undo_create_preview (top_undo_item, editor->context, FALSE);
        }
      else
        {
          ligma_container_view_select_item (LIGMA_CONTAINER_VIEW (editor->view),
                                           LIGMA_VIEWABLE (editor->base_item));
          ligma_undo_create_preview (editor->base_item, editor->context, TRUE);
        }

      g_signal_handlers_unblock_by_func (editor->view,
                                         ligma_undo_editor_select_items,
                                         editor);
      break;

    case LIGMA_UNDO_EVENT_UNDO_FREE:
      if (ligma_image_undo_is_enabled (image))
        ligma_undo_editor_clear (editor);
      break;

    case LIGMA_UNDO_EVENT_UNDO_FREEZE:
      ligma_undo_editor_clear (editor);
      break;

    case LIGMA_UNDO_EVENT_UNDO_THAW:
      ligma_undo_editor_fill (editor);
      break;
    }
}

static gboolean
ligma_undo_editor_select_items (LigmaContainerView *view,
                               GList             *undos,
                               GList             *paths,
                               LigmaUndoEditor    *editor)
{
  LigmaImage     *image      = LIGMA_IMAGE_EDITOR (editor)->image;
  LigmaUndoStack *undo_stack = ligma_image_get_undo_stack (image);
  LigmaUndoStack *redo_stack = ligma_image_get_redo_stack (image);
  LigmaUndo      *top_undo_item;
  LigmaUndo      *undo;

  g_return_val_if_fail (g_list_length (undos) < 2, FALSE);

  if (! undos)
    return TRUE;

  undo = undos->data;

  top_undo_item = ligma_undo_stack_peek (undo_stack);

  if (undo == editor->base_item)
    {
      /*  the base_item was selected, pop all available undo items
       */
      while (top_undo_item != NULL)
        {
          if (! ligma_image_undo (image))
            break;

          top_undo_item = ligma_undo_stack_peek (undo_stack);
        }
    }
  else if (ligma_container_have (undo_stack->undos, LIGMA_OBJECT (undo)))
    {
      /*  the selected item is on the undo stack, pop undos until it
       *  is on top of the undo stack
       */
      while (top_undo_item != undo)
        {
          if(! ligma_image_undo (image))
            break;

          top_undo_item = ligma_undo_stack_peek (undo_stack);
        }
    }
  else if (ligma_container_have (redo_stack->undos, LIGMA_OBJECT (undo)))
    {
      /*  the selected item is on the redo stack, pop redos until it
       *  is on top of the undo stack
       */
      while (top_undo_item != undo)
        {
          if (! ligma_image_redo (image))
            break;

          top_undo_item = ligma_undo_stack_peek (undo_stack);
        }
    }

  ligma_image_flush (image);

  return TRUE;
}
