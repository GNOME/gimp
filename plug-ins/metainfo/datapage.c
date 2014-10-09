/* datapage.c - user interface for the metadata editor
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
#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "datapage.h"
#include "metainfo-helper.h"

typedef struct _CallbackData CallbackData;

struct _CallbackData {
  Datapage    *datapage;
  GtkBuilder  *builder;
  gint         combo_array_nr;
};

typedef struct _DatapagePrivate DatapagePrivate;

struct _DatapagePrivate {
  GHashTable  *elements_table;
  GQueue      *current_shown_structure;
  GQueue      *highest_structure;
  GQueue      *combo_signal_handlers;
  GQueue      *expander_texts;
};

enum  {
	DATAPAGE_DUMMY_PROPERTY
};

#define DATAPAGE_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_DATAPAGE, DatapagePrivate))

static Datapage*                    datapage_construct                    (GType                 object_type,
                                                                           GtkBuilder           *builder);
static void                         datapage_set_combo_handler            (Datapage             *datapage,
                                                                           gint                  combo_nr,
                                                                           gulong                handler_id);
static gulong                       datapage_get_combo_handler            (Datapage             *datapage,
                                                                           gint                  combo_nr);
static void                         datapage_set_curr_shown_structure     (Datapage             *datapage,
                                                                           gint                  structure,
                                                                           gint                  value);
static gint                         datapage_get_curr_shown_structure     (Datapage             *datapage,
                                                                           gint                  structure);
static void                         datapage_set_highest_structure        (Datapage             *datapage,
                                                                           gint                  structure,
                                                                           gint                  value);
static gint                         datapage_get_highest_structure        (Datapage             *datapage,
                                                                           gint                  structure);
static void                         datapage_init_ui                      (Datapage             *datapage,
                                                                           GtkBuilder           *builder);
static void                         datapage_entry_activate_cb            (GtkWidget            *widget,
                                                                           gpointer              userdata);
static gboolean                     datapage_entry_focus_out_cb           (GtkWidget            *widget,
                                                                           GdkEvent             *event,
                                                                           gpointer              userdata);
static void                         datapage_entry_combo_changed_callback (GtkWidget            *combo,
                                                                           gpointer              userdata);
static void                         datapage_combobox_changed_callback    (GtkWidget            *combo,
                                                                           gpointer              userdata);
static gboolean                     datapage_store_in_hash_table          (Datapage             *datapage,
                                                                           const gchar          *entry_name,
                                                                           const gchar          *value,
                                                                           gint                  nr);
static void                         datapage_structure_add                (GtkButton            *button,
                                                                           gpointer              userdata);
static void                         datapage_structure_remove             (GtkButton            *button,
                                                                           gpointer              userdata);
static void                         datapage_structure_save               (Datapage             *datapage,
                                                                           gint                  struct_number);
static void                         datapage_set_to_ui                    (Datapage             *datapage,
                                                                           GtkBuilder           *builder,
                                                                           gint                  repaint);
static void                         datapage_init_combobox                (Datapage             *datapage,
                                                                           GtkBuilder           *builder);
static void                         datapage_set_entry_sensitive          (Datapage             *datapage,
                                                                           const gchar          *struct_name,
                                                                           gboolean              sensitive);
static void                         datapage_set_label_text               (Datapage             *datapage,
                                                                           gint                  struct_nr);

static gpointer datapage_parent_class = NULL;


static Datapage*
datapage_construct (GType object_type,
                    GtkBuilder *builder)
{
	Datapage *datapage = NULL;
	datapage = (Datapage*) g_object_new (object_type, NULL);
	datapage->builder = builder;
	return datapage;
}


Datapage*
datapage_new (GtkBuilder *builder)
{
	return datapage_construct (TYPE_DATAPAGE,
	                           builder);
}


static void
datapage_class_init (DatapageClass *klass)
{
	datapage_parent_class = g_type_class_peek_parent (klass);
  g_type_class_add_private (klass, sizeof(DatapagePrivate));
}


static void
datapage_instance_init (Datapage *datapage)
{
  DatapagePrivate *private = DATAPAGE_GET_PRIVATE (datapage);

  private->elements_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  private->current_shown_structure = g_queue_new ();
  private->highest_structure = g_queue_new ();
  private->combo_signal_handlers = g_queue_new ();
  private->expander_texts = g_queue_new ();

}


GType
datapage_get_type (void)
{
  static volatile gsize datapage_type_id__volatile = 0;

  if (g_once_init_enter (&datapage_type_id__volatile))
    {
      static const GTypeInfo g_define_type_info =
        {
          sizeof (DatapageClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) datapage_class_init,
          (GClassFinalizeFunc) NULL,
          NULL,
          sizeof (Datapage),
          0,
          (GInstanceInitFunc) datapage_instance_init,
          NULL
        };

      GType datapage_type_id;
      datapage_type_id = g_type_register_static (G_TYPE_OBJECT,
                                                 "datapage",
                                                 &g_define_type_info,
                                                 0);
      g_once_init_leave (&datapage_type_id__volatile, datapage_type_id);
    }

  return datapage_type_id__volatile;
}

void
datapage_set_structure_element (Datapage         *datapage,
                                StructureElement *struct_elem,
                                gint              structure_element_count)
{
  gint i;

  datapage->struct_element = struct_elem;
  datapage->structure_element_count = structure_element_count;

  for (i = 0; i < structure_element_count; i++)
    {
      datapage_set_curr_shown_structure (datapage, i, 0);
      datapage_set_highest_structure (datapage, i, 0);
    }
}

void
datapage_set_combobox_data (Datapage     *datapage,
                            ComboBoxData *combobox_data,
                            gint          combobox_data_count,
                            gint          max_combobox_entries)
{
//  gint i, j;
//  const gchar  *val_in_combo;
//  ComboBoxData e;
  datapage->combobox_data = combobox_data;
  datapage->combobox_data_count = combobox_data_count;
  datapage->max_combobox_entries = max_combobox_entries;

//  for (i = 0; i < combobox_data_count; i++)
//    {
//      for (j = 0; j < max_combobox_entries; j++)
//        {
//          e = combobox_data[(i * max_combobox_entries) + j];
//          val_in_combo = e.val_in_combo;
//          if ( val_in_combo)
//            g_print ("%s\n", val_in_combo);
//        }
//    }
}

void
datapage_set_metadata_entry (Datapage      *datapage,
                             MetadataEntry *metadata_entry,
                             gint           metadata_entry_count)
{
  datapage->metadata_entry = metadata_entry;
  datapage->metadata_entry_count = metadata_entry_count;
}

static void
datapage_set_combo_handler (Datapage *datapage,
                            gint      combo_nr,
                            gulong    handler_id)
{
  DatapagePrivate *private = DATAPAGE_GET_PRIVATE(datapage);

  g_queue_push_nth (private->combo_signal_handlers,
                    GINT_TO_POINTER (handler_id),
                    combo_nr);
}

static gulong
datapage_get_combo_handler (Datapage *datapage,
                            gint      combo_nr)
{
  DatapagePrivate *private = DATAPAGE_GET_PRIVATE(datapage);

  return GPOINTER_TO_INT (g_queue_peek_nth (private->combo_signal_handlers,
                                    combo_nr));
}

static void
datapage_set_curr_shown_structure (Datapage *datapage,
                                   gint      structure,
                                   gint      value)
{
  DatapagePrivate *private = DATAPAGE_GET_PRIVATE(datapage);

  g_queue_push_nth (private->current_shown_structure,
                    GINT_TO_POINTER (value),
                    structure);
}

static gint
datapage_get_curr_shown_structure (Datapage *datapage,
                                   gint      structure)
{
  DatapagePrivate *private = DATAPAGE_GET_PRIVATE(datapage);

  return GPOINTER_TO_INT (g_queue_peek_nth (private->current_shown_structure,
                                            structure));
}

static void
datapage_set_highest_structure (Datapage *datapage,
                                gint      structure,
                                gint      value)
{
  DatapagePrivate *private = DATAPAGE_GET_PRIVATE(datapage);

  g_queue_push_nth (private->highest_structure,
                    GINT_TO_POINTER (value),
                    structure);
}

static gint
datapage_get_highest_structure (Datapage *datapage,
                                   gint   structure)
{
  DatapagePrivate *private = DATAPAGE_GET_PRIVATE(datapage);

  return GPOINTER_TO_INT (g_queue_peek_nth (private->highest_structure,
                                            structure));
}

static void
datapage_init_ui (Datapage      *datapage,
                  GtkBuilder    *builder)
{
  GObject *obj;
  gint     i;

  DatapagePrivate *private = DATAPAGE_GET_PRIVATE(datapage);

  for (i = 0; i < datapage->metadata_entry_count; i++)
    {
      obj = G_OBJECT (get_widget_from_label (builder, datapage->metadata_entry[i].ui_entry));
      gtk_widget_set_name (GTK_WIDGET (obj), datapage->metadata_entry[i].ui_entry);

      if (datapage->metadata_entry[i].widget_type == WIDGET_TYPE_ENTRY)
        {
          g_signal_connect (GTK_WIDGET (obj), "focus-out-event",
                            G_CALLBACK (datapage_entry_focus_out_cb),
                            datapage);
          g_signal_connect (GTK_ENTRY (obj), "activate",
                            G_CALLBACK (datapage_entry_activate_cb),
                            datapage);
        }
      else if (datapage->metadata_entry[i].widget_type == WIDGET_TYPE_COMBOBOX)
        {
          gint array_nr = datapage->metadata_entry[i].number_of_comboarray;

          if (array_nr >= 0)
            {
              gulong handler_id;
              gint i;
              GtkTreeIter   iter;
              GtkListStore *liststore;
              gint array_length;
              CallbackData *callback_data = g_slice_new (CallbackData);

              callback_data->datapage = datapage;
              callback_data->combo_array_nr = array_nr;
              callback_data->builder = datapage->builder;

              liststore = GTK_LIST_STORE(gtk_combo_box_get_model (GTK_COMBO_BOX (obj)));

              array_length = datapage->max_combobox_entries;

              for (i = 0; i < array_length; i++) /*get info about structure */
                {
                  const gchar  *val_in_combo = datapage->combobox_data[(array_nr * datapage->max_combobox_entries) + i].val_in_combo;

                  if (! val_in_combo)
                    break;

                  gtk_list_store_append(liststore, &iter);

                  gtk_list_store_set (liststore, &iter,
                                      0, val_in_combo,
                                      -1);
                }

              handler_id = g_signal_connect (obj, "changed",
                                             G_CALLBACK (datapage_entry_combo_changed_callback),
                                             callback_data);
              datapage_set_combo_handler (datapage, array_nr, handler_id);
            }
        }
    }

  for (i = 0; i < datapage->structure_element_count; i++) /*get info about structure */
    {
      const gchar *label_text;

      obj = G_OBJECT (get_widget_from_label (builder, datapage->struct_element[i].expand_label_widget));
      label_text = gtk_label_get_text (GTK_LABEL (obj));
      g_queue_push_nth (private->expander_texts, g_strdup (label_text), datapage->struct_element[i].number_of_element);

      obj = G_OBJECT (get_widget_from_label (builder, datapage->struct_element[i].add_widget));
      gtk_widget_set_name (GTK_WIDGET (obj), datapage->struct_element[i].add_widget);
      g_signal_connect (obj, "clicked",
                        G_CALLBACK (datapage_structure_add),
                        datapage);

      obj = G_OBJECT (get_widget_from_label (builder, datapage->struct_element[i].remove_widget));
      gtk_widget_set_name (GTK_WIDGET (obj), datapage->struct_element[i].remove_widget);
      g_signal_connect (obj, "clicked",
                        G_CALLBACK (datapage_structure_remove),
                        datapage);

      obj = G_OBJECT (get_widget_from_label (builder, datapage->struct_element[i].struct_combo_widget));
      gtk_widget_set_name (GTK_WIDGET (obj), datapage->struct_element[i].struct_combo_widget);
      g_signal_connect (obj, "changed",
                        G_CALLBACK (datapage_combobox_changed_callback),
                        datapage);
    }
}

static void
datapage_entry_activate_cb (GtkWidget *widget,
                            gpointer   userdata)
{
  Datapage    *datapage   = (Datapage *) userdata;
  const gchar *value      = gtk_entry_get_text (GTK_ENTRY (widget));
  const gchar *entry_name = gtk_widget_get_name (GTK_WIDGET (widget));

  datapage_store_in_hash_table (datapage, entry_name, value, 0);
}

static gboolean
datapage_entry_focus_out_cb (GtkWidget *widget, GdkEvent *event, gpointer userdata)
{
  Datapage    *datapage   = (Datapage *) userdata;
  const gchar *value      = gtk_entry_get_text (GTK_ENTRY (widget));
  const gchar *entry_name = gtk_widget_get_name (GTK_WIDGET (widget));

  datapage_store_in_hash_table (datapage, entry_name, value, 0);
  set_save_attributes_button_sensitive (TRUE);

  return FALSE;
}

static void
datapage_entry_combo_changed_callback (GtkWidget *combo,
                                       gpointer   userdata)
{
  CallbackData  *callback_data = (CallbackData *) userdata;
  Datapage      *datapage      = callback_data->datapage;
  gint           array_nr      = callback_data->combo_array_nr;
  gint           index         = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
  const gchar   *widget_name;
  const gchar   *value;

  value = datapage->combobox_data[(array_nr * datapage->max_combobox_entries) + index].val_in_tag;
  widget_name = gtk_widget_get_name (GTK_WIDGET (combo));

  datapage_store_in_hash_table (datapage, widget_name, value, 0);
  set_save_attributes_button_sensitive (TRUE);
}

static void
datapage_combobox_changed_callback (GtkWidget *combo, gpointer userdata)
{
  Datapage        *datapage = (Datapage *) userdata;
  GtkComboBox     *combobox;
  gint             nr;
  const gchar     *widget_name;
  gint             sct;
  gint             repaint;

  widget_name = gtk_widget_get_name (GTK_WIDGET (combo));

  for (sct = 0; sct < datapage->structure_element_count; sct ++)
    {
      if (! g_strcmp0 (widget_name, datapage->struct_element[sct].struct_combo_widget))
        {
          datapage_structure_save (datapage, datapage->struct_element[sct].number_of_element);
          combobox = GTK_COMBO_BOX (get_widget_from_label (datapage->builder, datapage->struct_element[sct].struct_combo_widget));
          repaint  = datapage->struct_element[sct].number_of_element;
          break;
       }
    }

  nr = gtk_combo_box_get_active (combobox);
  nr++;
  datapage_set_curr_shown_structure (datapage, repaint, nr);

  datapage_set_to_ui (datapage, datapage->builder, repaint);
}

static gboolean
datapage_store_in_hash_table (Datapage      *datapage,
                              const gchar   *entry_name,
                              const gchar   *value,
                              gint           nr)
{
  gint             i;
  const gchar     *o_tag;
  gint             p;
  gint             number = 0;
  gboolean         success;
  DatapagePrivate *private;

  g_return_val_if_fail (entry_name != NULL, FALSE);

  private = DATAPAGE_GET_PRIVATE (datapage);

  if (nr > 0)
    number = nr;

  success = FALSE;

  for (i = 0; i < datapage->metadata_entry_count; i++)
    {
      if (! g_strcmp0 (entry_name, datapage->metadata_entry[i].ui_entry))
        {
          gchar   *new_tag;
          gchar   *nr_string;

          o_tag = datapage->metadata_entry[i].xmp_tag;

          new_tag =g_strdup (o_tag);

          p = string_index_of (new_tag, "[x]", 0);                           /* is it a structure tag? */

          if (p > -1) /* yes */
            {
              gint sct;

              if (number <= 0)
                {
                  for (sct = 0; sct < datapage->structure_element_count; sct ++)
                    {
                      if (g_strcmp0 (o_tag, datapage->struct_element[sct].struct_tag))
                        {
                          number = datapage_get_curr_shown_structure (datapage, datapage->struct_element[sct].number_of_element);
                          break;
                        }
                    }
                }

              nr_string = g_strdup_printf ("[%d]", number);
              new_tag = string_replace_str (o_tag, "[x]", nr_string);

              g_free (nr_string);
              if (number <=0 )
                {
                  g_free (new_tag);
                  return FALSE;
                }
            }

          if (value && g_strcmp0 (value, ""))
            {
              if (g_hash_table_insert (private->elements_table, (gpointer) g_strdup (new_tag), (gpointer) g_strdup (value)))
                success = TRUE;
            }
          else
            {
              if (g_hash_table_remove (private->elements_table, (gpointer) new_tag))
                success = TRUE;
            }

          g_free (new_tag);
          break;
        }
    }
  return success;
}

static void
datapage_structure_add (GtkButton *button,
                        gpointer   userdata)
{
  gint             number_to_add;
  GtkComboBox     *combo;
  Datapage        *datapage = (Datapage *) userdata;
  GtkListStore    *liststore;
  GtkTreeIter      iter;
  const gchar     *widget_name;
  gchar           *line;
  gint             sct;
  gint             repaint;

  widget_name = gtk_widget_get_name (GTK_WIDGET (button));

  for (sct = 0; sct < datapage->structure_element_count; sct ++)
    {
      if (! g_strcmp0 (widget_name, datapage->struct_element[sct].add_widget))
        {
          datapage_structure_save (datapage, datapage->struct_element[sct].number_of_element);

          number_to_add = datapage_get_highest_structure (datapage, datapage->struct_element[sct].number_of_element);
          datapage_set_highest_structure (datapage, datapage->struct_element[sct].number_of_element, ++number_to_add);
          liststore = GTK_LIST_STORE (get_widget_from_label (datapage->builder, datapage->struct_element[sct].struct_liststore_widget));
          combo = GTK_COMBO_BOX (get_widget_from_label (datapage->builder, datapage->struct_element[sct].struct_combo_widget));
          line = g_strdup_printf ("%s [%d]", datapage->struct_element[sct].identifier, number_to_add);
          datapage_set_curr_shown_structure (datapage, datapage->struct_element[sct].number_of_element, number_to_add);

          repaint = datapage->struct_element[sct].number_of_element;

          g_signal_handlers_block_by_func (G_OBJECT (combo), G_CALLBACK (datapage_combobox_changed_callback), datapage);

          gtk_list_store_append(liststore, &iter);

          gtk_list_store_set (liststore, &iter,
                              0, line,
                              -1);

          gtk_combo_box_set_active (combo, number_to_add-1);

          if (number_to_add == 1)
            datapage_set_entry_sensitive (datapage, datapage->struct_element[sct].struct_tag, TRUE);

          g_signal_handlers_unblock_by_func(G_OBJECT (combo), G_CALLBACK (datapage_combobox_changed_callback), datapage);

          g_free (line);

          datapage_set_label_text (datapage, sct);

          break;
       }
    }

  datapage_set_to_ui (datapage, datapage->builder, repaint);
}

static void
datapage_structure_remove (GtkButton *button,
                           gpointer   userdata)
{
  Datapage        *datapage = (Datapage *) userdata;
  DatapagePrivate *private;
  GtkListStore    *liststore;
  GHashTableIter   iter_remove;
  gboolean         found;
  gchar           *nr_string;
  gchar           *nr_string_new;
  gpointer         key, value;
  GSList          *delete_key_list   = NULL;
  GSList          *list;
  gchar           *tag_prefix;
  gchar           *new_key;
  gint             number_to_delete;
  GtkComboBox     *combo;
  GtkTreeIter      combo_iter;
  const gchar     *widget_name;
  gint             sct;
  gint             repaint;
  gint             combo_to_del;

  GHashTable           *reorder_table;

  private = DATAPAGE_GET_PRIVATE (datapage);

  reorder_table = g_hash_table_new_full  (g_str_hash, g_str_equal, g_free, g_free);

  widget_name = gtk_widget_get_name (GTK_WIDGET (button));

  for (sct = 0; sct < datapage->structure_element_count; sct ++)
    {
      if (! g_strcmp0 (widget_name, datapage->struct_element[sct].remove_widget))
        {
          gint new_highest;

          number_to_delete = datapage_get_curr_shown_structure (datapage, sct);

          tag_prefix = g_strdup (datapage->struct_element[sct].struct_tag);
          liststore = GTK_LIST_STORE (get_widget_from_label (datapage->builder, datapage->struct_element[sct].struct_liststore_widget));
          combo = GTK_COMBO_BOX (get_widget_from_label (datapage->builder, datapage->struct_element[sct].struct_combo_widget));
          combo_to_del = datapage_get_highest_structure (datapage, sct) - 1;

          if (number_to_delete == datapage_get_highest_structure (datapage, datapage->struct_element[sct].number_of_element))
            datapage_set_curr_shown_structure (datapage, datapage->struct_element[sct].number_of_element, number_to_delete - 1);
          else
            datapage_set_curr_shown_structure (datapage, datapage->struct_element[sct].number_of_element, number_to_delete);

          new_highest = datapage_get_highest_structure (datapage, sct);
          datapage_set_highest_structure (datapage, sct, --new_highest);

          repaint = datapage->struct_element[sct].number_of_element;

          datapage_set_label_text (datapage, sct);

          break;
       }
    }

  nr_string = g_strdup_printf ("%s[%d]", tag_prefix, number_to_delete);

  /* remove entries */
  {

    g_hash_table_iter_init (&iter_remove, private->elements_table);
    while (g_hash_table_iter_next (&iter_remove, &key, &value))
      {
        gchar *tag = (gchar *) key;
        if (g_str_has_prefix (tag, nr_string))
          {
            delete_key_list = g_slist_prepend (delete_key_list, g_strdup (tag));
          }
      }

    for (list = delete_key_list; list; list = list->next)
      {
        gchar *key = (gchar *) list->data;
        g_hash_table_remove (private->elements_table, key);
      }

    g_slist_free_full (delete_key_list, g_free);

  }
  /* reorder entries */

  found = TRUE;

  {
    while (found) /* first: element table */
      {
        GHashTableIter   iter_reorder;

        delete_key_list = NULL;
        found           = FALSE;

        nr_string_new = g_strdup_printf ("%s[%d]", tag_prefix, number_to_delete + 1);

        g_hash_table_iter_init (&iter_reorder, private->elements_table);
        while (g_hash_table_iter_next (&iter_reorder, &key, &value))
          {
            gchar *tag       = (gchar *) key;
            gchar *tag_value = (gchar *) value;

            if (g_str_has_prefix (tag, nr_string_new))
              {
                found = TRUE;
                new_key = string_replace_str (tag, nr_string_new, nr_string);
                g_hash_table_insert (reorder_table, g_strdup (new_key), g_strdup (tag_value));
                delete_key_list = g_slist_prepend (delete_key_list, g_strdup (tag));
              }
          }

        for (list = delete_key_list; list; list = list->next)
          {
            gchar *key = (gchar *) list->data;
            g_hash_table_remove (private->elements_table, key);
          }
        g_slist_free_full (delete_key_list, g_free);

        g_hash_table_iter_init (&iter_reorder, reorder_table);
        while (g_hash_table_iter_next (&iter_reorder, &key, &value))
          {
            gchar *tag       = (gchar *) key;
            gchar *tag_value = (gchar *) value;

            g_hash_table_insert (private->elements_table, g_strdup (tag), g_strdup (tag_value));
          }

        g_hash_table_remove_all (reorder_table);

        nr_string = g_strdup (nr_string_new);
        number_to_delete ++;
      }

  }

  g_free (tag_prefix);
  g_free (nr_string);
  g_free (nr_string_new);
  g_free (new_key);

  g_hash_table_unref (reorder_table);

  g_signal_handlers_block_by_func (G_OBJECT (combo), G_CALLBACK (datapage_combobox_changed_callback), (gpointer) datapage);

  gtk_combo_box_set_active (combo, combo_to_del);
  if (gtk_combo_box_get_active_iter (combo, &combo_iter))
    gtk_list_store_remove (liststore, &combo_iter);

  if (datapage_get_curr_shown_structure (datapage, repaint) == 0)
    datapage_set_entry_sensitive (datapage, datapage->struct_element[sct].struct_tag, FALSE);
  else
    gtk_combo_box_set_active (combo, datapage_get_curr_shown_structure (datapage, repaint) - 1);

  g_signal_handlers_unblock_by_func(G_OBJECT (combo), G_CALLBACK (datapage_combobox_changed_callback), (gpointer) datapage);

  set_save_attributes_button_sensitive (TRUE);

  datapage_set_to_ui (datapage, datapage->builder, repaint);

}

static void
datapage_structure_save (Datapage *datapage,
                         gint      struct_number)
{
  DatapagePrivate *private;
  gint         i;
  const gchar *prefix = datapage->struct_element[struct_number].struct_tag;

  private = DATAPAGE_GET_PRIVATE (datapage);

  for (i = 0; i < datapage->metadata_entry_count; i++)
    {
      GObject       *obj;
      gchar         *tag      = NULL;
      const gchar   *o_tag;

      if (datapage_get_curr_shown_structure (datapage, struct_number) > 0)
        {
          o_tag = datapage->metadata_entry[i].xmp_tag;
          if (g_str_has_prefix (o_tag, prefix))
            {
              gchar *nr_string;

              nr_string = g_strdup_printf ("[%d]", datapage_get_curr_shown_structure (datapage, struct_number));
              tag = string_replace_str (o_tag, "[x]", nr_string);

              obj   = get_widget_from_label (datapage->builder, datapage->metadata_entry[i].ui_entry);

              switch (datapage->metadata_entry[i].widget_type)
              {
                case WIDGET_TYPE_ENTRY:
                  {
                    gchar *value;
                    value = g_strdup (gtk_entry_get_text (GTK_ENTRY (obj)));
                    if (value && g_strcmp0 (value, ""))
                      g_hash_table_insert (private->elements_table, (gpointer) g_strdup (tag), (gpointer) g_strdup (value));
                    else
                      g_hash_table_remove (private->elements_table, (gpointer) tag);
                  }
                  break;
                case WIDGET_TYPE_COMBOBOX:
                  break;
                default:
                  break;
              }

              g_free (nr_string);
              g_free (tag);
            }
        }
    }
}

static void
datapage_set_to_ui (Datapage    *datapage,
                    GtkBuilder  *builder,
                    gint         repaint)
{
  gint             i;
  gint             sct;
  const gchar     *prefix;
  DatapagePrivate *private;

  private = DATAPAGE_GET_PRIVATE (datapage);

  if (repaint != -1)
    prefix = datapage->struct_element[repaint].struct_tag;
  else
    prefix = NULL;

  for (i = 0; i < datapage->metadata_entry_count; i++)
    {
      GObject       *obj;
      const gchar   *o_tag;
      gchar         *value = NULL;
      gchar         *new_tag;
      gint           p;

      o_tag = datapage->metadata_entry[i].xmp_tag;

      obj = get_widget_from_label (builder, datapage->metadata_entry[i].ui_entry);

      if (prefix)
        {
          if (! g_str_has_prefix (o_tag, prefix))
            continue;
        }

      p = string_index_of (o_tag, "[x]", 0);     /* is it a structure tag? */

      if (p > -1) /* yes! */
        {
          gchar         *nr_string;

          if (repaint != -1)
            {
              nr_string = g_strdup_printf ("[%d]", datapage_get_curr_shown_structure (datapage, repaint));
            }
          else
            {
              for (sct = 0; sct < datapage->structure_element_count; sct ++)
                {
                  if (g_str_has_prefix (o_tag, datapage->struct_element[sct].struct_tag))
                    {
                      nr_string = g_strdup_printf ("[%d]", datapage_get_curr_shown_structure (datapage, sct));
                      break;
                    }
                }
            }

          new_tag = string_replace_str (o_tag, "[x]", nr_string);

          g_free (nr_string);
        }
      else
        {
          new_tag = g_strdup (o_tag);
        }

      value = (gchar *) g_hash_table_lookup (private->elements_table, new_tag);

      switch (datapage->metadata_entry[i].widget_type)
      {
        case WIDGET_TYPE_ENTRY:
          {
           gtk_entry_set_text (GTK_ENTRY (obj), "");
          }
          break;
        default:
          break;
      }

      if (value)
        {
          switch (datapage->metadata_entry[i].widget_type)
          {
            case WIDGET_TYPE_ENTRY:
              {
               gtk_entry_set_text (GTK_ENTRY (obj), value);
              }
              break;
            case WIDGET_TYPE_COMBOBOX:
              {
                gint combobox_data_counter;
                gint array_length;
                gint number_in_comboarray = datapage->metadata_entry[i].number_of_comboarray;

                array_length = datapage->max_combobox_entries;

                for (combobox_data_counter = 0; combobox_data_counter < array_length; combobox_data_counter++)
                  {
                    const gchar  *val_in_tag = datapage->combobox_data[(number_in_comboarray * datapage->max_combobox_entries) + combobox_data_counter].val_in_tag;
                    if (! val_in_tag)
                      break;

                    if (! g_strcmp0 (value, val_in_tag))
                      {
                        gulong handler_id = 0;

                        handler_id = datapage_get_combo_handler (datapage, number_in_comboarray);

                        g_signal_handler_block (obj,
                                                handler_id);

                        gtk_combo_box_set_active (GTK_COMBO_BOX (obj), combobox_data_counter);

                        g_signal_handler_unblock (obj,
                                                  handler_id);
                        break;
                      }
                  }
              }
              break;
            default:
              break;
          }
        }
      g_free (new_tag);
    }
}

static void
datapage_init_combobox (Datapage   *datapage,
                        GtkBuilder *builder)
{
  GtkListStore    *liststore;
  GtkTreeIter      iter;
  GtkComboBox     *combo;
  gint             sct;

  for (sct = 0; sct < datapage->structure_element_count; sct ++)
    {
      gchar *line;
      gint   i;
      gint   high;
      gint   active;

      liststore = GTK_LIST_STORE (get_widget_from_label (builder, datapage->struct_element[sct].struct_liststore_widget));
      combo = GTK_COMBO_BOX (get_widget_from_label (builder, datapage->struct_element[sct].struct_combo_widget));

      high = datapage_get_highest_structure (datapage, datapage->struct_element[sct].number_of_element);
      active = datapage_get_curr_shown_structure (datapage, datapage->struct_element[sct].number_of_element);

      g_signal_handlers_block_by_func (G_OBJECT (combo), G_CALLBACK (datapage_combobox_changed_callback), datapage);

      for (i = 0; i < high; i++)
        {
          line = g_strdup_printf ("%s [%d]",datapage->struct_element[sct].identifier, i + 1);

          gtk_list_store_append(liststore, &iter);

          gtk_list_store_set (liststore, &iter,
                              0, line,
                              -1);
          g_free (line);
        }

      gtk_combo_box_set_active (combo, active-1);

      if (active >= 1)
        datapage_set_entry_sensitive (datapage, datapage->struct_element[sct].struct_tag, TRUE);
      else
        datapage_set_entry_sensitive (datapage, datapage->struct_element[sct].struct_tag, FALSE);

      g_signal_handlers_unblock_by_func(G_OBJECT (combo), G_CALLBACK (datapage_combobox_changed_callback), datapage);

      datapage_set_label_text (datapage, sct);
    }
}

static void
datapage_set_entry_sensitive (Datapage    *datapage,
                              const gchar *struct_name,
                              gboolean     sensitive)
{
  gint i;

  for (i = 0; i < datapage->metadata_entry_count; i++)
    {
      const gchar   *tag;
      GObject       *obj;

      tag = datapage->metadata_entry[i].xmp_tag;
      if (g_str_has_prefix (tag, struct_name))
        {
          obj = G_OBJECT (get_widget_from_label (datapage->builder, datapage->metadata_entry[i].ui_entry));
          gtk_widget_set_sensitive (GTK_WIDGET (obj), sensitive);
        }
    }

}

static void
datapage_set_label_text (Datapage        *datapage,
                         gint             struct_nr)
{
  DatapagePrivate *private;
  GObject         *obj;
  gchar           *label_text;
  gint             high;

  private = DATAPAGE_GET_PRIVATE (datapage);

  obj = get_widget_from_label (datapage->builder, datapage->struct_element[struct_nr].expand_label_widget);

  high = datapage_get_highest_structure (datapage, datapage->struct_element[struct_nr].number_of_element);

  label_text = g_strdup_printf ("%s (%d)",
                                (gchar *) g_queue_peek_nth (private->expander_texts,
                                                            datapage->struct_element[struct_nr].number_of_element),
                                high);

  gtk_label_set_text (GTK_LABEL (obj), label_text);

  g_free (label_text);
}

void
datapage_read_from_attributes (Datapage        *datapage,
                               GimpAttributes **attributes)
{
  gint i;
  gint sct;
  DatapagePrivate *private;

  g_return_if_fail (*attributes != NULL);

  private = DATAPAGE_GET_PRIVATE (datapage);

  datapage_init_ui (datapage, datapage->builder);

  for (i = 0; i < datapage->metadata_entry_count; i++)
    {
      GimpAttribute *attribute = NULL;
      const gchar   *o_tag      = NULL;
      gchar         *tag       = NULL;
      gint           p;
      gint           counter;

      o_tag = datapage->metadata_entry[i].xmp_tag;

      tag =g_strdup (o_tag);

//      if (g_str_has_prefix (tag, "Xmp.plus.ImageCreator"))
//          g_print ("found\n");
//
      p = string_index_of (tag, "[x]", 0);  /* is it a structure tag? */

      if (p > -1) /* yes! */
        {
          gint    j;
          gchar  *struct_string = NULL;

          /* check last number */

          struct_string = string_substring (tag, 0, p); /* get structure tag */

          for (j = 0; j < datapage->structure_element_count; j++) /*get info about structure */
            {
              if (! g_strcmp0 (struct_string, datapage->struct_element[j].struct_tag))  /* is a structure */
                {
                  GimpAttribute *struct_attribute = NULL;
                  gint num = datapage_get_highest_structure (datapage, datapage->struct_element[j].number_of_element);

                  counter = 0;  /*start the loop */

                  while (struct_attribute ||
                         counter == 0     ||  /* at least once, because counter IS 0 */
                         counter < num)       /* at least until the highest structure */
                    {
                      gchar           *nr_string = NULL;
                      gchar           *new_tag = NULL;
                      gchar           *value;

                      counter ++;
                      nr_string = g_strdup_printf ("[%d]", counter);
                      new_tag = string_replace_str (tag, "[x]", nr_string);

                      struct_attribute = gimp_attributes_get_attribute (*attributes, new_tag);

                      if (struct_attribute)
                        {
                          gint sct;
                          value = gimp_attribute_get_string (struct_attribute);

                          for (sct = 0; sct < datapage->structure_element_count; sct ++)
                            {
                              if (g_str_has_prefix (new_tag, datapage->struct_element[sct].struct_tag))
                                {
                                  datapage_set_highest_structure (datapage, datapage->struct_element[sct].number_of_element, counter);
                                  break;
                                }
                            }
                          g_hash_table_insert (private->elements_table,
                                               (gpointer) g_strdup (new_tag),
                                               (gpointer) g_strdup (value));

                          gimp_attributes_remove_attribute (*attributes, new_tag);
                          g_free (value);
                        }
                      g_free (nr_string);
                      g_free (new_tag);
                    }
                  break;
                }
            }
        }
      else
        {
          attribute = gimp_attributes_get_attribute (*attributes, tag);
          if (attribute)
            {
              gchar *value = gimp_attribute_get_string (attribute);
              g_hash_table_insert (private->elements_table,
                                   (gpointer) g_strdup (datapage->metadata_entry[i].xmp_tag),
                                   (gpointer) g_strdup (value));

              gimp_attributes_remove_attribute (*attributes, datapage->metadata_entry[i].xmp_tag);
              g_free (value);
            }
        }
      g_free (tag);
    }

  for (sct = 0; sct < datapage->structure_element_count; sct ++)
    {
      if (datapage_get_highest_structure (datapage, sct) > 0)
        datapage_set_curr_shown_structure (datapage, sct, 1);
      else
        datapage_set_curr_shown_structure (datapage, sct, 0);
    }

  datapage_init_combobox (datapage, datapage->builder);

  datapage_set_to_ui (datapage, datapage->builder,
                              -1); /* all */
}

void
datapage_save_to_attributes (Datapage *datapage, GimpAttributes **attributes)
{
  GHashTableIter     iter;
  gpointer           key, value;
  DatapagePrivate   *private;

  private = DATAPAGE_GET_PRIVATE (datapage);

  g_hash_table_iter_init (&iter, private->elements_table);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      GimpAttribute                   *attribute     = NULL;
      gchar                           *tag = (gchar *) key;
      gchar                           *value         = NULL;
      gint                             p;
      gint                             sct;
      gint i;

      value = (gchar *) g_hash_table_lookup (private->elements_table, (gpointer) tag);
      attribute = gimp_attribute_new_string (tag, value, TYPE_ASCII);

      if (attribute)
        {
          p = string_index_of (tag, "[", 0);                           /* is it a structure tag? */

          if (p > -1) /* yes! */
            {

              for (sct = 0; sct < datapage->structure_element_count; sct ++)
                {
                  if (g_str_has_prefix (tag, datapage->struct_element[sct].struct_tag))
                    {
                      gimp_attribute_set_structure_type (attribute, datapage->struct_element[sct].struct_type);
                      break;
                    }
                }
            }

          for (i = 0; i < datapage->metadata_entry_count; i++)
            {
              gchar *t_tag;
              gint   p1;
              gint   p2;

              p1 = string_index_of (tag, "[", 0);                           /* is it a structure tag? */
              if (p > -1) /* yes! */
                {
                  gchar *t1;
                  gchar *t2;
                  gint   l;

                  p2 = string_index_of (tag, "]", p1);

                  l = strlen (tag);

                  t1 = string_substring (tag, 0, p);
                  t2 = string_substring (tag, p2 + 1, l - (p2 + 1));

                  t_tag = g_strdup_printf ("%s[x]%s", t1, t2);
                }
              else
                {
                  t_tag = g_strdup (tag);
                }


              if (! g_strcmp0 (t_tag, datapage->metadata_entry[i].xmp_tag))
                {
                  if (datapage->metadata_entry[i].number_of_comboarray > -1)
                    {
                      gint combobox_data_counter;
                      gint array_length;
                      gint number_in_comboarray = datapage->metadata_entry[i].number_of_comboarray;

                      array_length = datapage->max_combobox_entries;

                      for (combobox_data_counter = 0; combobox_data_counter < array_length; combobox_data_counter++)
                        {
                          const gchar  *val_in_tag = datapage->combobox_data[number_in_comboarray * datapage->max_combobox_entries + combobox_data_counter].val_in_tag;
                          const gchar  *interpreted_val;

                          if (! val_in_tag)
                            break;

                          if (! g_strcmp0 (value, val_in_tag))
                            {
                              interpreted_val =  datapage->combobox_data[number_in_comboarray * datapage->max_combobox_entries + combobox_data_counter].val_in_combo;
                              gimp_attribute_set_interpreted_string (attribute, interpreted_val);
                              break;
                            }
                        }
                    }
                  break;
                }
            }
          gimp_attributes_add_attribute (*attributes, attribute);
        }
    }
}
