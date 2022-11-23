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

#include "widgets-types.h"

#include "core/ligmacontext.h"
#include "core/ligmaimage.h"

#include "ligmadocked.h"
#include "ligmaimageeditor.h"
#include "ligmauimanager.h"


static void   ligma_image_editor_docked_iface_init (LigmaDockedInterface *iface);

static void   ligma_image_editor_set_context    (LigmaDocked       *docked,
                                                LigmaContext      *context);

static void   ligma_image_editor_dispose        (GObject          *object);

static void   ligma_image_editor_real_set_image (LigmaImageEditor  *editor,
                                                LigmaImage        *image);
static void   ligma_image_editor_image_flush    (LigmaImage        *image,
                                                gboolean          invalidate_preview,
                                                LigmaImageEditor  *editor);


G_DEFINE_TYPE_WITH_CODE (LigmaImageEditor, ligma_image_editor, LIGMA_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_image_editor_docked_iface_init))

#define parent_class ligma_image_editor_parent_class


static void
ligma_image_editor_class_init (LigmaImageEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ligma_image_editor_dispose;

  klass->set_image      = ligma_image_editor_real_set_image;
}

static void
ligma_image_editor_init (LigmaImageEditor *editor)
{
  editor->image = NULL;

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

static void
ligma_image_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  iface->set_context = ligma_image_editor_set_context;
}

static void
ligma_image_editor_set_context (LigmaDocked  *docked,
                               LigmaContext *context)
{
  LigmaImageEditor *editor = LIGMA_IMAGE_EDITOR (docked);
  LigmaImage       *image  = NULL;

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            ligma_image_editor_set_image,
                                            editor);

      g_object_unref (editor->context);
    }

  editor->context = context;

  if (context)
    {
      g_object_ref (editor->context);

      g_signal_connect_swapped (context, "image-changed",
                                G_CALLBACK (ligma_image_editor_set_image),
                                editor);

      image = ligma_context_get_image (context);
    }

  ligma_image_editor_set_image (editor, image);
}

static void
ligma_image_editor_dispose (GObject *object)
{
  LigmaImageEditor *editor = LIGMA_IMAGE_EDITOR (object);

  if (editor->image)
    ligma_image_editor_set_image (editor, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_image_editor_real_set_image (LigmaImageEditor *editor,
                                  LigmaImage       *image)
{
  if (editor->image)
    g_signal_handlers_disconnect_by_func (editor->image,
                                          ligma_image_editor_image_flush,
                                          editor);

  editor->image = image;

  if (editor->image)
    g_signal_connect (editor->image, "flush",
                      G_CALLBACK (ligma_image_editor_image_flush),
                      editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), image != NULL);
}


/*  public functions  */

void
ligma_image_editor_set_image (LigmaImageEditor *editor,
                             LigmaImage       *image)
{
  g_return_if_fail (LIGMA_IS_IMAGE_EDITOR (editor));
  g_return_if_fail (image == NULL || LIGMA_IS_IMAGE (image));

  if (image != editor->image)
    {
      LIGMA_IMAGE_EDITOR_GET_CLASS (editor)->set_image (editor, image);

      if (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)))
        ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                                ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
    }
}

LigmaImage *
ligma_image_editor_get_image (LigmaImageEditor *editor)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE_EDITOR (editor), NULL);

  return editor->image;
}


/*  private functions  */

static void
ligma_image_editor_image_flush (LigmaImage       *image,
                               gboolean         invalidate_preview,
                               LigmaImageEditor *editor)
{
  if (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)))
    ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                            ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
}
