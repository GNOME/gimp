/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpiconpicker.h
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
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

#pragma once


#define GIMP_TYPE_ICON_PICKER            (gimp_icon_picker_get_type ())
#define GIMP_ICON_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ICON_PICKER, GimpIconPicker))
#define GIMP_ICON_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ICON_PICKER, GimpIconPickerClass))
#define GIMP_IS_ICON_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ICON_PICKER))
#define GIMP_IS_ICON_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ICON_PICKER))
#define GIMP_ICON_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ICON_PICKER, GimpIconPickerClass))


typedef struct _GimpIconPickerClass GimpIconPickerClass;

struct _GimpIconPicker
{
  GtkBox  parent_instance;
};

struct _GimpIconPickerClass
{
  GtkBoxClass   parent_class;
};


GType          gimp_icon_picker_get_type       (void) G_GNUC_CONST;

GtkWidget   * gimp_icon_picker_new             (Gimp           *gimp);

const gchar * gimp_icon_picker_get_icon_name   (GimpIconPicker *picker);
void          gimp_icon_picker_set_icon_name   (GimpIconPicker *picker,
                                                const gchar    *icon_name);

GdkPixbuf   * gimp_icon_picker_get_icon_pixbuf (GimpIconPicker *picker);
void          gimp_icon_picker_set_icon_pixbuf (GimpIconPicker *picker,
                                                GdkPixbuf      *value);
