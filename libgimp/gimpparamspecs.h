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
#define GIMP_IS_PARAM_SPEC_IMAGE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_IMAGE))

GType        gimp_param_image_get_type          (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_image              (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 gboolean      none_ok,
                                                 GParamFlags   flags);

gboolean     gimp_param_spec_image_none_allowed (GParamSpec   *pspec);


/*
 * GIMP_TYPE_PARAM_ITEM
 */

#define GIMP_VALUE_HOLDS_ITEM(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                        GIMP_TYPE_ITEM))

#define GIMP_TYPE_PARAM_ITEM           (gimp_param_item_get_type ())
#define GIMP_IS_PARAM_SPEC_ITEM(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_ITEM))

GType        gimp_param_item_get_type          (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_item              (const gchar  *name,
                                                const gchar  *nick,
                                                const gchar  *blurb,
                                                gboolean      none_ok,
                                                GParamFlags   flags);

gboolean     gimp_param_spec_item_none_allowed (GParamSpec   *pspec);


/*
 * GIMP_TYPE_PARAM_DRAWABLE
 */

#define GIMP_VALUE_HOLDS_DRAWABLE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                            GIMP_TYPE_DRAWABLE))

#define GIMP_TYPE_PARAM_DRAWABLE           (gimp_param_drawable_get_type ())
#define GIMP_IS_PARAM_SPEC_DRAWABLE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_DRAWABLE))

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
#define GIMP_IS_PARAM_SPEC_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_LAYER))

GType        gimp_param_layer_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_layer     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        gboolean      none_ok,
                                        GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_TEXT_LAYER
 */

#define GIMP_VALUE_HOLDS_TEXT_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                              GIMP_TYPE_TEXT_LAYER))

#define GIMP_TYPE_PARAM_TEXT_LAYER           (gimp_param_text_layer_get_type ())
#define GIMP_IS_PARAM_SPEC_TEXT_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_TEXT_LAYER))

GType        gimp_param_text_layer_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_text_layer     (const gchar  *name,
                                             const gchar  *nick,
                                             const gchar  *blurb,
                                             gboolean      none_ok,
                                             GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_VECTOR_LAYER
 */

#define GIMP_VALUE_HOLDS_VECTOR_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                                GIMP_TYPE_VECTOR_LAYER))

#define GIMP_TYPE_PARAM_VECTOR_LAYER           (gimp_param_vector_layer_get_type ())
#define GIMP_IS_PARAM_SPEC_VECTOR_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_VECTOR_LAYER))

GType        gimp_param_vector_layer_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_vector_layer     (const gchar  *name,
                                               const gchar  *nick,
                                               const gchar  *blurb,
                                               gboolean      none_ok,
                                               GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_LINK_LAYER
 */

#define GIMP_VALUE_HOLDS_LINK_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_LINK_LAYER))
#define GIMP_TYPE_PARAM_LINK_LAYER           (gimp_param_link_layer_get_type ())
#define GIMP_IS_PARAM_SPEC_LINK_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_LINK_LAYER))

GType        gimp_param_link_layer_get_type   (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_link_layer       (const gchar  *name,
                                               const gchar  *nick,
                                               const gchar  *blurb,
                                               gboolean      none_ok,
                                               GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_RASTERIZABLE
 */

#define GIMP_VALUE_HOLDS_RASTERIZABLE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_RASTERIZABLE))
#define GIMP_TYPE_PARAM_RASTERIZABLE           (gimp_param_rasterizable_get_type ())
#define GIMP_IS_PARAM_SPEC_RASTERIZABLE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_RASTERIZABLE))

GType        gimp_param_rasterizable_get_type   (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_rasterizable       (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 gboolean      none_ok,
                                                 GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_GROUP_LAYER
 */

#define GIMP_VALUE_HOLDS_GROUP_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                               GIMP_TYPE_GROUP_LAYER))

#define GIMP_TYPE_PARAM_GROUP_LAYER           (gimp_param_group_layer_get_type ())
#define GIMP_IS_PARAM_SPEC_GROUP_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_GROUP_LAYER))

GType        gimp_param_group_layer_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_group_layer     (const gchar  *name,
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
#define GIMP_IS_PARAM_SPEC_CHANNEL(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_CHANNEL))

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
#define GIMP_IS_PARAM_SPEC_LAYER_MASK(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_LAYER_MASK))

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
#define GIMP_IS_PARAM_SPEC_SELECTION(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_SELECTION))

GType        gimp_param_selection_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_selection     (const gchar   *name,
                                            const gchar   *nick,
                                            const gchar   *blurb,
                                            gboolean       none_ok,
                                            GParamFlags    flags);


/*
 * GIMP_TYPE_PARAM_PATH
 */

#define GIMP_VALUE_HOLDS_PATH(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                        GIMP_TYPE_PATH))

#define GIMP_TYPE_PARAM_PATH           (gimp_param_path_get_type ())
#define GIMP_IS_PARAM_SPEC_PATH(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_PATH))

GType        gimp_param_path_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_path     (const gchar  *name,
                                       const gchar  *nick,
                                       const gchar  *blurb,
                                       gboolean      none_ok,
                                       GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_DRAWABLE_FILTER
 */

#define GIMP_VALUE_HOLDS_DRAWABLE_FILTER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                                                            GIMP_TYPE_DRAWABLE_FILTER))

#define GIMP_TYPE_PARAM_DRAWABLE_FILTER           (gimp_param_drawable_filter_get_type ())
#define GIMP_IS_PARAM_SPEC_DRAWABLE_FILTER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_DRAWABLE_FILTER))

GType        gimp_param_drawable_filter_get_type          (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_drawable_filter              (const gchar  *name,
                                                           const gchar  *nick,
                                                           const gchar  *blurb,
                                                           gboolean      none_ok,
                                                           GParamFlags   flags);

gboolean      gimp_param_spec_drawable_filter_none_allowed (GParamSpec *pspec);


/*
 * GIMP_TYPE_PARAM_DISPLAY
 */

#define GIMP_VALUE_HOLDS_DISPLAY(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           GIMP_TYPE_DISPLAY))

#define GIMP_TYPE_PARAM_DISPLAY           (gimp_param_display_get_type ())
#define GIMP_IS_PARAM_SPEC_DISPLAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_DISPLAY))

GType        gimp_param_display_get_type         (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_display             (const gchar  *name,
                                                  const gchar  *nick,
                                                  const gchar  *blurb,
                                                  gboolean      none_ok,
                                                  GParamFlags   flags);

gboolean     gimp_param_spec_display_none_allowed (GParamSpec *pspec);


/*
 * GIMP_TYPE_PARAM_RESOURCE
 */

#define GIMP_VALUE_HOLDS_RESOURCE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_RESOURCE))

#define GIMP_TYPE_PARAM_RESOURCE           (gimp_param_resource_get_type ())
#define GIMP_IS_PARAM_SPEC_RESOURCE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_RESOURCE))

GType        gimp_param_resource_get_type                 (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_resource                     (const gchar  *name,
                                                           const gchar  *nick,
                                                           const gchar  *blurb,
                                                           GType         resource_type,
                                                           gboolean      none_ok,
                                                           GimpResource *default_value,
                                                           gboolean      default_to_context,
                                                           GParamFlags   flags);

gboolean     gimp_param_spec_resource_none_allowed        (GParamSpec   *pspec);
gboolean     gimp_param_spec_resource_defaults_to_context (GParamSpec   *pspec);


/*
 * GIMP_TYPE_PARAM_BRUSH
 */

#define GIMP_VALUE_HOLDS_BRUSH(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                         GIMP_TYPE_BRUSH))

#define GIMP_TYPE_PARAM_BRUSH           (gimp_param_brush_get_type ())
#define GIMP_IS_PARAM_SPEC_BRUSH(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_BRUSH))

GType        gimp_param_brush_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_brush (const gchar  *name,
                                    const gchar  *nick,
                                    const gchar  *blurb,
                                    gboolean      none_ok,
                                    GimpBrush    *default_value,
                                    gboolean      default_to_context,
                                    GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_PATTERN
 */

#define GIMP_VALUE_HOLDS_PATTERN(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                           GIMP_TYPE_PATTERN))

#define GIMP_TYPE_PARAM_PATTERN           (gimp_param_pattern_get_type ())
#define GIMP_IS_PARAM_SPEC_PATTERN(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_PATTERN))

GType        gimp_param_pattern_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_pattern (const gchar  *name,
                                      const gchar  *nick,
                                      const gchar  *blurb,
                                      gboolean      none_ok,
                                      GimpPattern  *default_value,
                                      gboolean      default_to_context,
                                      GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_GRADIENT
 */

#define GIMP_VALUE_HOLDS_GRADIENT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                            GIMP_TYPE_GRADIENT))

#define GIMP_TYPE_PARAM_GRADIENT           (gimp_param_gradient_get_type ())
#define GIMP_IS_PARAM_SPEC_GRADIENT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_GRADIENT))

GType        gimp_param_gradient_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_gradient (const gchar  *name,
                                       const gchar  *nick,
                                       const gchar  *blurb,
                                       gboolean      none_ok,
                                       GimpGradient *default_value,
                                       gboolean      default_to_context,
                                       GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_PALETTE
 */

#define GIMP_VALUE_HOLDS_PALETTE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                           GIMP_TYPE_PALETTE))

#define GIMP_TYPE_PARAM_PALETTE           (gimp_param_palette_get_type ())
#define GIMP_IS_PARAM_SPEC_PALETTE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_PALETTE))

GType        gimp_param_palette_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_palette (const gchar  *name,
                                      const gchar  *nick,
                                      const gchar  *blurb,
                                      gboolean      none_ok,
                                      GimpPalette  *default_value,
                                      gboolean      default_to_context,
                                      GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_FONT
 */

#define GIMP_VALUE_HOLDS_FONT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                        GIMP_TYPE_FONT))

#define GIMP_TYPE_PARAM_FONT           (gimp_param_font_get_type ())
#define GIMP_IS_PARAM_SPEC_FONT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_FONT))

GType        gimp_param_font_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_font (const gchar  *name,
                                   const gchar  *nick,
                                   const gchar  *blurb,
                                   gboolean      none_ok,
                                   GimpFont     *default_value,
                                   gboolean      default_to_context,
                                   GParamFlags   flags);


G_END_DECLS

#endif  /*  __LIBGIMP_GIMP_PARAM_SPECS_H__  */
