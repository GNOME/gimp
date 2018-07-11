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

#ifndef __GIMP_PARAM_SPECS_H__
#define __GIMP_PARAM_SPECS_H__


/*
 * Keep in sync with libgimpconfig/gimpconfig-params.h
 */
#define GIMP_PARAM_NO_VALIDATE (1 << (6 + G_PARAM_USER_SHIFT))


/*
 * GIMP_TYPE_INT32
 */

#define GIMP_TYPE_INT32               (gimp_int32_get_type ())
#define GIMP_VALUE_HOLDS_INT32(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                       GIMP_TYPE_INT32))

GType   gimp_int32_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_INT32
 */

#define GIMP_TYPE_PARAM_INT32           (gimp_param_int32_get_type ())
#define GIMP_PARAM_SPEC_INT32(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_INT32, GimpParamSpecInt32))
#define GIMP_IS_PARAM_SPEC_INT32(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_INT32))

typedef struct _GimpParamSpecInt32 GimpParamSpecInt32;

struct _GimpParamSpecInt32
{
  GParamSpecInt parent_instance;
};

GType        gimp_param_int32_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_int32     (const gchar *name,
                                        const gchar *nick,
                                        const gchar *blurb,
                                        gint         minimum,
                                        gint         maximum,
                                        gint         default_value,
                                        GParamFlags  flags);


/*
 * GIMP_TYPE_INT16
 */

#define GIMP_TYPE_INT16               (gimp_int16_get_type ())
#define GIMP_VALUE_HOLDS_INT16(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                       GIMP_TYPE_INT16))

GType   gimp_int16_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_INT16
 */

#define GIMP_TYPE_PARAM_INT16           (gimp_param_int16_get_type ())
#define GIMP_PARAM_SPEC_INT16(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_INT16, GimpParamSpecInt16))
#define GIMP_IS_PARAM_SPEC_INT16(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_INT16))

typedef struct _GimpParamSpecInt16 GimpParamSpecInt16;

struct _GimpParamSpecInt16
{
  GParamSpecInt parent_instance;
};

GType        gimp_param_int16_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_int16     (const gchar *name,
                                        const gchar *nick,
                                        const gchar *blurb,
                                        gint         minimum,
                                        gint         maximum,
                                        gint         default_value,
                                        GParamFlags  flags);


/*
 * GIMP_TYPE_INT8
 */

#define GIMP_TYPE_INT8               (gimp_int8_get_type ())
#define GIMP_VALUE_HOLDS_INT8(value) (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                      GIMP_TYPE_INT8))

GType   gimp_int8_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_INT8
 */

#define GIMP_TYPE_PARAM_INT8           (gimp_param_int8_get_type ())
#define GIMP_PARAM_SPEC_INT8(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_INT8, GimpParamSpecInt8))
#define GIMP_IS_PARAM_SPEC_INT8(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_INT8))

typedef struct _GimpParamSpecInt8 GimpParamSpecInt8;

struct _GimpParamSpecInt8
{
  GParamSpecUInt parent_instance;
};

GType        gimp_param_int8_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_int8     (const gchar *name,
                                       const gchar *nick,
                                       const gchar *blurb,
                                       guint        minimum,
                                       guint        maximum,
                                       guint        default_value,
                                       GParamFlags  flags);


/*
 * GIMP_TYPE_PARAM_STRING
 */

#define GIMP_TYPE_PARAM_STRING           (gimp_param_string_get_type ())
#define GIMP_PARAM_SPEC_STRING(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_STRING, GimpParamSpecString))
#define GIMP_IS_PARAM_SPEC_STRING(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_STRING))

typedef struct _GimpParamSpecString GimpParamSpecString;

struct _GimpParamSpecString
{
  GParamSpecString parent_instance;

  guint            allow_non_utf8 : 1;
  guint            non_empty      : 1;
};

GType        gimp_param_string_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_string     (const gchar *name,
                                         const gchar *nick,
                                         const gchar *blurb,
                                         gboolean     allow_non_utf8,
                                         gboolean     null_ok,
                                         gboolean     non_empty,
                                         const gchar *default_value,
                                         GParamFlags  flags);


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


/*
 * GIMP_TYPE_ARRAY
 */

typedef struct _GimpArray GimpArray;

struct _GimpArray
{
  guint8   *data;
  gsize     length;
  gboolean  static_data;
};

GimpArray * gimp_array_new  (const guint8    *data,
                             gsize            length,
                             gboolean         static_data);
GimpArray * gimp_array_copy (const GimpArray *array);
void        gimp_array_free (GimpArray       *array);

#define GIMP_TYPE_ARRAY               (gimp_array_get_type ())
#define GIMP_VALUE_HOLDS_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_ARRAY))

GType   gimp_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_ARRAY
 */

#define GIMP_TYPE_PARAM_ARRAY           (gimp_param_array_get_type ())
#define GIMP_PARAM_SPEC_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_ARRAY, GimpParamSpecArray))
#define GIMP_IS_PARAM_SPEC_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_ARRAY))

typedef struct _GimpParamSpecArray GimpParamSpecArray;

struct _GimpParamSpecArray
{
  GParamSpecBoxed parent_instance;
};

GType        gimp_param_array_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_array     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        GParamFlags   flags);


/*
 * GIMP_TYPE_INT8_ARRAY
 */

#define GIMP_TYPE_INT8_ARRAY               (gimp_int8_array_get_type ())
#define GIMP_VALUE_HOLDS_INT8_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_INT8_ARRAY))

GType   gimp_int8_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_INT8_ARRAY
 */

#define GIMP_TYPE_PARAM_INT8_ARRAY           (gimp_param_int8_array_get_type ())
#define GIMP_PARAM_SPEC_INT8_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_INT8_ARRAY, GimpParamSpecInt8Array))
#define GIMP_IS_PARAM_SPEC_INT8_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_INT8_ARRAY))

typedef struct _GimpParamSpecInt8Array GimpParamSpecInt8Array;

struct _GimpParamSpecInt8Array
{
  GimpParamSpecArray parent_instance;
};

GType          gimp_param_int8_array_get_type  (void) G_GNUC_CONST;

GParamSpec   * gimp_param_spec_int8_array      (const gchar  *name,
                                                const gchar  *nick,
                                                const gchar  *blurb,
                                                GParamFlags   flags);

const guint8 * gimp_value_get_int8array        (const GValue *value);
guint8       * gimp_value_dup_int8array        (const GValue *value);
void           gimp_value_set_int8array        (GValue       *value,
                                                const guint8 *array,
                                                gsize         length);
void           gimp_value_set_static_int8array (GValue       *value,
                                                const guint8 *array,
                                                gsize         length);
void           gimp_value_take_int8array       (GValue       *value,
                                                guint8       *array,
                                                gsize         length);


/*
 * GIMP_TYPE_INT16_ARRAY
 */

#define GIMP_TYPE_INT16_ARRAY               (gimp_int16_array_get_type ())
#define GIMP_VALUE_HOLDS_INT16_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_INT16_ARRAY))

GType   gimp_int16_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_INT16_ARRAY
 */

#define GIMP_TYPE_PARAM_INT16_ARRAY           (gimp_param_int16_array_get_type ())
#define GIMP_PARAM_SPEC_INT16_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_INT16_ARRAY, GimpParamSpecInt16Array))
#define GIMP_IS_PARAM_SPEC_INT16_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_INT16_ARRAY))

typedef struct _GimpParamSpecInt16Array GimpParamSpecInt16Array;

struct _GimpParamSpecInt16Array
{
  GimpParamSpecArray parent_instance;
};

GType          gimp_param_int16_array_get_type  (void) G_GNUC_CONST;

GParamSpec   * gimp_param_spec_int16_array      (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 GParamFlags   flags);

const gint16 * gimp_value_get_int16array        (const GValue *value);
gint16       * gimp_value_dup_int16array        (const GValue *value);
void           gimp_value_set_int16array        (GValue       *value,
                                                 const gint16 *array,
                                                 gsize         length);
void           gimp_value_set_static_int16array (GValue       *value,
                                                 const gint16 *array,
                                                 gsize         length);
void           gimp_value_take_int16array       (GValue       *value,
                                                 gint16       *array,
                                                 gsize         length);


/*
 * GIMP_TYPE_INT32_ARRAY
 */

#define GIMP_TYPE_INT32_ARRAY               (gimp_int32_array_get_type ())
#define GIMP_VALUE_HOLDS_INT32_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_INT32_ARRAY))

GType   gimp_int32_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_INT32_ARRAY
 */

#define GIMP_TYPE_PARAM_INT32_ARRAY           (gimp_param_int32_array_get_type ())
#define GIMP_PARAM_SPEC_INT32_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_INT32_ARRAY, GimpParamSpecInt32Array))
#define GIMP_IS_PARAM_SPEC_INT32_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_INT32_ARRAY))

typedef struct _GimpParamSpecInt32Array GimpParamSpecInt32Array;

struct _GimpParamSpecInt32Array
{
  GimpParamSpecArray parent_instance;
};

GType          gimp_param_int32_array_get_type  (void) G_GNUC_CONST;

GParamSpec   * gimp_param_spec_int32_array      (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 GParamFlags   flags);

const gint32 * gimp_value_get_int32array        (const GValue *value);
gint32       * gimp_value_dup_int32array        (const GValue *value);
void           gimp_value_set_int32array        (GValue       *value,
                                                 const gint32 *array,
                                                 gsize         length);
void           gimp_value_set_static_int32array (GValue       *value,
                                                 const gint32 *array,
                                                 gsize         length);
void           gimp_value_take_int32array       (GValue       *value,
                                                 gint32       *array,
                                                 gsize         length);


/*
 * GIMP_TYPE_FLOAT_ARRAY
 */

#define GIMP_TYPE_FLOAT_ARRAY               (gimp_float_array_get_type ())
#define GIMP_VALUE_HOLDS_FLOAT_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_FLOAT_ARRAY))

GType   gimp_float_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_FLOAT_ARRAY
 */

#define GIMP_TYPE_PARAM_FLOAT_ARRAY           (gimp_param_float_array_get_type ())
#define GIMP_PARAM_SPEC_FLOAT_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_FLOAT_ARRAY, GimpParamSpecFloatArray))
#define GIMP_IS_PARAM_SPEC_FLOAT_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_FLOAT_ARRAY))

typedef struct _GimpParamSpecFloatArray GimpParamSpecFloatArray;

struct _GimpParamSpecFloatArray
{
  GimpParamSpecArray parent_instance;
};

GType           gimp_param_float_array_get_type  (void) G_GNUC_CONST;

GParamSpec    * gimp_param_spec_float_array      (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  GParamFlags   flags);

const gdouble * gimp_value_get_floatarray        (const GValue  *value);
gdouble       * gimp_value_dup_floatarray        (const GValue  *value);
void            gimp_value_set_floatarray        (GValue        *value,
                                                  const gdouble *array,
                                                  gsize         length);
void            gimp_value_set_static_floatarray (GValue        *value,
                                                  const gdouble *array,
                                                  gsize         length);
void            gimp_value_take_floatarray       (GValue        *value,
                                                  gdouble       *array,
                                                  gsize         length);


/*
 * GIMP_TYPE_STRING_ARRAY
 */

GimpArray * gimp_string_array_new  (const gchar     **data,
                                    gsize             length,
                                    gboolean          static_data);
GimpArray * gimp_string_array_copy (const GimpArray  *array);
void        gimp_string_array_free (GimpArray        *array);

#define GIMP_TYPE_STRING_ARRAY               (gimp_string_array_get_type ())
#define GIMP_VALUE_HOLDS_STRING_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_STRING_ARRAY))

GType   gimp_string_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_STRING_ARRAY
 */

#define GIMP_TYPE_PARAM_STRING_ARRAY           (gimp_param_string_array_get_type ())
#define GIMP_PARAM_SPEC_STRING_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_STRING_ARRAY, GimpParamSpecStringArray))
#define GIMP_IS_PARAM_SPEC_STRING_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_STRING_ARRAY))

typedef struct _GimpParamSpecStringArray GimpParamSpecStringArray;

struct _GimpParamSpecStringArray
{
  GParamSpecBoxed parent_instance;
};

GType          gimp_param_string_array_get_type  (void) G_GNUC_CONST;

GParamSpec   * gimp_param_spec_string_array      (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  GParamFlags   flags);

const gchar ** gimp_value_get_stringarray        (const GValue *value);
gchar       ** gimp_value_dup_stringarray        (const GValue *value);
void           gimp_value_set_stringarray        (GValue       *value,
                                                  const gchar **array,
                                                  gsize         length);
void           gimp_value_set_static_stringarray (GValue       *value,
                                                  const gchar **array,
                                                  gsize         length);
void           gimp_value_take_stringarray       (GValue       *value,
                                                  gchar       **array,
                                                  gsize         length);


/*
 * GIMP_TYPE_COLOR_ARRAY
 */

#define GIMP_TYPE_COLOR_ARRAY               (gimp_color_array_get_type ())
#define GIMP_VALUE_HOLDS_COLOR_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_COLOR_ARRAY))

GType   gimp_color_array_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_COLOR_ARRAY
 */

#define GIMP_TYPE_PARAM_COLOR_ARRAY           (gimp_param_color_array_get_type ())
#define GIMP_PARAM_SPEC_COLOR_ARRAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_COLOR_ARRAY, GimpParamSpecColorArray))
#define GIMP_IS_PARAM_SPEC_COLOR_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_COLOR_ARRAY))

typedef struct _GimpParamSpecColorArray GimpParamSpecColorArray;

struct _GimpParamSpecColorArray
{
  GParamSpecBoxed parent_instance;
};

GType           gimp_param_color_array_get_type  (void) G_GNUC_CONST;

GParamSpec    * gimp_param_spec_color_array      (const gchar   *name,
                                                  const gchar   *nick,
                                                  const gchar   *blurb,
                                                  GParamFlags    flags);

const GimpRGB * gimp_value_get_colorarray        (const GValue  *value);
GimpRGB       * gimp_value_dup_colorarray        (const GValue  *value);
void            gimp_value_set_colorarray        (GValue        *value,
                                                  const GimpRGB *array,
                                                  gsize          length);
void            gimp_value_set_static_colorarray (GValue        *value,
                                                  const GimpRGB *array,
                                                  gsize          length);
void            gimp_value_take_colorarray       (GValue        *value,
                                                  GimpRGB       *array,
                                                  gsize          length);


#endif  /*  __GIMP_PARAM_SPECS_H__  */
