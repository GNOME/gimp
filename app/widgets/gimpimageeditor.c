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

#include "widgets-types.h"

#include "core/gimpimage.h"

#include "gimpimageeditor.h"


static void   gimp_image_editor_class_init     (GimpImageEditorClass *klass);
static void   gimp_image_editor_init           (GimpImageEditor      *editor);

static void   gimp_image_editor_destroy        (GtkObject            *object);

static void   gimp_image_editor_real_set_image (GimpImageEditor      *editor,
                                                GimpImage            *gimage);


static GimpEditorClass *parent_class = NULL;


GType
gimp_image_editor_get_type (void)
{
  static GType editor_type = 0;

  if (! editor_type)
    {
      static const GTypeInfo editor_info =
      {
        sizeof (GimpImageEditorClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_image_editor_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpImageEditor),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_image_editor_init,
      };

      editor_type = g_type_register_static (GIMP_TYPE_EDITOR,
                                            "GimpImageEditor",
                                            &editor_info, 0);
    }

  return editor_type;
}

static void
gimp_image_editor_class_init (GimpImageEditorClass *klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = gimp_image_editor_destroy;

  klass->set_image      = gimp_image_editor_real_set_image;
}

static void
gimp_image_editor_init (GimpImageEditor *editor)
{
  editor->gimage = NULL;

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

static void
gimp_image_editor_destroy (GtkObject *object)
{
  GimpImageEditor *editor;

  editor = GIMP_IMAGE_EDITOR (object);

  if (editor->gimage)
    gimp_image_editor_set_image (editor, NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_image_editor_real_set_image (GimpImageEditor *editor,
                                  GimpImage       *gimage)
{
  editor->gimage = gimage;

  gtk_widget_set_sensitive (GTK_WIDGET (editor), gimage != NULL);
}


/*  public functions  */

void
gimp_image_editor_set_image (GimpImageEditor *editor,
                             GimpImage       *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE_EDITOR (editor));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  if (gimage == editor->gimage)
    return;

  GIMP_IMAGE_EDITOR_GET_CLASS (editor)->set_image (editor, gimage);
}
