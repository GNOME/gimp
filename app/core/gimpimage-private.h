/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_IMAGE_PRIVATE_H__
#define __LIGMA_IMAGE_PRIVATE_H__


typedef struct _LigmaImageFlushAccumulator LigmaImageFlushAccumulator;

struct _LigmaImageFlushAccumulator
{
  gboolean alpha_changed;
  gboolean mask_changed;
  gboolean floating_selection_changed;
  gboolean preview_invalidated;
};


struct _LigmaImagePrivate
{
  gint               ID;                    /*  provides a unique ID         */

  LigmaPlugInProcedure *load_proc;           /*  procedure used for loading   */
  LigmaPlugInProcedure *save_proc;           /*  last save procedure used     */
  LigmaPlugInProcedure *export_proc;         /*  last export procedure used   */

  gchar             *display_name;          /*  display basename             */
  gchar             *display_path;          /*  display full path            */
  gint               width;                 /*  width in pixels              */
  gint               height;                /*  height in pixels             */
  gdouble            xresolution;           /*  image x-res, in dpi          */
  gdouble            yresolution;           /*  image y-res, in dpi          */
  LigmaUnit           resolution_unit;       /*  resolution unit              */
  gboolean           resolution_set;        /*  resolution explicitly set    */
  LigmaImageBaseType  base_type;             /*  base ligma_image type         */
  LigmaPrecision      precision;             /*  image's precision            */
  LigmaLayerMode      new_layer_mode;        /*  default mode of new layers   */

  gint               show_all;              /*  render full image content    */
  GeglRectangle      bounding_box;          /*  image content bounding box   */
  gint               bounding_box_freeze_count;
  gboolean           bounding_box_update_pending;
  GeglBuffer        *pickable_buffer;

  LigmaPalette       *palette;               /*  palette of colormap          */
  const Babl        *babl_palette_rgb;      /*  palette's RGB Babl format    */
  const Babl        *babl_palette_rgba;     /*  palette's RGBA Babl format   */

  LigmaColorProfile  *color_profile;         /*  image's color profile        */
  const Babl        *layer_space;           /*  image's Babl layer space     */
  LigmaColorProfile  *hidden_profile;        /*  hidden by "use sRGB"         */

  /* image's simulation/soft-proofing settings */
  LigmaColorProfile         *simulation_profile;
  LigmaColorRenderingIntent  simulation_intent;
  gboolean                  simulation_bpc;

  /*  Cached color transforms: from layer to sRGB u8 and double, and back    */
  gboolean            color_transforms_created;
  LigmaColorTransform *transform_to_srgb_u8;
  LigmaColorTransform *transform_from_srgb_u8;
  LigmaColorTransform *transform_to_srgb_double;
  LigmaColorTransform *transform_from_srgb_double;

  LigmaMetadata      *metadata;              /*  image's metadata             */

  GFile             *file;                  /*  the image's XCF file         */
  GFile             *imported_file;         /*  the image's source file      */
  GFile             *exported_file;         /*  the image's export file      */
  GFile             *save_a_copy_file;      /*  the image's save-a-copy file */
  GFile             *untitled_file;         /*  a file saying "Untitled"     */

  gboolean           xcf_compression;       /*  XCF compression enabled?     */

  gint               dirty;                 /*  dirty flag -- # of ops       */
  gint64             dirty_time;            /*  time when image became dirty */
  gint               export_dirty;          /*  'dirty' but for export       */

  gint               undo_freeze_count;     /*  counts the _freeze's         */

  gint               instance_count;        /*  number of instances          */
  gint               disp_count;            /*  number of displays           */

  LigmaTattoo         tattoo_state;          /*  the last used tattoo         */

  LigmaProjection    *projection;            /*  projection layers & channels */
  GeglNode          *graph;                 /*  GEGL projection graph        */
  GeglNode          *visible_mask;          /*  component visibility node    */

  GList             *symmetries;            /*  Painting symmetries          */
  LigmaSymmetry      *active_symmetry;       /*  Active symmetry              */

  GList             *guides;                /*  guides                       */
  LigmaGrid          *grid;                  /*  grid                         */
  GList             *sample_points;         /*  color sample points          */

  /*  Layer/Channel attributes  */
  LigmaItemTree      *layers;                /*  the tree of layers           */
  LigmaItemTree      *channels;              /*  the tree of masks            */
  LigmaItemTree      *vectors;               /*  the tree of vectors          */
  GSList            *layer_stack;           /*  the layers in MRU order      */

  GList             *hidden_items;          /*  internal process-only items  */

  GList             *stored_layer_sets;
  GList             *stored_channel_sets;
  GList             *stored_vectors_sets;

  GQuark             layer_offset_x_handler;
  GQuark             layer_offset_y_handler;
  GQuark             layer_bounding_box_handler;
  GQuark             layer_alpha_handler;
  GQuark             channel_name_changed_handler;
  GQuark             channel_color_changed_handler;

  LigmaLayer         *floating_sel;          /*  the FS layer                 */
  LigmaChannel       *selection_mask;        /*  the selection mask channel   */

  LigmaParasiteList  *parasites;             /*  Plug-in parasite data        */

  gboolean           visible[MAX_CHANNELS]; /*  visible channels             */
  gboolean           active[MAX_CHANNELS];  /*  active channels              */

  gboolean           quick_mask_state;      /*  TRUE if quick mask is on       */
  gboolean           quick_mask_inverted;   /*  TRUE if quick mask is inverted */
  LigmaRGB            quick_mask_color;      /*  rgba triplet of the color      */

  /*  Undo apparatus  */
  LigmaUndoStack     *undo_stack;            /*  stack for undo operations    */
  LigmaUndoStack     *redo_stack;            /*  stack for redo operations    */
  gint               group_count;           /*  nested undo groups           */
  LigmaUndoType       pushing_undo_group;    /*  undo group status flag       */

  /*  Signal emission accumulator  */
  LigmaImageFlushAccumulator  flush_accum;
};

#define LIGMA_IMAGE_GET_PRIVATE(image) (((LigmaImage *) (image))->priv)

void   ligma_image_take_mask (LigmaImage   *image,
                             LigmaChannel *mask);


#endif  /* __LIGMA_IMAGE_PRIVATE_H__ */
