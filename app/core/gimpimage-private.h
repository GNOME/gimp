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

#ifndef __GIMP_IMAGE_PRIVATE_H__
#define __GIMP_IMAGE_PRIVATE_H__


typedef struct _GimpImagePrivate GimpImagePrivate;

struct _GimpImagePrivate
{
  gint               ID;                    /*  provides a unique ID         */

  GimpPlugInProcedure *load_proc;           /*  procedure used for loading   */
  GimpPlugInProcedure *save_proc;           /*  last save procedure used     */

  gchar             *display_name;          /*  display basename             */
  gint               width;                 /*  width in pixels              */
  gint               height;                /*  height in pixels             */
  gdouble            xresolution;           /*  image x-res, in dpi          */
  gdouble            yresolution;           /*  image y-res, in dpi          */
  GimpUnit           resolution_unit;       /*  resolution unit              */
  GimpImageBaseType  base_type;             /*  base gimp_image type         */

  guchar            *colormap;              /*  colormap (for indexed)       */
  gint               n_colors;              /*  # of colors (for indexed)    */

  gint               dirty;                 /*  dirty flag -- # of ops       */
  guint              dirty_time;            /*  time when image became dirty */
  gint               export_dirty;          /*  'dirty' but for export       */

  gint               undo_freeze_count;     /*  counts the _freeze's         */

  gint               instance_count;        /*  number of instances          */
  gint               disp_count;            /*  number of displays           */

  GimpTattoo         tattoo_state;          /*  the last used tattoo         */

  GimpProjection    *projection;            /*  projection layers & channels */
  GeglNode          *graph;                 /*  GEGL projection graph        */

  GList             *guides;                /*  guides                       */
  GimpGrid          *grid;                  /*  grid                         */
  GList             *sample_points;         /*  color sample points          */

  /*  Layer/Channel attributes  */
  GimpContainer     *layers;                /*  the list of layers           */
  GimpContainer     *channels;              /*  the list of masks            */
  GimpContainer     *vectors;               /*  the list of vectors          */
  GSList            *layer_stack;           /*  the layers in MRU order      */

  GQuark             layer_alpha_handler;
  GQuark             channel_name_changed_handler;
  GQuark             channel_color_changed_handler;

  GimpLayer         *active_layer;          /*  the active layer             */
  GimpChannel       *active_channel;        /*  the active channel           */
  GimpVectors       *active_vectors;        /*  the active vectors           */

  GimpLayer         *floating_sel;          /*  the FS layer                 */
  GimpChannel       *selection_mask;        /*  the selection mask channel   */

  GimpParasiteList  *parasites;             /*  Plug-in parasite data        */

  gboolean           visible[MAX_CHANNELS]; /*  visible channels             */
  gboolean           active[MAX_CHANNELS];  /*  active channels              */

  gboolean           quick_mask_state;      /*  TRUE if quick mask is on       */
  gboolean           quick_mask_inverted;   /*  TRUE if quick mask is inverted */
  GimpRGB            quick_mask_color;      /*  rgba triplet of the color      */

  /*  Undo apparatus  */
  GimpUndoStack     *undo_stack;            /*  stack for undo operations    */
  GimpUndoStack     *redo_stack;            /*  stack for redo operations    */
  gint               group_count;           /*  nested undo groups           */
  GimpUndoType       pushing_undo_group;    /*  undo group status flag       */
};

#define GIMP_IMAGE_GET_PRIVATE(image) \
        G_TYPE_INSTANCE_GET_PRIVATE (image, \
                                     GIMP_TYPE_IMAGE, \
                                     GimpImagePrivate)


#endif  /* __GIMP_IMAGE_PRIVATE_H__ */
