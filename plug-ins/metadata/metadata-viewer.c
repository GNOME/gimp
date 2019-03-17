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


/*  local function prototypes  */

static void       query                            (void);
static void       run                              (const gchar      *name,
                                                    gint              nparams,
                                                    const GimpParam  *param,
                                                    gint             *nreturn_vals,
                                                    GimpParam       **return_vals);

static gboolean   metadata_viewer_dialog           (gint32          image_id,
                                                    GimpMetadata   *g_metadata);
static void       metadata_dialog_set_metadata     (GExiv2Metadata *metadata,
                                                    GtkBuilder     *builder);
static void       metadata_dialog_append_tags      (GExiv2Metadata  *metadata,
                                                    gchar          **tags,
                                                    GtkListStore    *store,
                                                    gint             tag_column,
                                                    gint             value_column);
static gchar    * metadata_dialog_format_tag_value (GExiv2Metadata  *metadata,
                                                    const gchar     *tag,
                                                    gboolean         truncate);


/* local variables */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

/*  functions  */

MAIN ()

static void
query (void)
{
  static const GimpParamDef metadata_args[] =
  {
    { GIMP_PDB_INT32, "run-mode", "Run mode { RUN-INTERACTIVE (0) }" },
    { GIMP_PDB_IMAGE, "image",    "Input image"                      }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("View metadata (Exif, IPTC, XMP)"),
                          "View metadata information attached to the "
                          "current image.  This can include Exif, IPTC and/or "
                          "XMP information.",
                          "Hartmut Kuhse, Michael Natterer, Ben Touchette",
                          "Hartmut Kuhse, Michael Natterer, Ben Touchette",
                          "2013, 2017",
                          N_("_View Metadata"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (metadata_args), 0,
                          metadata_args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Image/Metadata");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  INIT_I18N();
  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  if (! strcmp (name, PLUG_IN_PROC))
    {
      GimpMetadata *metadata;
      gint32        image_ID = param[1].data.d_image;

      metadata = gimp_image_get_metadata (image_ID);

      /* Always show metadata dialog so we can add
         appropriate iptc data as needed. Sometimes
         license data needs to be added after the
         fact and the image may not contain metadata
         but should have it added as needed. */

      if (!metadata)
        {
          metadata = gimp_metadata_new();
          gimp_image_set_metadata (image_ID, metadata);
        }

      metadata_viewer_dialog (image_ID, metadata);

      status = GIMP_PDB_SUCCESS;
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static gboolean
metadata_viewer_dialog (gint32        image_id,
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

  name = gimp_image_get_name (image_id);
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

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
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
