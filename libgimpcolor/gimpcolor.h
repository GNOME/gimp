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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_COLOR_H__
#define __GIMP_COLOR_H__

#define __GIMP_COLOR_H_INSIDE__

#include <libgimpbase/gimpbase.h>

#include <libgimpcolor/gimpcolortypes.h>

#include <libgimpcolor/gimpadaptivesupersample.h>
#include <libgimpcolor/gimpbilinear.h>
#include <libgimpcolor/gimpcairo.h>
#include <libgimpcolor/gimpcolormanaged.h>
#include <libgimpcolor/gimpcolorprofile.h>
#include <libgimpcolor/gimpcolorspace.h>
#include <libgimpcolor/gimpcolortransform.h>
#include <libgimpcolor/gimphsl.h>
#include <libgimpcolor/gimppixbuf.h>
#include <libgimpcolor/gimprgb.h>

#undef __GIMP_COLOR_H_INSIDE__

G_BEGIN_DECLS

/*
 * GEGL_TYPE_COLOR
 */

#define GIMP_VALUE_HOLDS_COLOR(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_COLOR))


void           gimp_color_set_alpha                 (GeglColor      *color,
                                                     gdouble         alpha);

gboolean       gimp_color_is_perceptually_identical (GeglColor      *color1,
                                                     GeglColor      *color2);

GeglColor    * gimp_color_parse_css                 (const gchar    *css);
GeglColor    * gimp_color_parse_hex                 (const gchar    *hex);
GeglColor    * gimp_color_parse_name                (const gchar    *name);
const gchar ** gimp_color_list_names                (GimpColorArray *colors);

GeglColor    * gimp_color_parse_css_substring       (const gchar    *css,
                                                     gint            len);
GeglColor    * gimp_color_parse_hex_substring       (const gchar    *hex,
                                                     gint            len);
GeglColor    * gimp_color_parse_name_substring      (const gchar    *name,
                                                     gint            len);

gboolean       gimp_color_is_out_of_self_gamut      (GeglColor      *color);
gboolean       gimp_color_is_out_of_gamut           (GeglColor      *color,
                                                     const Babl     *space);

/*
 * GIMP_TYPE_PARAM_COLOR
 */

#define GIMP_TYPE_PARAM_COLOR           (gimp_param_color_get_type ())
#define GIMP_PARAM_SPEC_COLOR(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_COLOR, GimpParamSpecColor))
#define GIMP_IS_PARAM_SPEC_COLOR(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_COLOR))

GType        gimp_param_color_get_type         (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_color             (const gchar *name,
                                                const gchar *nick,
                                                const gchar *blurb,
                                                gboolean     has_alpha,
                                                GeglColor   *default_color,
                                                GParamFlags  flags);

GParamSpec * gimp_param_spec_color_from_string (const gchar *name,
                                                const gchar *nick,
                                                const gchar *blurb,
                                                gboolean     has_alpha,
                                                const gchar *default_color_string,
                                                GParamFlags  flags);

GeglColor  * gimp_param_spec_color_get_default (GParamSpec  *pspec);
gboolean     gimp_param_spec_color_has_alpha   (GParamSpec  *pspec);

G_END_DECLS

#endif  /* __GIMP_COLOR_H__ */
