/* The GIMP -- an image manipulation program
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

  PlugInProcDef     *save_proc;             /*  last PDB save proc used      */

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

  GimpTattoo         tattoo_state;          /*  the next unique tattoo to use*/

  TileManager       *shadow;                /*  shadow buffer tiles          */

  GimpProjection    *projection;            /*  projection layers & channels */

  GList             *guides;                /*  guides                       */
  GimpGrid          *grid;                  /*  grid                         */

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

  gboolean           qmask_state;           /*  TRUE if qmask is on          */
  gboolean           qmask_inverted;        /*  TRUE if qmask is inverted    */
  GimpRGB            qmask_color;           /*  rgba triplet of the color    */

  /*  Undo apparatus  */
  GimpUndoStack     *undo_stack;            /*  stack for undo operations    */
  GimpUndoStack     *redo_stack;            /*  stack for redo operations    */
  gint               group_count;           /*  nested undo groups           */
  GimpUndoType       pushing_undo_group;    /*  undo group status flag       */

  /*  Composite preview  */
  TempBuf           *comp_preview;          /*  the composite preview        */
  gboolean           comp_preview_valid;    /*  preview valid-1/channel      */

  /*  Signal emmision accumulator  */
  GimpImageFlushAccumulator  flush_accum;
};

struct _GimpImageClass
{
  GimpViewableClass  parent_class;

  /*  signals  */
  void (* mode_changed)                 (GimpImage            *gimage);
  void (* alpha_changed)                (GimpImage            *gimage);
  void (* floating_selection_changed)   (GimpImage            *gimage);
  void (* active_layer_changed)         (GimpImage            *gimage);
  void (* active_channel_changed)       (GimpImage            *gimage);
  void (* active_vectors_changed)       (GimpImage            *gimage);
  void (* component_visibility_changed) (GimpImage            *gimage,
					 GimpChannelType       channel);
  void (* component_active_changed)     (GimpImage            *gimage,
					 GimpChannelType       channel);
  void (* mask_changed)                 (GimpImage            *gimage);
  void (* resolution_changed)           (GimpImage            *gimage);
  void (* unit_changed)                 (GimpImage            *gimage);
  void (* qmask_changed)                (GimpImage            *gimage);
  void (* selection_control)            (GimpImage            *gimage,
                                         GimpSelectionControl  control);

  void (* clean)                        (GimpImage            *gimage,
                                         GimpDirtyMask         dirty_mask);
  void (* dirty)                        (GimpImage            *gimage,
                                         GimpDirtyMask         dirty_mask);
  void (* update)                       (GimpImage            *gimage,
					 gint                  x,
					 gint                  y,
					 gint                  width,
					 gint                  height);
  void (* update_guide)                 (GimpImage            *gimage,
                                         GimpGuide            *guide);
  void (* colormap_changed)             (GimpImage            *gimage,
					 gint                  color_index);
  void (* undo_event)                   (GimpImage            *gimage,
					 GimpUndoEvent         event,
                                         GimpUndo             *undo);

  void (* flush)                        (GimpImage            *gimage);
};


GType           gimp_image_get_type              (void) G_GNUC_CONST;

GimpImage     * gimp_image_new                   (Gimp               *gimp,
                                                  gint                width,
                                                  gint                height,
                                                  GimpImageBaseType   base_type);

GimpImageBaseType  gimp_image_base_type            (const GimpImage  *gimage);
GimpImageType	   gimp_image_base_type_with_alpha (const GimpImage  *gimage);
CombinationMode    gimp_image_get_combination_mode (GimpImageType     dest_type,
                                                    gint              src_bytes);

gint            gimp_image_get_ID                (const GimpImage    *gimage);
GimpImage     * gimp_image_get_by_ID             (Gimp               *gimp,
                                                  gint                id);

void            gimp_image_set_uri               (GimpImage          *gimage,
                                                  const gchar        *uri);
const gchar   * gimp_image_get_uri               (const GimpImage    *gimage);

void            gimp_image_set_filename          (GimpImage          *gimage,
                                                  const gchar        *filename);
gchar         * gimp_image_get_filename          (const GimpImage    *gimage);

void            gimp_image_set_save_proc         (GimpImage          *gimage,
                                                  PlugInProcDef      *proc);
PlugInProcDef * gimp_image_get_save_proc         (const GimpImage    *gimage);

void            gimp_image_set_resolution        (GimpImage          *gimage,
                                                  gdouble             xres,
                                                  gdouble             yres);
void            gimp_image_get_resolution        (const GimpImage    *gimage,
                                                  gdouble            *xres,
                                                  gdouble            *yres);
void            gimp_image_resolution_changed    (GimpImage          *gimage);

void            gimp_image_set_unit              (GimpImage          *gimage,
                                                  GimpUnit            unit);
GimpUnit        gimp_image_get_unit              (const GimpImage    *gimage);
void            gimp_image_unit_changed          (GimpImage          *gimage);

gint		gimp_image_get_width             (const GimpImage    *gimage);
gint		gimp_image_get_height            (const GimpImage    *gimage);

gboolean        gimp_image_has_alpha             (const GimpImage    *gimage);
gboolean        gimp_image_is_empty              (const GimpImage    *gimage);

GimpLayer     * gimp_image_floating_sel          (const GimpImage    *gimage);
void       gimp_image_floating_selection_changed (GimpImage          *gimage);

GimpChannel   * gimp_image_get_mask              (const GimpImage    *gimage);
void            gimp_image_mask_changed          (GimpImage          *gimage);

gint            gimp_image_get_component_index   (const GimpImage    *gimage,
                                                  GimpChannelType     channel);

void            gimp_image_set_component_active  (GimpImage          *gimage,
						  GimpChannelType     type,
						  gboolean            active);
gboolean        gimp_image_get_component_active  (const GimpImage    *gimage,
						  GimpChannelType     type);

void            gimp_image_set_component_visible (GimpImage          *gimage,
						  GimpChannelType     type,
						  gboolean            visible);
gboolean        gimp_image_get_component_visible (const GimpImage    *gimage,
						  GimpChannelType     type);

void            gimp_image_mode_changed          (GimpImage          *gimage);
void            gimp_image_alpha_changed         (GimpImage          *gimage);
void            gimp_image_update                (GimpImage          *gimage,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height);
void            gimp_image_update_guide          (GimpImage          *gimage,
                                                  GimpGuide          *guide);
void		gimp_image_colormap_changed      (GimpImage          *gimage,
                                                  gint                col);
void            gimp_image_selection_control     (GimpImage          *gimage,
                                                  GimpSelectionControl  control);
void            gimp_image_qmask_changed         (GimpImage          *gimage);


/*  undo  */

gboolean        gimp_image_undo_is_enabled       (const GimpImage    *gimage);
gboolean        gimp_image_undo_enable           (GimpImage          *gimage);
gboolean        gimp_image_undo_disable          (GimpImage          *gimage);
gboolean        gimp_image_undo_freeze           (GimpImage          *gimage);
gboolean        gimp_image_undo_thaw             (GimpImage          *gimage);
void		gimp_image_undo_event            (GimpImage          *gimage,
                                                  GimpUndoEvent       event,
                                                  GimpUndo           *undo);
gint            gimp_image_dirty                 (GimpImage          *gimage,
                                                  GimpDirtyMask       dirty_mask);
gint            gimp_image_clean                 (GimpImage          *gimage,
                                                  GimpDirtyMask       dirty_mask);
void            gimp_image_clean_all             (GimpImage          *gimage);


/*  flush this image's displays  */

void            gimp_image_flush                 (GimpImage          *gimage);


/*  color transforms / utilities  */

void            gimp_image_get_foreground        (const GimpImage    *gimage,
                                                  const GimpDrawable *drawable,
                                                  GimpContext        *context,
                                                  guchar             *fg);
void            gimp_image_get_background        (const GimpImage    *gimage,
                                                  const GimpDrawable *drawable,
                                                  GimpContext        *context,
                                                  guchar             *bg);
void            gimp_image_get_color             (const GimpImage    *src_gimage,
                                                  GimpImageType       src_type,
                                                  const guchar       *src,
                                                  guchar             *rgba);
void            gimp_image_transform_rgb         (const GimpImage    *dest_gimage,
                                                  const GimpDrawable *dest_drawable,
                                                  const GimpRGB      *rgb,
                                                  guchar             *color);
void            gimp_image_transform_color       (const GimpImage    *dest_gimage,
                                                  const GimpDrawable *dest_drawable,
                                                  guchar             *dest,
                                                  GimpImageBaseType   src_type,
                                                  const guchar       *src);
TempBuf       * gimp_image_transform_temp_buf    (const GimpImage    *dest_gimage,
                                                  const GimpDrawable *dest_drawable,
                                                  TempBuf            *temp_buf,
                                                  gboolean           *new_buf);


/*  shadow tiles  */

TileManager   * gimp_image_shadow                (GimpImage          *gimage,
                                                  gint                width,
                                                  gint                height,
                                                  gint                bpp);
void            gimp_image_free_shadow           (GimpImage          *gimage);


/*  parasites  */

GimpParasite  * gimp_image_parasite_find         (const GimpImage    *gimage,
                                                  const gchar        *name);
gchar        ** gimp_image_parasite_list         (const GimpImage    *gimage,
                                                  gint               *count);
void            gimp_image_parasite_attach       (GimpImage          *gimage,
                                                  GimpParasite       *parasite);
void            gimp_image_parasite_detach       (GimpImage          *gimage,
                                                  const gchar        *parasite);


/*  tattoos  */

GimpTattoo      gimp_image_get_new_tattoo        (GimpImage          *gimage);
gboolean        gimp_image_set_tattoo_state      (GimpImage          *gimage,
                                                  GimpTattoo          val);
GimpTattoo      gimp_image_get_tattoo_state      (GimpImage          *gimage);


/*  layers / channels / vectors / old paths  */

GimpContainer * gimp_image_get_layers            (const GimpImage    *gimage);
GimpContainer * gimp_image_get_channels          (const GimpImage    *gimage);
GimpContainer * gimp_image_get_vectors           (const GimpImage    *gimage);

GimpDrawable  * gimp_image_active_drawable       (const GimpImage    *gimage);
GimpLayer     * gimp_image_get_active_layer      (const GimpImage    *gimage);
GimpChannel   * gimp_image_get_active_channel    (const GimpImage    *gimage);
GimpVectors   * gimp_image_get_active_vectors    (const GimpImage    *gimage);

GimpLayer     * gimp_image_set_active_layer      (GimpImage          *gimage,
						  GimpLayer          *layer);
GimpChannel   * gimp_image_set_active_channel    (GimpImage          *gimage,
						  GimpChannel        *channel);
GimpChannel   * gimp_image_unset_active_channel  (GimpImage          *gimage);
GimpVectors   * gimp_image_set_active_vectors    (GimpImage          *gimage,
                                                  GimpVectors        *vectors);

void            gimp_image_active_layer_changed   (GimpImage         *gimage);
void            gimp_image_active_channel_changed (GimpImage         *gimage);
void            gimp_image_active_vectors_changed (GimpImage         *gimage);

gint            gimp_image_get_layer_index       (const GimpImage    *gimage,
						  const GimpLayer    *layer);
gint            gimp_image_get_channel_index     (const GimpImage    *gimage,
						  const GimpChannel  *channel);
gint            gimp_image_get_vectors_index     (const GimpImage    *gimage,
                                                  const GimpVectors  *vectors);

GimpLayer     * gimp_image_get_layer_by_tattoo   (const GimpImage    *gimage,
						  GimpTattoo          tatoo);
GimpChannel   * gimp_image_get_channel_by_tattoo (const GimpImage    *gimage,
						  GimpTattoo          tatoo);
GimpVectors   * gimp_image_get_vectors_by_tattoo (const GimpImage    *gimage,
                                                  GimpTattoo          tatoo);

GimpLayer     * gimp_image_get_layer_by_name     (const GimpImage    *gimage,
						  const gchar        *name);
GimpChannel   * gimp_image_get_channel_by_name   (const GimpImage    *gimage,
						  const gchar        *name);
GimpVectors   * gimp_image_get_vectors_by_name   (const GimpImage    *gimage,
						  const gchar        *name);

gboolean        gimp_image_add_layer             (GimpImage          *gimage,
						  GimpLayer          *layer,
						  gint                position);
void            gimp_image_remove_layer          (GimpImage          *gimage,
						  GimpLayer          *layer);

gboolean        gimp_image_raise_layer           (GimpImage          *gimage,
						  GimpLayer          *layer);
gboolean        gimp_image_lower_layer           (GimpImage          *gimage,
						  GimpLayer          *layer);
gboolean        gimp_image_raise_layer_to_top    (GimpImage          *gimage,
						  GimpLayer          *layer);
gboolean        gimp_image_lower_layer_to_bottom (GimpImage          *gimage,
						  GimpLayer          *layer);
gboolean        gimp_image_position_layer        (GimpImage          *gimage,
						  GimpLayer          *layer,
						  gint                new_index,
						  gboolean            push_undo,
                                                  const gchar        *undo_desc);

gboolean        gimp_image_add_channel           (GimpImage          *gimage,
						  GimpChannel        *channel,
						  gint                position);
void            gimp_image_remove_channel        (GimpImage          *gimage,
						  GimpChannel        *channel);

gboolean        gimp_image_raise_channel         (GimpImage          *gimage,
						  GimpChannel        *channel);
gboolean        gimp_image_raise_channel_to_top  (GimpImage          *gimage,
						  GimpChannel        *channel);
gboolean        gimp_image_lower_channel         (GimpImage          *gimage,
						  GimpChannel        *channel);
gboolean      gimp_image_lower_channel_to_bottom (GimpImage          *gimage,
						  GimpChannel        *channel);
gboolean        gimp_image_position_channel      (GimpImage          *gimage,
						  GimpChannel        *channel,
						  gint                new_index,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc);

gboolean        gimp_image_add_vectors           (GimpImage          *gimage,
						  GimpVectors        *vectors,
						  gint                position);
void            gimp_image_remove_vectors        (GimpImage          *gimage,
						  GimpVectors        *vectors);

gboolean        gimp_image_raise_vectors         (GimpImage          *gimage,
						  GimpVectors        *vectors);
gboolean        gimp_image_raise_vectors_to_top  (GimpImage          *gimage,
						  GimpVectors        *vectors);
gboolean        gimp_image_lower_vectors         (GimpImage          *gimage,
						  GimpVectors        *vectors);
gboolean      gimp_image_lower_vectors_to_bottom (GimpImage          *gimage,
						  GimpVectors        *vectors);
gboolean        gimp_image_position_vectors      (GimpImage          *gimage,
						  GimpVectors        *vectors,
						  gint                new_index,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc);

gboolean        gimp_image_layer_boundary        (const GimpImage    *gimage,
						  BoundSeg          **segs,
						  gint               *n_segs);
GimpLayer     * gimp_image_pick_correlate_layer  (const GimpImage    *gimage,
						  gint                x,
						  gint                y);
gboolean    gimp_image_coords_in_active_drawable (GimpImage          *gimage,
                                                  const GimpCoords   *coords);

void        gimp_image_invalidate_layer_previews (GimpImage          *gimage);
void      gimp_image_invalidate_channel_previews (GimpImage          *gimage);


#endif /* __GIMP_IMAGE_H__ */
