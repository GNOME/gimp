/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaiconpicker.h
 * Copyright (C) 2011 Michael Natterer <mitch@ligma.org>
 *               2012 Daniel Sabo
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

#ifndef __LIGMA_ICON_PICKER_H__
#define __LIGMA_ICON_PICKER_H__


#define LIGMA_TYPE_ICON_PICKER            (ligma_icon_picker_get_type ())
#define LIGMA_ICON_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ICON_PICKER, LigmaIconPicker))
#define LIGMA_ICON_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ICON_PICKER, LigmaIconPickerClass))
#define LIGMA_IS_ICON_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ICON_PICKER))
#define LIGMA_IS_ICON_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ICON_PICKER))
#define LIGMA_ICON_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ICON_PICKER, LigmaIconPickerClass))


typedef struct _LigmaIconPickerClass LigmaIconPickerClass;

struct _LigmaIconPicker
{
  GtkBox  parent_instance;
};

struct _LigmaIconPickerClass
{
  GtkBoxClass   parent_class;
};


GType          ligma_icon_picker_get_type       (void) G_GNUC_CONST;

GtkWidget   * ligma_icon_picker_new             (Ligma           *ligma);

const gchar * ligma_icon_picker_get_icon_name   (LigmaIconPicker *picker);
void          ligma_icon_picker_set_icon_name   (LigmaIconPicker *picker,
                                                const gchar    *icon_name);

GdkPixbuf   * ligma_icon_picker_get_icon_pixbuf (LigmaIconPicker *picker);
void          ligma_icon_picker_set_icon_pixbuf (LigmaIconPicker *picker,
                                                GdkPixbuf      *value);


#endif  /*  __LIGMA_ICON_PICKER_H__  */
