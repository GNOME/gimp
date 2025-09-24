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
#include <gegl-plugin.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp-filter-history.h"
#include "core/gimpimage.h"
#include "core/gimpgrouplayer.h"
#include "core/gimplinklayer.h"
#include "core/gimplayermask.h"

#include "path/gimpvectorlayer.h"

#include "pdb/gimpprocedure.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpstringaction.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "actions.h"
#include "filters-actions.h"
#include "filters-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     filters_actions_set_tooltips    (GimpActionGroup             *group,
                                                 const GimpStringActionEntry *entries,
                                                 gint                         n_entries);
static void     filters_actions_history_changed (Gimp                        *gimp,
                                                 GimpActionGroup             *group);
static gboolean filters_is_non_interactive      (const gchar                 *action_name);


/*  private variables  */

static const GimpStringActionEntry filters_actions[] =
{
  { "filters-antialias", GIMP_ICON_GEGL,
    NC_("filters-action", "_Antialias"), NULL, { NULL }, NULL,
    "gegl:antialias",
    GIMP_HELP_FILTER_ANTIALIAS },

  { "filters-color-enhance", GIMP_ICON_GEGL,
    NC_("filters-action", "_Color Enhance"), NULL, { NULL }, NULL,
    "gegl:color-enhance",
    GIMP_HELP_FILTER_COLOR_ENHANCE },

  { "filters-invert-linear", GIMP_ICON_INVERT,
    NC_("filters-action", "L_inear Invert"), NULL, { NULL }, NULL,
    "gegl:invert-linear",
    GIMP_HELP_FILTER_INVERT_LINEAR },

  { "filters-invert-perceptual", GIMP_ICON_INVERT,
    NC_("filters-action", "In_vert"), NULL, { NULL }, NULL,
    "gegl:invert-gamma",
    GIMP_HELP_FILTER_INVERT_PERCEPTUAL },

  { "filters-invert-value", GIMP_ICON_INVERT,
    NC_("filters-action", "_Value Invert"), NULL, { NULL }, NULL,
    "gegl:value-invert",
    GIMP_HELP_FILTER_INVERT_VALUE },

  { "filters-stretch-contrast-hsv", GIMP_ICON_GEGL,
    NC_("filters-action", "_Stretch Contrast HSV"), NULL, { NULL }, NULL,
    "gegl:stretch-contrast-hsv",
    GIMP_HELP_FILTER_STRETCH_CONTRAST_HSV }
};

static const GimpStringActionEntry filters_settings_actions[] =
{
  { "filters-dilate", GIMP_ICON_GEGL,
    NC_("filters-action", "_Dilate"), NULL, { NULL },
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
    NC_("filters-action", "_Erode"), NULL, { NULL },
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
    NC_("filters-action", "_Alien Map..."), NULL, { NULL }, NULL,
    "gegl:alien-map",
    GIMP_HELP_FILTER_ALIEN_MAP },

  { "filters-apply-canvas", GIMP_ICON_GEGL,
    NC_("filters-action", "_Apply Canvas..."), NULL, { NULL }, NULL,
    "gegl:texturize-canvas",
    GIMP_HELP_FILTER_APPLY_CANVAS },

  { "filters-apply-lens", GIMP_ICON_GEGL,
    NC_("filters-action", "Apply _Lens..."), NULL, { NULL }, NULL,
    "gegl:apply-lens",
    GIMP_HELP_FILTER_APPLY_LENS },

  { "filters-bayer-matrix", GIMP_ICON_GEGL,
    NC_("filters-action", "_Bayer Matrix..."), NULL, { NULL }, NULL,
    "gegl:bayer-matrix",
    GIMP_HELP_FILTER_BAYER_MATRIX },

  { "filters-bloom", GIMP_ICON_GEGL,
    NC_("filters-action", "_Bloom..."), NULL, { NULL }, NULL,
    "gegl:bloom",
    GIMP_HELP_FILTER_BLOOM },

  { "filters-brightness-contrast", GIMP_ICON_TOOL_BRIGHTNESS_CONTRAST,
    NC_("filters-action", "B_rightness-Contrast..."), NULL, { NULL }, NULL,
    "gimp:brightness-contrast",
    GIMP_HELP_TOOL_BRIGHTNESS_CONTRAST },

  { "filters-bump-map", GIMP_ICON_GEGL,
    NC_("filters-action", "_Bump Map..."), NULL, { NULL }, NULL,
    "gegl:bump-map",
    GIMP_HELP_FILTER_BUMP_MAP },

  { "filters-c2g", GIMP_ICON_GEGL,
    NC_("filters-action", "_Color to Gray..."), NULL, { NULL }, NULL,
    "gegl:c2g",
    GIMP_HELP_FILTER_C2G },

  { "filters-cartoon", GIMP_ICON_GEGL,
    NC_("filters-action", "Ca_rtoon..."), NULL, { NULL }, NULL,
    "gegl:cartoon",
    GIMP_HELP_FILTER_CARTOON },

  { "filters-channel-mixer", GIMP_ICON_GEGL,
    NC_("filters-action", "_Channel Mixer..."), NULL, { NULL }, NULL,
    "gegl:channel-mixer",
    GIMP_HELP_FILTER_CHANNEL_MIXER },

  { "filters-checkerboard", GIMP_ICON_GEGL,
    NC_("filters-action", "_Checkerboard..."), NULL, { NULL }, NULL,
    "gegl:checkerboard",
    GIMP_HELP_FILTER_CHECKERBOARD },

  { "filters-color-balance", GIMP_ICON_TOOL_COLOR_BALANCE,
    NC_("filters-action", "Color _Balance..."), NULL, { NULL }, NULL,
    "gimp:color-balance",
    GIMP_HELP_TOOL_COLOR_BALANCE },

  { "filters-color-exchange", GIMP_ICON_GEGL,
    NC_("filters-action", "_Color Exchange..."), NULL, { NULL }, NULL,
    "gegl:color-exchange",
    GIMP_HELP_FILTER_COLOR_EXCHANGE },

  { "filters-colorize", GIMP_ICON_TOOL_COLORIZE,
    NC_("filters-action", "Colori_ze..."), NULL, { NULL }, NULL,
    "gimp:colorize",
    GIMP_HELP_TOOL_COLORIZE },

  { "filters-dither", GIMP_ICON_GEGL,
    NC_("filters-action", "Dithe_r..."), NULL, { NULL }, NULL,
    "gegl:dither",
    GIMP_HELP_FILTER_DITHER },

  { "filters-color-rotate", GIMP_ICON_GEGL,
    NC_("filters-action", "_Rotate Colors..."), NULL, { NULL }, NULL,
    "gegl:color-rotate",
    GIMP_HELP_FILTER_COLOR_ROTATE },

  { "filters-color-temperature", GIMP_ICON_TOOL_COLOR_TEMPERATURE,
    NC_("filters-action", "Color T_emperature..."), NULL, { NULL }, NULL,
    "gegl:color-temperature",
    GIMP_HELP_FILTER_COLOR_TEMPERATURE },

  { "filters-color-to-alpha", GIMP_ICON_GEGL,
    NC_("filters-action", "Color to _Alpha..."), NULL, { NULL }, NULL,
    "gegl:color-to-alpha",
    GIMP_HELP_FILTER_COLOR_TO_ALPHA },

  { "filters-component-extract", GIMP_ICON_GEGL,
    NC_("filters-action", "_Extract Component..."), NULL, { NULL }, NULL,
    "gegl:component-extract",
    GIMP_HELP_FILTER_COMPONENT_EXTRACT },

  { "filters-convolution-matrix", GIMP_ICON_GEGL,
    NC_("filters-action", "_Convolution Matrix..."), NULL, { NULL }, NULL,
    "gegl:convolution-matrix",
    GIMP_HELP_FILTER_CONVOLUTION_MATRIX },

  { "filters-cubism", GIMP_ICON_GEGL,
    NC_("filters-action", "_Cubism..."), NULL, { NULL }, NULL,
    "gegl:cubism",
    GIMP_HELP_FILTER_CUBISM },

  { "filters-curves", GIMP_ICON_TOOL_CURVES,
    NC_("filters-action", "_Curves..."), NULL, { NULL }, NULL,
    "gimp:curves",
    GIMP_HELP_TOOL_CURVES },

  { "filters-deinterlace", GIMP_ICON_GEGL,
    NC_("filters-action", "_Deinterlace..."), NULL, { NULL }, NULL,
    "gegl:deinterlace",
    GIMP_HELP_FILTER_DEINTERLACE },

  { "filters-desaturate", GIMP_ICON_TOOL_DESATURATE,
    NC_("filters-action", "_Desaturate..."), NULL, { NULL }, NULL,
    "gimp:desaturate",
    GIMP_HELP_FILTER_DESATURATE },

  { "filters-difference-of-gaussians", GIMP_ICON_GEGL,
    NC_("filters-action", "Difference of _Gaussians..."), NULL, { NULL }, NULL,
    "gegl:difference-of-gaussians",
    GIMP_HELP_FILTER_DIFFERENCE_OF_GAUSSIANS },

  { "filters-diffraction-patterns", GIMP_ICON_GEGL,
    NC_("filters-action", "D_iffraction Patterns..."), NULL, { NULL }, NULL,
    "gegl:diffraction-patterns",
    GIMP_HELP_FILTER_DIFFRACTION_PATTERNS },

  { "filters-displace", GIMP_ICON_GEGL,
    NC_("filters-action", "_Displace..."), NULL, { NULL }, NULL,
    "gegl:displace",
    GIMP_HELP_FILTER_DISPLACE },

  { "filters-distance-map", GIMP_ICON_GEGL,
    NC_("filters-action", "Distance _Map..."), NULL, { NULL }, NULL,
    "gegl:distance-transform",
    GIMP_HELP_FILTER_DISTANCE_MAP },

  { "filters-dropshadow", GIMP_ICON_GEGL,
    NC_("filters-action", "_Drop Shadow..."), NULL, { NULL }, NULL,
    "gegl:dropshadow",
    GIMP_HELP_FILTER_DROPSHADOW },

  { "filters-edge", GIMP_ICON_GEGL,
    NC_("filters-action", "_Edge..."), NULL, { NULL }, NULL,
    "gegl:edge",
    GIMP_HELP_FILTER_EDGE },

  { "filters-edge-laplace", GIMP_ICON_GEGL,
    NC_("filters-action", "_Laplace..."), NULL, { NULL }, NULL,
    "gegl:edge-laplace",
    GIMP_HELP_FILTER_EDGE_LAPLACE },

  { "filters-edge-neon", GIMP_ICON_GEGL,
    NC_("filters-action", "_Neon..."), NULL, { NULL }, NULL,
    "gegl:edge-neon",
    GIMP_HELP_FILTER_EDGE_NEON },

  { "filters-edge-sobel", GIMP_ICON_GEGL,
    NC_("filters-action", "_Sobel..."), NULL, { NULL }, NULL,
    "gegl:edge-sobel",
    GIMP_HELP_FILTER_EDGE_SOBEL },

  { "filters-emboss", GIMP_ICON_GEGL,
    NC_("filters-action", "_Emboss..."), NULL, { NULL }, NULL,
    "gegl:emboss",
    GIMP_HELP_FILTER_EMBOSS },

  { "filters-engrave", GIMP_ICON_GEGL,
    NC_("filters-action", "En_grave..."), NULL, { NULL }, NULL,
    "gegl:engrave",
    GIMP_HELP_FILTER_ENGRAVE },

  { "filters-exposure", GIMP_ICON_TOOL_EXPOSURE,
    NC_("filters-action", "E_xposure..."), NULL, { NULL }, NULL,
    "gegl:exposure",
    GIMP_HELP_FILTER_EXPOSURE },

  { "filters-fattal-2002", GIMP_ICON_GEGL,
    NC_("filters-action", "_Fattal et al. 2002..."), NULL, { NULL }, NULL,
    "gegl:fattal02",
    GIMP_HELP_FILTER_FATTAL_2002 },

  { "filters-focus-blur", GIMP_ICON_GEGL,
    NC_("filters-action", "_Focus Blur..."), NULL, { NULL }, NULL,
    "gegl:focus-blur",
    GIMP_HELP_FILTER_FOCUS_BLUR },

  { "filters-fractal-trace", GIMP_ICON_GEGL,
    NC_("filters-action", "_Fractal Trace..."), NULL, { NULL }, NULL,
    "gegl:fractal-trace",
    GIMP_HELP_FILTER_FRACTAL_TRACE },

  { "filters-gaussian-blur", GIMP_ICON_GEGL,
    NC_("filters-action", "_Gaussian Blur..."), NULL, { NULL }, NULL,
    "gegl:gaussian-blur",
    GIMP_HELP_FILTER_GAUSSIAN_BLUR },

  { "filters-gaussian-blur-selective", GIMP_ICON_GEGL,
    NC_("filters-action", "_Selective Gaussian Blur..."), NULL, { NULL }, NULL,
    "gegl:gaussian-blur-selective",
    GIMP_HELP_FILTER_GAUSSIAN_BLUR_SELECTIVE },

  { "filters-gegl-graph", GIMP_ICON_GEGL,
    NC_("filters-action", "_GEGL Graph..."), NULL, { NULL }, NULL,
    "gegl:gegl",
    GIMP_HELP_FILTER_GEGL_GRAPH },

  { "filters-grid", GIMP_ICON_GRID,
    NC_("filters-action", "_Grid..."), NULL, { NULL }, NULL,
    "gegl:grid",
    GIMP_HELP_FILTER_GRID },

  { "filters-high-pass", GIMP_ICON_GEGL,
    NC_("filters-action", "_High Pass..."), NULL, { NULL }, NULL,
    "gegl:high-pass",
    GIMP_HELP_FILTER_HIGH_PASS },

  { "filters-hue-chroma", GIMP_ICON_GEGL,
    NC_("filters-action", "Hue-_Chroma..."), NULL, { NULL }, NULL,
    "gegl:hue-chroma",
    GIMP_HELP_FILTER_HUE_CHROMA },

  { "filters-hue-saturation", GIMP_ICON_TOOL_HUE_SATURATION,
    NC_("filters-action", "Hue-_Saturation..."), NULL, { NULL }, NULL,
    "gimp:hue-saturation",
    GIMP_HELP_TOOL_HUE_SATURATION },

  { "filters-illusion", GIMP_ICON_GEGL,
    NC_("filters-action", "_Illusion..."), NULL, { NULL }, NULL,
    "gegl:illusion",
    GIMP_HELP_FILTER_ILLUSION },

  { "filters-image-gradient", GIMP_ICON_GEGL,
    NC_("filters-action", "_Image Gradient..."), NULL, { NULL }, NULL,
    "gegl:image-gradient",
    GIMP_HELP_FILTER_IMAGE_GRADIENT },

  { "filters-kaleidoscope", GIMP_ICON_GEGL,
    NC_("filters-action", "_Kaleidoscope..."), NULL, { NULL }, NULL,
    "gegl:mirrors",
    GIMP_HELP_FILTER_KALEIDOSCOPE },

  { "filters-lens-blur", GIMP_ICON_GEGL,
    NC_("filters-action", "Le_ns Blur..."), NULL, { NULL }, NULL,
    "gegl:lens-blur",
    GIMP_HELP_FILTER_LENS_BLUR },

  { "filters-lens-distortion", GIMP_ICON_GEGL,
    NC_("filters-action", "Le_ns Distortion..."), NULL, { NULL }, NULL,
    "gegl:lens-distortion",
    GIMP_HELP_FILTER_LENS_DISTORTION },

  { "filters-lens-flare", GIMP_ICON_GEGL,
    NC_("filters-action", "Lens _Flare..."), NULL, { NULL }, NULL,
    "gegl:lens-flare",
    GIMP_HELP_FILTER_LENS_FLARE },

  { "filters-levels", GIMP_ICON_TOOL_LEVELS,
    NC_("filters-action", "_Levels..."), NULL, { NULL }, NULL,
    "gimp:levels",
    GIMP_HELP_TOOL_LEVELS },

  { "filters-linear-sinusoid", GIMP_ICON_TOOL_LEVELS,
    NC_("filters-action", "_Linear Sinusoid..."), NULL, { NULL }, NULL,
    "gegl:linear-sinusoid",
    GIMP_HELP_FILTER_LINEAR_SINUSOID },

  { "filters-little-planet", GIMP_ICON_GEGL,
    NC_("filters-action", "_Little Planet..."), NULL, { NULL }, NULL,
    "gegl:stereographic-projection",
    GIMP_HELP_FILTER_LITTLE_PLANET },

  { "filters-long-shadow", GIMP_ICON_GEGL,
    NC_("filters-action", "_Long Shadow..."), NULL, { NULL }, NULL,
    "gegl:long-shadow",
    GIMP_HELP_FILTER_LONG_SHADOW },

  { "filters-mantiuk-2006", GIMP_ICON_GEGL,
    NC_("filters-action", "_Mantiuk 2006..."), NULL, { NULL }, NULL,
    "gegl:mantiuk06",
    GIMP_HELP_FILTER_MANTIUK_2006 },

  { "filters-maze", GIMP_ICON_GEGL,
    NC_("filters-action", "_Maze..."), NULL, { NULL }, NULL,
    "gegl:maze",
    GIMP_HELP_FILTER_MAZE },

  { "filters-mean-curvature-blur", GIMP_ICON_GEGL,
    NC_("filters-action", "Mean C_urvature Blur..."), NULL, { NULL }, NULL,
    "gegl:mean-curvature-blur",
    GIMP_HELP_FILTER_MEAN_CURVATURE_BLUR },

  { "filters-median-blur", GIMP_ICON_GEGL,
    NC_("filters-action", "_Median Blur..."), NULL, { NULL }, NULL,
    "gegl:median-blur",
    GIMP_HELP_FILTER_MEDIAN_BLUR },

  { "filters-mono-mixer", GIMP_ICON_GEGL,
    NC_("filters-action", "_Mono Mixer..."), NULL, { NULL }, NULL,
    "gegl:mono-mixer",
    GIMP_HELP_FILTER_MONO_MIXER },

  { "filters-mosaic", GIMP_ICON_GEGL,
    NC_("filters-action", "_Mosaic..."), NULL, { NULL }, NULL,
    "gegl:mosaic",
    GIMP_HELP_FILTER_MOSAIC },

  { "filters-motion-blur-circular", GIMP_ICON_GEGL,
    NC_("filters-action", "_Circular Motion Blur..."), NULL, { NULL }, NULL,
    "gegl:motion-blur-circular",
    GIMP_HELP_FILTER_MOTION_BLUR_CIRCULAR },

  { "filters-motion-blur-linear", GIMP_ICON_GEGL,
    NC_("filters-action", "_Linear Motion Blur..."), NULL, { NULL }, NULL,
    "gegl:motion-blur-linear",
    GIMP_HELP_FILTER_MOTION_BLUR_LINEAR },

  { "filters-motion-blur-zoom", GIMP_ICON_GEGL,
    NC_("filters-action", "_Zoom Motion Blur..."), NULL, { NULL }, NULL,
    "gegl:motion-blur-zoom",
    GIMP_HELP_FILTER_MOTION_BLUR_ZOOM },

  { "filters-noise-cell", GIMP_ICON_GEGL,
    NC_("filters-action", "_Cell Noise..."), NULL, { NULL }, NULL,
    "gegl:cell-noise",
    GIMP_HELP_FILTER_NOISE_CELL },

  { "filters-newsprint", GIMP_ICON_GEGL,
    NC_("filters-action", "_Newsprint..."), NULL, { NULL }, NULL,
    "gegl:newsprint",
    GIMP_HELP_FILTER_NEWSPRINT },

  { "filters-noise-cie-lch", GIMP_ICON_GEGL,
    NC_("filters-action", "_CIE lch Noise..."), NULL, { NULL }, NULL,
    "gegl:noise-cie-lch",
    GIMP_HELP_FILTER_NOISE_CIE_LCH },

  { "filters-noise-hsv", GIMP_ICON_GEGL,
    NC_("filters-action", "HS_V Noise..."), NULL, { NULL }, NULL,
    "gegl:noise-hsv",
    GIMP_HELP_FILTER_NOISE_HSV },

  { "filters-noise-hurl", GIMP_ICON_GEGL,
    NC_("filters-action", "_Hurl..."), NULL, { NULL }, NULL,
    "gegl:noise-hurl",
    GIMP_HELP_FILTER_NOISE_HURL },

  { "filters-noise-perlin", GIMP_ICON_GEGL,
    NC_("filters-action", "Perlin _Noise..."), NULL, { NULL }, NULL,
    "gegl:perlin-noise",
    GIMP_HELP_FILTER_NOISE_PERLIN },

  { "filters-noise-pick", GIMP_ICON_GEGL,
    NC_("filters-action", "_Pick..."), NULL, { NULL }, NULL,
    "gegl:noise-pick",
    GIMP_HELP_FILTER_NOISE_PICK },

  { "filters-noise-rgb", GIMP_ICON_GEGL,
    NC_("filters-action", "_RGB Noise..."), NULL, { NULL }, NULL,
    "gegl:noise-rgb",
    GIMP_HELP_FILTER_NOISE_RGB },

  { "filters-noise-reduction", GIMP_ICON_GEGL,
    NC_("filters-action", "Noise R_eduction..."), NULL, { NULL }, NULL,
    "gegl:noise-reduction",
    GIMP_HELP_FILTER_NOISE_REDUCTION },

  { "filters-noise-simplex", GIMP_ICON_GEGL,
    NC_("filters-action", "_Simplex Noise..."), NULL, { NULL }, NULL,
    "gegl:simplex-noise",
    GIMP_HELP_FILTER_NOISE_SIMPLEX },

  { "filters-noise-slur", GIMP_ICON_GEGL,
    NC_("filters-action", "_Slur..."), NULL, { NULL }, NULL,
    "gegl:noise-slur",
    GIMP_HELP_FILTER_NOISE_SLUR },

  { "filters-noise-solid", GIMP_ICON_GEGL,
    NC_("filters-action", "_Solid Noise..."), NULL, { NULL }, NULL,
    "gegl:noise-solid",
    GIMP_HELP_FILTER_NOISE_SOLID },

  { "filters-noise-spread", GIMP_ICON_GEGL,
    NC_("filters-action", "Sp_read..."), NULL, { NULL }, NULL,
    "gegl:noise-spread",
    GIMP_HELP_FILTER_NOISE_SPREAD },

  { "filters-normal-map", GIMP_ICON_GEGL,
    NC_("filters-action", "_Normal Map..."), NULL, { NULL }, NULL,
    "gegl:normal-map",
    GIMP_HELP_FILTER_NORMAL_MAP },

  { "filters-offset", GIMP_ICON_TOOL_OFFSET,
    NC_("filters-action", "_Offset..."), NULL, { "<primary><shift>O", NULL }, NULL,
    "gimp:offset",
    GIMP_HELP_TOOL_OFFSET },

  { "filters-oilify", GIMP_ICON_GEGL,
    NC_("filters-action", "Oili_fy..."), NULL, { NULL }, NULL,
    "gegl:oilify",
    GIMP_HELP_FILTER_OILIFY },

  { "filters-panorama-projection", GIMP_ICON_GEGL,
    NC_("filters-action", "_Panorama Projection..."), NULL, { NULL }, NULL,
    "gegl:panorama-projection",
    GIMP_HELP_FILTER_PANORAMA_PROJECTION },

  { "filters-photocopy", GIMP_ICON_GEGL,
    NC_("filters-action", "_Photocopy..."), NULL, { NULL }, NULL,
    "gegl:photocopy",
    GIMP_HELP_FILTER_PHOTOCOPY },

  { "filters-pixelize", GIMP_ICON_GEGL,
    NC_("filters-action", "_Pixelize..."), NULL, { NULL }, NULL,
    "gegl:pixelize",
    GIMP_HELP_FILTER_PIXELIZE },

  { "filters-plasma", GIMP_ICON_GEGL,
    NC_("filters-action", "_Plasma..."), NULL, { NULL }, NULL,
    "gegl:plasma",
    GIMP_HELP_FILTER_PLASMA },

  { "filters-polar-coordinates", GIMP_ICON_GEGL,
    NC_("filters-action", "P_olar Coordinates..."), NULL, { NULL }, NULL,
    "gegl:polar-coordinates",
    GIMP_HELP_FILTER_POLAR_COORDINATES },

  { "filters-posterize", GIMP_ICON_TOOL_POSTERIZE,
    NC_("filters-action", "_Posterize..."), NULL, { NULL }, NULL,
    "gimp:posterize",
    GIMP_HELP_FILTER_POSTERIZE },

  { "filters-recursive-transform", GIMP_ICON_GEGL,
    NC_("filters-action", "_Recursive Transform..."), NULL, { NULL }, NULL,
    "gegl:recursive-transform",
    GIMP_HELP_FILTER_RECURSIVE_TRANSFORM },

  { "filters-red-eye-removal", GIMP_ICON_GEGL,
    NC_("filters-action", "_Red Eye Removal..."), NULL, { NULL }, NULL,
    "gegl:red-eye-removal",
    GIMP_HELP_FILTER_RED_EYE_REMOVAL },

  { "filters-reinhard-2005", GIMP_ICON_GEGL,
    NC_("filters-action", "_Reinhard 2005..."), NULL, { NULL }, NULL,
    "gegl:reinhard05",
    GIMP_HELP_FILTER_REINHARD_2005 },

  { "filters-rgb-clip", GIMP_ICON_GEGL,
    NC_("filters-action", "RGB _Clip..."), NULL, { NULL }, NULL,
    "gegl:rgb-clip",
    GIMP_HELP_FILTER_RGB_CLIP },

  { "filters-ripple", GIMP_ICON_GEGL,
    NC_("filters-action", "_Ripple..."), NULL, { NULL }, NULL,
    "gegl:ripple",
    GIMP_HELP_FILTER_RIPPLE },

  { "filters-saturation", GIMP_ICON_GEGL,
    NC_("filters-action", "Sat_uration..."), NULL, { NULL }, NULL,
    "gegl:saturation",
    GIMP_HELP_FILTER_SATURATION },

  { "filters-semi-flatten", GIMP_ICON_GEGL,
    NC_("filters-action", "_Semi-Flatten..."), NULL, { NULL }, NULL,
    "gimp:semi-flatten",
    GIMP_HELP_FILTER_SEMI_FLATTEN },

  { "filters-sepia", GIMP_ICON_GEGL,
    NC_("filters-action", "_Sepia..."), NULL, { NULL }, NULL,
    "gegl:sepia",
    GIMP_HELP_FILTER_SEPIA },

  { "filters-shadows-highlights", GIMP_ICON_TOOL_SHADOWS_HIGHLIGHTS,
    NC_("filters-action", "S_hadows-Highlights..."), NULL, { NULL }, NULL,
    "gegl:shadows-highlights",
    GIMP_HELP_FILTER_SHADOWS_HIGHLIGHTS },

  { "filters-shift", GIMP_ICON_GEGL,
    NC_("filters-action", "_Shift..."), NULL, { NULL }, NULL,
    "gegl:shift",
    GIMP_HELP_FILTER_SHIFT },

  { "filters-sinus", GIMP_ICON_GEGL,
    NC_("filters-action", "_Sinus..."), NULL, { NULL }, NULL,
    "gegl:sinus",
    GIMP_HELP_FILTER_SINUS },

  { "filters-slic", GIMP_ICON_GEGL,
    NC_("filters-action", "_Simple Linear Iterative Clustering..."), NULL, { NULL }, NULL,
    "gegl:slic",
    GIMP_HELP_FILTER_SLIC },

  { "filters-snn-mean", GIMP_ICON_GEGL,
    NC_("filters-action", "_Symmetric Nearest Neighbor..."), NULL, { NULL }, NULL,
    "gegl:snn-mean",
    GIMP_HELP_FILTER_SNN_MEAN },

  { "filters-softglow", GIMP_ICON_GEGL,
    NC_("filters-action", "_Softglow..."), NULL, { NULL }, NULL,
    "gegl:softglow",
    GIMP_HELP_FILTER_SOFTGLOW },

  { "filters-spherize", GIMP_ICON_GEGL,
    NC_("filters-action", "Spheri_ze..."), NULL, { NULL }, NULL,
    "gegl:spherize",
    GIMP_HELP_FILTER_SPHERIZE },

  { "filters-spiral", GIMP_ICON_GEGL,
    NC_("filters-action", "S_piral..."), NULL, { NULL }, NULL,
    "gegl:spiral",
    GIMP_HELP_FILTER_SPIRAL },

  { "filters-stretch-contrast", GIMP_ICON_GEGL,
    NC_("filters-action", "_Stretch Contrast..."), NULL, { NULL }, NULL,
    "gegl:stretch-contrast",
    GIMP_HELP_FILTER_STRETCH_CONTRAST },

  { "filters-stress", GIMP_ICON_GEGL,
    NC_("filters-action", "_Stress..."), NULL, { NULL }, NULL,
    "gegl:stress",
    GIMP_HELP_FILTER_STRESS },

  { "filters-supernova", GIMP_ICON_GEGL,
    NC_("filters-action", "Super_nova..."), NULL, { NULL }, NULL,
    "gegl:supernova",
    GIMP_HELP_FILTER_SUPERNOVA },

  { "filters-threshold", GIMP_ICON_TOOL_THRESHOLD,
    NC_("filters-action", "_Threshold..."), NULL, { NULL }, NULL,
    "gimp:threshold",
    GIMP_HELP_TOOL_THRESHOLD },

  { "filters-threshold-alpha", GIMP_ICON_GEGL,
    NC_("filters-action", "_Threshold Alpha..."), NULL, { NULL }, NULL,
    "gimp:threshold-alpha",
    GIMP_HELP_FILTER_THRESHOLD_ALPHA },

  { "filters-tile-glass", GIMP_ICON_GEGL,
    NC_("filters-action", "_Glass Tile..."), NULL, { NULL }, NULL,
    "gegl:tile-glass",
    GIMP_HELP_FILTER_TILE_GLASS },

  { "filters-tile-paper", GIMP_ICON_GEGL,
    NC_("filters-action", "_Paper Tile..."), NULL, { NULL }, NULL,
    "gegl:tile-paper",
    GIMP_HELP_FILTER_TILE_PAPER },

  { "filters-tile-seamless", GIMP_ICON_GEGL,
    NC_("filters-action", "_Tile Seamless..."), NULL, { NULL }, NULL,
    "gegl:tile-seamless",
    GIMP_HELP_FILTER_TILE_SEAMLESS },

  { "filters-unsharp-mask", GIMP_ICON_GEGL,
    NC_("filters-action", "Sharpen (_Unsharp Mask)..."), NULL, { NULL }, NULL,
    "gegl:unsharp-mask",
    GIMP_HELP_FILTER_UNSHARP_MASK },

  { "filters-value-propagate", GIMP_ICON_GEGL,
    NC_("filters-action", "_Value Propagate..."), NULL, { NULL }, NULL,
    "gegl:value-propagate",
    GIMP_HELP_FILTER_VALUE_PROPAGATE },

  { "filters-variable-blur", GIMP_ICON_GEGL,
    NC_("filters-action", "_Variable Blur..."), NULL, { NULL }, NULL,
    "gegl:variable-blur",
    GIMP_HELP_FILTER_VARIABLE_BLUR },

  { "filters-video-degradation", GIMP_ICON_GEGL,
    NC_("filters-action", "Vi_deo Degradation..."), NULL, { NULL }, NULL,
    "gegl:video-degradation",
    GIMP_HELP_FILTER_VIDEO_DEGRADATION },

  { "filters-vignette", GIMP_ICON_GEGL,
    NC_("filters-action", "_Vignette..."), NULL, { NULL }, NULL,
    "gegl:vignette",
    GIMP_HELP_FILTER_VIGNETTE },

  { "filters-waterpixels", GIMP_ICON_GEGL,
    NC_("filters-action", "_Waterpixels..."), NULL, { NULL }, NULL,
    "gegl:waterpixels",
    GIMP_HELP_FILTER_WATERPIXELS },

  { "filters-waves", GIMP_ICON_GEGL,
    NC_("filters-action", "_Waves..."), NULL, { NULL }, NULL,
    "gegl:waves",
    GIMP_HELP_FILTER_WAVES },

  { "filters-whirl-pinch", GIMP_ICON_GEGL,
    NC_("filters-action", "W_hirl and Pinch..."), NULL, { NULL }, NULL,
    "gegl:whirl-pinch",
    GIMP_HELP_FILTER_WHIRL_PINCH },

  { "filters-wind", GIMP_ICON_GEGL,
    NC_("filters-action", "W_ind..."), NULL, { NULL }, NULL,
    "gegl:wind",
    GIMP_HELP_FILTER_WIND }
};

static const GimpEnumActionEntry filters_repeat_actions[] =
{
  { "filters-repeat", GIMP_ICON_SYSTEM_RUN,
    NC_("filters-action", "Re_peat Last"), NULL, { "<primary>F", NULL },
    NC_("filters-action",
        "Rerun the last used filter using the same settings"),
    GIMP_RUN_WITH_LAST_VALS, FALSE,
    GIMP_HELP_FILTER_REPEAT },

  { "filters-reshow", GIMP_ICON_DIALOG_RESHOW_FILTER,
    NC_("filters-action", "R_e-Show Last"), NULL, { "<primary><shift>F", NULL },
    NC_("filters-action", "Show the last used filter dialog again"),
    GIMP_RUN_INTERACTIVE, FALSE,
    GIMP_HELP_FILTER_RESHOW }
};


void
filters_actions_setup (GimpActionGroup *group)
{
  static gboolean           first_setup = TRUE;
  GimpProcedureActionEntry *entries;
  gint                      n_entries;
  gint                      i;
  GList                    *op_classes;
  GList                    *iter;
  GStrvBuilder             *gegl_actions;

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

  /* XXX Hardcoded list to prevent expensive node/graph creation of a
   * well-known list of operations.
   * This whole code will disappear when we'll support filters with aux
   * input non-destructively anyway.
   */
  if (first_setup)
    {
      actions_filter_set_aux ("filters-variable-blur");
      actions_filter_set_aux ("filters-oilify");
      actions_filter_set_aux ("filters-lens-blur");
      actions_filter_set_aux ("filters-gaussian-blur-selective");
      actions_filter_set_aux ("filters-displace");
      actions_filter_set_aux ("filters-bump-map");
    }

  gegl_actions = g_strv_builder_new ();
  op_classes   = gimp_gegl_get_op_classes (TRUE);

  for (iter = op_classes; iter; iter = iter->next)
    {
      GeglOperationClass    *op_class = GEGL_OPERATION_CLASS (iter->data);
      GimpStringActionEntry  entry   = { 0, };
      gchar                 *formatted_op_name;
      gchar                 *action_name;
      const gchar           *title;
      const gchar           *op_name;
      gchar                 *label;

      if (filters_actions_gegl_op_blocklisted (op_class->name))
        continue;

      formatted_op_name = g_strdup (op_class->name);
      gimp_make_valid_action_name (formatted_op_name);
      action_name = g_strdup_printf ("filters-%s", formatted_op_name);
      i           = 2;
      while (gimp_action_group_get_action (group, action_name) != NULL)
        {
          /* In the off-chance that after formatting to a valid action name, 2
           * operations end up generating the same action name. Typically we
           * don't want a third-party operation called "my-op" to have the same
           * action name than "my_op" (which is to say that one will be
           * overridden by the other).
           */
          g_free (action_name);
          action_name = g_strdup_printf ("filters-%s-%d", formatted_op_name, i++);
        }

      title   = gegl_operation_class_get_key (op_class, "title");
      op_name = op_class->name;

      if (g_str_has_prefix (op_name, "gegl:"))
        op_name += strlen ("gegl:");

      if (title)
        label = g_strdup_printf ("%s (%s)", title, op_name);
      else
        label = g_strdup (op_name);

      entry.name      = action_name;
      entry.icon_name = GIMP_ICON_GEGL;
      entry.label     = label;
      entry.tooltip   = gegl_operation_class_get_key (op_class, "description");
      entry.value     = op_class->name;
      entry.help_id   = GIMP_HELP_TOOL_GEGL;

      if (gegl_operation_class_get_key (op_class, "gimp:menu-path") &&
          g_str_has_prefix (op_class->name, "gegl:"))
        /* We automatically create an help ID from the operation name
         * for all core GEGL operations with a menu path key.
         */
        entry.help_id = formatted_op_name;

      gimp_action_group_add_string_actions (group, "filters-action",
                                            &entry, 1,
                                            filters_apply_interactive_cmd_callback);

      if (gegl_operation_class_get_key (op_class, "gimp:menu-label"))
        {
          GimpAction *action;

          action = gimp_action_group_get_action (group, action_name);

          gimp_action_set_short_label (action,
                                       gegl_operation_class_get_key (op_class,
                                                                     "gimp:menu-label"));
        }
      else if (title)
        {
          GimpAction *action;
          /* TRANSLATORS: %s is the title of a GEGL operation (filter), after
           * which we append "..." as the standardized labelling to indicate
           * that this action raises a dialog.
           */
          gchar      *short_label = g_strdup_printf (_("%s..."), title);

          action = gimp_action_group_get_action (group, action_name);
          gimp_action_set_short_label (action, short_label);
          g_free (short_label);
        }

      /* Identify third-party filters based on operations with an
       * auxiliary pad in first setup because of slowness on Windows.
       * See #14781.
       */
      if (first_setup)
        {
          GeglNode *node = gegl_node_new ();

          gegl_node_set (node,
                         "operation", op_class->name,
                         NULL);

          if (gegl_node_has_pad (node, "aux"))
            actions_filter_set_aux (action_name);

          g_clear_object (&node);
        }
      g_strv_builder_add (gegl_actions, action_name);

      g_free (label);
      g_free (action_name);
      g_free (formatted_op_name);
    }

  g_object_set_data_full (G_OBJECT (group),
                          "filters-group-generated-gegl-actions",
                          g_strv_builder_end (gegl_actions),
                          (GDestroyNotify) g_strfreev);
  g_strv_builder_unref (gegl_actions);
  g_list_free (op_classes);

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

  first_setup = FALSE;
}

void
filters_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpImage    *image;
  GList        *actions;
  GList        *iter;
  gboolean      writable       = FALSE;
  gboolean      gray           = FALSE;
  gboolean      alpha          = FALSE;
  gboolean      supports_alpha = FALSE;
  gboolean      is_group       = FALSE;
  gboolean      force_nde      = FALSE;

  image = action_data_get_image (data);

  if (image)
    {
      GList *drawables;

      drawables = gimp_image_get_selected_drawables (image);

      if (g_list_length (drawables) == 1)
        {
          GimpDrawable *drawable = drawables->data;
          GimpItem     *item;

          gray           = gimp_drawable_is_gray (drawable);
          alpha          = gimp_drawable_has_alpha (drawable);
          supports_alpha = gimp_drawable_supports_alpha (drawable);

          if (GIMP_IS_LAYER_MASK (drawable))
            item = GIMP_ITEM (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (drawable)));
          else
            item = GIMP_ITEM (drawable);

          writable = ! gimp_item_is_content_locked (item, NULL);

          if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
            is_group = TRUE;

          if (GIMP_IS_GROUP_LAYER (drawable)                   ||
              gimp_item_is_vector_layer (GIMP_ITEM (drawable)) ||
              gimp_item_is_link_layer (GIMP_ITEM (drawable)))
            force_nde = TRUE;
        }

      g_list_free (drawables);
   }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  actions = gimp_action_group_list_actions (group);
  for (iter = actions; iter; iter = iter->next)
    {
      GimpAction  *action = iter->data;
      const gchar *action_name;

      action_name = gimp_action_get_name (action);

      if (filters_is_non_interactive (action_name))
        {
          /* Even I'm not sure they should, right now non-interactive
           * actions are always applied destructively. So these filters
           * are incompatible with layers where non-destructivity is
           * mandatory.
           */
          SET_SENSITIVE (action_name, writable && ! force_nde);
        }
      else if (GIMP_IS_STRING_ACTION (action))
        {
          const gchar *opname;

          opname = GIMP_STRING_ACTION (action)->value;

          if (opname == NULL)
            /* These are the filters-recent-*, repeat and reshow handled
             * below.
             */
            continue;

          if (g_strcmp0 (opname, "gegl:gegl") == 0)
            {
              /* GEGL graph filter can only be run destructively, unless
               * the GIMP_ALLOW_GEGL_GRAPH_LAYER_EFFECT environment
               * variable is set.
               */
              SET_SENSITIVE (gimp_action_get_name (action), writable &&
                             (g_getenv ("GIMP_ALLOW_GEGL_GRAPH_LAYER_EFFECT") != NULL || ! force_nde));
            }
          else if (gegl_has_operation (opname))
            {
              gboolean sensitive = writable;

              if (sensitive && force_nde)
                /* Operations with auxiliary inputs can only be applied
                 * destructively. Therefore they must be deactivated on
                 * types of layers where filters can only be applied
                 * non-destructively.
                 */
                sensitive = ! actions_filter_get_aux (action_name);

              SET_SENSITIVE (gimp_action_get_name (action), sensitive);
            }
        }
    }
  g_list_free (actions);

  /* Special-cased filters */
  SET_SENSITIVE ("filters-c2g",                     writable && !gray);
  SET_SENSITIVE ("filters-color-balance",           writable && !gray);
  SET_SENSITIVE ("filters-color-enhance",           writable && !force_nde && !gray);
  SET_SENSITIVE ("filters-colorize",                writable && !gray);
  SET_SENSITIVE ("filters-color-temperature",       writable && !gray);
  SET_SENSITIVE ("filters-color-to-alpha",          writable && supports_alpha);
  SET_SENSITIVE ("filters-desaturate",              writable && !gray);

  SET_SENSITIVE ("filters-dropshadow",              writable && alpha);
  SET_SENSITIVE ("filters-edge",                    writable && !is_group);
  SET_SENSITIVE ("filters-hue-saturation",          writable && !gray);
  SET_SENSITIVE ("filters-long-shadow",             writable && alpha);
  SET_SENSITIVE ("filters-mono-mixer",              writable && !gray);
  SET_SENSITIVE ("filters-noise-hsv",               writable && !gray);
  SET_SENSITIVE ("filters-red-eye-removal",         writable && !gray);
  SET_SENSITIVE ("filters-saturation",              writable && !gray);
  SET_SENSITIVE ("filters-semi-flatten",            writable && alpha);
  SET_SENSITIVE ("filters-sepia",                   writable && !gray);
  SET_SENSITIVE ("filters-threshold-alpha",         writable && alpha);

#undef SET_SENSITIVE

  {
    GimpProcedure *proc   = gimp_filter_history_nth (group->gimp, 0);
    const gchar   *reason = NULL;
    gint           i;

    if (proc &&
        gimp_procedure_get_sensitive (proc, GIMP_OBJECT (image), &reason))
      {
        gimp_action_group_set_action_sensitive (group, "filters-repeat", TRUE, NULL);
        gimp_action_group_set_action_sensitive (group, "filters-reshow", TRUE, NULL);
      }
    else
      {
        gimp_action_group_set_action_sensitive (group, "filters-repeat", FALSE, reason);
        gimp_action_group_set_action_sensitive (group, "filters-reshow", FALSE, reason);
     }

    for (i = 0; i < gimp_filter_history_length (group->gimp); i++)
      {
        gchar    *name = g_strdup_printf ("filters-recent-%02d", i + 1);
        gboolean  sensitive;

        proc = gimp_filter_history_nth (group->gimp, i);

        reason = NULL;
        sensitive = gimp_procedure_get_sensitive (proc, GIMP_OBJECT (image),
                                                  &reason);

        gimp_action_group_set_action_sensitive (group, name, sensitive, reason);

        g_free (name);
      }
  }
}

gboolean
filters_actions_gegl_op_blocklisted (const gchar *operation_name)
{
  for (gint i = 0; i < G_N_ELEMENTS (filters_actions); i++)
    {
      const GimpStringActionEntry *action_entry = &filters_actions[i];

      if (g_strcmp0 (operation_name, action_entry->value) == 0)
        return TRUE;
    }

  for (gint i = 0; i < G_N_ELEMENTS (filters_settings_actions); i++)
    {
      const GimpStringActionEntry *action_entry = &filters_settings_actions[i];

      if (g_strcmp0 (operation_name, action_entry->value) == 0)
        return TRUE;
    }

  for (gint i = 0; i < G_N_ELEMENTS (filters_interactive_actions); i++)
    {
      const GimpStringActionEntry *action_entry = &filters_interactive_actions[i];

      if (g_strcmp0 (operation_name, action_entry->value) == 0)
        return TRUE;
    }

  return FALSE;
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
      const gchar *reason    = NULL;
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
        sensitive = gimp_action_get_sensitive (actual_action, &reason);

      gimp_action_group_set_action_sensitive (group, "filters-repeat",
                                              sensitive, reason);
      gimp_action_group_set_action_sensitive (group, "filters-reshow",
                                              sensitive, reason);
   }
  else
    {
      gimp_action_group_set_action_label (group, "filters-repeat",
                                          _("Repeat Last"));
      gimp_action_group_set_action_label (group, "filters-reshow",
                                          _("Re-Show Last"));

      gimp_action_group_set_action_sensitive (group, "filters-repeat",
                                              FALSE, _("No last used filters"));
      gimp_action_group_set_action_sensitive (group, "filters-reshow",
                                              FALSE, _("No last used filters"));
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
        sensitive = gimp_action_get_sensitive (actual_action, NULL);

      g_object_set (action,
                    "sensitive", sensitive,
                    "procedure", proc,
                    "label",     label,
                    "icon-name", gimp_viewable_get_icon_name (GIMP_VIEWABLE (proc)),
                    "tooltip",   gimp_procedure_get_blurb (proc),
                    /* It is very important that "visible" is set at the
                     * end, because the docs says that:
                     *
                     * > "notify" signals are queued and only emitted (in reverse order) after all properties have been set.
                     *
                     * If "visible" is set before "label" in particular,
                     * we end up in the inconsistent situation where the
                     * "visible" callbacks have not been run yet, so
                     * menus don't have the corresponding item whereas
                     * the action already shows as visible. In
                     * particular, g_menu_model_items_changed() may
                     * crash on an empty item list in GIMP_GTK_MENUBAR
                     * codepath.
                     */
                    "visible",   TRUE,
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

static gboolean
filters_is_non_interactive (const gchar *action_name)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (filters_actions); i++)
    if (g_strcmp0 (filters_actions[i].name, action_name) == 0)
      return TRUE;

  for (i = 0; i < G_N_ELEMENTS (filters_settings_actions); i++)
    if (g_strcmp0 (filters_settings_actions[i].name, action_name) == 0)
      return TRUE;

  return FALSE;
}
