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

#pragma once


typedef struct _GimpImageFlushAccumulator GimpImageFlushAccumulator;

struct _GimpImageFlushAccumulator
{
  gboolean alpha_changed;
  gboolean mask_changed;
  gboolean floating_selection_changed;
  gboolean preview_invalidated;
};


struct _GimpImagePrivate
{
  gint               ID;                    /*  provides a unique ID         */

  GimpPlugInProcedure *load_proc;           /*  procedure used for loading   */
  GimpPlugInProcedure *save_proc;           /*  last save procedure used     */
  GimpPlugInProcedure *export_proc;         /*  last export procedure used   */

  gchar             *display_name;          /*  display basename             */
  gchar             *display_path;          /*  display full path            */
  gint               width;                 /*  width in pixels              */
  gint               height;                /*  height in pixels             */
  gdouble            xresolution;           /*  image x-res, in dpi          */
  gdouble            yresolution;           /*  image y-res, in dpi          */
  GimpUnit          *resolution_unit;       /*  resolution unit              */
  gboolean           resolution_set;        /*  resolution explicitly set    */
  GimpImageBaseType  base_type;             /*  base gimp_image type         */
  GimpPrecision      precision;             /*  image's precision            */
  GimpLayerMode      new_layer_mode;        /*  default mode of new layers   */

  gint               show_all;              /*  render full image content    */
  GeglRectangle      bounding_box;          /*  image content bounding box   */
  gint               bounding_box_freeze_count;
  gboolean           bounding_box_update_pending;
  GeglBuffer        *pickable_buffer;

  GimpPalette       *palette;               /*  palette of colormap          */
  const Babl        *babl_palette_rgb;      /*  palette's RGB Babl format    */
  const Babl        *babl_palette_rgba;     /*  palette's RGBA Babl format   */

  GimpColorProfile  *color_profile;         /*  image's color profile        */
  const Babl        *layer_space;           /*  image's Babl layer space     */
  GimpColorProfile  *hidden_profile;        /*  hidden by "use sRGB"         */

  /* image's simulation/soft-proofing settings */
  GimpColorProfile         *simulation_profile;
  GimpColorRenderingIntent  simulation_intent;
  gboolean                  simulation_bpc;

  gboolean           converting;            /*  color model or profile in middle of conversion?  */

  /*  Cached color transforms: from layer to sRGB u8 and double, and back    */
  gboolean            color_transforms_created;
  GimpColorTransform *transform_to_srgb_u8;
  GimpColorTransform *transform_to_srgb_double;
  GimpColorTransform *transform_from_srgb_double;

  GimpMetadata      *metadata;              /*  image's metadata             */

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

  GimpTattoo         tattoo_state;          /*  the last used tattoo         */

  GimpProjection    *projection;            /*  projection layers & channels */
  GeglNode          *graph;                 /*  GEGL projection graph        */
  GeglNode          *visible_mask;          /*  component visibility node    */

  GList             *symmetries;            /*  Painting symmetries          */
  GimpSymmetry      *active_symmetry;       /*  Active symmetry              */

  GList             *guides;                /*  guides                       */
  GimpGrid          *grid;                  /*  grid                         */
  GList             *sample_points;         /*  color sample points          */

  /*  Layer/Channel attributes  */
  GimpItemTree      *layers;                /*  the tree of layers           */
  GimpItemTree      *channels;              /*  the tree of masks            */
  GimpItemTree      *paths;                 /*  the tree of paths            */
  GSList            *layer_stack;           /*  the layers in MRU order      */

  GList             *hidden_items;          /*  internal process-only items  */

  GList             *stored_layer_sets;
  GList             *stored_channel_sets;
  GList             *stored_path_sets;

  GQuark             layer_offset_x_handler;
  GQuark             layer_offset_y_handler;
  GQuark             layer_bounding_box_handler;
  GQuark             layer_alpha_handler;
  GQuark             channel_name_changed_handler;
  GQuark             channel_color_changed_handler;

  GimpLayer         *floating_sel;          /*  the FS layer                 */
  GimpChannel       *selection_mask;        /*  the selection mask channel   */

  GimpParasiteList  *parasites;             /*  Plug-in parasite data        */

  gboolean           visible[MAX_CHANNELS]; /*  visible channels             */
  gboolean           active[MAX_CHANNELS];  /*  active channels              */

  gboolean           quick_mask_state;      /*  TRUE if quick mask is on       */
  gboolean           quick_mask_inverted;   /*  TRUE if quick mask is inverted */
  GeglColor         *quick_mask_color;      /*  rgba triplet of the color      */
  GList             *quick_mask_selected;   /*  Drawables selected to revert   */

  /*  Undo apparatus  */
  GimpUndoStack     *undo_stack;            /*  stack for undo operations    */
  GimpUndoStack     *redo_stack;            /*  stack for redo operations    */
  gint               group_count;           /*  nested undo groups           */
  GimpUndoType       pushing_undo_group;    /*  undo group status flag       */

  /*  Signal emission accumulator  */
  GimpImageFlushAccumulator  flush_accum;
};

#define GIMP_IMAGE_GET_PRIVATE(image) (((GimpImage *) (image))->priv)

void   gimp_image_take_mask (GimpImage   *image,
                             GimpChannel *mask);
