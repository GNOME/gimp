
/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#ifndef __LIBGIMP_GIMP_PARAM_SPECS_RESOURCE_H__
#define __LIBGIMP_GIMP_PARAM_SPECS_RESOURCE_H__

G_BEGIN_DECLS

/*
 * GIMP_TYPE_PARAM_RESOURCE
 */

/* See bottom of this file for definition of GIMP_VALUE_HOLDS_RESOURCE(value) */

#define GIMP_TYPE_PARAM_RESOURCE           (gimp_param_resource_get_type ())
#define GIMP_PARAM_SPEC_RESOURCE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_RESOURCE, GimpParamSpecResource))
#define GIMP_IS_PARAM_SPEC_RESOURCE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_RESOURCE))

typedef struct _GimpParamSpecResource GimpParamSpecResource;

struct _GimpParamSpecResource
{
  GParamSpecObject  parent_instance;
  gboolean          none_ok;
};

GType        gimp_param_resource_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_resource (
    const gchar  *name,
    const gchar  *nick,
    const gchar  *blurb,
    gboolean      none_ok,
    GParamFlags   flags);



/*
 * GIMP_TYPE_PARAM_BRUSH
 */

#define GIMP_VALUE_HOLDS_BRUSH(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_BRUSH))
#define GIMP_TYPE_PARAM_BRUSH           (gimp_param_brush_get_type ())
#define GIMP_PARAM_SPEC_BRUSH(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_BRUSH, GimpParamSpecBrush))
#define GIMP_IS_PARAM_SPEC_BRUSH(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_BRUSH))

typedef struct _GimpParamSpecBrush GimpParamSpecBrush;

struct _GimpParamSpecBrush
{
  GParamSpecObject  parent_instance;
  gboolean          none_ok;
};

GType        gimp_param_brush_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_brush (const gchar  *name,
                                    const gchar  *nick,
                                    const gchar  *blurb,
                                    gboolean      none_ok,
                                    GParamFlags   flags);



/*
 * GIMP_TYPE_PARAM_FONT
 */

#define GIMP_VALUE_HOLDS_FONT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_FONT))
#define GIMP_TYPE_PARAM_FONT           (gimp_param_font_get_type ())
#define GIMP_PARAM_SPEC_FONT(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_FONT, GimpParamSpecFont))
#define GIMP_IS_PARAM_SPEC_FONT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_FONT))

typedef struct _GimpParamSpecFont GimpParamSpecFont;

struct _GimpParamSpecFont
{
  GParamSpecObject  parent_instance;
  gboolean          none_ok;
};

GType        gimp_param_font_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_font (const gchar  *name,
                                   const gchar  *nick,
                                   const gchar  *blurb,
                                   gboolean      none_ok,
                                   GParamFlags   flags);



/*
 * GIMP_TYPE_PARAM_GRADIENT
 */

#define GIMP_VALUE_HOLDS_GRADIENT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_GRADIENT))
#define GIMP_TYPE_PARAM_GRADIENT           (gimp_param_gradient_get_type ())
#define GIMP_PARAM_SPEC_GRADIENT(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_GRADIENT, GimpParamSpecGradient))
#define GIMP_IS_PARAM_SPEC_GRADIENT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_GRADIENT))

typedef struct _GimpParamSpecGradient GimpParamSpecGradient;

struct _GimpParamSpecGradient
{
  GParamSpecObject  parent_instance;
  gboolean          none_ok;
};

GType        gimp_param_gradient_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_gradient (const gchar  *name,
                                       const gchar  *nick,
                                       const gchar  *blurb,
                                       gboolean      none_ok,
                                       GParamFlags   flags);



/*
 * GIMP_TYPE_PARAM_PALETTE
 */

#define GIMP_VALUE_HOLDS_PALETTE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_PALETTE))
#define GIMP_TYPE_PARAM_PALETTE           (gimp_param_palette_get_type ())
#define GIMP_PARAM_SPEC_PALETTE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_PALETTE, GimpParamSpecPalette))
#define GIMP_IS_PARAM_SPEC_PALETTE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_PALETTE))

typedef struct _GimpParamSpecPalette GimpParamSpecPalette;

struct _GimpParamSpecPalette
{
  GParamSpecObject  parent_instance;
  gboolean          none_ok;
};

GType        gimp_param_palette_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_palette (const gchar  *name,
                                      const gchar  *nick,
                                      const gchar  *blurb,
                                      gboolean      none_ok,
                                      GParamFlags   flags);



/*
 * GIMP_TYPE_PARAM_PATTERN
 */

#define GIMP_VALUE_HOLDS_PATTERN(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_PATTERN))
#define GIMP_TYPE_PARAM_PATTERN           (gimp_param_pattern_get_type ())
#define GIMP_PARAM_SPEC_PATTERN(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_PATTERN, GimpParamSpecPattern))
#define GIMP_IS_PARAM_SPEC_PATTERN(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_PATTERN))

typedef struct _GimpParamSpecPattern GimpParamSpecPattern;

struct _GimpParamSpecPattern
{
  GParamSpecObject  parent_instance;
  gboolean          none_ok;
};

GType        gimp_param_pattern_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_pattern (const gchar  *name,
                                      const gchar  *nick,
                                      const gchar  *blurb,
                                      gboolean      none_ok,
                                      GParamFlags   flags);




#define GIMP_VALUE_HOLDS_RESOURCE(value)  (GIMP_VALUE_HOLDS_BRUSH (value)    || \
                                           GIMP_VALUE_HOLDS_FONT (value)     || \
                                           GIMP_VALUE_HOLDS_GRADIENT (value) || \
                                           GIMP_VALUE_HOLDS_PALETTE (value)  || \
                                           GIMP_VALUE_HOLDS_PATTERN (value) )



G_END_DECLS

#endif  /*  __LIBGIMP_GIMP_PARAM_SPECS_RESOURCE_H__  */
