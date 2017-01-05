/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * attributes.c
 * Copyright (C) 2014 Hartmut Kuhse <hatti@gimp.org>
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
 *
 * additions: share/gimp/2.0/ui/plug-ins/plug-in-attributes.ui
 * This plug-in reads in a (libgimpbase/gimpattributes.c) object and
 * presents it:
 * as GtkTreeView lists
 * in a GtkNotebook.
 * The GtkNotebook pages represents the type of attributes, as "exif",
 * "xmp", "iptc" or more.
 * The expanders of the GtkTreeView represents the IFD (Exif) or Key (XMP, IPTC).
 * of the attribute.
 * The list entry of the GtkTreeView represents the tag of the attribute.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#include <gio/gio.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include <gtk/gtk.h>
#include <gexiv2/gexiv2.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimpbase/gimpbase.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_IMAGE_PROC     "plug-in-image-attributes-viewer"
#define PLUG_IN_LAYER_PROC     "plug-in-layer-attributes-viewer"
#define PLUG_IN_HELP           "plug-in-attributes-viewer"
#define PLUG_IN_BINARY         "attributes"

#define THUMB_SIZE       48
#define RESPONSE_EXPORT   1
#define RESPONSE_GPS      3

typedef enum
{
  ATT_IMAGE,
  ATT_LAYER,
  ATT_CHANNEL
} AttributesSource;

enum
{
  C_IFD = 0,
  C_TAG,
  C_VALUE,
  C_SORT,
  NUM_COL
};

/*  local function prototypes  */

static void            query                              (void);
static void            run                                (const gchar      *name,
                                                           gint              nparams,
                                                           const GimpParam  *param,
                                                           gint             *nreturn_vals,
                                                           GimpParam       **return_vals);
static void            attributes_dialog_response         (GtkWidget        *widget,
                                                           gint              response_id,
                                                           gpointer          data);
static gboolean        attributes_dialog                  (gint32            image_id,
                                                           AttributesSource  source,
                                                           GimpMetadata   *metadata);
static void            attributes_dialog_set_attributes   (GimpMetadata   *metadata,
                                                           GtkBuilder       *builder);
static GtkTreeIter    *attributes_get_parent              (const gchar      *type,
                                                           const gchar      *name);
static void            attributes_add_parent              (const gchar      *type,
                                                           const gchar      *name,
                                                           GtkTreeIter      *iter);
static GtkTreeStore   *attributes_notebook_page_get       (GtkWidget        *notebook,
                                                           const gchar      *name);
static GtkWidget      *attributes_treeview_new            (void);
static void            attributes_file_export_dialog      (GtkWidget        *parent,
                                                           GimpMetadata   *metadata);
static void            attributes_export_dialog_response  (GtkWidget        *dlg,
                                                           gint              response_id,
                                                           gpointer          data);
static void            attributes_show_gps                (GtkWidget        *parent,
                                                           GimpMetadata   *metadata);

static void            attributes_message_dialog          (GtkMessageType    type,
                                                           GtkWindow        *parent,
                                                           const gchar      *title,
                                                           const gchar      *message);

/* local variables */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static GHashTable *ifd_table = NULL;
static GHashTable *tab_table = NULL;

static gchar      *item_name = NULL;

static GtkWidget   *gps_button;
static gdouble      gps_latitude       = 0.0;
static gdouble      gps_longitude      = 0.0;
static gchar       *gps_latitude_ref   = "N";
static gchar       *gps_longitude_ref  = "W";

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
                          N_("View image metadata (Exif, IPTC, XMP, ...)"),
                          "View metadata information attached to the "
                          "current image.  This can include Exif, IPTC, "
                          "XMP and/or other information. Some or all of these "
                          "metadata will be saved in the file, depending on "
                          "the output file format.",
                          "Hartmut Kuhse",
                          "Hartmut Kuhse",
                          "2014",
                          N_("Image Metadata"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (image_attribs_args), 0,
                          image_attribs_args, NULL);

  gimp_plugin_menu_register (PLUG_IN_IMAGE_PROC, "<Image>/Image");
  gimp_plugin_icon_register (PLUG_IN_IMAGE_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_STOCK_IMAGE_METADATA); 

  gimp_install_procedure (PLUG_IN_LAYER_PROC,
                          N_("View layer metadata (Exif, IPTC, XMP, ...)"),
                          "View metadata information attached to the "
                          "current layer.  This can include Exif, IPTC, "
                          "XMP and/or other information.",
                          "Hartmut Kuhse",
                          "Hartmut Kuhse",
                          "2014",
                          N_("Layer Metadata"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (layer_attribs_args), 0,
                          layer_attribs_args, NULL);

  gimp_plugin_menu_register (PLUG_IN_LAYER_PROC, "<Layers>");
  gimp_plugin_icon_register (PLUG_IN_LAYER_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_STOCK_IMAGE_METADATA); 

}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpMetadata   *metadata;
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

  if (metadata)
    {
      ifd_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
      tab_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
      attributes_dialog (item_ID, source, metadata);
    }
  else
    {
      g_message (_("This image has no attributes attached to it."));
    }

  if (metadata)
    g_object_unref (metadata);

  if (ifd_table)
    g_hash_table_unref (ifd_table);

  values[0].data.d_status = status;
}

static void
attributes_dialog_response (GtkWidget *widget,
                            gint       response_id,
                            gpointer   data)
{

  GimpMetadata *metadata = (GimpMetadata *) data;
  switch (response_id)
    {
    case RESPONSE_EXPORT:
      attributes_file_export_dialog (widget, metadata);
      break;
    case RESPONSE_GPS:
      attributes_show_gps (widget, metadata);
      break;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

static gboolean
attributes_dialog (gint32           item_id,
                   AttributesSource source,
                   GimpMetadata  *metadata)
{
  GtkBuilder *builder;
  GtkWidget  *dialog;
  GtkWidget  *attributes_vbox;
  GtkWidget  *content_area;
  GdkPixbuf  *pixbuf;
  GtkWidget  *label_header;
  GtkWidget  *label_info;
  GtkWidget  *thumb_box;

  gchar      *header;
  gchar      *ui_file;
  gchar      *title;
  gchar      *role;
  GError     *error = NULL;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  switch (source)
  {
    case ATT_IMAGE:
      item_name = g_filename_display_basename (gimp_image_get_uri (item_id));
      header  = g_strdup_printf ("Image");
      role  = g_strdup_printf ("gimp-image-attributes-dialog");
      pixbuf = gimp_image_get_thumbnail (item_id, THUMB_SIZE, THUMB_SIZE,
                                         GIMP_PIXBUF_SMALL_CHECKS);

      break;
    case ATT_LAYER:
      item_name = gimp_item_get_name (item_id);
      header  = g_strdup_printf ("Layer");
      role  = g_strdup_printf ("gimp-layer-attributes-dialog");
      pixbuf = gimp_drawable_get_thumbnail (item_id, THUMB_SIZE, THUMB_SIZE,
                                            GIMP_PIXBUF_SMALL_CHECKS);
      break;
    default:
      item_name = g_strdup_printf ("unknown");
      header  = g_strdup_printf ("Unknown");
      role  = g_strdup_printf ("gimp-attributes-dialog");
      pixbuf = NULL;
      break;
  }

  title = g_strdup_printf ("Attributes: %s", item_name);

  builder = gtk_builder_new ();

  ui_file = g_build_filename (gimp_data_directory (),
                              "ui", "plug-ins", "plug-in-attributes.ui", NULL);

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
                            _("_Export XMP..."), RESPONSE_EXPORT,
                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                            NULL);

  gps_button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Show GPS..."), RESPONSE_GPS);
  gtk_widget_set_sensitive (GTK_WIDGET (gps_button), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (attributes_dialog_response),
                    metadata);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  g_free (title);
  g_free (role);

  gtk_window_set_default_size (GTK_WINDOW (dialog),
                               650,
                               550);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_GPS,
                                           RESPONSE_EXPORT,
                                           GTK_RESPONSE_CLOSE,
                                           -1);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  attributes_vbox = GTK_WIDGET (gtk_builder_get_object (builder,
                                                      "attributes-vbox"));
  gtk_container_set_border_width (GTK_CONTAINER (attributes_vbox), 2);
  gtk_box_pack_start (GTK_BOX (content_area), attributes_vbox, TRUE, TRUE, 0);

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
  gtk_label_set_text (GTK_LABEL (label_info), item_name);

  g_free (header);

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

  attributes_dialog_set_attributes (metadata, builder);

  gtk_widget_show (GTK_WIDGET (dialog));

  gtk_main ();

  return TRUE;
}


/*  private functions  */

static void
attributes_dialog_set_attributes (GimpMetadata   *metadata,
                                  GtkBuilder     *builder)
{
  GtkTreeStore   *exif_store;
  GtkTreeStore   *xmp_store;
  GtkTreeStore   *iptc_store;
  GtkTreeView    *exif_treeview;
  GtkTreeView    *xmp_treeview;
  GtkTreeView    *iptc_treeview;
  GtkWidget      *attributes_notebook;
  GList          *iter_list   = NULL;
  GimpAttribute  *attribute   = NULL;
  gboolean        gps_lat     = FALSE;
  gboolean        gps_lon     = FALSE;

  exif_store = GTK_TREE_STORE (gtk_builder_get_object (builder,
                                                       "exif-treestore"));
  xmp_store  = GTK_TREE_STORE (gtk_builder_get_object (builder,
                                                       "xmp-treestore"));
  iptc_store  = GTK_TREE_STORE (gtk_builder_get_object (builder,
                                                       "iptc-treestore"));

  exif_treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder,
                                                       "exif-treeview"));
  xmp_treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder,
                                                       "xmp-treeview"));
  iptc_treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder,
                                                       "iptc-treeview"));

  attributes_notebook = GTK_WIDGET (gtk_builder_get_object (builder,
                                                       "attributes-notebook"));

  gimp_metadata_iter_init (metadata, &iter_list);

  while (gimp_metadata_iter_next (metadata, &attribute, &iter_list))
    {
      const gchar    *name;
      const gchar    *type;
      const gchar    *ifd;
      const gchar    *tag;
      const gchar    *value;
      const gchar    *tag_sorted;
      gchar          *value_utf;
      GtkTreeIter    *parent = NULL;
      GtkTreeIter    *iter = g_slice_new (GtkTreeIter);

      name       = gimp_attribute_get_name (attribute);
      type       = gimp_attribute_get_attribute_type (attribute);
      ifd        = gimp_attribute_get_attribute_ifd (attribute);
      tag        = gimp_attribute_get_attribute_tag (attribute);
      tag_sorted = gimp_attribute_get_sortable_name (attribute);
      value      = gimp_attribute_get_interpreted_string (attribute);

      if (! g_utf8_validate (value, -1, NULL))
        value_utf = g_locale_to_utf8 (value, -1, NULL, NULL, NULL);
      else
        value_utf = g_strdup (value);

      parent = attributes_get_parent (type, ifd);

      if (! g_strcmp0 (name, "Exif.GPSInfo.GPSLatitude"))
        {
          gps_lat = TRUE;
          gps_latitude = gimp_attribute_get_gps_degree (attribute);
          if (gps_latitude < 0.0 && !gps_latitude_ref)
            {
              gps_latitude = gps_latitude * -1.0;
              gps_latitude_ref = "S";
            }

        }
      if (! g_strcmp0 (name, "Exif.GPSInfo.GPSLatitudeRef"))
        {
          gps_latitude_ref = gimp_attribute_get_string (attribute);
        }
      if (! g_strcmp0 (name, "Exif.GPSInfo.GPSLongitude"))
        {
          gps_lon = TRUE;
          gps_longitude = gimp_attribute_get_gps_degree (attribute);
          if (gps_longitude < 0.0 && !gps_longitude_ref)
            {
              gps_longitude = gps_longitude * -1.0;
              gps_longitude_ref = "W";
            }
        }
      if (! g_strcmp0 (name, "Exif.GPSInfo.GPSLongitudeRef"))
        {
          gps_longitude_ref = gimp_attribute_get_string (attribute);
        }

      if (gps_lat && gps_lon)
        {
          gtk_widget_set_sensitive (gps_button, TRUE);
          gps_lat = FALSE;
          gps_lon = FALSE;
        }

      switch (gimp_attribute_get_tag_type (attribute))
      {
        case TAG_EXIF:
          {
            gtk_tree_store_append (exif_store, iter, parent);

            if (!parent)
              {
                GtkTreeIter child;
                gtk_tree_store_set (exif_store, iter,
                                    C_IFD, ifd,
                                    C_TAG, "",
                                    C_VALUE, "",
                                    C_SORT, "",
                                    -1);
                parent = iter;
                gtk_tree_store_append (exif_store, &child, parent);
                gtk_tree_store_set (exif_store, &child,
                                    C_IFD, "",
                                    C_TAG, tag,
                                    C_VALUE, value_utf,
                                    C_SORT, tag_sorted,
                                    -1);
                attributes_add_parent (type, ifd, parent);
              }
            else
              {

                gtk_tree_store_set (exif_store, iter,
                                    C_IFD, "",
                                    C_TAG, tag,
                                    C_VALUE, value_utf,
                                    C_SORT, tag_sorted,
                                    -1);
              }
          }
          break;
        case TAG_XMP:
          {
            gtk_tree_store_append (xmp_store, iter, parent);
            if (!parent)
              {
                GtkTreeIter child;
                gtk_tree_store_set (xmp_store, iter,
                                    C_IFD, ifd,
                                    C_TAG, "",
                                    C_VALUE, "",
                                    C_SORT, "",
                                    -1);
                parent = iter;
                gtk_tree_store_append (xmp_store, &child, parent);
                gtk_tree_store_set (xmp_store, &child,
                                    C_IFD, "",
                                    C_TAG, tag,
                                    C_VALUE, value_utf,
                                    C_SORT, tag_sorted,
                                    -1);
                attributes_add_parent (type, ifd, parent);
              }
            else
              {

                gtk_tree_store_set (xmp_store, iter,
                                    C_IFD, "",
                                    C_TAG, tag,
                                    C_VALUE, value_utf,
                                    C_SORT, tag_sorted,
                                    -1);
              }
          }
          break;
        case TAG_IPTC:
          {
            gtk_tree_store_append (iptc_store, iter, parent);
            if (!parent)
              {
                GtkTreeIter child;
                gtk_tree_store_set (iptc_store, iter,
                                    C_IFD, ifd,
                                    C_TAG, "",
                                    C_VALUE, "",
                                    C_SORT, "",
                                    -1);
                parent = iter;
                gtk_tree_store_append (iptc_store, &child, parent);
                gtk_tree_store_set (iptc_store, &child,
                                    C_IFD, "",
                                    C_TAG, tag,
                                    C_VALUE, value_utf,
                                    C_SORT, tag_sorted,
                                    -1);
                attributes_add_parent (type, ifd, parent);
              }
            else
              {

                gtk_tree_store_set (iptc_store, iter,
                                    C_IFD, "",
                                    C_TAG, tag,
                                    C_VALUE, value_utf,
                                    C_SORT, tag_sorted,
                                    -1);
              }
          }
          break;
        default:
          {
            GtkTreeStore   *treestore;
            GtkTreeIter     iter;

            treestore = attributes_notebook_page_get (attributes_notebook, type);

            gtk_tree_store_append (treestore, &iter, NULL);
            gtk_tree_store_set (treestore, &iter,
                                C_IFD, "",
                                C_TAG, name,
                                C_VALUE, value_utf,
                                -1);
          }
          break;
      }
    }
  gtk_tree_view_expand_all (exif_treeview);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(exif_store),
                                       C_SORT, GTK_SORT_ASCENDING);

  gtk_tree_view_expand_all (xmp_treeview);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(xmp_store),
                                       C_SORT, GTK_SORT_ASCENDING);
  gtk_tree_view_expand_all (iptc_treeview);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(iptc_store),
                                       C_SORT, GTK_SORT_ASCENDING);
}

static GtkTreeIter *
attributes_get_parent (const gchar *type, const gchar *name)
{
  gchar          *key;
  gchar          *lowchar;
  GtkTreeIter    *iter;
  gpointer       *data;

  key = g_strdup_printf ("%s.%s", type, name);
  lowchar = g_ascii_strdown (key, -1);
  g_free (key);

  data = g_hash_table_lookup (ifd_table, (gpointer) lowchar);
  g_free (lowchar);

  if (data)
    {
      iter = (GtkTreeIter *) data;
      return iter;
    }
  else
    {
      return NULL;
    }
}

static void
attributes_add_parent (const gchar *type, const gchar *name, GtkTreeIter *iter)
{
  gchar          *key;
  gchar          *lowchar;

  key = g_strdup_printf ("%s.%s", type, name);
  lowchar = g_ascii_strdown (key, -1);
  g_free (key);

  g_hash_table_insert (ifd_table, (gpointer) lowchar, (gpointer) iter);

}

static GtkTreeStore *
attributes_notebook_page_get (GtkWidget *notebook,
                              const gchar *name)
{
  GtkWidget        *scrolledwindow;
  GtkWidget        *treeview;
  gchar            *lowchar;
  gpointer         *data;
  GtkTreeStore     *treestore;

  lowchar = g_ascii_strdown (name, -1);

  data = g_hash_table_lookup (tab_table, (gpointer) lowchar);

  if (data)
    {
      treestore = (GtkTreeStore *) data;
      g_free (lowchar);
    }
  else
    {
      treeview = attributes_treeview_new ();

      scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
      gtk_widget_show (scrolledwindow);
      gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);

      gtk_widget_show (treeview);
      gtk_container_add (GTK_CONTAINER (scrolledwindow), treeview);
      gtk_tree_view_set_enable_search (GTK_TREE_VIEW (treeview), FALSE);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                scrolledwindow,
                                gtk_label_new (name));

      treestore = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)));

      g_hash_table_insert (tab_table, (gpointer) lowchar, (gpointer) treestore);
    }
  return treestore;
}

static GtkWidget *
attributes_treeview_new (void)
{
  GtkWidget        *treeview;
  GtkTreeStore     *treestore;
  GtkCellRenderer  *ifd_renderer;
  GtkCellRenderer  *tag_renderer;
  GtkCellRenderer  *val_renderer;
  GtkTreeViewColumn *ifd_column;
  GtkTreeViewColumn *tag_column;
  GtkTreeViewColumn *val_column;

  ifd_renderer = gtk_cell_renderer_text_new ();
  tag_renderer = gtk_cell_renderer_text_new ();
  val_renderer = gtk_cell_renderer_text_new ();
  treestore = gtk_tree_store_new (NUM_COL, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  treeview = gtk_tree_view_new ();
  gtk_tree_view_set_model(GTK_TREE_VIEW (treeview), GTK_TREE_MODEL(treestore));

  ifd_column = gtk_tree_view_column_new_with_attributes ("", ifd_renderer, "text", C_IFD, NULL);
  tag_column = gtk_tree_view_column_new_with_attributes ("Tag", tag_renderer, "text", C_TAG, NULL);
  val_column = gtk_tree_view_column_new_with_attributes ("Value", val_renderer, "text", C_VALUE, NULL);

  gtk_tree_view_column_set_resizable (ifd_column, TRUE);
  gtk_tree_view_column_set_resizable (tag_column, TRUE);
  gtk_tree_view_column_set_resizable (val_column, TRUE);

  gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), ifd_column, -1);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), tag_column, -1);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), val_column, -1);

  return treeview;
}

/* select file to export */
static void
attributes_file_export_dialog (GtkWidget      *parent,
                               GimpMetadata *metadata)
{
  static GtkWidget *dlg = NULL;
  gchar            *suggest_file;
//                                          GTK_WINDOW (parent),

  suggest_file = g_strdup_printf ("%s.xmp", item_name);

  if (! dlg)
    {
      dlg = gtk_file_chooser_dialog_new (_("Export XMP to File"),
                                         GTK_WINDOW (parent),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,

                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                         NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
      gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dlg),
                                                      TRUE);

      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dlg), suggest_file);

      g_signal_connect (dlg, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dlg);
      g_signal_connect (dlg, "response",
                        G_CALLBACK (attributes_export_dialog_response),
                        metadata);

      g_free (suggest_file);
    }
  gtk_window_present (GTK_WINDOW (dlg));
}

/* call default browser for google maps */
static void
attributes_show_gps (GtkWidget      *parent,
                     GimpMetadata *metadata)
{
  GError  *error = NULL;
  gchar   *uri   = NULL;
  gchar    lat_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar    lon_buf[G_ASCII_DTOSTR_BUF_SIZE];

  if (!g_strcmp0 (gps_latitude_ref, "S"))
    gps_latitude = gps_latitude * -1.0;
  if (!g_strcmp0 (gps_longitude_ref, "W"))
    gps_longitude = gps_longitude * -1.0;

  g_ascii_formatd (lat_buf, G_ASCII_DTOSTR_BUF_SIZE, "%.6f",
                   gps_latitude);
  g_ascii_formatd (lon_buf, G_ASCII_DTOSTR_BUF_SIZE, "%.6f",
                   gps_longitude);

  uri = g_strdup_printf ("http://maps.google.com?q=%s,%s&z=15",
                         lat_buf,
                         lon_buf);

  g_app_info_launch_default_for_uri (uri, NULL, &error);
  if (error)
    {
      attributes_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (parent),
                               _("GPS failure"),
                               error->message);
    }


}

/* save XMP metadata to a file (only XMP, nothing else) */
static void
attributes_export_dialog_response (GtkWidget *dlg,
                                   gint       response_id,
                                   gpointer   data)
{
  GimpMetadata *metadata = (GimpMetadata *) data;

  g_return_if_fail (metadata != NULL);

  if (response_id == GTK_RESPONSE_OK)
    {
      GString       *buffer;
      const gchar   *xmp_data = NULL;
      gchar         *filename = NULL;
      int            fd;

      xmp_data = gimp_metadata_to_xmp_packet (metadata, "file/xmp");

      if (!xmp_data)
        {
          attributes_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dlg),
                                   _("XMP creation failed"),
                                   _("Cannot create xmp packet"));
          return;
        }

      buffer = g_string_new (xmp_data);

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
      fd = g_open (filename, O_CREAT | O_TRUNC | O_WRONLY | _O_BINARY, 0666);
      if (fd < 0)
        {
          attributes_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dlg),
                                   _("Open failed"),
                                   _("Cannot create file"));
          g_string_free (buffer, TRUE);
          g_free (filename);
          return;
        }

      if (write (fd, buffer->str, buffer->len) < 0)
        {
          attributes_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dlg),
                                   _("Save failed"),
                                   _("Some error occurred while saving"));
          g_string_free (buffer, TRUE);
          g_free (filename);
          return;
        }

      if (close (fd) < 0)
        {
          attributes_message_dialog (GTK_MESSAGE_ERROR, GTK_WINDOW (dlg),
                                   _("Save failed"),
                                   _("Could not close the file"));
          g_string_free (buffer, TRUE);
          g_free (filename);
          return;
        }

      g_string_free (buffer, TRUE);
      g_free (filename);
    }

  gtk_widget_destroy (dlg); /* FIXME: destroy or unmap? */
}

/* show a transient message dialog */
static void
attributes_message_dialog (GtkMessageType  type,
                           GtkWindow      *parent,
                           const gchar    *title,
                           const gchar    *message)
{
  GtkWidget *dlg;

  dlg = gtk_message_dialog_new (parent, 0, type, GTK_BUTTONS_OK, "%s", message);

  if (title)
    gtk_window_set_title (GTK_WINDOW (dlg), title);

  gtk_window_set_role (GTK_WINDOW (dlg), "metadata-message");
  gtk_dialog_run (GTK_DIALOG (dlg));
  gtk_widget_destroy (dlg);
}

