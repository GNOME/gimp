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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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


/* Declare local functions.
 */
static void      query        (void);

static void      run          (const gchar      *name,
                               gint              nparams,
                               const GimpParam  *param,
                               gint             *nreturn_vals,
                               GimpParam       **return_vals);

static void      tile         (gint32     image_id,
                               gint32     drawable_id,
                               gint32    *new_image_id,
                               gint32    *new_layer_id);

static void      tile_gegl    (GeglBuffer  *src,
                               gint         src_width,
                               gint         src_height,
                               GeglBuffer  *dst,
                               gint         dst_width,
                               gint         dst_height);

static gboolean  tile_dialog  (gint32     image_ID,
                               gint32     drawable_ID);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static TileVals tvals =
{
  1,     /* new_width  */
  1,     /* new_height */
  TRUE,  /* constrain  */
  TRUE   /* new_image  */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",      "Input image (unused)"        },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable"              },
    { GIMP_PDB_INT32,    "new-width",  "New (tiled) image width"     },
    { GIMP_PDB_INT32,    "new-height", "New (tiled) image height"    },
    { GIMP_PDB_INT32,    "new-image",  "Create a new image?"         }
  };

  static const GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "new-image", "Output image (-1 if new-image == FALSE)" },
    { GIMP_PDB_LAYER, "new-layer", "Output layer (-1 if new-image == FALSE)" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Create an array of copies of the image"),
                          "This function creates a new image with a single "
                          "layer sized to the specified 'new_width' and "
                          "'new_height' parameters.  The specified drawable "
                          "is tiled into this layer.  The new layer will have "
                          "the same type as the specified drawable and the "
                          "new image will have a corresponding base type.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1996-1997",
                          N_("_Tile..."),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
                          args, return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Map");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[3];
  GimpRunMode       run_mode;
  GimpPDBStatusType status    = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 3;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type          = GIMP_PDB_IMAGE;
  values[2].type          = GIMP_PDB_LAYER;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &tvals);

      /*  First acquire information with a dialog  */
      if (! tile_dialog (param[1].data.d_image,
                         param[2].data.d_drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 6)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          tvals.new_width  = param[3].data.d_int32;
          tvals.new_height = param[4].data.d_int32;
          tvals.new_image  = param[5].data.d_int32 ? TRUE : FALSE;

          if (tvals.new_width < 1 || tvals.new_height < 1)
            status = GIMP_PDB_CALLING_ERROR;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &tvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gint32 new_layer_id;
      gint32 new_image_id;

      gimp_progress_init (_("Tiling"));

      tile (param[1].data.d_image,
            param[2].data.d_drawable,
            &new_image_id,
            &new_layer_id);

      values[1].data.d_image = new_image_id;
      values[2].data.d_layer = new_layer_id;

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &tvals, sizeof (TileVals));

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        {
          if (tvals.new_image)
            gimp_display_new (values[1].data.d_image);
          else
            gimp_displays_flush ();
        }
    }

  values[0].data.d_status = status;

  gegl_exit ();
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
tile (gint32  image_id,
      gint32  drawable_id,
      gint32 *new_image_id,
      gint32 *new_layer_id)
{
  gint32       dst_drawable_id;
  GeglBuffer  *dst_buffer;
  GeglBuffer  *src_buffer;
  gint         dst_width  = tvals.new_width;
  gint         dst_height = tvals.new_height;
  gint         src_width  = gimp_drawable_width (drawable_id);
  gint         src_height = gimp_drawable_height (drawable_id);

  GimpImageBaseType  image_type   = GIMP_RGB;

  /* sanity check parameters */
  if (dst_width < 1 || dst_height < 1)
    {
      *new_image_id = -1;
      *new_layer_id = -1;
      return;
    }

  if (tvals.new_image)
    {
      /*  create  a new image  */
      gint32 precision = gimp_image_get_precision (image_id);

      switch (gimp_drawable_type (drawable_id))
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

      *new_image_id = gimp_image_new_with_precision (dst_width,
                                                     dst_height,
                                                     image_type,
                                                     precision);
      gimp_image_undo_disable (*new_image_id);

      /*  copy the colormap, if necessary  */
      if (image_type == GIMP_INDEXED)
        {
          guchar *cmap;
          gint    ncols;

          cmap = gimp_image_get_colormap (image_id, &ncols);
          gimp_image_set_colormap (*new_image_id, cmap, ncols);
          g_free (cmap);
        }

      *new_layer_id = gimp_layer_new (*new_image_id, _("Background"),
                                      dst_width, dst_height,
                                      gimp_drawable_type (drawable_id),
                                      100,
                                      gimp_image_get_default_new_layer_mode (*new_image_id));

      if (*new_layer_id == -1)
        return;

      gimp_image_insert_layer (*new_image_id, *new_layer_id, -1, 0);
      dst_drawable_id = *new_layer_id;
    }
  else
    {
      *new_image_id = -1;
      *new_layer_id = -1;

      gimp_image_undo_group_start (image_id);
      gimp_image_resize (image_id, dst_width, dst_height, 0, 0);

      if (gimp_item_is_layer (drawable_id))
        gimp_layer_resize (drawable_id, dst_width, dst_height, 0, 0);
      else if (gimp_item_is_layer_mask (drawable_id))
        {
          gint32 layer_id = gimp_layer_from_mask (drawable_id);
          gimp_layer_resize (layer_id, dst_width, dst_height, 0, 0);
        }

      dst_drawable_id = drawable_id;
    }

  src_buffer = gimp_drawable_get_buffer (drawable_id);
  dst_buffer = gimp_drawable_get_buffer (dst_drawable_id);

  tile_gegl (src_buffer, src_width, src_height,
             dst_buffer, dst_width, dst_height);

  gegl_buffer_flush (dst_buffer);
  gimp_drawable_update (dst_drawable_id, 0, 0, dst_width, dst_height);

  if (tvals.new_image)
    {
      gimp_image_undo_enable (*new_image_id);
    }
  else
    {
      gimp_image_undo_group_end (image_id);
    }

  g_object_unref (src_buffer);
  g_object_unref (dst_buffer);
}

static gboolean
tile_dialog (gint32 image_ID,
             gint32 drawable_ID)
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

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  width  = gimp_drawable_width (drawable_ID);
  height = gimp_drawable_height (drawable_ID);
  unit   = gimp_image_get_unit (image_ID);
  gimp_image_get_resolution (image_ID, &xres, &yres);

  tvals.new_width  = width;
  tvals.new_height = height;

  dlg = gimp_dialog_new (_("Tile"), PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
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
