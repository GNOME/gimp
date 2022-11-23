/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadeviceeditor.h
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DEVICE_EDITOR_H__
#define __LIGMA_DEVICE_EDITOR_H__


#define LIGMA_TYPE_DEVICE_EDITOR            (ligma_device_editor_get_type ())
#define LIGMA_DEVICE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DEVICE_EDITOR, LigmaDeviceEditor))
#define LIGMA_DEVICE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DEVICE_EDITOR, LigmaDeviceEditorClass))
#define LIGMA_IS_DEVICE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DEVICE_EDITOR))
#define LIGMA_IS_DEVICE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DEVICE_EDITOR))
#define LIGMA_DEVICE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DEVICE_EDITOR, LigmaDeviceEditorClass))


typedef struct _LigmaDeviceEditorClass LigmaDeviceEditorClass;

struct _LigmaDeviceEditor
{
  GtkPaned  parent_instance;
};

struct _LigmaDeviceEditorClass
{
  GtkPanedClass  parent_class;
};


GType       ligma_device_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_device_editor_new      (Ligma *ligma);


#endif /* __LIGMA_DEVICE_EDITOR_H__ */
