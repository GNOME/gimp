/* GIMP - The GNU Image Manipulation Program
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

#pragma once

#include "gimpviewable.h"


#define GIMP_IMAGE_ACTIVE_PARENT ((gpointer) 1)


#define GIMP_TYPE_IMAGE            (gimp_image_get_type ())
#define GIMP_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE, GimpImage))
#define GIMP_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE, GimpImageClass))
#define GIMP_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE))
#define GIMP_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE))
#define GIMP_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE, GimpImageClass))


typedef struct _GimpImageClass   GimpImageClass;
typedef struct _GimpImagePrivate GimpImagePrivate;

struct _GimpImage
{
  GimpViewable      parent_instance;

  Gimp             *gimp;  /*  the GIMP the image belongs to  */

  GimpImagePrivate *priv;
};

struct _GimpImageClass
{
  GimpViewableClass  parent_class;

  /*  signals  */
  void (* mode_changed)                 (GimpImage            *image);
  void (* precision_changed)            (GimpImage            *image);
  void (* alpha_changed)                (GimpImage            *image);
  void (* floating_selection_changed)   (GimpImage            *image);
  void (* selected_channels_changed)    (GimpImage            *image);
  void (* selected_paths_changed)       (GimpImage            *image);
  void (* selected_layers_changed)      (GimpImage            *image);
  void (* component_visibility_changed) (GimpImage            *image,
                                         GimpChannelType       channel);
  void (* component_active_changed)     (GimpImage            *image,
                                         GimpChannelType       channel);
  void (* mask_changed)                 (GimpImage            *image);
  void (* resolution_changed)           (GimpImage            *image);
  void (* size_changed_detailed)        (GimpImage            *image,
                                         gint                  previous_origin_x,
                                         gint                  previous_origin_y,
                                         gint                  previous_width,
                                         gint                  previous_height);
  void (* unit_changed)                 (GimpImage            *image);
  void (* quick_mask_changed)           (GimpImage            *image);
  void (* selection_invalidate)         (GimpImage            *image);

  void (* clean)                        (GimpImage            *image,
                                         GimpDirtyMask         dirty_mask);
  void (* dirty)                        (GimpImage            *image,
                                         GimpDirtyMask         dirty_mask);
  void (* saving)                       (GimpImage            *image);
  void (* saved)                        (GimpImage            *image,
                                         GFile                *file);
  void (* exported)                     (GimpImage            *image,
                                         GFile                *file);

  void (* guide_added)                  (GimpImage            *image,
                                         GimpGuide            *guide);
  void (* guide_removed)                (GimpImage            *image,
                                         GimpGuide            *guide);
  void (* guide_moved)                  (GimpImage            *image,
                                         GimpGuide            *guide);
  void (* sample_point_added)           (GimpImage            *image,
                                         GimpSamplePoint      *sample_point);
  void (* sample_point_removed)         (GimpImage            *image,
                                         GimpSamplePoint      *sample_point);
  void (* sample_point_moved)           (GimpImage            *image,
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
  void (* item_sets_changed)            (GimpImage            *image,
                                         GType                 item_type);
};


GType           gimp_image_get_type              (void) G_GNUC_CONST;

GimpImage     * gimp_image_new                   (Gimp               *gimp,
                                                  gint                width,
                                                  gint                height,
                                                  GimpImageBaseType   base_type,
                                                  GimpPrecision       precision);

gint64          gimp_image_estimate_memsize      (GimpImage          *image,
                                                  GimpComponentType   component_type,
                                                  gint                width,
                                                  gint                height);

GimpImageBaseType  gimp_image_get_base_type      (GimpImage          *image);
GimpComponentType  gimp_image_get_component_type (GimpImage          *image);
GimpPrecision      gimp_image_get_precision      (GimpImage          *image);

const Babl    * gimp_image_get_format            (GimpImage          *image,
                                                  GimpImageBaseType   base_type,
                                                  GimpPrecision       precision,
                                                  gboolean            with_alpha,
                                                  const Babl         *space);

const Babl    * gimp_image_get_layer_space       (GimpImage          *image);
const Babl    * gimp_image_get_layer_format      (GimpImage          *image,
                                                  gboolean            with_alpha);
const Babl    * gimp_image_get_channel_format    (GimpImage          *image);
const Babl    * gimp_image_get_mask_format       (GimpImage          *image);

GimpLayerMode   gimp_image_get_default_new_layer_mode
                                                 (GimpImage          *image);
void            gimp_image_unset_default_new_layer_mode
                                                 (GimpImage          *image);

gint            gimp_image_get_id                (GimpImage          *image);
GimpImage     * gimp_image_get_by_id             (Gimp               *gimp,
                                                  gint                id);

GFile         * gimp_image_get_file              (GimpImage          *image);
GFile         * gimp_image_get_untitled_file     (GimpImage          *image);
GFile         * gimp_image_get_file_or_untitled  (GimpImage          *image);
GFile         * gimp_image_get_imported_file     (GimpImage          *image);
GFile         * gimp_image_get_exported_file     (GimpImage          *image);
GFile         * gimp_image_get_save_a_copy_file  (GimpImage          *image);
GFile         * gimp_image_get_any_file          (GimpImage          *image);

void            gimp_image_set_file              (GimpImage          *image,
                                                  GFile              *file);
void            gimp_image_set_imported_file     (GimpImage          *image,
                                                  GFile              *file);
void            gimp_image_set_exported_file     (GimpImage          *image,
                                                  GFile              *file);
void            gimp_image_set_save_a_copy_file  (GimpImage          *image,
                                                  GFile              *file);

const gchar   * gimp_image_get_display_name      (GimpImage          *image);
const gchar   * gimp_image_get_display_path      (GimpImage          *image);

void            gimp_image_set_load_proc         (GimpImage          *image,
                                                  GimpPlugInProcedure *proc);
GimpPlugInProcedure * gimp_image_get_load_proc   (GimpImage          *image);
void            gimp_image_set_save_proc         (GimpImage          *image,
                                                  GimpPlugInProcedure *proc);
GimpPlugInProcedure * gimp_image_get_save_proc   (GimpImage          *image);
void            gimp_image_saving                (GimpImage          *image);
void            gimp_image_saved                 (GimpImage          *image,
                                                  GFile              *file);
void            gimp_image_set_export_proc       (GimpImage          *image,
                                                  GimpPlugInProcedure *proc);
GimpPlugInProcedure * gimp_image_get_export_proc (GimpImage          *image);
void            gimp_image_exported              (GimpImage          *image,
                                                  GFile              *file);

gint            gimp_image_get_xcf_version       (GimpImage          *image,
                                                  gboolean            zlib_compression,
                                                  gint               *gimp_version,
                                                  const gchar       **version_string,
                                                  gchar             **version_reason);

void            gimp_image_set_xcf_compression   (GimpImage          *image,
                                                  gboolean            compression);
gboolean        gimp_image_get_xcf_compression   (GimpImage          *image);

void            gimp_image_set_resolution        (GimpImage          *image,
                                                  gdouble             xres,
                                                  gdouble             yres);
void            gimp_image_get_resolution        (GimpImage          *image,
                                                  gdouble            *xres,
                                                  gdouble            *yres);
void            gimp_image_resolution_changed    (GimpImage          *image);

void            gimp_image_set_unit              (GimpImage          *image,
                                                  GimpUnit           *unit);
GimpUnit      * gimp_image_get_unit              (GimpImage          *image);
void            gimp_image_unit_changed          (GimpImage          *image);

gint            gimp_image_get_width             (GimpImage          *image);
gint            gimp_image_get_height            (GimpImage          *image);

gboolean        gimp_image_has_alpha             (GimpImage          *image);
gboolean        gimp_image_is_empty              (GimpImage          *image);

void           gimp_image_set_floating_selection (GimpImage          *image,
                                                  GimpLayer          *floating_sel);
GimpLayer    * gimp_image_get_floating_selection (GimpImage          *image);
void       gimp_image_floating_selection_changed (GimpImage          *image);

GimpChannel   * gimp_image_get_mask              (GimpImage          *image);
void            gimp_image_mask_changed          (GimpImage          *image);
gboolean        gimp_image_mask_intersect        (GimpImage          *image,
                                                  GList              *items,
                                                  gint               *x,
                                                  gint               *y,
                                                  gint               *width,
                                                  gint               *height);


/*  image components  */

const Babl    * gimp_image_get_component_format  (GimpImage          *image,
                                                  GimpChannelType     channel);
gint            gimp_image_get_component_index   (GimpImage          *image,
                                                  GimpChannelType     channel);

void            gimp_image_set_component_active  (GimpImage          *image,
                                                  GimpChannelType     type,
                                                  gboolean            active);
gboolean        gimp_image_get_component_active  (GimpImage          *image,
                                                  GimpChannelType     type);
void            gimp_image_get_active_array      (GimpImage          *image,
                                                  gboolean           *components);
GimpComponentMask gimp_image_get_active_mask     (GimpImage          *image);

void            gimp_image_set_component_visible (GimpImage          *image,
                                                  GimpChannelType     type,
                                                  gboolean            visible);
gboolean        gimp_image_get_component_visible (GimpImage          *image,
                                                  GimpChannelType     type);
void            gimp_image_get_visible_array     (GimpImage          *image,
                                                  gboolean           *components);
GimpComponentMask gimp_image_get_visible_mask    (GimpImage          *image);


/*  emitting image signals  */

void            gimp_image_mode_changed          (GimpImage          *image);
void            gimp_image_precision_changed     (GimpImage          *image);
void            gimp_image_alpha_changed         (GimpImage          *image);
void            gimp_image_invalidate            (GimpImage          *image,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height);
void            gimp_image_invalidate_all        (GimpImage          *image);
void            gimp_image_guide_added           (GimpImage          *image,
                                                  GimpGuide          *guide);
void            gimp_image_guide_removed         (GimpImage          *image,
                                                  GimpGuide          *guide);
void            gimp_image_guide_moved           (GimpImage          *image,
                                                  GimpGuide          *guide);

void            gimp_image_sample_point_added    (GimpImage          *image,
                                                  GimpSamplePoint    *sample_point);
void            gimp_image_sample_point_removed  (GimpImage          *image,
                                                  GimpSamplePoint    *sample_point);
void            gimp_image_sample_point_moved    (GimpImage          *image,
                                                  GimpSamplePoint    *sample_point);
void            gimp_image_colormap_changed      (GimpImage          *image,
                                                  gint                col);
void            gimp_image_selection_invalidate  (GimpImage          *image);
void            gimp_image_quick_mask_changed    (GimpImage          *image);
void            gimp_image_size_changed_detailed (GimpImage          *image,
                                                  gint                previous_origin_x,
                                                  gint                previous_origin_y,
                                                  gint                previous_width,
                                                  gint                previous_height);
void            gimp_image_undo_event            (GimpImage          *image,
                                                  GimpUndoEvent       event,
                                                  GimpUndo           *undo);


/*  dirty counters  */

gint            gimp_image_dirty                 (GimpImage          *image,
                                                  GimpDirtyMask       dirty_mask);
gint            gimp_image_clean                 (GimpImage          *image,
                                                  GimpDirtyMask       dirty_mask);
void            gimp_image_clean_all             (GimpImage          *image);
void            gimp_image_export_clean_all      (GimpImage          *image);
gint            gimp_image_is_dirty              (GimpImage          *image);
gboolean        gimp_image_is_export_dirty       (GimpImage          *image);
gint64          gimp_image_get_dirty_time        (GimpImage          *image);


/*  flush this image's displays  */

void            gimp_image_flush                 (GimpImage          *image);


/*  display / instance counters  */

gint            gimp_image_get_display_count     (GimpImage          *image);
void            gimp_image_inc_display_count     (GimpImage          *image);
void            gimp_image_dec_display_count     (GimpImage          *image);

gint            gimp_image_get_instance_count    (GimpImage          *image);
void            gimp_image_inc_instance_count    (GimpImage          *image);

void            gimp_image_inc_show_all_count    (GimpImage          *image);
void            gimp_image_dec_show_all_count    (GimpImage          *image);


/*  parasites  */

const GimpParasite * gimp_image_parasite_find    (GimpImage          *image,
                                                  const gchar        *name);
gchar        ** gimp_image_parasite_list         (GimpImage          *image);
gboolean        gimp_image_parasite_validate     (GimpImage          *image,
                                                  const GimpParasite *parasite,
                                                  GError            **error);
void            gimp_image_parasite_attach       (GimpImage          *image,
                                                  const GimpParasite *parasite,
                                                  gboolean            push_undo);
void            gimp_image_parasite_detach       (GimpImage          *image,
                                                  const gchar        *name,
                                                  gboolean            push_undo);


/*  tattoos  */

GimpTattoo      gimp_image_get_new_tattoo        (GimpImage          *image);
gboolean        gimp_image_set_tattoo_state      (GimpImage          *image,
                                                  GimpTattoo          val);
GimpTattoo      gimp_image_get_tattoo_state      (GimpImage          *image);


/*  projection  */

GimpProjection * gimp_image_get_projection       (GimpImage          *image);


/*  layers / channels / paths  */

GimpItemTree  * gimp_image_get_layer_tree        (GimpImage          *image);
GimpItemTree  * gimp_image_get_channel_tree      (GimpImage          *image);
GimpItemTree  * gimp_image_get_path_tree         (GimpImage          *image);

GimpContainer * gimp_image_get_items             (GimpImage          *image,
                                                  GType               item_type);
GimpContainer * gimp_image_get_layers            (GimpImage          *image);
GimpContainer * gimp_image_get_channels          (GimpImage          *image);
GimpContainer * gimp_image_get_paths             (GimpImage          *image);

gint            gimp_image_get_n_layers          (GimpImage          *image);
gint            gimp_image_get_n_channels        (GimpImage          *image);
gint            gimp_image_get_n_paths           (GimpImage          *image);

GList         * gimp_image_get_layer_iter        (GimpImage          *image);
GList         * gimp_image_get_channel_iter      (GimpImage          *image);
GList         * gimp_image_get_path_iter         (GimpImage          *image);

GList         * gimp_image_get_layer_list        (GimpImage          *image);
GList         * gimp_image_get_channel_list      (GimpImage          *image);
GList         * gimp_image_get_path_list         (GimpImage          *image);

void          gimp_image_unset_selected_channels (GimpImage          *image);

gboolean        gimp_image_is_selected_drawable  (GimpImage          *image,
                                                  GimpDrawable       *drawable);
gboolean     gimp_image_equal_selected_drawables (GimpImage          *image,
                                                  GList              *drawables);

GList        * gimp_image_get_selected_drawables (GimpImage          *image);

GList         * gimp_image_get_selected_items    (GimpImage          *image,
                                                  GType               item_type);
GList         * gimp_image_get_selected_layers   (GimpImage          *image);
GList         * gimp_image_get_selected_channels (GimpImage          *image);
GList         * gimp_image_get_selected_paths    (GimpImage          *image);

void            gimp_image_set_selected_items    (GimpImage          *image,
                                                  GType               item_type,
                                                  GList              *items);
void            gimp_image_set_selected_layers   (GimpImage          *image,
                                                  GList              *layers);
void            gimp_image_set_selected_channels (GimpImage          *image,
                                                  GList              *channels);
void            gimp_image_set_selected_paths    (GimpImage          *image,
                                                  GList              *paths);

GimpLayer     * gimp_image_get_layer_by_tattoo   (GimpImage          *image,
                                                  GimpTattoo          tattoo);
GimpChannel   * gimp_image_get_channel_by_tattoo (GimpImage          *image,
                                                  GimpTattoo          tattoo);
GimpPath      * gimp_image_get_path_by_tattoo    (GimpImage          *image,
                                                  GimpTattoo          tattoo);

GimpLayer     * gimp_image_get_layer_by_name     (GimpImage          *image,
                                                  const gchar        *name);
GimpChannel   * gimp_image_get_channel_by_name   (GimpImage          *image,
                                                  const gchar        *name);
GimpPath      * gimp_image_get_path_by_name      (GimpImage          *image,
                                                  const gchar        *name);

gboolean        gimp_image_reorder_item          (GimpImage          *image,
                                                  GimpItem           *item,
                                                  GimpItem           *new_parent,
                                                  gint                new_index,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc);
gboolean        gimp_image_raise_item            (GimpImage          *image,
                                                  GimpItem           *item,
                                                  GError            **error);
gboolean        gimp_image_raise_item_to_top     (GimpImage          *image,
                                                  GimpItem           *item);
gboolean        gimp_image_lower_item            (GimpImage          *image,
                                                  GimpItem           *item,
                                                  GError            **error);
gboolean        gimp_image_lower_item_to_bottom  (GimpImage          *image,
                                                  GimpItem           *item);

gboolean        gimp_image_add_item              (GimpImage          *image,
                                                  GimpItem           *item,
                                                  GimpItem          *parent,
                                                  gint                position,
                                                  gboolean            push_undo);
void            gimp_image_remove_item           (GimpImage          *image,
                                                  GimpItem           *item,
                                                  gboolean            push_undo,
                                                  GList              *new_selected);

gboolean        gimp_image_add_layer             (GimpImage          *image,
                                                  GimpLayer          *layer,
                                                  GimpLayer          *parent,
                                                  gint                position,
                                                  gboolean            push_undo);
void            gimp_image_remove_layer          (GimpImage          *image,
                                                  GimpLayer          *layer,
                                                  gboolean            push_undo,
                                                  GList              *new_selected);

void            gimp_image_add_layers            (GimpImage          *image,
                                                  GList              *layers,
                                                  GimpLayer          *parent,
                                                  gint                position,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height,
                                                  const gchar        *undo_desc);

gboolean        gimp_image_add_channel           (GimpImage          *image,
                                                  GimpChannel        *channel,
                                                  GimpChannel        *parent,
                                                  gint                position,
                                                  gboolean            push_undo);
void            gimp_image_remove_channel        (GimpImage          *image,
                                                  GimpChannel        *channel,
                                                  gboolean            push_undo,
                                                  GList              *new_selected);

gboolean        gimp_image_add_path              (GimpImage          *image,
                                                  GimpPath           *path,
                                                  GimpPath           *parent,
                                                  gint                position,
                                                  gboolean            push_undo);
void            gimp_image_remove_path           (GimpImage          *image,
                                                  GimpPath           *path,
                                                  gboolean            push_undo,
                                                  GList              *new_selected);

void            gimp_image_store_item_set        (GimpImage          *image,
                                                  GimpItemList       *set);
gboolean        gimp_image_unlink_item_set       (GimpImage          *image,
                                                  GimpItemList       *set);
GList         * gimp_image_get_stored_item_sets   (GimpImage         *image,
                                                   GType              item_type);
void            gimp_image_select_item_set       (GimpImage          *image,
                                                  GimpItemList       *set);
void            gimp_image_add_item_set          (GimpImage          *image,
                                                  GimpItemList       *set);
void            gimp_image_remove_item_set       (GimpImage          *image,
                                                  GimpItemList       *set);
void            gimp_image_intersect_item_set    (GimpImage        *image,
                                                  GimpItemList       *set);

gboolean        gimp_image_add_hidden_item       (GimpImage          *image,
                                                  GimpItem           *item);
void            gimp_image_remove_hidden_item    (GimpImage          *image,
                                                  GimpItem           *item);
gboolean        gimp_image_is_hidden_item        (GimpImage          *image,
                                                  GimpItem           *item);

gboolean    gimp_image_coords_in_active_pickable (GimpImage          *image,
                                                  const GimpCoords   *coords,
                                                  gboolean            show_all,
                                                  gboolean            sample_merged,
                                                  gboolean            selected_only);

void            gimp_image_invalidate_previews   (GimpImage          *image);

void            gimp_image_set_converting        (GimpImage          *image,
                                                  gboolean            converting);
gboolean        gimp_image_get_converting        (GimpImage          *image);
