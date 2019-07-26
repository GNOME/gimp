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

#ifndef __APP_GIMP_PARAM_SPECS_H__
#define __APP_GIMP_PARAM_SPECS_H__


/*
 * GIMP_TYPE_PARAM_ENUM
 */

#define GIMP_TYPE_PARAM_ENUM           (gimp_param_enum_get_type ())
#define GIMP_PARAM_SPEC_ENUM(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_ENUM, GimpParamSpecEnum))

#define GIMP_IS_PARAM_SPEC_ENUM(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_ENUM))

typedef struct _GimpParamSpecEnum GimpParamSpecEnum;

struct _GimpParamSpecEnum
{
  GParamSpecEnum  parent_instance;

  GSList         *excluded_values;
};

GType        gimp_param_enum_get_type     (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_enum         (const gchar       *name,
                                           const gchar       *nick,
                                           const gchar       *blurb,
                                           GType              enum_type,
                                           gint               default_value,
                                           GParamFlags        flags);

void   gimp_param_spec_enum_exclude_value (GimpParamSpecEnum *espec,
                                           gint               value);


/*
 * GIMP_TYPE_IMAGE_ID
 */

#define GIMP_TYPE_IMAGE_ID               (gimp_image_id_get_type ())
#define GIMP_VALUE_HOLDS_IMAGE_ID(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                          GIMP_TYPE_IMAGE_ID))

GType   gimp_image_id_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_IMAGE_ID
 */

#define GIMP_TYPE_PARAM_IMAGE_ID           (gimp_param_image_id_get_type ())
#define GIMP_PARAM_SPEC_IMAGE_ID(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_IMAGE_ID, GimpParamSpecImageID))
#define GIMP_IS_PARAM_SPEC_IMAGE_ID(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_IMAGE_ID))

typedef struct _GimpParamSpecImageID GimpParamSpecImageID;

struct _GimpParamSpecImageID
{
  GParamSpecInt  parent_instance;

  Gimp          *gimp;
  gboolean       none_ok;
};

GType        gimp_param_image_id_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_image_id     (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           Gimp         *gimp,
                                           gboolean      none_ok,
                                           GParamFlags   flags);

GimpImage  * gimp_value_get_image         (const GValue *value,
                                           Gimp         *gimp);
void         gimp_value_set_image         (GValue       *value,
                                           GimpImage    *image);



/*
 * GIMP_TYPE_ITEM_ID
 */

#define GIMP_TYPE_ITEM_ID               (gimp_item_id_get_type ())
#define GIMP_VALUE_HOLDS_ITEM_ID(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                         GIMP_TYPE_ITEM_ID))

GType   gimp_item_id_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_ITEM_ID
 */

#define GIMP_TYPE_PARAM_ITEM_ID           (gimp_param_item_id_get_type ())
#define GIMP_PARAM_SPEC_ITEM_ID(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_ITEM_ID, GimpParamSpecItemID))
#define GIMP_IS_PARAM_SPEC_ITEM_ID(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_ITEM_ID))

typedef struct _GimpParamSpecItemID GimpParamSpecItemID;

struct _GimpParamSpecItemID
{
  GParamSpecInt  parent_instance;

  Gimp          *gimp;
  GType          item_type;
  gboolean       none_ok;
};

GType        gimp_param_item_id_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_item_id     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          Gimp         *gimp,
                                          gboolean      none_ok,
                                          GParamFlags   flags);

GimpItem   * gimp_value_get_item         (const GValue *value,
                                          Gimp         *gimp);
void         gimp_value_set_item         (GValue       *value,
                                          GimpItem     *item);


/*
 * GIMP_TYPE_DRAWABLE_ID
 */

#define GIMP_TYPE_DRAWABLE_ID               (gimp_drawable_id_get_type ())
#define GIMP_VALUE_HOLDS_DRAWABLE_ID(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                             GIMP_TYPE_DRAWABLE_ID))

GType   gimp_drawable_id_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_DRAWABLE_ID
 */

#define GIMP_TYPE_PARAM_DRAWABLE_ID           (gimp_param_drawable_id_get_type ())
#define GIMP_PARAM_SPEC_DRAWABLE_ID(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_DRAWABLE_ID, GimpParamSpecDrawableID))
#define GIMP_IS_PARAM_SPEC_DRAWABLE_ID(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_DRAWABLE_ID))

typedef struct _GimpParamSpecDrawableID GimpParamSpecDrawableID;

struct _GimpParamSpecDrawableID
{
  GimpParamSpecItemID parent_instance;
};

GType         gimp_param_drawable_id_get_type (void) G_GNUC_CONST;

GParamSpec  * gimp_param_spec_drawable_id     (const gchar  *name,
                                               const gchar  *nick,
                                               const gchar  *blurb,
                                               Gimp         *gimp,
                                               gboolean      none_ok,
                                               GParamFlags   flags);

GimpDrawable * gimp_value_get_drawable        (const GValue *value,
                                               Gimp         *gimp);
void           gimp_value_set_drawable        (GValue       *value,
                                               GimpDrawable *drawable);


/*
 * GIMP_TYPE_LAYER_ID
 */

#define GIMP_TYPE_LAYER_ID               (gimp_layer_id_get_type ())
#define GIMP_VALUE_HOLDS_LAYER_ID(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                          GIMP_TYPE_LAYER_ID))

GType   gimp_layer_id_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_LAYER_ID
 */

#define GIMP_TYPE_PARAM_LAYER_ID           (gimp_param_layer_id_get_type ())
#define GIMP_PARAM_SPEC_LAYER_ID(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_LAYER_ID, GimpParamSpecLayerID))
#define GIMP_IS_PARAM_SPEC_LAYER_ID(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_LAYER_ID))

typedef struct _GimpParamSpecLayerID GimpParamSpecLayerID;

struct _GimpParamSpecLayerID
{
  GimpParamSpecDrawableID parent_instance;
};

GType        gimp_param_layer_id_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_layer_id     (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           Gimp         *gimp,
                                           gboolean      none_ok,
                                           GParamFlags   flags);

GimpLayer  * gimp_value_get_layer         (const GValue *value,
                                           Gimp         *gimp);
void         gimp_value_set_layer         (GValue       *value,
                                           GimpLayer    *layer);


/*
 * GIMP_TYPE_CHANNEL_ID
 */

#define GIMP_TYPE_CHANNEL_ID               (gimp_channel_id_get_type ())
#define GIMP_VALUE_HOLDS_CHANNEL_ID(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                            GIMP_TYPE_CHANNEL_ID))

GType   gimp_channel_id_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_CHANNEL_ID
 */

#define GIMP_TYPE_PARAM_CHANNEL_ID           (gimp_param_channel_id_get_type ())
#define GIMP_PARAM_SPEC_CHANNEL_ID(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_CHANNEL_ID, GimpParamSpecChannelID))
#define GIMP_IS_PARAM_SPEC_CHANNEL_ID(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_CHANNEL_ID))

typedef struct _GimpParamSpecChannelID GimpParamSpecChannelID;

struct _GimpParamSpecChannelID
{
  GimpParamSpecDrawableID parent_instance;
};

GType         gimp_param_channel_id_get_type (void) G_GNUC_CONST;

GParamSpec  * gimp_param_spec_channel_id     (const gchar  *name,
                                              const gchar  *nick,
                                              const gchar  *blurb,
                                              Gimp         *gimp,
                                              gboolean      none_ok,
                                              GParamFlags   flags);

GimpChannel * gimp_value_get_channel         (const GValue *value,
                                              Gimp         *gimp);
void          gimp_value_set_channel         (GValue       *value,
                                              GimpChannel  *channel);


/*
 * GIMP_TYPE_LAYER_MASK_ID
 */

#define GIMP_TYPE_LAYER_MASK_ID               (gimp_layer_mask_id_get_type ())
#define GIMP_VALUE_HOLDS_LAYER_MASK_ID(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                               GIMP_TYPE_LAYER_MASK_ID))

GType   gimp_layer_mask_id_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_LAYER_MASK_ID
 */

#define GIMP_TYPE_PARAM_LAYER_MASK_ID           (gimp_param_layer_mask_id_get_type ())
#define GIMP_PARAM_SPEC_LAYER_MASK_ID(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_LAYER_MASK_ID, GimpParamSpecLayerMaskID))
#define GIMP_IS_PARAM_SPEC_LAYER_MASK_ID(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_LAYER_MASK_ID))

typedef struct _GimpParamSpecLayerMaskID GimpParamSpecLayerMaskID;

struct _GimpParamSpecLayerMaskID
{
  GimpParamSpecChannelID parent_instance;
};

GType           gimp_param_layer_mask_id_get_type (void) G_GNUC_CONST;

GParamSpec    * gimp_param_spec_layer_mask_id     (const gchar   *name,
                                                   const gchar   *nick,
                                                   const gchar   *blurb,
                                                   Gimp          *gimp,
                                                   gboolean       none_ok,
                                                   GParamFlags    flags);

GimpLayerMask * gimp_value_get_layer_mask         (const GValue  *value,
                                                   Gimp          *gimp);
void            gimp_value_set_layer_mask         (GValue        *value,
                                                   GimpLayerMask *layer_mask);


/*
 * GIMP_TYPE_SELECTION_ID
 */

#define GIMP_TYPE_SELECTION_ID               (gimp_selection_id_get_type ())
#define GIMP_VALUE_HOLDS_SELECTION_ID(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                              GIMP_TYPE_SELECTION_ID))

GType   gimp_selection_id_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_SELECTION_ID
 */

#define GIMP_TYPE_PARAM_SELECTION_ID           (gimp_param_selection_id_get_type ())
#define GIMP_PARAM_SPEC_SELECTION_ID(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_SELECTION_ID, GimpParamSpecSelectionID))
#define GIMP_IS_PARAM_SPEC_SELECTION_ID(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_SELECTION_ID))

typedef struct _GimpParamSpecSelectionID GimpParamSpecSelectionID;

struct _GimpParamSpecSelectionID
{
  GimpParamSpecChannelID parent_instance;
};

GType           gimp_param_selection_id_get_type (void) G_GNUC_CONST;

GParamSpec    * gimp_param_spec_selection_id     (const gchar   *name,
                                                  const gchar   *nick,
                                                  const gchar   *blurb,
                                                  Gimp          *gimp,
                                                  gboolean       none_ok,
                                                  GParamFlags    flags);

GimpSelection * gimp_value_get_selection         (const GValue  *value,
                                                  Gimp          *gimp);
void            gimp_value_set_selection         (GValue        *value,
                                                  GimpSelection *selection);


/*
 * GIMP_TYPE_VECTORS_ID
 */

#define GIMP_TYPE_VECTORS_ID               (gimp_vectors_id_get_type ())
#define GIMP_VALUE_HOLDS_VECTORS_ID(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                            GIMP_TYPE_VECTORS_ID))

GType   gimp_vectors_id_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_VECTORS_ID
 */

#define GIMP_TYPE_PARAM_VECTORS_ID           (gimp_param_vectors_id_get_type ())
#define GIMP_PARAM_SPEC_VECTORS_ID(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_VECTORS_ID, GimpParamSpecVectorsID))
#define GIMP_IS_PARAM_SPEC_VECTORS_ID(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_VECTORS_ID))

typedef struct _GimpParamSpecVectorsID GimpParamSpecVectorsID;

struct _GimpParamSpecVectorsID
{
  GimpParamSpecItemID parent_instance;
};

GType         gimp_param_vectors_id_get_type (void) G_GNUC_CONST;

GParamSpec  * gimp_param_spec_vectors_id     (const gchar  *name,
                                              const gchar  *nick,
                                              const gchar  *blurb,
                                              Gimp         *gimp,
                                              gboolean      none_ok,
                                              GParamFlags   flags);

GimpVectors * gimp_value_get_vectors         (const GValue *value,
                                              Gimp         *gimp);
void          gimp_value_set_vectors         (GValue       *value,
                                              GimpVectors  *vectors);


/*
 * GIMP_TYPE_DISPLAY_ID
 */

#define GIMP_TYPE_DISPLAY_ID               (gimp_display_id_get_type ())
#define GIMP_VALUE_HOLDS_DISPLAY_ID(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                            GIMP_TYPE_DISPLAY_ID))

GType   gimp_display_id_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_DISPLAY_ID
 */

#define GIMP_TYPE_PARAM_DISPLAY_ID           (gimp_param_display_id_get_type ())
#define GIMP_PARAM_SPEC_DISPLAY_ID(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_DISPLAY_ID, GimpParamSpecDisplayID))
#define GIMP_IS_PARAM_SPEC_DISPLAY_ID(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_DISPLAY_ID))

typedef struct _GimpParamSpecDisplayID GimpParamSpecDisplayID;

struct _GimpParamSpecDisplayID
{
  GParamSpecInt  parent_instance;

  Gimp          *gimp;
  gboolean       none_ok;
};

GType        gimp_param_display_id_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_display_id     (const gchar  *name,
                                             const gchar  *nick,
                                             const gchar  *blurb,
                                             Gimp         *gimp,
                                             gboolean      none_ok,
                                             GParamFlags   flags);

GimpObject * gimp_value_get_display         (const GValue *value,
                                             Gimp         *gimp);
void         gimp_value_set_display         (GValue       *value,
                                             GimpObject   *display);


#endif  /*  __APP_GIMP_PARAM_SPECS_H__  */
