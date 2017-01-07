/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * iptc.c
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


#define PLUG_IN_IMAGE_PROC     "plug-in-image-iptc-editor"
#define PLUG_IN_LAYER_PROC     "plug-in-layer-iptc-editor"
#define PLUG_IN_HELP           "plug-in-iptc-editor"
#define PLUG_IN_BINARY         "iptc"

#define THUMB_SIZE 48

#define IPTC_PREFIX "Iptc."

typedef struct
{
  gchar *tag;
  gchar *mode;
} iptc_tag;

typedef enum
{
  ATT_IMAGE,
  ATT_LAYER,
  ATT_CHANNEL
} AttributesSource;

/*  local function prototypes  */

static void              query                      (void);
static void              run                        (const gchar           *name,
                                                     gint                   nparams,
                                                     const GimpParam       *param,
                                                     gint                  *nreturn_vals,
                                                     GimpParam            **return_vals);

static gboolean        iptc_dialog                  (gint32                 image_id,
                                                     GimpMetadata          *metadata,
                                                     AttributesSource       source,
                                                     GtkBuilder            *builder);

static void            iptc_dialog_set_iptc         (GimpMetadata          *metadata,
                                                     GtkBuilder            *builder);

static void            iptc_change_iptc             (GimpMetadata          *metadata,
                                                     GtkBuilder            *builder);


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
  static const GimpParamDef image_attribs_args[] =
  {
    { GIMP_PDB_INT32, "run-mode", "Run mode { RUN-INTERACTIVE (0) }" },
    { GIMP_PDB_IMAGE, "image",    "Input image"                      }
  };

  static const GimpParamDef layer_attribs_args[] =
  {
    { GIMP_PDB_INT32, "run-mode", "Run mode { RUN-INTERACTIVE (0) }" },
    { GIMP_PDB_IMAGE, "image",    "Input image"                      },
    { GIMP_PDB_LAYER, "layer",    "Input layer"                      }
  };

  gimp_install_procedure (PLUG_IN_IMAGE_PROC,
                          N_("View and edit IPTC data"),
                          "View and edit IPTC information attached to the "
                          "current image. This iptc will be saved in the "
                          "file, depending on the output file format.",
                          "Hartmut Kuhse",
                          "Hartmut Kuhse",
                          "2014",
                          N_("Edit IPTC Metadata"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (image_attribs_args), 0,
                          image_attribs_args, NULL);

  gimp_plugin_menu_register (PLUG_IN_IMAGE_PROC, "<Image>/Image");

  gimp_install_procedure (PLUG_IN_LAYER_PROC,
                          N_("View and edit IPTC data"),
                          "View and edit IPTC information attached to the "
                          "current layer. This iptc will be saved in the "
                          "xcf file.",
                          "Hartmut Kuhse",
                          "Hartmut Kuhse",
                          "2014",
                          N_("Edit layers IPTC Metadata"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (layer_attribs_args), 0,
                          layer_attribs_args, NULL);

  gimp_plugin_menu_register (PLUG_IN_LAYER_PROC, "<Layers>");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpMetadata      *metadata;
  GtkBuilder        *builder;
  AttributesSource   source;
  gint32             item_ID;
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  INIT_I18N();
  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  if (! strcmp (name, PLUG_IN_IMAGE_PROC))
    {
      item_ID = param[1].data.d_image;

      metadata = gimp_image_get_metadata (item_ID);
      source = ATT_IMAGE;

      status = GIMP_PDB_SUCCESS;
    }
  else if (! strcmp (name, PLUG_IN_LAYER_PROC))
    {
      item_ID = param[2].data.d_layer;

      metadata = gimp_item_get_metadata (item_ID);
      source = ATT_LAYER;

      status = GIMP_PDB_SUCCESS;
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (! metadata)
    {
      metadata = gimp_metadata_new();
    }

  builder = gtk_builder_new ();

  if (status == GIMP_PDB_SUCCESS && iptc_dialog (item_ID, metadata, source, builder))
    {

      iptc_change_iptc (metadata, builder);

      if (! strcmp (name, PLUG_IN_IMAGE_PROC))
        {
          gimp_image_set_metadata (item_ID, metadata);
        }
      else if (! strcmp (name, PLUG_IN_LAYER_PROC))
        {
          gimp_item_set_metadata (item_ID, metadata);
        }
      status = GIMP_PDB_SUCCESS;
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  g_object_unref (metadata);

  values[0].data.d_status = status;
}

static gboolean
iptc_dialog (gint32            item_id,
             GimpMetadata     *metadata,
             AttributesSource  source,
             GtkBuilder       *builder)
{
  GtkWidget  *dialog;
  GtkWidget  *iptc_vbox;
  GtkWidget  *content_area;
  GdkPixbuf  *pixbuf;
  GtkWidget  *label_header;
  GtkWidget  *label_info;
  GtkWidget  *thumb_box;
  gchar      *ui_file;
  gchar      *title;
  gchar      *fname;
  gchar      *header;
  gchar      *role;
  GError     *error = NULL;
  gboolean    run;


  switch (source)
  {
    case ATT_IMAGE:
      fname = gimp_image_get_name (item_id);
      header  = g_strdup_printf ("Image");
      title = g_strdup_printf ("Image: %s", fname);
      role  = g_strdup_printf ("gimp-image-iptc-dialog");
      pixbuf = gimp_image_get_thumbnail (item_id, THUMB_SIZE, THUMB_SIZE,
                                         GIMP_PIXBUF_SMALL_CHECKS);

      break;
    case ATT_LAYER:
      fname = gimp_item_get_name (item_id);
      header  = g_strdup_printf ("Layer");
      title = g_strdup_printf ("Layer: %s", fname);
      role  = g_strdup_printf ("gimp-layer-iptc-dialog");
      pixbuf = gimp_drawable_get_thumbnail (item_id, THUMB_SIZE, THUMB_SIZE,
                                            GIMP_PIXBUF_SMALL_CHECKS);
      break;
    default:
      fname = g_strdup_printf ("unknown");
      header  = g_strdup_printf ("Unknown");
      title = g_strdup_printf ("Metadata  : %s", fname);
      role  = g_strdup_printf ("gimp-iptc-dialog");
      pixbuf = NULL;
      break;
  }

  ui_file = g_build_filename (gimp_data_directory (),
                              "ui", "plug-ins", "plug-in-iptc.ui", NULL);

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

  dialog = gimp_dialog_new (title,
                            role,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_HELP,
                            
                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                            GTK_STOCK_OK,    GTK_RESPONSE_OK,

                            NULL);

  g_free (title);
  g_free (role);

  gtk_window_set_default_size (GTK_WINDOW (dialog),
                               600,
                               -1);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CLOSE,
                                           -1);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  iptc_vbox = GTK_WIDGET (gtk_builder_get_object (builder,
                                                  "iptc-vbox"));
  gtk_container_set_border_width (GTK_CONTAINER (iptc_vbox), 2);
  gtk_box_pack_start (GTK_BOX (content_area), iptc_vbox, TRUE, TRUE, 0);

  label_header = GTK_WIDGET (gtk_builder_get_object (builder, "label-header"));
  gimp_label_set_attributes (GTK_LABEL (label_header),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_label_set_text (GTK_LABEL (label_header), header);

  label_info = GTK_WIDGET (gtk_builder_get_object (builder, "label-info"));
  gimp_label_set_attributes (GTK_LABEL (label_info),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_label_set_text (GTK_LABEL (label_info), fname);

  g_free (header);
  g_free (fname);

  if (pixbuf)
    {
      GtkWidget *image;

      thumb_box = GTK_WIDGET (gtk_builder_get_object (builder, "thumb-box"));

      if (thumb_box)
        {
          image = gtk_image_new_from_pixbuf (pixbuf);
          gtk_box_pack_end (GTK_BOX (thumb_box), image, FALSE, FALSE, 0);
          gtk_widget_show (image);
        }
    }

  iptc_dialog_set_iptc (metadata, builder);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  return run;
}


/*  private functions  */

static void
iptc_dialog_set_iptc (GimpMetadata   *metadata,
                      GtkBuilder     *builder)
{
  gchar         *value;
  gint           i;

  for (i = 0; i < G_N_ELEMENTS (iptc_tags); i++)
    {
      GimpAttribute *attribute = NULL;
      GtkWidget     *widget    = NULL;

      attribute = gimp_metadata_get_attribute (metadata, iptc_tags[i].tag);
      widget = GTK_WIDGET (gtk_builder_get_object (builder, iptc_tags[i].tag));

      if (attribute && widget)
        {
          value = gimp_attribute_get_string (attribute);

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
}

static void
iptc_change_iptc (GimpMetadata   *metadata,
                  GtkBuilder     *builder)
{
  gint            i;

  for (i = 0; i < G_N_ELEMENTS (iptc_tags); i++)
    {
      GimpAttribute *attribute;

      GObject *object = gtk_builder_get_object (builder,
                                                iptc_tags[i].tag);

      if (! strcmp ("single", iptc_tags[i].mode))
        {
          GtkEntry     *entry = GTK_ENTRY (object);
          const gchar  *text  = gtk_entry_get_text (entry);

          if (strcmp ("", text))
            {
              attribute = gimp_attribute_new_string (iptc_tags[i].tag, (gchar *) text, TYPE_UNICODE);
              gimp_metadata_add_attribute (metadata, attribute);
            }
          else
            {
              gimp_metadata_remove_attribute (metadata, iptc_tags[i].tag);
            }
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
          
          if (strcmp ("", text))
            {
              attribute = gimp_attribute_new_string (iptc_tags[i].tag, (gchar *) text, TYPE_MULTIPLE);
              gimp_metadata_add_attribute (metadata, attribute);
            }
          else
            {
              gimp_metadata_remove_attribute (metadata, iptc_tags[i].tag);
            }

          g_free (text);
        }
    }
}
