/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcombotagentry.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_COMBO_TAG_ENTRY_H__
#define __GIMP_COMBO_TAG_ENTRY_H__

#include "gimptagentry.h"

#define GIMP_TYPE_COMBO_TAG_ENTRY            (gimp_combo_tag_entry_get_type ())
#define GIMP_COMBO_TAG_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COMBO_TAG_ENTRY, GimpComboTagEntry))
#define GIMP_COMBO_TAG_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COMBO_TAG_ENTRY, GimpComboTagEntryClass))
#define GIMP_IS_COMBO_TAG_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COMBO_TAG_ENTRY))
#define GIMP_IS_COMBO_TAG_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COMBO_TAG_ENTRY))
#define GIMP_COMBO_TAG_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COMBO_TAG_ENTRY, GimpComboTagEntryClass))


typedef struct _GimpComboTagEntryClass  GimpComboTagEntryClass;

struct _GimpComboTagEntry
{
  GimpTagEntry    parent_instance;

  GtkWidget      *popup;
  PangoAttrList  *normal_item_attr;
  PangoAttrList  *selected_item_attr;
  PangoAttrList  *insensitive_item_attr;
  GdkRGBA         selected_item_color;
};

struct _GimpComboTagEntryClass
{
  GimpTagEntryClass  parent_class;
};


GType       gimp_combo_tag_entry_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_combo_tag_entry_new      (GimpTaggedContainer *container,
                                           GimpTagEntryMode     mode);


#endif  /*  __GIMP_COMBO_TAG_ENTRY_H__  */
