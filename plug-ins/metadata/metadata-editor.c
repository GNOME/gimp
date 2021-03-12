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

#include "metadata-tags.h"
#include "metadata-xml.h"
#include "metadata-impexp.h"
#include "metadata-misc.h"

#define PLUG_IN_PROC            "plug-in-metadata-editor"
#define PLUG_IN_BINARY          "metadata-editor"
#define PLUG_IN_ROLE            "gimp-metadata"

#define DEFAULT_TEMPLATE_FILE   "gimp_metadata_template.xml"


typedef struct _Metadata      Metadata;
typedef struct _MetadataClass MetadataClass;

struct _Metadata
{
  GimpPlugIn parent_instance;
};

struct _MetadataClass
{
  GimpPlugInClass parent_class;
};


#define METADATA_TYPE  (metadata_get_type ())
#define METADATA (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), METADATA_TYPE, Metadata))

GType                   metadata_get_type         (void) G_GNUC_CONST;

static GList          * metadata_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * metadata_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * metadata_run              (GimpProcedure        *procedure,
                                                   const GimpValueArray *args,
                                                   gpointer              run_data);

static gboolean metadata_editor_dialog           (GimpImage           *image,
                                                  GimpMetadata        *metadata,
                                                  GError             **error);

static void metadata_dialog_editor_set_metadata  (GExiv2Metadata      *metadata,
                                                  GtkBuilder          *builder);

void metadata_editor_write_callback              (GtkWidget           *dialog,
                                                  GtkBuilder          *builder,
                                                  GimpImage           *image);

static void impex_combo_callback                (GtkComboBoxText      *combo,
                                                 gpointer              data);

static void gpsaltsys_combo_callback            (GtkComboBoxText      *combo,
                                                 gpointer              data);

static void remove_substring                    (const gchar          *string,
                                                 const gchar          *substring);

static gchar * clean_xmp_string                 (const gchar          *value);
static gchar ** split_metadata_string           (gchar                *value);
static void     add_to_store                    (gchar                *value,
                                                 GtkListStore         *liststore,
                                                 gint                  store_column);

static void     set_tag_string                  (GimpMetadata         *metadata,
                                                 const gchar          *name,
                                                 const gchar          *value);

static void     write_metadata_tag              (GtkBuilder           *builder,
                                                 GimpMetadata         *metadata,
                                                 gchar                *tag,
                                                 gint                  data_column);

gboolean hasCreatorTagData                      (GtkBuilder           *builder);
gboolean hasLocationCreationTagData             (GtkBuilder           *builder);
gboolean hasImageSupplierTagData                (GtkBuilder           *builder);

void on_date_button_clicked                     (GtkButton            *widget,
                                                 GtkWidget            *entry_widget,
                                                 gchar                *tag);

void on_create_date_button_clicked              (GtkButton            *widget,
                                                 gpointer              data);

void on_patient_dob_date_button_clicked         (GtkButton            *widget,
                                                 gpointer              data);

void on_study_date_button_clicked               (GtkButton            *widget,
                                                 gpointer              data);

void on_series_date_button_clicked              (GtkButton            *widget,
                                                 gpointer              data);


static void
property_release_id_remove_callback             (GtkWidget            *widget,
                                                 gpointer              data);
static void
property_release_id_add_callback                (GtkWidget            *widget,
                                                 gpointer              data);
static void
model_release_id_remove_callback                (GtkWidget            *widget,
                                                 gpointer              data);
static void
model_release_id_add_callback                   (GtkWidget            *widget,
                                                 gpointer              data);
static void
shown_location_remove_callback                  (GtkWidget            *widget,
                                                 gpointer              data);
static void
shown_location_add_callback                     (GtkWidget            *widget,
                                                 gpointer              data);
static void
feat_org_name_add_callback                      (GtkWidget            *widget,
                                                 gpointer              data);
static void
feat_org_name_remove_callback                   (GtkWidget            *widget,
                                                 gpointer              data);
static void
feat_org_code_add_callback                      (GtkWidget            *widget,
                                                 gpointer              data);
static void
feat_org_code_remove_callback                   (GtkWidget            *widget,
                                                 gpointer              data);
static void
artwork_object_add_callback                     (GtkWidget            *widget,
                                                 gpointer              data);
static void
artwork_object_remove_callback                  (GtkWidget            *widget,
                                                 gpointer              data);
static void
reg_entry_add_callback                          (GtkWidget            *widget,
                                                 gpointer              data);
static void
reg_entry_remove_callback                       (GtkWidget            *widget,
                                                 gpointer              data);
static void
image_creator_add_callback                      (GtkWidget            *widget,
                                                 gpointer              data);
static void
image_creator_remove_callback                   (GtkWidget            *widget,
                                                 gpointer              data);

static void
copyright_own_add_callback                      (GtkWidget            *widget,
                                                 gpointer              data);
static void
copyright_own_remove_callback                   (GtkWidget            *widget,
                                                 gpointer              data);
static void
licensor_add_callback                           (GtkWidget            *widget,
                                                 gpointer              data);
static void
licensor_remove_callback                        (GtkWidget            *widget,
                                                 gpointer              data);

static void
list_row_remove_callback                        (GtkWidget            *widget,
                                                 gpointer              data,
                                                 gchar                *tag);

static void
list_row_add_callback                           (GtkWidget            *widget,
                                                 gpointer              data,
                                                 gchar                *tag);

static gint
count_tags                                      (GExiv2Metadata       *metadata,
                                                 const gchar          *header,
                                                 const gchar         **tags,
                                                 int                   items);

static gchar **
get_tags                                        (GExiv2Metadata       *metadata,
                                                 const gchar          *header,
                                                 const gchar         **tags,
                                                 const int             items,
                                                 const int             count);

static void
free_tagdata                                    (gchar               **tagdata,
                                                 gint                  rows,
                                                 gint                  cols);

gboolean
hasModelReleaseTagData                          (GtkBuilder           *builder);

gboolean
hasPropertyReleaseTagData                       (GtkBuilder           *builder);

static void
organisation_image_code_cell_edited_callback    (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
organisation_image_name_cell_edited_callback    (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
prop_rel_id_cell_edited_callback                (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
loc_sho_sub_loc_cell_edited_callback            (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
loc_sho_city_cell_edited_callback               (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
loc_sho_state_prov_cell_edited_callback         (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
loc_sho_cntry_cell_edited_callback              (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
loc_sho_cntry_iso_cell_edited_callback          (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
reg_org_id_cell_edited_callback                 (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
reg_item_id_cell_edited_callback                (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_title_cell_edited_callback                  (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_copyright_notice_cell_edited_callback       (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_source_inv_cell_edited_callback             (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_source_cell_edited_callback                 (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_creator_cell_edited_callback                (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
aoo_date_creat_cell_edited_callback             (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
cr_owner_name_cell_edited_callback              (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
cr_owner_id_cell_edited_callback                (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_name_cell_edited_callback              (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_id_cell_edited_callback                (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_phone1_cell_edited_callback            (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_phone_type1_cell_edited_callback       (GtkCellRendererCombo *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_phone2_cell_edited_callback            (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_phone_type2_cell_edited_callback       (GtkCellRendererCombo *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_email_cell_edited_callback             (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

static void
licensor_web_cell_edited_callback               (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data);

void
cell_edited_callback                            (GtkCellRendererText  *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data,
                                                 int                   index);

void
cell_edited_callback_combo                      (GtkCellRendererCombo *cell,
                                                 const gchar          *path_string,
                                                 const gchar          *new_text,
                                                 gpointer              data,
                                                 int                   index);


G_DEFINE_TYPE (Metadata, metadata, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (METADATA_TYPE)


static int last_gpsaltsys_sel;

gboolean gimpmetadata;
gboolean force_write;

static const gchar *lang_default = "lang=\"x-default\"";
static const gchar *seq_default = "type=\"Seq\"";
static const gchar *bag_default = "type=\"Bag\"";

metadata_editor meta_args;

#define ME_LOG_DOMAIN "metadata-editor"


static void
metadata_class_init (MetadataClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = metadata_query_procedures;
  plug_in_class->create_procedure = metadata_create_procedure;
}

static void
metadata_init (Metadata *metadata)
{
}

static GList *
metadata_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
metadata_create_procedure (GimpPlugIn  *plug_in,
                           const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      metadata_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("_Edit Metadata"));
      gimp_procedure_add_menu_path (procedure, "<Image>/Image/Metadata");

      gimp_procedure_set_documentation (procedure,
                                        N_("Edit metadata (IPTC, EXIF, XMP)"),
                                        "Edit metadata information attached "
                                        "to the current image. Some or all "
                                        "of this metadata will be saved in "
                                        "the file, depending on the output "
                                        "file format.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Ben Touchette",
                                      "Ben Touchette",
                                      "2017");

      GIMP_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          GIMP_TYPE_RUN_MODE,
                          GIMP_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);

      GIMP_PROC_ARG_IMAGE (procedure, "image",
                           "Image",
                           "The input image",
                           FALSE,
                           G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
metadata_run (GimpProcedure        *procedure,
              const GimpValueArray *args,
              gpointer              run_data)
{
  GimpImage    *image;
  GimpMetadata *metadata;
  GError       *error  = NULL;

  INIT_I18N ();

  gimp_ui_init (PLUG_IN_BINARY);

  image = GIMP_VALUES_GET_IMAGE (args, 1);

  metadata = gimp_image_get_metadata (image);

  /* Always show metadata dialog so we can add appropriate iptc data
   * as needed. Sometimes license data needs to be added after the
   * fact and the image may not contain metadata but should have it
   * added as needed.
   */
  if (! metadata)
    {
      metadata = gimp_metadata_new ();
      gimp_image_set_metadata (image, metadata);
    }

  if (metadata_editor_dialog (image, metadata, &error))
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
  else
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_EXECUTION_ERROR, error);
}


/* ============================================================================
 * ==[ EDITOR DIALOG UI ]======================================================
 * ============================================================================
 */

static GtkWidget *
builder_get_widget (GtkBuilder  *builder,
                    const gchar *name)
{
  GObject *object = gtk_builder_get_object (builder, name);

  return GTK_WIDGET (object);
}

static gboolean
metadata_editor_dialog (GimpImage     *image,
                        GimpMetadata  *g_metadata,
                        GError       **error)
{
  GtkBuilder     *builder;
  GtkWidget      *dialog;
  GtkWidget      *metadata_vbox;
  GtkWidget      *impex_combo;
  GtkWidget      *content_area;
  GExiv2Metadata *metadata;
  gchar          *ui_file;
  gchar          *title;
  gchar          *name;
  GError         *local_error = NULL;
  gboolean        run;

  metadata = GEXIV2_METADATA (g_metadata);

  builder = gtk_builder_new ();

  meta_args.image    = image;
  meta_args.builder  = builder;
  meta_args.metadata = metadata;
  meta_args.filename = g_strconcat (g_get_home_dir (), "/", DEFAULT_TEMPLATE_FILE,
                                    NULL);

  ui_file = g_build_filename (gimp_data_directory (),
                              "ui", "plug-ins", "plug-in-metadata-editor.ui", NULL);

  if (! gtk_builder_add_from_file (builder, ui_file, &local_error))
    {
      if (! local_error)
        local_error = g_error_new_literal (G_FILE_ERROR, 0,
                                           _("Error loading metadata-editor dialog."));
      g_propagate_error (error, local_error);

      g_free (ui_file);
      g_object_unref (builder);
      return FALSE;
    }

  g_free (ui_file);

  name = gimp_image_get_name (image);
  title = g_strdup_printf (_("Metadata Editor: %s"), name);
  if (name)
    g_free (name);

  dialog = gimp_dialog_new (title,
                            "gimp-metadata-editor-dialog",
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,
                            _("_Cancel"),         GTK_RESPONSE_CANCEL,
                            _("_Write Metadata"), GTK_RESPONSE_OK,
                            NULL);

  meta_args.dialog = dialog;

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  if (title)
    g_free (title);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  metadata_vbox = builder_get_widget (builder, "metadata-vbox");
  impex_combo   = builder_get_widget (builder, "impex_combo");

  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (impex_combo),
                                  _("Select:"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (impex_combo),
                                  _("Import metadata"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (impex_combo),
                                  _("Export metadata"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (impex_combo), 0);

  g_signal_connect (G_OBJECT (impex_combo),
                    "changed", G_CALLBACK (impex_combo_callback), &meta_args);

  gtk_container_set_border_width (GTK_CONTAINER (metadata_vbox), 12);
  gtk_box_pack_start (GTK_BOX (content_area), metadata_vbox, TRUE, TRUE, 0);

  metadata_dialog_editor_set_metadata (metadata, builder);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);
  if (run)
    {
      metadata_editor_write_callback (dialog, builder, image);
    }

  if (meta_args.filename)
    {
      g_free (meta_args.filename);
    }

  return TRUE;
}

/* ============================================================================
 * ==[                   ]=====================================================
 * ==[ PRIVATE FUNCTIONS ]=====================================================
 * ==[                   ]=====================================================
 * ============================================================================
 */
static void
remove_substring (const gchar *string,
                  const gchar *substring)
{
  if (string != NULL && substring != NULL && substring[0] != '\0')
    {
      gchar *p = strstr (string, substring);
      if (p)
        {
          memmove (p, p + strlen (substring), strlen (p + strlen (substring)) + 1);
        }
    }
}

static gchar *
clean_xmp_string (const gchar *value)
{
  gchar *value_clean;
  gchar *value_utf8;

  value_clean = g_strdup (value);

  if (strstr (value_clean, lang_default) != NULL)
    {
      remove_substring (value_clean, lang_default);
      if (strstr (value_clean, " ") != NULL)
        {
          remove_substring (value_clean, " ");
        }
    }

  if (strstr (value_clean, bag_default) != NULL)
    {
      remove_substring (value_clean, bag_default);
      if (strstr (value_clean, " ") != NULL)
        {
          remove_substring (value_clean, " ");
        }
    }

  if (strstr (value_clean, seq_default) != NULL)
    {
      remove_substring (value_clean, seq_default);
      if (strstr (value_clean, " ") != NULL)
        {
          remove_substring (value_clean, " ");
        }
    }

  if (! g_utf8_validate (value_clean, -1, NULL))
    {
      value_utf8 = g_locale_to_utf8 (value_clean, -1, NULL, NULL, NULL);
    }
  else
    {
      value_utf8 = g_strdup (value_clean);
    }

  g_free (value_clean);

  return value_utf8;
}

/* We split a string and accept "," and ";" as delimiters.
 * The result needs to be freed with g_strfreev.
 */
static gchar **
split_metadata_string (gchar *value)
{
  gchar  **split;
  gint     item;

  /* Can't use g_strsplit_set since we work with utf-8 here. */
  split = g_strsplit (g_strdelimit (value, ";", ','), ",", 0);

  for (item = 0; split[item]; item++)
    {
      split[item] = g_strstrip(split[item]);
    }

  return split;
}

static void
add_to_store (gchar *value, GtkListStore *liststore, gint store_column)
{
  gchar       **strings;
  gint          cnt = 0;
  gint          item;
  GtkTreeIter   iter;

  if (value)
    {
      strings = split_metadata_string (value);
      if (strings)
        {
          for (item = 0; strings[item]; item++)
            {
              if (strings[item][0] != '\0')
                {
                  cnt++;

                  gtk_list_store_append (liststore, &iter);
                  gtk_list_store_set (liststore, &iter,
                                      store_column, strings[item],
                                      -1);
                }
            }
          g_strfreev(strings);
        }
    }

  /* If there are less than two rows, add empty ones. */
  for (item = cnt; item < 2; item++)
    {
      gtk_list_store_append (liststore, &iter);
      gtk_list_store_set (liststore, &iter,
                          store_column, NULL,
                          -1);
    }
}

static gint
count_tags (GExiv2Metadata  *metadata,
            const gchar     *header,
            const gchar    **tags,
            gint             items)
{
  int   tagcount;
  gchar tag[256];
  int   row, col;

  tagcount = 0;
  for (row = 1; row < 256; row++)
    {
      for (col = 0; col < items; col++)
        {
          g_snprintf ((gchar *) &tag, 256, "%s[%d]", header, row);
          g_snprintf ((gchar *) &tag, 256, "%s%s",
                      (gchar *) &tag, (gchar *) tags[col]);
          if (gexiv2_metadata_has_tag (metadata, (gchar *) &tag))
            {
              tagcount++;
              break;
            }
        }
    }
  return tagcount;
}

static gchar **
get_tags (GExiv2Metadata  *metadata,
          const gchar     *header,
          const gchar    **tags,
          const gint       items,
          const gint       count)
{
  gchar **tagdata;
  gchar **_datarow;
  gchar   tag[256];
  int     row, col;

  g_return_val_if_fail (header != NULL && tags != NULL, NULL);
  g_return_val_if_fail (items > 0, NULL);

  if (count <= 0)
    return NULL;
  tagdata = g_new0 (gchar *, count);
  if (! tagdata)
    return NULL;

  for (row = 1; row < count + 1; row++)
    {
      tagdata[row-1] = g_malloc0 (sizeof (gchar *) * items);
      for (col = 0; col < items; col++)
        {
          gchar *value;

          g_snprintf ((gchar *) &tag, 256, "%s[%d]", header, row);
          g_snprintf ((gchar *) &tag, 256, "%s%s",
                      (gchar *) &tag, (gchar *) tags[col]);

          value = gexiv2_metadata_get_tag_string (metadata, (gchar *) &tag);
          g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "get_tags tag: %s, value: %s", (gchar *) &tag, value);

          _datarow = (gchar **) tagdata[row-1];
          if (_datarow)
            _datarow[col] = strdup (value);
        }
    }
  return tagdata;
}

static void
free_tagdata(gchar **tagdata, gint rows, gint cols)
{
  gint    row, col;
  gchar **tagdatarow;

  for (row = 0; row < rows; row++)
    {
      tagdatarow = (gpointer) tagdata[row];

      for (col = 0; col < cols; col++)
        {
          g_free (tagdatarow[col]);
        }
      g_free (tagdatarow);
    }
  g_free (tagdata);
}

/* ============================================================================
 * ==[ DATE CALLBACKS ]========================================================
 * ============================================================================
 */
void
on_create_date_button_clicked (GtkButton *widget,
                               gpointer   data)
{
  on_date_button_clicked (widget, (GtkWidget*)data,
                          "Xmp.photoshop.DateCreated");
}

void
on_patient_dob_date_button_clicked (GtkButton *widget,
                                    gpointer   data)
{
  on_date_button_clicked (widget, (GtkWidget*)data,
                          "Xmp.DICOM.PatientDOB");
}

void
on_study_date_button_clicked (GtkButton *widget,
                              gpointer   data)
{
  on_date_button_clicked (widget, (GtkWidget*)data,
                          "Xmp.DICOM.StudyDateTime");
}

void
on_series_date_button_clicked (GtkButton *widget,
                               gpointer   data)
{
  on_date_button_clicked (widget, (GtkWidget*)data,
                          "Xmp.DICOM.SeriesDateTime");
}

void
on_date_button_clicked (GtkButton *widget,
                        GtkWidget *entry_widget,
                        gchar     *tag)
{
  GtkBuilder     *builder;
  GtkWidget      *calendar_dialog;
  GtkWidget      *calendar_content_area;
  GtkWidget      *calendar_vbox;
  GtkWidget      *calendar;
  const gchar    *date_text;
  gchar          *ui_file;
  GError         *error = NULL;
  GDateTime      *current_datetime;
  guint           year, month, day;

  builder = gtk_builder_new ();

  ui_file = g_build_filename (gimp_data_directory (),
                              "ui", "plug-ins",
                              "plug-in-metadata-editor-calendar.ui", NULL);

  if (! gtk_builder_add_from_file (builder, ui_file, &error))
    {
      g_log ("", G_LOG_LEVEL_MESSAGE,
             _("Error loading calendar. %s"),
             error ? error->message : "");
      g_clear_error (&error);

      if (ui_file)
        g_free (ui_file);
      g_object_unref (builder);
      return;
    }

  if (ui_file)
    g_free (ui_file);

  date_text = gtk_entry_get_text (GTK_ENTRY (entry_widget));
  if (date_text && date_text[0] != '\0')
    {
      sscanf (date_text, "%d-%d-%d;", &year, &month, &day);
      month--;
    }
  else
    {
      current_datetime = g_date_time_new_now_local ();
      year = g_date_time_get_year (current_datetime);
      month = g_date_time_get_month (current_datetime) - 1;
      day = g_date_time_get_day_of_month (current_datetime);
    }

  calendar_dialog =
    gtk_dialog_new_with_buttons (_("Calendar Date:"),
                                 NULL,
                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                 _("_Cancel"),  GTK_RESPONSE_CANCEL,
                                 _("Set Date"), GTK_RESPONSE_OK,
                                 NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (calendar_dialog),
                                   GTK_RESPONSE_OK);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (calendar_dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gimp_window_set_transient (GTK_WINDOW (calendar_dialog));

  calendar_content_area = gtk_dialog_get_content_area (GTK_DIALOG (
                            calendar_dialog));

  calendar_vbox = builder_get_widget (builder, "calendar-vbox");

  gtk_container_set_border_width (GTK_CONTAINER (calendar_vbox), 12);
  gtk_box_pack_start (GTK_BOX (calendar_content_area), calendar_vbox, TRUE, TRUE,
                      0);

  calendar = builder_get_widget (builder, "calendar");

  gtk_calendar_select_month (GTK_CALENDAR (calendar), month, year);
  gtk_calendar_select_day (GTK_CALENDAR (calendar), day);
  gtk_calendar_mark_day (GTK_CALENDAR (calendar), day);

  if (gtk_dialog_run (GTK_DIALOG (calendar_dialog)) == GTK_RESPONSE_OK)
    {
      gchar date[25];
      gtk_calendar_get_date (GTK_CALENDAR (calendar), &year, &month, &day);
      g_sprintf ((gchar*) &date, "%d-%02d-%02d", year, month+1, day);
      gtk_entry_set_text (GTK_ENTRY (entry_widget), date);
    }

  gtk_widget_destroy (calendar_dialog);
}

/* ============================================================================
 * ==[ SPECIAL TAGS HANDLERS ]=================================================
 * ============================================================================
 */

gboolean
hasImageSupplierTagData (GtkBuilder *builder)
{
  gint loop;

  for (loop = 0; loop < imageSupplierInfoHeader.size; loop++)
    {
      GObject     *object;
      const gchar *text;

      object = gtk_builder_get_object (builder, default_metadata_tags[loop].tag);

      if (! strcmp (imageSupplierInfoTags[loop].mode, "single"))
        {
          text = gtk_entry_get_text (GTK_ENTRY (object));

          if (text && *text)
            return TRUE;
        }
      else if (! strcmp (imageSupplierInfoTags[loop].mode, "multi"))
        {
          text = gtk_entry_get_text (GTK_ENTRY (object));

          if (text && *text)
            return TRUE;
        }
    }

  return FALSE;
}

gboolean
hasLocationCreationTagData (GtkBuilder *builder)
{
  gint loop;

  for (loop = 0; loop < creatorContactInfoHeader.size; loop++)
    {
      GObject     *object;
      const gchar *text;

      object = gtk_builder_get_object (builder, default_metadata_tags[loop].tag);

      if (! strcmp (locationCreationInfoTags[loop].mode, "single"))
        {
          text = gtk_entry_get_text (GTK_ENTRY (object));

          if (text && *text)
            return TRUE;
        }
    }

  return FALSE;
}

gboolean
hasModelReleaseTagData (GtkBuilder *builder)
{
  return FALSE;
}

gboolean
hasPropertyReleaseTagData (GtkBuilder *builder)
{
  return FALSE;
}


gboolean
hasCreatorTagData (GtkBuilder *builder)
{
  gboolean has_data = FALSE;
  gint     loop;

  for (loop = 0; loop < creatorContactInfoHeader.size; loop++)
    {
      GObject *object;

      object = gtk_builder_get_object (builder, default_metadata_tags[loop].tag);

      if (GTK_IS_ENTRY (object))
        {
          const gchar *text = gtk_entry_get_text (GTK_ENTRY (object));

          if (text && *text)
            has_data = TRUE;
        }
      else if (GTK_IS_TEXT_VIEW (object))
        {
          GtkTextView   *text_view = GTK_TEXT_VIEW (object);
          GtkTextBuffer *buffer    = gtk_text_view_get_buffer (text_view);
          GtkTextIter    start;
          GtkTextIter    end;
          gchar         *text;

          gtk_text_buffer_get_start_iter (buffer, &start);
          gtk_text_buffer_get_end_iter (buffer, &end);

          text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

          if (text && *text)
            has_data = TRUE;

          if (text)
            g_free (text);
        }
    }

  return has_data;
}

/* ============================================================================
 * ==[ SET DIALOG METADATA ]===================================================
 * ============================================================================
 */

/* CELL EDITED */

void
cell_edited_callback (GtkCellRendererText *cell,
                      const gchar         *path_string,
                      const gchar         *new_text,
                      gpointer             data,
                      int                  index)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, index,
                      new_text, -1);
}

void
cell_edited_callback_combo (GtkCellRendererCombo *cell,
                            const gchar          *path_string,
                            const gchar          *new_text,
                            gpointer              data,
                            int                   column)
{
  GtkWidget        *widget;
  GtkTreeModel     *treemodel;
  GtkListStore     *liststore;
  GtkTreeIter       iter;
  GtkTreePath      *path;
  GtkTreeSelection *selection;

  widget = GTK_WIDGET (data);

  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
  liststore = GTK_LIST_STORE (treemodel);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

  if (gtk_tree_selection_get_selected (GTK_TREE_SELECTION (selection),
                                       NULL, &iter))
    {
      path = gtk_tree_model_get_path (treemodel, &iter);
      gtk_tree_path_free (path);
      gtk_list_store_set (liststore, &iter, column, new_text, -1);
    }
}

static void
licensor_name_cell_edited_callback (GtkCellRendererText *cell,
                                    const gchar         *path_string,
                                    const gchar         *new_text,
                                    gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
licensor_id_cell_edited_callback (GtkCellRendererText *cell,
                                  const gchar         *path_string,
                                  const gchar         *new_text,
                                  gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
licensor_phone1_cell_edited_callback (GtkCellRendererText *cell,
                                      const gchar         *path_string,
                                      const gchar         *new_text,
                                      gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 2);
}

static void
licensor_phone_type1_cell_edited_callback (GtkCellRendererCombo *cell,
                                           const gchar          *path_string,
                                           const gchar          *new_text,
                                           gpointer              data)
{
  cell_edited_callback_combo (cell, path_string, new_text, data, 3);
}

static void
licensor_phone2_cell_edited_callback (GtkCellRendererText *cell,
                                      const gchar         *path_string,
                                      const gchar         *new_text,
                                      gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 4);
}

static void
licensor_phone_type2_cell_edited_callback (GtkCellRendererCombo *cell,
                                           const gchar          *path_string,
                                           const gchar          *new_text,
                                           gpointer              data)
{
  cell_edited_callback_combo (cell, path_string, new_text, data, 5);
}

static void
licensor_email_cell_edited_callback (GtkCellRendererText *cell,
                                     const gchar         *path_string,
                                     const gchar         *new_text,
                                     gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 6);
}

static void
licensor_web_cell_edited_callback (GtkCellRendererText *cell,
                                   const gchar         *path_string,
                                   const gchar         *new_text,
                                   gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 7);
}

static void
cr_owner_name_cell_edited_callback (GtkCellRendererText *cell,
                                    const gchar         *path_string,
                                    const gchar         *new_text,
                                    gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
cr_owner_id_cell_edited_callback (GtkCellRendererText *cell,
                                  const gchar         *path_string,
                                  const gchar         *new_text,
                                  gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
img_cr8_name_cell_edited_callback (GtkCellRendererText *cell,
                                   const gchar         *path_string,
                                   const gchar         *new_text,
                                   gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
img_cr8_id_cell_edited_callback (GtkCellRendererText *cell,
                                 const gchar         *path_string,
                                 const gchar         *new_text,
                                 gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
aoo_copyright_notice_cell_edited_callback (GtkCellRendererText *cell,
                                           const gchar         *path_string,
                                           const gchar         *new_text,
                                           gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 5);
}

static void
aoo_source_inv_cell_edited_callback (GtkCellRendererText *cell,
                                     const gchar         *path_string,
                                     const gchar         *new_text,
                                     gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 4);
}

static void
aoo_source_cell_edited_callback (GtkCellRendererText *cell,
                                 const gchar         *path_string,
                                 const gchar         *new_text,
                                 gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 3);
}

static void
aoo_creator_cell_edited_callback (GtkCellRendererText *cell,
                                  const gchar         *path_string,
                                  const gchar         *new_text,
                                  gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 2);
}

static void
aoo_date_creat_cell_edited_callback (GtkCellRendererText *cell,
                                     const gchar         *path_string,
                                     const gchar         *new_text,
                                     gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
aoo_title_cell_edited_callback (GtkCellRendererText *cell,
                                const gchar         *path_string,
                                const gchar         *new_text,
                                gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
reg_org_id_cell_edited_callback (GtkCellRendererText *cell,
                                 const gchar         *path_string,
                                 const gchar         *new_text,
                                 gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
reg_item_id_cell_edited_callback (GtkCellRendererText *cell,
                                  const gchar         *path_string,
                                  const gchar         *new_text,
                                  gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
loc_sho_sub_loc_cell_edited_callback (GtkCellRendererText *cell,
                                      const gchar         *path_string,
                                      const gchar         *new_text,
                                      gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 0);
}

static void
loc_sho_city_cell_edited_callback (GtkCellRendererText *cell,
                                   const gchar         *path_string,
                                   const gchar         *new_text,
                                   gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 1);
}

static void
loc_sho_state_prov_cell_edited_callback (GtkCellRendererText *cell,
    const gchar         *path_string,
    const gchar         *new_text,
    gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 2);
}

static void
loc_sho_cntry_cell_edited_callback (GtkCellRendererText *cell,
                                    const gchar         *path_string,
                                    const gchar         *new_text,
                                    gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 3);
}

static void
loc_sho_cntry_iso_cell_edited_callback (GtkCellRendererText *cell,
                                        const gchar         *path_string,
                                        const gchar         *new_text,
                                        gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 4);
}

static void
loc_sho_wrld_reg_cell_edited_callback (GtkCellRendererText *cell,
                                       const gchar         *path_string,
                                       const gchar         *new_text,
                                       gpointer             data)
{
  cell_edited_callback (cell, path_string, new_text, data, 5);
}

static void
prop_rel_id_cell_edited_callback (GtkCellRendererText *cell,
                                  const gchar         *path_string,
                                  const gchar         *new_text,
                                  gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;
  gint          column;
  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                      new_text, -1);
}

static void
mod_rel_id_cell_edited_callback (GtkCellRendererText *cell,
                                 const gchar         *path_string,
                                 const gchar         *new_text,
                                 gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;
  gint          column;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                      new_text, -1);
}

static void
organisation_image_name_cell_edited_callback (GtkCellRendererText *cell,
                                              const gchar         *path_string,
                                              const gchar         *new_text,
                                              gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;
  gint          column;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                      new_text, -1);
}

static void
organisation_image_code_cell_edited_callback (GtkCellRendererText *cell,
                                              const gchar         *path_string,
                                              const gchar         *new_text,
                                              gpointer             data)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;
  gint          column;

  model = (GtkTreeModel *)data;
  path = gtk_tree_path_new_from_string (path_string);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,
                      new_text, -1);
}


/* CELL / ROW REMOVE */

static void
list_row_remove_callback (GtkWidget *widget,
                          gpointer   data,
                          gchar     *tag)
{
  GtkBuilder       *builder = data;
  GtkWidget        *list_widget;
  GtkListStore     *liststore;
  GtkTreeIter       iter;
  GtkTreeModel     *treemodel;
  GtkTreeSelection *selection;
  GtkTreePath      *path;

  list_widget = builder_get_widget (builder, tag);

  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));
  liststore = GTK_LIST_STORE (treemodel);

  selection = gtk_tree_view_get_selection ((GtkTreeView *)list_widget);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gint number_of_rows;

      path = gtk_tree_model_get_path (treemodel, &iter);
      gtk_list_store_remove (liststore, &iter);
      gtk_tree_path_free (path);

      number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);
      /* Make sur that two rows are always showing, else it looks ugly. */
      if (number_of_rows < 2)
        {
          gtk_list_store_append (liststore, &iter);
        }
    }
}

static void
property_release_id_remove_callback (GtkWidget *widget,
                                     gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.plus.PropertyReleaseID");
}

static void
model_release_id_remove_callback (GtkWidget *widget,
                                  gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.plus.ModelReleaseID");
}

static void
shown_location_remove_callback (GtkWidget *widget,
                                gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.iptcExt.LocationShown");
}

static void
feat_org_name_remove_callback (GtkWidget *widget,
                               gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.iptcExt.OrganisationInImageName");
}

static void
feat_org_code_remove_callback (GtkWidget *widget,
                               gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.iptcExt.OrganisationInImageCode");
}

static void
artwork_object_remove_callback (GtkWidget *widget,
                                gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.iptcExt.ArtworkOrObject");
}

static void
reg_entry_remove_callback (GtkWidget *widget,
                           gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.iptcExt.RegistryId");
}

static void
image_creator_remove_callback (GtkWidget *widget,
                               gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.plus.ImageCreator");
}

static void
copyright_own_remove_callback (GtkWidget *widget,
                               gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.plus.CopyrightOwner");
}

static void
licensor_remove_callback (GtkWidget *widget,
                          gpointer   data)
{
  list_row_remove_callback (widget, data, "Xmp.plus.Licensor");
}


/* CELL / ROW ADD */

static void
list_row_add_callback (GtkWidget *widget,
                       gpointer   data,
                       gchar     *tag)
{
  GtkBuilder    *builder = data;
  GtkWidget     *list_widget;
  GtkListStore  *liststore;
  GtkTreeIter    iter;

  list_widget = builder_get_widget (builder, tag);

  liststore = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget)));

  gtk_list_store_append (liststore, &iter);
}

static void
property_release_id_add_callback (GtkWidget *widget,
                                  gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.plus.PropertyReleaseID");
}

static void
model_release_id_add_callback (GtkWidget *widget,
                               gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.plus.ModelReleaseID");
}

static void
shown_location_add_callback (GtkWidget *widget,
                             gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.iptcExt.LocationShown");
}

static void
feat_org_name_add_callback (GtkWidget *widget,
                            gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.iptcExt.OrganisationInImageName");
}

static void
feat_org_code_add_callback (GtkWidget *widget,
                            gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.iptcExt.OrganisationInImageCode");
}

static void
artwork_object_add_callback (GtkWidget *widget,
                             gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.iptcExt.ArtworkOrObject");
}

static void
reg_entry_add_callback (GtkWidget *widget,
                        gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.iptcExt.RegistryId");
}

static void
image_creator_add_callback (GtkWidget *widget,
                            gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.plus.ImageCreator");
}

static void
copyright_own_add_callback (GtkWidget *widget,
                            gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.plus.CopyrightOwner");
}

static void
licensor_add_callback (GtkWidget *widget,
                       gpointer   data)
{
  list_row_add_callback (widget, data, "Xmp.plus.Licensor");
}

const gchar *gpstooltips[] =
{
    N_ ("Enter or edit GPS value here.\n"
        "Valid values consist of 1, 2 or 3 numbers "
        "(degrees, minutes, seconds), see the following examples:\n"
        "10deg 15' 20\", or 10\u00b0 15' 20\", or 10:15:20.45, or "
        "10 15 20, or 10 15.30, or 10.45\n"
        "Delete all text to remove the current value."),
    N_ ("Enter or edit GPS altitude value here.\n"
        "A valid value consists of one number:\n"
        "e.g. 100, or 12.24\n"
        "Depending on the selected measurement type "
        "the value should be entered in meter (m) "
        "or feet (ft)\n"
        "Delete all text to remove the current value.")
};

enum
{
  GPS_LONG_LAT_TOOLTIP,
  GPS_ALTITUDE_TOOLTIP,
};

/* Set dialog display settings and data */

static void
metadata_dialog_editor_set_metadata (GExiv2Metadata *metadata,
                                     GtkBuilder     *builder)
{
  GtkWidget *combo_widget;
  GtkWidget *entry_widget;
  GtkWidget *text_widget;
  GtkWidget *button_widget;
  gint       width, height;
  gchar     *value;
  gint       i;

  gint32 numele = n_default_metadata_tags;

  /* Setup Buttons */
  button_widget = builder_get_widget (builder, "add_licensor_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (licensor_add_callback),
                    builder);

  button_widget = builder_get_widget (builder, "rem_licensor_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (licensor_remove_callback),
                    builder);

  button_widget = builder_get_widget (builder, "add_copyright_own_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (copyright_own_add_callback),
                    builder);

  button_widget = builder_get_widget (builder, "rem_copyright_own_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (copyright_own_remove_callback),
                    builder);

  button_widget = builder_get_widget (builder, "add_image_creator_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (image_creator_add_callback),
                    builder);

  button_widget = builder_get_widget (builder, "rem_image_creator_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (image_creator_remove_callback),
                    builder);

  button_widget = builder_get_widget (builder, "add_reg_entry_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (reg_entry_add_callback),
                    builder);

  button_widget = builder_get_widget (builder, "rem_reg_entry_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (reg_entry_remove_callback),
                    builder);

  button_widget = builder_get_widget (builder, "add_artwork_object_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (artwork_object_add_callback),
                    builder);

  button_widget = builder_get_widget (builder, "rem_artwork_object_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (artwork_object_remove_callback),
                    builder);

  button_widget = builder_get_widget (builder, "add_feat_org_code_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (feat_org_code_add_callback),
                    builder);

  button_widget = builder_get_widget (builder, "rem_feat_org_code_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (feat_org_code_remove_callback),
                    builder);

  button_widget = builder_get_widget (builder, "add_feat_org_name_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (feat_org_name_add_callback),
                    builder);

  button_widget = builder_get_widget (builder, "rem_feat_org_name_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (feat_org_name_remove_callback),
                    builder);

  button_widget = builder_get_widget (builder, "add_shown_location_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (shown_location_add_callback),
                    builder);

  button_widget = builder_get_widget (builder, "rem_shown_location_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (shown_location_remove_callback),
                    builder);

  button_widget = builder_get_widget (builder, "add_model_rel_id_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (model_release_id_add_callback),
                    builder);

  button_widget = builder_get_widget (builder, "rem_model_rel_id_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (model_release_id_remove_callback),
                    builder);

  button_widget = builder_get_widget (builder, "add_prop_rel_id_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (property_release_id_add_callback),
                    builder);

  button_widget = builder_get_widget (builder, "rem_prop_rel_id_button");
  g_signal_connect (G_OBJECT (button_widget), "clicked",
                    G_CALLBACK (property_release_id_remove_callback),
                    builder);


  /* Setup Comboboxes */
  combo_widget = builder_get_widget (builder, "Xmp.xmp.Rating");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                  _("Unrated"));
  for (i = 1; i < 6; i++)
    {
      gchar *display = g_strdup_printf ("%d", i);

      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      display);
      g_free (display);
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = builder_get_widget (builder, "Xmp.xmpRights.Marked");
  for (i = 0; i < 3; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (marked[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = builder_get_widget (builder, "Xmp.photoshop.Urgency");
  for (i = 0; i < 9; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (urgency[i]));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = builder_get_widget (builder, "Xmp.plus.MinorModelAgeDisclosure");
  for (i = 0; i < 13; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (minormodelagedisclosure[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = builder_get_widget (builder, "Xmp.plus.ModelReleaseStatus");
  for (i = 0; i < 4; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (modelreleasestatus[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);
  gtk_widget_get_size_request (combo_widget, &width, &height);
  gtk_widget_set_size_request (combo_widget, 180, height);

  combo_widget = builder_get_widget (builder, "Xmp.iptcExt.DigitalSourceType");
  for (i = 0; i < 5; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (digitalsourcetype[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = builder_get_widget (builder, "Xmp.plus.PropertyReleaseStatus");
  for (i = 0; i < 4; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (propertyreleasestatus[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);
  gtk_widget_get_size_request (combo_widget, &width, &height);
  gtk_widget_set_size_request (combo_widget, 180, height);

  combo_widget = builder_get_widget (builder, "Xmp.DICOM.PatientSex");
  for (i = 0; i < 4; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (dicom[i].display));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = builder_get_widget (builder, "Exif.GPSInfo.GPSLatitudeRef");
  for (i = 0; i < 3; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (gpslatref[i]));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = builder_get_widget (builder, "Exif.GPSInfo.GPSLongitudeRef");
  for (i = 0; i < 3; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (gpslngref[i]));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = builder_get_widget (builder, "Exif.GPSInfo.GPSAltitudeRef");
  for (i = 0; i < 3; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (gpsaltref[i]));
    }
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  combo_widget = builder_get_widget (builder, "GPSAltitudeSystem");
  for (i = 0; i < 2; i++)
    {
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_widget),
                                      gettext (gpsaltsys[i]));
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_widget), 0);

  g_signal_connect (G_OBJECT (combo_widget), "changed",
                    G_CALLBACK (gpsaltsys_combo_callback),
                    builder);

  /* Set up text view heights */
  for (i = 0; i < numele; i++)
    {
      if (! strcmp ("multi", default_metadata_tags[i].mode))
        {
          text_widget = builder_get_widget (builder,
                                            default_metadata_tags[i].tag);
          gtk_widget_get_size_request (text_widget, &width, &height);
          gtk_widget_set_size_request (text_widget, width, height + 60);
        }
    }

  for (i = 0; i < creatorContactInfoHeader.size; i++)
    {
      if (! strcmp ("multi", creatorContactInfoTags[i].mode))
        {
          text_widget = builder_get_widget (builder,
                                            creatorContactInfoTags[i].id);
          gtk_widget_get_size_request (text_widget, &width, &height);
          gtk_widget_set_size_request (text_widget, width, height + 60);
        }
    }

  /* Set up lists */
  for (i = 0; i < imageSupplierInfoHeader.size; i++)
    {
      GtkWidget *widget;

      widget = builder_get_widget (builder,
                                   imageSupplierInfoTags[i].id);

      value = gexiv2_metadata_get_tag_interpreted_string (metadata,
                                                          imageSupplierInfoTags[i].tag);

      if (value)
        {
          gchar *value_utf;

          value_utf = clean_xmp_string (value);

          if (! strcmp ("single", imageSupplierInfoTags[i].mode))
            {
              gtk_entry_set_text (GTK_ENTRY (widget), value_utf);
            }
          else if (! strcmp ("multi", imageSupplierInfoTags[i].mode))
            {
              GtkTextBuffer *buffer;

              buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
              gtk_text_buffer_set_text (buffer, value_utf, -1);
            }
        }
    }

  for (i = 0; i < locationCreationInfoHeader.size; i++)
    {
      GtkWidget *widget;

      widget = builder_get_widget (builder,
                                   locationCreationInfoTags[i].id);

      value = gexiv2_metadata_get_tag_interpreted_string (metadata,
                                                          locationCreationInfoTags[i].tag);

      if (value)
        {
          gchar *value_utf;

          value_utf = clean_xmp_string (value);

          if (! strcmp ("single", locationCreationInfoTags[i].mode))
            {
              gtk_entry_set_text (GTK_ENTRY (widget), value_utf);
            }
        }
    }

  /* Set up tag data */

  for (i = 0; i < creatorContactInfoHeader.size; i++)
    {
      GtkWidget *widget;

      widget = builder_get_widget (builder, creatorContactInfoTags[i].id);

      value = gexiv2_metadata_get_tag_interpreted_string (metadata,
                                                          creatorContactInfoTags[i].tag);

      if (value)
        {
          gchar *value_utf;

          value_utf = clean_xmp_string (value);

          if (! strcmp ("single", creatorContactInfoTags[i].mode))
            {
              gtk_entry_set_text (GTK_ENTRY (widget), value_utf);
            }
          else if (! strcmp ("multi", creatorContactInfoTags[i].mode))
            {
              GtkTextBuffer *buffer;
              buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
              gtk_text_buffer_set_text (buffer, value_utf, -1);
            }
        }
    }

  for (i = 0; i < numele; i++)
    {
      GtkWidget *widget;
      gint       index;

      widget = builder_get_widget (builder, default_metadata_tags[i].tag);

      if (! strcmp ("Exif.GPSInfo.GPSLongitude",
                    default_metadata_tags[i].tag))
        {
          gdouble  gps_value;
          gchar   *str;

          if (gexiv2_metadata_get_gps_longitude (metadata, &gps_value))
            {
              str = metadata_format_gps_longitude_latitude (gps_value);
              gtk_entry_set_text (GTK_ENTRY (widget), str);
              g_free (str);
            }
          gtk_widget_set_tooltip_text (widget,
                                       gettext (gpstooltips[GPS_LONG_LAT_TOOLTIP]));
          continue;
        }
      else if (! strcmp ("Exif.GPSInfo.GPSLatitude",
                         default_metadata_tags[i].tag))
        {
          gdouble  gps_value;
          gchar   *str;

          if (gexiv2_metadata_get_gps_latitude (metadata, &gps_value))
            {
              str = metadata_format_gps_longitude_latitude (gps_value);
              gtk_entry_set_text (GTK_ENTRY (widget), str);
              g_free (str);
            }
          gtk_widget_set_tooltip_text (widget,
                                       gettext (gpstooltips[GPS_LONG_LAT_TOOLTIP]));
          continue;
        }
      else if (! strcmp ("Exif.GPSInfo.GPSAltitude",
                        default_metadata_tags[i].tag))
        {
          gdouble  gps_value;
          gchar   *str;

          if (gexiv2_metadata_get_gps_altitude (metadata, &gps_value))
            {
              str = metadata_format_gps_altitude (gps_value, TRUE, "");
              gtk_entry_set_text (GTK_ENTRY (widget), str);
              g_free (str);
            }
          gtk_widget_set_tooltip_text (widget,
                                       gettext (gpstooltips[GPS_ALTITUDE_TOOLTIP]));
          continue;
        }

      value = gexiv2_metadata_get_tag_interpreted_string (metadata,
                                                          default_metadata_tags[i].tag);

      if (value)
        {
          gchar *value_utf8 = clean_xmp_string (value);

          g_free (value);

          if (value_utf8 && value_utf8[0] != '\0')
            {
              value = g_strdup (value_utf8);
            }
          else
            {
              value = NULL;
            }

          g_free (value_utf8);
        }

      index = default_metadata_tags[i].other_tag_index;
      if (index > -1)
        {
          gchar **values;

          /* These are all IPTC tags some of which can appear multiple times so
            * we will use get_tag_multiple. Also IPTC most commonly uses UTF-8
            * not current locale so get_tag_interpreted was wrong anyway.
            * FIXME For now lets interpret as UTF-8 and in the future read
            * and interpret based on the CharacterSet tag.
            */
          values = gexiv2_metadata_get_tag_multiple (metadata,
                                                     equivalent_metadata_tags[index].tag);

          if (values)
            {
              gint     i;
              GString *str = NULL;

              for (i = 0; values[i] != NULL; i++)
                {
                  if (values[i][0] != '\0')
                    {
                      if (! str)
                        {
                          str = g_string_new (values[i]);
                        }
                      else
                        {
                          if (! strcmp ("multi", equivalent_metadata_tags[index].mode))
                            {
                              g_string_append (str, "\n");
                            }
                          else
                            {
                              g_string_append (str, ", ");
                            }
                          g_string_append (str, values[i]);
                        }
                    }
                }

              if (str)
                {
                  /* If we got values from both Xmp and Iptc then compare those
                    * values and if they are different concatenate them. Usually they
                    * should be the same in which case we won't duplicate the string.
                    */
                  if (value && strcmp (value, str->str))
                    {
                      if (! strcmp ("multi", equivalent_metadata_tags[index].mode))
                        {
                          g_string_prepend (str, "\n");
                        }
                      else
                        {
                          g_string_prepend (str, ", ");
                        }
                      g_string_prepend (str, value);
                      g_free (value);
                    }
                  value = g_string_free (str, FALSE);
                }
            }
        }

      if (!strcmp ("list", default_metadata_tags[i].mode))
        {
          /* Tab: IPTC Extension, Label: Location Shown */
          if (! strcmp ("Xmp.iptcExt.LocationShown",
                        default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;
              GtkTreeIter        iter;
              gint               counter;
              gchar            **tagdata;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* LOCATION SHOWN - SUB LOCATION */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_SUB_LOC);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_sub_loc_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_SUB_LOC));
                }

              /* LOCATION SHOWN - CITY */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_CITY);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_city_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_CITY));
                }

              /* LOCATION SHOWN - STATE PROVINCE */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_STATE_PROV);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_state_prov_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_STATE_PROV));
                }

              /* LOCATION SHOWN - COUNTRY */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_CNTRY);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_cntry_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_CNTRY));
                }

              /* LOCATION SHOWN - COUNTRY ISO */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_CNTRY_ISO);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_cntry_iso_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_CNTRY_ISO));
                }

              /* LOCATION SHOWN - WORLD REGION */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LOC_SHO_CNTRY_WRLD_REG);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (loc_sho_wrld_reg_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LOC_SHO_CNTRY_WRLD_REG));
                }

              counter = count_tags (metadata, LOCATIONSHOWN_HEADER,
                                    locationshown,
                                    n_locationshown);

              tagdata = get_tags (metadata, LOCATIONSHOWN_HEADER,
                                  locationshown,
                                  n_locationshown, counter);

              if (counter > 0 && tagdata)
                {
                  gint item;

                  for (item = 0; item < counter; item++)
                    {
                      gchar **tagdatarow = (gchar **) tagdata[item];

                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_LOC_SHO_SUB_LOC,        tagdatarow[0],
                                          COL_LOC_SHO_CITY,           tagdatarow[1],
                                          COL_LOC_SHO_STATE_PROV,     tagdatarow[2],
                                          COL_LOC_SHO_CNTRY,          tagdatarow[3],
                                          COL_LOC_SHO_CNTRY_ISO,      tagdatarow[4],
                                          COL_LOC_SHO_CNTRY_WRLD_REG, tagdatarow[5],
                                          -1);
                    }
                  free_tagdata(tagdata, counter, n_locationshown);

                  if (counter == 1)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_LOC_SHO_SUB_LOC,        NULL,
                                          COL_LOC_SHO_CITY,           NULL,
                                          COL_LOC_SHO_STATE_PROV,     NULL,
                                          COL_LOC_SHO_CNTRY,          NULL,
                                          COL_LOC_SHO_CNTRY_ISO,      NULL,
                                          COL_LOC_SHO_CNTRY_WRLD_REG, NULL,
                                          -1);
                    }
                }
              else
                {
                  gint item;

                  for (item = 0; item < 2; item++)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_LOC_SHO_SUB_LOC,        NULL,
                                          COL_LOC_SHO_CITY,           NULL,
                                          COL_LOC_SHO_STATE_PROV,     NULL,
                                          COL_LOC_SHO_CNTRY,          NULL,
                                          COL_LOC_SHO_CNTRY_ISO,      NULL,
                                          COL_LOC_SHO_CNTRY_WRLD_REG, NULL,
                                          -1);
                    }
                }
            }
          /* Tab: IPTC Extension, Label: Featured Organization - Name */
          else if (! strcmp ("Xmp.iptcExt.OrganisationInImageName",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)),
                                           GTK_SELECTION_SINGLE);

              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget), 0);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (organisation_image_name_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_ORG_IMG_NAME));
                }

              add_to_store (value, liststore, COL_ORG_IMG_NAME);
            }
          /* Tab: IPTC Extension, Label: Featured Organization - Code */
          else if (! strcmp ("Xmp.iptcExt.OrganisationInImageCode",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)),
                                           GTK_SELECTION_SINGLE);

              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget), 0);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (organisation_image_code_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_ORG_IMG_CODE));
                }

              add_to_store (value, liststore, COL_ORG_IMG_CODE);
            }
          /* Tab: IPTC Extension, Label: Artwork or Object */
          else if (! strcmp ("Xmp.iptcExt.ArtworkOrObject",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;
              GtkTreeIter        iter;
              gint               counter;
              gchar            **tagdata;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* ARTWORK OR OBJECT - TITLE */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_TITLE);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_title_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_TITLE));
                }

              /* ARTWORK OR OBJECT - DATE CREATED */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_DATE_CREAT);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r != NULL; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_date_creat_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_DATE_CREAT));
                }

              /* ARTWORK OR OBJECT - CREATOR */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_CREATOR);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r != NULL; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_creator_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_CREATOR));
                }

              /* ARTWORK OR OBJECT - SOURCE */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_SOURCE);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_source_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_SOURCE));
                }

              /* ARTWORK OR OBJECT - SOURCE INVENTORY ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_SRC_INV_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_source_inv_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_SRC_INV_ID));
                }

              /* ARTWORK OR OBJECT - COPYRIGHT NOTICE */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_AOO_CR_NOT);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (aoo_copyright_notice_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_AOO_CR_NOT));
                }

              counter = count_tags (metadata, ARTWORKOROBJECT_HEADER,
                                    artworkorobject,
                                    n_artworkorobject);

              tagdata = get_tags (metadata, ARTWORKOROBJECT_HEADER,
                                  artworkorobject,
                                  n_artworkorobject, counter);


              if (counter > 0 && tagdata)
                {
                  gint item;

                  for (item = 0; item < counter; item++)
                    {
                      gchar **tagdatarow = (gchar **) tagdata[item];

                      /* remove substring for language id in title field */
                      remove_substring (tagdatarow[4], lang_default);
                      if (strstr (tagdatarow[4], " "))
                        {
                          remove_substring (tagdatarow[4], " ");
                        }

                      remove_substring (tagdatarow[4], bag_default);
                      if (strstr (tagdatarow[4], " "))
                        {
                          remove_substring (tagdatarow[4], " ");
                        }

                      remove_substring (tagdatarow[4], seq_default);
                      if (strstr (tagdatarow[4], " "))
                        {
                          remove_substring (tagdatarow[4], " ");
                        }

                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_AOO_TITLE,      tagdatarow[4],
                                          COL_AOO_DATE_CREAT, tagdatarow[0],
                                          COL_AOO_CREATOR,    tagdatarow[5],
                                          COL_AOO_SOURCE,     tagdatarow[1],
                                          COL_AOO_SRC_INV_ID, tagdatarow[2],
                                          COL_AOO_CR_NOT,     tagdatarow[3],
                                          -1);
                    }
                  free_tagdata(tagdata, counter, n_artworkorobject);
                }
              else
                {
                  gint item;

                  for (item = 0; item < 2; item++)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_AOO_TITLE,      NULL,
                                          COL_AOO_DATE_CREAT, NULL,
                                          COL_AOO_CREATOR,    NULL,
                                          COL_AOO_SOURCE,     NULL,
                                          COL_AOO_SRC_INV_ID, NULL,
                                          COL_AOO_CR_NOT,     NULL,
                                          -1);
                    }
                }
            }
          /* Tab: IPTC Extension, Label: Model Release Identifier */
          else if (! strcmp ("Xmp.plus.ModelReleaseID",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)),
                                           GTK_SELECTION_SINGLE);

              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget), 0);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (mod_rel_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_PROP_REL_ID));
                }

              add_to_store (value, liststore, COL_MOD_REL_ID);
            }
          /* Tab: IPTC Extension, Label: Registry Entry */
          else if (! strcmp ("Xmp.iptcExt.RegistryId",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;
              GtkTreeIter        iter;
              gint               counter;
              gchar            **tagdata;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* REGISTRY - ORGANIZATION ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_REGISTRY_ORG_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r != NULL; r = r->next)
                {
                  renderer = (GtkCellRenderer*) r->data;
                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);
                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (reg_org_id_cell_edited_callback),
                                    treemodel);
                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_REGISTRY_ORG_ID));
                }

              /* REGISTRY - ITEM ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_REGISTRY_ITEM_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (reg_item_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_REGISTRY_ITEM_ID));
                }

              counter = count_tags (metadata, REGISTRYID_HEADER,
                                    registryid,
                                    n_registryid);

              tagdata = get_tags (metadata, REGISTRYID_HEADER,
                                  registryid,
                                  n_registryid, counter);

              if (counter > 0 && tagdata)
                {
                  gint item;

                  for (item = 0; item < counter; item++)
                    {
                      gchar **tagdatarow = (gchar **) tagdata[item];

                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_REGISTRY_ORG_ID,  tagdatarow[0],
                                          COL_REGISTRY_ITEM_ID, tagdatarow[1],
                                          -1);
                    }
                  free_tagdata(tagdata, counter, n_registryid);
                }
              else
                {
                  gint item;

                  for (item = 0; item < 2; item++)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_REGISTRY_ORG_ID,  NULL,
                                          COL_REGISTRY_ITEM_ID, NULL,
                                          -1);
                    }
                }
            }
          /* Tab: IPTC Extension, Label: Image Creator */
          else if (! strcmp ("Xmp.plus.ImageCreator",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;
              GtkTreeIter        iter;
              gint               counter;
              gchar            **tagdata;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* IMAGE CREATOR - NAME */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_IMG_CR8_NAME);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (img_cr8_name_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_IMG_CR8_NAME));
                }

              /* IMAGE CREATOR - ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_IMG_CR8_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (img_cr8_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_IMG_CR8_ID));
                }

                counter = count_tags (metadata, IMAGECREATOR_HEADER,
                                      imagecreator,
                                      n_imagecreator);

                tagdata = get_tags (metadata, IMAGECREATOR_HEADER,
                                    imagecreator,
                                    n_imagecreator, counter);

                if (counter > 0 && tagdata)
                  {
                    gint item;

                    for (item = 0; item < counter; item++)
                      {
                        gchar **tagdatarow = (gchar **) tagdata[item];

                        gtk_list_store_append (liststore, &iter);
                        gtk_list_store_set (liststore, &iter,
                                            COL_IMG_CR8_NAME, tagdatarow[0],
                                            COL_IMG_CR8_ID,   tagdatarow[1],
                                            -1);
                      }
                    free_tagdata(tagdata, counter, n_imagecreator);
                  }
                else
                  {
                    gint item;

                    for (item = 0; item < 2; item++)
                      {
                        gtk_list_store_append (liststore, &iter);
                        gtk_list_store_set (liststore, &iter,
                                            COL_IMG_CR8_NAME, NULL,
                                            COL_IMG_CR8_ID,   NULL,
                                            -1);
                      }
                  }
            }
          /* Tab: IPTC Extension, Label: Copyright Owner */
          else if (! strcmp ("Xmp.plus.CopyrightOwner",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;
              GtkTreeIter        iter;
              gint               counter;
              gchar            **tagdata;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* COPYRIGHT OWNER - NAME */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_CR_OWNER_NAME);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (cr_owner_name_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_CR_OWNER_NAME));
                }

              /* COPYRIGHT OWNER - ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_CR_OWNER_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (cr_owner_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_CR_OWNER_ID));
                }

              counter = count_tags (metadata, COPYRIGHTOWNER_HEADER,
                                    copyrightowner,
                                    n_copyrightowner);

              tagdata = get_tags (metadata, COPYRIGHTOWNER_HEADER,
                                  copyrightowner,
                                  n_copyrightowner, counter);

              if (counter > 0 && tagdata)
                {
                  gint item;

                  for (item = 0; item < counter; item++)
                    {
                      gchar **tagdatarow = (gchar **) tagdata[item];

                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_CR_OWNER_NAME, tagdatarow[0],
                                          COL_CR_OWNER_ID,   tagdatarow[1],
                                          -1);
                    }
                  free_tagdata(tagdata, counter, n_copyrightowner);
                }
              else
                {
                  gint item;

                  for (item = 0; item < 2; item++)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_CR_OWNER_NAME, NULL,
                                          COL_CR_OWNER_ID,   NULL,
                                          -1);
                    }
                }
            }
          /* Tab: IPTC Extension, Label: Licensor */
          else if (! strcmp ("Xmp.plus.Licensor",
                             default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkTreeModel      *phonemodel;
              GtkListStore      *liststore;
              GtkListStore      *phonestore;
              GtkTreeIter        iter;
              GtkTreeIter        phoneiter;
              gint               counter;
              gint               j;
              gchar            **tagdata;

              phonestore = gtk_list_store_new (1, G_TYPE_STRING);
              gtk_list_store_append (phonestore, &phoneiter);
              gtk_list_store_set (phonestore, &phoneiter, 0, "Unknown", -1);
              for (j=1; j < 6; j++)
                {
                  gtk_list_store_append (phonestore, &phoneiter);
                  gtk_list_store_set (phonestore, &phoneiter,
                                      0, gettext (phone_types[j].display),
                                      -1);
                }
              phonemodel = GTK_TREE_MODEL (phonestore);

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              /* LICENSOR - NAME */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_NAME);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_name_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_NAME));
                }

              /* LICENSOR - ID */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_ID);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_ID));
                }

              /* LICENSOR - PHONE NUMBER 1 */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_PHONE1);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_phone1_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_PHONE1));
                }

              /* LICENSOR - PHONE TYPE 1 */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_PHONE_TYPE1);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable",    TRUE,
                                "text-column", 0,
                                "has-entry",   FALSE,
                                "model",       phonemodel,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_phone_type1_cell_edited_callback),
                                    widget);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_PHONE_TYPE1));
                }

              /* LICENSOR - PHONE NUMBER 2 */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_PHONE2);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_phone2_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_PHONE2));
                }

              /* LICENSOR - PHONE TYPE 2 */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_PHONE_TYPE2);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable",    TRUE,
                                "text-column", 0,
                                "has-entry",   FALSE,
                                "model",       phonemodel,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_phone_type2_cell_edited_callback),
                                    widget);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_PHONE_TYPE2));
                }

              /* LICENSOR - EMAIL */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_EMAIL);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_email_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_EMAIL));
                }

              /* LICENSOR - WEB ADDRESS */
              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget),
                                                 COL_LICENSOR_WEB);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (licensor_web_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_LICENSOR_WEB));
                }

              counter = count_tags (metadata, LICENSOR_HEADER,
                                    licensor,
                                    n_licensor);

              tagdata = get_tags (metadata, LICENSOR_HEADER,
                                  licensor,
                                  n_licensor, counter);

              if (counter > 0 && tagdata)
                {
                  gint item;

                  for (item = 0; item < counter; item++)
                    {
                      gchar **tagdatarow = (gchar **) tagdata[item];
                      gchar   *type1;
                      gchar   *type2;
                      gint    types;

                      type1 = g_strdup (gettext (phone_types[0].display));
                      type2 = g_strdup (gettext (phone_types[0].display));

                      for (types = 0; types < 6; types++)
                        {
                          /* phone type 1 */
                          if (tagdatarow[3] &&
                              ! strcmp (tagdatarow[3],
                                        phone_types[types].data))
                            {
                              g_free (type1);
                              type1 = g_strdup (gettext (phone_types[types].display));
                            }

                          /* phone type 2 */
                          if (tagdatarow[5] &&
                              ! strcmp (tagdatarow[5],
                                        phone_types[types].data))
                            {
                              g_free (type2);
                              type2 = g_strdup (gettext (phone_types[types].display));
                            }
                        }

                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_LICENSOR_NAME,        tagdatarow[0],
                                          COL_LICENSOR_ID,          tagdatarow[1],
                                          COL_LICENSOR_PHONE1,      tagdatarow[2],
                                          COL_LICENSOR_PHONE_TYPE1, type1,
                                          COL_LICENSOR_PHONE2,      tagdatarow[4],
                                          COL_LICENSOR_PHONE_TYPE2, type2,
                                          COL_LICENSOR_EMAIL,       tagdatarow[6],
                                          COL_LICENSOR_WEB,         tagdatarow[7],
                                          -1);
                      g_free (type1);
                      g_free (type2);
                    }
                  free_tagdata(tagdata, counter, n_licensor);
                }
              else
                {
                  gint item;

                  for (item = 0; item < 2; item++)
                    {
                      gtk_list_store_append (liststore, &iter);
                      gtk_list_store_set (liststore, &iter,
                                          COL_LICENSOR_NAME,        NULL,
                                          COL_LICENSOR_ID,          NULL,
                                          COL_LICENSOR_PHONE1,      NULL,
                                          COL_LICENSOR_PHONE_TYPE1, gettext (phone_types[0].display),
                                          COL_LICENSOR_PHONE2,      NULL,
                                          COL_LICENSOR_PHONE_TYPE1, gettext (phone_types[0].display),
                                          COL_LICENSOR_EMAIL,       NULL,
                                          COL_LICENSOR_WEB,         NULL,
                                          -1);
                    }
                }
            }
          /* Tab: IPTC Extension, Label: Property Release Identifier */
          else if (! strcmp ("Xmp.plus.PropertyReleaseID",
                              default_metadata_tags[i].tag))
            {
              GList             *rlist;
              GList             *r;
              GtkTreeViewColumn *column;
              GtkCellRenderer   *renderer;
              GtkTreeModel      *treemodel;
              GtkListStore      *liststore;

              treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
              liststore = GTK_LIST_STORE (treemodel);

              gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)),
                                           GTK_SELECTION_SINGLE);

              column = gtk_tree_view_get_column (GTK_TREE_VIEW (widget), 0);
              rlist = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
              for (r = rlist; r; r = r->next)
                {
                  renderer = r->data;

                  g_object_set (renderer,
                                "editable", TRUE,
                                NULL);

                  g_signal_connect (renderer, "edited",
                                    G_CALLBACK (prop_rel_id_cell_edited_callback),
                                    treemodel);

                  g_object_set_data (G_OBJECT (renderer),
                                     "column",
                                     GINT_TO_POINTER (COL_PROP_REL_ID));
                }

              add_to_store (value, liststore, COL_PROP_REL_ID);
            }
        }

      if (value)
        {
          if (! strcmp ("single", default_metadata_tags[i].mode))
            {
              gtk_entry_set_text (GTK_ENTRY (widget), value);
            }
          else if (! strcmp ("multi", default_metadata_tags[i].mode))
            {
              GtkTextBuffer *buffer;

              buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
              gtk_text_buffer_set_text (buffer, value, -1);
            }
          else if (! strcmp ("combo", default_metadata_tags[i].mode))
            {
              gint32 data = 0;

              if (! strcmp ("Exif.GPSInfo.GPSLatitudeRef",
                            default_metadata_tags[i].tag))
                {
                  if (! strncmp ("N", value, 1))
                    {
                      data = 1;
                    }
                  else if (! strncmp ("S", value, 1))
                    {
                      data = 2;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Exif.GPSInfo.GPSLongitudeRef",
                                 default_metadata_tags[i].tag))
                {
                  if (! strncmp ("E", value, 1))
                    {
                      data = 1;
                    }
                  else if (! strncmp ("W", value, 1))
                    {
                      data = 2;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Exif.GPSInfo.GPSAltitudeRef",
                                 default_metadata_tags[i].tag))
                {
                  if (! strncmp ("A", value, 1))
                    {
                      data = 1;
                    }
                  else if (! strncmp ("B", value, 1))
                    {
                      data = 2;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Xmp.xmp.Rating", default_metadata_tags[i].tag))
                {
                  if (! strcmp ("1", value))
                    {
                      data = 1;
                    }
                  else if (! strcmp ("2", value))
                    {
                      data = 2;
                    }
                  else if (! strcmp ("3", value))
                    {
                      data = 3;
                    }
                  else if (! strcmp ("4", value))
                    {
                      data = 4;
                    }
                  else if (! strcmp ("5", value))
                    {
                      data = 5;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Xmp.xmpRights.Marked",
                                 default_metadata_tags[i].tag))
                {
                  if (! strcmp ("True", value))
                    {
                      data = 1;
                    }
                  else if (! strcmp ("False", value))
                    {
                      data = 2;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Xmp.photoshop.Urgency",
                                 default_metadata_tags[i].tag))
                {
                  if (! strcmp ("1", value))
                    {
                      data = 1;
                    }
                  else if (! strcmp ("2", value))
                    {
                      data = 2;
                    }
                  else if (! strcmp ("3", value))
                    {
                      data = 3;
                    }
                  else if (! strcmp ("4", value))
                    {
                      data = 4;
                    }
                  else if (! strcmp ("5", value))
                    {
                      data = 5;
                    }
                  else if (! strcmp ("6", value))
                    {
                      data = 6;
                    }
                  else if (! strcmp ("7", value))
                    {
                      data = 7;
                    }
                  else if (! strcmp ("8", value))
                    {
                      data = 8;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Xmp.plus.MinorModelAgeDisclosure",
                                 default_metadata_tags[i].tag))
                {
                  if (! strcmp ("Age Unknown", value))
                    {
                      data = 0;
                    }
                  else if (! strcmp ("Age 25 or Over", value))
                    {
                      data = 1;
                    }
                  else if (! strcmp ("Age 24", value))
                    {
                      data = 2;
                    }
                  else if (! strcmp ("Age 23", value))
                    {
                      data = 3;
                    }
                  else if (! strcmp ("Age 22", value))
                    {
                      data = 4;
                    }
                  else if (! strcmp ("Age 21", value))
                    {
                      data = 5;
                    }
                  else if (! strcmp ("Age 20", value))
                    {
                      data = 6;
                    }
                  else if (! strcmp ("Age 19", value))
                    {
                      data = 7;
                    }
                  else if (! strcmp ("Age 18", value))
                    {
                      data = 8;
                    }
                  else if (! strcmp ("Age 17", value))
                    {
                      data = 9;
                    }
                  else if (! strcmp ("Age 16", value))
                    {
                      data = 10;
                    }
                  else if (! strcmp ("Age 15", value))
                    {
                      data = 11;
                    }
                  else if (! strcmp ("Age 14 or Under", value))
                    {
                      data = 12;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
              else if (! strcmp ("Xmp.plus.ModelReleaseStatus",
                                 default_metadata_tags[i].tag))
                {
                  gint loop;

                  for (loop = 0; loop < 4; loop++)
                    {
                      if (! strcmp (modelreleasestatus[loop].data, value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }

                      if (! strcmp (gettext (modelreleasestatus[loop].display),
                                    value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }
                    }
                }
              else if (! strcmp ("Xmp.iptcExt.DigitalSourceType",
                                 default_metadata_tags[i].tag))
                {
                  gint loop;

                  for (loop = 0; loop < 5; loop++)
                    {
                      if (! strcmp (digitalsourcetype[loop].data, value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }

                      if (! strcmp (gettext (digitalsourcetype[loop].display),
                                    value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }
                    }
                }
              else if (! strcmp ("Xmp.plus.PropertyReleaseStatus",
                                 default_metadata_tags[i].tag))
                {
                  gint loop;

                  for (loop = 0; loop < 4; loop++)
                    {
                      if (! strcmp (propertyreleasestatus[loop].data, value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }

                      if (! strcmp (gettext (propertyreleasestatus[loop].display),
                                    value))
                        {
                          gtk_combo_box_set_active (GTK_COMBO_BOX (widget), loop);
                          break;
                        }
                    }
                }
              else if (! strcmp ("Xmp.DICOM.PatientSex",
                                 default_metadata_tags[i].tag))
                {
                  if (! strcmp ("male", value))
                    {
                      data = 1;
                    }
                  else if (! strcmp ("female", value))
                    {
                      data = 2;
                    }
                  else if (! strcmp ("other", value))
                    {
                      data = 3;
                    }

                  gtk_combo_box_set_active (GTK_COMBO_BOX (widget), data);
                }
            }
        }
    }

  /* Set creation date */
  entry_widget = builder_get_widget (builder, "create_date_button");
  g_signal_connect (entry_widget, "clicked",
                    G_CALLBACK (on_create_date_button_clicked),
                    builder_get_widget (builder,
                                        "Xmp.photoshop.DateCreated"));

  /* Set patient dob date */
  entry_widget = builder_get_widget (builder, "dob_date_button");
  g_signal_connect (entry_widget, "clicked",
                    G_CALLBACK (on_patient_dob_date_button_clicked),
                    builder_get_widget (builder,
                                        "Xmp.DICOM.PatientDOB"));

  /* Set study date */
  entry_widget = builder_get_widget (builder, "study_date_button");
  g_signal_connect (entry_widget, "clicked",
                    G_CALLBACK (on_study_date_button_clicked),
                    builder_get_widget (builder,
                                        "Xmp.DICOM.StudyDateTime"));

  /* Set series date */
  entry_widget = builder_get_widget (builder, "series_date_button");
  g_signal_connect (entry_widget, "clicked",
                    G_CALLBACK (on_series_date_button_clicked),
                    builder_get_widget (builder,
                                        "Xmp.DICOM.SeriesDateTime"));
}


/* ============================================================================
 * ==[ WRITE METADATA ]========================================================
 * ============================================================================
 */

static void
set_tag_failed (const gchar *tag)
{
  g_log ("", G_LOG_LEVEL_MESSAGE,
         _("Failed to set metadata tag %s"), tag);
}

static void
set_tag_string (GimpMetadata *metadata,
                const gchar  *name,
                const gchar  *value)
{
  gexiv2_metadata_clear_tag (GEXIV2_METADATA (metadata), name);

  if (metadata == NULL) return;
  if (name == NULL) return;
  if (value == NULL) return;

  if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (metadata),
                                        name, value))
    {
      set_tag_failed (name);
    }
}

static void
write_metadata_tag (GtkBuilder *builder, GimpMetadata *metadata, gchar * tag, gint data_column)
{
  GtkWidget     *list_widget;
  GtkTreeModel  *treemodel;
  gint           row;
  gint           number_of_rows;
  gchar         *rc_data;
  GString       *data;

  list_widget = builder_get_widget (builder, tag);
  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));

  number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);

  if (number_of_rows <= 0)
    return;

  data = g_string_sized_new (256);

  for (row = 0; row < number_of_rows; row++)
    {
      GtkTreeIter iter;

      if (gtk_tree_model_iter_nth_child (treemodel, &iter, NULL, row))
        {
          gtk_tree_model_get (treemodel, &iter,
                              data_column, &rc_data,
                              -1);
          if (rc_data && rc_data[0] != '\0')
            {
              if (row > 0)
                g_string_append (data, ", ");

              g_string_append (data, rc_data);
            }
          g_free (rc_data);
        }
    }

  g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
         "write_metadata_tag tag: %s, value: %s",
         tag, data->str);

  set_tag_string (metadata, tag, data->str);
  g_string_free (data, TRUE);
}

static void
set_gps_longitude_latitude (GimpMetadata *metadata,
                            const gchar  *tag,
                            const gchar  *value)
{
  /* \u00b0 - degree symbol */
  const gchar  delimiters_dms[] = " deg'\":\u00b0";
  gchar        lng_lat[256];
  gchar       *s    = g_strdup (value);
  gchar       *str1 = NULL;
  gchar       *str2 = NULL;
  gchar       *str3 = NULL;
  gdouble      val  = 0.f;
  gint         degrees, minutes;
  gdouble      seconds;
  gboolean     remove_val = FALSE;

  g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "set_gps_longitude_latitude - Tag %s, Input value: %s", tag, value);

  if (s && s[0] != '\0')
    {
      str1 = strtok (s, delimiters_dms);
      str2 = strtok (NULL, delimiters_dms);
      str3 = strtok (NULL, delimiters_dms);

      g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "String split into: %s - %s - %s", str1, str2, str3);
    }

  g_free (s);

  if (str1 && str2 && str3)
    {
      /* Assuming degrees, minutes, seconds */
      degrees = g_ascii_strtoll (str1, NULL, 10);
      minutes = g_ascii_strtoll (str2, NULL, 10);
      seconds = g_ascii_strtod (str3, NULL);
    }
  else if (str1 && str2)
    {
      /* Assuming degrees, minutes */
      gdouble min;

      degrees = g_ascii_strtoll (str1, NULL, 10);
      min = g_ascii_strtod (str2, NULL);
      minutes = (gint) min;
      seconds = (min - (gdouble) minutes) * 60.f;
    }
  else if (str1)
    {
      /* Assuming degrees only */
      val = g_ascii_strtod (str1, NULL);
      degrees = (gint) val;
      minutes = (gint) ((val - (gdouble) degrees) * 60.f);
      seconds = ((val - (gdouble) degrees - (gdouble) (minutes / 60.f)) * 60.f * 60.f);
    }
  else
    remove_val = TRUE;

  if (!remove_val)
    {
      g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Converted values: %d - %d - %f", degrees, minutes, seconds);
      g_snprintf (lng_lat, sizeof (lng_lat),
                  "%d/1 %d/1 %d/1000",
                  abs (degrees), abs (minutes), abs ((gint) (seconds * 1000.f)));
      g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Tag: %s, output string: %s", tag, lng_lat);

      if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (metadata),
                                            tag, lng_lat))
        {
          set_tag_failed (tag);
        }
    }
  else
    {
      gexiv2_metadata_clear_tag (GEXIV2_METADATA (metadata), tag);
      g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Removed tag %s (no value).", tag);
    }
}

void
metadata_editor_write_callback (GtkWidget  *dialog,
                                GtkBuilder *builder,
                                GimpImage  *image)
{
  GimpMetadata     *g_metadata;
  gint              max_elements;
  gint              i;
  GtkWidget        *list_widget;
  GtkTreeIter       iter;
  GtkTreeModel     *treemodel;
  gint              number_of_rows;
  gchar             tag[1024];
  gint              counter;
  gint              row;

  i = 0;

  g_metadata = gimp_image_get_metadata (image);

  gimp_metadata_add_xmp_history (g_metadata, "metadata");

  write_metadata_tag (builder, g_metadata,
                      "Xmp.iptcExt.OrganisationInImageName",
                      COL_ORG_IMG_NAME);

  write_metadata_tag (builder, g_metadata,
                      "Xmp.iptcExt.OrganisationInImageCode",
                      COL_ORG_IMG_CODE);

  write_metadata_tag (builder, g_metadata,
                      "Xmp.plus.ModelReleaseID",
                      COL_MOD_REL_ID);

  write_metadata_tag (builder, g_metadata,
                      "Xmp.plus.PropertyReleaseID",
                      COL_PROP_REL_ID);

  /* DO LOCATION SHOWN (LISTSTORE) */

  list_widget = builder_get_widget (builder, "Xmp.iptcExt.LocationShown");

  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));

  number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);

  /* CLEAR LOCATION SHOW DATA */

  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                             LOCATIONSHOWN_HEADER);

  for (row = 0; row < 256; row++)
    {
      gint item;

      for (item = 0; item < n_locationshown; item++)
        {
          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                     LOCATIONSHOWN_HEADER, row, locationshown[item]);
          gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata), tag);
        }
    }

  /* SET LOCATION SHOW DATA */
  gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                      "Xmp.iptcExt.LocationShown",
                                      GEXIV2_STRUCTURE_XA_BAG);

  counter = 1;
  for (row = 0; row < number_of_rows; row++)
    {
      if (gtk_tree_model_iter_nth_child (treemodel, &iter, NULL, row))
        {
          gchar *tag_data;

          gtk_tree_model_get (treemodel, &iter,
                              COL_LOC_SHO_SUB_LOC, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LOCATIONSHOWN_HEADER, counter, locationshown[0]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LOC_SHO_CITY, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LOCATIONSHOWN_HEADER, counter, locationshown[1]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LOC_SHO_STATE_PROV, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LOCATIONSHOWN_HEADER, counter, locationshown[2]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LOC_SHO_CNTRY, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LOCATIONSHOWN_HEADER, counter, locationshown[3]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LOC_SHO_CNTRY_ISO, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LOCATIONSHOWN_HEADER, counter, locationshown[4]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LOC_SHO_CNTRY_WRLD_REG, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LOCATIONSHOWN_HEADER, counter, locationshown[5]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          counter++;
        }
    }

  /* DO ARTWORK OR OBJECT (LISTSTORE) */

  list_widget = builder_get_widget (builder, "Xmp.iptcExt.ArtworkOrObject");

  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));

  number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);

  /* CLEAR ARTWORK OR OBJECT DATA */

  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                             ARTWORKOROBJECT_HEADER);

  for (row = 0; row < 256; row++)
    {
      gint item;

      for (item = 0; item < n_artworkorobject; item++)
        {
          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      ARTWORKOROBJECT_HEADER, row, artworkorobject[item]);

          gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata), tag);
        }
    }

  /* SET ARTWORK OR OBJECT DATA */

  gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                      "Xmp.iptcExt.ArtworkOrObject",
                                      GEXIV2_STRUCTURE_XA_BAG);

  counter = 1;
  for (row = 0; row < number_of_rows; row++)
    {
      if (gtk_tree_model_iter_nth_child (treemodel, &iter, NULL, row))
        {
          gchar *tag_data;

          gtk_tree_model_get (treemodel, &iter,
                              COL_AOO_TITLE, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      ARTWORKOROBJECT_HEADER, counter, artworkorobject[0]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_AOO_DATE_CREAT, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      ARTWORKOROBJECT_HEADER, counter, artworkorobject[1]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_AOO_CREATOR, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      ARTWORKOROBJECT_HEADER, counter, artworkorobject[2]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_AOO_SOURCE, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      ARTWORKOROBJECT_HEADER, counter, artworkorobject[3]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_AOO_SRC_INV_ID, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      ARTWORKOROBJECT_HEADER, counter, artworkorobject[4]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_AOO_CR_NOT, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      ARTWORKOROBJECT_HEADER, counter, artworkorobject[5]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          counter++;
        }
    }

  /* DO REGISTRY ID (LISTSTORE) */

  list_widget = builder_get_widget (builder, "Xmp.iptcExt.RegistryId");

  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));

  number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);

  /* CLEAR REGISTRY ID DATA */

  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                             REGISTRYID_HEADER);

  for (row = 0; row < 256; row++)
    {
      gint item;

      for (item = 0; item < n_registryid; item++)
        {
          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      REGISTRYID_HEADER, row, registryid[item]);

          gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata), tag);
        }
    }

  /* SET REGISTRY ID DATA */

  gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                      "Xmp.iptcExt.RegistryId",
                                      GEXIV2_STRUCTURE_XA_BAG);

  counter = 1;
  for (row = 0; row < number_of_rows; row++)
    {
      if (gtk_tree_model_iter_nth_child (treemodel, &iter, NULL, row))
        {
          gchar *tag_data;

          gtk_tree_model_get (treemodel, &iter,
                              COL_REGISTRY_ORG_ID, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      REGISTRYID_HEADER, counter, registryid[0]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_REGISTRY_ITEM_ID, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      REGISTRYID_HEADER, counter, registryid[1]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          counter++;
        }
    }

  /* DO IMAGE CREATOR (LISTSTORE) */

  list_widget = builder_get_widget (builder, "Xmp.plus.ImageCreator");

  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));

  number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);

  /* CLEAR IMAGE CREATOR DATA */

  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                             IMAGECREATOR_HEADER);

  for (row = 0; row < 256; row++)
    {
      gint item;

      for (item = 0; item < n_imagecreator; item++)
        {
          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      IMAGECREATOR_HEADER, row, imagecreator[item]);

          gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata), tag);
        }
    }

  /* SET IMAGE CREATOR DATA */

  gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                      "Xmp.plus.ImageCreator",
                                      GEXIV2_STRUCTURE_XA_SEQ);

  counter = 1;
  for (row = 0; row < number_of_rows; row++)
    {
      if (gtk_tree_model_iter_nth_child (treemodel, &iter, NULL, row))
        {
          gchar *tag_data;

          gtk_tree_model_get (treemodel, &iter,
                              COL_IMG_CR8_NAME, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      IMAGECREATOR_HEADER, counter, imagecreator[0]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_IMG_CR8_ID, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      IMAGECREATOR_HEADER, counter, imagecreator[1]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          counter++;
        }
    }

  /* DO COPYRIGHT OWNER (LISTSTORE) */

  list_widget = builder_get_widget (builder, "Xmp.plus.CopyrightOwner");

  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));

  number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);

  /* CLEAR COPYRIGHT OWNER DATA */

  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                             COPYRIGHTOWNER_HEADER);

  for (row = 0; row < 256; row++)
    {
      gint item;

      for (item = 0; item < n_copyrightowner; item++)
        {
          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      COPYRIGHTOWNER_HEADER, row, copyrightowner[item]);

          gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata), tag);
        }
    }

  /* SET COPYRIGHT OWNER DATA */

  gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                      "Xmp.plus.CopyrightOwner",
                                      GEXIV2_STRUCTURE_XA_SEQ);

  counter = 1;
  for (row = 0; row < number_of_rows; row++)
    {
      if (gtk_tree_model_iter_nth_child (treemodel, &iter, NULL, row))
        {
          gchar *tag_data;

          gtk_tree_model_get (treemodel, &iter,
                              COL_CR_OWNER_NAME, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      COPYRIGHTOWNER_HEADER, counter, copyrightowner[0]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_CR_OWNER_ID, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      COPYRIGHTOWNER_HEADER, counter, copyrightowner[1]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          counter++;
        }
    }

  /* DO LICENSOR (LISTSTORE) */

  list_widget = builder_get_widget (builder, "Xmp.plus.Licensor");

  treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW (list_widget));

  number_of_rows = gtk_tree_model_iter_n_children (treemodel, NULL);

  /* CLEAR LICENSOR DATA */

  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                             LICENSOR_HEADER);

  for (row = 0; row < 256; row++)
    {
      gint item;

      for (item = 0; item < n_licensor; item++)
        {
          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LICENSOR_HEADER, row, licensor[item]);

          gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata), tag);
        }
    }

  /* SET LICENSOR DATA */

  gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                      "Xmp.plus.Licensor",
                                      GEXIV2_STRUCTURE_XA_SEQ);

  counter = 1;
  for (row = 0; row < number_of_rows; row++)
    {
      if (gtk_tree_model_iter_nth_child (treemodel, &iter, NULL, row))
        {
          gchar *tag_data;
          gchar  type1[256];
          gchar  type2[256];
          gint   types;

          gtk_tree_model_get (treemodel, &iter,
                              COL_LICENSOR_NAME, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LICENSOR_HEADER, counter, licensor[0]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LICENSOR_ID, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LICENSOR_HEADER, counter, licensor[1]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LICENSOR_PHONE1, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LICENSOR_HEADER, counter, licensor[2]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LICENSOR_PHONE_TYPE1, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LICENSOR_HEADER, counter, licensor[3]);

          strcpy (type1, phone_types[0].data);

          if (tag_data != NULL)
            {
              for (types = 0; types < 6; types++)
                {
                  if (! strcmp (tag_data, gettext (phone_types[types].display)))
                    {
                      strcpy (type1, phone_types[types].data);
                      break;
                    }
                }
            }

          set_tag_string (g_metadata, tag, type1);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LICENSOR_PHONE2, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LICENSOR_HEADER, counter, licensor[4]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LICENSOR_PHONE_TYPE2, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LICENSOR_HEADER, counter, licensor[5]);

          strcpy (type2, phone_types[0].data);

          if (tag_data != NULL)
            {
              for (types = 0; types < 6; types++)
                {
                  g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%d %s %s", types, tag_data,
                           gettext (phone_types[types].display));
                  if (! strcmp (tag_data, gettext (phone_types[types].display)))
                    {
                      g_log (ME_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%d %s", types, phone_types[types].data);
                      strcpy (type2, phone_types[types].data);
                      break;
                    }
                }
            }
          set_tag_string (g_metadata, tag, type2);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LICENSOR_EMAIL, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LICENSOR_HEADER, counter, licensor[6]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          gtk_tree_model_get (treemodel, &iter,
                              COL_LICENSOR_WEB, &tag_data,
                              -1);

          g_snprintf (tag, sizeof (tag), "%s[%d]%s",
                      LICENSOR_HEADER, counter, licensor[7]);

          set_tag_string (g_metadata, tag, tag_data);
          g_free (tag_data);

          counter++;
        }
    }

  /* DO CREATOR TAGS */

  if (hasCreatorTagData (builder))
    {
      if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                            creatorContactInfoHeader.header,
                                            "type=\"Struct\""))
        {
          set_tag_failed (creatorContactInfoTags[i].tag);
        }

      for (i = 0; i < creatorContactInfoHeader.size; i++)
        {
          GObject *object = gtk_builder_get_object (builder,
                                                    creatorContactInfoTags[i].id);

          if (! strcmp ("single", creatorContactInfoTags[i].mode))
            {
              GtkEntry *entry = GTK_ENTRY (object);

              if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                    creatorContactInfoTags[i].tag,
                                                    gtk_entry_get_text (entry)))
                {
                  set_tag_failed (creatorContactInfoTags[i].tag);
                }
            }
          else if (! strcmp ("multi", creatorContactInfoTags[i].mode))
            {
              GtkTextView   *text_view = GTK_TEXT_VIEW (object);
              GtkTextBuffer *buffer;
              GtkTextIter    start;
              GtkTextIter    end;
              gchar         *text;

              buffer = gtk_text_view_get_buffer (text_view);
              gtk_text_buffer_get_start_iter (buffer, &start);
              gtk_text_buffer_get_end_iter (buffer, &end);

              text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

              if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                    creatorContactInfoTags[i].tag,
                                                    text))
                {
                  set_tag_failed (creatorContactInfoTags[i].tag);
                }

              g_free (text);
            }
        }
    }

  /* DO SINGLE, MULTI AND COMBO TAGS */

  else
    {
      gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                 creatorContactInfoHeader.header);

      for (i = 0; i < creatorContactInfoHeader.size; i++)
        {
          gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                     creatorContactInfoTags[i].tag);
        }
    }

  max_elements = n_default_metadata_tags;

  for (i = 0; i < max_elements; i++)
    {
      GObject *object = gtk_builder_get_object (builder,
                                                default_metadata_tags[i].tag);

      /* SINGLE TAGS */

      if (! strcmp ("single", default_metadata_tags[i].mode))
        {
          GtkEntry *entry       = GTK_ENTRY (object);
          gchar    *value_entry = g_strdup (gtk_entry_get_text (entry));

          if (! strcmp ("Exif.GPSInfo.GPSLongitude",
                        default_metadata_tags[i].tag) ||
              ! strcmp ("Exif.GPSInfo.GPSLatitude",
                        default_metadata_tags[i].tag))
            {
              set_gps_longitude_latitude (g_metadata,
                                          default_metadata_tags[i].tag,
                                          value_entry);
            }
          else if (! strcmp ("Exif.GPSInfo.GPSAltitude",
                             default_metadata_tags[i].tag))
            {
              GtkWidget *combo_widget;
              gchar      alt_str[256];
              gdouble    alt_d;
              gint       msr;

              combo_widget = builder_get_widget (builder,
                                                 "GPSAltitudeSystem");
              msr = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_widget));

              alt_d = atof (gtk_entry_get_text (entry));
              if (msr == 1)
                alt_d = (alt_d * 12 * 2.54);
              else
                alt_d *= 100.f;

              g_snprintf (alt_str, sizeof (alt_str), "%d/100", (gint) alt_d);

              if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                    default_metadata_tags[i].tag,
                                                    alt_str))
                {
                  set_tag_failed (default_metadata_tags[i].tag);
                }
            }
          else
            {
              gint         index;
              const gchar *text_value = gtk_entry_get_text (entry);

              if (default_metadata_tags[i].xmp_type == GIMP_XMP_TEXT ||
                  default_metadata_tags[i].xmp_type == GIMP_XMP_NONE)
                {
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                  gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                                      default_metadata_tags[i].tag,
                                                      GEXIV2_STRUCTURE_XA_NONE);
                  if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                        default_metadata_tags[i].tag,
                                                        text_value))
                    {
                      set_tag_failed (default_metadata_tags[i].tag);
                    }
                }
              else if (default_metadata_tags[i].xmp_type == GIMP_XMP_BAG)
                {
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                  gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                                      default_metadata_tags[i].tag,
                                                      GEXIV2_STRUCTURE_XA_BAG);
                  if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                        default_metadata_tags[i].tag,
                                                        text_value))
                    {
                      set_tag_failed (default_metadata_tags[i].tag);
                    }
                }
              else if (default_metadata_tags[i].xmp_type == GIMP_XMP_SEQ)
                {
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                  gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                                      default_metadata_tags[i].tag,
                                                      GEXIV2_STRUCTURE_XA_SEQ);
                  if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                        default_metadata_tags[i].tag,
                                                        text_value))
                    {
                      set_tag_failed (default_metadata_tags[i].tag);
                    }
                }

              index = default_metadata_tags[i].other_tag_index;
              if (index > -1)
                {
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             equivalent_metadata_tags[index].tag);
                  if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                        equivalent_metadata_tags[index].tag,
                                                        text_value))
                    {
                      set_tag_failed (equivalent_metadata_tags[index].tag);
                    }
                }
            }
        }

      /* MULTI TAGS */

      else if (! strcmp ("multi", default_metadata_tags[i].mode))
        {
          GtkTextView   *text_view = GTK_TEXT_VIEW (object);
          GtkTextBuffer *buffer;
          GtkTextIter    start;
          GtkTextIter    end;
          gchar         *text;
          gint           index;

          buffer = gtk_text_view_get_buffer (text_view);
          gtk_text_buffer_get_start_iter (buffer, &start);
          gtk_text_buffer_get_end_iter (buffer, &end);

          text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

          if (default_metadata_tags[i].xmp_type == GIMP_XMP_TEXT ||
              default_metadata_tags[i].xmp_type == GIMP_XMP_NONE)
            {
              gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                         default_metadata_tags[i].tag);
              gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  GEXIV2_STRUCTURE_XA_NONE);
              if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                    default_metadata_tags[i].tag,
                                                    text))
                {
                  set_tag_failed (default_metadata_tags[i].tag);
                }
            }
          else if (default_metadata_tags[i].xmp_type == GIMP_XMP_BAG)
            {
              gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                         default_metadata_tags[i].tag);
              gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  GEXIV2_STRUCTURE_XA_BAG);
              if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                    default_metadata_tags[i].tag,
                                                    text))
                {
                  set_tag_failed (default_metadata_tags[i].tag);
                }
            }
          else if (default_metadata_tags[i].xmp_type == GIMP_XMP_SEQ)
            {
              gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                         default_metadata_tags[i].tag);
              gexiv2_metadata_set_xmp_tag_struct (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  GEXIV2_STRUCTURE_XA_SEQ);
              if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                    default_metadata_tags[i].tag,
                                                    text))
                {
                  set_tag_failed (default_metadata_tags[i].tag);
                }
            }

            index = default_metadata_tags[i].other_tag_index;
            if (index > -1)
              {
                gchar **multi;

                gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                           equivalent_metadata_tags[index].tag);
                multi = g_strsplit (text, "\n", 0);
                if (! gexiv2_metadata_set_tag_multiple (GEXIV2_METADATA (g_metadata),
                                                        equivalent_metadata_tags[index].tag,
                                                        (const gchar **) multi))
                  {
                    set_tag_failed (equivalent_metadata_tags[index].tag);
                  }

                g_strfreev (multi);
              }

          if (text)
            g_free (text);
        }
      else if (! strcmp ("list", default_metadata_tags[i].mode))
        {
          /* MIGHT DO SOMETHING HERE */
        }

      /* COMBO TAGS */

      else if (! strcmp ("combo", default_metadata_tags[i].mode))
        {
          GtkComboBoxText *combo;
          gint32           value;

          combo = GTK_COMBO_BOX_TEXT (object);
          value = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

          if (! strcmp ("Xmp.photoshop.Urgency", default_metadata_tags[i].tag))
            {
              /* IPTC tab - Urgency */
              if (value == 0)
                {
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             "Iptc.Application2.Urgency");
                }
              else
                {
                  gchar *save;

                  save = g_strdup_printf ("%d", value);

                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  save);
                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  "Iptc.Application2.Urgency",
                                                  save);
                  g_free (save);
                }
            }
          else if (! strcmp ("Xmp.xmpRights.Marked",
                             default_metadata_tags[i].tag))
            {
              /* Description tab - Copyright Status */
              if (value == 0)
                {
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                }
              else
                {
                  gchar *save_value;

                  if (value == 1)
                    save_value = g_strdup_printf ("%s", "True");
                  else /* (value == 2) */
                    save_value = g_strdup_printf ("%s", "False");

                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  save_value);
                  g_free (save_value);
                }
            }
          else if (! strcmp ("Xmp.xmp.Rating", default_metadata_tags[i].tag))
            {
              if (value == 0)
                {
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                }
              else
                {
                  gchar *save;

                  save = g_strdup_printf ("%d", value);

                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  save);
                  g_free (save);
                }
            }
          else if (! strcmp ("Xmp.DICOM.PatientSex",
                             default_metadata_tags[i].tag))
            {
              switch (value)
                {
                case 0:
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                  break;

                case 1:
                  if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                        default_metadata_tags[i].tag,
                                                        "male"))
                    {
                      set_tag_failed (default_metadata_tags[i].tag);
                    }
                  break;

                case 2:
                  if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                        default_metadata_tags[i].tag,
                                                        "female"))
                    {
                      set_tag_failed (default_metadata_tags[i].tag);
                    }
                  break;

                case 3:
                  if (! gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                        default_metadata_tags[i].tag,
                                                        "other"))
                    {
                      set_tag_failed (default_metadata_tags[i].tag);
                    }
                  break;
                }
            }
          else if (! strcmp ("Exif.GPSInfo.GPSLongitudeRef",
                             default_metadata_tags[i].tag))
            {
              switch (value)
                {
                case 0:
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                  break;

                case 1:
                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  "E");
                  break;

                case 2:
                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  "W");
                  break;
                }
            }
          else if (! strcmp ("Exif.GPSInfo.GPSLatitudeRef",
                             default_metadata_tags[i].tag))
            {
              switch (value)
                {
                case 0:
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                  break;

                case 1:
                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  "N");
                  break;

                case 2:
                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  "S");
                  break;
                }
            }
          else if (! strcmp ("Exif.GPSInfo.GPSAltitudeRef",
                             default_metadata_tags[i].tag))
            {
              switch (value)
                {
                case 0:
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                  break;

                case 1:
                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  "0");
                  break;

                case 2:
                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  "1");
                  break;
                }
            }
          else if (! strcmp ("Xmp.plus.ModelReleaseStatus",
                             default_metadata_tags[i].tag))
            {
              gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                              default_metadata_tags[i].tag,
                                              modelreleasestatus[value].data);
            }
          else if (! strcmp ("Xmp.plus.PropertyReleaseStatus",
                             default_metadata_tags[i].tag))
            {
              if (value == 0)
                {
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                }
              else
                {
                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  propertyreleasestatus[value].data);
                }
            }
          else if (! strcmp ("Xmp.plus.MinorModelAgeDisclosure",
                             default_metadata_tags[i].tag))
            {
              if (value == 0)
                {
                  gexiv2_metadata_clear_tag (GEXIV2_METADATA (g_metadata),
                                             default_metadata_tags[i].tag);
                }
              else
                {
                  gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                                  default_metadata_tags[i].tag,
                                                  minormodelagedisclosure[value].data);
                }
            }
          else if (! strcmp ("Xmp.iptcExt.DigitalSourceType",
                             default_metadata_tags[i].tag))
            {
              gexiv2_metadata_set_tag_string (GEXIV2_METADATA (g_metadata),
                                              default_metadata_tags[i].tag,
                                              digitalsourcetype[value].data);
            }
        }
    }

  gimp_image_set_metadata (image, g_metadata);
}

/* ============================================================================
 * ==[ METADATA IMPORT / EXPORT FILE DIALOG UI ]===============================
 * ============================================================================
 */

static void
import_dialog_metadata (metadata_editor *args)
{
  GtkWidget *file_dialog;
  gchar     *filename;

  file_dialog = gtk_file_chooser_dialog_new (_("Import Metadata File"),
                                             NULL,
                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                             _("_Cancel"), GTK_RESPONSE_CANCEL,
                                             _("_Import"), GTK_RESPONSE_ACCEPT,
                                             NULL);

  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (file_dialog),
                                 args->filename);

  if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog));

      if (filename)
        {
          if (args->filename)
            {
              g_free (args->filename);
            }

          args->filename = g_strdup (filename);
          import_file_metadata (args);
        }
    }

  gtk_widget_destroy (file_dialog);
}

static void
export_dialog_metadata (metadata_editor *args)
{
  GtkWidget *file_dialog;
  gchar     *filename;

  file_dialog = gtk_file_chooser_dialog_new (_("Export Metadata File"),
                                             NULL,
                                             GTK_FILE_CHOOSER_ACTION_SAVE,
                                             _("_Cancel"), GTK_RESPONSE_CANCEL,
                                             _("_Export"), GTK_RESPONSE_ACCEPT,
                                             NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (file_dialog),
                                                  TRUE);
  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (file_dialog),
                                 args->filename);

  if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog));

      if (filename)
        {
          if (args->filename)
            {
              g_free (args->filename);
            }

          args->filename = g_strdup (filename);
          export_file_metadata (args);
        }
    }

  gtk_widget_destroy (file_dialog);
}

static void
impex_combo_callback (GtkComboBoxText *combo,
                      gpointer         data)
{
  metadata_editor *args;
  gint32           selection;

  args = data;
  selection = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  switch (selection)
    {
    case 1: /* Import */
      import_dialog_metadata (args);
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
      break;

    case 2: /* Export */
      export_dialog_metadata (args);
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
      break;
    }
}


static void
gpsaltsys_combo_callback (GtkComboBoxText *combo,
                          gpointer         data)
{
  GtkWidget  *entry;
  GtkBuilder *builder;
  gint32      selection;
  gchar       alt_str[256];
  double      alt_d;

  builder = data;
  selection = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  entry = builder_get_widget (builder, "Exif.GPSInfo.GPSAltitude");

  switch (selection)
    {
    case 0: /* Meters */
      if (last_gpsaltsys_sel != 0)
        {
          alt_d = atof (gtk_entry_get_text (GTK_ENTRY (entry)));
          alt_d = (alt_d * (12 * 2.54)) / 100;

          g_snprintf (alt_str, sizeof (alt_str), "%.2f", (float)alt_d);

          gtk_entry_set_text (GTK_ENTRY (entry), alt_str);
        }
      break;

    case 1: /* Feet */
      if (last_gpsaltsys_sel != 1)
        {
          alt_d = atof (gtk_entry_get_text (GTK_ENTRY (entry)));
          alt_d = alt_d * 3.28;

          g_snprintf (alt_str, sizeof (alt_str), "%.2f", (float)alt_d);

          gtk_entry_set_text (GTK_ENTRY (entry), alt_str);
        }
      break;
    }

  last_gpsaltsys_sel = selection;
}
