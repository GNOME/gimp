/* page.h
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

// int (*arr)[8];

#ifndef __PAGE_H__
#define __PAGE_H__

#include <glib.h>

G_BEGIN_DECLS

#define TYPE_DATAPAGE (datapage_get_type ())
#define DATAPAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DATAPAGE, datapage))
#define DATAPAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_DATAPAGE, datapageClass))
#define IS_DATAPAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_DATAPAGE))
#define IS_DATAPAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_DATAPAGE))
#define DATAPAGE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_DATAPAGE, datapageClass))

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
  GimpAttributeStructureType  struct_type;             /* type of structure, gexiv2 cannot get the right list type from tag*/
  const gchar                *expand_label_widget;     /* name of the expander label widget */
  const gchar                *struct_combo_widget;     /* name of the combobox widget for this structure */
  const gchar                *struct_liststore_widget; /* name of the liststore of the combobox for this structure */
  const gchar                *add_widget;              /* name of the add structure button for this structure */
  const gchar                *remove_widget;           /* name of the remove structure button for this structure */
};

typedef struct _Datapage Datapage;
typedef struct _DatapageClass DatapageClass;

struct _Datapage {
  GObject parent_instance;
  StructureElement *struct_element;
  gint              structure_element_count;
  MetadataEntry    *metadata_entry;
  gint              metadata_entry_count;
  ComboBoxData     *combobox_data;
  gint              combobox_data_count;
  gint              max_combobox_entries;
  GtkBuilder       *builder;
//  GimpAttributes   *attributes;
};

struct _DatapageClass {
  GObjectClass parent_class;
};

GType datapage_get_type (void) G_GNUC_CONST;


Datapage*                    datapage_new                       (GtkBuilder        *builder);
void                         datapage_set_structure_element     (Datapage          *datapage,
                                                                 StructureElement *struct_elem,
                                                                 gint              structure_element_count);
void                         datapage_set_combobox_data         (Datapage         *datapage,
                                                                 ComboBoxData     *combobox_data,
                                                                 gint              combobox_data_count,
                                                                 gint              max_combobox_entries);
void                         datapage_set_metadata_entry        (Datapage         *datapage,
                                                                 MetadataEntry    *metadata_entry,
                                                                 gint              metadata_entry_count);

void                         datapage_read_from_attributes      (Datapage         *datapage,
                                                                 GimpAttributes  **attributes);
void                         datapage_save_to_attributes        (Datapage         *datapage,
                                                                 GimpAttributes  **attributes);

G_END_DECLS

#endif
