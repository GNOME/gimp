/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
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

#ifndef __LIGMA_IMAGE_H__
#define __LIGMA_IMAGE_H__


#include "ligmaviewable.h"


#define LIGMA_IMAGE_ACTIVE_PARENT ((gpointer) 1)


#define LIGMA_TYPE_IMAGE            (ligma_image_get_type ())
#define LIGMA_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_IMAGE, LigmaImage))
#define LIGMA_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_IMAGE, LigmaImageClass))
#define LIGMA_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_IMAGE))
#define LIGMA_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_IMAGE))
#define LIGMA_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_IMAGE, LigmaImageClass))


typedef struct _LigmaImageClass   LigmaImageClass;
typedef struct _LigmaImagePrivate LigmaImagePrivate;

struct _LigmaImage
{
  LigmaViewable      parent_instance;

  Ligma             *ligma;  /*  the LIGMA the image belongs to  */

  LigmaImagePrivate *priv;
};

struct _LigmaImageClass
{
  LigmaViewableClass  parent_class;

  /*  signals  */
  void (* mode_changed)                 (LigmaImage            *image);
  void (* precision_changed)            (LigmaImage            *image);
  void (* alpha_changed)                (LigmaImage            *image);
  void (* floating_selection_changed)   (LigmaImage            *image);
  void (* selected_channels_changed)    (LigmaImage            *image);
  void (* selected_vectors_changed)     (LigmaImage            *image);
  void (* selected_layers_changed)      (LigmaImage            *image);
  void (* component_visibility_changed) (LigmaImage            *image,
                                         LigmaChannelType       channel);
  void (* component_active_changed)     (LigmaImage            *image,
                                         LigmaChannelType       channel);
  void (* mask_changed)                 (LigmaImage            *image);
  void (* resolution_changed)           (LigmaImage            *image);
  void (* size_changed_detailed)        (LigmaImage            *image,
                                         gint                  previous_origin_x,
                                         gint                  previous_origin_y,
                                         gint                  previous_width,
                                         gint                  previous_height);
  void (* unit_changed)                 (LigmaImage            *image);
  void (* quick_mask_changed)           (LigmaImage            *image);
  void (* selection_invalidate)         (LigmaImage            *image);

  void (* clean)                        (LigmaImage            *image,
                                         LigmaDirtyMask         dirty_mask);
  void (* dirty)                        (LigmaImage            *image,
                                         LigmaDirtyMask         dirty_mask);
  void (* saving)                       (LigmaImage            *image);
  void (* saved)                        (LigmaImage            *image,
                                         GFile                *file);
  void (* exported)                     (LigmaImage            *image,
                                         GFile                *file);

  void (* guide_added)                  (LigmaImage            *image,
                                         LigmaGuide            *guide);
  void (* guide_removed)                (LigmaImage            *image,
                                         LigmaGuide            *guide);
  void (* guide_moved)                  (LigmaImage            *image,
                                         LigmaGuide            *guide);
  void (* sample_point_added)           (LigmaImage            *image,
                                         LigmaSamplePoint      *sample_point);
  void (* sample_point_removed)         (LigmaImage            *image,
                                         LigmaSamplePoint      *sample_point);
  void (* sample_point_moved)           (LigmaImage            *image,
                                         LigmaSamplePoint      *sample_point);
  void (* parasite_attached)            (LigmaImage            *image,
                                         const gchar          *name);
  void (* parasite_detached)            (LigmaImage            *image,
                                         const gchar          *name);
  void (* colormap_changed)             (LigmaImage            *image,
                                         gint                  color_index);
  void (* undo_event)                   (LigmaImage            *image,
                                         LigmaUndoEvent         event,
                                         LigmaUndo             *undo);
  void (* layer_sets_changed)           (LigmaImage            *image);
  void (* channel_sets_changed)         (LigmaImage            *image);
  void (* vectors_sets_changed)         (LigmaImage            *image);
};


GType           ligma_image_get_type              (void) G_GNUC_CONST;

LigmaImage     * ligma_image_new                   (Ligma               *ligma,
                                                  gint                width,
                                                  gint                height,
                                                  LigmaImageBaseType   base_type,
                                                  LigmaPrecision       precision);

gint64          ligma_image_estimate_memsize      (LigmaImage          *image,
                                                  LigmaComponentType   component_type,
                                                  gint                width,
                                                  gint                height);

LigmaImageBaseType  ligma_image_get_base_type      (LigmaImage          *image);
LigmaComponentType  ligma_image_get_component_type (LigmaImage          *image);
LigmaPrecision      ligma_image_get_precision      (LigmaImage          *image);

const Babl    * ligma_image_get_format            (LigmaImage          *image,
                                                  LigmaImageBaseType   base_type,
                                                  LigmaPrecision       precision,
                                                  gboolean            with_alpha,
                                                  const Babl         *space);

const Babl    * ligma_image_get_layer_space       (LigmaImage          *image);
const Babl    * ligma_image_get_layer_format      (LigmaImage          *image,
                                                  gboolean            with_alpha);
const Babl    * ligma_image_get_channel_format    (LigmaImage          *image);
const Babl    * ligma_image_get_mask_format       (LigmaImage          *image);

LigmaLayerMode   ligma_image_get_default_new_layer_mode
                                                 (LigmaImage          *image);
void            ligma_image_unset_default_new_layer_mode
                                                 (LigmaImage          *image);

gint            ligma_image_get_id                (LigmaImage          *image);
LigmaImage     * ligma_image_get_by_id             (Ligma               *ligma,
                                                  gint                id);

GFile         * ligma_image_get_file              (LigmaImage          *image);
GFile         * ligma_image_get_untitled_file     (LigmaImage          *image);
GFile         * ligma_image_get_file_or_untitled  (LigmaImage          *image);
GFile         * ligma_image_get_imported_file     (LigmaImage          *image);
GFile         * ligma_image_get_exported_file     (LigmaImage          *image);
GFile         * ligma_image_get_save_a_copy_file  (LigmaImage          *image);
GFile         * ligma_image_get_any_file          (LigmaImage          *image);

void            ligma_image_set_file              (LigmaImage          *image,
                                                  GFile              *file);
void            ligma_image_set_imported_file     (LigmaImage          *image,
                                                  GFile              *file);
void            ligma_image_set_exported_file     (LigmaImage          *image,
                                                  GFile              *file);
void            ligma_image_set_save_a_copy_file  (LigmaImage          *image,
                                                  GFile              *file);

const gchar   * ligma_image_get_display_name      (LigmaImage          *image);
const gchar   * ligma_image_get_display_path      (LigmaImage          *image);

void            ligma_image_set_load_proc         (LigmaImage          *image,
                                                  LigmaPlugInProcedure *proc);
LigmaPlugInProcedure * ligma_image_get_load_proc   (LigmaImage          *image);
void            ligma_image_set_save_proc         (LigmaImage          *image,
                                                  LigmaPlugInProcedure *proc);
LigmaPlugInProcedure * ligma_image_get_save_proc   (LigmaImage          *image);
void            ligma_image_saving                (LigmaImage          *image);
void            ligma_image_saved                 (LigmaImage          *image,
                                                  GFile              *file);
void            ligma_image_set_export_proc       (LigmaImage          *image,
                                                  LigmaPlugInProcedure *proc);
LigmaPlugInProcedure * ligma_image_get_export_proc (LigmaImage          *image);
void            ligma_image_exported              (LigmaImage          *image,
                                                  GFile              *file);

gint            ligma_image_get_xcf_version       (LigmaImage          *image,
                                                  gboolean            zlib_compression,
                                                  gint               *ligma_version,
                                                  const gchar       **version_string,
                                                  gchar             **version_reason);

void            ligma_image_set_xcf_compression   (LigmaImage          *image,
                                                  gboolean            compression);
gboolean        ligma_image_get_xcf_compression   (LigmaImage          *image);

void            ligma_image_set_resolution        (LigmaImage          *image,
                                                  gdouble             xres,
                                                  gdouble             yres);
void            ligma_image_get_resolution        (LigmaImage          *image,
                                                  gdouble            *xres,
                                                  gdouble            *yres);
void            ligma_image_resolution_changed    (LigmaImage          *image);

void            ligma_image_set_unit              (LigmaImage          *image,
                                                  LigmaUnit            unit);
LigmaUnit        ligma_image_get_unit              (LigmaImage          *image);
void            ligma_image_unit_changed          (LigmaImage          *image);

gint            ligma_image_get_width             (LigmaImage          *image);
gint            ligma_image_get_height            (LigmaImage          *image);

gboolean        ligma_image_has_alpha             (LigmaImage          *image);
gboolean        ligma_image_is_empty              (LigmaImage          *image);

void           ligma_image_set_floating_selection (LigmaImage          *image,
                                                  LigmaLayer          *floating_sel);
LigmaLayer    * ligma_image_get_floating_selection (LigmaImage          *image);
void       ligma_image_floating_selection_changed (LigmaImage          *image);

LigmaChannel   * ligma_image_get_mask              (LigmaImage          *image);
void            ligma_image_mask_changed          (LigmaImage          *image);
gboolean        ligma_image_mask_intersect        (LigmaImage          *image,
                                                  GList              *items,
                                                  gint               *x,
                                                  gint               *y,
                                                  gint               *width,
                                                  gint               *height);


/*  image components  */

const Babl    * ligma_image_get_component_format  (LigmaImage          *image,
                                                  LigmaChannelType     channel);
gint            ligma_image_get_component_index   (LigmaImage          *image,
                                                  LigmaChannelType     channel);

void            ligma_image_set_component_active  (LigmaImage          *image,
                                                  LigmaChannelType     type,
                                                  gboolean            active);
gboolean        ligma_image_get_component_active  (LigmaImage          *image,
                                                  LigmaChannelType     type);
void            ligma_image_get_active_array      (LigmaImage          *image,
                                                  gboolean           *components);
LigmaComponentMask ligma_image_get_active_mask     (LigmaImage          *image);

void            ligma_image_set_component_visible (LigmaImage          *image,
                                                  LigmaChannelType     type,
                                                  gboolean            visible);
gboolean        ligma_image_get_component_visible (LigmaImage          *image,
                                                  LigmaChannelType     type);
void            ligma_image_get_visible_array     (LigmaImage          *image,
                                                  gboolean           *components);
LigmaComponentMask ligma_image_get_visible_mask    (LigmaImage          *image);


/*  emitting image signals  */

void            ligma_image_mode_changed          (LigmaImage          *image);
void            ligma_image_precision_changed     (LigmaImage          *image);
void            ligma_image_alpha_changed         (LigmaImage          *image);
void            ligma_image_invalidate            (LigmaImage          *image,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height);
void            ligma_image_invalidate_all        (LigmaImage          *image);
void            ligma_image_guide_added           (LigmaImage          *image,
                                                  LigmaGuide          *guide);
void            ligma_image_guide_removed         (LigmaImage          *image,
                                                  LigmaGuide          *guide);
void            ligma_image_guide_moved           (LigmaImage          *image,
                                                  LigmaGuide          *guide);

void            ligma_image_sample_point_added    (LigmaImage          *image,
                                                  LigmaSamplePoint    *sample_point);
void            ligma_image_sample_point_removed  (LigmaImage          *image,
                                                  LigmaSamplePoint    *sample_point);
void            ligma_image_sample_point_moved    (LigmaImage          *image,
                                                  LigmaSamplePoint    *sample_point);
void            ligma_image_colormap_changed      (LigmaImage          *image,
                                                  gint                col);
void            ligma_image_selection_invalidate  (LigmaImage          *image);
void            ligma_image_quick_mask_changed    (LigmaImage          *image);
void            ligma_image_size_changed_detailed (LigmaImage          *image,
                                                  gint                previous_origin_x,
                                                  gint                previous_origin_y,
                                                  gint                previous_width,
                                                  gint                previous_height);
void            ligma_image_undo_event            (LigmaImage          *image,
                                                  LigmaUndoEvent       event,
                                                  LigmaUndo           *undo);


/*  dirty counters  */

gint            ligma_image_dirty                 (LigmaImage          *image,
                                                  LigmaDirtyMask       dirty_mask);
gint            ligma_image_clean                 (LigmaImage          *image,
                                                  LigmaDirtyMask       dirty_mask);
void            ligma_image_clean_all             (LigmaImage          *image);
void            ligma_image_export_clean_all      (LigmaImage          *image);
gint            ligma_image_is_dirty              (LigmaImage          *image);
gboolean        ligma_image_is_export_dirty       (LigmaImage          *image);
gint64          ligma_image_get_dirty_time        (LigmaImage          *image);


/*  flush this image's displays  */

void            ligma_image_flush                 (LigmaImage          *image);


/*  display / instance counters  */

gint            ligma_image_get_display_count     (LigmaImage          *image);
void            ligma_image_inc_display_count     (LigmaImage          *image);
void            ligma_image_dec_display_count     (LigmaImage          *image);

gint            ligma_image_get_instance_count    (LigmaImage          *image);
void            ligma_image_inc_instance_count    (LigmaImage          *image);

void            ligma_image_inc_show_all_count    (LigmaImage          *image);
void            ligma_image_dec_show_all_count    (LigmaImage          *image);


/*  parasites  */

const LigmaParasite * ligma_image_parasite_find    (LigmaImage          *image,
                                                  const gchar        *name);
gchar        ** ligma_image_parasite_list         (LigmaImage          *image);
gboolean        ligma_image_parasite_validate     (LigmaImage          *image,
                                                  const LigmaParasite *parasite,
                                                  GError            **error);
void            ligma_image_parasite_attach       (LigmaImage          *image,
                                                  const LigmaParasite *parasite,
                                                  gboolean            push_undo);
void            ligma_image_parasite_detach       (LigmaImage          *image,
                                                  const gchar        *name,
                                                  gboolean            push_undo);


/*  tattoos  */

LigmaTattoo      ligma_image_get_new_tattoo        (LigmaImage          *image);
gboolean        ligma_image_set_tattoo_state      (LigmaImage          *image,
                                                  LigmaTattoo          val);
LigmaTattoo      ligma_image_get_tattoo_state      (LigmaImage          *image);


/*  projection  */

LigmaProjection * ligma_image_get_projection       (LigmaImage          *image);


/*  layers / channels / vectors  */

LigmaItemTree  * ligma_image_get_layer_tree        (LigmaImage          *image);
LigmaItemTree  * ligma_image_get_channel_tree      (LigmaImage          *image);
LigmaItemTree  * ligma_image_get_vectors_tree      (LigmaImage          *image);

LigmaContainer * ligma_image_get_layers            (LigmaImage          *image);
LigmaContainer * ligma_image_get_channels          (LigmaImage          *image);
LigmaContainer * ligma_image_get_vectors           (LigmaImage          *image);

gint            ligma_image_get_n_layers          (LigmaImage          *image);
gint            ligma_image_get_n_channels        (LigmaImage          *image);
gint            ligma_image_get_n_vectors         (LigmaImage          *image);

GList         * ligma_image_get_layer_iter        (LigmaImage          *image);
GList         * ligma_image_get_channel_iter      (LigmaImage          *image);
GList         * ligma_image_get_vectors_iter      (LigmaImage          *image);

GList         * ligma_image_get_layer_list        (LigmaImage          *image);
GList         * ligma_image_get_channel_list      (LigmaImage          *image);
GList         * ligma_image_get_vectors_list      (LigmaImage          *image);

LigmaLayer     * ligma_image_get_active_layer      (LigmaImage          *image);
LigmaChannel   * ligma_image_get_active_channel    (LigmaImage          *image);
LigmaVectors   * ligma_image_get_active_vectors    (LigmaImage          *image);

LigmaLayer     * ligma_image_set_active_layer      (LigmaImage          *image,
                                                  LigmaLayer          *layer);
LigmaChannel   * ligma_image_set_active_channel    (LigmaImage          *image,
                                                  LigmaChannel        *channel);
void          ligma_image_unset_selected_channels (LigmaImage          *image);
LigmaVectors   * ligma_image_set_active_vectors    (LigmaImage          *image,
                                                  LigmaVectors        *vectors);

gboolean        ligma_image_is_selected_drawable  (LigmaImage          *image,
                                                  LigmaDrawable       *drawable);
gboolean     ligma_image_equal_selected_drawables (LigmaImage          *image,
                                                  GList              *drawables);

GList        * ligma_image_get_selected_drawables (LigmaImage          *image);
GList         * ligma_image_get_selected_layers   (LigmaImage          *image);
GList         * ligma_image_get_selected_channels (LigmaImage          *image);
GList         * ligma_image_get_selected_vectors  (LigmaImage          *image);

void            ligma_image_set_selected_layers   (LigmaImage          *image,
                                                  GList              *layers);
void            ligma_image_set_selected_channels (LigmaImage          *image,
                                                  GList              *channels);
void            ligma_image_set_selected_vectors  (LigmaImage          *image,
                                                  GList              *vectors);

LigmaLayer     * ligma_image_get_layer_by_tattoo   (LigmaImage          *image,
                                                  LigmaTattoo          tattoo);
LigmaChannel   * ligma_image_get_channel_by_tattoo (LigmaImage          *image,
                                                  LigmaTattoo          tattoo);
LigmaVectors   * ligma_image_get_vectors_by_tattoo (LigmaImage          *image,
                                                  LigmaTattoo          tattoo);

LigmaLayer     * ligma_image_get_layer_by_name     (LigmaImage          *image,
                                                  const gchar        *name);
LigmaChannel   * ligma_image_get_channel_by_name   (LigmaImage          *image,
                                                  const gchar        *name);
LigmaVectors   * ligma_image_get_vectors_by_name   (LigmaImage          *image,
                                                  const gchar        *name);

gboolean        ligma_image_reorder_item          (LigmaImage          *image,
                                                  LigmaItem           *item,
                                                  LigmaItem           *new_parent,
                                                  gint                new_index,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc);
gboolean        ligma_image_raise_item            (LigmaImage          *image,
                                                  LigmaItem           *item,
                                                  GError            **error);
gboolean        ligma_image_raise_item_to_top     (LigmaImage          *image,
                                                  LigmaItem           *item);
gboolean        ligma_image_lower_item            (LigmaImage          *image,
                                                  LigmaItem           *item,
                                                  GError            **error);
gboolean        ligma_image_lower_item_to_bottom  (LigmaImage          *image,
                                                  LigmaItem           *item);

gboolean        ligma_image_add_layer             (LigmaImage          *image,
                                                  LigmaLayer          *layer,
                                                  LigmaLayer          *parent,
                                                  gint                position,
                                                  gboolean            push_undo);
void            ligma_image_remove_layer          (LigmaImage          *image,
                                                  LigmaLayer          *layer,
                                                  gboolean            push_undo,
                                                  GList              *new_selected);

void            ligma_image_add_layers            (LigmaImage          *image,
                                                  GList              *layers,
                                                  LigmaLayer          *parent,
                                                  gint                position,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height,
                                                  const gchar        *undo_desc);

void            ligma_image_store_item_set        (LigmaImage          *image,
                                                  LigmaItemList       *set);
gboolean        ligma_image_unlink_item_set       (LigmaImage          *image,
                                                  LigmaItemList       *set);
GList         * ligma_image_get_stored_item_sets   (LigmaImage         *image,
                                                   GType              item_type);
void            ligma_image_select_item_set       (LigmaImage          *image,
                                                  LigmaItemList       *set);
void            ligma_image_add_item_set          (LigmaImage          *image,
                                                  LigmaItemList       *set);
void            ligma_image_remove_item_set       (LigmaImage          *image,
                                                  LigmaItemList       *set);
void            ligma_image_intersect_item_set    (LigmaImage        *image,
                                                  LigmaItemList       *set);

gboolean        ligma_image_add_channel           (LigmaImage          *image,
                                                  LigmaChannel        *channel,
                                                  LigmaChannel        *parent,
                                                  gint                position,
                                                  gboolean            push_undo);
void            ligma_image_remove_channel        (LigmaImage          *image,
                                                  LigmaChannel        *channel,
                                                  gboolean            push_undo,
                                                  GList              *new_selected);

gboolean        ligma_image_add_vectors           (LigmaImage          *image,
                                                  LigmaVectors        *vectors,
                                                  LigmaVectors        *parent,
                                                  gint                position,
                                                  gboolean            push_undo);
void            ligma_image_remove_vectors        (LigmaImage          *image,
                                                  LigmaVectors        *vectors,
                                                  gboolean            push_undo,
                                                  GList              *new_selected);

gboolean        ligma_image_add_hidden_item       (LigmaImage          *image,
                                                  LigmaItem           *item);
void            ligma_image_remove_hidden_item    (LigmaImage          *image,
                                                  LigmaItem           *item);
gboolean        ligma_image_is_hidden_item        (LigmaImage          *image,
                                                  LigmaItem           *item);

gboolean    ligma_image_coords_in_active_pickable (LigmaImage          *image,
                                                  const LigmaCoords   *coords,
                                                  gboolean            show_all,
                                                  gboolean            sample_merged,
                                                  gboolean            selected_only);

void            ligma_image_invalidate_previews   (LigmaImage          *image);


#endif /* __LIGMA_IMAGE_H__ */
