/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * metadata.c
 * Copyright (C) 2013 Hartmut Kuhse
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gexiv2/gexiv2.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-metadata-editor"
#define PLUG_IN_BINARY "metadata"
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


typedef struct
{
  gchar *tag;
  gchar *mode;
} iptc_tag;


/*  local function prototypes  */

static void       query                            (void);
static void       run                              (const gchar      *name,
                                                    gint              nparams,
                                                    const GimpParam  *param,
                                                    gint             *nreturn_vals,
                                                    GimpParam       **return_vals);

static gboolean   metadata_dialog                  (gint32            image_id,
                                                    GExiv2Metadata   *metadata);

static void       metadata_dialog_set_metadata     (GExiv2Metadata   *metadata,
                                                    GtkBuilder       *builder);

static void       metadata_dialog_append_tags      (GExiv2Metadata  *metadata,
                                                    gchar          **tags,
                                                    GtkListStore    *store,
                                                    gint             tag_column,
                                                    gint             value_column);

static gchar    * metadata_dialog_format_tag_value (GExiv2Metadata  *metadata,
                                                    const gchar     *tag,
                                                    gboolean         truncate);

static void       metadata_dialog_iptc_callback    (GtkWidget      *dialog,
                                                    GtkBuilder     *builder);


/* local variables */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static const iptc_tag const iptc_tags[] =
{
  { "Iptc.Application2.Byline",                  "single" },
  { "Iptc.Application2.BylineTitle",             "single" },
  { "Iptc.Application2.Caption",                 "multi"  },
  { "Iptc.Application2.Category",                "single" },
  { "Iptc.Application2.City",                    "single" },
  { "Iptc.Application2.Copyright",               "single" },
  { "Iptc.Application2.CountryName",             "single" },
  { "Iptc.Application2.Credit",                  "single" },
  { "Iptc.Application2.Headline",                "multi"  },
  { "Iptc.Application2.Keywords",                "multi"  },
  { "Iptc.Application2.ObjectName",              "single" },
  { "Iptc.Application2.ProvinceState",           "single" },
  { "Iptc.Application2.Source",                  "single" },
  { "Iptc.Application2.SpecialInstructions",     "multi"  },
  { "Iptc.Application2.SubLocation",             "single" },
  { "Iptc.Application2.SuppCategory",            "multi"  },
  { "Iptc.Application2.TransmissionReference",   "single" },
  { "Iptc.Application2.Urgency",                 "single" },
  { "Iptc.Application2.Writer",                  "single" }
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
                          N_("View and edit metadata (Exif, IPTC, XMP)"),
                          "View and edit metadata information attached to the "
                          "current image.  This can include Exif, IPTC and/or "
                          "XMP information.  Some or all of this metadata "
                          "will be saved in the file, depending on the output "
                          "file format.",
                          "Hartmut Kuhse, Michael Natterer",
                          "Hartmut Kuhse, Michael Natterer",
                          "2013",
                          N_("Image Metadata"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (metadata_args), 0,
                          metadata_args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Image");
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

      if (metadata)
        {
          metadata_dialog (image_ID, metadata);
        }
      else
        {
          g_message (_("This image has no metadata attached to it."));
        }

      status = GIMP_PDB_SUCCESS;
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static gboolean
metadata_dialog (gint32          image_id,
                 GExiv2Metadata *metadata)
{
  GtkBuilder *builder;
  GtkWidget  *dialog;
  GtkWidget  *metadata_vbox;
  GtkWidget  *content_area;
  GObject    *write_button;
  gchar      *ui_file;
  gchar      *title;
  gchar      *name;
  GError     *error = NULL;

  builder = gtk_builder_new ();

  ui_file = g_build_filename (gimp_data_directory (),
                              "ui", "plug-ins", "plug-in-metadata.ui", NULL);

  if (! gtk_builder_add_from_file (builder, ui_file, &error))
    {
      g_printerr ("Error occured while loading UI file!\n");
      g_printerr ("Message: %s\n", error->message);
      g_clear_error (&error);
      g_free (ui_file);
      g_object_unref (builder);
      return FALSE;
    }

  g_free (ui_file);

  name = gimp_image_get_name (image_id);
  title = g_strdup_printf ("Metadata: %s", name);
  g_free (name);

  dialog = gimp_dialog_new (title,
                            "gimp-metadata-dialog",
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                            NULL);

  g_free (title);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_CLOSE,
                                           -1);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  metadata_vbox = GTK_WIDGET (gtk_builder_get_object (builder,
                                                      "metadata-vbox"));
  gtk_container_set_border_width (GTK_CONTAINER (metadata_vbox), 12);
  gtk_box_pack_start (GTK_BOX (content_area), metadata_vbox, TRUE, TRUE, 0);

  write_button = gtk_builder_get_object (builder, "iptc-write-button");

  g_signal_connect (write_button, "clicked",
                    G_CALLBACK (metadata_dialog_iptc_callback),
                    builder);

  metadata_dialog_set_metadata (metadata, builder);

  gtk_dialog_run (GTK_DIALOG (dialog));

  return TRUE;
}


/*  private functions  */

static void
metadata_dialog_set_metadata (GExiv2Metadata *metadata,
                              GtkBuilder     *builder)
{
  GtkListStore  *exif_store;
  GtkListStore  *xmp_store;
  gchar        **exif_data;
  gchar        **xmp_data;
  gchar        **iptc_data;
  gchar         *value;
  gint           i;

  exif_data  = gexiv2_metadata_get_exif_tags (metadata);
  exif_store = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                       "exif-liststore"));

  metadata_dialog_append_tags (metadata, exif_data,
                               exif_store, C_EXIF_TAG, C_EXIF_VALUE);

  xmp_data  = gexiv2_metadata_get_xmp_tags (metadata);
  xmp_store = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                       "xmp-liststore"));

  metadata_dialog_append_tags (metadata, xmp_data,
                               xmp_store, C_XMP_TAG, C_XMP_VALUE);

  iptc_data = gexiv2_metadata_get_iptc_tags (metadata);

  for (i = 0; iptc_data[i] != NULL; i++)
    {
      GtkWidget *widget;

      widget = GTK_WIDGET (gtk_builder_get_object (builder, iptc_data[i]));

      value = metadata_dialog_format_tag_value (metadata, iptc_data[i],
                                                /* truncate = */ FALSE);

      if (GTK_IS_ENTRY (widget))
        {
          GtkEntry *entry_widget = GTK_ENTRY (widget);

          gtk_entry_set_text (entry_widget, value);
        }
      else if (GTK_IS_TEXT_VIEW (widget))
        {
          GtkTextView   *text_view = GTK_TEXT_VIEW (widget);
          GtkTextBuffer *buffer;

          buffer = gtk_text_view_get_buffer (text_view);
          gtk_text_buffer_set_text (buffer, value, -1);
        }

      g_free (value);
    }
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
      const gchar *tag_label;
      gchar       *value;

      tag_label = gexiv2_metadata_get_tag_label (tag);

      /* skip private tags */
      if (g_strcmp0 (tag_label, "") == 0 || g_strcmp0 (tag_label, NULL) == 0)
        continue;

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

static void
metadata_dialog_iptc_callback (GtkWidget  *dialog,
                               GtkBuilder *builder)
{
#if 0
  GExiv2Metadata *metadata;
  gint            i;

  metadata = gimp_image_get_metadata (handler->image);

  for (i = 0; i < G_N_ELEMENTS (iptc_tags); i++)
    {
      GObject *object = gtk_builder_get_object (handler->builder,
                                                iptc_tags[i].tag);

      if (! strcmp ("single", iptc_tags[i].mode))
        {
          GtkEntry *entry = GTK_ENTRY (object);

          gexiv2_metadata_set_tag_string (metadata, iptc_tags[i].tag,
                                          gtk_entry_get_text (entry));
        }
      else  if (!strcmp ("multi", iptc_tags[i].mode))
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
          gexiv2_metadata_set_tag_string (metadata, iptc_tags[i].tag, text);
          g_free (text);
        }
    }
#endif
}
