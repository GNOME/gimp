/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpundostack.h"

#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpdocked.h"
#include "gimphelp-ids.h"
#include "gimpmenufactory.h"
#include "gimpundoeditor.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_VIEW_SIZE
};


static void   gimp_undo_editor_docked_iface_init (GimpDockedInterface *iface);

static void   gimp_undo_editor_constructed    (GObject           *object);
static void   gimp_undo_editor_set_property   (GObject           *object,
                                               guint              property_id,
                                               const GValue      *value,
                                               GParamSpec        *pspec);

static void   gimp_undo_editor_set_image      (GimpImageEditor   *editor,
                                               GimpImage         *image);

static void   gimp_undo_editor_set_context    (GimpDocked        *docked,
                                               GimpContext       *context);

static void   gimp_undo_editor_fill           (GimpUndoEditor    *editor);
static void   gimp_undo_editor_clear          (GimpUndoEditor    *editor);

static void   gimp_undo_editor_undo_event     (GimpImage         *image,
                                               GimpUndoEvent      event,
                                               GimpUndo          *undo,
                                               GimpUndoEditor    *editor);

static void   gimp_undo_editor_selection_changed
                                              (GimpContainerView *view,
                                               GimpUndoEditor    *editor);


G_DEFINE_TYPE_WITH_CODE (GimpUndoEditor, gimp_undo_editor,
                         GIMP_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_undo_editor_docked_iface_init))

#define parent_class gimp_undo_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_undo_editor_class_init (GimpUndoEditorClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GimpImageEditorClass *image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

  object_class->constructed     = gimp_undo_editor_constructed;
  object_class->set_property    = gimp_undo_editor_set_property;

  image_editor_class->set_image = gimp_undo_editor_set_image;

  g_object_class_install_property (object_class, PROP_VIEW_SIZE,
                                   g_param_spec_enum ("view-size",
                                                      NULL, NULL,
                                                      GIMP_TYPE_VIEW_SIZE,
                                                      GIMP_VIEW_SIZE_LARGE,
                                                      GIMP_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_undo_editor_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_context = gimp_undo_editor_set_context;
}

static void
gimp_undo_editor_init (GimpUndoEditor *undo_editor)
{
}

static void
gimp_undo_editor_constructed (GObject *object)
{
  GimpUndoEditor *undo_editor = GIMP_UNDO_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  undo_editor->view = gimp_container_tree_view_new (NULL, NULL,
                                                    undo_editor->view_size,
                                                    1);

  gtk_box_pack_start (GTK_BOX (undo_editor), undo_editor->view, TRUE, TRUE, 0);
  gtk_widget_show (undo_editor->view);

  g_signal_connect (undo_editor->view, "selection-changed",
                    G_CALLBACK (gimp_undo_editor_selection_changed),
                    undo_editor);

  undo_editor->undo_button =
    gimp_editor_add_action_button (GIMP_EDITOR (undo_editor), "edit",
                                   "edit-undo", NULL);

  undo_editor->redo_button =
    gimp_editor_add_action_button (GIMP_EDITOR (undo_editor), "edit",
                                   "edit-redo", NULL);

  undo_editor->clear_button =
    gimp_editor_add_action_button (GIMP_EDITOR (undo_editor), "edit",
                                   "edit-undo-clear", NULL);
}

static void
gimp_undo_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpUndoEditor *undo_editor = GIMP_UNDO_EDITOR (object);

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
gimp_undo_editor_set_image (GimpImageEditor *image_editor,
                            GimpImage       *image)
{
  GimpUndoEditor *editor = GIMP_UNDO_EDITOR (image_editor);

  if (image_editor->image)
    {
      gimp_undo_editor_clear (editor);

      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_undo_editor_undo_event,
                                            editor);
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image_editor->image)
    {
      if (gimp_image_undo_is_enabled (image_editor->image))
        gimp_undo_editor_fill (editor);

      g_signal_connect (image_editor->image, "undo-event",
                        G_CALLBACK (gimp_undo_editor_undo_event),
                        editor);
    }
}

static void
gimp_undo_editor_set_context (GimpDocked  *docked,
                              GimpContext *context)
{
  GimpUndoEditor *editor = GIMP_UNDO_EDITOR (docked);

  if (editor->context)
    g_object_unref (editor->context);

  editor->context = context;

  if (editor->context)
    g_object_ref (editor->context);

  /* This calls gimp_undo_editor_set_image(), so make sure that it
   * isn't called before editor->context has been initialized.
   */
  parent_docked_iface->set_context (docked, context);

  gimp_container_view_set_context (GIMP_CONTAINER_VIEW (editor->view),
                                   context);
}


/*  public functions  */

GtkWidget *
gimp_undo_editor_new (GimpCoreConfig  *config,
                      GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_CORE_CONFIG (config), NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (GIMP_TYPE_UNDO_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<Undo>",
                       "ui-path",         "/undo-popup",
                       "view-size",       config->undo_preview_size,
                       NULL);
}


/*  private functions  */

static void
gimp_undo_editor_fill (GimpUndoEditor *editor)
{
  GimpImage     *image      = GIMP_IMAGE_EDITOR (editor)->image;
  GimpUndoStack *undo_stack = gimp_image_get_undo_stack (image);
  GimpUndoStack *redo_stack = gimp_image_get_redo_stack (image);
  GimpUndo      *top_undo_item;
  GList         *list;

  /*  create a container as model for the undo history list  */
  editor->container = gimp_list_new (GIMP_TYPE_UNDO, FALSE);
  editor->base_item = g_object_new (GIMP_TYPE_UNDO,
                                    "image", image,
                                    "name",  _("[ Base Image ]"),
                                    NULL);

  /*  the list prepends its items, so first add the redo items in
   *  reverse (ascending) order...
   */
  for (list = GIMP_LIST (redo_stack->undos)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      gimp_container_add (editor->container, GIMP_OBJECT (list->data));
    }

  /*  ...then add the undo items in descending order...  */
  for (list = GIMP_LIST (undo_stack->undos)->queue->head;
       list;
       list = g_list_next (list))
    {
      /*  Don't add the topmost item if it is an open undo group,
       *  it will be added upon closing of the group.
       */
      if (list->prev || ! GIMP_IS_UNDO_STACK (list->data) ||
          gimp_image_get_undo_group_count (image) == 0)
        {
          gimp_container_add (editor->container, GIMP_OBJECT (list->data));
        }
    }

  /*  ...finally, the first item is the special "base_item" which stands
   *  for the image with no more undos available to pop
   */
  gimp_container_add (editor->container, GIMP_OBJECT (editor->base_item));

  /*  display the container  */
  gimp_container_view_set_container (GIMP_CONTAINER_VIEW (editor->view),
                                     editor->container);

  top_undo_item = gimp_undo_stack_peek (undo_stack);

  g_signal_handlers_block_by_func (editor->view,
                                   gimp_undo_editor_selection_changed,
                                   editor);

  /*  select the current state of the image  */
  if (top_undo_item)
    {
      gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (editor->view),
                                          GIMP_VIEWABLE (top_undo_item));
      gimp_undo_create_preview (top_undo_item, editor->context, FALSE);
    }
  else
    {
      gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (editor->view),
                                          GIMP_VIEWABLE (editor->base_item));
      gimp_undo_create_preview (editor->base_item, editor->context, TRUE);
    }

  g_signal_handlers_unblock_by_func (editor->view,
                                     gimp_undo_editor_selection_changed,
                                     editor);
}

static void
gimp_undo_editor_clear (GimpUndoEditor *editor)
{
  if (editor->container)
    {
      gimp_container_view_set_container (GIMP_CONTAINER_VIEW (editor->view),
                                         NULL);
      g_clear_object (&editor->container);
    }

  g_clear_object (&editor->base_item);
}

static void
gimp_undo_editor_undo_event (GimpImage      *image,
                             GimpUndoEvent   event,
                             GimpUndo       *undo,
                             GimpUndoEditor *editor)
{
  GimpUndoStack *undo_stack    = gimp_image_get_undo_stack (image);
  GimpUndo      *top_undo_item = gimp_undo_stack_peek (undo_stack);

  switch (event)
    {
    case GIMP_UNDO_EVENT_UNDO_PUSHED:
      g_signal_handlers_block_by_func (editor->view,
                                       gimp_undo_editor_selection_changed,
                                       editor);

      gimp_container_insert (editor->container, GIMP_OBJECT (undo), -1);
      gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (editor->view),
                                          GIMP_VIEWABLE (undo));
      gimp_undo_create_preview (undo, editor->context, FALSE);

      g_signal_handlers_unblock_by_func (editor->view,
                                         gimp_undo_editor_selection_changed,
                                         editor);
      break;

    case GIMP_UNDO_EVENT_UNDO_EXPIRED:
    case GIMP_UNDO_EVENT_REDO_EXPIRED:
      gimp_container_remove (editor->container, GIMP_OBJECT (undo));
      break;

    case GIMP_UNDO_EVENT_UNDO:
    case GIMP_UNDO_EVENT_REDO:
      g_signal_handlers_block_by_func (editor->view,
                                       gimp_undo_editor_selection_changed,
                                       editor);

      if (top_undo_item)
        {
          gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (editor->view),
                                              GIMP_VIEWABLE (top_undo_item));
          gimp_undo_create_preview (top_undo_item, editor->context, FALSE);
        }
      else
        {
          gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (editor->view),
                                              GIMP_VIEWABLE (editor->base_item));
          gimp_undo_create_preview (editor->base_item, editor->context, TRUE);
        }

      g_signal_handlers_unblock_by_func (editor->view,
                                         gimp_undo_editor_selection_changed,
                                         editor);
      break;

    case GIMP_UNDO_EVENT_UNDO_FREE:
      if (gimp_image_undo_is_enabled (image))
        gimp_undo_editor_clear (editor);
      break;

    case GIMP_UNDO_EVENT_UNDO_FREEZE:
      gimp_undo_editor_clear (editor);
      break;

    case GIMP_UNDO_EVENT_UNDO_THAW:
      gimp_undo_editor_fill (editor);
      break;
    }
}

static void
gimp_undo_editor_selection_changed (GimpContainerView *view,
                                    GimpUndoEditor    *editor)
{
  GimpImage     *image      = GIMP_IMAGE_EDITOR (editor)->image;
  GimpUndoStack *undo_stack = gimp_image_get_undo_stack (image);
  GimpUndoStack *redo_stack = gimp_image_get_redo_stack (image);
  GimpUndo      *top_undo_item;
  GimpUndo      *undo;

  undo = GIMP_UNDO (gimp_container_view_get_1_selected (view));

  if (! undo)
    return;

  top_undo_item = gimp_undo_stack_peek (undo_stack);

  if (undo == editor->base_item)
    {
      /*  the base_item was selected, pop all available undo items
       */
      while (top_undo_item != NULL)
        {
          if (! gimp_image_undo (image))
            break;

          top_undo_item = gimp_undo_stack_peek (undo_stack);
        }
    }
  else if (gimp_container_have (undo_stack->undos, GIMP_OBJECT (undo)))
    {
      /*  the selected item is on the undo stack, pop undos until it
       *  is on top of the undo stack
       */
      while (top_undo_item != undo)
        {
          if(! gimp_image_undo (image))
            break;

          top_undo_item = gimp_undo_stack_peek (undo_stack);
        }
    }
  else if (gimp_container_have (redo_stack->undos, GIMP_OBJECT (undo)))
    {
      /*  the selected item is on the redo stack, pop redos until it
       *  is on top of the undo stack
       */
      while (top_undo_item != undo)
        {
          if (! gimp_image_redo (image))
            break;

          top_undo_item = gimp_undo_stack_peek (undo_stack);
        }
    }

  gimp_image_flush (image);
}
