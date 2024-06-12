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
#define TILE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TILE_TYPE, Tile))

GType                   tile_get_type         (void) G_GNUC_CONST;

static GList          * tile_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * tile_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * tile_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               gint                  n_drawables,
                                               GimpDrawable        **drawables,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static void             tile                  (GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               gint                  new_width,
                                               gint                  new_height,
                                               gboolean              create_new_image,
                                               GimpImage           **new_image,
                                               GimpLayer           **new_layer);

static void             tile_gegl             (GeglBuffer           *src,
                                               gint                  src_width,
                                               gint                  src_height,
                                               GeglBuffer           *dst,
                                               gint                  dst_width,
                                               gint                  dst_height);

static gboolean         tile_dialog           (GimpProcedure        *procedure,
                                               GimpProcedureConfig  *config,
                                               GimpImage            *image,
                                               GimpDrawable         *drawable);


G_DEFINE_TYPE (Tile, tile, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (TILE_TYPE)
DEFINE_STD_SET_I18N


static void
tile_class_init (TileClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = tile_query_procedures;
  plug_in_class->create_procedure = tile_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
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
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Tile..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Map");

      gimp_procedure_set_documentation (procedure,
                                        _("Create an array of copies "
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

      /* XXX: we actually don't care about the default value as we always set
       * the width/height of the active image, though it would be nice to reuse
       * the previously stored value if we run tile on the same image twice.
       * TODO: a flag and/or some special handler function would be interesting
       * for such specific cases where a hardcoded default value doesn't make
       * sense or when we want stored values to only apply when run on a same
       * image.
       */
      gimp_procedure_add_int_argument (procedure, "new-width",
                                       _("New _width"),
                                       _("New (tiled) image width"),
                                       1, GIMP_MAX_IMAGE_SIZE, 1,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "new-height",
                                       _("New _height"),
                                       _("New (tiled) image height"),
                                       1, GIMP_MAX_IMAGE_SIZE, 1,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "new-image",
                                           _("New _image"),
                                           _("Create a new image"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_image_return_value (procedure, "new-image",
                                             "New image",
                                             "Output image (NULL if new-image == FALSE)",
                                             TRUE,
                                             G_PARAM_READWRITE);

      gimp_procedure_add_layer_return_value (procedure, "new-layer",
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
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpLayer      *new_layer;
  GimpImage      *new_image;
  GimpDrawable   *drawable;
  gint            new_width;
  gint            new_height;
  gboolean        create_new_image;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   gimp_procedure_get_name (procedure));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (! tile_dialog (procedure, config, image, drawable))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      break;
    }

  g_object_get (config,
                "new-width",  &new_width,
                "new-height", &new_height,
                "new-image",  &create_new_image,
                NULL);

  gimp_progress_init (_("Tiling"));

  tile (image, drawable, new_width, new_height,
        create_new_image, &new_image, &new_layer);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    {
      if (create_new_image)
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
      gint           new_width,
      gint           new_height,
      gboolean       create_new_image,
      GimpImage    **new_image,
      GimpLayer    **new_layer)
{
  GimpDrawable *dst_drawable;
  GeglBuffer   *dst_buffer;
  GeglBuffer   *src_buffer;
  gint          src_width  = gimp_drawable_get_width (drawable);
  gint          src_height = gimp_drawable_get_height (drawable);

  GimpImageBaseType  image_type   = GIMP_RGB;

  if (create_new_image)
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

      *new_image = gimp_image_new_with_precision (new_width,
                                                  new_height,
                                                  image_type,
                                                  precision);
      gimp_image_undo_disable (*new_image);

      /*  copy the colormap, if necessary  */
      if (image_type == GIMP_INDEXED)
        {
          guchar *cmap;
          gint    ncols;

          cmap = gimp_image_get_colormap (image, NULL, &ncols);
          gimp_image_set_colormap (*new_image, cmap, ncols);
          g_free (cmap);
        }

      *new_layer = gimp_layer_new (*new_image, _("Background"),
                                   new_width, new_height,
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
      gimp_image_resize (image, new_width, new_height, 0, 0);

      if (gimp_item_is_layer (GIMP_ITEM (drawable)))
        {
          gimp_layer_resize (GIMP_LAYER (drawable), new_width, new_height, 0, 0);
        }
      else if (gimp_item_is_layer_mask (GIMP_ITEM (drawable)))
        {
          GimpLayer *layer = gimp_layer_from_mask (GIMP_LAYER_MASK (drawable));

          gimp_layer_resize (layer, new_width, new_height, 0, 0);
        }

      dst_drawable = drawable;
    }

  src_buffer = gimp_drawable_get_buffer (drawable);
  dst_buffer = gimp_drawable_get_buffer (dst_drawable);

  tile_gegl (src_buffer, src_width, src_height,
             dst_buffer, new_width, new_height);

  gegl_buffer_flush (dst_buffer);
  gimp_drawable_update (dst_drawable, 0, 0, new_width, new_height);

  if (create_new_image)
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
tile_dialog (GimpProcedure       *procedure,
             GimpProcedureConfig *config,
             GimpImage           *image,
             GimpDrawable        *drawable)
{
  GtkWidget *dlg;
  gint       width;
  gint       height;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  width  = gimp_drawable_get_width (drawable);
  height = gimp_drawable_get_height (drawable);

  g_object_set (config,
                "new-width",  width,
                "new-height", height,
                NULL);

  dlg = gimp_procedure_dialog_new (procedure,
                                   GIMP_PROCEDURE_CONFIG (config),
                                   _("Tile"));
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dlg),
                                   "new-size-label", _("Tile to New Size"),
                                   FALSE, FALSE);
  /* TODO: we should have a new GimpProcedureDialog widget which would tie 2
   * arguments for dimensions (or coordinates), and possibly more aux args for
   * the constrain boolean choice, the unit, etc.
   */
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dlg),
                                  "new-size-box",
                                  "new-width", "new-height",
                                  NULL);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dlg),
                                    "new-size-frame", "new-size-label", FALSE,
                                    "new-size-box");
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dlg), "new-size-frame", "new-image", NULL);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dlg));

  gtk_widget_destroy (dlg);

  return run;
}
