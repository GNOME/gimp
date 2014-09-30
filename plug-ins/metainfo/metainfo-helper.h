/* metainfo-helper.h
 *
 * Copyright (C) 2014, Hartmut Kuhse <hatti@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __METAINFO_HELPER_H__
#define __METAINFO_HELPER_H__

#include <glib-object.h>

#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_regex_unref0(var) ((var == NULL) ? NULL : (var = (g_regex_unref (var), NULL)))
#define _g_error_free0(var) ((var == NULL) ? NULL : (var = (g_error_free (var), NULL)))

G_BEGIN_DECLS

typedef enum
{
  WIDGET_TYPE_ENTRY,
  WIDGET_TYPE_COMBOBOX
} WidgetType;

typedef enum
{
  LIST_TYPE_NONE,
  LIST_TYPE_SEQ,
  LIST_TYPE_BAG,
  LIST_TYPE_LANGALT
} ListType;

typedef struct _ComboBoxData      ComboBoxData;

struct _ComboBoxData
{
  const gchar  *val_in_combo;                          /* translateable - value shown in the combobox */
  const gchar  *val_in_tag;                            /* value saved as tag value */
};


typedef struct _MetadataEntry     MetadataEntry;

struct _MetadataEntry
{
        gchar      *label;                             /* translateable - label of the widget */
  const gchar      *ui_label;                          /* name of the label widget in GtkBuilder ui file */
  const gchar      *ui_entry;                          /* name of the entry widget in GtkBuilder ui file */
  const gchar      *xmp_tag;                           /* xmp tag, saved in GimpAttribute.name */
  WidgetType        widget_type;                       /* type of entry widget in GtkBuilder ui : GtkEntry or GtkComboBox */
        gint        number_of_comboarray;              /*number of entry in ComboBoxData - array. Only valid for combobox entries */
};

typedef struct _StructureElement StructureElement;

struct _StructureElement
{
  gint                        number_of_element;       /* simply the number, corresponding to STRUCTURES_ON_PAGE */
  const gchar                *identifier;              /* translateble - identifier for combobox entries */
  const gchar                *struct_tag;              /* structure tag without array number */
  GimpAttributeStructureType  struct_type;               /* type of structure, gexiv2 cannot get the right list type from tag*/
  const gchar                *struct_combo_widget;     /* name of the combobox widget for this structure */
  const gchar                *struct_liststore_widget; /* name of the liststore of the combobox for this structure */
  const gchar                *add_widget;              /* name of the add structure button for this structure */
  const gchar                *remove_widget;           /* name of the remove structure button for this structure */
};


gchar*                  string_replace_str                        (const gchar         *original,
                                                                   const gchar         *old_pattern,
                                                                   const gchar         *replacement);
gint                    string_index_of                           (const gchar         *haystack,
                                                                   const gchar         *needle,
                                                                   gint                 start_index);
glong                   string_strnlen                            (gchar               *str,
                                                                   glong                maxlen);
gchar*                  string_substring                          (const gchar         *attribute,
                                                                   glong                offset,
                                                                   glong                len);
GObject*                get_widget_from_label                     (GtkBuilder          *builder,
                                                                   const gchar         *label);
void                    set_save_attributes_button                (GtkButton           *button);
void                    set_save_attributes_button_sensitive      (gboolean             sensitive);




G_END_DECLS

#endif /* __METAINFO_HELPER_H__ */
