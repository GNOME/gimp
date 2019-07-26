/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpparamspecs.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __LIBGIMP_GIMP_PARAM_SPECS_H__
#define __LIBGIMP_GIMP_PARAM_SPECS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


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

  gboolean       none_ok;
};

GType        gimp_param_image_id_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_image_id     (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           gboolean      none_ok,
                                           GParamFlags   flags);

gint32       gimp_value_get_image_id      (const GValue *value);
void         gimp_value_set_image_id      (GValue       *value,
                                           gint32        image_id);


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

  gboolean       none_ok;
};

GType        gimp_param_item_id_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_item_id     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          gboolean      none_ok,
                                          GParamFlags   flags);

gint32       gimp_value_get_item_id      (const GValue *value);
void         gimp_value_set_item_id      (GValue       *value,
                                          gint32        item_id);


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
                                               gboolean      none_ok,
                                               GParamFlags   flags);

gint32         gimp_value_get_drawable_id     (const GValue *value);
void           gimp_value_set_drawable_id     (GValue       *value,
                                               gint32        drawable_id);


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
                                           gboolean      none_ok,
                                           GParamFlags   flags);

gint32       gimp_value_get_layer_id      (const GValue *value);
void         gimp_value_set_layer_id      (GValue       *value,
                                           gint32        layer_id);


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
                                              gboolean      none_ok,
                                              GParamFlags   flags);

gint32        gimp_value_get_channel_id      (const GValue *value);
void          gimp_value_set_channel_id      (GValue       *value,
                                              gint32        channel_id);


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
                                                   gboolean       none_ok,
                                                   GParamFlags    flags);

gint32          gimp_value_get_layer_mask_id      (const GValue  *value);
void            gimp_value_set_layer_mask_id      (GValue        *value,
                                                   gint32         layer_mask_id);


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
                                                  gboolean       none_ok,
                                                  GParamFlags    flags);

gint32          gimp_value_get_selection_id      (const GValue  *value);
void            gimp_value_set_selection_id      (GValue        *value,
                                                  gint32         selection_id);


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
                                              gboolean      none_ok,
                                              GParamFlags   flags);

gint32        gimp_value_get_vectors_id      (const GValue *value);
void          gimp_value_set_vectors_id      (GValue       *value,
                                              gint32        vectors_id);


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

  gboolean       none_ok;
};

GType        gimp_param_display_id_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_display_id     (const gchar  *name,
                                             const gchar  *nick,
                                             const gchar  *blurb,
                                             gboolean      none_ok,
                                             GParamFlags   flags);

gint32       gimp_value_get_display_id      (const GValue *value);
void         gimp_value_set_display_id      (GValue       *value,
                                             gint32        display_id);

G_END_DECLS

#endif  /*  __LIBGIMP_GIMP_PARAM_SPECS_H__  */
