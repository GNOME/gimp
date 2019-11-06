/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * metadata-editor.c
 * Copyright (C) 2016, 2017 Ben Touchette
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

#include <stdlib.h>
#include <ctype.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gexiv2/gexiv2.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "metadata-misc.h"
#include "metadata-xml.h"
#include "metadata-tags.h"

extern gboolean gimpmetadata;
extern gboolean force_write;

gboolean xmptag;
gboolean iptctag;
gboolean tagvalue;
gboolean taglistvalue;
gboolean tagname;
gboolean tagmode;
gboolean listelement;
gboolean element;
gchar *str_tag_value;
gchar *str_tag_name;
gchar *str_tag_mode;
gchar *str_element;
gchar *list_tag_data[256][256];
gint row_count = 0;
gint item_count = 0;


void
xml_parser_start_element (GMarkupParseContext  *context,
                          const gchar          *element_name,
                          const gchar         **attribute_names,
                          const gchar         **attribute_values,
                          gpointer              user_data,
                          GError              **error)
{
  if (strcmp (element_name, "gimp-metadata") == 0)
    {
      gimpmetadata = TRUE;
    }
  else if (strcmp (element_name, "iptc-tag") == 0)
    {
      item_count = 0;
      row_count = 0;
      iptctag = TRUE;
    }
  else if (strcmp (element_name, "xmp-tag") == 0)
    {
      item_count = 0;
      row_count = 0;
      xmptag = TRUE;
    }
  else if (strcmp (element_name, "tag-value") == 0)
    {
      tagvalue = TRUE;
    }
  else if (strcmp (element_name, "tag-list-value") == 0)
    {
      taglistvalue = TRUE;
    }
  else if (strcmp (element_name, "tag-name") == 0)
    {
      tagname = TRUE;
    }
  else if (strcmp (element_name, "tag-mode") == 0)
    {
      tagmode = TRUE;
    }
  else if (strcmp (element_name, "list-element") == 0)
    {
      listelement = TRUE;
      row_count += 1;
    }
  else if (strcmp (element_name, "element") == 0)
    {
      element = TRUE;
      item_count += 1;
    }
}

void
xml_parser_data (GMarkupParseContext *context,
                 const gchar         *text,
                 gsize                text_len,
                 gpointer             user_data,
                 GError             **error)
{
  if (tagvalue)
    {
      if (str_tag_value)
        g_free(str_tag_value);

      if (text)
        str_tag_value = g_strdup(text);
      else
        str_tag_value = g_strconcat("", NULL);
    }
  else if (tagname)
    {
      if (str_tag_name)
        g_free(str_tag_name);

      if (text)
        str_tag_name = g_strdup(text);
      else
        str_tag_name = g_strconcat("", NULL);
    }
  else if (tagmode)
    {
      if (str_tag_mode)
        g_free(str_tag_mode);

      if (text)
        str_tag_mode = g_strdup(text);
      else
        str_tag_mode = g_strconcat("", NULL);
    }
  else if (element)
    {
      if (str_element)
        g_free(str_element);

      if (text)
        str_element = g_strdup(text);
      else
        str_element = g_strconcat("", NULL);
    }
}

void
set_tag_ui (metadata_editor *args,
            gint             index,
            gchar           *name,
            gchar           *value,
            gchar*           mode)
{
  GtkWidget *widget;
  gchar *value_utf;

  widget = GTK_WIDGET (gtk_builder_get_object (args->builder, str_tag_name));

  if (!strcmp ("single", mode))
    {
      GtkEntry *entry_widget;

      value_utf = g_locale_to_utf8 (str_tag_value, -1, NULL, NULL, NULL);
      entry_widget = GTK_ENTRY (widget);
      gtk_entry_set_text (entry_widget, value_utf);
    }
  else if (!strcmp ("multi", mode))
    {
      GtkTextView   *text_view;
      GtkTextBuffer *buffer;

      value_utf = g_locale_to_utf8 (str_tag_value, -1, NULL, NULL, NULL);
      text_view = GTK_TEXT_VIEW (widget);
      buffer = gtk_text_view_get_buffer (text_view);
      gtk_text_buffer_set_text (buffer, value_utf, -1);
    }
  else if (!strcmp ("combo", mode))
    {
      gint32 value;

      value_utf = g_locale_to_utf8 (str_tag_value, -1, NULL, NULL, NULL);
      value = atoi(value_utf);
      gtk_combo_box_set_active (GTK_COMBO_BOX(widget), value);
    }
  else if (!strcmp ("list", mode))
    {
      GtkTreeModel  *treemodel;
      GtkListStore  *liststore;
      GtkTreeIter    iter;
      gint           number_of_rows;
      gint           row;
      gint           item;

      liststore = GTK_LIST_STORE(gtk_tree_view_get_model((GtkTreeView *)widget));
      treemodel = GTK_TREE_MODEL (liststore);
      number_of_rows =
        gtk_tree_model_iter_n_children(GTK_TREE_MODEL(liststore), NULL);

      /* Clear all current values */
      for (row = number_of_rows; row > -1; row--)
        {
          if (gtk_tree_model_iter_nth_child(treemodel, &iter, NULL, row))
            {
              gtk_list_store_remove(liststore, &iter);
            }
        }
      /* Add new values values */
      if (!strcmp (LICENSOR_HEADER, name))
        {
          for (row = 1; row < row_count+1; row++)
            {
              gtk_list_store_append (liststore, &iter);
              gtk_list_store_set (liststore, &iter,
                                  COL_LICENSOR_NAME, list_tag_data[row][1],
                                  COL_LICENSOR_ID, list_tag_data[row][2],
                                  COL_LICENSOR_PHONE1, list_tag_data[row][3],
                                  COL_LICENSOR_PHONE_TYPE1, list_tag_data[row][4],
                                  COL_LICENSOR_PHONE2, list_tag_data[row][5],
                                  COL_LICENSOR_PHONE_TYPE2, list_tag_data[row][6],
                                  COL_LICENSOR_EMAIL, list_tag_data[row][7],
                                  COL_LICENSOR_WEB, list_tag_data[row][8],
                                  -1);
              for (item = 1; item < n_licensor + 1; item++)
                {
                  if (list_tag_data[row][item])
                    {
                      if (list_tag_data[row][item])
                        g_free(list_tag_data[row][item]);
                    }
                }
            }

          if (row_count < 2)
            {
              for (row = 0; row < 2 - row_count; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_LICENSOR_NAME, NULL,
                                      COL_LICENSOR_ID, NULL,
                                      COL_LICENSOR_PHONE1, NULL,
                                      COL_LICENSOR_PHONE_TYPE1, NULL,
                                      COL_LICENSOR_PHONE2, NULL,
                                      COL_LICENSOR_PHONE_TYPE2, NULL,
                                      COL_LICENSOR_EMAIL, NULL,
                                      COL_LICENSOR_WEB, NULL,
                                      -1);
                }
            }
        }
      else if (!strcmp (IMAGECREATOR_HEADER, name))
        {
          for (row = 1; row < row_count+1; row++)
            {
              gtk_list_store_append (liststore, &iter);
              gtk_list_store_set (liststore, &iter,
                                  COL_IMG_CR8_NAME, list_tag_data[row][1],
                                  COL_IMG_CR8_ID, list_tag_data[row][2],
                                  -1);
              for (item = 1; item < n_imagecreator + 1; item++)
                {
                  if (list_tag_data[row][item])
                    {
                      if (list_tag_data[row][item])
                        g_free(list_tag_data[row][item]);
                    }
                }
            }

          if (row_count < 2)
            {
              for (row = 0; row < 2 - row_count; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_IMG_CR8_NAME, NULL,
                                      COL_IMG_CR8_ID, NULL,
                                      -1);
                }
            }
        }
      else if (!strcmp (ARTWORKOROBJECT_HEADER, name))
        {
          for (row = 1; row < row_count+1; row++)
            {
              gtk_list_store_append (liststore, &iter);
              gtk_list_store_set (liststore, &iter,
                                  COL_AOO_TITLE, list_tag_data[row][1],
                                  COL_AOO_DATE_CREAT, list_tag_data[row][2],
                                  COL_AOO_CREATOR, list_tag_data[row][3],
                                  COL_AOO_SOURCE, list_tag_data[row][4],
                                  COL_AOO_SRC_INV_ID, list_tag_data[row][5],
                                  COL_AOO_CR_NOT, list_tag_data[row][6],
                                  -1);
              for (item = 1; item < n_artworkorobject + 1; item++)
                {
                  if (list_tag_data[row][item])
                    {
                      if (list_tag_data[row][item])
                        g_free(list_tag_data[row][item]);
                    }
                }
            }

          if (row_count < 2)
            {
              for (row = 0; row < 2 - row_count; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_AOO_TITLE, NULL,
                                      COL_AOO_DATE_CREAT, NULL,
                                      COL_AOO_CREATOR, NULL,
                                      COL_AOO_SOURCE, NULL,
                                      COL_AOO_SRC_INV_ID, NULL,
                                      COL_AOO_CR_NOT, NULL,
                                      -1);
                }
            }
        }
      else if (!strcmp (REGISTRYID_HEADER, name))
        {
          for (row = 1; row < row_count+1; row++)
            {
              gtk_list_store_append (liststore, &iter);
              gtk_list_store_set (liststore, &iter,
                                  COL_REGSITRY_ORG_ID, list_tag_data[row][1],
                                  COL_REGSITRY_ITEM_ID, list_tag_data[row][2],
                                  -1);
              for (item = 1; item < n_registryid + 1; item++)
                {
                  if (list_tag_data[row][item])
                    {
                      if (list_tag_data[row][item])
                        g_free(list_tag_data[row][item]);
                    }
                }
            }

          if (row_count < 2)
            {
              for (row = 0; row < 2 - row_count; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_REGSITRY_ORG_ID, NULL,
                                      COL_REGSITRY_ITEM_ID, NULL,
                                      -1);
                }
            }
        }
      else if (!strcmp (COPYRIGHTOWNER_HEADER, name))
        {
          if (row_count > 0)
            {
              for (row = 1; row < row_count+1; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_CR_OWNER_NAME, list_tag_data[row][1],
                                      COL_CR_OWNER_ID, list_tag_data[row][2],
                                      -1);
                  for (item = 1; item < n_copyrightowner + 1; item++)
                    {
                      if (list_tag_data[row][item])
                        {
                          if (list_tag_data[row][item])
                            g_free(list_tag_data[row][item]);
                        }
                    }
                }
            }

          if (row_count < 2)
            {
              for (row = 0; row < 2 - row_count; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_CR_OWNER_NAME, NULL,
                                      COL_CR_OWNER_ID, NULL,
                                      -1);
                }
            }
        }
      else if (!strcmp (LOCATIONSHOWN_HEADER, name))
        {
          for (row = 1; row < row_count+1; row++)
            {
              gtk_list_store_append (liststore, &iter);
              gtk_list_store_set (liststore, &iter,
                                  COL_LOC_SHO_SUB_LOC, list_tag_data[row][1],
                                  COL_LOC_SHO_CITY, list_tag_data[row][2],
                                  COL_LOC_SHO_STATE_PROV, list_tag_data[row][3],
                                  COL_LOC_SHO_CNTRY, list_tag_data[row][4],
                                  COL_LOC_SHO_CNTRY_ISO, list_tag_data[row][5],
                                  COL_LOC_SHO_CNTRY_WRLD_REG, list_tag_data[row][6],
                                  -1);
              for (item = 1; item < n_locationshown + 1; item++)
                {
                  if (list_tag_data[row][item])
                    {
                      if (list_tag_data[row][item])
                        g_free(list_tag_data[row][item]);
                    }
                }
            }

          if (row_count < 2)
            {
              for (row = 0; row < 2 - row_count; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_LOC_SHO_SUB_LOC, NULL,
                                      COL_LOC_SHO_CITY, NULL,
                                      COL_LOC_SHO_STATE_PROV, NULL,
                                      COL_LOC_SHO_CNTRY, NULL,
                                      COL_LOC_SHO_CNTRY_ISO, NULL,
                                      COL_LOC_SHO_CNTRY_WRLD_REG, NULL,
                                      -1);
                }
            }
        }
      else if (!strcmp ("Xmp.iptcExt.OrganisationInImageName", name))
        {
          for (row = 1; row < row_count+1; row++)
            {
              gtk_list_store_append (liststore, &iter);
              gtk_list_store_set (liststore, &iter,
                                  COL_ORG_IMG_NAME, list_tag_data[row][1],
                                  -1);
              if (list_tag_data[row][1])
                {
                   if (list_tag_data[row][1])
                     g_free(list_tag_data[row][1]);
                }
            }

          if (row_count < 2)
            {
              for (row = 0; row < 2 - row_count; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_ORG_IMG_NAME, NULL,
                                      -1);
                }
            }
        }
      else if (!strcmp ("Xmp.iptcExt.OrganisationInImageCode", name))
        {
          for (row = 1; row < row_count+1; row++)
            {
              gtk_list_store_append (liststore, &iter);
              gtk_list_store_set (liststore, &iter,
                                  COL_ORG_IMG_CODE, list_tag_data[row][1],
                                  -1);
              if (list_tag_data[row][1])
                {
                   if (list_tag_data[row][1])
                     g_free(list_tag_data[row][1]);
                }
            }

          if (row_count < 2)
            {
              for (row = 0; row < 2 - row_count; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_ORG_IMG_CODE, NULL,
                                      -1);
                }
            }
        }
      else if (!strcmp ("Xmp.plus.PropertyReleaseID", name))
        {
          for (row = 1; row < row_count+1; row++)
            {
              gtk_list_store_append (liststore, &iter);
              gtk_list_store_set (liststore, &iter,
                                  COL_PROP_REL_ID, list_tag_data[row][1],
                                  -1);
              if (list_tag_data[row][1])
                {
                   if (list_tag_data[row][1])
                     g_free(list_tag_data[row][1]);
                }
            }

          if (row_count < 2)
            {
              for (row = 0; row < 2 - row_count; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_PROP_REL_ID, NULL,
                                      -1);
                }
            }
        }
      else if (!strcmp ("Xmp.plus.ModelReleaseID", name))
        {
          for (row = 1; row < row_count+1; row++)
            {
              gtk_list_store_append (liststore, &iter);
              gtk_list_store_set (liststore, &iter,
                                  COL_MOD_REL_ID, list_tag_data[row][1],
                                  -1);
              if (list_tag_data[row][1])
                {
                   if (list_tag_data[row][1])
                     g_free(list_tag_data[row][1]);
                }
            }

          if (row_count < 2)
            {
              for (row = 0; row < 2 - row_count; row++)
                {
                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      COL_MOD_REL_ID, NULL,
                                      -1);
                }
            }
        }
    }
}

const gchar *
get_tag_ui_text (metadata_editor *args,
                 gchar           *name,
                 gchar           *mode)
{
  GObject *object;

  object = gtk_builder_get_object (args->builder, name);

  if (! strcmp ("single", mode))
    {
      GtkEntry *entry = GTK_ENTRY (object);
      return gtk_entry_get_text (entry);
    }
  else if (!strcmp ("multi", mode))
    {
      GtkTextView   *text_view = GTK_TEXT_VIEW (object);
      GtkTextBuffer *buffer;
      GtkTextIter    start;
      GtkTextIter    end;

      buffer = gtk_text_view_get_buffer (text_view);
      gtk_text_buffer_get_start_iter (buffer, &start);
      gtk_text_buffer_get_end_iter (buffer, &end);

      return gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
    }

  return NULL;
}

gchar *
get_tag_ui_list (metadata_editor *args, gchar *name, gchar *mode)
{
  GObject       *object;
  GtkWidget     *widget;
  GtkTreeModel  *treemodel;
  GtkListStore  *liststore;
  GtkTreeIter    iter;
  gchar         *xmldata;
  gint           number_of_rows;
  gint           row;
  gint           has_data;
  gchar         *tagdata[256][256];

  has_data = FALSE;
  xmldata = (gchar*)g_malloc(262144);
  object = gtk_builder_get_object (args->builder, name);
  widget = GTK_WIDGET(object);

  liststore = GTK_LIST_STORE(gtk_tree_view_get_model((GtkTreeView *)widget));
  treemodel = GTK_TREE_MODEL (liststore);
  number_of_rows =
    gtk_tree_model_iter_n_children(GTK_TREE_MODEL(liststore), NULL);

  for (row = 0; row < number_of_rows; row++)
    {
      if (gtk_tree_model_iter_nth_child(treemodel, &iter, NULL, row))
        {
          if (!strcmp (LICENSOR_HEADER, name))
            {
              gtk_tree_model_get (treemodel, &iter,
                                  COL_LICENSOR_NAME, &tagdata[row][0],
                                  COL_LICENSOR_ID, &tagdata[row][1],
                                  COL_LICENSOR_PHONE1, &tagdata[row][2],
                                  COL_LICENSOR_PHONE_TYPE1, &tagdata[row][3],
                                  COL_LICENSOR_PHONE2, &tagdata[row][4],
                                  COL_LICENSOR_PHONE_TYPE2, &tagdata[row][5],
                                  COL_LICENSOR_EMAIL, &tagdata[row][6],
                                  COL_LICENSOR_WEB, &tagdata[row][7],
                                  -1);

              if ((tagdata[row][0] != NULL && strlen(tagdata[row][0]) > 0) ||
                  (tagdata[row][1] != NULL && strlen(tagdata[row][1]) > 0) ||
                  (tagdata[row][2] != NULL && strlen(tagdata[row][2]) > 0) ||
                  (tagdata[row][3] != NULL && strlen(tagdata[row][3]) > 0) ||
                  (tagdata[row][4] != NULL && strlen(tagdata[row][4]) > 0) ||
                  (tagdata[row][5] != NULL && strlen(tagdata[row][5]) > 0) ||
                  (tagdata[row][6] != NULL && strlen(tagdata[row][6]) > 0) ||
                  (tagdata[row][7] != NULL && strlen(tagdata[row][7]) > 0))
                {
                  gint  types;

                  has_data = TRUE;

                  xmldata = g_strconcat (xmldata, "\t\t\t<list-element>\n", NULL);
                  for (types = 0; types < 8; types++)
                    {
                      xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                      xmldata = g_strconcat (xmldata, tagdata[row][types], NULL);
                      xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                    }
                  xmldata = g_strconcat (xmldata, "\t\t\t</list-element>\n", NULL);
                }
            }
          else if (!strcmp (COPYRIGHTOWNER_HEADER, name))
            {
              gtk_tree_model_get (treemodel, &iter,
                                  COL_CR_OWNER_NAME, &tagdata[row][0],
                                  COL_CR_OWNER_ID, &tagdata[row][1],
                                  -1);

              if ((tagdata[row][0] != NULL && strlen(tagdata[row][0]) > 0) ||
                  (tagdata[row][1] != NULL && strlen(tagdata[row][1]) > 0))
                {
                  has_data = TRUE;

                  xmldata = g_strconcat (xmldata, "\t\t\t<list-element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                  xmldata = g_strconcat (xmldata, tagdata[row][0], NULL);
                  xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                  xmldata = g_strconcat (xmldata, tagdata[row][1], NULL);
                  xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t</list-element>\n", NULL);
                }
            }
          else if (!strcmp (IMAGECREATOR_HEADER, name))
            {
              gtk_tree_model_get (treemodel, &iter,
                                  COL_IMG_CR8_NAME, &tagdata[row][0],
                                  COL_IMG_CR8_ID, &tagdata[row][1],
                                  -1);

              if ((tagdata[row][0] != NULL && strlen(tagdata[row][0]) > 0) ||
                  (tagdata[row][1] != NULL && strlen(tagdata[row][1]) > 0))
                {
                  gint  types;

                  has_data = TRUE;

                  xmldata = g_strconcat (xmldata, "\t\t\t<list-element>\n", NULL);
                  for (types = 0; types < 2; types++)
                    {
                      xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                      xmldata = g_strconcat (xmldata, tagdata[row][types], NULL);
                      xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                    }
                  xmldata = g_strconcat (xmldata, "\t\t\t</list-element>\n", NULL);
                }
            }
          else if (!strcmp (ARTWORKOROBJECT_HEADER, name))
            {
              gtk_tree_model_get (treemodel, &iter,
                                  COL_AOO_TITLE, &tagdata[row][0],
                                  COL_AOO_DATE_CREAT, &tagdata[row][1],
                                  COL_AOO_CREATOR, &tagdata[row][2],
                                  COL_AOO_SOURCE, &tagdata[row][3],
                                  COL_AOO_SRC_INV_ID, &tagdata[row][4],
                                  COL_AOO_CR_NOT, &tagdata[row][5],
                                  -1);

              if ((tagdata[row][0] != NULL && strlen(tagdata[row][0]) > 0) ||
                  (tagdata[row][1] != NULL && strlen(tagdata[row][1]) > 0) ||
                  (tagdata[row][2] != NULL && strlen(tagdata[row][2]) > 0) ||
                  (tagdata[row][3] != NULL && strlen(tagdata[row][3]) > 0) ||
                  (tagdata[row][4] != NULL && strlen(tagdata[row][4]) > 0) ||
                  (tagdata[row][5] != NULL && strlen(tagdata[row][5]) > 0))
                {
                  gint  types;

                  has_data = TRUE;

                  xmldata = g_strconcat (xmldata, "\t\t\t<list-element>\n", NULL);
                  for (types = 0; types < 6; types++)
                    {
                      xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                      xmldata = g_strconcat (xmldata, tagdata[row][types], NULL);
                      xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                    }
                  xmldata = g_strconcat (xmldata, "\t\t\t</list-element>\n", NULL);
                }
            }
          else if (!strcmp (REGISTRYID_HEADER, name))
            {
              gtk_tree_model_get (treemodel, &iter,
                                  COL_REGSITRY_ORG_ID, &tagdata[row][0],
                                  COL_REGSITRY_ITEM_ID, &tagdata[row][1],
                                  -1);

              if ((tagdata[row][0] != NULL && strlen(tagdata[row][0]) > 0) ||
                  (tagdata[row][1] != NULL && strlen(tagdata[row][1]) > 0))
                {
                  gint  types;

                  has_data = TRUE;

                  xmldata = g_strconcat (xmldata, "\t\t\t<list-element>\n", NULL);
                  for (types = 0; types < 2; types++)
                    {
                      xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                      xmldata = g_strconcat (xmldata, tagdata[row][types], NULL);
                      xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                    }
                  xmldata = g_strconcat (xmldata, "\t\t\t</list-element>\n", NULL);
                }
            }
          else if (!strcmp (LOCATIONSHOWN_HEADER, name))
            {
              gtk_tree_model_get (treemodel, &iter,
                                  COL_LOC_SHO_SUB_LOC, &tagdata[row][0],
                                  COL_LOC_SHO_CITY, &tagdata[row][1],
                                  COL_LOC_SHO_STATE_PROV, &tagdata[row][2],
                                  COL_LOC_SHO_CNTRY, &tagdata[row][3],
                                  COL_LOC_SHO_CNTRY_ISO, &tagdata[row][4],
                                  COL_LOC_SHO_CNTRY_WRLD_REG, &tagdata[row][5],
                                  -1);

              if ((tagdata[row][0] != NULL && strlen(tagdata[row][0]) > 0) ||
                  (tagdata[row][1] != NULL && strlen(tagdata[row][1]) > 0) ||
                  (tagdata[row][2] != NULL && strlen(tagdata[row][2]) > 0) ||
                  (tagdata[row][3] != NULL && strlen(tagdata[row][3]) > 0) ||
                  (tagdata[row][4] != NULL && strlen(tagdata[row][4]) > 0) ||
                  (tagdata[row][5] != NULL && strlen(tagdata[row][5]) > 0))
                {
                  gint  types;

                  has_data = TRUE;

                  xmldata = g_strconcat (xmldata, "\t\t\t<list-element>\n", NULL);
                  for (types = 0; types < 6; types++)
                    {
                      xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                      xmldata = g_strconcat (xmldata, tagdata[row][types], NULL);
                      xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                    }
                  xmldata = g_strconcat (xmldata, "\t\t\t</list-element>\n", NULL);
                }
            }
          else if (!strcmp ("Xmp.iptcExt.OrganisationInImageName", name))
            {
              gtk_tree_model_get (treemodel, &iter,
                                  COL_ORG_IMG_NAME, &tagdata[row][0],
                                  -1);

              if ((tagdata[row][0] != NULL && strlen(tagdata[row][0]) > 0))
                {
                  has_data = TRUE;

                  xmldata = g_strconcat (xmldata, "\t\t\t<list-element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                  xmldata = g_strconcat (xmldata, tagdata[row][0], NULL);
                  xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t</list-element>\n", NULL);
                }
            }
          else if (!strcmp ("Xmp.iptcExt.OrganisationInImageCode", name))
            {
              gtk_tree_model_get (treemodel, &iter,
                                  COL_ORG_IMG_CODE, &tagdata[row][0],
                                  -1);

              if ((tagdata[row][0] != NULL && strlen(tagdata[row][0]) > 0))
                {
                  has_data = TRUE;

                  xmldata = g_strconcat (xmldata, "\t\t\t<list-element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                  xmldata = g_strconcat (xmldata, tagdata[row][0], NULL);
                  xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t</list-element>\n", NULL);
                }
            }
          else if (!strcmp ("Xmp.plus.PropertyReleaseID", name))
            {
              gtk_tree_model_get (treemodel, &iter,
                                  COL_PROP_REL_ID, &tagdata[row][0],
                                  -1);

              if ((tagdata[row][0] != NULL && strlen(tagdata[row][0]) > 0))
                {
                  has_data = TRUE;

                  xmldata = g_strconcat (xmldata, "\t\t\t<list-element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                  xmldata = g_strconcat (xmldata, tagdata[row][0], NULL);
                  xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t</list-element>\n", NULL);
                }
            }
          else if (!strcmp ("Xmp.plus.ModelReleaseID", name))
            {
              gtk_tree_model_get (treemodel, &iter,
                                  COL_MOD_REL_ID, &tagdata[row][0],
                                  -1);

              if ((tagdata[row][0] != NULL && strlen(tagdata[row][0]) > 0))
                {
                  has_data = TRUE;

                  xmldata = g_strconcat (xmldata, "\t\t\t<list-element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t\t<element>", NULL);
                  xmldata = g_strconcat (xmldata, tagdata[row][0], NULL);
                  xmldata = g_strconcat (xmldata, "</element>\n", NULL);
                  xmldata = g_strconcat (xmldata, "\t\t\t</list-element>\n", NULL);
                }
            }
        }
    }

  if (has_data == TRUE)
    {
      return xmldata;
    }

  g_free(xmldata);

  return NULL;
}

gint
get_tag_ui_combo (metadata_editor *args, gchar *name, gchar *mode)
{
  GObject *object;
  GtkComboBoxText *combo;
  gint value;

  object = gtk_builder_get_object (args->builder, name);

  combo = GTK_COMBO_BOX_TEXT (object);
  value = gtk_combo_box_get_active (GTK_COMBO_BOX(combo));

  return value;
}

void
xml_parser_end_element (GMarkupParseContext  *context,
                        const gchar          *element_name,
                        gpointer              user_data,
                        GError              **error)
{
  metadata_editor *args;
  int i;

  args = user_data;

  if (strcmp (element_name, "gimp-metadata") == 0)
    {
      gimpmetadata = FALSE;
    }
  else if (strcmp (element_name, "iptc-tag") == 0)
    {
      iptctag = FALSE;
#ifdef _ENABLE_IPTC_TAG_
      if (str_tag_name && str_tag_value)
        {
          /* make sure to only allow supported tags */
          for (i = 0; i < n_equivalent_metadata_tags; i++)
            {
              if (strcmp(equivalent_metadata_tags[i].tag, str_tag_name) == 0)
                {
#ifdef _SET_IPTC_TAG_
                  set_tag_ui (args, i, str_tag_name, str_tag_value,
                              equivalent_metadata_tags[i].mode);
#endif
                  if (force_write == TRUE)
                    gexiv2_metadata_set_tag_string (args->metadata,
                                                    str_tag_name,
                                                    str_tag_value);
                  break;
                }
            }
        }
#endif
    }
  else if (strcmp (element_name, "xmp-tag") == 0)
    {
      xmptag = FALSE;
      if (strcmp (str_tag_mode, "list") != 0)
        {
          if (str_tag_name && str_tag_value)
            {
              /* make sure to only allow supported tags */
              for (i = 0; i < n_default_metadata_tags; i++)
                {
                  if (strcmp(default_metadata_tags[i].tag, str_tag_name) == 0)
                    {
                      set_tag_ui (args, i, str_tag_name, str_tag_value,
                                  default_metadata_tags[i].mode);
#ifdef _ENABLE_FORCE_WRITE_
                      if (force_write == TRUE)
                        gexiv2_metadata_set_tag_string (args->metadata,
                                                        str_tag_name,
                                                        str_tag_value);
#endif
                      break;
                    }
                }
            }

        }
      else if (strcmp (str_tag_mode, "list") == 0)
        {
          if (row_count > 0)
            {
              /* make sure to only allow supported tags */
              for (i = 0; i < n_default_metadata_tags; i++)
                {
                  if (strcmp(default_metadata_tags[i].tag, str_tag_name) == 0)
                    {
                      set_tag_ui (args, i, str_tag_name, str_tag_value,
                                       default_metadata_tags[i].mode);
#ifdef _ENABLE_FORCE_WRITE_
                      if (force_write == TRUE)
                        gexiv2_metadata_set_tag_string (args->metadata,
                                                        str_tag_name,
                                                        str_tag_value);
#endif
                      break;
                    }
                }
            }
          row_count = 0;
          item_count = 0;
        }
    }
  else if (strcmp (element_name, "tag-value") == 0)
    {
      tagvalue = FALSE;
    }
  else if (strcmp (element_name, "tag-list-value") == 0)
    {
      taglistvalue = FALSE;
    }
  else if (strcmp (element_name, "tag-name") == 0)
    {
      tagname = FALSE;
    }
  else if (strcmp (element_name, "tag-mode") == 0)
    {
      tagmode = FALSE;
    }
  else if (strcmp (element_name, "list-element") == 0)
    {
      listelement = FALSE;
      item_count = 0;
    }
  else if (strcmp (element_name, "element") == 0)
    {
      element = FALSE;
      list_tag_data[row_count][item_count] = g_strdup(str_element);
    }
}

gboolean
xml_parser_parse_file (GimpXmlParser  *parser,
                       const gchar    *filename,
                       GError        **error)
{
  GIOChannel *io;
  gboolean    success;

  g_return_val_if_fail (parser != NULL, FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  io = g_io_channel_new_file (filename, "r", error);
  if (!io)
    return FALSE;

  success = xml_parser_parse_io_channel (parser, io, error);

  g_io_channel_unref (io);

  return success;
}

GimpXmlParser *
xml_parser_new (const GMarkupParser *markup_parser,
                gpointer             user_data)
{
  GimpXmlParser *parser;

  g_return_val_if_fail (markup_parser != NULL, NULL);

  parser = g_slice_new (GimpXmlParser);

  parser->context = g_markup_parse_context_new (markup_parser,
                    0, user_data, NULL);

  return parser;
}

void
xml_parser_free (GimpXmlParser *parser)
{
  g_return_if_fail (parser != NULL);

  g_markup_parse_context_free (parser->context);
  g_slice_free (GimpXmlParser, parser);
}

gboolean
parse_encoding (const gchar  *text,
                gint          text_len,
                gchar       **encoding)
{
  const gchar *start;
  const gchar *end;
  gint         i;

  g_return_val_if_fail (text, FALSE);

  if (text_len < 20)
    return FALSE;

  start = g_strstr_len (text, text_len, "<?xml");
  if (!start)
    return FALSE;

  end = g_strstr_len (start, text_len - (start - text), "?>");
  if (!end)
    return FALSE;

  *encoding = NULL;

  text_len = end - start;
  if (text_len < 12)
    return TRUE;

  start = g_strstr_len (start + 1, text_len - 1, "encoding");
  if (!start)
    return TRUE;

  start += 8;

  while (start < end && *start == ' ')
    start++;

  if (*start != '=')
    return TRUE;

  start++;

  while (start < end && *start == ' ')
    start++;

  if (*start != '\"' && *start != '\'')
    return TRUE;

  text_len = end - start;
  if (text_len < 1)
    return TRUE;

  for (i = 1; i < text_len; i++)
    if (start[i] == start[0])
      break;

  if (i == text_len || i < 3)
    return TRUE;

  *encoding = g_strndup (start + 1, i - 1);

  return TRUE;
}

gboolean
xml_parser_parse_io_channel (GimpXmlParser  *parser,
                             GIOChannel     *io,
                             GError        **error)
{
  GIOStatus    status;
  gchar        buffer[4096];
  gsize        len = 0;
  gsize        bytes;
  const gchar *io_encoding;
  gchar       *encoding = NULL;

  g_return_val_if_fail (parser != NULL, FALSE);
  g_return_val_if_fail (io != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  io_encoding = g_io_channel_get_encoding (io);
  if (g_strcmp0 (io_encoding, "UTF-8"))
    {
      g_warning ("xml_parser_parse_io_channel():\n"
                 "The encoding has already been set on this GIOChannel!");
      return FALSE;
    }

  /* try to determine the encoding */

  g_io_channel_set_encoding (io, NULL, NULL);

  while (len < sizeof (buffer))
    {
      status = g_io_channel_read_chars (io, buffer + len, 1, &bytes, error);
      len += bytes;

      if (status == G_IO_STATUS_ERROR)
        return FALSE;
      if (status == G_IO_STATUS_EOF)
        break;

      if (parse_encoding (buffer, len, &encoding))
        break;
    }

  if (encoding)
    {
      if (! g_io_channel_set_encoding (io, encoding, error))
        return FALSE;

      if (encoding)
        g_free (encoding);
    }
  else
    {
      g_io_channel_set_encoding (io, "UTF-8", NULL);
    }

  while (TRUE)
    {
      if (!g_markup_parse_context_parse (parser->context, buffer, len, error))
        return FALSE;

      status = g_io_channel_read_chars (io,
                                        buffer, sizeof (buffer), &len, error);

      switch (status)
        {
        case G_IO_STATUS_ERROR:
          return FALSE;
        case G_IO_STATUS_EOF:
          return g_markup_parse_context_end_parse (parser->context, error);
        case G_IO_STATUS_NORMAL:
        case G_IO_STATUS_AGAIN:
          break;
        }
    }
}

