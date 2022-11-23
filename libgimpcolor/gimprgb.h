/* LIBLIGMA - The LIGMA Library
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

#if !defined (__LIGMA_COLOR_H_INSIDE__) && !defined (LIGMA_COLOR_COMPILATION)
#error "Only <libligmacolor/ligmacolor.h> can be included directly."
#endif

#ifndef __LIGMA_RGB_H__
#define __LIGMA_RGB_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * LIGMA_TYPE_RGB
 */

#define LIGMA_TYPE_RGB               (ligma_rgb_get_type ())
#define LIGMA_VALUE_HOLDS_RGB(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_RGB))

GType   ligma_rgb_get_type           (void) G_GNUC_CONST;

void    ligma_value_get_rgb          (const GValue  *value,
                                     LigmaRGB       *rgb);
void    ligma_value_set_rgb          (GValue        *value,
                                     const LigmaRGB *rgb);


/*
 * LIGMA_TYPE_PARAM_RGB
 */

#define LIGMA_TYPE_PARAM_RGB           (ligma_param_rgb_get_type ())
#define LIGMA_IS_PARAM_SPEC_RGB(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_RGB))

typedef struct _LigmaParamSpecRGB LigmaParamSpecRGB;

GType        ligma_param_rgb_get_type         (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_rgb             (const gchar    *name,
                                              const gchar    *nick,
                                              const gchar    *blurb,
                                              gboolean        has_alpha,
                                              const LigmaRGB  *default_value,
                                              GParamFlags     flags);

void         ligma_param_spec_rgb_get_default (GParamSpec     *pspec,
                                              LigmaRGB        *default_value);
gboolean     ligma_param_spec_rgb_has_alpha   (GParamSpec     *pspec);


/*  RGB and RGBA color types and operations taken from LibGCK  */

/**
 * LigmaRGBCompositeMode:
 * @LIGMA_RGB_COMPOSITE_NONE: don't do compositing
 * @LIGMA_RGB_COMPOSITE_NORMAL: composite on top
 * @LIGMA_RGB_COMPOSITE_BEHIND: composite behind
 **/
typedef enum
{
  LIGMA_RGB_COMPOSITE_NONE = 0,
  LIGMA_RGB_COMPOSITE_NORMAL,
  LIGMA_RGB_COMPOSITE_BEHIND
} LigmaRGBCompositeMode;


void      ligma_rgb_set             (LigmaRGB       *rgb,
                                    gdouble        red,
                                    gdouble        green,
                                    gdouble        blue);
void      ligma_rgb_set_alpha       (LigmaRGB       *rgb,
                                    gdouble        alpha);

void      ligma_rgb_set_pixel       (LigmaRGB       *rgb,
                                    const Babl    *format,
                                    gconstpointer  pixel);
void      ligma_rgb_get_pixel       (const LigmaRGB *rgb,
                                    const Babl    *format,
                                    gpointer       pixel);

void      ligma_rgb_set_uchar       (LigmaRGB       *rgb,
                                    guchar         red,
                                    guchar         green,
                                    guchar         blue);
void      ligma_rgb_get_uchar       (const LigmaRGB *rgb,
                                    guchar        *red,
                                    guchar        *green,
                                    guchar        *blue);

gboolean  ligma_rgb_parse_name      (LigmaRGB       *rgb,
                                    const gchar   *name,
                                    gint           len);
gboolean  ligma_rgb_parse_hex       (LigmaRGB       *rgb,
                                    const gchar   *hex,
                                    gint           len);
gboolean  ligma_rgb_parse_css       (LigmaRGB       *rgb,
                                    const gchar   *css,
                                    gint           len);

void      ligma_rgb_add             (LigmaRGB       *rgb1,
                                    const LigmaRGB *rgb2);
void      ligma_rgb_subtract        (LigmaRGB       *rgb1,
                                    const LigmaRGB *rgb2);
void      ligma_rgb_multiply        (LigmaRGB       *rgb1,
                                    gdouble        factor);
gdouble   ligma_rgb_distance        (const LigmaRGB *rgb1,
                                    const LigmaRGB *rgb2);

gdouble   ligma_rgb_max             (const LigmaRGB *rgb);
gdouble   ligma_rgb_min             (const LigmaRGB *rgb);
void      ligma_rgb_clamp           (LigmaRGB       *rgb);

void      ligma_rgb_gamma           (LigmaRGB       *rgb,
                                    gdouble        gamma);

gdouble   ligma_rgb_luminance       (const LigmaRGB *rgb);
guchar    ligma_rgb_luminance_uchar (const LigmaRGB *rgb);

void      ligma_rgb_composite       (LigmaRGB              *color1,
                                    const LigmaRGB        *color2,
                                    LigmaRGBCompositeMode  mode);

/*  access to the list of color names  */
void      ligma_rgb_list_names      (const gchar ***names,
                                    LigmaRGB      **colors,
                                    gint          *n_colors);


void      ligma_rgba_set            (LigmaRGB       *rgba,
                                    gdouble        red,
                                    gdouble        green,
                                    gdouble        blue,
                                    gdouble        alpha);

void      ligma_rgba_set_pixel      (LigmaRGB       *rgba,
                                    const Babl    *format,
                                    gconstpointer  pixel);
void      ligma_rgba_get_pixel      (const LigmaRGB *rgba,
                                    const Babl    *format,
                                    gpointer       pixel);

void      ligma_rgba_set_uchar      (LigmaRGB       *rgba,
                                    guchar         red,
                                    guchar         green,
                                    guchar         blue,
                                    guchar         alpha);
void      ligma_rgba_get_uchar      (const LigmaRGB *rgba,
                                    guchar        *red,
                                    guchar        *green,
                                    guchar        *blue,
                                    guchar        *alpha);

gboolean  ligma_rgba_parse_css      (LigmaRGB       *rgba,
                                    const gchar   *css,
                                    gint           len);

void      ligma_rgba_add            (LigmaRGB       *rgba1,
                                    const LigmaRGB *rgba2);
void      ligma_rgba_subtract       (LigmaRGB       *rgba1,
                                    const LigmaRGB *rgba2);
void      ligma_rgba_multiply       (LigmaRGB       *rgba,
                                    gdouble        factor);

gdouble   ligma_rgba_distance       (const LigmaRGB *rgba1,
                                    const LigmaRGB *rgba2);



/*  Map D50-adapted sRGB to luminance  */

/*
 * The weights to compute true CIE luminance from linear red, green
 * and blue as defined by the sRGB color space specs in an ICC profile
 * color managed application. The weights below have been chromatically
 * adapted from D65 (as specified by the sRGB color space specs)
 * to D50 (as specified by D50 illuminant values in the ICC V4 specs).
 */

#define LIGMA_RGB_LUMINANCE_RED    (0.22248840)
#define LIGMA_RGB_LUMINANCE_GREEN  (0.71690369)
#define LIGMA_RGB_LUMINANCE_BLUE   (0.06060791)

#define LIGMA_RGB_LUMINANCE(r,g,b) ((r) * LIGMA_RGB_LUMINANCE_RED   + \
                                   (g) * LIGMA_RGB_LUMINANCE_GREEN + \
                                   (b) * LIGMA_RGB_LUMINANCE_BLUE)


G_END_DECLS

#endif  /* __LIGMA_RGB_H__ */
