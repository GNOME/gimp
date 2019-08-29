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
 * GIMP_TYPE_PARAM_IMAGE
 */

#define GIMP_VALUE_HOLDS_IMAGE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                         GIMP_TYPE_IMAGE))

#define GIMP_TYPE_PARAM_IMAGE           (gimp_param_image_get_type ())
#define GIMP_PARAM_SPEC_IMAGE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_IMAGE, GimpParamSpecImage))
#define GIMP_IS_PARAM_SPEC_IMAGE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_IMAGE))

typedef struct _GimpParamSpecImage GimpParamSpecImage;

struct _GimpParamSpecImage
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

GType        gimp_param_image_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_image     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        gboolean      none_ok,
                                        GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_ITEM
 */

#define GIMP_VALUE_HOLDS_ITEM(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                        GIMP_TYPE_ITEM))

#define GIMP_TYPE_PARAM_ITEM           (gimp_param_item_get_type ())
#define GIMP_PARAM_SPEC_ITEM(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_ITEM, GimpParamSpecItem))
#define GIMP_IS_PARAM_SPEC_ITEM(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_ITEM))

typedef struct _GimpParamSpecItem GimpParamSpecItem;

struct _GimpParamSpecItem
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

GType        gimp_param_item_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_item     (const gchar  *name,
                                       const gchar  *nick,
                                       const gchar  *blurb,
                                       gboolean      none_ok,
                                       GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_DRAWABLE
 */

#define GIMP_VALUE_HOLDS_DRAWABLE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                            GIMP_TYPE_DRAWABLE))

#define GIMP_TYPE_PARAM_DRAWABLE           (gimp_param_drawable_get_type ())
#define GIMP_PARAM_SPEC_DRAWABLE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_DRAWABLE, GimpParamSpecDrawable))
#define GIMP_IS_PARAM_SPEC_DRAWABLE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_DRAWABLE))

typedef struct _GimpParamSpecDrawable GimpParamSpecDrawable;

struct _GimpParamSpecDrawable
{
  GimpParamSpecItem parent_instance;
};

GType        gimp_param_drawable_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_drawable     (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           gboolean      none_ok,
                                           GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_LAYER
 */

#define GIMP_VALUE_HOLDS_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                         GIMP_TYPE_LAYER))

#define GIMP_TYPE_PARAM_LAYER           (gimp_param_layer_get_type ())
#define GIMP_PARAM_SPEC_LAYER(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_LAYER, GimpParamSpecLayer))
#define GIMP_IS_PARAM_SPEC_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_LAYER))

typedef struct _GimpParamSpecLayer GimpParamSpecLayer;

struct _GimpParamSpecLayer
{
  GimpParamSpecDrawable parent_instance;
};

GType        gimp_param_layer_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_layer     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        gboolean      none_ok,
                                        GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_CHANNEL
 */

#define GIMP_VALUE_HOLDS_CHANNEL(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           GIMP_TYPE_CHANNEL))

#define GIMP_TYPE_PARAM_CHANNEL           (gimp_param_channel_get_type ())
#define GIMP_PARAM_SPEC_CHANNEL(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_CHANNEL, GimpParamSpecChannel))
#define GIMP_IS_PARAM_SPEC_CHANNEL(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_CHANNEL))

typedef struct _GimpParamSpecChannel GimpParamSpecChannel;

struct _GimpParamSpecChannel
{
  GimpParamSpecDrawable parent_instance;
};

GType        gimp_param_channel_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_channel     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          gboolean      none_ok,
                                          GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_LAYER_MASK
 */

#define GIMP_VALUE_HOLDS_LAYER_MASK(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                              GIMP_TYPE_LAYER_MASK))

#define GIMP_TYPE_PARAM_LAYER_MASK           (gimp_param_layer_mask_get_type ())
#define GIMP_PARAM_SPEC_LAYER_MASK(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_LAYER_MASK, GimpParamSpecLayerMask))
#define GIMP_IS_PARAM_SPEC_LAYER_MASK(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_LAYER_MASK))

typedef struct _GimpParamSpecLayerMask GimpParamSpecLayerMask;

struct _GimpParamSpecLayerMask
{
  GimpParamSpecChannel parent_instance;
};

GType        gimp_param_layer_mask_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_layer_mask     (const gchar   *name,
                                             const gchar   *nick,
                                             const gchar   *blurb,
                                             gboolean       none_ok,
                                             GParamFlags    flags);


/*
 * GIMP_TYPE_PARAM_SELECTION
 */

#define GIMP_VALUE_HOLDS_SELECTION(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                             GIMP_TYPE_SELECTION))

#define GIMP_TYPE_PARAM_SELECTION           (gimp_param_selection_get_type ())
#define GIMP_PARAM_SPEC_SELECTION(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_SELECTION, GimpParamSpecSelection))
#define GIMP_IS_PARAM_SPEC_SELECTION(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_SELECTION))

typedef struct _GimpParamSpecSelection GimpParamSpecSelection;

struct _GimpParamSpecSelection
{
  GimpParamSpecChannel parent_instance;
};

GType        gimp_param_selection_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_selection     (const gchar   *name,
                                            const gchar   *nick,
                                            const gchar   *blurb,
                                            gboolean       none_ok,
                                            GParamFlags    flags);


/*
 * GIMP_TYPE_PARAM_VECTORS
 */

#define GIMP_VALUE_HOLDS_VECTORS(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           GIMP_TYPE_VECTORS))

#define GIMP_TYPE_PARAM_VECTORS           (gimp_param_vectors_get_type ())
#define GIMP_PARAM_SPEC_VECTORS(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_VECTORS, GimpParamSpecVectors))
#define GIMP_IS_PARAM_SPEC_VECTORS(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_VECTORS))

typedef struct _GimpParamSpecVectors GimpParamSpecVectors;

struct _GimpParamSpecVectors
{
  GimpParamSpecItem parent_instance;
};

GType        gimp_param_vectors_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_vectors     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          gboolean      none_ok,
                                          GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_DISPLAY
 */

#define GIMP_VALUE_HOLDS_DISPLAY(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           GIMP_TYPE_DISPLAY))

#define GIMP_TYPE_PARAM_DISPLAY           (gimp_param_display_get_type ())
#define GIMP_PARAM_SPEC_DISPLAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_DISPLAY, GimpParamSpecDisplay))
#define GIMP_IS_PARAM_SPEC_DISPLAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_DISPLAY))

typedef struct _GimpParamSpecDisplay GimpParamSpecDisplay;

struct _GimpParamSpecDisplay
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

GType        gimp_param_display_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_display     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          gboolean      none_ok,
                                          GParamFlags   flags);


G_END_DECLS

#endif  /*  __LIBGIMP_GIMP_PARAM_SPECS_H__  */
