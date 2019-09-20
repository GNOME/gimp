/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * metadata.c
 * Copyright (C) 2013 Hartmut Kuhse
 * Copyright (C) 2016 Ben Touchette
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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gexiv2/gexiv2.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-metadata-viewer"
#define PLUG_IN_BINARY "metadata-viewer"
#define PLUG_IN_ROLE   "gimp-metadata"

#define EXIF_PREFIX "Exif."
#define IPTC_PREFIX "Iptc."
#define XMP_PREFIX  "Xmp."

/* The length at which to truncate tag values, in characters. */
#define TAG_VALUE_MAX_SIZE 1024

/* The length at which to truncate raw data (i.e., tag values
 * of type "Byte" or "Undefined"), in bytes.
 */
#define RAW_DATA_MAX_SIZE 16


enum
{
  C_XMP_TAG = 0,
  C_XMP_VALUE,
  NUM_XMP_COLS
};

enum
{
  C_EXIF_TAG = 0,
  C_EXIF_VALUE,
  NUM_EXIF_COLS
};

enum
{
  C_IPTC_TAG = 0,
  C_IPTC_VALUE,
  NUM_IPTC_COLS
};


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

static gboolean  metadata_viewer_dialog           (GimpImage      *image,
                                                   GimpMetadata   *g_metadata);
static void      metadata_dialog_set_metadata     (GExiv2Metadata *metadata,
                                                   GtkBuilder     *builder);
static void      metadata_dialog_append_tags      (GExiv2Metadata  *metadata,
                                                   gchar          **tags,
                                                   GtkListStore    *store,
                                                   gint             tag_column,
                                                   gint             value_column);
static gchar   * metadata_dialog_format_tag_value (GExiv2Metadata  *metadata,
                                                   const gchar     *tag,
                                                   gboolean         truncate);


G_DEFINE_TYPE (Metadata, metadata, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (METADATA_TYPE)


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

      gimp_procedure_set_menu_label (procedure, N_("_View Metadata"));
      gimp_procedure_add_menu_path (procedure, "<Image>/Image/Metadata");

      gimp_procedure_set_documentation (procedure,
                                        N_("View metadata (Exif, IPTC, XMP)"),
                                        "View metadata information attached "
                                        "to the current image. This can "
                                        "include Exif, IPTC and/or XMP "
                                        "information.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Hartmut Kuhse, Michael Natterer, "
                                      "Ben Touchette",
                                      "Hartmut Kuhse, Michael Natterer, "
                                      "Ben Touchette",
                                      "2013, 2017");

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

  metadata_viewer_dialog (image, metadata);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
metadata_viewer_dialog (GimpImage    *image,
                        GimpMetadata *g_metadata)
{
  GtkBuilder     *builder;
  GtkWidget      *dialog;
  GtkWidget      *metadata_vbox;
  GtkWidget      *content_area;
  gchar          *ui_file;
  gchar          *title;
  gchar          *name;
  GError         *error = NULL;
  GExiv2Metadata *metadata;

  metadata = GEXIV2_METADATA(g_metadata);

  builder = gtk_builder_new ();

  ui_file = g_build_filename (gimp_data_directory (),
                              "ui", "plug-ins", "plug-in-metadata-viewer.ui", NULL);

  if (! gtk_builder_add_from_file (builder, ui_file, &error))
    {
      g_printerr ("Error occurred while loading UI file!\n");
      g_printerr ("Message: %s\n", error->message);
      g_clear_error (&error);
      g_free (ui_file);
      g_object_unref (builder);
      return FALSE;
    }

  g_free (ui_file);

  name = gimp_image_get_name (image);
  title = g_strdup_printf (_("Metadata Viewer: %s"), name);
  g_free (name);

  dialog = gimp_dialog_new (title,
                            "gimp-metadata-viewer-dialog",
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,
                            _("_Close"), GTK_RESPONSE_CLOSE,
                            NULL);

  gtk_widget_set_size_request(dialog, 460, 340);

  g_free (title);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_CLOSE,
                                           -1);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  metadata_vbox = GTK_WIDGET (gtk_builder_get_object (builder,
                                                      "metadata-vbox"));
  gtk_container_set_border_width (GTK_CONTAINER (metadata_vbox), 12);
  gtk_box_pack_start (GTK_BOX (content_area), metadata_vbox, TRUE, TRUE, 0);

  metadata_dialog_set_metadata (metadata, builder);

  gtk_dialog_run (GTK_DIALOG (dialog));

  return TRUE;
}


/*  private functions  */

static void
metadata_dialog_set_metadata (GExiv2Metadata *metadata,
                              GtkBuilder     *builder)
{
  gchar        **tags;
  GtkListStore  *store;

  /* load exif tags */
  tags  = gexiv2_metadata_get_exif_tags (metadata);
  store = GTK_LIST_STORE (gtk_builder_get_object (builder, "exif-liststore"));

  metadata_dialog_append_tags (metadata, tags, store, C_EXIF_TAG, C_EXIF_VALUE);

  g_strfreev (tags);

  /* load xmp tags */
  tags  = gexiv2_metadata_get_xmp_tags (metadata);
  store = GTK_LIST_STORE (gtk_builder_get_object (builder, "xmp-liststore"));

  metadata_dialog_append_tags (metadata, tags, store, C_XMP_TAG, C_XMP_VALUE);

  g_strfreev (tags);

  /* load iptc tags */
  tags  = gexiv2_metadata_get_iptc_tags (metadata);
  store = GTK_LIST_STORE (gtk_builder_get_object (builder, "iptc-liststore"));

  metadata_dialog_append_tags (metadata, tags, store, C_IPTC_TAG, C_IPTC_VALUE);

  g_strfreev (tags);
}

static void
metadata_dialog_append_tags (GExiv2Metadata  *metadata,
                             gchar          **tags,
                             GtkListStore    *store,
                             gint             tag_column,
                             gint             value_column)
{
  GtkTreeIter  iter;
  const gchar *tag;

  while ((tag = *tags++))
    {
      gchar       *value;

      gtk_list_store_append (store, &iter);

      value = metadata_dialog_format_tag_value (metadata, tag,
                                                /* truncate = */ TRUE);

      gtk_list_store_set (store, &iter,
                          tag_column,   tag,
                          value_column, value,
                          -1);

      g_free (value);
    }
}

static gchar *
metadata_dialog_format_tag_value (GExiv2Metadata *metadata,
                                  const gchar    *tag,
                                  gboolean        truncate)
{
  const gchar *tag_type;
  gchar       *result;

  tag_type = gexiv2_metadata_get_tag_type (tag);

  if (g_strcmp0 (tag_type, "Byte")      != 0 &&
      g_strcmp0 (tag_type, "Undefined") != 0 &&
      g_strcmp0 (tag_type, NULL)        != 0)
    {
      gchar *value;
      gchar *value_utf8;
      glong  size;

      value      = gexiv2_metadata_get_tag_interpreted_string (metadata, tag);
      value_utf8 = g_locale_to_utf8 (value, -1, NULL, NULL, NULL);

      g_free (value);

      size = g_utf8_strlen (value_utf8, -1);

      if (! truncate || size <= TAG_VALUE_MAX_SIZE)
        {
          result = value_utf8;
        }
      else
        {
          gchar   *value_utf8_trunc;
          GString *str;

          value_utf8_trunc = g_utf8_substring (value_utf8,
                                               0, TAG_VALUE_MAX_SIZE);

          g_free (value_utf8);

          str = g_string_new (value_utf8_trunc);

          g_free (value_utf8_trunc);

          g_string_append (str, "... ");
          g_string_append_printf (str,
                                  _("(%lu more character(s))"),
                                  size - TAG_VALUE_MAX_SIZE);

          result = g_string_free (str, FALSE);
        }
    }
  else
    {
      GBytes       *bytes;
      const guchar *data;
      gsize         size;
      gsize         display_size;
      GString      *str;
      gint          i;

      bytes = gexiv2_metadata_get_tag_raw (metadata, tag);
      data  = g_bytes_get_data (bytes, &size);

      if (! truncate)
        display_size = size;
      else
        display_size = MIN (size, RAW_DATA_MAX_SIZE);

      str = g_string_sized_new (3 * display_size);

      for (i = 0; i < display_size; i++)
        g_string_append_printf (str, i == 0 ? "%02x" : " %02x", data[i]);

      if (display_size < size)
        {
          g_string_append (str, " ... ");
          g_string_append_printf (str,
                                  _("(%llu more byte(s))"),
                                  (unsigned long long) (size - display_size));
        }

      result = g_string_free (str, FALSE);

      g_bytes_unref (bytes);
    }

  return result;
}
