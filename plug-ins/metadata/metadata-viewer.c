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

#include "metadata-tags.h"

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
                                                    GimpMetadata   *g_metadata,
                                                    GError        **error);
static void       metadata_dialog_set_metadata     (GExiv2Metadata *metadata,
                                                    GtkBuilder     *builder);
static void       metadata_dialog_append_tags      (GExiv2Metadata  *metadata,
                                                    gchar          **tags,
                                                    GtkListStore    *store,
                                                    gint             tag_column,
                                                    gint             value_column,
                                                    gboolean         load_iptc);
static void metadata_dialog_add_tag                (GtkListStore    *store,
                                                    GtkTreeIter      iter,
                                                    gint             tag_column,
                                                    gint             value_column,
                                                    const gchar     *tag,
                                                    const gchar     *value);
static void metadata_dialog_add_translated_tag     (GExiv2Metadata  *metadata,
                                                    GtkListStore    *store,
                                                    GtkTreeIter      iter,
                                                    gint             tag_column,
                                                    gint             value_column,
                                                    const gchar     *tag);
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
  static GimpParam   values[2];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GError            *error  = NULL;

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

      if (metadata_viewer_dialog (image_ID, metadata, &error))
        status = GIMP_PDB_SUCCESS;
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
          if (error)
            {
             *nreturn_vals = 2;
              values[1].type          = GIMP_PDB_STRING;
              values[1].data.d_string = error->message;
            }
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static gboolean
metadata_viewer_dialog (gint32         image_id,
                        GimpMetadata  *g_metadata,
                        GError       **error)
{
  GtkBuilder     *builder;
  GtkWidget      *dialog;
  GtkWidget      *metadata_vbox;
  GtkWidget      *content_area;
  gchar          *ui_file;
  gchar          *title;
  gchar          *name;
  GError         *local_error = NULL;
  GExiv2Metadata *metadata;

  metadata = GEXIV2_METADATA(g_metadata);

  builder = gtk_builder_new ();

  ui_file = g_build_filename (gimp_data_directory (),
                              "ui", "plug-ins", "plug-in-metadata-viewer.ui", NULL);

  if (! gtk_builder_add_from_file (builder, ui_file, &local_error))
    {
      if (! local_error)
        local_error = g_error_new_literal (G_FILE_ERROR, 0,
                                           _("Error loading metadata-viewer dialog."));
      g_propagate_error (error, local_error);

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

  metadata_dialog_append_tags (metadata, tags, store, C_EXIF_TAG, C_EXIF_VALUE, FALSE);

  g_strfreev (tags);

  /* load xmp tags */
  tags  = gexiv2_metadata_get_xmp_tags (metadata);
  store = GTK_LIST_STORE (gtk_builder_get_object (builder, "xmp-liststore"));

  metadata_dialog_append_tags (metadata, tags, store, C_XMP_TAG, C_XMP_VALUE, FALSE);

  g_strfreev (tags);

  /* load iptc tags */
  tags  = gexiv2_metadata_get_iptc_tags (metadata);
  store = GTK_LIST_STORE (gtk_builder_get_object (builder, "iptc-liststore"));

  metadata_dialog_append_tags (metadata, tags, store, C_IPTC_TAG, C_IPTC_VALUE, TRUE);

  g_strfreev (tags);
}

static gchar *
metadata_format_string_value (const gchar *value,
                              gboolean     truncate)
{
  glong  size;
  gchar *result;

  size = g_utf8_strlen (value, -1);

  if (! truncate || size <= TAG_VALUE_MAX_SIZE)
    {
      result = g_strdup(value);
    }
  else
    {
      gchar   *value_utf8_trunc;
      GString *str;

      value_utf8_trunc = g_utf8_substring (value, 0, TAG_VALUE_MAX_SIZE);
      str = g_string_new (value_utf8_trunc);

      g_free (value_utf8_trunc);

      g_string_append (str, "... ");
      g_string_append_printf (str,
                              _("(%lu more character(s))"),
                              size - TAG_VALUE_MAX_SIZE);

      result = g_string_free (str, FALSE);
    }
  return result;
}

static inline gboolean
metadata_tag_is_string (const gchar *tag)
{
  const gchar *tag_type;

  tag_type = gexiv2_metadata_get_tag_type (tag);

  return (g_strcmp0 (tag_type, "Byte")      != 0 &&
          g_strcmp0 (tag_type, "Undefined") != 0 &&
          g_strcmp0 (tag_type, NULL)        != 0);
}

static void
metadata_dialog_add_tag (GtkListStore    *store,
                         GtkTreeIter      iter,
                         gint             tag_column,
                         gint             value_column,
                         const gchar     *tag,
                         const gchar     *value)
{
  if (value)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          tag_column,   tag,
                          value_column, value,
                          -1);
    }
}

static void
metadata_dialog_add_translated_tag (GExiv2Metadata  *metadata,
                                    GtkListStore    *store,
                                    GtkTreeIter      iter,
                                    gint             tag_column,
                                    gint             value_column,
                                    const gchar     *tag)
{
  gchar *value;

  value = gexiv2_metadata_get_tag_interpreted_string (metadata, tag);
  metadata_dialog_add_tag (store, iter, tag_column, value_column,
                           tag, gettext (value));
  g_free (value);
}

static gchar *
metadata_interpret_user_comment (gchar *comment)
{
  /* Exiv2 can return unwanted text at the start of a comment
   * taken from Exif.Photo.UserComment since 0.27.3.
   * Let's remove that part and replace with an empty string
   * if there is nothing else left as comment. */

  if (comment && g_str_has_prefix (comment, "charset=Ascii "))
    {
      gchar *real_comment;

      /* Skip "charset=Ascii " (length 14) to find the real comment */
      real_comment = comment + 14;
      if (real_comment[0] == '\0' ||
          ! g_strcmp0 (real_comment, "binary comment"))
        {
          g_free (comment);
          /* Make empty comment instead of returning NULL or else
           * the exif value will not be shown at all. */
          comment = g_strdup ("");
        }
      else
        {
          real_comment = g_strdup (real_comment);
          g_free (comment);
          return real_comment;
        }
    }

  return comment;
}

static void
metadata_dialog_append_tags (GExiv2Metadata  *metadata,
                             gchar          **tags,
                             GtkListStore    *store,
                             gint             tag_column,
                             gint             value_column,
                             gboolean         load_iptc)
{
  GtkTreeIter  iter;
  const gchar *tag;
  const gchar *last_tag = NULL;
  gboolean     gps_done = FALSE;

  while ((tag = *tags++))
    {
      gchar  *value;
      gchar **values;

      /* We need special handling for iptc tags like "Keywords" which
       * can appear multiple times. For now assuming that this can
       * only happen for iptc tags of String and related types.
       * See also: https://exiv2.org/iptc.html which only lists
       * one Date type as repeatable (Iptc.Application2.ReferenceDate),
       * and Date is handled here as string.
       */
      if (load_iptc && metadata_tag_is_string (tag))
        {
          if (last_tag && ! strcmp (tag, last_tag))
            {
              continue;
            }
          last_tag = tag;

          values = gexiv2_metadata_get_tag_multiple (GEXIV2_METADATA (metadata),
                                                     tag);

          if (values)
            {
              gint i;

              for (i = 0; values[i] != NULL; i++)
                {
                  gtk_list_store_append (store, &iter);

                  value = metadata_format_string_value (values[i], /* truncate = */ TRUE);

                  gtk_list_store_set (store, &iter,
                                      tag_column,   tag,
                                      value_column, value,
                                      -1);

                  g_free (value);
                }

              g_strfreev (values);
            }
        }
      else if (! strcmp ("Exif.GPSInfo.GPSLongitude",    tag) ||
               ! strcmp ("Exif.GPSInfo.GPSLongitudeRef", tag) ||
               ! strcmp ("Exif.GPSInfo.GPSLatitude",     tag) ||
               ! strcmp ("Exif.GPSInfo.GPSLatitudeRef",  tag) ||
               ! strcmp ("Exif.GPSInfo.GPSAltitude",     tag) ||
               ! strcmp ("Exif.GPSInfo.GPSAltitudeRef",  tag))
        {
          /* We need special handling for some of the GPS tags to
           * be able to show better values than the default. */
          if (! gps_done)
            {
              gdouble  lng, lat, alt;
              gchar   *str;
              gchar  *value;

              if (gexiv2_metadata_get_gps_longitude (GEXIV2_METADATA (metadata),
                                                     &lng))
                {
                  str = metadata_format_gps_longitude_latitude (lng);
                  metadata_dialog_add_tag (store, iter,
                                           tag_column, value_column,
                                           "Exif.GPSInfo.GPSLongitude",
                                           str);
                  g_free (str);
                }
              metadata_dialog_add_translated_tag (metadata, store, iter,
                                                  tag_column, value_column,
                                                  "Exif.GPSInfo.GPSLongitudeRef");

              if (gexiv2_metadata_get_gps_latitude (GEXIV2_METADATA (metadata),
                                                    &lat))
                {
                  str = metadata_format_gps_longitude_latitude (lat);
                  metadata_dialog_add_tag (store, iter,
                                           tag_column, value_column,
                                           "Exif.GPSInfo.GPSLatitude",
                                           str);
                  g_free (str);
                }
              metadata_dialog_add_translated_tag (metadata, store, iter,
                                                  tag_column, value_column,
                                                  "Exif.GPSInfo.GPSLatitudeRef");

              if (gexiv2_metadata_get_gps_altitude (GEXIV2_METADATA (metadata),
                                                    &alt))
                {
                  gchar *str2, *str3;

                  str  = metadata_format_gps_altitude (alt, TRUE,  _(" meter"));
                  str2 = metadata_format_gps_altitude (alt, FALSE, _(" feet"));
                  str3 = g_strdup_printf ("%s (%s)", str, str2);
                  metadata_dialog_add_tag (store, iter,
                                           tag_column, value_column,
                                           "Exif.GPSInfo.GPSAltitude",
                                           str3);
                  g_free (str);
                  g_free (str2);
                  g_free (str3);
                  value = gexiv2_metadata_get_tag_string (metadata,
                                                          "Exif.GPSInfo.GPSAltitudeRef");

                  if (value)
                    {
                      gint index;

                      if (value[0] == '0')
                        index = 1;
                      else if (value[0] == '1')
                        index = 2;
                      else
                        index = 0;
                      metadata_dialog_add_tag (store, iter,
                                              tag_column, value_column,
                                              "Exif.GPSInfo.GPSAltitudeRef",
                                              gettext (gpsaltref[index]));
                      g_free (value);
                    }
                }
              gps_done = TRUE;
            }
        }
      else if (! strcmp ("Exif.Photo.UserComment", tag))
        {
          value = gexiv2_metadata_get_tag_interpreted_string (metadata, tag);
          /* Can start with charset. Remove part that is not relevant. */
          value = metadata_interpret_user_comment (value);

          metadata_dialog_add_tag (store, iter,
                                   tag_column, value_column,
                                   tag, value);
          g_free (value);
        }
      else
        {
          value = metadata_dialog_format_tag_value (metadata, tag,
                                                    /* truncate = */ TRUE);
          metadata_dialog_add_tag (store, iter,
                                   tag_column, value_column,
                                   tag, value);
          g_free (value);
        }
    }
}

static gchar *
metadata_dialog_format_tag_value (GExiv2Metadata *metadata,
                                  const gchar    *tag,
                                  gboolean        truncate)
{
  gchar       *result;

  if (metadata_tag_is_string(tag))
    {
      gchar *value;
      gchar *value_utf8;

      value      = gexiv2_metadata_get_tag_interpreted_string (metadata, tag);
      if (! g_utf8_validate (value, -1, NULL))
        {
          value_utf8 = g_locale_to_utf8 (value, -1, NULL, NULL, NULL);
        }
      else
        {
          value_utf8 = g_strdup (value);
        }

      g_free (value);

      result = metadata_format_string_value (value_utf8, truncate);
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
