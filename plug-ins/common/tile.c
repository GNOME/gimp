/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/*
 * This filter tiles an image to arbitrary width and height
 */
#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-tile"
#define PLUG_IN_BINARY "tile"
#define PLUG_IN_ROLE   "gimp-tile"


typedef struct
{
  gint new_width;
  gint new_height;
  gint constrain;
  gint new_image;
} TileVals;


typedef struct _Tile      Tile;
typedef struct _TileClass TileClass;

struct _Tile
{
  GimpPlugIn parent_instance;
};

struct _TileClass
{
  GimpPlugInClass parent_class;
};


#define TILE_TYPE  (tile_get_type ())
#define TILE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TILE_TYPE, Tile))

GType                   tile_get_type         (void) G_GNUC_CONST;

static GList          * tile_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * tile_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * tile_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               const GimpValueArray *args,
                                               gpointer              run_data);

static void             tile                  (GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               GimpImage           **new_image,
                                               GimpLayer           **new_layer);

static void             tile_gegl             (GeglBuffer           *src,
                                               gint                  src_width,
                                               gint                  src_height,
                                               GeglBuffer           *dst,
                                               gint                  dst_width,
                                               gint                  dst_height);

static gboolean         tile_dialog           (GimpImage            *image,
                                               GimpDrawable         *drawable);


G_DEFINE_TYPE (Tile, tile, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (TILE_TYPE)


static TileVals tvals =
{
  1,     /* new_width  */
  1,     /* new_height */
  TRUE,  /* constrain  */
  TRUE   /* new_image  */
};


static void
tile_class_init (TileClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = tile_query_procedures;
  plug_in_class->create_procedure = tile_create_procedure;
}

static void
tile_init (Tile *tile)
{
}

static GList *
tile_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
tile_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            tile_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("_Tile..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Map");

      gimp_procedure_set_documentation (procedure,
                                        N_("Create an array of copies "
                                           "of the image"),
                                        "This function creates a new image "
                                        "with a single layer sized to the "
                                        "specified 'new_width' and "
                                        "'new_height' parameters. The "
                                        "specified drawable is tiled into "
                                        "this layer.  The new layer will have "
                                        "the same type as the specified "
                                        "drawable and the new image will "
                                        "have a corresponding base type.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1996-1997");

      GIMP_PROC_ARG_INT (procedure, "new-width",
                         "New width",
                         "New (tiled) image width",
                         1, GIMP_MAX_IMAGE_SIZE, 1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "new-height",
                         "New height",
                         "New (tiled) image height",
                         1, GIMP_MAX_IMAGE_SIZE, 1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "new-image",
                             "New image",
                             "Create a new image",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_VAL_IMAGE (procedure, "new-image",
                           "New image",
                           "Output image (NULL if new-image == FALSE)",
                           TRUE,
                           G_PARAM_READWRITE);

      GIMP_PROC_VAL_LAYER (procedure, "new-layer",
                           "New layer",
                           "Output layer (NULL if new-image == FALSE)",
                           TRUE,
                           G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
tile_run (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable         *drawable,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpLayer      *new_layer;
  GimpImage      *new_image;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &tvals);

      if (! tile_dialog (image, drawable))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      tvals.new_width  = GIMP_VALUES_GET_INT     (args, 0);
      tvals.new_height = GIMP_VALUES_GET_INT     (args, 1);
      tvals.new_image  = GIMP_VALUES_GET_BOOLEAN (args, 2);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &tvals);
      break;
    }

  gimp_progress_init (_("Tiling"));

  tile (image,
        drawable,
        &new_image,
        &new_layer);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_set_data (PLUG_IN_PROC, &tvals, sizeof (TileVals));

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    {
      if (tvals.new_image)
        gimp_display_new (new_image);
      else
        gimp_displays_flush ();
    }

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, new_image);
  GIMP_VALUES_SET_LAYER (return_vals, 2, new_layer);

  return return_vals;
}

static void
tile_gegl (GeglBuffer  *src,
           gint         src_width,
           gint         src_height,
           GeglBuffer  *dst,
           gint         dst_width,
           gint         dst_height)
{
  GeglNode *node;
  GeglNode *buffer_src_node;
  GeglNode *tile_node;
  GeglNode *crop_src_node;
  GeglNode *crop_dst_node;
  GeglNode *buffer_dst_node;

  GeglProcessor *processor;
  gdouble        progress;

  node = gegl_node_new ();

  buffer_src_node = gegl_node_new_child (node,
                                         "operation", "gegl:buffer-source",
                                         "buffer",    src,
                                         NULL);

  crop_src_node = gegl_node_new_child (node,
                                       "operation", "gegl:crop",
                                       "width",     (gdouble) src_width,
                                       "height",    (gdouble) src_height,
                                       NULL);

  tile_node = gegl_node_new_child (node,
                                   "operation", "gegl:tile",
                                   NULL);

  crop_dst_node = gegl_node_new_child (node,
                                       "operation", "gegl:crop",
                                       "width",     (gdouble) dst_width,
                                       "height",    (gdouble) dst_height,
                                       NULL);

  buffer_dst_node = gegl_node_new_child (node,
                                         "operation", "gegl:write-buffer",
                                         "buffer",    dst,
                                         NULL);

  gegl_node_link_many (buffer_src_node,
                       crop_src_node,
                       tile_node,
                       crop_dst_node,
                       buffer_dst_node,
                       NULL);

  processor = gegl_node_new_processor (buffer_dst_node, NULL);

  while (gegl_processor_work (processor, &progress))
    if (!((gint) (progress * 100.0) % 10))
      gimp_progress_update (progress);

  gimp_progress_update (1.0);

  g_object_unref (processor);
  g_object_unref (node);
}

static void
tile (GimpImage     *image,
      GimpDrawable  *drawable,
      GimpImage    **new_image,
      GimpLayer    **new_layer)
{
  GimpDrawable *dst_drawable;
  GeglBuffer   *dst_buffer;
  GeglBuffer   *src_buffer;
  gint          dst_width  = tvals.new_width;
  gint          dst_height = tvals.new_height;
  gint          src_width  = gimp_drawable_width (drawable);
  gint          src_height = gimp_drawable_height (drawable);

  GimpImageBaseType  image_type   = GIMP_RGB;

  if (tvals.new_image)
    {
      /*  create  a new image  */
      gint32 precision = gimp_image_get_precision (image);

      switch (gimp_drawable_type (drawable))
        {
        case GIMP_RGB_IMAGE:
        case GIMP_RGBA_IMAGE:
          image_type = GIMP_RGB;
          break;

        case GIMP_GRAY_IMAGE:
        case GIMP_GRAYA_IMAGE:
          image_type = GIMP_GRAY;
          break;

        case GIMP_INDEXED_IMAGE:
        case GIMP_INDEXEDA_IMAGE:
          image_type = GIMP_INDEXED;
          break;
        }

      *new_image = gimp_image_new_with_precision (dst_width,
                                                  dst_height,
                                                  image_type,
                                                  precision);
      gimp_image_undo_disable (*new_image);

      /*  copy the colormap, if necessary  */
      if (image_type == GIMP_INDEXED)
        {
          guchar *cmap;
          gint    ncols;

          cmap = gimp_image_get_colormap (image, &ncols);
          gimp_image_set_colormap (*new_image, cmap, ncols);
          g_free (cmap);
        }

      *new_layer = gimp_layer_new (*new_image, _("Background"),
                                   dst_width, dst_height,
                                   gimp_drawable_type (drawable),
                                   100,
                                   gimp_image_get_default_new_layer_mode (*new_image));

      if (*new_layer == NULL)
        return;

      gimp_image_insert_layer (*new_image, *new_layer, NULL, 0);
      dst_drawable = GIMP_DRAWABLE (*new_layer);
    }
  else
    {
      *new_image = NULL;
      *new_layer = NULL;

      gimp_image_undo_group_start (image);
      gimp_image_resize (image, dst_width, dst_height, 0, 0);

      if (gimp_item_is_layer (GIMP_ITEM (drawable)))
        {
          gimp_layer_resize (GIMP_LAYER (drawable), dst_width, dst_height, 0, 0);
        }
      else if (gimp_item_is_layer_mask (GIMP_ITEM (drawable)))
        {
          GimpLayer *layer = gimp_layer_from_mask (GIMP_LAYER_MASK (drawable));

          gimp_layer_resize (layer, dst_width, dst_height, 0, 0);
        }

      dst_drawable = drawable;
    }

  src_buffer = gimp_drawable_get_buffer (drawable);
  dst_buffer = gimp_drawable_get_buffer (dst_drawable);

  tile_gegl (src_buffer, src_width, src_height,
             dst_buffer, dst_width, dst_height);

  gegl_buffer_flush (dst_buffer);
  gimp_drawable_update (dst_drawable, 0, 0, dst_width, dst_height);

  if (tvals.new_image)
    {
      gimp_image_undo_enable (*new_image);
    }
  else
    {
      gimp_image_undo_group_end (image);
    }

  g_object_unref (src_buffer);
  g_object_unref (dst_buffer);
}

static gboolean
tile_dialog (GimpImage    *image,
             GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *sizeentry;
  GtkWidget *chainbutton;
  GtkWidget *toggle;
  gint       width;
  gint       height;
  gdouble    xres;
  gdouble    yres;
  GimpUnit   unit;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  width  = gimp_drawable_width (drawable);
  height = gimp_drawable_height (drawable);
  unit   = gimp_image_get_unit (image);
  gimp_image_get_resolution (image, &xres, &yres);

  tvals.new_width  = width;
  tvals.new_height = height;

  dlg = gimp_dialog_new (_("Tile"), PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gimp_frame_new (_("Tile to New Size"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  sizeentry = gimp_coordinates_new (unit, "%a", TRUE, TRUE, 8,
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE,

                                    tvals.constrain, TRUE,

                                    _("_Width:"), width, xres,
                                    1, GIMP_MAX_IMAGE_SIZE,
                                    0, width,

                                    _("_Height:"), height, yres,
                                    1, GIMP_MAX_IMAGE_SIZE,
                                    0, height);
  gtk_container_add (GTK_CONTAINER (frame), sizeentry);
  gtk_widget_show (sizeentry);

  chainbutton = GTK_WIDGET (GIMP_COORDINATES_CHAINBUTTON (sizeentry));

  toggle = gtk_check_button_new_with_mnemonic (_("C_reate new image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), tvals.new_image);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &tvals.new_image);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run)
    {
      tvals.new_width =
        RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (sizeentry), 0));
      tvals.new_height =
        RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (sizeentry), 1));

      tvals.constrain =
        gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (chainbutton));
    }

  gtk_widget_destroy (dlg);

  return run;
}
