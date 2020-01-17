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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp-filter-history.h"
#include "core/gimpimage.h"
#include "core/gimplayermask.h"

#include "pdb/gimpprocedure.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpuimanager.h"

#include "actions.h"
#include "filters-actions.h"
#include "filters-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   filters_actions_set_tooltips    (GimpActionGroup             *group,
                                               const GimpStringActionEntry *entries,
                                               gint                         n_entries);
static void   filters_actions_history_changed (Gimp                        *gimp,
                                               GimpActionGroup             *group);


/*  private variables  */

static const GimpActionEntry filters_menu_actions[] =
{
  { "filters-menu",                 NULL, NC_("filters-action",
                                              "Filte_rs")          },
  { "filters-recent-menu",          NULL, NC_("filters-action",
                                              "Recently _Used")     },
  { "filters-blur-menu",            NULL, NC_("filters-action",
                                              "_Blur")             },
  { "filters-noise-menu",           NULL, NC_("filters-action",
                                              "_Noise")            },
  { "filters-edge-detect-menu",     NULL, NC_("filters-action",
                                              "Edge-De_tect")      },
  { "filters-enhance-menu",         NULL, NC_("filters-action",
                                              "En_hance")          },
  { "filters-combine-menu",         NULL, NC_("filters-action",
                                              "C_ombine")          },
  { "filters-generic-menu",         NULL, NC_("filters-action",
                                              "_Generic")          },
  { "filters-light-shadow-menu",    NULL, NC_("filters-action",
                                              "_Light and Shadow") },
  { "filters-distorts-menu",        NULL, NC_("filters-action",
                                              "_Distorts")         },
  { "filters-artistic-menu",        NULL, NC_("filters-action",
                                              "_Artistic")         },
  { "filters-decor-menu",           NULL, NC_("filters-action",
                                              "_Decor")            },
  { "filters-map-menu",             NULL, NC_("filters-action",
                                              "_Map")              },
  { "filters-render-menu",          NULL, NC_("filters-action",
                                              "_Render")           },
  { "filters-render-clouds-menu",   NULL, NC_("filters-action",
                                              "_Clouds")           },
  { "filters-render-fractals-menu", NULL, NC_("filters-action",
                                              "_Fractals")         },
  { "filters-render-nature-menu",   NULL, NC_("filters-action",
                                              "_Nature")           },
  { "filters-render-noise-menu",    NULL, NC_("filters-action",
                                              "N_oise")            },
  { "filters-render-pattern-menu",  NULL, NC_("filters-action",
                                              "_Pattern")          },
  { "filters-web-menu",             NULL, NC_("filters-action",
                                              "_Web")              },
  { "filters-animation-menu",       NULL, NC_("filters-action",
                                              "An_imation")        }
};

static const GimpStringActionEntry filters_actions[] =
{
  { "filters-antialias", GIMP_ICON_GEGL,
    NC_("filters-action", "_Antialias"), NULL, NULL,
    "gegl:antialias",
    GIMP_HELP_FILTER_ANTIALIAS },

  { "filters-color-enhance", GIMP_ICON_GEGL,
    NC_("filters-action", "_Color Enhance"), NULL, NULL,
    "gegl:color-enhance",
    GIMP_HELP_FILTER_COLOR_ENHANCE },

  { "filters-invert-linear", GIMP_ICON_INVERT,
    NC_("filters-action", "L_inear Invert"), NULL, NULL,
    "gegl:invert-linear",
    GIMP_HELP_FILTER_INVERT_LINEAR },

  { "filters-invert-perceptual", GIMP_ICON_INVERT,
    NC_("filters-action", "In_vert"), NULL, NULL,
    "gegl:invert-gamma",
    GIMP_HELP_FILTER_INVERT_PERCEPTUAL },

  { "filters-invert-value", GIMP_ICON_INVERT,
    NC_("filters-action", "_Value Invert"), NULL, NULL,
    "gegl:value-invert",
    GIMP_HELP_FILTER_INVERT_VALUE },

  { "filters-stretch-contrast-hsv", GIMP_ICON_GEGL,
    NC_("filters-action", "_Stretch Contrast HSV"), NULL, NULL,
    "gegl:stretch-contrast-hsv",
    GIMP_HELP_FILTER_STRETCH_CONTRAST_HSV }
};

static const GimpStringActionEntry filters_settings_actions[] =
{
  { "filters-dilate", GIMP_ICON_GEGL,
    NC_("filters-action", "_Dilate"), NULL,
    NC_("filters-action", "Grow lighter areas of the image"),
    "gegl:value-propagate\n"
    "(mode white)"
    "(lower-threshold 0.000000)"
    "(upper-threshold 1.000000)"
    "(rate 1.000000)"
    "(top yes)"
    "(left yes)"
    "(right yes)"
    "(bottom yes)"
    "(value yes)"
    "(alpha no)",
    GIMP_HELP_FILTER_DILATE },

  { "filters-erode", GIMP_ICON_GEGL,
    NC_("filters-action", "_Erode"), NULL,
    NC_("filters-action", "Grow darker areas of the image"),
    "gegl:value-propagate\n"
    "(mode black)"
    "(lower-threshold 0.000000)"
    "(upper-threshold 1.000000)"
    "(rate 1.000000)"
    "(top yes)"
    "(left yes)"
    "(right yes)"
    "(bottom yes)"
    "(value yes)"
    "(alpha no)",
    GIMP_HELP_FILTER_ERODE }
};

static const GimpStringActionEntry filters_interactive_actions[] =
{
  { "filters-alien-map", GIMP_ICON_GEGL,
    NC_("filters-action", "_Alien Map..."), NULL, NULL,
    "gegl:alien-map",
    GIMP_HELP_FILTER_ALIEN_MAP },

  { "filters-apply-canvas", GIMP_ICON_GEGL,
    NC_("filters-action", "_Apply Canvas..."), NULL, NULL,
    "gegl:texturize-canvas",
    GIMP_HELP_FILTER_APPLY_CANVAS },

  { "filters-apply-lens", GIMP_ICON_GEGL,
    NC_("filters-action", "Apply _Lens..."), NULL, NULL,
    "gegl:apply-lens",
    GIMP_HELP_FILTER_APPLY_LENS },

  { "filters-bayer-matrix", GIMP_ICON_GEGL,
    NC_("filters-action", "_Bayer Matrix..."), NULL, NULL,
    "gegl:bayer-matrix",
    GIMP_HELP_FILTER_BAYER_MATRIX },

  { "filters-brightness-contrast", GIMP_ICON_TOOL_BRIGHTNESS_CONTRAST,
    NC_("filters-action", "B_rightness-Contrast..."), NULL, NULL,
    "gimp:brightness-contrast",
    GIMP_HELP_TOOL_BRIGHTNESS_CONTRAST },

  { "filters-bump-map", GIMP_ICON_GEGL,
    NC_("filters-action", "_Bump Map..."), NULL, NULL,
    "gegl:bump-map",
    GIMP_HELP_FILTER_BUMP_MAP },

  { "filters-c2g", GIMP_ICON_GEGL,
    NC_("filters-action", "_Color to Gray..."), NULL, NULL,
    "gegl:c2g",
    GIMP_HELP_FILTER_C2G },

  { "filters-cartoon", GIMP_ICON_GEGL,
    NC_("filters-action", "Ca_rtoon..."), NULL, NULL,
    "gegl:cartoon",
    GIMP_HELP_FILTER_CARTOON },

  { "filters-channel-mixer", GIMP_ICON_GEGL,
    NC_("filters-action", "_Channel Mixer..."), NULL, NULL,
    "gegl:channel-mixer",
    GIMP_HELP_FILTER_CHANNEL_MIXER },

  { "filters-checkerboard", GIMP_ICON_GEGL,
    NC_("filters-action", "_Checkerboard..."), NULL, NULL,
    "gegl:checkerboard",
    GIMP_HELP_FILTER_CHECKERBOARD },

  { "filters-color-balance", GIMP_ICON_TOOL_COLOR_BALANCE,
    NC_("filters-action", "Color _Balance..."), NULL, NULL,
    "gimp:color-balance",
    GIMP_HELP_TOOL_COLOR_BALANCE },

  { "filters-color-exchange", GIMP_ICON_GEGL,
    NC_("filters-action", "_Color Exchange..."), NULL, NULL,
    "gegl:color-exchange",
    GIMP_HELP_FILTER_COLOR_EXCHANGE },

  { "filters-colorize", GIMP_ICON_TOOL_COLORIZE,
    NC_("filters-action", "Colori_ze..."), NULL, NULL,
    "gimp:colorize",
    GIMP_HELP_TOOL_COLORIZE },

  { "filters-dither", GIMP_ICON_GEGL,
    NC_("filters-action", "Dithe_r..."), NULL, NULL,
    "gegl:dither",
    GIMP_HELP_FILTER_DITHER },

  { "filters-color-rotate", GIMP_ICON_GEGL,
    NC_("filters-action", "_Rotate Colors..."), NULL, NULL,
    "gegl:color-rotate",
    GIMP_HELP_FILTER_COLOR_ROTATE },

  { "filters-color-temperature", GIMP_ICON_TOOL_COLOR_TEMPERATURE,
    NC_("filters-action", "Color T_emperature..."), NULL, NULL,
    "gegl:color-temperature",
    GIMP_HELP_FILTER_COLOR_TEMPERATURE },

  { "filters-color-to-alpha", GIMP_ICON_GEGL,
    NC_("filters-action", "Color to _Alpha..."), NULL, NULL,
    "gegl:color-to-alpha",
    GIMP_HELP_FILTER_COLOR_TO_ALPHA },

  { "filters-component-extract", GIMP_ICON_GEGL,
    NC_("filters-action", "_Extract Component..."), NULL, NULL,
    "gegl:component-extract",
    GIMP_HELP_FILTER_COMPONENT_EXTRACT },

  { "filters-convolution-matrix", GIMP_ICON_GEGL,
    NC_("filters-action", "_Convolution Matrix..."), NULL, NULL,
    "gegl:convolution-matrix",
    GIMP_HELP_FILTER_CONVOLUTION_MATRIX },

  { "filters-cubism", GIMP_ICON_GEGL,
    NC_("filters-action", "_Cubism..."), NULL, NULL,
    "gegl:cubism",
    GIMP_HELP_FILTER_CUBISM },

  { "filters-curves", GIMP_ICON_TOOL_CURVES,
    NC_("filters-action", "_Curves..."), NULL, NULL,
    "gimp:curves",
    GIMP_HELP_TOOL_CURVES },

  { "filters-deinterlace", GIMP_ICON_GEGL,
    NC_("filters-action", "_Deinterlace..."), NULL, NULL,
    "gegl:deinterlace",
    GIMP_HELP_FILTER_DEINTERLACE },

  { "filters-desaturate", GIMP_ICON_TOOL_DESATURATE,
    NC_("filters-action", "_Desaturate..."), NULL, NULL,
    "gimp:desaturate",
    GIMP_HELP_FILTER_DESATURATE },

  { "filters-difference-of-gaussians", GIMP_ICON_GEGL,
    NC_("filters-action", "Difference of _Gaussians..."), NULL, NULL,
    "gegl:difference-of-gaussians",
    GIMP_HELP_FILTER_DIFFERENCE_OF_GAUSSIANS },

  { "filters-diffraction-patterns", GIMP_ICON_GEGL,
    NC_("filters-action", "D_iffraction Patterns..."), NULL, NULL,
    "gegl:diffraction-patterns",
    GIMP_HELP_FILTER_DIFFRACTION_PATTERNS },

  { "filters-displace", GIMP_ICON_GEGL,
    NC_("filters-action", "_Displace..."), NULL, NULL,
    "gegl:displace",
    GIMP_HELP_FILTER_DISPLACE },

  { "filters-distance-map", GIMP_ICON_GEGL,
    NC_("filters-action", "Distance _Map..."), NULL, NULL,
    "gegl:distance-transform",
    GIMP_HELP_FILTER_DISTANCE_MAP },

  { "filters-dropshadow", GIMP_ICON_GEGL,
    NC_("filters-action", "_Drop Shadow..."), NULL, NULL,
    "gegl:dropshadow",
    GIMP_HELP_FILTER_DROPSHADOW },

  { "filters-edge", GIMP_ICON_GEGL,
    NC_("filters-action", "_Edge..."), NULL, NULL,
    "gegl:edge",
    GIMP_HELP_FILTER_EDGE },

  { "filters-edge-laplace", GIMP_ICON_GEGL,
    NC_("filters-action", "_Laplace"), NULL, NULL,
    "gegl:edge-laplace",
    GIMP_HELP_FILTER_EDGE_LAPLACE },

  { "filters-edge-neon", GIMP_ICON_GEGL,
    NC_("filters-action", "_Neon..."), NULL, NULL,
    "gegl:edge-neon",
    GIMP_HELP_FILTER_EDGE_NEON },

  { "filters-edge-sobel", GIMP_ICON_GEGL,
    NC_("filters-action", "_Sobel..."), NULL, NULL,
    "gegl:edge-sobel",
    GIMP_HELP_FILTER_EDGE_SOBEL },

  { "filters-emboss", GIMP_ICON_GEGL,
    NC_("filters-action", "_Emboss..."), NULL, NULL,
    "gegl:emboss",
    GIMP_HELP_FILTER_EMBOSS },

  { "filters-engrave", GIMP_ICON_GEGL,
    NC_("filters-action", "En_grave..."), NULL, NULL,
    "gegl:engrave",
    GIMP_HELP_FILTER_ENGRAVE },

  { "filters-exposure", GIMP_ICON_TOOL_EXPOSURE,
    NC_("filters-action", "E_xposure..."), NULL, NULL,
    "gegl:exposure",
    GIMP_HELP_FILTER_EXPOSURE },

  { "filters-fattal-2002", GIMP_ICON_GEGL,
    NC_("filters-action", "_Fattal et al. 2002..."), NULL, NULL,
    "gegl:fattal02",
    GIMP_HELP_FILTER_FATTAL_2002 },

  { "filters-fractal-trace", GIMP_ICON_GEGL,
    NC_("filters-action", "_Fractal Trace..."), NULL, NULL,
    "gegl:fractal-trace",
    GIMP_HELP_FILTER_FRACTAL_TRACE },

  { "filters-gaussian-blur", GIMP_ICON_GEGL,
    NC_("filters-action", "_Gaussian Blur..."), NULL, NULL,
    "gegl:gaussian-blur",
    GIMP_HELP_FILTER_GAUSSIAN_BLUR },

  { "filters-gaussian-blur-selective", GIMP_ICON_GEGL,
    NC_("filters-action", "_Selective Gaussian Blur..."), NULL, NULL,
    "gegl:gaussian-blur-selective",
    GIMP_HELP_FILTER_GAUSSIAN_BLUR_SELECTIVE },

  { "filters-gegl-graph", GIMP_ICON_GEGL,
    NC_("filters-action", "_GEGL graph..."), NULL, NULL,
    "gegl:gegl",
    GIMP_HELP_FILTER_GEGL_GRAPH },

  { "filters-grid", GIMP_ICON_GRID,
    NC_("filters-action", "_Grid..."), NULL, NULL,
    "gegl:grid",
    GIMP_HELP_FILTER_GRID },

  { "filters-high-pass", GIMP_ICON_GEGL,
    NC_("filters-action", "_High Pass..."), NULL, NULL,
    "gegl:high-pass",
    GIMP_HELP_FILTER_HIGH_PASS },

  { "filters-hue-chroma", GIMP_ICON_GEGL,
    NC_("filters-action", "Hue-_Chroma..."), NULL, NULL,
    "gegl:hue-chroma",
    GIMP_HELP_FILTER_HUE_CHROMA },

  { "filters-hue-saturation", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("filters-action", "Hue-_Saturation..."), NULL, NULL,
    "gimp:hue-saturation",
    GIMP_HELP_TOOL_HUE_SATURATION },

  { "filters-illusion", GIMP_ICON_GEGL,
    NC_("filters-action", "_Illusion..."), NULL, NULL,
    "gegl:illusion",
    GIMP_HELP_FILTER_ILLUSION },

  { "filters-image-gradient", GIMP_ICON_GEGL,
    NC_("filters-action", "_Image Gradient..."), NULL, NULL,
    "gegl:image-gradient",
    GIMP_HELP_FILTER_IMAGE_GRADIENT },

  { "filters-kaleidoscope", GIMP_ICON_GEGL,
    NC_("filters-action", "_Kaleidoscope..."), NULL, NULL,
    "gegl:mirrors",
    GIMP_HELP_FILTER_KALEIDOSCOPE },

  { "filters-lens-distortion", GIMP_ICON_GEGL,
    NC_("filters-action", "Le_ns Distortion..."), NULL, NULL,
    "gegl:lens-distortion",
    GIMP_HELP_FILTER_LENS_DISTORTION },

  { "filters-lens-flare", GIMP_ICON_GEGL,
    NC_("filters-action", "Lens _Flare..."), NULL, NULL,
    "gegl:lens-flare",
    GIMP_HELP_FILTER_LENS_FLARE },

  { "filters-levels", GIMP_ICON_TOOL_LEVELS,
    NC_("filters-action", "_Levels..."), NULL, NULL,
    "gimp:levels",
    GIMP_HELP_TOOL_LEVELS },

  { "filters-linear-sinusoid", GIMP_ICON_TOOL_LEVELS,
    NC_("filters-action", "_Linear Sinusoid..."), NULL, NULL,
    "gegl:linear-sinusoid",
    GIMP_HELP_FILTER_LINEAR_SINUSOID },

  { "filters-little-planet", GIMP_ICON_GEGL,
    NC_("filters-action", "_Little Planet..."), NULL, NULL,
    "gegl:stereographic-projection",
    GIMP_HELP_FILTER_LITTLE_PLANET },

  { "filters-long-shadow", GIMP_ICON_GEGL,
    NC_("filters-action", "_Long Shadow..."), NULL, NULL,
    "gegl:long-shadow",
    GIMP_HELP_FILTER_LONG_SHADOW },

  { "filters-mantiuk-2006", GIMP_ICON_GEGL,
    NC_("filters-action", "_Mantiuk 2006..."), NULL, NULL,
    "gegl:mantiuk06",
    GIMP_HELP_FILTER_MANTIUK_2006 },

  { "filters-maze", GIMP_ICON_GEGL,
    NC_("filters-action", "_Maze..."), NULL, NULL,
    "gegl:maze",
    GIMP_HELP_FILTER_MAZE },

  { "filters-mean-curvature-blur", GIMP_ICON_GEGL,
    NC_("filters-action", "Mean C_urvature Blur..."), NULL, NULL,
    "gegl:mean-curvature-blur",
    GIMP_HELP_FILTER_MEAN_CURVATURE_BLUR },

  { "filters-median-blur", GIMP_ICON_GEGL,
    NC_("filters-action", "_Median Blur..."), NULL, NULL,
    "gegl:median-blur",
    GIMP_HELP_FILTER_MEDIAN_BLUR },

  { "filters-mono-mixer", GIMP_ICON_GEGL,
    NC_("filters-action", "_Mono Mixer..."), NULL, NULL,
    "gegl:mono-mixer",
    GIMP_HELP_FILTER_MONO_MIXER },

  { "filters-mosaic", GIMP_ICON_GEGL,
    NC_("filters-action", "_Mosaic..."), NULL, NULL,
    "gegl:mosaic",
    GIMP_HELP_FILTER_MOSAIC },

  { "filters-motion-blur-circular", GIMP_ICON_GEGL,
    NC_("filters-action", "_Circular Motion Blur..."), NULL, NULL,
    "gegl:motion-blur-circular",
    GIMP_HELP_FILTER_MOTION_BLUR_CIRCULAR },

  { "filters-motion-blur-linear", GIMP_ICON_GEGL,
    NC_("filters-action", "_Linear Motion Blur..."), NULL, NULL,
    "gegl:motion-blur-linear",
    GIMP_HELP_FILTER_MOTION_BLUR_LINEAR },

  { "filters-motion-blur-zoom", GIMP_ICON_GEGL,
    NC_("filters-action", "_Zoom Motion Blur..."), NULL, NULL,
    "gegl:motion-blur-zoom",
    GIMP_HELP_FILTER_MOTION_BLUR_ZOOM },

  { "filters-noise-cell", GIMP_ICON_GEGL,
    NC_("filters-action", "_Cell Noise..."), NULL, NULL,
    "gegl:cell-noise",
    GIMP_HELP_FILTER_NOISE_CELL },

  { "filters-newsprint", GIMP_ICON_GEGL,
    NC_("filters-action", "_Newsprint..."), NULL, NULL,
    "gegl:newsprint",
    GIMP_HELP_FILTER_NEWSPRINT },

  { "filters-noise-cie-lch", GIMP_ICON_GEGL,
    NC_("filters-action", "_CIE lch Noise..."), NULL, NULL,
    "gegl:noise-cie-lch",
    GIMP_HELP_FILTER_NOISE_CIE_LCH },

  { "filters-noise-hsv", GIMP_ICON_GEGL,
    NC_("filters-action", "HS_V Noise..."), NULL, NULL,
    "gegl:noise-hsv",
    GIMP_HELP_FILTER_NOISE_HSV },

  { "filters-noise-hurl", GIMP_ICON_GEGL,
    NC_("filters-action", "_Hurl..."), NULL, NULL,
    "gegl:noise-hurl",
    GIMP_HELP_FILTER_NOISE_HURL },

  { "filters-noise-perlin", GIMP_ICON_GEGL,
    NC_("filters-action", "Perlin _Noise..."), NULL, NULL,
    "gegl:perlin-noise",
    GIMP_HELP_FILTER_NOISE_PERLIN },

  { "filters-noise-pick", GIMP_ICON_GEGL,
    NC_("filters-action", "_Pick..."), NULL, NULL,
    "gegl:noise-pick",
    GIMP_HELP_FILTER_NOISE_PICK },

  { "filters-noise-rgb", GIMP_ICON_GEGL,
    NC_("filters-action", "_RGB Noise..."), NULL, NULL,
    "gegl:noise-rgb",
    GIMP_HELP_FILTER_NOISE_RGB },

  { "filters-noise-reduction", GIMP_ICON_GEGL,
    NC_("filters-action", "Noise R_eduction..."), NULL, NULL,
    "gegl:noise-reduction",
    GIMP_HELP_FILTER_NOISE_REDUCTION },

  { "filters-noise-simplex", GIMP_ICON_GEGL,
    NC_("filters-action", "_Simplex Noise..."), NULL, NULL,
    "gegl:simplex-noise",
    GIMP_HELP_FILTER_NOISE_SIMPLEX },

  { "filters-noise-slur", GIMP_ICON_GEGL,
    NC_("filters-action", "_Slur..."), NULL, NULL,
    "gegl:noise-slur",
    GIMP_HELP_FILTER_NOISE_SLUR },

  { "filters-noise-solid", GIMP_ICON_GEGL,
    NC_("filters-action", "_Solid Noise..."), NULL, NULL,
    "gegl:noise-solid",
    GIMP_HELP_FILTER_NOISE_SOLID },

  { "filters-noise-spread", GIMP_ICON_GEGL,
    NC_("filters-action", "Sp_read..."), NULL, NULL,
    "gegl:noise-spread",
    GIMP_HELP_FILTER_NOISE_SPREAD },

  { "filters-normal-map", GIMP_ICON_GEGL,
    NC_("filters-action", "_Normal Map..."), NULL, NULL,
    "gegl:normal-map",
    GIMP_HELP_FILTER_NORMAL_MAP },

  { "filters-offset", GIMP_ICON_TOOL_OFFSET,
    NC_("filters-action", "_Offset..."), "<primary><shift>O", NULL,
    "gimp:offset",
    GIMP_HELP_TOOL_OFFSET },

  { "filters-oilify", GIMP_ICON_GEGL,
    NC_("filters-action", "Oili_fy..."), NULL, NULL,
    "gegl:oilify",
    GIMP_HELP_FILTER_OILIFY },

  { "filters-panorama-projection", GIMP_ICON_GEGL,
    NC_("filters-action", "_Panorama Projection..."), NULL, NULL,
    "gegl:panorama-projection",
    GIMP_HELP_FILTER_PANORAMA_PROJECTION },

  { "filters-photocopy", GIMP_ICON_GEGL,
    NC_("filters-action", "_Photocopy..."), NULL, NULL,
    "gegl:photocopy",
    GIMP_HELP_FILTER_PHOTOCOPY },

  { "filters-pixelize", GIMP_ICON_GEGL,
    NC_("filters-action", "_Pixelize..."), NULL, NULL,
    "gegl:pixelize",
    GIMP_HELP_FILTER_PIXELIZE },

  { "filters-plasma", GIMP_ICON_GEGL,
    NC_("filters-action", "_Plasma..."), NULL, NULL,
    "gegl:plasma",
    GIMP_HELP_FILTER_PLASMA },

  { "filters-polar-coordinates", GIMP_ICON_GEGL,
    NC_("filters-action", "P_olar Coordinates..."), NULL, NULL,
    "gegl:polar-coordinates",
    GIMP_HELP_FILTER_POLAR_COORDINATES },

  { "filters-posterize", GIMP_ICON_TOOL_POSTERIZE,
    NC_("filters-action", "_Posterize..."), NULL, NULL,
    "gimp:posterize",
    GIMP_HELP_FILTER_POSTERIZE },

  { "filters-recursive-transform", GIMP_ICON_GEGL,
    NC_("filters-action", "_Recursive Transform..."), NULL, NULL,
    "gegl:recursive-transform",
    GIMP_HELP_FILTER_RECURSIVE_TRANSFORM },

  { "filters-red-eye-removal", GIMP_ICON_GEGL,
    NC_("filters-action", "_Red Eye Removal..."), NULL, NULL,
    "gegl:red-eye-removal",
    GIMP_HELP_FILTER_RED_EYE_REMOVAL },

  { "filters-reinhard-2005", GIMP_ICON_GEGL,
    NC_("filters-action", "_Reinhard 2005..."), NULL, NULL,
    "gegl:reinhard05",
    GIMP_HELP_FILTER_REINHARD_2005 },

  { "filters-rgb-clip", GIMP_ICON_GEGL,
    NC_("filters-action", "RGB _Clip..."), NULL, NULL,
    "gegl:rgb-clip",
    GIMP_HELP_FILTER_RGB_CLIP },

  { "filters-ripple", GIMP_ICON_GEGL,
    NC_("filters-action", "_Ripple..."), NULL, NULL,
    "gegl:ripple",
    GIMP_HELP_FILTER_RIPPLE },

  { "filters-saturation", GIMP_ICON_GEGL,
    NC_("filters-action", "Sat_uration..."), NULL, NULL,
    "gegl:saturation",
    GIMP_HELP_FILTER_SATURATION },

  { "filters-semi-flatten", GIMP_ICON_GEGL,
    NC_("filters-action", "_Semi-Flatten..."), NULL, NULL,
    "gimp:semi-flatten",
    GIMP_HELP_FILTER_SEMI_FLATTEN },

  { "filters-sepia", GIMP_ICON_GEGL,
    NC_("filters-action", "_Sepia..."), NULL, NULL,
    "gegl:sepia",
    GIMP_HELP_FILTER_SEPIA },

  { "filters-shadows-highlights", GIMP_ICON_TOOL_SHADOWS_HIGHLIGHTS,
    NC_("filters-action", "S_hadows-Highlights..."), NULL, NULL,
    "gegl:shadows-highlights",
    GIMP_HELP_FILTER_SHADOWS_HIGHLIGHTS },

  { "filters-shift", GIMP_ICON_GEGL,
    NC_("filters-action", "_Shift..."), NULL, NULL,
    "gegl:shift",
    GIMP_HELP_FILTER_SHIFT },

  { "filters-sinus", GIMP_ICON_GEGL,
    NC_("filters-action", "_Sinus..."), NULL, NULL,
    "gegl:sinus",
    GIMP_HELP_FILTER_SINUS },

  { "filters-slic", GIMP_ICON_GEGL,
    NC_("filters-action", "_Simple Linear Iterative Clustering..."), NULL, NULL,
    "gegl:slic",
    GIMP_HELP_FILTER_SLIC },

  { "filters-snn-mean", GIMP_ICON_GEGL,
    NC_("filters-action", "_Symmetric Nearest Neighbor..."), NULL, NULL,
    "gegl:snn-mean",
    GIMP_HELP_FILTER_SNN_MEAN },

  { "filters-softglow", GIMP_ICON_GEGL,
    NC_("filters-action", "_Softglow..."), NULL, NULL,
    "gegl:softglow",
    GIMP_HELP_FILTER_SOFTGLOW },

  { "filters-spherize", GIMP_ICON_GEGL,
    NC_("filters-action", "Spheri_ze..."), NULL, NULL,
    "gegl:spherize",
    GIMP_HELP_FILTER_SPHERIZE },

  { "filters-spiral", GIMP_ICON_GEGL,
    NC_("filters-action", "S_piral..."), NULL, NULL,
    "gegl:spiral",
    GIMP_HELP_FILTER_SPIRAL },

  { "filters-stretch-contrast", GIMP_ICON_GEGL,
    NC_("filters-action", "_Stretch Contrast..."), NULL, NULL,
    "gegl:stretch-contrast",
    GIMP_HELP_FILTER_STRETCH_CONTRAST },

  { "filters-stress", GIMP_ICON_GEGL,
    NC_("filters-action", "_Stress..."), NULL, NULL,
    "gegl:stress",
    GIMP_HELP_FILTER_STRESS },

  { "filters-supernova", GIMP_ICON_GEGL,
    NC_("filters-action", "Super_nova..."), NULL, NULL,
    "gegl:supernova",
    GIMP_HELP_FILTER_SUPERNOVA },

  { "filters-threshold", GIMP_ICON_TOOL_THRESHOLD,
    NC_("filters-action", "_Threshold..."), NULL, NULL,
    "gimp:threshold",
    GIMP_HELP_TOOL_THRESHOLD },

  { "filters-threshold-alpha", GIMP_ICON_GEGL,
    NC_("filters-action", "_Threshold Alpha..."), NULL, NULL,
    "gimp:threshold-alpha",
    GIMP_HELP_FILTER_THRESHOLD_ALPHA },

  { "filters-tile-glass", GIMP_ICON_GEGL,
    NC_("filters-action", "_Glass Tile..."), NULL, NULL,
    "gegl:tile-glass",
    GIMP_HELP_FILTER_TILE_GLASS },

  { "filters-tile-paper", GIMP_ICON_GEGL,
    NC_("filters-action", "_Paper Tile..."), NULL, NULL,
    "gegl:tile-paper",
    GIMP_HELP_FILTER_TILE_PAPER },

  { "filters-tile-seamless", GIMP_ICON_GEGL,
    NC_("filters-action", "_Tile Seamless..."), NULL, NULL,
    "gegl:tile-seamless",
    GIMP_HELP_FILTER_TILE_SEAMLESS },

  { "filters-unsharp-mask", GIMP_ICON_GEGL,
    NC_("filters-action", "Sharpen (_Unsharp Mask)..."), NULL, NULL,
    "gegl:unsharp-mask",
    GIMP_HELP_FILTER_UNSHARP_MASK },

  { "filters-value-propagate", GIMP_ICON_GEGL,
    NC_("filters-action", "_Value Propagate..."), NULL, NULL,
    "gegl:value-propagate",
    GIMP_HELP_FILTER_VALUE_PROPAGATE },

  { "filters-video-degradation", GIMP_ICON_GEGL,
    NC_("filters-action", "Vi_deo Degradation..."), NULL, NULL,
    "gegl:video-degradation",
    GIMP_HELP_FILTER_VIDEO_DEGRADATION },

  { "filters-vignette", GIMP_ICON_GEGL,
    NC_("filters-action", "_Vignette..."), NULL, NULL,
    "gegl:vignette",
    GIMP_HELP_FILTER_VIGNETTE },

  { "filters-waterpixels", GIMP_ICON_GEGL,
    NC_("filters-action", "_Waterpixels..."), NULL, NULL,
    "gegl:waterpixels",
    GIMP_HELP_FILTER_WATERPIXELS },

  { "filters-waves", GIMP_ICON_GEGL,
    NC_("filters-action", "_Waves..."), NULL, NULL,
    "gegl:waves",
    GIMP_HELP_FILTER_WAVES },

  { "filters-whirl-pinch", GIMP_ICON_GEGL,
    NC_("filters-action", "W_hirl and Pinch..."), NULL, NULL,
    "gegl:whirl-pinch",
    GIMP_HELP_FILTER_WHIRL_PINCH },

  { "filters-wind", GIMP_ICON_GEGL,
    NC_("filters-action", "W_ind..."), NULL, NULL,
    "gegl:wind",
    GIMP_HELP_FILTER_WIND }
};

static const GimpEnumActionEntry filters_repeat_actions[] =
{
  { "filters-repeat", GIMP_ICON_SYSTEM_RUN,
    NC_("filters-action", "Re_peat Last"), "<primary>F",
    NC_("filters-action",
        "Rerun the last used filter using the same settings"),
    GIMP_RUN_WITH_LAST_VALS, FALSE,
    GIMP_HELP_FILTER_REPEAT },

  { "filters-reshow", GIMP_ICON_DIALOG_RESHOW_FILTER,
    NC_("filters-action", "R_e-Show Last"), "<primary><shift>F",
    NC_("filters-action", "Show the last used filter dialog again"),
    GIMP_RUN_INTERACTIVE, FALSE,
    GIMP_HELP_FILTER_RESHOW }
};


void
filters_actions_setup (GimpActionGroup *group)
{
  GimpProcedureActionEntry *entries;
  gint                      n_entries;
  gint                      i;

  gimp_action_group_add_actions (group, "filters-action",
                                 filters_menu_actions,
                                 G_N_ELEMENTS (filters_menu_actions));

  gimp_action_group_add_string_actions (group, "filters-action",
                                        filters_actions,
                                        G_N_ELEMENTS (filters_actions),
                                        filters_apply_cmd_callback);
  filters_actions_set_tooltips (group, filters_actions,
                                G_N_ELEMENTS (filters_actions));

  gimp_action_group_add_string_actions (group, "filters-action",
                                        filters_settings_actions,
                                        G_N_ELEMENTS (filters_settings_actions),
                                        filters_apply_cmd_callback);
  filters_actions_set_tooltips (group, filters_settings_actions,
                                G_N_ELEMENTS (filters_settings_actions));

  gimp_action_group_add_string_actions (group, "filters-action",
                                        filters_interactive_actions,
                                        G_N_ELEMENTS (filters_interactive_actions),
                                        filters_apply_interactive_cmd_callback);
  filters_actions_set_tooltips (group, filters_interactive_actions,
                                G_N_ELEMENTS (filters_interactive_actions));

  gimp_action_group_add_enum_actions (group, "filters-action",
                                      filters_repeat_actions,
                                      G_N_ELEMENTS (filters_repeat_actions),
                                      filters_repeat_cmd_callback);

  n_entries = gimp_filter_history_size (group->gimp);

  entries = g_new0 (GimpProcedureActionEntry, n_entries);

  for (i = 0; i < n_entries; i++)
    {
      entries[i].name        = g_strdup_printf ("filters-recent-%02d", i + 1);
      entries[i].icon_name   = NULL;
      entries[i].label       = "";
      entries[i].accelerator = "";
      entries[i].tooltip     = NULL;
      entries[i].procedure   = NULL;
      entries[i].help_id     = GIMP_HELP_FILTER_RESHOW;
    }

  gimp_action_group_add_procedure_actions (group, entries, n_entries,
                                           filters_history_cmd_callback);

  for (i = 0; i < n_entries; i++)
    {
      gimp_action_group_set_action_visible (group, entries[i].name, FALSE);
      g_free ((gchar *) entries[i].name);
    }

  g_free (entries);

  g_signal_connect_object (group->gimp, "filter-history-changed",
                           G_CALLBACK (filters_actions_history_changed),
                           group, 0);

  filters_actions_history_changed (group->gimp, group);
}

void
filters_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpImage    *image;
  GimpDrawable *drawable       = NULL;
  gboolean      writable       = FALSE;
  gboolean      gray           = FALSE;
  gboolean      alpha          = FALSE;
  gboolean      supports_alpha = FALSE;

  image = action_data_get_image (data);

  if (image)
    {
      drawable = gimp_image_get_active_drawable (image);

      if (drawable)
        {
          GimpItem *item;

          gray           = gimp_drawable_is_gray (drawable);
          alpha          = gimp_drawable_has_alpha (drawable);
          supports_alpha = gimp_drawable_supports_alpha (drawable);

          if (GIMP_IS_LAYER_MASK (drawable))
            item = GIMP_ITEM (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (drawable)));
          else
            item = GIMP_ITEM (drawable);

          writable = ! gimp_item_is_content_locked (item);

          if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
            writable = FALSE;
        }
   }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("filters-alien-map",               writable);
  SET_SENSITIVE ("filters-antialias",               writable);
  SET_SENSITIVE ("filters-apply-canvas",            writable);
  SET_SENSITIVE ("filters-apply-lens",              writable);
  SET_SENSITIVE ("filters-bayer-matrix",            writable);
  SET_SENSITIVE ("filters-brightness-contrast",     writable);
  SET_SENSITIVE ("filters-bump-map",                writable);
  SET_SENSITIVE ("filters-c2g",                     writable && !gray);
  SET_SENSITIVE ("filters-cartoon",                 writable);
  SET_SENSITIVE ("filters-channel-mixer",           writable);
  SET_SENSITIVE ("filters-checkerboard",            writable);
  SET_SENSITIVE ("filters-color-balance",           writable && !gray);
  SET_SENSITIVE ("filters-color-enhance",           writable && !gray);
  SET_SENSITIVE ("filters-color-exchange",          writable);
  SET_SENSITIVE ("filters-colorize",                writable && !gray);
  SET_SENSITIVE ("filters-dither",                  writable);
  SET_SENSITIVE ("filters-color-rotate",            writable);
  SET_SENSITIVE ("filters-color-temperature",       writable && !gray);
  SET_SENSITIVE ("filters-color-to-alpha",          writable && supports_alpha);
  SET_SENSITIVE ("filters-component-extract",       writable);
  SET_SENSITIVE ("filters-convolution-matrix",      writable);
  SET_SENSITIVE ("filters-cubism",                  writable);
  SET_SENSITIVE ("filters-curves",                  writable);
  SET_SENSITIVE ("filters-deinterlace",             writable);
  SET_SENSITIVE ("filters-desaturate",              writable && !gray);
  SET_SENSITIVE ("filters-difference-of-gaussians", writable);
  SET_SENSITIVE ("filters-diffraction-patterns",    writable);
  SET_SENSITIVE ("filters-dilate",                  writable);
  SET_SENSITIVE ("filters-displace",                writable);
  SET_SENSITIVE ("filters-distance-map",            writable);
  SET_SENSITIVE ("filters-dropshadow",              writable && alpha);
  SET_SENSITIVE ("filters-edge",                    writable);
  SET_SENSITIVE ("filters-edge-laplace",            writable);
  SET_SENSITIVE ("filters-edge-neon",               writable);
  SET_SENSITIVE ("filters-edge-sobel",              writable);
  SET_SENSITIVE ("filters-emboss",                  writable);
  SET_SENSITIVE ("filters-engrave",                 writable);
  SET_SENSITIVE ("filters-erode",                   writable);
  SET_SENSITIVE ("filters-exposure",                writable);
  SET_SENSITIVE ("filters-fattal-2002",             writable);
  SET_SENSITIVE ("filters-fractal-trace",           writable);
  SET_SENSITIVE ("filters-gaussian-blur",           writable);
  SET_SENSITIVE ("filters-gaussian-blur-selective", writable);
  SET_SENSITIVE ("filters-gegl-graph",              writable);
  SET_SENSITIVE ("filters-grid",                    writable);
  SET_SENSITIVE ("filters-high-pass",               writable);
  SET_SENSITIVE ("filters-hue-chroma",              writable);
  SET_SENSITIVE ("filters-hue-saturation",          writable && !gray);
  SET_SENSITIVE ("filters-illusion",                writable);
  SET_SENSITIVE ("filters-invert-linear",           writable);
  SET_SENSITIVE ("filters-invert-perceptual",       writable);
  SET_SENSITIVE ("filters-invert-value",            writable);
  SET_SENSITIVE ("filters-image-gradient",          writable);
  SET_SENSITIVE ("filters-kaleidoscope",            writable);
  SET_SENSITIVE ("filters-lens-distortion",         writable);
  SET_SENSITIVE ("filters-lens-flare",              writable);
  SET_SENSITIVE ("filters-levels",                  writable);
  SET_SENSITIVE ("filters-linear-sinusoid",         writable);
  SET_SENSITIVE ("filters-little-planet",           writable);
  SET_SENSITIVE ("filters-long-shadow",             writable && alpha);
  SET_SENSITIVE ("filters-mantiuk-2006",            writable);
  SET_SENSITIVE ("filters-maze",                    writable);
  SET_SENSITIVE ("filters-mean-curvature-blur",     writable);
  SET_SENSITIVE ("filters-median-blur",             writable);
  SET_SENSITIVE ("filters-mono-mixer",              writable && !gray);
  SET_SENSITIVE ("filters-mosaic",                  writable);
  SET_SENSITIVE ("filters-motion-blur-circular",    writable);
  SET_SENSITIVE ("filters-motion-blur-linear",      writable);
  SET_SENSITIVE ("filters-motion-blur-zoom",        writable);
  SET_SENSITIVE ("filters-newsprint",               writable);
  SET_SENSITIVE ("filters-noise-cell",              writable);
  SET_SENSITIVE ("filters-noise-cie-lch",           writable);
  SET_SENSITIVE ("filters-noise-hsv",               writable && !gray);
  SET_SENSITIVE ("filters-noise-hurl",              writable);
  SET_SENSITIVE ("filters-noise-perlin",            writable);
  SET_SENSITIVE ("filters-noise-pick",              writable);
  SET_SENSITIVE ("filters-noise-reduction",         writable);
  SET_SENSITIVE ("filters-noise-rgb",               writable);
  SET_SENSITIVE ("filters-noise-simplex",           writable);
  SET_SENSITIVE ("filters-noise-slur",              writable);
  SET_SENSITIVE ("filters-noise-solid",             writable);
  SET_SENSITIVE ("filters-noise-spread",            writable);
  SET_SENSITIVE ("filters-normal-map",              writable);
  SET_SENSITIVE ("filters-offset",                  writable);
  SET_SENSITIVE ("filters-oilify",                  writable);
  SET_SENSITIVE ("filters-panorama-projection",     writable);
  SET_SENSITIVE ("filters-photocopy",               writable);
  SET_SENSITIVE ("filters-pixelize",                writable);
  SET_SENSITIVE ("filters-plasma",                  writable);
  SET_SENSITIVE ("filters-polar-coordinates",       writable);
  SET_SENSITIVE ("filters-posterize",               writable);
  SET_SENSITIVE ("filters-recursive-transform",     writable);
  SET_SENSITIVE ("filters-red-eye-removal",         writable && !gray);
  SET_SENSITIVE ("filters-reinhard-2005",           writable);
  SET_SENSITIVE ("filters-rgb-clip",                writable);
  SET_SENSITIVE ("filters-ripple",                  writable);
  SET_SENSITIVE ("filters-saturation",              writable && !gray);
  SET_SENSITIVE ("filters-semi-flatten",            writable && alpha);
  SET_SENSITIVE ("filters-sepia",                   writable && !gray);
  SET_SENSITIVE ("filters-shadows-highlights",      writable);
  SET_SENSITIVE ("filters-shift",                   writable);
  SET_SENSITIVE ("filters-sinus",                   writable);
  SET_SENSITIVE ("filters-slic",                    writable);
  SET_SENSITIVE ("filters-snn-mean",                writable);
  SET_SENSITIVE ("filters-softglow",                writable);
  SET_SENSITIVE ("filters-spherize",                writable);
  SET_SENSITIVE ("filters-spiral",                  writable);
  SET_SENSITIVE ("filters-stretch-contrast",        writable);
  SET_SENSITIVE ("filters-stretch-contrast-hsv",    writable);
  SET_SENSITIVE ("filters-stress",                  writable);
  SET_SENSITIVE ("filters-supernova",               writable);
  SET_SENSITIVE ("filters-threshold",               writable);
  SET_SENSITIVE ("filters-threshold-alpha",         writable && alpha);
  SET_SENSITIVE ("filters-tile-glass",              writable);
  SET_SENSITIVE ("filters-tile-paper",              writable);
  SET_SENSITIVE ("filters-tile-seamless",           writable);
  SET_SENSITIVE ("filters-unsharp-mask",            writable);
  SET_SENSITIVE ("filters-value-propagate",         writable);
  SET_SENSITIVE ("filters-video-degradation",       writable);
  SET_SENSITIVE ("filters-vignette",                writable);
  SET_SENSITIVE ("filters-waterpixels",             writable);
  SET_SENSITIVE ("filters-waves",                   writable);
  SET_SENSITIVE ("filters-whirl-pinch",             writable);
  SET_SENSITIVE ("filters-wind",                    writable);

#undef SET_SENSITIVE

  {
    GimpProcedure *proc = gimp_filter_history_nth (group->gimp, 0);
    gint           i;

    if (proc &&
        gimp_procedure_get_sensitive (proc, GIMP_OBJECT (drawable), NULL))
      {
        gimp_action_group_set_action_sensitive (group, "filters-repeat", TRUE);
        gimp_action_group_set_action_sensitive (group, "filters-reshow", TRUE);
      }
    else
      {
        gimp_action_group_set_action_sensitive (group, "filters-repeat", FALSE);
        gimp_action_group_set_action_sensitive (group, "filters-reshow", FALSE);
     }

    for (i = 0; i < gimp_filter_history_length (group->gimp); i++)
      {
        gchar    *name = g_strdup_printf ("filters-recent-%02d", i + 1);
        gboolean  sensitive;

        proc = gimp_filter_history_nth (group->gimp, i);

        sensitive = gimp_procedure_get_sensitive (proc, GIMP_OBJECT (drawable),
                                                  NULL);

        gimp_action_group_set_action_sensitive (group, name, sensitive);

        g_free (name);
      }
  }
}

static void
filters_actions_set_tooltips (GimpActionGroup             *group,
                              const GimpStringActionEntry *entries,
                              gint                         n_entries)
{
  gint i;

  for (i = 0; i < n_entries; i++)
    {
      const GimpStringActionEntry *entry = entries + i;
      const gchar                 *description;

      description = gegl_operation_get_key (entry->value, "description");

      if (description)
        gimp_action_group_set_action_tooltip (group, entry->name,
                                              description);
    }
}

static GimpActionGroup *
filters_actions_get_plug_in_group (GimpActionGroup *group)
{
  GList *list;

  for (list = gimp_ui_managers_from_name ("<Image>");
       list;
       list = g_list_next (list))
    {
      GimpUIManager *manager = list->data;

      /* if this is our UI manager */
      if (gimp_ui_manager_get_action_group (manager, "filters") == group)
        return gimp_ui_manager_get_action_group (manager, "plug-in");
    }

  /* this happens during initial UI manager construction */
  return NULL;
}

static void
filters_actions_history_changed (Gimp            *gimp,
                                 GimpActionGroup *group)
{
  GimpProcedure   *proc;
  GimpActionGroup *plug_in_group;
  gint             i;

  plug_in_group = filters_actions_get_plug_in_group (group);

  proc = gimp_filter_history_nth (gimp, 0);

  if (proc)
    {
      GimpAction  *actual_action = NULL;
      const gchar *label;
      gchar       *repeat;
      gchar       *reshow;
      gboolean     sensitive = FALSE;

      label = gimp_procedure_get_label (proc);

      repeat = g_strdup_printf (_("Re_peat \"%s\""),  label);
      reshow = g_strdup_printf (_("R_e-Show \"%s\""), label);

      gimp_action_group_set_action_label (group, "filters-repeat", repeat);
      gimp_action_group_set_action_label (group, "filters-reshow", reshow);

      g_free (repeat);
      g_free (reshow);

      if (g_str_has_prefix (gimp_object_get_name (proc), "filters-"))
        {
          actual_action =
            gimp_action_group_get_action (group,
                                          gimp_object_get_name (proc));
        }
      else if (plug_in_group)
        {
          /*  copy the sensitivity of the plug-in procedure's actual
           *  action instead of calling filters_actions_update()
           *  because doing the latter would set the sensitivity of
           *  this image's action on all images' actions. See bug
           *  #517683.
           */
          actual_action =
            gimp_action_group_get_action (plug_in_group,
                                          gimp_object_get_name (proc));
        }

      if (actual_action)
        sensitive = gimp_action_get_sensitive (actual_action);

      gimp_action_group_set_action_sensitive (group, "filters-repeat",
                                              sensitive);
      gimp_action_group_set_action_sensitive (group, "filters-reshow",
                                              sensitive);
   }
  else
    {
      gimp_action_group_set_action_label (group, "filters-repeat",
                                          _("Repeat Last"));
      gimp_action_group_set_action_label (group, "filters-reshow",
                                          _("Re-Show Last"));

      gimp_action_group_set_action_sensitive (group, "filters-repeat", FALSE);
      gimp_action_group_set_action_sensitive (group, "filters-reshow", FALSE);
    }

  for (i = 0; i < gimp_filter_history_length (gimp); i++)
    {
      GimpAction  *action;
      GimpAction  *actual_action = NULL;
      const gchar *label;
      gchar       *name;
      gboolean     sensitive = FALSE;

      name = g_strdup_printf ("filters-recent-%02d", i + 1);
      action = gimp_action_group_get_action (group, name);
      g_free (name);

      proc = gimp_filter_history_nth (gimp, i);

      label = gimp_procedure_get_menu_label (proc);

      if (g_str_has_prefix (gimp_object_get_name (proc), "filters-"))
        {
          actual_action =
            gimp_action_group_get_action (group,
                                          gimp_object_get_name (proc));
        }
      else if (plug_in_group)
        {
          /*  see comment above  */
          actual_action =
            gimp_action_group_get_action (plug_in_group,
                                          gimp_object_get_name (proc));
        }

      if (actual_action)
        sensitive = gimp_action_get_sensitive (actual_action);

      g_object_set (action,
                    "visible",   TRUE,
                    "sensitive", sensitive,
                    "procedure", proc,
                    "label",     label,
                    "icon-name", gimp_viewable_get_icon_name (GIMP_VIEWABLE (proc)),
                    "tooltip",   gimp_procedure_get_blurb (proc),
                    NULL);
    }

  for (; i < gimp_filter_history_size (gimp); i++)
    {
      GimpAction *action;
      gchar      *name = g_strdup_printf ("filters-recent-%02d", i + 1);

      action = gimp_action_group_get_action (group, name);
      g_free (name);

      g_object_set (action,
                    "visible",   FALSE,
                    "procedure", NULL,
                    NULL);
    }
}
