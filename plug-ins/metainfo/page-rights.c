/* page-administration.c
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "metainfo-helper.h"
#include "page-rights.h"


static void                page_rights_init_ui                       (GtkBuilder        *builder);
static void                page_rights_set_entry_sensitive           (GtkBuilder        *builder,
                                                                      const gchar       *struct_name,
                                                                      gboolean           sensitive);
static void                page_rights_init_combobox                 (GtkBuilder        *builder);
static void                page_rights_set_to_ui                     (GtkBuilder        *builder,
                                                                      gint               repaint);
static void                page_rights_entry_activate_cb             (GtkWidget         *widget,
                                                                      gpointer           userdata);
static gboolean            page_rights_entry_focus_out_cb            (GtkWidget         *widget,
                                                                      GdkEvent          *event,
                                                                      gpointer           userdata);
static gboolean            page_rights_store_in_hash_table           (const gchar       *entry_name,
                                                                      const gchar       *value,
                                                                      gint               nr);
static void                page_rights_structure_add                 (GtkButton         *button,
                                                                      gpointer           userdata);
static void                page_rights_structure_remove              (GtkButton         *button,
                                                                      gpointer           userdata);
static void                page_rights_entry_combo_changed_callback  (GtkWidget         *combo,
                                                                      gpointer           userdata);
static void                page_rights_combobox_changed_callback     (GtkWidget         *combo,
                                                                      gpointer           userdata);
static void                page_rights_structure_save                (GtkBuilder        *builder,
                                                                      gint               struct_number);


#define STRUCTURES_ON_PAGE 3
#define COMBOBOX_WITH_DATA 1


static GHashTable           *elements_table;
static gint                  curr_shown_structure[STRUCTURES_ON_PAGE];
static gint                  highest_structure[STRUCTURES_ON_PAGE];

static StructureElement struct_element [] =
    {
        {0,
            N_("Image creator"),
            "Xmp.plus.ImageCreator",
            STRUCTURE_TYPE_SEQ,
            "imagecreator-combo",
            "imagecreator-liststore",
            "imagecreator-button-plus",
            "imagecreator-button-minus"},

        {1,
            N_("Copyright Owner"),
            "Xmp.plus.CopyrightOwner",
            STRUCTURE_TYPE_SEQ,
            "copyrightowner-combo",
            "copyrightowner-liststore",
            "copyrightowner-button-plus",
            "copyrightowner-button-minus"},

        {2,
            N_("Licensor"),
            "Xmp.plus.Licensor",
            STRUCTURE_TYPE_BAG,
            "licensor-combo",
            "licensor-liststore",
            "licensor-button-plus",
            "licensor-button-minus"}
    };


static ComboBoxData combobox_data[][256] =
{
    {
        {N_(" "),                                         ""},
        {N_("None"),                                      "http://ns.useplus.org/ldf/vocab/PR-NON"},
        {N_("Not applicable"),                            "http://ns.useplus.org/ldf/vocab/PR-NAP"},
        {N_("Unlimited property release"),                "http://ns.useplus.org/ldf/vocab/PR-UPR"},
        {N_("Limited or Incomplete Property Releases"),   "http://ns.useplus.org/ldf/vocab/PR-LPR"}
    },
};

static MetadataEntry rights_entries[] =
    {
        {N_("Image creator name"),
            "imagecreator-name-label",
            "imagecreator-name-entry",
            "Xmp.plus.ImageCreator[x]/plus:ImageCreatorName",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Image creator ID"),
            "imagecreator-id-label",
            "imagecreator-id-entry",
            "Xmp.plus.ImageCreator[x]/plus:ImageCreatorID",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Copyright owner name"),
            "copyrightowner-name-label",
            "copyrightowner-name-entry",
            "Xmp.plus.CopyrightOwner[x]/plus:CopyrightOwnerName",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Copyright owner ID"),
            "copyrightowner-id-label",
            "copyrightowner-id-entry",
            "Xmp.plus.CopyrightOwner[x]/plus:CopyrightOwnerID",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Name"),
            "licensor-name-label",
            "licensor-name-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorName",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("ID"),
            "licensor-id-label",
            "licensor-id-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorID",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Address"),
            "licensor-streetaddress-label",
            "licensor-streetaddress-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorStreetAddress",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Extended Address"),
            "licensor-extendedaddress-label",
            "licensor-extendedaddress-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorExtendedAddress",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("City"),
            "licensor-city-label",
            "licensor-city-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorCity",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Region"),
            "licensor-region-label",
            "licensor-region-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorRegion",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Postal code"),
            "licensor-postalcode-label",
            "licensor-postalcode-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorPostalCode",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Country"),
            "licensor-country-label",
            "licensor-country-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorCountry",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Telephone 1"),
            "licensor-telephone1-label",
            "licensor-telephone1-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorTelephone1",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Telephone 2"),
            "licensor-telephone2-label",
            "licensor-telephone2-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorTelephone2",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("E-Mail"),
            "licensor-email-label",
            "licensor-email-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorEmail",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("URL"),
            "licensor-url-label",
            "licensor-url-entry",
            "Xmp.plus.Licensor[x]/plus:LicensorURL",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Release ID"),
            "release-id-label",
            "release-id-entry",
            "Xmp.plus.PropertyReleaseID",
            WIDGET_TYPE_ENTRY,
            -1},

        {N_("Release status"),
            "release-status-label",
            "release-status-combobox",
            "Xmp.plus.PropertyReleaseStatus",
            WIDGET_TYPE_COMBOBOX,
            0}
    };


static void
page_rights_init_ui (GtkBuilder *builder)
{
  GObject *obj;
  gint     i;

  elements_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  for (i = 0; i < G_N_ELEMENTS(rights_entries); i++)
    {
      obj = G_OBJECT (get_widget_from_label (builder, rights_entries[i].ui_label));

      gtk_label_set_text (GTK_LABEL (obj), rights_entries[i].label);

      obj = G_OBJECT (get_widget_from_label (builder, rights_entries[i].ui_entry));
      gtk_widget_set_name (GTK_WIDGET (obj), rights_entries[i].ui_entry);

      if (rights_entries[i].widget_type == WIDGET_TYPE_ENTRY)
        {
          g_signal_connect (GTK_WIDGET (obj), "focus-out-event",
                            G_CALLBACK (page_rights_entry_focus_out_cb),
                            (gpointer) rights_entries[i].ui_entry);
          g_signal_connect (GTK_ENTRY (obj), "activate",
                            G_CALLBACK (page_rights_entry_activate_cb),
                            (gpointer) rights_entries[i].ui_entry);
        }
      else if (rights_entries[i].widget_type == WIDGET_TYPE_COMBOBOX)
        {
          gint array_nr = rights_entries[i].number_of_comboarray;

          if (array_nr >= 0)
            {
              gint i;
              GtkTreeIter   iter;
              GtkListStore *liststore;
              gint array_length;

              liststore = GTK_LIST_STORE(gtk_combo_box_get_model (GTK_COMBO_BOX (obj)));

              array_length = G_N_ELEMENTS(combobox_data[array_nr]);

              for (i = 0; i < array_length; i++) /*get info about structure */
                {
                  const gchar  *val_in_combo = combobox_data[array_nr][i].val_in_combo;

                  if (! val_in_combo)
                    break;

                  gtk_list_store_append(liststore, &iter);

                  gtk_list_store_set (liststore, &iter,
                                      0, val_in_combo,
                                      -1);
                }

              g_signal_connect (obj, "changed",
                                G_CALLBACK (page_rights_entry_combo_changed_callback),
                                GINT_TO_POINTER (array_nr));
            }
        }
    }

  for (i = 0; i < G_N_ELEMENTS(struct_element); i++) /*get info about structure */
    {
      obj = G_OBJECT (get_widget_from_label (builder, struct_element[i].add_widget));
      gtk_widget_set_name (GTK_WIDGET (obj), struct_element[i].add_widget);
      g_signal_connect (obj, "clicked",
                        G_CALLBACK (page_rights_structure_add),
                        builder);

      obj = G_OBJECT (get_widget_from_label (builder, struct_element[i].remove_widget));
      gtk_widget_set_name (GTK_WIDGET (obj), struct_element[i].remove_widget);
      g_signal_connect (obj, "clicked",
                        G_CALLBACK (page_rights_structure_remove),
                        builder);

      obj = G_OBJECT (get_widget_from_label (builder, struct_element[i].struct_combo_widget));
      gtk_widget_set_name (GTK_WIDGET (obj), struct_element[i].struct_combo_widget);
      g_signal_connect (obj, "changed",
                        G_CALLBACK (page_rights_combobox_changed_callback),
                        builder);
    }
}

static void
page_rights_set_entry_sensitive (GtkBuilder  *builder,
                                      const gchar *struct_name,
                                      gboolean     sensitive)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS(rights_entries); i++)
    {
      const gchar   *tag;
      GObject       *obj;

      tag = rights_entries[i].xmp_tag;
      if (g_str_has_prefix (tag, struct_name))
        {
          obj = G_OBJECT (get_widget_from_label (builder, rights_entries[i].ui_entry));
          gtk_widget_set_sensitive (GTK_WIDGET (obj), sensitive);
        }
    }

}

void
page_rights_read_from_attributes (GimpAttributes *attributes,
                                       GtkBuilder     *builder)
{
  gint i;
  gint sct;

  g_return_if_fail (attributes != NULL);

  page_rights_init_ui (builder);

  for (i = 0; i < G_N_ELEMENTS(rights_entries); i++)
    {
      GimpAttribute *attribute = NULL;
      const gchar   *o_tag      = NULL;
      gchar         *tag       = NULL;
      gint           p;
      gint           counter;

      o_tag = rights_entries[i].xmp_tag;

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

          for (j = 0; j < G_N_ELEMENTS(struct_element); j++) /*get info about structure */
            {
              if (! g_strcmp0 (struct_string, struct_element[j].struct_tag))  /* is a structure */
                {
                  GimpAttribute *struct_attribute = NULL;
                  gint num = highest_structure[struct_element[j].number_of_element];

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

                      struct_attribute = gimp_attributes_get_attribute (attributes, new_tag);

                      if (struct_attribute)
                        {
                          gint sct;
                          value = gimp_attribute_get_string (struct_attribute);

                          for (sct = 0; sct < STRUCTURES_ON_PAGE; sct ++)
                            {
                              if (g_str_has_prefix (new_tag, struct_element[sct].struct_tag))
                                {
                                  highest_structure[struct_element[sct].number_of_element] = counter;
                                  break;
                                }
                            }
                          g_hash_table_insert (elements_table,
                                               (gpointer) g_strdup (new_tag),
                                               (gpointer) g_strdup (value));

                          gimp_attributes_remove_attribute (attributes, new_tag);
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
          attribute = gimp_attributes_get_attribute (attributes, tag);
          if (attribute)
            {
              gchar *value = gimp_attribute_get_string (attribute);
              g_hash_table_insert (elements_table,
                                   (gpointer) g_strdup (rights_entries[i].xmp_tag),
                                   (gpointer) g_strdup (value));

              gimp_attributes_remove_attribute (attributes, rights_entries[i].xmp_tag);
              g_free (value);
            }
        }
      g_free (tag);
    }

  for (sct = 0; sct < STRUCTURES_ON_PAGE; sct ++)
    {
      if (highest_structure[sct] > 0)
        curr_shown_structure[sct] = 1;
      else
        curr_shown_structure[sct] = 0;
    }

  page_rights_init_combobox (builder);

  page_rights_set_to_ui (builder,
                              -1); /* all */
}

void
page_rights_save_to_attributes (GimpAttributes *attributes)
{
  GHashTableIter     iter;
  gpointer           key, value;


  g_hash_table_iter_init (&iter, elements_table);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      GimpAttribute                   *attribute     = NULL;
      gchar                           *tag = (gchar *) key;
      gchar                           *value         = NULL;
      gint                             p;
      gint                             sct;
      gint i;

      value = (gchar *) g_hash_table_lookup (elements_table, (gpointer) tag);
      attribute = gimp_attribute_new_string (tag, value, TYPE_ASCII);

      if (attribute)
        {
          p = string_index_of (tag, "[", 0);                           /* is it a structure tag? */

          if (p > -1) /* yes! */
            {

              for (sct = 0; sct < STRUCTURES_ON_PAGE; sct ++)
                {
                  if (g_str_has_prefix (tag, struct_element[sct].struct_tag))
                    {
                      gimp_attribute_set_structure_type (attribute, struct_element[sct].struct_type);
                      break;
                    }
                }
            }

          for (i = 0; i < G_N_ELEMENTS(rights_entries); i++)
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


              if (! g_strcmp0 (t_tag, rights_entries[i].xmp_tag))
                {
                  if (rights_entries[i].number_of_comboarray > -1)
                    {
                      gint combobox_data_counter;
                      gint array_length;
                      gint number_in_comboarray = rights_entries[i].number_of_comboarray;

                      array_length = G_N_ELEMENTS(combobox_data[number_in_comboarray]);

                      for (combobox_data_counter = 0; combobox_data_counter < array_length; combobox_data_counter++)
                        {
                          const gchar  *val_in_tag = combobox_data[number_in_comboarray][combobox_data_counter].val_in_tag;
                          const gchar  *interpreted_val;

                          if (! val_in_tag)
                            break;

                          if (! g_strcmp0 (value, val_in_tag))
                            {
                              interpreted_val =  combobox_data[number_in_comboarray][combobox_data_counter].val_in_combo;
                              gimp_attribute_set_interpreted_string (attribute, interpreted_val);
                              break;
                            }
                        }
                    }
                  break;
                }
            }
          gimp_attributes_add_attribute (attributes, attribute);
        }
    }
}

static void
page_rights_entry_activate_cb (GtkWidget *widget, gpointer userdata)
{
  const gchar *entry_name = (const gchar *) userdata;
  const gchar *value      = gtk_entry_get_text (GTK_ENTRY (widget));

  page_rights_store_in_hash_table (entry_name, value, 0);
}

static gboolean
page_rights_entry_focus_out_cb (GtkWidget *widget, GdkEvent *event, gpointer userdata)
{
  const gchar *entry_name = (const gchar *) userdata;
  const gchar *value      = gtk_entry_get_text (GTK_ENTRY (widget));

  page_rights_store_in_hash_table (entry_name, value, 0);

  return FALSE;
}

static void
page_rights_entry_combo_changed_callback (GtkWidget *combo, gpointer userdata)
{
  const gchar *widget_name;
  const gchar *value;
  gint         array_nr   = GPOINTER_TO_INT (userdata);
  gint         index      = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  value = combobox_data[array_nr][index].val_in_tag;
  widget_name = gtk_widget_get_name (GTK_WIDGET (combo));

  page_rights_store_in_hash_table (widget_name, value, 0);
}

static gboolean
page_rights_store_in_hash_table (const gchar *entry_name, const gchar *value, gint nr)
{
  gint           i;
  const gchar   *o_tag;
  gint           p;
  gint           number = 0;
  gboolean       success;

  g_return_val_if_fail (entry_name != NULL, FALSE);

  if (nr > 0)
    number = nr;

  success = FALSE;

  for (i = 0; i < G_N_ELEMENTS(rights_entries); i++)
    {
      if (! g_strcmp0 (entry_name, rights_entries[i].ui_entry))
        {
          gchar   *new_tag;
          gchar   *nr_string;

          o_tag = rights_entries[i].xmp_tag;

          new_tag =g_strdup (o_tag);

          p = string_index_of (new_tag, "[x]", 0);                           /* is it a structure tag? */

          if (p > -1) /* yes */
            {
              gint sct;

              if (number <= 0)
                {
                  for (sct = 0; sct < STRUCTURES_ON_PAGE; sct ++)
                    {
                      if (g_strcmp0 (o_tag, struct_element[sct].struct_tag))
                        {
                          number = curr_shown_structure[struct_element[sct].number_of_element];
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
              if (g_hash_table_insert (elements_table, (gpointer) g_strdup (new_tag), (gpointer) g_strdup (value)))
                success = TRUE;
            }
          else
            {
              if (g_hash_table_remove (elements_table, (gpointer) new_tag))
                success = TRUE;
            }

          set_save_attributes_button_sensitive (TRUE);

          g_free (new_tag);
          break;
        }
    }
  return success;
}

static void
page_rights_structure_add (GtkButton *button, gpointer userdata)
{
  gint             number_to_add;
  GtkComboBox     *combo;
  GtkBuilder      *builder = (GtkBuilder *) userdata;
  GtkListStore    *liststore;
  GtkTreeIter      iter;
  const gchar     *widget_name;
  gchar           *line;
  gint             sct;
  gint             repaint;

  widget_name = gtk_widget_get_name (GTK_WIDGET (button));

  for (sct = 0; sct < STRUCTURES_ON_PAGE; sct ++)
    {
      if (! g_strcmp0 (widget_name, struct_element[sct].add_widget))
        {
          page_rights_structure_save (builder, struct_element[sct].number_of_element);
          number_to_add = ++highest_structure[struct_element[sct].number_of_element];

          liststore = GTK_LIST_STORE (get_widget_from_label (builder, struct_element[sct].struct_liststore_widget));
          combo = GTK_COMBO_BOX (get_widget_from_label (builder, struct_element[sct].struct_combo_widget));
          line = g_strdup_printf ("%s [%d]", struct_element[sct].identifier, number_to_add);
          curr_shown_structure [struct_element[sct].number_of_element] = number_to_add;

          repaint = struct_element[sct].number_of_element;

          g_signal_handlers_block_by_func (G_OBJECT (combo), G_CALLBACK (page_rights_combobox_changed_callback), builder);

          gtk_list_store_append(liststore, &iter);

          gtk_list_store_set (liststore, &iter,
                              0, line,
                              -1);

          gtk_combo_box_set_active (combo, number_to_add-1);

          if (number_to_add == 1)
            page_rights_set_entry_sensitive (builder, struct_element[sct].struct_tag, TRUE);

          g_signal_handlers_unblock_by_func(G_OBJECT (combo), G_CALLBACK (page_rights_combobox_changed_callback), builder);

          g_free (line);
          break;
       }
    }

  page_rights_set_to_ui (builder, repaint);
}

static void
page_rights_structure_remove (GtkButton *button, gpointer userdata)
{
  GtkBuilder      *builder           = (GtkBuilder *) userdata;
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

  reorder_table = g_hash_table_new_full  (g_str_hash, g_str_equal, g_free, g_free);

  widget_name = gtk_widget_get_name (GTK_WIDGET (button));

  for (sct = 0; sct < STRUCTURES_ON_PAGE; sct ++)
    {
      if (! g_strcmp0 (widget_name, struct_element[sct].remove_widget))
        {
          number_to_delete = curr_shown_structure[sct];
          tag_prefix = g_strdup (struct_element[sct].struct_tag);
          liststore = GTK_LIST_STORE (get_widget_from_label (builder, struct_element[sct].struct_liststore_widget));
          combo = GTK_COMBO_BOX (get_widget_from_label (builder, struct_element[sct].struct_combo_widget));
          combo_to_del = highest_structure[sct] - 1;

          if (number_to_delete == highest_structure[struct_element[sct].number_of_element])
            curr_shown_structure[struct_element[sct].number_of_element] = number_to_delete - 1;
          else
            curr_shown_structure[struct_element[sct].number_of_element] = number_to_delete;

          highest_structure[sct]--;
          repaint = struct_element[sct].number_of_element;
          break;
       }
    }

  nr_string = g_strdup_printf ("%s[%d]", tag_prefix, number_to_delete);

  /* remove entries */
  {

    g_hash_table_iter_init (&iter_remove, elements_table);
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
        g_hash_table_remove (elements_table, key);
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

        g_hash_table_iter_init (&iter_reorder, elements_table);
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
            g_hash_table_remove (elements_table, key);
          }
        g_slist_free_full (delete_key_list, g_free);

        g_hash_table_iter_init (&iter_reorder, reorder_table);
        while (g_hash_table_iter_next (&iter_reorder, &key, &value))
          {
            gchar *tag       = (gchar *) key;
            gchar *tag_value = (gchar *) value;

            g_hash_table_insert (elements_table, g_strdup (tag), g_strdup (tag_value));
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

  g_signal_handlers_block_by_func (G_OBJECT (combo), G_CALLBACK (page_rights_combobox_changed_callback), builder);

  gtk_combo_box_set_active (combo, combo_to_del);
  if (gtk_combo_box_get_active_iter (combo, &combo_iter))
    gtk_list_store_remove (liststore, &combo_iter);

  if (curr_shown_structure[repaint] == 0)
    page_rights_set_entry_sensitive (builder, struct_element[sct].struct_tag, FALSE);
  else
    gtk_combo_box_set_active (combo, curr_shown_structure[repaint] - 1);

  g_signal_handlers_unblock_by_func(G_OBJECT (combo), G_CALLBACK (page_rights_combobox_changed_callback), builder);

  page_rights_set_to_ui (builder, repaint);
}

static void
page_rights_combobox_changed_callback (GtkWidget *combo, gpointer userdata)
{
  GtkBuilder      *builder = (GtkBuilder *) userdata;
  GtkComboBox     *combobox;
  gint             nr;
  const gchar     *widget_name;
  gint             sct;
  gint             repaint;

  widget_name = gtk_widget_get_name (GTK_WIDGET (combo));

  for (sct = 0; sct < STRUCTURES_ON_PAGE; sct ++)
    {
      if (! g_strcmp0 (widget_name, struct_element[sct].struct_combo_widget))
        {
          page_rights_structure_save (builder, struct_element[sct].number_of_element);
          combobox = GTK_COMBO_BOX (get_widget_from_label (builder, struct_element[sct].struct_combo_widget));
          repaint  = struct_element[sct].number_of_element;
          break;
       }
    }

  nr = gtk_combo_box_get_active (combobox);
  nr++;
  curr_shown_structure[repaint] = nr;

  page_rights_set_to_ui (builder, repaint);
}

static void
page_rights_structure_save (GtkBuilder *builder, gint struct_number)
{
  gint         i;
  const gchar *prefix = struct_element[struct_number].struct_tag;

  for (i = 0; i < G_N_ELEMENTS(rights_entries); i++)
    {
      GObject       *obj;
      gchar         *tag      = NULL;
      const gchar   *o_tag;

      if (curr_shown_structure[struct_number] > 0)
        {
          o_tag = rights_entries[i].xmp_tag;
          if (g_str_has_prefix (o_tag, prefix))
            {
              gchar *nr_string;

              nr_string = g_strdup_printf ("[%d]", curr_shown_structure[struct_number]);
              tag = string_replace_str (o_tag, "[x]", nr_string);

              obj   = get_widget_from_label (builder, rights_entries[i].ui_entry);

              switch (rights_entries[i].widget_type)
              {
                case WIDGET_TYPE_ENTRY:
                  {
                    gchar *value;
                    value = g_strdup (gtk_entry_get_text (GTK_ENTRY (obj)));
                    if (value && g_strcmp0 (value, ""))
                      g_hash_table_insert (elements_table, (gpointer) g_strdup (tag), (gpointer) g_strdup (value));
                    else
                      g_hash_table_remove (elements_table, (gpointer) tag);
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
page_rights_set_to_ui (GtkBuilder *builder, gint repaint)
{
  gint         i;
  gint         sct;
  const gchar *prefix;

  if (repaint != -1)
    prefix = struct_element[repaint].struct_tag;
  else
    prefix = NULL;

  for (i = 0; i < G_N_ELEMENTS(rights_entries); i++)
    {
      GObject       *obj;
      const gchar   *o_tag;
      gchar         *value = NULL;
      gchar         *new_tag;
      gint           p;

      o_tag = rights_entries[i].xmp_tag;

      obj = get_widget_from_label (builder, rights_entries[i].ui_entry);

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
              nr_string = g_strdup_printf ("[%d]", curr_shown_structure[repaint]);
            }
          else
            {
              for (sct = 0; sct < STRUCTURES_ON_PAGE; sct ++)
                {
                  if (g_str_has_prefix (o_tag, struct_element[sct].struct_tag))
                    {
                      nr_string = g_strdup_printf ("[%d]", curr_shown_structure[sct]);
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

      value = (gchar *) g_hash_table_lookup (elements_table, new_tag);

      switch (rights_entries[i].widget_type)
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
          switch (rights_entries[i].widget_type)
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
                gint number_in_comboarray = rights_entries[i].number_of_comboarray;

                array_length = G_N_ELEMENTS(combobox_data[number_in_comboarray]);

                for (combobox_data_counter = 0; combobox_data_counter < array_length; combobox_data_counter++)
                  {
                    const gchar  *val_in_tag = combobox_data[number_in_comboarray][combobox_data_counter].val_in_tag;

                    if (! val_in_tag)
                      break;

                    if (! g_strcmp0 (value, val_in_tag))
                      {
                        g_signal_handlers_block_by_func (obj,
                                                         G_CALLBACK (page_rights_entry_combo_changed_callback),
                                                         GINT_TO_POINTER (number_in_comboarray));

                        gtk_combo_box_set_active (GTK_COMBO_BOX (obj), combobox_data_counter);

                        g_signal_handlers_unblock_by_func (obj,
                                                           G_CALLBACK (page_rights_entry_combo_changed_callback),
                                                           GINT_TO_POINTER (number_in_comboarray));
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
page_rights_init_combobox (GtkBuilder *builder)
{
  GtkListStore    *liststore;
  GtkTreeIter      iter;
  GtkComboBox     *combo;
  gint             sct;

  for (sct = 0; sct < STRUCTURES_ON_PAGE; sct ++)
    {
      gchar *line;
      gint   i;
      gint   high;
      gint   active;

      liststore = GTK_LIST_STORE (get_widget_from_label (builder, struct_element[sct].struct_liststore_widget));
      combo = GTK_COMBO_BOX (get_widget_from_label (builder, struct_element[sct].struct_combo_widget));

      high = highest_structure[struct_element[sct].number_of_element];
      active = curr_shown_structure[struct_element[sct].number_of_element];

      g_signal_handlers_block_by_func (G_OBJECT (combo), G_CALLBACK (page_rights_combobox_changed_callback), builder);

      for (i = 0; i < high; i++)
        {
          line = g_strdup_printf ("%s [%d]",struct_element[sct].identifier, i + 1);

          gtk_list_store_append(liststore, &iter);

          gtk_list_store_set (liststore, &iter,
                              0, line,
                              -1);
          g_free (line);
        }

      gtk_combo_box_set_active (combo, active-1);

      if (active >= 1)
        page_rights_set_entry_sensitive (builder, struct_element[sct].struct_tag, TRUE);
      else
        page_rights_set_entry_sensitive (builder, struct_element[sct].struct_tag, FALSE);

      g_signal_handlers_unblock_by_func(G_OBJECT (combo), G_CALLBACK (page_rights_combobox_changed_callback), builder);

    }
}
