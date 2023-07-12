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

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gimpdocked.h"
#include "gimpimageeditor.h"
#include "gimpuimanager.h"


static void   gimp_image_editor_docked_iface_init (GimpDockedInterface *iface);

static void   gimp_image_editor_set_context    (GimpDocked       *docked,
                                                GimpContext      *context);

static void   gimp_image_editor_dispose        (GObject          *object);

static void   gimp_image_editor_real_set_image (GimpImageEditor  *editor,
                                                GimpImage        *image);
static void   gimp_image_editor_image_flush    (GimpImage        *image,
                                                gboolean          invalidate_preview,
                                                GimpImageEditor  *editor);
static gboolean
              gimp_image_editor_image_flush_idle
                                               (gpointer user_data);


G_DEFINE_TYPE_WITH_CODE (GimpImageEditor, gimp_image_editor, GIMP_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_image_editor_docked_iface_init))

#define parent_class gimp_image_editor_parent_class


static void
gimp_image_editor_class_init (GimpImageEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gimp_image_editor_dispose;

  klass->set_image      = gimp_image_editor_real_set_image;
}

static void
gimp_image_editor_init (GimpImageEditor *editor)
{
  editor->image = NULL;

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

static void
gimp_image_editor_docked_iface_init (GimpDockedInterface *iface)
{
  iface->set_context = gimp_image_editor_set_context;
}

static void
gimp_image_editor_set_context (GimpDocked  *docked,
                               GimpContext *context)
{
  GimpImageEditor *editor = GIMP_IMAGE_EDITOR (docked);
  GimpImage       *image  = NULL;

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            gimp_image_editor_set_image,
                                            editor);

      g_object_unref (editor->context);
    }

  editor->context = context;

  if (context)
    {
      g_object_ref (editor->context);

      g_signal_connect_swapped (context, "image-changed",
                                G_CALLBACK (gimp_image_editor_set_image),
                                editor);

      image = gimp_context_get_image (context);
    }

  gimp_image_editor_set_image (editor, image);
}

static void
gimp_image_editor_dispose (GObject *object)
{
  GimpImageEditor *editor = GIMP_IMAGE_EDITOR (object);

  if (editor->image)
    gimp_image_editor_set_image (editor, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_image_editor_real_set_image (GimpImageEditor *editor,
                                  GimpImage       *image)
{
  if (editor->image)
    g_signal_handlers_disconnect_by_func (editor->image,
                                          gimp_image_editor_image_flush,
                                          editor);

  editor->image = image;

  if (editor->image)
    g_signal_connect (editor->image, "flush",
                      G_CALLBACK (gimp_image_editor_image_flush),
                      editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), image != NULL);
}


/*  public functions  */

void
gimp_image_editor_set_image (GimpImageEditor *editor,
                             GimpImage       *image)
{
  g_return_if_fail (GIMP_IS_IMAGE_EDITOR (editor));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  if (image != editor->image)
    {
      GIMP_IMAGE_EDITOR_GET_CLASS (editor)->set_image (editor, image);

      if (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)))
        gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                                gimp_editor_get_popup_data (GIMP_EDITOR (editor)));
    }
}

GimpImage *
gimp_image_editor_get_image (GimpImageEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_EDITOR (editor), NULL);

  return editor->image;
}


/*  private functions  */

static void
gimp_image_editor_image_flush (GimpImage       *image,
                               gboolean         invalidate_preview,
                               GimpImageEditor *editor)
{
  g_idle_add_full (G_PRIORITY_LOW,
                   (GSourceFunc) gimp_image_editor_image_flush_idle,
                   g_object_ref (editor), g_object_unref);
}

static gboolean
gimp_image_editor_image_flush_idle (gpointer user_data)
{
  GimpImageEditor *editor = user_data;

  if (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)))
    gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                            gimp_editor_get_popup_data (GIMP_EDITOR (editor)));

  return G_SOURCE_REMOVE;
}
