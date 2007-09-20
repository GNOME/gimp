/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_IMAGE_H__
#define __GIMP_IMAGE_H__


#include "gimpviewable.h"


#define GIMP_IMAGE_TYPE_IS_RGB(t)          ((t) == GIMP_RGB_IMAGE ||         \
                                            (t) == GIMP_RGBA_IMAGE)
#define GIMP_IMAGE_TYPE_IS_GRAY(t)         ((t) == GIMP_GRAY_IMAGE ||        \
                                            (t) == GIMP_GRAYA_IMAGE)
#define GIMP_IMAGE_TYPE_IS_INDEXED(t)      ((t) == GIMP_INDEXED_IMAGE ||     \
                                            (t) == GIMP_INDEXEDA_IMAGE)

#define GIMP_IMAGE_TYPE_HAS_ALPHA(t)       ((t) == GIMP_RGBA_IMAGE  ||       \
                                            (t) == GIMP_GRAYA_IMAGE ||       \
                                            (t) == GIMP_INDEXEDA_IMAGE)

#define GIMP_IMAGE_TYPE_WITH_ALPHA(t)     (((t) == GIMP_RGB_IMAGE ||         \
                                            (t) == GIMP_RGBA_IMAGE) ?        \
                                           GIMP_RGBA_IMAGE :                 \
                                           ((t) == GIMP_GRAY_IMAGE ||        \
                                            (t) == GIMP_GRAYA_IMAGE) ?       \
                                           GIMP_GRAYA_IMAGE :                \
                                           ((t) == GIMP_INDEXED_IMAGE ||     \
                                            (t) == GIMP_INDEXEDA_IMAGE) ?    \
                                           GIMP_INDEXEDA_IMAGE : -1)
#define GIMP_IMAGE_TYPE_WITHOUT_ALPHA(t)  (((t) == GIMP_RGB_IMAGE ||         \
                                            (t) == GIMP_RGBA_IMAGE) ?        \
                                           GIMP_RGB_IMAGE :                  \
                                           ((t) == GIMP_GRAY_IMAGE ||        \
                                            (t) == GIMP_GRAYA_IMAGE) ?       \
                                           GIMP_GRAY_IMAGE :                 \
                                           ((t) == GIMP_INDEXED_IMAGE ||     \
                                            (t) == GIMP_INDEXEDA_IMAGE) ?    \
                                           GIMP_INDEXED_IMAGE : -1)
#define GIMP_IMAGE_TYPE_BYTES(t)           ((t) == GIMP_RGBA_IMAGE     ? 4 : \
                                            (t) == GIMP_RGB_IMAGE      ? 3 : \
                                            (t) == GIMP_GRAYA_IMAGE    ? 2 : \
                                            (t) == GIMP_GRAY_IMAGE     ? 1 : \
                                            (t) == GIMP_INDEXEDA_IMAGE ? 2 : \
                                            (t) == GIMP_INDEXED_IMAGE  ? 1 : -1)
#define GIMP_IMAGE_TYPE_BASE_TYPE(t)      (((t) == GIMP_RGB_IMAGE ||         \
                                            (t) == GIMP_RGBA_IMAGE) ?        \
                                           GIMP_RGB :                        \
                                           ((t) == GIMP_GRAY_IMAGE ||        \
                                            (t) == GIMP_GRAYA_IMAGE) ?       \
                                           GIMP_GRAY :                       \
                                           ((t) == GIMP_INDEXED_IMAGE ||     \
                                            (t) == GIMP_INDEXEDA_IMAGE) ?    \
                                           GIMP_INDEXED : -1)

#define GIMP_IMAGE_TYPE_FROM_BASE_TYPE(b)  ((b) == GIMP_RGB ?                \
                                            GIMP_RGB_IMAGE :                 \
                                            (b) == GIMP_GRAY ?               \
                                            GIMP_GRAY_IMAGE :                \
                                            (b) == GIMP_INDEXED ?            \
                                            GIMP_INDEXED_IMAGE : -1)


typedef struct _GimpImageFlushAccumulator GimpImageFlushAccumulator;

struct _GimpImageFlushAccumulator
{
  gboolean alpha_changed;
  gboolean mask_changed;
  gboolean preview_invalidated;
};


#define GIMP_TYPE_IMAGE            (gimp_image_get_type ())
#define GIMP_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE, GimpImage))
#define GIMP_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE, GimpImageClass))
#define GIMP_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE))
#define GIMP_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE))
#define GIMP_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE, GimpImageClass))


typedef struct _GimpImageClass GimpImageClass;

struct _GimpImage
{
  GimpViewable       parent_instance;

  Gimp              *gimp;                  /*  the GIMP the image belongs to*/

  gint               ID;                    /*  provides a unique ID         */

  GimpPlugInProcedure *load_proc;           /*  procedure used for loading   */
  GimpPlugInProcedure *save_proc;           /*  last save procedure used     */

  gint               width, height;         /*  width and height attributes  */
  gdouble            xresolution;           /*  image x-res, in dpi          */
  gdouble            yresolution;           /*  image y-res, in dpi          */
  GimpUnit           resolution_unit;       /*  resolution unit              */
  GimpImageBaseType  base_type;             /*  base gimp_image type         */

  guchar            *cmap;                  /*  colormap--for indexed        */
  gint               num_cols;              /*  number of cols--for indexed  */

  gint               dirty;                 /*  dirty flag -- # of ops       */
  guint              dirty_time;            /*  time when image became dirty */
  gint               undo_freeze_count;     /*  counts the _freeze's         */

  gint               instance_count;        /*  number of instances          */
  gint               disp_count;            /*  number of displays           */

  GimpTattoo         tattoo_state;          /*  the last used tattoo         */

  TileManager       *shadow;                /*  shadow buffer tiles          */

  GimpProjection    *projection;            /*  projection layers & channels */

  GList             *guides;                /*  guides                       */
  GimpGrid          *grid;                  /*  grid                         */
  GList             *sample_points;         /*  color sample points          */

  /*  Layer/Channel attributes  */
  GimpContainer     *layers;                /*  the list of layers           */
  GimpContainer     *channels;              /*  the list of masks            */
  GimpContainer     *vectors;               /*  the list of vectors          */
  GSList            *layer_stack;           /*  the layers in MRU order      */

  GQuark             layer_update_handler;
  GQuark             layer_visible_handler;
  GQuark             layer_alpha_handler;
  GQuark             channel_update_handler;
  GQuark             channel_visible_handler;
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

  /*  Preview  */
  TempBuf           *preview;               /*  the projection preview       */

  /*  Signal emmision accumulator  */
  GimpImageFlushAccumulator  flush_accum;
};

struct _GimpImageClass
{
  GimpViewableClass  parent_class;

  /*  signals  */
  void (* mode_changed)                 (GimpImage            *image);
  void (* alpha_changed)                (GimpImage            *image);
  void (* floating_selection_changed)   (GimpImage            *image);
  void (* active_layer_changed)         (GimpImage            *image);
  void (* active_channel_changed)       (GimpImage            *image);
  void (* active_vectors_changed)       (GimpImage            *image);
  void (* component_visibility_changed) (GimpImage            *image,
                                         GimpChannelType       channel);
  void (* component_active_changed)     (GimpImage            *image,
                                         GimpChannelType       channel);
  void (* mask_changed)                 (GimpImage            *image);
  void (* resolution_changed)           (GimpImage            *image);
  void (* unit_changed)                 (GimpImage            *image);
  void (* quick_mask_changed)           (GimpImage            *image);
  void (* selection_control)            (GimpImage            *image,
                                         GimpSelectionControl  control);

  void (* clean)                        (GimpImage            *image,
                                         GimpDirtyMask         dirty_mask);
  void (* dirty)                        (GimpImage            *image,
                                         GimpDirtyMask         dirty_mask);
  void (* saved)                        (GimpImage            *image,
                                         const gchar          *uri);

  void (* update)                       (GimpImage            *image,
                                         gint                  x,
                                         gint                  y,
                                         gint                  width,
                                         gint                  height);
  void (* update_guide)                 (GimpImage            *image,
                                         GimpGuide            *guide);
  void (* update_sample_point)          (GimpImage            *image,
                                         GimpSamplePoint      *sample_point);
  void (* sample_point_added)           (GimpImage            *image,
                                         GimpSamplePoint      *sample_point);
  void (* sample_point_removed)         (GimpImage            *image,
                                         GimpSamplePoint      *sample_point);
  void (* parasite_attached)            (GimpImage            *image,
                                         const gchar          *name);
  void (* parasite_detached)            (GimpImage            *image,
                                         const gchar          *name);
  void (* colormap_changed)             (GimpImage            *image,
                                         gint                  color_index);
  void (* undo_event)                   (GimpImage            *image,
                                         GimpUndoEvent         event,
                                         GimpUndo             *undo);

  void (* flush)                        (GimpImage            *image,
                                         gboolean              invalidate_preview);
};


GType           gimp_image_get_type              (void) G_GNUC_CONST;

GimpImage     * gimp_image_new                   (Gimp               *gimp,
                                                  gint                width,
                                                  gint                height,
                                                  GimpImageBaseType   base_type);

GimpImageBaseType  gimp_image_base_type            (const GimpImage  *image);
GimpImageType      gimp_image_base_type_with_alpha (const GimpImage  *image);
CombinationMode    gimp_image_get_combination_mode (GimpImageType     dest_type,
                                                    gint              src_bytes);

gint            gimp_image_get_ID                (const GimpImage    *image);
GimpImage     * gimp_image_get_by_ID             (Gimp               *gimp,
                                                  gint                id);

void            gimp_image_set_uri               (GimpImage          *image,
                                                  const gchar        *uri);
const gchar   * gimp_image_get_uri               (const GimpImage    *image);

void            gimp_image_set_filename          (GimpImage          *image,
                                                  const gchar        *filename);
gchar         * gimp_image_get_filename          (const GimpImage    *image);

void            gimp_image_set_load_proc         (GimpImage          *image,
                                                  GimpPlugInProcedure *proc);
GimpPlugInProcedure * gimp_image_get_load_proc   (const GimpImage    *image);
void            gimp_image_set_save_proc         (GimpImage          *image,
                                                  GimpPlugInProcedure *proc);
GimpPlugInProcedure * gimp_image_get_save_proc   (const GimpImage    *image);
void            gimp_image_saved                 (GimpImage          *image,
                                                  const gchar        *uri);

void            gimp_image_set_resolution        (GimpImage          *image,
                                                  gdouble             xres,
                                                  gdouble             yres);
void            gimp_image_get_resolution        (const GimpImage    *image,
                                                  gdouble            *xres,
                                                  gdouble            *yres);
void            gimp_image_resolution_changed    (GimpImage          *image);

void            gimp_image_set_unit              (GimpImage          *image,
                                                  GimpUnit            unit);
GimpUnit        gimp_image_get_unit              (const GimpImage    *image);
void            gimp_image_unit_changed          (GimpImage          *image);

gint            gimp_image_get_width             (const GimpImage    *image);
gint            gimp_image_get_height            (const GimpImage    *image);

gboolean        gimp_image_has_alpha             (const GimpImage    *image);
gboolean        gimp_image_is_empty              (const GimpImage    *image);

GimpLayer     * gimp_image_floating_sel          (const GimpImage    *image);
void       gimp_image_floating_selection_changed (GimpImage          *image);

GimpChannel   * gimp_image_get_mask              (const GimpImage    *image);
void            gimp_image_mask_changed          (GimpImage          *image);

gint            gimp_image_get_component_index   (const GimpImage    *image,
                                                  GimpChannelType     channel);

void            gimp_image_set_component_active  (GimpImage          *image,
                                                  GimpChannelType     type,
                                                  gboolean            active);
gboolean        gimp_image_get_component_active  (const GimpImage    *image,
                                                  GimpChannelType     type);

void            gimp_image_set_component_visible (GimpImage          *image,
                                                  GimpChannelType     type,
                                                  gboolean            visible);
gboolean        gimp_image_get_component_visible (const GimpImage    *image,
                                                  GimpChannelType     type);

void            gimp_image_mode_changed          (GimpImage          *image);
void            gimp_image_alpha_changed         (GimpImage          *image);
void            gimp_image_update                (GimpImage          *image,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height);
void            gimp_image_update_guide          (GimpImage          *image,
                                                  GimpGuide          *guide);
void            gimp_image_update_sample_point   (GimpImage          *image,
                                                  GimpSamplePoint    *sample_point);
void            gimp_image_sample_point_added    (GimpImage          *image,
                                                  GimpSamplePoint    *sample_point);
void            gimp_image_sample_point_removed  (GimpImage          *image,
                                                  GimpSamplePoint    *sample_point);
void            gimp_image_colormap_changed      (GimpImage          *image,
                                                  gint                col);
void            gimp_image_selection_control     (GimpImage          *image,
                                                  GimpSelectionControl  control);
void            gimp_image_quick_mask_changed    (GimpImage          *image);


/*  undo  */

gboolean        gimp_image_undo_is_enabled       (const GimpImage    *image);
gboolean        gimp_image_undo_enable           (GimpImage          *image);
gboolean        gimp_image_undo_disable          (GimpImage          *image);
gboolean        gimp_image_undo_freeze           (GimpImage          *image);
gboolean        gimp_image_undo_thaw             (GimpImage          *image);
void            gimp_image_undo_event            (GimpImage          *image,
                                                  GimpUndoEvent       event,
                                                  GimpUndo           *undo);
gint            gimp_image_dirty                 (GimpImage          *image,
                                                  GimpDirtyMask       dirty_mask);
gint            gimp_image_clean                 (GimpImage          *image,
                                                  GimpDirtyMask       dirty_mask);
void            gimp_image_clean_all             (GimpImage          *image);


/*  flush this image's displays  */

void            gimp_image_flush                 (GimpImage          *image);


/*  color transforms / utilities  */

void            gimp_image_get_foreground        (const GimpImage    *image,
                                                  GimpContext        *context,
                                                  GimpImageType       dest_type,
                                                  guchar             *fg);
void            gimp_image_get_background        (const GimpImage    *image,
                                                  GimpContext        *context,
                                                  GimpImageType       dest_type,
                                                  guchar             *bg);
void            gimp_image_get_color             (const GimpImage    *src_image,
                                                  GimpImageType       src_type,
                                                  const guchar       *src,
                                                  guchar             *rgba);
void            gimp_image_transform_rgb         (const GimpImage    *dest_image,
                                                  GimpImageType       dest_type,
                                                  const GimpRGB      *rgb,
                                                  guchar             *color);
void            gimp_image_transform_color       (const GimpImage    *dest_image,
                                                  GimpImageType       dest_type,
                                                  guchar             *dest,
                                                  GimpImageBaseType   src_type,
                                                  const guchar       *src);
TempBuf       * gimp_image_transform_temp_buf    (const GimpImage    *dest_image,
                                                  GimpImageType       dest_type,
                                                  TempBuf            *temp_buf,
                                                  gboolean           *new_buf);


/*  shadow tiles  */

TileManager   * gimp_image_get_shadow_tiles      (GimpImage          *image,
                                                  gint                width,
                                                  gint                height,
                                                  gint                bpp);
void            gimp_image_free_shadow_tiles     (GimpImage          *image);


/*  parasites  */

const GimpParasite * gimp_image_parasite_find    (const GimpImage    *image,
                                                  const gchar        *name);
gchar        ** gimp_image_parasite_list         (const GimpImage    *image,
                                                  gint               *count);
void            gimp_image_parasite_attach       (GimpImage          *image,
                                                  const GimpParasite *parasite);
void            gimp_image_parasite_detach       (GimpImage          *image,
                                                  const gchar        *name);


/*  tattoos  */

GimpTattoo      gimp_image_get_new_tattoo        (GimpImage          *image);
gboolean        gimp_image_set_tattoo_state      (GimpImage          *image,
                                                  GimpTattoo          val);
GimpTattoo      gimp_image_get_tattoo_state      (GimpImage          *image);


/*  layers / channels / vectors  */

GimpContainer * gimp_image_get_layers            (const GimpImage    *image);
GimpContainer * gimp_image_get_channels          (const GimpImage    *image);
GimpContainer * gimp_image_get_vectors           (const GimpImage    *image);

GimpDrawable  * gimp_image_get_active_drawable   (const GimpImage    *image);
GimpLayer     * gimp_image_get_active_layer      (const GimpImage    *image);
GimpChannel   * gimp_image_get_active_channel    (const GimpImage    *image);
GimpVectors   * gimp_image_get_active_vectors    (const GimpImage    *image);

GimpLayer     * gimp_image_set_active_layer      (GimpImage          *image,
                                                  GimpLayer          *layer);
GimpChannel   * gimp_image_set_active_channel    (GimpImage          *image,
                                                  GimpChannel        *channel);
GimpChannel   * gimp_image_unset_active_channel  (GimpImage          *image);
GimpVectors   * gimp_image_set_active_vectors    (GimpImage          *image,
                                                  GimpVectors        *vectors);

void            gimp_image_active_layer_changed   (GimpImage         *image);
void            gimp_image_active_channel_changed (GimpImage         *image);
void            gimp_image_active_vectors_changed (GimpImage         *image);

gint            gimp_image_get_layer_index       (const GimpImage    *image,
                                                  const GimpLayer    *layer);
gint            gimp_image_get_channel_index     (const GimpImage    *image,
                                                  const GimpChannel  *channel);
gint            gimp_image_get_vectors_index     (const GimpImage    *image,
                                                  const GimpVectors  *vectors);

GimpLayer     * gimp_image_get_layer_by_tattoo   (const GimpImage    *image,
                                                  GimpTattoo          tattoo);
GimpChannel   * gimp_image_get_channel_by_tattoo (const GimpImage    *image,
                                                  GimpTattoo          tattoo);
GimpVectors   * gimp_image_get_vectors_by_tattoo (const GimpImage    *image,
                                                  GimpTattoo          tattoo);

GimpLayer     * gimp_image_get_layer_by_name     (const GimpImage    *image,
                                                  const gchar        *name);
GimpChannel   * gimp_image_get_channel_by_name   (const GimpImage    *image,
                                                  const gchar        *name);
GimpVectors   * gimp_image_get_vectors_by_name   (const GimpImage    *image,
                                                  const gchar        *name);

gboolean        gimp_image_add_layer             (GimpImage          *image,
                                                  GimpLayer          *layer,
                                                  gint                position);
void            gimp_image_remove_layer          (GimpImage          *image,
                                                  GimpLayer          *layer);

void            gimp_image_add_layers            (GimpImage          *image,
                                                  GList              *layers,
                                                  gint                position,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height,
                                                  const gchar        *undo_desc);

gboolean        gimp_image_raise_layer           (GimpImage          *image,
                                                  GimpLayer          *layer);
gboolean        gimp_image_lower_layer           (GimpImage          *image,
                                                  GimpLayer          *layer);
gboolean        gimp_image_raise_layer_to_top    (GimpImage          *image,
                                                  GimpLayer          *layer);
gboolean        gimp_image_lower_layer_to_bottom (GimpImage          *image,
                                                  GimpLayer          *layer);
gboolean        gimp_image_position_layer        (GimpImage          *image,
                                                  GimpLayer          *layer,
                                                  gint                new_index,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc);

gboolean        gimp_image_add_channel           (GimpImage          *image,
                                                  GimpChannel        *channel,
                                                  gint                position);
void            gimp_image_remove_channel        (GimpImage          *image,
                                                  GimpChannel        *channel);

gboolean        gimp_image_raise_channel         (GimpImage          *image,
                                                  GimpChannel        *channel);
gboolean        gimp_image_raise_channel_to_top  (GimpImage          *image,
                                                  GimpChannel        *channel);
gboolean        gimp_image_lower_channel         (GimpImage          *image,
                                                  GimpChannel        *channel);
gboolean      gimp_image_lower_channel_to_bottom (GimpImage          *image,
                                                  GimpChannel        *channel);
gboolean        gimp_image_position_channel      (GimpImage          *image,
                                                  GimpChannel        *channel,
                                                  gint                new_index,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc);

gboolean        gimp_image_add_vectors           (GimpImage          *image,
                                                  GimpVectors        *vectors,
                                                  gint                position);
void            gimp_image_remove_vectors        (GimpImage          *image,
                                                  GimpVectors        *vectors);

gboolean        gimp_image_raise_vectors         (GimpImage          *image,
                                                  GimpVectors        *vectors);
gboolean        gimp_image_raise_vectors_to_top  (GimpImage          *image,
                                                  GimpVectors        *vectors);
gboolean        gimp_image_lower_vectors         (GimpImage          *image,
                                                  GimpVectors        *vectors);
gboolean      gimp_image_lower_vectors_to_bottom (GimpImage          *image,
                                                  GimpVectors        *vectors);
gboolean        gimp_image_position_vectors      (GimpImage          *image,
                                                  GimpVectors        *vectors,
                                                  gint                new_index,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc);

gboolean        gimp_image_layer_boundary        (const GimpImage    *image,
                                                  BoundSeg          **segs,
                                                  gint               *n_segs);
GimpLayer     * gimp_image_pick_correlate_layer  (const GimpImage    *image,
                                                  gint                x,
                                                  gint                y);
gboolean    gimp_image_coords_in_active_pickable (GimpImage          *image,
                                                  const GimpCoords   *coords,
                                                  gboolean            sample_merged,
                                                  gboolean            selected_only);

void        gimp_image_invalidate_layer_previews (GimpImage          *image);
void      gimp_image_invalidate_channel_previews (GimpImage          *image);


#endif /* __GIMP_IMAGE_H__ */
