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

#include <gexiv2/gexiv2.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "metadata-tag-object.h"
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


#define GIMP_TYPE_METADATA_VIEWER  (gimp_metadata_viewer_get_type ())
G_DECLARE_FINAL_TYPE (GimpMetadataViewer, gimp_metadata_viewer, GIMP, METADATA_VIEWER, GimpPlugIn)

struct _GimpMetadataViewer
{
  GimpPlugIn parent_instance;
};


static GList          * metadata_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * metadata_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * metadata_run              (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   GimpDrawable        **drawables,
                                                   GimpProcedureConfig  *config,
                                                   gpointer              run_data);

static gboolean  metadata_viewer_dialog           (GimpImage            *image,
                                                   GimpMetadata         *g_metadata,
                                                   GError              **error);
static void      metadata_dialog_set_metadata     (GExiv2Metadata       *metadata,
                                                   GListStore           *exif_store,
                                                   GListStore           *xmp_store,
                                                   GListStore           *iptc_store);
static GtkWidget * create_widget_for_tag_object   (gpointer              item,
                                                   gpointer              user_data);
static void metadata_dialog_add_multiple_values   (GExiv2Metadata       *metadata,
                                                   const gchar          *tag,
                                                   GListStore           *store);
static void      metadata_dialog_append_tags      (GExiv2Metadata       *metadata,
                                                   gchar               **tags,
                                                   GListStore           *store,
                                                   gboolean              load_iptc);
static void metadata_dialog_add_tag               (GListStore           *store,
                                                   const gchar          *tag,
                                                   const gchar          *value);
static void metadata_dialog_add_translated_tag    (GExiv2Metadata       *metadata,
                                                   GListStore           *store,
                                                   const gchar          *tag);
static gchar   * metadata_interpret_user_comment  (gchar                *comment);
static gchar   * metadata_dialog_format_tag_value (GExiv2Metadata       *metadata,
                                                   const gchar          *tag,
                                                   gboolean              truncate);
static gchar   * metadata_format_string_value     (const gchar          *value,
                                                   gboolean              truncate);
static inline gboolean metadata_tag_is_string     (const gchar          *tag);


G_DEFINE_TYPE (GimpMetadataViewer, gimp_metadata_viewer, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIMP_TYPE_METADATA_VIEWER)
DEFINE_STD_SET_I18N


static void
gimp_metadata_viewer_class_init (GimpMetadataViewerClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = metadata_query_procedures;
  plug_in_class->create_procedure = metadata_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gimp_metadata_viewer_init (GimpMetadataViewer *self)
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
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            metadata_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("_View Metadata"));
      gimp_procedure_add_menu_path (procedure, "<Image>/Image/Metadata");

      gimp_procedure_set_documentation (procedure,
                                        _("View metadata (Exif, IPTC, XMP)"),
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
    }

  return procedure;
}

static GimpValueArray *
metadata_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GimpDrawable        **drawables,
              GimpProcedureConfig  *config,
              gpointer              run_data)
{
  GimpMetadata *metadata;
  GError       *error  = NULL;

  gimp_ui_init (PLUG_IN_BINARY);

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

  if (metadata_viewer_dialog (image, metadata, &error))
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
  else
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_EXECUTION_ERROR, error);
}

static gboolean
metadata_viewer_dialog (GimpImage     *image,
                        GimpMetadata  *g_metadata,
                        GError       **error)
{
  gchar             *title;
  gchar             *name;
  GtkWidget         *dialog;
  GtkWidget         *content_area;
  GtkWidget         *metadata_vbox;
  GtkWidget         *notebook;
  GtkWidget         *scrolled_win;
  GtkWidget         *list_box;
  GtkWidget         *label;
  GListStore        *exif_store, *xmp_store, *iptc_store;
  GExiv2Metadata    *metadata;

  metadata = GEXIV2_METADATA(g_metadata);

  name = gimp_image_get_name (image);
  title = g_strdup_printf (_("Metadata Viewer: %s"), name);
  g_free (name);

  dialog = gimp_dialog_new (title,
                            "gimp-metadata-viewer-dialog",
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,
                            _("_Close"), GTK_RESPONSE_CLOSE,
                            NULL);

  gtk_widget_set_size_request(dialog, 650, 500);

  g_free (title);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_CLOSE,
                                           -1);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  /* Top-level Box */

  metadata_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (content_area), metadata_vbox);
  gtk_container_set_border_width (GTK_CONTAINER (metadata_vbox), 12);
  gtk_widget_show (metadata_vbox);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (metadata_vbox), notebook, TRUE, TRUE, 0);

  /* EXIF tab */

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 6);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_IN);

  exif_store = g_list_store_new (GIMP_TYPE_METADATA_TAG_OBJECT);
  list_box = gtk_list_box_new ();
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list_box),
                                   GTK_SELECTION_NONE);
  gtk_list_box_bind_model (GTK_LIST_BOX (list_box), G_LIST_MODEL (exif_store),
                           create_widget_for_tag_object, NULL, NULL);
  gtk_widget_set_vexpand (list_box, TRUE);

  label = gtk_label_new (_("Exif"));
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled_win, label);
  gtk_container_add (GTK_CONTAINER (scrolled_win), list_box);
  gtk_widget_show (list_box);
  gtk_widget_show (scrolled_win);

  /* XMP tab */

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 6);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_IN);

  xmp_store = g_list_store_new (GIMP_TYPE_METADATA_TAG_OBJECT);
  list_box = gtk_list_box_new ();
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list_box),
                                   GTK_SELECTION_NONE);
  gtk_list_box_bind_model (GTK_LIST_BOX (list_box), G_LIST_MODEL (xmp_store),
                           create_widget_for_tag_object, NULL, NULL);
  gtk_widget_set_vexpand (list_box, TRUE);

  label = gtk_label_new (_("XMP"));
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled_win, label);
  gtk_container_add (GTK_CONTAINER (scrolled_win), list_box);
  gtk_widget_show (list_box);
  gtk_widget_show (scrolled_win);

  /* IPTC tab */

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 6);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_IN);

  iptc_store = g_list_store_new (GIMP_TYPE_METADATA_TAG_OBJECT);
  list_box = gtk_list_box_new ();
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list_box),
                                   GTK_SELECTION_NONE);
  gtk_list_box_bind_model (GTK_LIST_BOX (list_box), G_LIST_MODEL (iptc_store),
                           create_widget_for_tag_object, NULL, NULL);
  gtk_widget_set_vexpand (list_box, TRUE);

  label = gtk_label_new (_("IPTC"));
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled_win, label);
  gtk_container_add (GTK_CONTAINER (scrolled_win), list_box);
  gtk_widget_show (list_box);
  gtk_widget_show (scrolled_win);

  gtk_widget_show (notebook);

  /* Add the metadata to the list models */

  metadata_dialog_set_metadata (metadata, exif_store, xmp_store, iptc_store);
  g_object_unref (exif_store);
  g_object_unref (xmp_store);
  g_object_unref (iptc_store);

  gtk_dialog_run (GTK_DIALOG (dialog));

  return TRUE;
}


/*  private functions  */

static void
metadata_dialog_set_metadata (GExiv2Metadata *metadata,
                              GListStore     *exif_store,
                              GListStore     *xmp_store,
                              GListStore     *iptc_store)
{
  gchar        **tags;

  /* load exif tags */
  tags  = gexiv2_metadata_get_exif_tags (metadata);

  metadata_dialog_append_tags (metadata, tags, exif_store, FALSE);

  g_strfreev (tags);

  /* load xmp tags */
  tags  = gexiv2_metadata_get_xmp_tags (metadata);

  metadata_dialog_append_tags (metadata, tags, xmp_store, FALSE);

  g_strfreev (tags);

  /* load iptc tags */
  tags  = gexiv2_metadata_get_iptc_tags (metadata);

  metadata_dialog_append_tags (metadata, tags, iptc_store, TRUE);

  g_strfreev (tags);
}

static GtkWidget *
create_widget_for_tag_object (gpointer item,
                              gpointer user_data)
{
  GimpMetadataTagObject *tag_obj = GIMP_METADATA_TAG_OBJECT (item);
  GtkWidget             *row;
  GtkWidget             *box;
  GtkWidget             *tag_label;
  GtkWidget             *value_label;

  row = gtk_list_box_row_new ();
  gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (row), FALSE);
  gtk_style_context_add_class (gtk_widget_get_style_context (row),
                               "metadata-tag-row");
  gtk_widget_show (row);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
  gtk_container_add (GTK_CONTAINER (row), box);
  gtk_widget_show (box);

  tag_label = gtk_label_new (gimp_metadata_tag_object_get_tag (tag_obj));
  gtk_label_set_xalign (GTK_LABEL (tag_label), 0.0);
  gtk_label_set_selectable (GTK_LABEL (tag_label), TRUE);
  gtk_style_context_add_class (gtk_widget_get_style_context (row),
                               "metadata-tag-label");
  gtk_style_context_add_class (gtk_widget_get_style_context (tag_label),
                               "dim-label");
  gtk_widget_show (tag_label);
  gtk_box_pack_start (GTK_BOX (box), tag_label, FALSE, FALSE, 0);

  value_label = gtk_label_new (gimp_metadata_tag_object_get_value (tag_obj));
  gtk_style_context_add_class (gtk_widget_get_style_context (value_label),
                               "metadata-value-label");
  gtk_label_set_xalign (GTK_LABEL (value_label), 0.0);
  gtk_label_set_selectable (GTK_LABEL (value_label), TRUE);
  gtk_widget_show (value_label);
  gtk_box_pack_start (GTK_BOX (box), value_label, FALSE, FALSE, 0);

  return row;
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
  GError      *error = NULL;

  tag_type = gexiv2_metadata_try_get_tag_type (tag, &error);
  if (error)
    {
      g_printerr ("%s: Failed to get metadata tag type for tag %s: %s",
                  G_STRFUNC, tag, error->message);
      g_clear_error (&error);

      return FALSE;
    }

  return (g_strcmp0 (tag_type, "Undefined") != 0 &&
          g_strcmp0 (tag_type, NULL)        != 0);
}

static void
metadata_dialog_add_tag (GListStore      *store,
                         const gchar     *tag,
                         const gchar     *value)
{
  GimpMetadataTagObject *tag_object;

  if (value == NULL)
    return;

  tag_object = gimp_metadata_tag_object_new (tag, value);
  g_list_store_append (store, tag_object);
  g_object_unref (tag_object);
}

static void
metadata_dialog_add_translated_tag (GExiv2Metadata  *metadata,
                                    GListStore      *store,
                                    const gchar     *tag)
{
  gchar  *value = NULL;
  GError *error = NULL;

  value = gexiv2_metadata_try_get_tag_interpreted_string (metadata, tag, &error);
  if (error)
    {
      g_printerr ("%s: unreadable '%s' metadata tag: %s\n",
                  G_STRFUNC, tag, error->message);
      g_clear_error (&error);
    }

  metadata_dialog_add_tag (store, tag, gettext (value));
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
metadata_dialog_add_multiple_values (GExiv2Metadata  *metadata,
                                     const gchar     *tag,
                                     GListStore      *store)
{
  gchar  **values;
  GError  *error = NULL;

  values = gexiv2_metadata_try_get_tag_multiple (GEXIV2_METADATA (metadata), tag, &error);
  if (error)
    {
      g_printerr ("%s: unreadable '%s' metadata tag: %s\n",
                  G_STRFUNC, tag, error->message);
      g_clear_error (&error);
    }

  if (values == NULL)
    return;

  for (gsize i = 0; values[i] != NULL; i++)
    {
      gchar       *value;

      value = metadata_format_string_value (values[i], /* truncate = */ TRUE);
      metadata_dialog_add_tag (store, tag, value);
      g_free (value);
    }

  g_strfreev (values);
}

static void
metadata_dialog_append_tags (GExiv2Metadata  *metadata,
                             gchar          **tags,
                             GListStore      *store,
                             gboolean         load_iptc)
{
  const gchar *tag;
  const gchar *last_tag = NULL;
  gboolean     gps_done = FALSE;

  while ((tag = *tags++))
    {
      gchar  *value;

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

          metadata_dialog_add_multiple_values (GEXIV2_METADATA (metadata),
                                               tag, store);
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
              GError *error = NULL;

              if (gexiv2_metadata_try_get_gps_longitude (GEXIV2_METADATA (metadata),
                                                         &lng, &error))
                {
                  str = metadata_format_gps_longitude_latitude (lng);
                  metadata_dialog_add_tag (store,
                                           "Exif.GPSInfo.GPSLongitude",
                                           str);
                  g_free (str);
                }
              else if (error)
                {
                  g_printerr ("%s: unreadable gps longitude tag: %s\n",
                              G_STRFUNC, error->message);
                  g_clear_error (&error);
                }

              metadata_dialog_add_translated_tag (metadata, store,
                                                  "Exif.GPSInfo.GPSLongitudeRef");

              if (gexiv2_metadata_try_get_gps_latitude (GEXIV2_METADATA (metadata),
                                                        &lat, &error))
                {
                  str = metadata_format_gps_longitude_latitude (lat);
                  metadata_dialog_add_tag (store,
                                           "Exif.GPSInfo.GPSLatitude",
                                           str);
                  g_free (str);
                }
              else if (error)
                {
                  g_printerr ("%s: unreadable gps latitude tag: %s\n",
                              G_STRFUNC, error->message);
                  g_clear_error (&error);
                }

              metadata_dialog_add_translated_tag (metadata, store,
                                                  "Exif.GPSInfo.GPSLatitudeRef");

              if (gexiv2_metadata_try_get_gps_altitude (GEXIV2_METADATA (metadata),
                                                        &alt, &error))
                {
                  gchar  *str2, *str3;
                  GError *error = NULL;

                  str  = metadata_format_gps_altitude (alt, TRUE,  _(" meter"));
                  str2 = metadata_format_gps_altitude (alt, FALSE, _(" feet"));
                  str3 = g_strdup_printf ("%s (%s)", str, str2);
                  metadata_dialog_add_tag (store,
                                           "Exif.GPSInfo.GPSAltitude",
                                           str3);
                  g_free (str);
                  g_free (str2);
                  g_free (str3);
                  value = gexiv2_metadata_try_get_tag_string (metadata,
                                                              "Exif.GPSInfo.GPSAltitudeRef",
                                                              &error);
                  if (error)
                    {
                      g_printerr ("%s: unreadable '%s' metadata tag: %s\n",
                                  G_STRFUNC, "Exif.GPSInfo.GPSAltitudeRef",
                                  error->message);
                      g_clear_error (&error);
                    }

                  if (value)
                    {
                      gint index;

                      if (value[0] == '0')
                        index = 1;
                      else if (value[0] == '1')
                        index = 2;
                      else
                        index = 0;
                      metadata_dialog_add_tag (store,
                                              "Exif.GPSInfo.GPSAltitudeRef",
                                              gettext (gpsaltref[index]));
                      g_free (value);
                    }
                }
              else if (error)
                {
                  g_printerr ("%s: unreadable gps altitude tag: %s\n",
                              G_STRFUNC, error->message);
                  g_clear_error (&error);
                }
              gps_done = TRUE;
            }
        }
      else if (! strcmp ("Exif.Photo.UserComment", tag))
        {
          GError *error = NULL;

          value = gexiv2_metadata_try_get_tag_interpreted_string (metadata, tag, &error);
          if (error)
            {
              g_printerr ("%s: unreadable '%s' metadata tag: %s\n",
                          G_STRFUNC, tag, error->message);
              g_clear_error (&error);
            }

          /* Can start with charset. Remove part that is not relevant. */
          value = metadata_interpret_user_comment (value);

          metadata_dialog_add_tag (store, tag, value);
          g_free (value);
        }
      else
        {
          if (g_str_has_prefix (tag, "Xmp."))
            {
              GError      *error = NULL;
              const gchar *tag_type;

              tag_type = gexiv2_metadata_try_get_tag_type (tag, &error);
              if (error)
                {
                  g_printerr ("%s: Failed to get metadata tag type for tag %s: %s",
                              G_STRFUNC, tag, error->message);
                  g_clear_error (&error);
                }
              else if (g_strcmp0 (tag_type, "XmpText") != 0)
                {
                  metadata_dialog_add_multiple_values (GEXIV2_METADATA (metadata),
                                                       tag, store);
                  continue;
                }
            }

          value = metadata_dialog_format_tag_value (metadata, tag,
                                                    /* truncate = */ TRUE);
          metadata_dialog_add_tag (store, tag, value);
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
      gchar  *value;
      gchar  *value_utf8;
      GError *error = NULL;

      value = gexiv2_metadata_try_get_tag_interpreted_string (metadata, tag, &error);
      if (error)
        {
          g_printerr ("%s: unreadable '%s' metadata tag: %s\n",
                      G_STRFUNC, tag, error->message);
          g_clear_error (&error);
        }

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
      GError       *error = NULL;

      bytes = gexiv2_metadata_try_get_tag_raw (metadata, tag, &error);
      if (error)
        {
          g_printerr ("%s: unreadable '%s' metadata tag: %s\n",
                      G_STRFUNC, tag, error->message);
          g_clear_error (&error);
        }

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
