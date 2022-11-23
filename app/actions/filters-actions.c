/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma-filter-history.h"
#include "core/ligmaimage.h"
#include "core/ligmalayermask.h"

#include "pdb/ligmaprocedure.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmauimanager.h"

#include "actions.h"
#include "filters-actions.h"
#include "filters-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   filters_actions_set_tooltips    (LigmaActionGroup             *group,
                                               const LigmaStringActionEntry *entries,
                                               gint                         n_entries);
static void   filters_actions_history_changed (Ligma                        *ligma,
                                               LigmaActionGroup             *group);


/*  private variables  */

static const LigmaActionEntry filters_menu_actions[] =
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
                                              /* TRANSLATORS: menu group
                                               * containing mapping
                                               * filters.
                                               */
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

static const LigmaStringActionEntry filters_actions[] =
{
  { "filters-antialias", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Antialias"), NULL, NULL,
    "gegl:antialias",
    LIGMA_HELP_FILTER_ANTIALIAS },

  { "filters-color-enhance", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Color Enhance"), NULL, NULL,
    "gegl:color-enhance",
    LIGMA_HELP_FILTER_COLOR_ENHANCE },

  { "filters-invert-linear", LIGMA_ICON_INVERT,
    NC_("filters-action", "L_inear Invert"), NULL, NULL,
    "gegl:invert-linear",
    LIGMA_HELP_FILTER_INVERT_LINEAR },

  { "filters-invert-perceptual", LIGMA_ICON_INVERT,
    NC_("filters-action", "In_vert"), NULL, NULL,
    "gegl:invert-gamma",
    LIGMA_HELP_FILTER_INVERT_PERCEPTUAL },

  { "filters-invert-value", LIGMA_ICON_INVERT,
    NC_("filters-action", "_Value Invert"), NULL, NULL,
    "gegl:value-invert",
    LIGMA_HELP_FILTER_INVERT_VALUE },

  { "filters-stretch-contrast-hsv", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Stretch Contrast HSV"), NULL, NULL,
    "gegl:stretch-contrast-hsv",
    LIGMA_HELP_FILTER_STRETCH_CONTRAST_HSV }
};

static const LigmaStringActionEntry filters_settings_actions[] =
{
  { "filters-dilate", LIGMA_ICON_GEGL,
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
    LIGMA_HELP_FILTER_DILATE },

  { "filters-erode", LIGMA_ICON_GEGL,
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
    LIGMA_HELP_FILTER_ERODE }
};

static const LigmaStringActionEntry filters_interactive_actions[] =
{
  { "filters-alien-map", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Alien Map..."), NULL, NULL,
    "gegl:alien-map",
    LIGMA_HELP_FILTER_ALIEN_MAP },

  { "filters-apply-canvas", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Apply Canvas..."), NULL, NULL,
    "gegl:texturize-canvas",
    LIGMA_HELP_FILTER_APPLY_CANVAS },

  { "filters-apply-lens", LIGMA_ICON_GEGL,
    NC_("filters-action", "Apply _Lens..."), NULL, NULL,
    "gegl:apply-lens",
    LIGMA_HELP_FILTER_APPLY_LENS },

  { "filters-bayer-matrix", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Bayer Matrix..."), NULL, NULL,
    "gegl:bayer-matrix",
    LIGMA_HELP_FILTER_BAYER_MATRIX },

  { "filters-bloom", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Bloom..."), NULL, NULL,
    "gegl:bloom",
    LIGMA_HELP_FILTER_BLOOM },

  { "filters-brightness-contrast", LIGMA_ICON_TOOL_BRIGHTNESS_CONTRAST,
    NC_("filters-action", "B_rightness-Contrast..."), NULL, NULL,
    "ligma:brightness-contrast",
    LIGMA_HELP_TOOL_BRIGHTNESS_CONTRAST },

  { "filters-bump-map", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Bump Map..."), NULL, NULL,
    "gegl:bump-map",
    LIGMA_HELP_FILTER_BUMP_MAP },

  { "filters-c2g", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Color to Gray..."), NULL, NULL,
    "gegl:c2g",
    LIGMA_HELP_FILTER_C2G },

  { "filters-cartoon", LIGMA_ICON_GEGL,
    NC_("filters-action", "Ca_rtoon..."), NULL, NULL,
    "gegl:cartoon",
    LIGMA_HELP_FILTER_CARTOON },

  { "filters-channel-mixer", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Channel Mixer..."), NULL, NULL,
    "gegl:channel-mixer",
    LIGMA_HELP_FILTER_CHANNEL_MIXER },

  { "filters-checkerboard", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Checkerboard..."), NULL, NULL,
    "gegl:checkerboard",
    LIGMA_HELP_FILTER_CHECKERBOARD },

  { "filters-color-balance", LIGMA_ICON_TOOL_COLOR_BALANCE,
    NC_("filters-action", "Color _Balance..."), NULL, NULL,
    "ligma:color-balance",
    LIGMA_HELP_TOOL_COLOR_BALANCE },

  { "filters-color-exchange", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Color Exchange..."), NULL, NULL,
    "gegl:color-exchange",
    LIGMA_HELP_FILTER_COLOR_EXCHANGE },

  { "filters-colorize", LIGMA_ICON_TOOL_COLORIZE,
    NC_("filters-action", "Colori_ze..."), NULL, NULL,
    "ligma:colorize",
    LIGMA_HELP_TOOL_COLORIZE },

  { "filters-dither", LIGMA_ICON_GEGL,
    NC_("filters-action", "Dithe_r..."), NULL, NULL,
    "gegl:dither",
    LIGMA_HELP_FILTER_DITHER },

  { "filters-color-rotate", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Rotate Colors..."), NULL, NULL,
    "gegl:color-rotate",
    LIGMA_HELP_FILTER_COLOR_ROTATE },

  { "filters-color-temperature", LIGMA_ICON_TOOL_COLOR_TEMPERATURE,
    NC_("filters-action", "Color T_emperature..."), NULL, NULL,
    "gegl:color-temperature",
    LIGMA_HELP_FILTER_COLOR_TEMPERATURE },

  { "filters-color-to-alpha", LIGMA_ICON_GEGL,
    NC_("filters-action", "Color to _Alpha..."), NULL, NULL,
    "gegl:color-to-alpha",
    LIGMA_HELP_FILTER_COLOR_TO_ALPHA },

  { "filters-component-extract", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Extract Component..."), NULL, NULL,
    "gegl:component-extract",
    LIGMA_HELP_FILTER_COMPONENT_EXTRACT },

  { "filters-convolution-matrix", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Convolution Matrix..."), NULL, NULL,
    "gegl:convolution-matrix",
    LIGMA_HELP_FILTER_CONVOLUTION_MATRIX },

  { "filters-cubism", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Cubism..."), NULL, NULL,
    "gegl:cubism",
    LIGMA_HELP_FILTER_CUBISM },

  { "filters-curves", LIGMA_ICON_TOOL_CURVES,
    NC_("filters-action", "_Curves..."), NULL, NULL,
    "ligma:curves",
    LIGMA_HELP_TOOL_CURVES },

  { "filters-deinterlace", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Deinterlace..."), NULL, NULL,
    "gegl:deinterlace",
    LIGMA_HELP_FILTER_DEINTERLACE },

  { "filters-desaturate", LIGMA_ICON_TOOL_DESATURATE,
    NC_("filters-action", "_Desaturate..."), NULL, NULL,
    "ligma:desaturate",
    LIGMA_HELP_FILTER_DESATURATE },

  { "filters-difference-of-gaussians", LIGMA_ICON_GEGL,
    NC_("filters-action", "Difference of _Gaussians..."), NULL, NULL,
    "gegl:difference-of-gaussians",
    LIGMA_HELP_FILTER_DIFFERENCE_OF_GAUSSIANS },

  { "filters-diffraction-patterns", LIGMA_ICON_GEGL,
    NC_("filters-action", "D_iffraction Patterns..."), NULL, NULL,
    "gegl:diffraction-patterns",
    LIGMA_HELP_FILTER_DIFFRACTION_PATTERNS },

  { "filters-displace", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Displace..."), NULL, NULL,
    "gegl:displace",
    LIGMA_HELP_FILTER_DISPLACE },

  { "filters-distance-map", LIGMA_ICON_GEGL,
    NC_("filters-action", "Distance _Map..."), NULL, NULL,
    "gegl:distance-transform",
    LIGMA_HELP_FILTER_DISTANCE_MAP },

  { "filters-dropshadow", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Drop Shadow..."), NULL, NULL,
    "gegl:dropshadow",
    LIGMA_HELP_FILTER_DROPSHADOW },

  { "filters-edge", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Edge..."), NULL, NULL,
    "gegl:edge",
    LIGMA_HELP_FILTER_EDGE },

  { "filters-edge-laplace", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Laplace"), NULL, NULL,
    "gegl:edge-laplace",
    LIGMA_HELP_FILTER_EDGE_LAPLACE },

  { "filters-edge-neon", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Neon..."), NULL, NULL,
    "gegl:edge-neon",
    LIGMA_HELP_FILTER_EDGE_NEON },

  { "filters-edge-sobel", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Sobel..."), NULL, NULL,
    "gegl:edge-sobel",
    LIGMA_HELP_FILTER_EDGE_SOBEL },

  { "filters-emboss", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Emboss..."), NULL, NULL,
    "gegl:emboss",
    LIGMA_HELP_FILTER_EMBOSS },

  { "filters-engrave", LIGMA_ICON_GEGL,
    NC_("filters-action", "En_grave..."), NULL, NULL,
    "gegl:engrave",
    LIGMA_HELP_FILTER_ENGRAVE },

  { "filters-exposure", LIGMA_ICON_TOOL_EXPOSURE,
    NC_("filters-action", "E_xposure..."), NULL, NULL,
    "gegl:exposure",
    LIGMA_HELP_FILTER_EXPOSURE },

  { "filters-fattal-2002", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Fattal et al. 2002..."), NULL, NULL,
    "gegl:fattal02",
    LIGMA_HELP_FILTER_FATTAL_2002 },

  { "filters-focus-blur", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Focus Blur..."), NULL, NULL,
    "gegl:focus-blur",
    LIGMA_HELP_FILTER_FOCUS_BLUR },

  { "filters-fractal-trace", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Fractal Trace..."), NULL, NULL,
    "gegl:fractal-trace",
    LIGMA_HELP_FILTER_FRACTAL_TRACE },

  { "filters-gaussian-blur", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Gaussian Blur..."), NULL, NULL,
    "gegl:gaussian-blur",
    LIGMA_HELP_FILTER_GAUSSIAN_BLUR },

  { "filters-gaussian-blur-selective", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Selective Gaussian Blur..."), NULL, NULL,
    "gegl:gaussian-blur-selective",
    LIGMA_HELP_FILTER_GAUSSIAN_BLUR_SELECTIVE },

  { "filters-gegl-graph", LIGMA_ICON_GEGL,
    NC_("filters-action", "_GEGL Graph..."), NULL, NULL,
    "gegl:gegl",
    LIGMA_HELP_FILTER_GEGL_GRAPH },

  { "filters-grid", LIGMA_ICON_GRID,
    NC_("filters-action", "_Grid..."), NULL, NULL,
    "gegl:grid",
    LIGMA_HELP_FILTER_GRID },

  { "filters-high-pass", LIGMA_ICON_GEGL,
    NC_("filters-action", "_High Pass..."), NULL, NULL,
    "gegl:high-pass",
    LIGMA_HELP_FILTER_HIGH_PASS },

  { "filters-hue-chroma", LIGMA_ICON_GEGL,
    NC_("filters-action", "Hue-_Chroma..."), NULL, NULL,
    "gegl:hue-chroma",
    LIGMA_HELP_FILTER_HUE_CHROMA },

  { "filters-hue-saturation", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("filters-action", "Hue-_Saturation..."), NULL, NULL,
    "ligma:hue-saturation",
    LIGMA_HELP_TOOL_HUE_SATURATION },

  { "filters-illusion", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Illusion..."), NULL, NULL,
    "gegl:illusion",
    LIGMA_HELP_FILTER_ILLUSION },

  { "filters-image-gradient", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Image Gradient..."), NULL, NULL,
    "gegl:image-gradient",
    LIGMA_HELP_FILTER_IMAGE_GRADIENT },

  { "filters-kaleidoscope", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Kaleidoscope..."), NULL, NULL,
    "gegl:mirrors",
    LIGMA_HELP_FILTER_KALEIDOSCOPE },

  { "filters-lens-blur", LIGMA_ICON_GEGL,
    NC_("filters-action", "Le_ns Blur..."), NULL, NULL,
    "gegl:lens-blur",
    LIGMA_HELP_FILTER_LENS_BLUR },

  { "filters-lens-distortion", LIGMA_ICON_GEGL,
    NC_("filters-action", "Le_ns Distortion..."), NULL, NULL,
    "gegl:lens-distortion",
    LIGMA_HELP_FILTER_LENS_DISTORTION },

  { "filters-lens-flare", LIGMA_ICON_GEGL,
    NC_("filters-action", "Lens _Flare..."), NULL, NULL,
    "gegl:lens-flare",
    LIGMA_HELP_FILTER_LENS_FLARE },

  { "filters-levels", LIGMA_ICON_TOOL_LEVELS,
    NC_("filters-action", "_Levels..."), NULL, NULL,
    "ligma:levels",
    LIGMA_HELP_TOOL_LEVELS },

  { "filters-linear-sinusoid", LIGMA_ICON_TOOL_LEVELS,
    NC_("filters-action", "_Linear Sinusoid..."), NULL, NULL,
    "gegl:linear-sinusoid",
    LIGMA_HELP_FILTER_LINEAR_SINUSOID },

  { "filters-little-planet", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Little Planet..."), NULL, NULL,
    "gegl:stereographic-projection",
    LIGMA_HELP_FILTER_LITTLE_PLANET },

  { "filters-long-shadow", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Long Shadow..."), NULL, NULL,
    "gegl:long-shadow",
    LIGMA_HELP_FILTER_LONG_SHADOW },

  { "filters-mantiuk-2006", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Mantiuk 2006..."), NULL, NULL,
    "gegl:mantiuk06",
    LIGMA_HELP_FILTER_MANTIUK_2006 },

  { "filters-maze", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Maze..."), NULL, NULL,
    "gegl:maze",
    LIGMA_HELP_FILTER_MAZE },

  { "filters-mean-curvature-blur", LIGMA_ICON_GEGL,
    NC_("filters-action", "Mean C_urvature Blur..."), NULL, NULL,
    "gegl:mean-curvature-blur",
    LIGMA_HELP_FILTER_MEAN_CURVATURE_BLUR },

  { "filters-median-blur", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Median Blur..."), NULL, NULL,
    "gegl:median-blur",
    LIGMA_HELP_FILTER_MEDIAN_BLUR },

  { "filters-mono-mixer", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Mono Mixer..."), NULL, NULL,
    "gegl:mono-mixer",
    LIGMA_HELP_FILTER_MONO_MIXER },

  { "filters-mosaic", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Mosaic..."), NULL, NULL,
    "gegl:mosaic",
    LIGMA_HELP_FILTER_MOSAIC },

  { "filters-motion-blur-circular", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Circular Motion Blur..."), NULL, NULL,
    "gegl:motion-blur-circular",
    LIGMA_HELP_FILTER_MOTION_BLUR_CIRCULAR },

  { "filters-motion-blur-linear", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Linear Motion Blur..."), NULL, NULL,
    "gegl:motion-blur-linear",
    LIGMA_HELP_FILTER_MOTION_BLUR_LINEAR },

  { "filters-motion-blur-zoom", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Zoom Motion Blur..."), NULL, NULL,
    "gegl:motion-blur-zoom",
    LIGMA_HELP_FILTER_MOTION_BLUR_ZOOM },

  { "filters-noise-cell", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Cell Noise..."), NULL, NULL,
    "gegl:cell-noise",
    LIGMA_HELP_FILTER_NOISE_CELL },

  { "filters-newsprint", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Newsprint..."), NULL, NULL,
    "gegl:newsprint",
    LIGMA_HELP_FILTER_NEWSPRINT },

  { "filters-noise-cie-lch", LIGMA_ICON_GEGL,
    NC_("filters-action", "_CIE lch Noise..."), NULL, NULL,
    "gegl:noise-cie-lch",
    LIGMA_HELP_FILTER_NOISE_CIE_LCH },

  { "filters-noise-hsv", LIGMA_ICON_GEGL,
    NC_("filters-action", "HS_V Noise..."), NULL, NULL,
    "gegl:noise-hsv",
    LIGMA_HELP_FILTER_NOISE_HSV },

  { "filters-noise-hurl", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Hurl..."), NULL, NULL,
    "gegl:noise-hurl",
    LIGMA_HELP_FILTER_NOISE_HURL },

  { "filters-noise-perlin", LIGMA_ICON_GEGL,
    NC_("filters-action", "Perlin _Noise..."), NULL, NULL,
    "gegl:perlin-noise",
    LIGMA_HELP_FILTER_NOISE_PERLIN },

  { "filters-noise-pick", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Pick..."), NULL, NULL,
    "gegl:noise-pick",
    LIGMA_HELP_FILTER_NOISE_PICK },

  { "filters-noise-rgb", LIGMA_ICON_GEGL,
    NC_("filters-action", "_RGB Noise..."), NULL, NULL,
    "gegl:noise-rgb",
    LIGMA_HELP_FILTER_NOISE_RGB },

  { "filters-noise-reduction", LIGMA_ICON_GEGL,
    NC_("filters-action", "Noise R_eduction..."), NULL, NULL,
    "gegl:noise-reduction",
    LIGMA_HELP_FILTER_NOISE_REDUCTION },

  { "filters-noise-simplex", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Simplex Noise..."), NULL, NULL,
    "gegl:simplex-noise",
    LIGMA_HELP_FILTER_NOISE_SIMPLEX },

  { "filters-noise-slur", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Slur..."), NULL, NULL,
    "gegl:noise-slur",
    LIGMA_HELP_FILTER_NOISE_SLUR },

  { "filters-noise-solid", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Solid Noise..."), NULL, NULL,
    "gegl:noise-solid",
    LIGMA_HELP_FILTER_NOISE_SOLID },

  { "filters-noise-spread", LIGMA_ICON_GEGL,
    NC_("filters-action", "Sp_read..."), NULL, NULL,
    "gegl:noise-spread",
    LIGMA_HELP_FILTER_NOISE_SPREAD },

  { "filters-normal-map", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Normal Map..."), NULL, NULL,
    "gegl:normal-map",
    LIGMA_HELP_FILTER_NORMAL_MAP },

  { "filters-offset", LIGMA_ICON_TOOL_OFFSET,
    NC_("filters-action", "_Offset..."), "<primary><shift>O", NULL,
    "ligma:offset",
    LIGMA_HELP_TOOL_OFFSET },

  { "filters-oilify", LIGMA_ICON_GEGL,
    NC_("filters-action", "Oili_fy..."), NULL, NULL,
    "gegl:oilify",
    LIGMA_HELP_FILTER_OILIFY },

  { "filters-panorama-projection", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Panorama Projection..."), NULL, NULL,
    "gegl:panorama-projection",
    LIGMA_HELP_FILTER_PANORAMA_PROJECTION },

  { "filters-photocopy", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Photocopy..."), NULL, NULL,
    "gegl:photocopy",
    LIGMA_HELP_FILTER_PHOTOCOPY },

  { "filters-pixelize", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Pixelize..."), NULL, NULL,
    "gegl:pixelize",
    LIGMA_HELP_FILTER_PIXELIZE },

  { "filters-plasma", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Plasma..."), NULL, NULL,
    "gegl:plasma",
    LIGMA_HELP_FILTER_PLASMA },

  { "filters-polar-coordinates", LIGMA_ICON_GEGL,
    NC_("filters-action", "P_olar Coordinates..."), NULL, NULL,
    "gegl:polar-coordinates",
    LIGMA_HELP_FILTER_POLAR_COORDINATES },

  { "filters-posterize", LIGMA_ICON_TOOL_POSTERIZE,
    NC_("filters-action", "_Posterize..."), NULL, NULL,
    "ligma:posterize",
    LIGMA_HELP_FILTER_POSTERIZE },

  { "filters-recursive-transform", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Recursive Transform..."), NULL, NULL,
    "gegl:recursive-transform",
    LIGMA_HELP_FILTER_RECURSIVE_TRANSFORM },

  { "filters-red-eye-removal", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Red Eye Removal..."), NULL, NULL,
    "gegl:red-eye-removal",
    LIGMA_HELP_FILTER_RED_EYE_REMOVAL },

  { "filters-reinhard-2005", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Reinhard 2005..."), NULL, NULL,
    "gegl:reinhard05",
    LIGMA_HELP_FILTER_REINHARD_2005 },

  { "filters-rgb-clip", LIGMA_ICON_GEGL,
    NC_("filters-action", "RGB _Clip..."), NULL, NULL,
    "gegl:rgb-clip",
    LIGMA_HELP_FILTER_RGB_CLIP },

  { "filters-ripple", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Ripple..."), NULL, NULL,
    "gegl:ripple",
    LIGMA_HELP_FILTER_RIPPLE },

  { "filters-saturation", LIGMA_ICON_GEGL,
    NC_("filters-action", "Sat_uration..."), NULL, NULL,
    "gegl:saturation",
    LIGMA_HELP_FILTER_SATURATION },

  { "filters-semi-flatten", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Semi-Flatten..."), NULL, NULL,
    "ligma:semi-flatten",
    LIGMA_HELP_FILTER_SEMI_FLATTEN },

  { "filters-sepia", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Sepia..."), NULL, NULL,
    "gegl:sepia",
    LIGMA_HELP_FILTER_SEPIA },

  { "filters-shadows-highlights", LIGMA_ICON_TOOL_SHADOWS_HIGHLIGHTS,
    NC_("filters-action", "S_hadows-Highlights..."), NULL, NULL,
    "gegl:shadows-highlights",
    LIGMA_HELP_FILTER_SHADOWS_HIGHLIGHTS },

  { "filters-shift", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Shift..."), NULL, NULL,
    "gegl:shift",
    LIGMA_HELP_FILTER_SHIFT },

  { "filters-sinus", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Sinus..."), NULL, NULL,
    "gegl:sinus",
    LIGMA_HELP_FILTER_SINUS },

  { "filters-slic", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Simple Linear Iterative Clustering..."), NULL, NULL,
    "gegl:slic",
    LIGMA_HELP_FILTER_SLIC },

  { "filters-snn-mean", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Symmetric Nearest Neighbor..."), NULL, NULL,
    "gegl:snn-mean",
    LIGMA_HELP_FILTER_SNN_MEAN },

  { "filters-softglow", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Softglow..."), NULL, NULL,
    "gegl:softglow",
    LIGMA_HELP_FILTER_SOFTGLOW },

  { "filters-spherize", LIGMA_ICON_GEGL,
    NC_("filters-action", "Spheri_ze..."), NULL, NULL,
    "gegl:spherize",
    LIGMA_HELP_FILTER_SPHERIZE },

  { "filters-spiral", LIGMA_ICON_GEGL,
    NC_("filters-action", "S_piral..."), NULL, NULL,
    "gegl:spiral",
    LIGMA_HELP_FILTER_SPIRAL },

  { "filters-stretch-contrast", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Stretch Contrast..."), NULL, NULL,
    "gegl:stretch-contrast",
    LIGMA_HELP_FILTER_STRETCH_CONTRAST },

  { "filters-stress", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Stress..."), NULL, NULL,
    "gegl:stress",
    LIGMA_HELP_FILTER_STRESS },

  { "filters-supernova", LIGMA_ICON_GEGL,
    NC_("filters-action", "Super_nova..."), NULL, NULL,
    "gegl:supernova",
    LIGMA_HELP_FILTER_SUPERNOVA },

  { "filters-threshold", LIGMA_ICON_TOOL_THRESHOLD,
    NC_("filters-action", "_Threshold..."), NULL, NULL,
    "ligma:threshold",
    LIGMA_HELP_TOOL_THRESHOLD },

  { "filters-threshold-alpha", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Threshold Alpha..."), NULL, NULL,
    "ligma:threshold-alpha",
    LIGMA_HELP_FILTER_THRESHOLD_ALPHA },

  { "filters-tile-glass", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Glass Tile..."), NULL, NULL,
    "gegl:tile-glass",
    LIGMA_HELP_FILTER_TILE_GLASS },

  { "filters-tile-paper", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Paper Tile..."), NULL, NULL,
    "gegl:tile-paper",
    LIGMA_HELP_FILTER_TILE_PAPER },

  { "filters-tile-seamless", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Tile Seamless..."), NULL, NULL,
    "gegl:tile-seamless",
    LIGMA_HELP_FILTER_TILE_SEAMLESS },

  { "filters-unsharp-mask", LIGMA_ICON_GEGL,
    NC_("filters-action", "Sharpen (_Unsharp Mask)..."), NULL, NULL,
    "gegl:unsharp-mask",
    LIGMA_HELP_FILTER_UNSHARP_MASK },

  { "filters-value-propagate", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Value Propagate..."), NULL, NULL,
    "gegl:value-propagate",
    LIGMA_HELP_FILTER_VALUE_PROPAGATE },

  { "filters-variable-blur", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Variable Blur..."), NULL, NULL,
    "gegl:variable-blur",
    LIGMA_HELP_FILTER_VARIABLE_BLUR },

  { "filters-video-degradation", LIGMA_ICON_GEGL,
    NC_("filters-action", "Vi_deo Degradation..."), NULL, NULL,
    "gegl:video-degradation",
    LIGMA_HELP_FILTER_VIDEO_DEGRADATION },

  { "filters-vignette", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Vignette..."), NULL, NULL,
    "gegl:vignette",
    LIGMA_HELP_FILTER_VIGNETTE },

  { "filters-waterpixels", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Waterpixels..."), NULL, NULL,
    "gegl:waterpixels",
    LIGMA_HELP_FILTER_WATERPIXELS },

  { "filters-waves", LIGMA_ICON_GEGL,
    NC_("filters-action", "_Waves..."), NULL, NULL,
    "gegl:waves",
    LIGMA_HELP_FILTER_WAVES },

  { "filters-whirl-pinch", LIGMA_ICON_GEGL,
    NC_("filters-action", "W_hirl and Pinch..."), NULL, NULL,
    "gegl:whirl-pinch",
    LIGMA_HELP_FILTER_WHIRL_PINCH },

  { "filters-wind", LIGMA_ICON_GEGL,
    NC_("filters-action", "W_ind..."), NULL, NULL,
    "gegl:wind",
    LIGMA_HELP_FILTER_WIND }
};

static const LigmaEnumActionEntry filters_repeat_actions[] =
{
  { "filters-repeat", LIGMA_ICON_SYSTEM_RUN,
    NC_("filters-action", "Re_peat Last"), "<primary>F",
    NC_("filters-action",
        "Rerun the last used filter using the same settings"),
    LIGMA_RUN_WITH_LAST_VALS, FALSE,
    LIGMA_HELP_FILTER_REPEAT },

  { "filters-reshow", LIGMA_ICON_DIALOG_RESHOW_FILTER,
    NC_("filters-action", "R_e-Show Last"), "<primary><shift>F",
    NC_("filters-action", "Show the last used filter dialog again"),
    LIGMA_RUN_INTERACTIVE, FALSE,
    LIGMA_HELP_FILTER_RESHOW }
};


void
filters_actions_setup (LigmaActionGroup *group)
{
  LigmaProcedureActionEntry *entries;
  gint                      n_entries;
  gint                      i;

  ligma_action_group_add_actions (group, "filters-action",
                                 filters_menu_actions,
                                 G_N_ELEMENTS (filters_menu_actions));

  ligma_action_group_add_string_actions (group, "filters-action",
                                        filters_actions,
                                        G_N_ELEMENTS (filters_actions),
                                        filters_apply_cmd_callback);
  filters_actions_set_tooltips (group, filters_actions,
                                G_N_ELEMENTS (filters_actions));

  ligma_action_group_add_string_actions (group, "filters-action",
                                        filters_settings_actions,
                                        G_N_ELEMENTS (filters_settings_actions),
                                        filters_apply_cmd_callback);
  filters_actions_set_tooltips (group, filters_settings_actions,
                                G_N_ELEMENTS (filters_settings_actions));

  ligma_action_group_add_string_actions (group, "filters-action",
                                        filters_interactive_actions,
                                        G_N_ELEMENTS (filters_interactive_actions),
                                        filters_apply_interactive_cmd_callback);
  filters_actions_set_tooltips (group, filters_interactive_actions,
                                G_N_ELEMENTS (filters_interactive_actions));

  ligma_action_group_add_enum_actions (group, "filters-action",
                                      filters_repeat_actions,
                                      G_N_ELEMENTS (filters_repeat_actions),
                                      filters_repeat_cmd_callback);

  n_entries = ligma_filter_history_size (group->ligma);

  entries = g_new0 (LigmaProcedureActionEntry, n_entries);

  for (i = 0; i < n_entries; i++)
    {
      entries[i].name        = g_strdup_printf ("filters-recent-%02d", i + 1);
      entries[i].icon_name   = NULL;
      entries[i].label       = "";
      entries[i].accelerator = "";
      entries[i].tooltip     = NULL;
      entries[i].procedure   = NULL;
      entries[i].help_id     = LIGMA_HELP_FILTER_RESHOW;
    }

  ligma_action_group_add_procedure_actions (group, entries, n_entries,
                                           filters_history_cmd_callback);

  for (i = 0; i < n_entries; i++)
    {
      ligma_action_group_set_action_visible (group, entries[i].name, FALSE);
      g_free ((gchar *) entries[i].name);
    }

  g_free (entries);

  g_signal_connect_object (group->ligma, "filter-history-changed",
                           G_CALLBACK (filters_actions_history_changed),
                           group, 0);

  filters_actions_history_changed (group->ligma, group);
}

void
filters_actions_update (LigmaActionGroup *group,
                        gpointer         data)
{
  LigmaImage    *image;
  gboolean      writable       = FALSE;
  gboolean      gray           = FALSE;
  gboolean      alpha          = FALSE;
  gboolean      supports_alpha = FALSE;

  image = action_data_get_image (data);

  if (image)
    {
      GList *drawables;

      drawables = ligma_image_get_selected_drawables (image);

      if (g_list_length (drawables) == 1)
        {
          LigmaDrawable *drawable = drawables->data;
          LigmaItem     *item;

          gray           = ligma_drawable_is_gray (drawable);
          alpha          = ligma_drawable_has_alpha (drawable);
          supports_alpha = ligma_drawable_supports_alpha (drawable);

          if (LIGMA_IS_LAYER_MASK (drawable))
            item = LIGMA_ITEM (ligma_layer_mask_get_layer (LIGMA_LAYER_MASK (drawable)));
          else
            item = LIGMA_ITEM (drawable);

          writable = ! ligma_item_is_content_locked (item, NULL);

          if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable)))
            writable = FALSE;
        }

      g_list_free (drawables);
   }

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("filters-alien-map",               writable);
  SET_SENSITIVE ("filters-antialias",               writable);
  SET_SENSITIVE ("filters-apply-canvas",            writable);
  SET_SENSITIVE ("filters-apply-lens",              writable);
  SET_SENSITIVE ("filters-bayer-matrix",            writable);
  SET_SENSITIVE ("filters-bloom",                   writable);
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
  SET_SENSITIVE ("filters-focus-blur",              writable);
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
  SET_SENSITIVE ("filters-lens-blur",               writable);
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
  SET_SENSITIVE ("filters-variable-blur",           writable);
  SET_SENSITIVE ("filters-video-degradation",       writable);
  SET_SENSITIVE ("filters-vignette",                writable);
  SET_SENSITIVE ("filters-waterpixels",             writable);
  SET_SENSITIVE ("filters-waves",                   writable);
  SET_SENSITIVE ("filters-whirl-pinch",             writable);
  SET_SENSITIVE ("filters-wind",                    writable);

#undef SET_SENSITIVE

  {
    LigmaProcedure *proc   = ligma_filter_history_nth (group->ligma, 0);
    const gchar   *reason = NULL;
    gint           i;

    if (proc &&
        ligma_procedure_get_sensitive (proc, LIGMA_OBJECT (image), &reason))
      {
        ligma_action_group_set_action_sensitive (group, "filters-repeat", TRUE, NULL);
        ligma_action_group_set_action_sensitive (group, "filters-reshow", TRUE, NULL);
      }
    else
      {
        ligma_action_group_set_action_sensitive (group, "filters-repeat", FALSE, reason);
        ligma_action_group_set_action_sensitive (group, "filters-reshow", FALSE, reason);
     }

    for (i = 0; i < ligma_filter_history_length (group->ligma); i++)
      {
        gchar    *name = g_strdup_printf ("filters-recent-%02d", i + 1);
        gboolean  sensitive;

        proc = ligma_filter_history_nth (group->ligma, i);

        reason = NULL;
        sensitive = ligma_procedure_get_sensitive (proc, LIGMA_OBJECT (image),
                                                  &reason);

        ligma_action_group_set_action_sensitive (group, name, sensitive, reason);

        g_free (name);
      }
  }
}

static void
filters_actions_set_tooltips (LigmaActionGroup             *group,
                              const LigmaStringActionEntry *entries,
                              gint                         n_entries)
{
  gint i;

  for (i = 0; i < n_entries; i++)
    {
      const LigmaStringActionEntry *entry = entries + i;
      const gchar                 *description;

      description = gegl_operation_get_key (entry->value, "description");

      if (description)
        ligma_action_group_set_action_tooltip (group, entry->name,
                                              description);
    }
}

static LigmaActionGroup *
filters_actions_get_plug_in_group (LigmaActionGroup *group)
{
  GList *list;

  for (list = ligma_ui_managers_from_name ("<Image>");
       list;
       list = g_list_next (list))
    {
      LigmaUIManager *manager = list->data;

      /* if this is our UI manager */
      if (ligma_ui_manager_get_action_group (manager, "filters") == group)
        return ligma_ui_manager_get_action_group (manager, "plug-in");
    }

  /* this happens during initial UI manager construction */
  return NULL;
}

static void
filters_actions_history_changed (Ligma            *ligma,
                                 LigmaActionGroup *group)
{
  LigmaProcedure   *proc;
  LigmaActionGroup *plug_in_group;
  gint             i;

  plug_in_group = filters_actions_get_plug_in_group (group);

  proc = ligma_filter_history_nth (ligma, 0);

  if (proc)
    {
      LigmaAction  *actual_action = NULL;
      const gchar *label;
      gchar       *repeat;
      gchar       *reshow;
      const gchar *reason    = NULL;
      gboolean     sensitive = FALSE;

      label = ligma_procedure_get_label (proc);

      repeat = g_strdup_printf (_("Re_peat \"%s\""),  label);
      reshow = g_strdup_printf (_("R_e-Show \"%s\""), label);

      ligma_action_group_set_action_label (group, "filters-repeat", repeat);
      ligma_action_group_set_action_label (group, "filters-reshow", reshow);

      g_free (repeat);
      g_free (reshow);

      if (g_str_has_prefix (ligma_object_get_name (proc), "filters-"))
        {
          actual_action =
            ligma_action_group_get_action (group,
                                          ligma_object_get_name (proc));
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
            ligma_action_group_get_action (plug_in_group,
                                          ligma_object_get_name (proc));
        }

      if (actual_action)
        sensitive = ligma_action_get_sensitive (actual_action, &reason);

      ligma_action_group_set_action_sensitive (group, "filters-repeat",
                                              sensitive, reason);
      ligma_action_group_set_action_sensitive (group, "filters-reshow",
                                              sensitive, reason);
   }
  else
    {
      ligma_action_group_set_action_label (group, "filters-repeat",
                                          _("Repeat Last"));
      ligma_action_group_set_action_label (group, "filters-reshow",
                                          _("Re-Show Last"));

      ligma_action_group_set_action_sensitive (group, "filters-repeat",
                                              FALSE, _("No last used filters"));
      ligma_action_group_set_action_sensitive (group, "filters-reshow",
                                              FALSE, _("No last used filters"));
    }

  for (i = 0; i < ligma_filter_history_length (ligma); i++)
    {
      LigmaAction  *action;
      LigmaAction  *actual_action = NULL;
      const gchar *label;
      gchar       *name;
      gboolean     sensitive = FALSE;

      name = g_strdup_printf ("filters-recent-%02d", i + 1);
      action = ligma_action_group_get_action (group, name);
      g_free (name);

      proc = ligma_filter_history_nth (ligma, i);

      label = ligma_procedure_get_menu_label (proc);

      if (g_str_has_prefix (ligma_object_get_name (proc), "filters-"))
        {
          actual_action =
            ligma_action_group_get_action (group,
                                          ligma_object_get_name (proc));
        }
      else if (plug_in_group)
        {
          /*  see comment above  */
          actual_action =
            ligma_action_group_get_action (plug_in_group,
                                          ligma_object_get_name (proc));
        }

      if (actual_action)
        sensitive = ligma_action_get_sensitive (actual_action, NULL);

      g_object_set (action,
                    "visible",   TRUE,
                    "sensitive", sensitive,
                    "procedure", proc,
                    "label",     label,
                    "icon-name", ligma_viewable_get_icon_name (LIGMA_VIEWABLE (proc)),
                    "tooltip",   ligma_procedure_get_blurb (proc),
                    NULL);
    }

  for (; i < ligma_filter_history_size (ligma); i++)
    {
      LigmaAction *action;
      gchar      *name = g_strdup_printf ("filters-recent-%02d", i + 1);

      action = ligma_action_group_get_action (group, name);
      g_free (name);

      g_object_set (action,
                    "visible",   FALSE,
                    "procedure", NULL,
                    NULL);
    }
}
