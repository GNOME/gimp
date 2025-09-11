/* GIMP plug-in to load and export Paint Shop Pro files (.PSP and .TUB)
 *
 * Copyright (C) 1999 Tor Lillqvist
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

/*
 *
 * Work in progress! Doesn't handle exporting yet.
 *
 * For a copy of the PSP file format documentation, surf to
 * http://www.jasc.com.
 *
 */

#define LOAD_PROC      "file-psp-load"
#define EXPORT_PROC    "file-psp-export"
#define PLUG_IN_BINARY "file-psp"
#define PLUG_IN_ROLE   "gimp-file-psp"

/* set to the level of debugging output you want, 0 for none */
#define PSP_DEBUG 0

#define IFDBG(level) if (PSP_DEBUG >= level)

#include "config.h"

#include <errno.h>
#include <string.h>
#include <zlib.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimpbase/gimpparasiteio.h>

#include "libgimp/stdplugins-intl.h"


/* Note that the upcoming PSP version 6 writes PSP file format version
 * 4.0, but the documentation for that apparently isn't publicly
 * available (yet). The format is luckily designed to be somwehat
 * downward compatible, however. The semantics of many of the
 * additional fields and block types can be relatively easily reverse
 * engineered.
 */

/* The following was cut and pasted from the PSP file format
 * documentation version 3.0.(Minor stylistic changes done.)
 *
 *
 * To be on the safe side, here is the whole copyright notice from the
 * specification:
 *
 * The Paint Shop Pro File Format Specification (the Specification) is
 * copyright 1998 by Jasc Software, Inc. Jasc grants you a
 * nonexclusive license to use the Specification for the sole purposes
 * of developing software products(s) incorporating the
 * Specification. You are also granted the right to identify your
 * software product(s) as incorporating the Paint Shop Pro Format
 * (PSP) provided that your software in incorporating the
 * Specification complies with the terms, definitions, constraints and
 * specifications contained in the Specification and subject to the
 * following: DISCLAIMER OF WARRANTIES. THE SPECIFICATION IS PROVIDED
 * AS IS. JASC DISCLAIMS ALL OTHER WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.
 *
 * You are solely responsible for the selection, use, efficiency and
 * suitability of the Specification for your software products.  OTHER
 * WARRANTIES EXCLUDED. JASC SHALL NOT BE LIABLE FOR ANY DIRECT,
 * INDIRECT, CONSEQUENTIAL, EXEMPLARY, PUNITIVE OR INCIDENTAL DAMAGES
 * ARISING FROM ANY CAUSE EVEN IF JASC HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES. CERTAIN JURISDICTIONS DO NOT PERMIT
 * THE LIMITATION OR EXCLUSION OF INCIDENTAL DAMAGES, SO THIS
 * LIMITATION MAY NOT APPLY TO YOU.  IN NO EVENT WILL JASC BE LIABLE
 * FOR ANY AMOUNT GREATER THAN WHAT YOU ACTUALLY PAID FOR THE
 * SPECIFICATION. Should any warranties be found to exist, such
 * warranties shall be limited in duration to ninety (90) days
 * following the date you receive the Specification.
 *
 * Indemnification. By your inclusion of the Paint Shop Pro File
 * Format in your software product(s) you agree to indemnify and hold
 * Jasc Software, Inc. harmless from any and all claims of any kind or
 * nature made by any of your customers with respect to your software
 * product(s).
 *
 * Export Laws. You agree that you and your customers will not export
 * your software or Specification except in compliance with the laws
 * and regulations of the United States.
 *
 * US Government Restricted Rights. The Specification and any
 * accompanying materials are provided with Restricted Rights. Use,
 * duplication or disclosure by the Government is subject to
 * restrictions as set forth in subparagraph (c)(1)(ii) of The Rights
 * in Technical Data and Computer Software clause at DFARS
 * 252.227-7013, or subparagraphs (c)(1) and (2) of the Commercial
 * Computer Software - Restricted Rights at 48 CFR 52.227-19, as
 * applicable. Contractor/manufacturer is Jasc Software, Inc., PO Box
 * 44997, Eden Prairie MN 55344.
 *
 * Jasc reserves the right to amend, modify, change, revoke or
 * withdraw the Specification at any time and from time to time. Jasc
 * shall have no obligation to support or maintain the Specification.
 */

/* Block identifiers.
 */
typedef enum {
  PSP_IMAGE_BLOCK = 0,                  /* General Image Attributes Block (main) */
  PSP_CREATOR_BLOCK,                    /* Creator Data Block (main) */
  PSP_COLOR_BLOCK,                      /* Color Palette Block (main and sub) */
  PSP_LAYER_START_BLOCK,                /* Layer Bank Block (main) */
  PSP_LAYER_BLOCK,                      /* Layer Block (sub) */
  PSP_CHANNEL_BLOCK,                    /* Channel Block (sub) */
  PSP_SELECTION_BLOCK,                  /* Selection Block (main) */
  PSP_ALPHA_BANK_BLOCK,                 /* Alpha Bank Block (main) */
  PSP_ALPHA_CHANNEL_BLOCK,              /* Alpha Channel Block (sub) */
  PSP_THUMBNAIL_BLOCK,                  /* Thumbnail Block (main) */
  PSP_EXTENDED_DATA_BLOCK,              /* Extended Data Block (main) */
  PSP_TUBE_BLOCK,                       /* Picture Tube Data Block (main) */
  PSP_ADJUSTMENT_EXTENSION_BLOCK,       /* Adjustment Layer Extension Block (sub) (since PSP6)*/
  PSP_VECTOR_EXTENSION_BLOCK,           /* Vector Layer Extension Block (sub) (since PSP6) */
  PSP_SHAPE_BLOCK,                      /* Vector Shape Block (sub) (since PSP6) */
  PSP_PAINTSTYLE_BLOCK,                 /* Paint Style Block (sub) (since PSP6) */
  PSP_COMPOSITE_IMAGE_BANK_BLOCK,       /* Composite Image Bank (main) (since PSP6) */
  PSP_COMPOSITE_ATTRIBUTES_BLOCK,       /* Composite Image Attributes (sub) (since PSP6) */
  PSP_JPEG_BLOCK,                       /* JPEG Image Block (sub) (since PSP6) */
  PSP_LINESTYLE_BLOCK,                  /* Line Style Block (sub) (since PSP7) */
  PSP_TABLE_BANK_BLOCK,                 /* Table Bank Block (main) (since PSP7) */
  PSP_TABLE_BLOCK,                      /* Table Block (sub) (since PSP7) */
  PSP_PAPER_BLOCK,                      /* Vector Table Paper Block (sub) (since PSP7) */
  PSP_PATTERN_BLOCK,                    /* Vector Table Pattern Block (sub) (since PSP7) */
  PSP_GRADIENT_BLOCK,                   /* Vector Table Gradient Block (not used) (since PSP8) */
  PSP_GROUP_EXTENSION_BLOCK,            /* Group Layer Block (sub) (since PSP8) */
  PSP_MASK_EXTENSION_BLOCK,             /* Mask Layer Block (sub) (since PSP8) */
  PSP_BRUSH_BLOCK,                      /* Brush Data Block (main) (since PSP8) */
  PSP_ART_MEDIA_BLOCK,                  /* Art Media Layer Block (main) (since PSP9) */
  PSP_ART_MEDIA_MAP_BLOCK,              /* Art Media Layer map data Block (main) (since PSP9) */
  PSP_ART_MEDIA_TILE_BLOCK,             /* Art Media Layer map tile Block(main) (since PSP9) */
  PSP_ART_MEDIA_TEXTURE_BLOCK,          /* AM Layer map texture Block (main) (since PSP9) */
  PSP_COLORPROFILE_BLOCK,               /* ICC Color profile block (since PSP10) */
  PSP_RASTER_EXTENSION_BLOCK,           /* Assumed name based on usage, probably since PSP11 */
} PSPBlockID;

/* Bitmap type.
 */
typedef enum {
  PSP_DIB_IMAGE = 0,            /* Layer color bitmap */
  PSP_DIB_TRANS_MASK,           /* Layer transparency mask bitmap */
  PSP_DIB_USER_MASK,            /* Layer user mask bitmap */
  PSP_DIB_SELECTION,            /* Selection mask bitmap */
  PSP_DIB_ALPHA_MASK,           /* Alpha channel mask bitmap */
  PSP_DIB_THUMBNAIL,            /* Thumbnail bitmap */
  PSP_DIB_THUMBNAIL_TRANS_MASK, /* Thumbnail transparency mask (since PSP6) */
  PSP_DIB_ADJUSTMENT_LAYER,     /* Adjustment layer bitmap (since PSP6) */
  PSP_DIB_COMPOSITE,            /* Composite image bitmap (since PSP6) */
  PSP_DIB_COMPOSITE_TRANS_MASK, /* Composite image transparency (since PSP6) */
  PSP_DIB_PAPER,                /* Paper bitmap (since PSP7) */
  PSP_DIB_PATTERN,              /* Pattern bitmap (since PSP7) */
  PSP_DIB_PATTERN_TRANS_MASK,   /* Pattern transparency mask (since PSP7) */
} PSPDIBType;

/* Type of image in the composite image bank block. (since PSP6)
 */
typedef enum {
  PSP_IMAGE_COMPOSITE = 0,      /* Composite Image */
  PSP_IMAGE_THUMBNAIL,          /* Thumbnail Image */
} PSPCompositeImageType;

/* Graphic contents flags. (since PSP6)
 */
typedef enum {
  /* Layer types */
  keGCRasterLayers     = 0x00000001,    /* At least one raster layer */
  keGCVectorLayers     = 0x00000002,    /* At least one vector layer */
  keGCAdjustmentLayers = 0x00000004,    /* At least one adjustment layer */
  keGCGroupLayers      = 0x00000008,    /* at least one group layer */
  keGCMaskLayers       = 0x00000010,    /* at least one mask layer */
  keGCArtMediaLayers   = 0x00000020,    /* at least one art media layer */

  /* Additional attributes */
  keGCMergedCache            = 0x00800000,      /* merged cache (composite image) */
  keGCThumbnail              = 0x01000000,      /* Has a thumbnail */
  keGCThumbnailTransparency  = 0x02000000,      /* Thumbnail transp. */
  keGCComposite              = 0x04000000,      /* Has a composite image */
  keGCCompositeTransparency  = 0x08000000,      /* Composite transp. */
  keGCFlatImage              = 0x10000000,      /* Just a background */
  keGCSelection              = 0x20000000,      /* Has a selection */
  keGCFloatingSelectionLayer = 0x40000000,      /* Has float. selection */
  keGCAlphaChannels          = 0x80000000,      /* Has alpha channel(s) */
} PSPGraphicContents;

/* Character style flags. (since PSP6)
 */
typedef enum {
  keStyleItalic      = 0x00000001,      /* Italic property bit */
  keStyleStruck      = 0x00000002,      /* Strike­out property bit */
  keStyleUnderlined  = 0x00000004,      /* Underlined property bit */
  keStyleWarped      = 0x00000008,      /* Warped property bit (since PSP8) */
  keStyleAntiAliased = 0x00000010,      /* Anti­aliased property bit (since PSP8) */
} PSPCharacterProperties;

/* Table type. (since PSP7)
 */
typedef enum {
  keTTUndefined = 0,     /* Undefined table type */
  keTTGradientTable,     /* Gradient table type */
  keTTPaperTable,        /* Paper table type */
  keTTPatternTable       /* Pattern table type */
} PSPTableType;

/* Layer flags. (since PSP6)
 */
typedef enum {
  keVisibleFlag      = 0x00000001,      /* Layer is visible */
  keMaskPresenceFlag = 0x00000002,      /* Layer has a mask */
} PSPLayerProperties;

/* Shape property flags. (since PSP6)
 */
typedef enum {
  keShapeAntiAliased = 0x00000001,      /* Shape is anti­aliased */
  keShapeSelected    = 0x00000002,      /* Shape is selected */
  keShapeVisible     = 0x00000004,      /* Shape is visible */
} PSPShapeProperties;

/* Polyline node type flags. (since PSP7)
 */
typedef enum {
  keNodeUnconstrained     = 0x0000,     /* Default node type */
  keNodeSmooth            = 0x0001,     /* Node is smooth */
  keNodeSymmetric         = 0x0002,     /* Node is symmetric */
  keNodeAligned           = 0x0004,     /* Node is aligned */
  keNodeActive            = 0x0008,     /* Node is active */
  keNodeLocked            = 0x0010,     /* Node is locked */
  keNodeSelected          = 0x0020,     /* Node is selected */
  keNodeVisible           = 0x0040,     /* Node is visible */
  keNodeClosed            = 0x0080,     /* Node is closed */

  /* TODO: This might be a thinko in the spec document only or in the image
   *       format itself. Need to investigate that later
   */
  keNodeLockedPSP6        = 0x0016,     /* Node is locked */
  keNodeSelectedPSP6      = 0x0032,     /* Node is selected */
  keNodeVisiblePSP6       = 0x0064,     /* Node is visible */
  keNodeClosedPSP6        = 0x0128,     /* Node is closed */

} PSPPolylineNodeTypes;

/* Blend modes. (since PSP6)
 */
typedef enum {
  PSP_BLEND_NORMAL,
  PSP_BLEND_DARKEN,
  PSP_BLEND_LIGHTEN,
  PSP_BLEND_HUE,
  PSP_BLEND_SATURATION,
  PSP_BLEND_COLOR,
  PSP_BLEND_LUMINOSITY,
  PSP_BLEND_MULTIPLY,
  PSP_BLEND_SCREEN,
  PSP_BLEND_DISSOLVE,
  PSP_BLEND_OVERLAY,
  PSP_BLEND_HARD_LIGHT,
  PSP_BLEND_SOFT_LIGHT,
  PSP_BLEND_DIFFERENCE,
  PSP_BLEND_DODGE,
  PSP_BLEND_BURN,
  PSP_BLEND_EXCLUSION,
  PSP_BLEND_TRUE_HUE, /* since PSP8 */
  PSP_BLEND_TRUE_SATURATION, /* since PSP8 */
  PSP_BLEND_TRUE_COLOR, /* since PSP8 */
  PSP_BLEND_TRUE_LIGHTNESS, /* since PSP8 */
  PSP_BLEND_ADJUST = 255,
} PSPBlendModes;

/* Adjustment layer types. (since PSP6)
 */
typedef enum {
  keAdjNone = 0,        /* Undefined adjustment layer type */
  keAdjLevel,           /* Level adjustment */
  keAdjCurve,           /* Curve adjustment */
  keAdjBrightContrast,  /* Brightness­contrast adjustment */
  keAdjColorBal,        /* Color balance adjustment */
  keAdjHSL,             /* HSL adjustment */
  keAdjChannelMixer,    /* Channel mixer adjustment */
  keAdjInvert,          /* Invert adjustment */
  keAdjThreshold,       /* Threshold adjustment */
  keAdjPoster           /* Posterize adjustment */
} PSPAdjustmentLayerType;

/* Vector shape types. (since PSP6)
 */
typedef enum {
  keVSTUnknown = 0,     /* Undefined vector type */
  keVSTText,            /* Shape represents lines of text */
  keVSTPolyline,        /* Shape represents a multiple segment line */
  keVSTEllipse,         /* Shape represents an ellipse (or circle) */
  keVSTPolygon,         /* Shape represents a closed polygon */
  keVSTGroup,           /* Shape represents a group shape (since PSP7) */
} PSPVectorShapeType;

/* Text element types. (since PSP6)
 */
typedef enum {
  keTextElemUnknown = 0,        /* Undefined text element type */
  keTextElemChar,               /* A single character code */
  keTextElemCharStyle,          /* A character style change */
  keTextElemLineStyle           /* A line style change */
} PSPTextElementType;

/* Text alignment types. (since PSP6)
 */
typedef enum {
  keTextAlignmentLeft = 0,      /* Left text alignment */
  keTextAlignmentCenter,        /* Center text alignment */
  keTextAlignmentRight          /* Right text alignment */
} PSPTextAlignment;

/* Text antialias modes. */
typedef enum {
  keNoAntialias = 0,            /* Antialias off */
  keSharpAntialias,             /* Sharp */
  keSmoothAntialias             /* Smooth */
} PSPAntialiasMode;

/* Text flow types */
typedef enum {
  keTFHorizontalDown = 0,       /* Horizontal then down */
  keTFVerticalLeft,             /* Vertical then left */
  keTFVerticalRight,            /* Vertical then right */
  keTFHorizontalUp              /* Horizontal then up */
} PSPTextFlow;

/* Paint style types. (since PSP6)
 */
typedef enum {
  keStyleNone     = 0x0000,     /* No paint style info applies */
  keStyleColor    = 0x0001,     /* Color paint style info */
  keStyleGradient = 0x0002,     /* Gradient paint style info */
  keStylePattern  = 0x0004,     /* Pattern paint style info (since PSP7) */
  keStylePaper    = 0x0008,     /* Paper paint style info (since PSP7) */
  keStylePen      = 0x0010,     /* Organic pen paint style info (since PSP7) */
} PSPPaintStyleType;

/* Gradient type. (since PSP7)
 */
typedef enum {
  keSGTLinear = 0,      /* Linera gradient type */
  keSGTRadial,          /* Radial gradient type */
  keSGTRectangular,     /* Rectangulat gradient type */
  keSGTSunburst         /* Sunburst gradient type */
} PSPStyleGradientType;

/* Paint Style Cap Type (Start & End). (since PSP7)
 */
typedef enum {
  keSCTCapFlat = 0,             /* Flat cap type (was round in psp6) */
  keSCTCapRound,                /* Round cap type (was square in psp6) */
  keSCTCapSquare,               /* Square cap type (was flat in psp6) */
  keSCTCapArrow,                /* Arrow cap type */
  keSCTCapCadArrow,             /* Cad arrow cap type */
  keSCTCapCurvedTipArrow,       /* Curved tip arrow cap type */
  keSCTCapRingBaseArrow,        /* Ring base arrow cap type */
  keSCTCapFluerDelis,           /* Fluer deLis cap type */
  keSCTCapFootball,             /* Football cap type */
  keSCTCapXr71Arrow,            /* Xr71 arrow cap type */
  keSCTCapLilly,                /* Lilly cap type */
  keSCTCapPinapple,             /* Pinapple cap type */
  keSCTCapBall,                 /* Ball cap type */
  keSCTCapTulip                 /* Tulip cap type */
} PSPStyleCapType;

/* Paint Style Join Type. (since PSP7)
 */
typedef enum {
  keSJTJoinMiter = 0,
  keSJTJoinRound,
  keSJTJoinBevel
} PSPStyleJoinType;

/* Organic pen type. (since PSP7)
 */
typedef enum {
  keSPTOrganicPenNone = 0,      /* Undefined pen type */
  keSPTOrganicPenMesh,          /* Mesh pen type */
  keSPTOrganicPenSand,          /* Sand pen type */
  keSPTOrganicPenCurlicues,     /* Curlicues pen type */
  keSPTOrganicPenRays,          /* Rays pen type */
  keSPTOrganicPenRipple,        /* Ripple pen type */
  keSPTOrganicPenWave,          /* Wave pen type */
  keSPTOrganicPen               /* Generic pen type */
} PSPStylePenType;


/* Channel types.
 */
typedef enum {
  PSP_CHANNEL_COMPOSITE = 0,    /* Channel of single channel bitmap */
  PSP_CHANNEL_RED,              /* Red channel of 24 bit bitmap */
  PSP_CHANNEL_GREEN,            /* Green channel of 24 bit bitmap */
  PSP_CHANNEL_BLUE              /* Blue channel of 24 bit bitmap */
} PSPChannelType;

/* Possible metrics used to measure resolution.
 */
typedef enum {
  PSP_METRIC_UNDEFINED = 0,     /* Metric unknown */
  PSP_METRIC_INCH,              /* Resolution is in inches */
  PSP_METRIC_CM                 /* Resolution is in centimeters */
} PSP_METRIC;

/* Possible types of compression.
 */
typedef enum {
  PSP_COMP_NONE = 0,            /* No compression */
  PSP_COMP_RLE,                 /* RLE compression */
  PSP_COMP_LZ77,                /* LZ77 compression */
  PSP_COMP_JPEG                 /* JPEG compression (only used by thumbnail and composite image) (since PSP6) */
} PSPCompression;

/* Picture tube placement mode.
 */
typedef enum {
  tpmRandom,                    /* Place tube images in random intervals */
  tpmConstant                   /* Place tube images in constant intervals */
} TubePlacementMode;

/* Picture tube selection mode.
 */
typedef enum {
  tsmRandom,                    /* Randomly select the next image in  */
                                /* tube to display */
  tsmIncremental,               /* Select each tube image in turn */
  tsmAngular,                   /* Select image based on cursor direction */
  tsmPressure,                  /* Select image based on pressure  */
                                /* (from pressure-sensitive pad) */
  tsmVelocity                   /* Select image based on cursor speed */
} TubeSelectionMode;

/* Extended data field types.
 */
typedef enum {
  PSP_XDATA_TRNS_INDEX = 0,     /* Transparency index field */
  PSP_XDATA_GRID,               /* Image grid information (since PSP7) */
  PSP_XDATA_GUIDE,              /* Image guide information (since PSP7) */
  PSP_XDATA_EXIF,               /* Image Exif information (since PSP8) */
  PSP_XDATA_IPTC,               /* Image IPTC information (since PSP8) */
} PSPExtendedDataID;

/* Creator field types.
 */
typedef enum {
  PSP_CRTR_FLD_TITLE = 0,       /* Image document title field */
  PSP_CRTR_FLD_CRT_DATE,        /* Creation date field */
  PSP_CRTR_FLD_MOD_DATE,        /* Modification date field */
  PSP_CRTR_FLD_ARTIST,          /* Artist name field */
  PSP_CRTR_FLD_CPYRGHT,         /* Copyright holder name field */
  PSP_CRTR_FLD_DESC,            /* Image document description field */
  PSP_CRTR_FLD_APP_ID,          /* Creating app id field */
  PSP_CRTR_FLD_APP_VER          /* Creating app version field */
} PSPCreatorFieldID;

/* Grid units type. (since PSP7)
 */
typedef enum {
  keGridUnitsPixels = 0,        /* Grid units is pixels */
  keGridUnitsInches,            /* Grid units is inches */
  keGridUnitsCentimeters        /* Grid units is centimeters */
} PSPGridUnitsType;

/* Guide orientation type. (since PSP7)
 */
typedef enum  {
  keHorizontalGuide = 0,
  keVerticalGuide
} PSPGuideOrientationType;

/* Creator application identifiers.
 */
typedef enum {
  PSP_CREATOR_APP_UNKNOWN = 0,  /* Creator application unknown */
  PSP_CREATOR_APP_PAINT_SHOP_PRO /* Creator is Paint Shop Pro */
} PSPCreatorAppID;

/* Layer types.
 */
typedef enum {
  PSP_LAYER_NORMAL = 0,         /* Normal layer */
  PSP_LAYER_FLOATING_SELECTION  /* Floating selection layer */
} PSPLayerTypePSP5;

/* Layer types. (since PSP6)
 */
typedef enum {
  keGLTUndefined = 0,           /* Undefined layer type */
  keGLTRaster,                  /* Standard raster layer */
  keGLTFloatingRasterSelection, /* Floating selection (raster layer) */
  keGLTVector,                  /* Vector layer */
  keGLTAdjustment,              /* Adjustment layer */
  keGLTGroup,                   /* Group layer (since PSP8) */
  keGLTMask,                    /* Mask layer (since PSP8) */
  keGLTArtMedia                 /* Art media layer (since PSP9) */
} PSPLayerTypePSP6;

/* Art media layer map types (since PSP7) */
typedef enum {
keArtMediaColorMap = 0,
keArtMediaBumpMap,
keArtMediaShininessMap,
keArtMediaReflectivityMap,
keArtMediaDrynessMap
} PSPArtMediaMapType;


/* Truth values.
 */
#if 0                           /* FALSE and TRUE taken by GLib */
typedef enum {
  FALSE = 0,
  TRUE
} PSP_BOOLEAN;
#else
typedef gboolean PSP_BOOLEAN;
#endif

/* End of cut&paste from psp spec */

/* We store the various PSP data in own structures.
 * We cannot use structs intended to be direct copies of the file block
 * headers because of struct alignment issues.
 */
typedef struct
{
  guint32           width, height;
  gdouble           resolution;
  guchar            metric;
  guint16           compression;
  guint16           depth;
  guchar            grayscale;
  guint32           active_layer;
  guint16           layer_count;
  guint16           bytes_per_sample;
  GimpImageBaseType base_type;
  GimpPrecision     precision;
} PSPimage;


typedef struct _Psp      Psp;
typedef struct _PspClass PspClass;

struct _Psp
{
  GimpPlugIn      parent_instance;
};

struct _PspClass
{
  GimpPlugInClass parent_class;
};


#define PSP_TYPE  (psp_get_type ())
#define PSP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PSP_TYPE, Psp))

GType                   psp_get_type         (void) G_GNUC_CONST;

static GList          * psp_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * psp_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static GimpValueArray * psp_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);
static GimpValueArray * psp_export           (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GimpImage             *image,
                                              GFile                 *file,
                                              GimpExportOptions     *options,
                                              GimpMetadata          *metadata,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage      * load_image           (GFile                 *file,
                                              GError               **error);
static gboolean         export_image         (GFile                 *file,
                                              GimpImage             *image,
                                              gint                   n_drawables,
                                              GList                 *drawables,
                                              GObject               *config,
                                              GError               **error);
static gboolean         save_dialog          (GimpProcedure         *procedure,
                                              GObject               *config,
                                              GimpImage             *image);


G_DEFINE_TYPE (Psp, psp, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PSP_TYPE)
DEFINE_STD_SET_I18N


static guint16 psp_ver_major;
static guint16 psp_ver_minor;


static void
psp_class_init (PspClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = psp_query_procedures;
  plug_in_class->create_procedure = psp_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
psp_init (Psp *psp)
{
}

static GList *
psp_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
#if 0
  /* commented out until exporting is implemented */
  list = g_list_append (list, g_strdup (EXPORT_PROC));
#endif
  return list;
}

static GimpProcedure *
psp_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           psp_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("Paint Shop Pro image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads images from the Paint Shop "
                                          "Pro PSP file format"),
                                        _("This plug-in loads and exports images "
                                          "in Paint Shop Pro's native PSP format. "
                                          "Vector layers aren't handled. "
                                          "Exporting isn't yet implemented."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Tor Lillqvist",
                                      "Tor Lillqvist",
                                      "1999");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-psp");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "psp,tub,pspimage,psptube");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,Paint\\040Shop\\040Pro\\040Image\\040File\n\032");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, psp_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("Paint Shop Pro image"));
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           "PSP");
      gimp_procedure_set_documentation (procedure,
                                        "Exports images in the Paint Shop Pro "
                                        "PSP file format",
                                        "This plug-in loads and exports images "
                                        "in Paint Shop Pro's native PSP format. "
                                        "Vector layers aren't handled. "
                                        "Exporting isn't yet implemented.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Tor Lillqvist",
                                      "Tor Lillqvist",
                                      "1999");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-psp");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "psp,tub");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB     |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                              GIMP_EXPORT_CAN_HANDLE_LAYERS,
                                              NULL, NULL, NULL);

      gimp_procedure_add_choice_argument (procedure, "compression",
                                          _("_Data Compression"),
                                          _("Type of compression"),
                                          gimp_choice_new_with_values ("none", PSP_COMP_NONE, _("none"), NULL,
                                                                       "rle",  PSP_COMP_RLE,  _("rle"),  NULL,
                                                                       "lz77", PSP_COMP_LZ77, _("lz77"), NULL,
                                                                       NULL),
                                          "lz77",
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
psp_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
psp_export (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GFile                *file,
            GimpExportOptions    *options,
            GimpMetadata         *metadata,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GList             *drawables;
  gint               n_drawables;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (procedure, G_OBJECT (config), image))
        status = GIMP_PDB_CANCEL;
    }

  export      = gimp_export_options_get_image (options, &image);
  drawables   = gimp_image_list_layers (image);
  n_drawables = g_list_length (drawables);

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! export_image (file, image, n_drawables, drawables,
                          G_OBJECT (config), &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

static gboolean
save_dialog (GimpProcedure *procedure,
             GObject       *config,
             GimpImage     *image)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  gint       run;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  /*  file save type  */
  frame = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                            "compression", GIMP_TYPE_INT_RADIO_FRAME);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), NULL);
  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

/* This helper method is used to get the name of the block for the known block
 * types. The enum PSPBlockID must cover the input values.
 */
static const gchar *
block_name (gint id)
{
  static const gchar *block_names[] =
  {
    "IMAGE",
    "CREATOR",
    "COLOR",
    "LAYER_START",
    "LAYER",
    "CHANNEL",
    "SELECTION",
    "ALPHA_BANK",
    "ALPHA_CHANNEL",
    "THUMBNAIL",
    "EXTENDED_DATA",
    "TUBE",
    "ADJUSTMENT_EXTENSION",
    "VECTOR_EXTENSION_BLOCK",
    "SHAPE_BLOCK",
    "PAINTSTYLE_BLOCK",
    "COMPOSITE_IMAGE_BANK_BLOCK",
    "COMPOSITE_ATTRIBUTES_BLOCK",
    "JPEG_BLOCK",
    "LINESTYLE_BLOCK",
    "TABLE_BANK_BLOCK",
    "TABLE_BLOCK",
    "PAPER_BLOCK",
    "PATTERN_BLOCK",
    "GRADIENT_BLOCK",
    "GROUP_EXTENSION_BLOCK",
    "MASK_EXTENSION_BLOCK",
    "BRUSH_BLOCK",
    "ART_MEDIA_BLOCK",
    "ART_MEDIA_MAP_BLOCK",
    "ART_MEDIA_TILE_BLOCK",
    "ART_MEDIA_TEXTURE_BLOCK",
    "COLORPROFILE_BLOCK",
    "RASTER_EXTENSION_BLOCK",
};
  static gchar *err_name = NULL;

  if (id >= 0 && id <= PSP_RASTER_EXTENSION_BLOCK)
    return block_names[id];

  g_free (err_name);
  err_name = g_strdup_printf ("id=%d", id);

  return err_name;
}

/* This helper method is used during loading. It verifies the block we are
 * reading has a valid header. Fills the variables init_len and total_len
 */
static gint
read_block_header (FILE     *f,
                   guint32  *init_len,
                   guint32  *total_len,
                   GError  **error)
{
  guchar buf[4];
  guint16 id;
  long header_start;
  guint32 len;

  IFDBG(3) header_start = ftell (f);

  if (fread (buf, 4, 1, f) < 1
      || fread (&id, 2, 1, f) < 1
      || fread (&len, 4, 1, f) < 1
      || (psp_ver_major < 4 && fread (total_len, 4, 1, f) < 1))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading block header"));
      return -1;
    }
  if (memcmp (buf, "~BK\0", 4) != 0)
    {
      IFDBG(3)
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Invalid block header at %ld"), header_start);
      else
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Invalid block header"));
      return -1;
    }

  IFDBG(3) g_message ("%s at %ld", block_name (id), header_start);

  if (psp_ver_major < 4)
    {
      *init_len = GUINT32_FROM_LE (len);
      *total_len = GUINT32_FROM_LE (*total_len);
    }
  else
    {
      /* Version 4.0 seems to have dropped the initial data chunk length
       * field.
       */
      *init_len = 0xDEADBEEF;   /* Intentionally bogus, should not be used */
      *total_len = GUINT32_FROM_LE (len);
    }

  return GUINT16_FROM_LE (id);
}

static gint
try_fseek (FILE    *f,
           glong    pos,
           gint     whence,
           GError **error)
{
  if (fseek (f, pos, whence) < 0)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Seek error: %s"), g_strerror (errno));
      fclose (f);
      return -1;
    }
  return 0;
}

/* Read the PSP_IMAGE_BLOCK */
static gint
read_general_image_attribute_block (FILE     *f,
                                    guint     init_len,
                                    guint     total_len,
                                    PSPimage *ia,
                                    GError  **error)
{
  gchar buf[6];
  guint64 res;
  gchar graphics_content[4];
  long chunk_start;

  if (init_len < 38 || total_len < 38)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Invalid general image attribute chunk size."));
      return -1;
    }

  chunk_start = ftell (f);
  if ((psp_ver_major >= 4
      && (fread (&init_len, 4, 1, f) < 1 || ((init_len = GUINT32_FROM_LE (init_len)) < 46)))
      || fread (&ia->width, 4, 1, f) < 1
      || fread (&ia->height, 4, 1, f) < 1
      || fread (&res, 8, 1, f) < 1
      || fread (&ia->metric, 1, 1, f) < 1
      || fread (&ia->compression, 2, 1, f) < 1
      || fread (&ia->depth, 2, 1, f) < 1
      || fread (buf, 2+4, 1, f) < 1 /* Skip plane and color count */
      || fread (&ia->grayscale, 1, 1, f) < 1
      || fread (buf, 4, 1, f) < 1 /* Skip total image size */
      || fread (&ia->active_layer, 4, 1, f) < 1
      || fread (&ia->layer_count, 2, 1, f) < 1
      || (psp_ver_major >= 4 && fread (graphics_content, 4, 1, f) < 1)
      || try_fseek (f, chunk_start + init_len, SEEK_SET, error) < 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading general image attribute block."));
      return -1;
    }
  ia->width = GUINT32_FROM_LE (ia->width);
  ia->height = GUINT32_FROM_LE (ia->height);

  res = GUINT64_FROM_LE (res);
  memcpy (&ia->resolution, &res, 8);
  if (ia->metric == PSP_METRIC_CM)
    ia->resolution /= 2.54;

  ia->compression = GUINT16_FROM_LE (ia->compression);
  if (ia->compression > PSP_COMP_LZ77)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unknown compression type %d"), ia->compression);
      return -1;
    }

  ia->depth = GUINT16_FROM_LE (ia->depth);
  switch (ia->depth)
    {
      case 24:
      case 48:
        ia->base_type = GIMP_RGB;
        if (ia->depth == 24)
          ia->precision = GIMP_PRECISION_U8_NON_LINEAR;
        else
          ia->precision = GIMP_PRECISION_U16_NON_LINEAR;
        break;

      case 1:
      case 4:
      case 8:
      case 16:
        if (ia->grayscale && ia->depth >= 8)
          {
            ia->base_type = GIMP_GRAY;
            if (ia->depth == 8)
              ia->precision = GIMP_PRECISION_U8_NON_LINEAR;
            else
              ia->precision = GIMP_PRECISION_U16_NON_LINEAR;
          }
        else if (ia->depth <= 8 && ! ia->grayscale)
          {
            ia->base_type = GIMP_INDEXED;
            ia->precision = GIMP_PRECISION_U8_NON_LINEAR;
          }
        else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Unsupported bit depth %d"), ia->depth);
          return -1;
        }
        break;

      default:
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Unsupported bit depth %d"), ia->depth);
        return -1;
        break;
    }

    if (ia->precision == GIMP_PRECISION_U16_NON_LINEAR)
      ia->bytes_per_sample = 2;
    else
      ia->bytes_per_sample = 1;

  ia->active_layer = GUINT32_FROM_LE (ia->active_layer);
  ia->layer_count = GUINT16_FROM_LE (ia->layer_count);

  return 0;
}

static gint
read_creator_block (FILE      *f,
                    GimpImage *image,
                    guint      total_len,
                    PSPimage  *ia,
                    GError   **error)
{
  long          data_start;
  guchar        buf[4];
  guint16       keyword;
  guint32       length;
  gchar        *string;
  gchar        *string2;
  gchar        *title = NULL, *artist = NULL, *copyright = NULL, *description = NULL;
  guint32       dword;
  guint32       __attribute__((unused))cdate = 0;
  guint32       __attribute__((unused))mdate = 0;
  guint32       __attribute__((unused))appid;
  guint32       __attribute__((unused))appver;
  GString      *comment;
  GimpParasite *comment_parasite;

  data_start = ftell (f);
  comment = g_string_new (NULL);

  while (ftell (f) < data_start + total_len)
    {
      if (fread (buf, 4, 1, f) < 1
          || fread (&keyword, 2, 1, f) < 1
          || fread (&length, 4, 1, f) < 1)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading creator keyword chunk"));
          return -1;
        }
      if (memcmp (buf, "~FL\0", 4) != 0)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Invalid keyword chunk header"));
          return -1;
        }
      keyword = GUINT16_FROM_LE (keyword);
      length = GUINT32_FROM_LE (length);
      switch (keyword)
        {
        case PSP_CRTR_FLD_TITLE:
        case PSP_CRTR_FLD_ARTIST:
        case PSP_CRTR_FLD_CPYRGHT:
        case PSP_CRTR_FLD_DESC:
          string = g_malloc (length + 1);
          if (fread (string, length, 1, f) < 1)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Error reading creator keyword data"));
              g_free (string);
              return -1;
            }
          /* PSP does not zero terminate strings */
          string[length] = '\0';
          string2 = string;
          /* Strings are in ASCII format according to PSP8 specs. */
          string = g_convert (string2, -1, "utf-8", "iso8859-1", NULL, NULL, NULL);
          if (string)
            g_free (string2);
          else
            string = string2;
          if (! g_utf8_validate (string, -1, NULL))
            {
              g_printerr ("Invalid creator keyword ignored.\n");
              break;
            }

          switch (keyword)
            {
            case PSP_CRTR_FLD_TITLE:
              g_free (title); title = string; break;
            case PSP_CRTR_FLD_ARTIST:
              g_free (artist); artist = string; break;
            case PSP_CRTR_FLD_CPYRGHT:
              g_free (copyright); copyright = string; break;
            case PSP_CRTR_FLD_DESC:
              g_free (description); description = string; break;
            default:
              g_free (string);
            }
          break;
        case PSP_CRTR_FLD_CRT_DATE:
        case PSP_CRTR_FLD_MOD_DATE:
        case PSP_CRTR_FLD_APP_ID:
        case PSP_CRTR_FLD_APP_VER:
          if (fread (&dword, 4, 1, f) < 1)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Error reading creator keyword data"));
              return -1;
            }
          switch (keyword)
            {
            case PSP_CRTR_FLD_CRT_DATE:
              cdate = dword; break;
            case PSP_CRTR_FLD_MOD_DATE:
              mdate = dword; break;
            case PSP_CRTR_FLD_APP_ID:
              appid = dword; break;
            case PSP_CRTR_FLD_APP_VER:
              appver = dword; break;
            }
          break;
        default:
          if (try_fseek (f, length, SEEK_CUR, error) < 0)
            {
              return -1;
            }
          break;
        }
    }

  if (title)
    {
      g_string_append (comment, title);
      g_free (title);
      g_string_append (comment, "\n");
    }
  if (artist)
    {
      g_string_append (comment, artist);
      g_free (artist);
      g_string_append (comment, "\n");
    }
  if (copyright)
    {
      g_string_append (comment, "Copyright ");
      g_string_append (comment, copyright);
      g_free (copyright);
      g_string_append (comment, "\n");
    }
  if (description)
    {
      g_string_append (comment, description);
      g_free (description);
      g_string_append (comment, "\n");
    }
  if (comment->len > 0)
    {
      comment_parasite = gimp_parasite_new ("gimp-comment",
                                            GIMP_PARASITE_PERSISTENT,
                                            strlen (comment->str) + 1,
                                            comment->str);
      gimp_image_attach_parasite (image, comment_parasite);
      gimp_parasite_free (comment_parasite);
    }

  g_string_free (comment, TRUE);

  return 0;
}

static gint
read_color_block (FILE      *f,
                  GimpImage *image,
                  guint      total_len,
                  PSPimage  *ia,
                  GError   **error)
{
  long     block_start;
  guint32  chunk_len, entry_count, pal_size;
  guint32  color_palette_entries;
  guchar  *color_palette;

  block_start = ftell (f);

  if (psp_ver_major >= 4)
    {
      if (fread (&chunk_len, 4, 1, f) < 1
          || fread (&entry_count, 4, 1, f) < 1)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading color block"));
          return -1;
        }

      chunk_len = GUINT32_FROM_LE (chunk_len);

      if (try_fseek (f, block_start + chunk_len, SEEK_SET, error) < 0)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading color block"));
          return -1;
        }
    }
  else
    {
      if (fread (&entry_count, 4, 1, f) < 1)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading color block"));
          return -1;
        }
    }

  color_palette_entries = GUINT32_FROM_LE (entry_count);
  /* TODO: GIMP currently only supports a maximum of 256 colors
   * in an indexed image. If this changes, we can change this check */
  if (color_palette_entries > 256)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error: Unsupported palette size"));
      return -1;
    }

  /* psp color palette entries are stored as RGBA so 4 bytes per entry
   * where the fourth bytes is always zero */
  pal_size = color_palette_entries * 4;
  color_palette = g_malloc (pal_size);
  if (fread (color_palette, pal_size, 1, f) < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading color palette"));
      return -1;
    }
  else
    {
      guchar *tmpmap;
      gint    i;

      /* Convert to BGR palette */
      tmpmap = g_malloc (3 * color_palette_entries);
      for (i = 0; i < color_palette_entries; ++i)
        {
          tmpmap[i*3  ] = color_palette[i*4+2];
          tmpmap[i*3+1] = color_palette[i*4+1];
          tmpmap[i*3+2] = color_palette[i*4];
        }

      memcpy (color_palette, tmpmap, color_palette_entries * 3);
      g_free (tmpmap);

      gimp_palette_set_colormap (gimp_image_get_palette (image), babl_format ("R'G'B' u8"), color_palette, color_palette_entries * 3);
      g_free (color_palette);
    }

  return 0;
}

static void inline
swab_rect (guint32 *rect)
{
  rect[0] = GUINT32_FROM_LE (rect[0]);
  rect[1] = GUINT32_FROM_LE (rect[1]);
  rect[2] = GUINT32_FROM_LE (rect[2]);
  rect[3] = GUINT32_FROM_LE (rect[3]);
}

static GimpLayerMode
gimp_layer_mode_from_psp_blend_mode (PSPBlendModes mode)
{
  switch (mode)
    {
    case PSP_BLEND_NORMAL:
      return GIMP_LAYER_MODE_NORMAL_LEGACY;

    case PSP_BLEND_DARKEN:
      return GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY;

    case PSP_BLEND_LIGHTEN:
      return GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY;

    case PSP_BLEND_HUE:
      return GIMP_LAYER_MODE_HSV_HUE_LEGACY;

    case PSP_BLEND_SATURATION:
      return GIMP_LAYER_MODE_HSV_SATURATION_LEGACY;

    case PSP_BLEND_COLOR:
      return GIMP_LAYER_MODE_HSL_COLOR_LEGACY;

    case PSP_BLEND_LUMINOSITY:
      return GIMP_LAYER_MODE_HSV_VALUE_LEGACY;   /* ??? */

    case PSP_BLEND_MULTIPLY:
      return GIMP_LAYER_MODE_MULTIPLY_LEGACY;

    case PSP_BLEND_SCREEN:
      return GIMP_LAYER_MODE_SCREEN_LEGACY;

    case PSP_BLEND_DISSOLVE:
      return GIMP_LAYER_MODE_DISSOLVE;

    case PSP_BLEND_OVERLAY:
      return GIMP_LAYER_MODE_OVERLAY;

    case PSP_BLEND_HARD_LIGHT:
      return GIMP_LAYER_MODE_HARDLIGHT_LEGACY;

    case PSP_BLEND_SOFT_LIGHT:
      return GIMP_LAYER_MODE_SOFTLIGHT_LEGACY;

    case PSP_BLEND_DIFFERENCE:
      return GIMP_LAYER_MODE_DIFFERENCE_LEGACY;

    case PSP_BLEND_DODGE:
      return GIMP_LAYER_MODE_DODGE_LEGACY;

    case PSP_BLEND_BURN:
      return GIMP_LAYER_MODE_BURN_LEGACY;

    case PSP_BLEND_EXCLUSION:
      return GIMP_LAYER_MODE_EXCLUSION;

    case PSP_BLEND_TRUE_HUE:
      return GIMP_LAYER_MODE_HSV_HUE;

    case PSP_BLEND_TRUE_SATURATION:
      return GIMP_LAYER_MODE_HSV_SATURATION;

    case PSP_BLEND_TRUE_COLOR:
      return GIMP_LAYER_MODE_HSL_COLOR;

    case PSP_BLEND_TRUE_LIGHTNESS:
      return GIMP_LAYER_MODE_HSV_VALUE;

    case PSP_BLEND_ADJUST:
      return -1;                /* ??? */
    }
  return -1;
}

static const gchar *
blend_mode_name (PSPBlendModes mode)
{
  static const gchar *blend_mode_names[] =
  {
    "NORMAL",
    "DARKEN",
    "LIGHTEN",
    "HUE",
    "SATURATION",
    "COLOR",
    "LUMINOSITY",
    "MULTIPLY",
    "SCREEN",
    "DISSOLVE",
    "OVERLAY",
    "HARD_LIGHT",
    "SOFT_LIGHT",
    "DIFFERENCE",
    "DODGE",
    "BURN",
    "EXCLUSION",
    "TRUE HUE",
    "TRUE SATURATION",
    "TRUE COLOR",
    "TRUE LIGHTNESS",
    /* ADJUST should always be the last one. */
    "ADJUST"
  };
  static gchar *err_name = NULL;

  if (mode <= PSP_BLEND_TRUE_LIGHTNESS)
    return blend_mode_names[mode];
  else if (mode == PSP_BLEND_ADJUST)
    return blend_mode_names[PSP_BLEND_TRUE_LIGHTNESS+1];

  g_free (err_name);
  err_name = g_strdup_printf ("unknown layer blend mode %d", mode);

  return err_name;
}

static const gchar *
layer_type_name (PSPLayerTypePSP6 type)
{
  static const gchar *layer_type_names[] =
  {
    "Undefined",
    "Raster",
    "Floating Raster Selection",
    "Vector",
    "Adjustment",
    "Group",
    "Mask",
    "Art Media",
  };
  static gchar *err_name = NULL;

  if (type <= keGLTArtMedia)
    return layer_type_names[type];

  g_free (err_name);
  err_name = g_strdup_printf ("unknown layer type %d", type);

  return err_name;
}

static const gchar *
bitmap_type_name (gint type)
{
  static const gchar *bitmap_type_names[] =
  {
    "IMAGE",
    "TRANS_MASK",
    "USER_MASK",
    "SELECTION",
    "ALPHA_MASK",
    "THUMBNAIL"
  };
  static gchar *err_name = NULL;

  if (type >= 0 && type <= PSP_DIB_THUMBNAIL)
    return bitmap_type_names[type];

  g_free (err_name);
  err_name = g_strdup_printf ("unknown bitmap type %d", type);

  return err_name;
}

static const gchar *
channel_type_name (gint type)
{
  static const gchar *channel_type_names[] =
  {
    "COMPOSITE",
    "RED",
    "GREEN",
    "BLUE"
  };
  static gchar *err_name = NULL;

  if (type >= 0 && type <= PSP_CHANNEL_BLUE)
    return channel_type_names[type];

  g_free (err_name);
  err_name = g_strdup_printf ("unknown channel type %d", type);

  return err_name;
}

static void *
psp_zalloc (void  *opaque,
            guint  items,
            guint  size)
{
  return g_malloc (items*size);
}

static void
psp_zfree (void *opaque,
           void *ptr)
{
  g_free (ptr);
}

static void
upscale_indexed_sub_8 (FILE    *f,
                       gint     width,
                       gint     height,
                       gint     bpp,
                       guchar  *buf)
{
  gint     x, y, b, line_width;
  gint     bpp_zero_based = bpp - 1;
  gint     current_bit = 0;
  guchar  *tmpbuf, *buf_start, *src;

  /* Scanlines for 1 and 4 bit only end on a 4-byte boundary. */
  line_width = (((width * bpp + 7) / 8) + bpp_zero_based) / 4 * 4;
  buf_start = g_malloc0 (width * height);
  tmpbuf = buf_start;

  for (y = 0; y < height; tmpbuf += width, ++y)
    {
      src = buf + y * line_width;
      for (x = 0; x < width; ++x)
        {
          for (b = 0; b < bpp; b++)
            {
              current_bit = bpp * x + b;
              if (src[current_bit / 8] & (128 >> (current_bit % 8)))
                tmpbuf[x] += (1 << (bpp_zero_based - b));
            }
        }
    }

  memcpy (buf, buf_start, width * height);
  g_free (buf_start);
}

static int
read_channel_data (FILE        *f,
                   PSPimage    *ia,
                   guchar     **pixels,
                   guint        bytespp,
                   guint        offset,
                   GeglBuffer  *buffer,
                   guint32      compressed_len,
                   GError     **error)
{
  gint i, y, line_width;
  gint width = gegl_buffer_get_width (buffer);
  gint height = gegl_buffer_get_height (buffer);
  gint npixels = width * height;
  guchar *buf;
  guchar *buf2 = NULL;  /* please the compiler */
  guchar runcount, byte;
  z_stream zstream;

  g_assert (ia->bytes_per_sample <= 2);

  if (ia->depth < 8)
    {
      /* Scanlines for 1 and 4 bit only end on a 4-byte boundary. */
      line_width = (((width * ia->depth + 7) / 8) + ia->depth - 1) / 4 * 4;
    }
  else
    {
      line_width = width * ia->bytes_per_sample;
    }

  switch (ia->compression)
    {
    case PSP_COMP_NONE:
      if (bytespp == 1)
        {
          fread (pixels[0], height * line_width, 1, f);
        }
      else
        {
          buf = g_malloc (line_width);
          if (ia->bytes_per_sample == 1)
            {
              for (y = 0; y < height; y++)
                {
                  guchar *p, *q;

                  fread (buf, width, 1, f);
                  /* Contrary to what the PSP specification seems to suggest
                    scanlines are not stored on a 4-byte boundary. */
                  p = buf;
                  q = pixels[y] + offset;
                  for (i = 0; i < width; i++)
                    {
                      *q = *p++;
                      q += bytespp;
                    }
                }
            }
          else if (ia->bytes_per_sample == 2)
            {
              for (y = 0; y < height; y++)
                {
                  guint16 *p, *q;

                  fread (buf, width * ia->bytes_per_sample, 1, f);
                  /* Contrary to what the PSP specification seems to suggest
                    scanlines are not stored on a 4-byte boundary. */
                  p = (guint16 *) buf;
                  q = (guint16 *) (pixels[y] + offset);
                  for (i = 0; i < width; i++)
                    {
                      *q = GUINT16_FROM_LE (*p++);
                      q += bytespp / 2;
                    }
                }
            }

          g_free (buf);
        }
      break;

    case PSP_COMP_RLE:
      {
        guchar *q, *endq;

        q = pixels[0] + offset;
        if (ia->depth >= 8)
          endq = q + npixels * bytespp;
        else
          endq = q + line_width * height;

        buf = g_malloc (128);
        while (q < endq)
          {
            fread (&runcount, 1, 1, f);
            if (runcount > 128)
              {
                runcount -= 128;
                fread (&byte, 1, 1, f);
                memset (buf, byte, runcount);
              }
            else
              fread (buf, runcount, 1, f);

            /* prevent buffer overflow for bogus data */
            if (runcount > (endq - q) / bytespp + ia->bytes_per_sample - 1)
              {
                g_printerr ("Buffer overflow decompressing RLE data.\n");
                break;
              }

            if (bytespp == 1)
              {
                memmove (q, buf, runcount);
                q += runcount;
              }
            else if (ia->bytes_per_sample == 1)
              {
                guchar *p = buf;

                for (i = 0; i < runcount; i++)
                  {
                    *q = *p++;
                    q += bytespp;
                  }
              }
            else if (ia->bytes_per_sample == 2)
              {
                guint16 *p = (guint16 *) buf;
                guint16 *r = (guint16 *) q;

                for (i = 0; i < runcount / 2; i++)
                  {
                    *r = GUINT16_FROM_LE (*p++);
                    r += bytespp / 2;
                  }
                q = (guchar *) r;
              }
          }
        g_free (buf);
      }
      break;

    case PSP_COMP_LZ77:
      buf = g_malloc (compressed_len);
      fread (buf, compressed_len, 1, f);
      zstream.next_in = buf;
      zstream.avail_in = compressed_len;
      zstream.zalloc = psp_zalloc;
      zstream.zfree = psp_zfree;
      zstream.opaque = f;
      if (inflateInit (&zstream) != Z_OK)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("zlib error"));
          return -1;
        }
      if (bytespp == 1)
        zstream.next_out = pixels[0];
      else
        {
          buf2 = g_malloc (npixels * ia->bytes_per_sample);
          zstream.next_out = buf2;
        }
      zstream.avail_out = npixels * ia->bytes_per_sample;
      if (inflate (&zstream, Z_FINISH) != Z_STREAM_END)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("zlib error"));
          inflateEnd (&zstream);
          return -1;
        }
      inflateEnd (&zstream);
      g_free (buf);

      if (bytespp > 1)
        {
          if (ia->bytes_per_sample == 1)
            {
              guchar *p, *q;

              p = buf2;
              q = pixels[0] + offset;
              for (i = 0; i < npixels; i++)
                {
                  *q = *p++;
                  q += bytespp;
                }
              g_free (buf2);
            }
          else if (ia->bytes_per_sample == 2)
            {
              guint16 *p, *q;

              p = (guint16 *) buf2;
              q = (guint16 *) (pixels[0] + offset);
              for (i = 0; i < npixels; i++)
                {
                  *q = GUINT16_FROM_LE (*p++);
                  q += bytespp / 2;
                }
              g_free (buf2);
            }
        }
      break;
    }

  if (ia->base_type == GIMP_INDEXED && ia->depth < 8)
    {
      /* We need to convert 1 and 4 bit to 8 bit indexed */
      upscale_indexed_sub_8 (f, width, height, ia->depth, pixels[0]);
    }

  return 0;
}

static gboolean
read_raster_layer_info (FILE      *f,
                        long       layer_extension_start,
                        guint16   *bitmap_count,
                        guint16   *channel_count,
                        GError   **error)
{
  long    block_start;
  guint32 layer_extension_len, block_len;
  gint    block_id;

  if (fseek (f, layer_extension_start, SEEK_SET) < 0
      || fread (&layer_extension_len, 4, 1, f) < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading layer extension information"));
      return FALSE;
    }

    /* Newer versions of PSP have an extra block here for raster layers
       with block id = 0x21. Most likely this was to fix an oversight in
       the specification since vector and adjustment layers already were
       using a similar extension block since file version 4.
       The old chunk with bitmap_count and channel_count starts after this block.
       We do not know starting from which version this change was implemented
       but most likely version 9 (could also be version 10) so we can't test
       based on version number only.
       Although this is kind of a hack we can safely test for the block starting
       code since the layer_extension_len here is always a small number.
      */
    if (psp_ver_major > 8 && memcmp (&layer_extension_len, "~BK\0", 4) == 0)
      {
        if (fread (&block_id, 2, 1, f) < 1
            || fread (&block_len, 4, 1, f) < 1)
          {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("Error reading block information"));
            return FALSE;
          }
        block_id = GUINT16_FROM_LE (block_id);
        block_len = GUINT32_FROM_LE (block_len);

        block_start = ftell (f);
        layer_extension_start = block_start + block_len;

        if (fseek (f, layer_extension_start, SEEK_SET) < 0
            || fread (&layer_extension_len, 4, 1, f) < 1)
          {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("Error reading layer extension information"));
            return FALSE;
          }
      }
    layer_extension_len = GUINT32_FROM_LE (layer_extension_len);

    if (fread (bitmap_count, 2, 1, f) < 1
        || fread (channel_count, 2, 1, f) < 1)
      {
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     _("Error reading layer extension information"));
        return FALSE;
      }
    if (try_fseek (f, layer_extension_start + layer_extension_len, SEEK_SET, error) < 0)
      {
        return FALSE;
      }

  return TRUE;
}

static GimpLayer *
read_layer_block (FILE      *f,
                  GimpImage *image,
                  guint      total_len,
                  PSPimage  *ia,
                  GError   **error)
{
  gint i;
  long block_start, sub_block_start, channel_start;
  long layer_extension_start;
  gint sub_id;
  guint32 sub_init_len, sub_total_len;
  guint32 chunk_len;
  gchar *name = NULL;
  gchar *layer_name = NULL;
  guint16 namelen;
  guchar type, opacity, blend_mode, visibility, transparency_protected;
  guchar link_group_id, mask_linked, mask_disabled;
  guint32 image_rect[4], saved_image_rect[4], mask_rect[4], saved_mask_rect[4];
  gboolean null_layer, can_handle_layer;
  guint16 bitmap_count, channel_count;
  GimpImageType drawable_type;
  GimpLayer *layer = NULL;
  GimpLayerMode layer_mode;
  guint32 channel_init_len, channel_total_len;
  guint32 compressed_len, uncompressed_len;
  guint16 bitmap_type, channel_type;
  gint width, height, bytespp, offset;
  guchar **pixels, *pixel;
  GeglBuffer *buffer;

  block_start = ftell (f);

  while (ftell (f) < block_start + total_len)
    {
      null_layer = FALSE;
      can_handle_layer = FALSE;

      /* Read the layer sub-block header */
      sub_id = read_block_header (f, &sub_init_len, &sub_total_len, error);
      if (sub_id == -1)
        return NULL;

      if (sub_id != PSP_LAYER_BLOCK)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Invalid layer sub-block %s, should be LAYER"),
                       block_name (sub_id));
          return NULL;
        }

      sub_block_start = ftell (f);

      /* Read layer information chunk */
      if (psp_ver_major >= 4)
        {
          if (fread (&chunk_len, 4, 1, f) < 1
              || fread (&namelen, 2, 1, f) < 1
              /* A zero length layer name is apparently valid. To not get a warning for
                 namelen < 0 always being false we use this more complicated comparison. */
              || ((namelen = GUINT16_FROM_LE (namelen)) && (FALSE || namelen == 0))
              || (name = g_malloc (namelen + 1)) == NULL
              || (namelen > 0 && fread (name, namelen, 1, f) < 1)
              || fread (&type, 1, 1, f) < 1
              || fread (&image_rect, 16, 1, f) < 1
              || fread (&saved_image_rect, 16, 1, f) < 1
              || fread (&opacity, 1, 1, f) < 1
              || fread (&blend_mode, 1, 1, f) < 1
              || fread (&visibility, 1, 1, f) < 1
              || fread (&transparency_protected, 1, 1, f) < 1
              || fread (&link_group_id, 1, 1, f) < 1
              || fread (&mask_rect, 16, 1, f) < 1
              || fread (&saved_mask_rect, 16, 1, f) < 1
              || fread (&mask_linked, 1, 1, f) < 1
              || fread (&mask_disabled, 1, 1, f) < 1)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Error reading layer information chunk"));
              g_free (name);
              return NULL;
            }

          name[namelen] = 0;
          layer_name = g_convert (name, -1, "utf-8", "iso8859-1", NULL, NULL, NULL);
          g_free (name);

          chunk_len = GUINT32_FROM_LE (chunk_len);
          layer_extension_start = sub_block_start + chunk_len;

          switch (type)
            {
            case keGLTFloatingRasterSelection:
              g_message ("Floating selection restored as normal layer (%s)", layer_name);
            case keGLTRaster:
              if (! read_raster_layer_info (f, layer_extension_start,
                                            &bitmap_count,
                                            &channel_count,
                                            error))
                {
                  g_free (layer_name);
                  return NULL;
                }
              can_handle_layer = TRUE;
              break;
            default:
              bitmap_count = 0;
              channel_count = 0;
              g_message ("Unsupported layer type %s (%s)", layer_type_name(type), layer_name);
              break;
            }
        }
      else
        {
          name = g_malloc (257);
          name[256] = 0;

          if (fread (name, 256, 1, f) < 1
              || fread (&type, 1, 1, f) < 1
              || fread (&image_rect, 16, 1, f) < 1
              || fread (&saved_image_rect, 16, 1, f) < 1
              || fread (&opacity, 1, 1, f) < 1
              || fread (&blend_mode, 1, 1, f) < 1
              || fread (&visibility, 1, 1, f) < 1
              || fread (&transparency_protected, 1, 1, f) < 1
              || fread (&link_group_id, 1, 1, f) < 1
              || fread (&mask_rect, 16, 1, f) < 1
              || fread (&saved_mask_rect, 16, 1, f) < 1
              || fread (&mask_linked, 1, 1, f) < 1
              || fread (&mask_disabled, 1, 1, f) < 1
              || fseek (f, 43, SEEK_CUR) < 0
              || fread (&bitmap_count, 2, 1, f) < 1
              || fread (&channel_count, 2, 1, f) < 1)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Error reading layer information chunk"));
              g_free (name);
              return NULL;
            }
          layer_name = g_convert (name, -1, "utf-8", "iso8859-1", NULL, NULL, NULL);
          g_free (name);
          if (type == PSP_LAYER_FLOATING_SELECTION)
            g_message ("Floating selection restored as normal layer");
          type = keGLTRaster;
          can_handle_layer = TRUE;
          if (try_fseek (f, sub_block_start + sub_init_len, SEEK_SET, error) < 0)
            {
              g_free (layer_name);
              return NULL;
            }
        }

      swab_rect (image_rect);
      swab_rect (saved_image_rect);
      swab_rect (mask_rect);
      swab_rect (saved_mask_rect);
      bitmap_count = GUINT16_FROM_LE (bitmap_count);
      channel_count = GUINT16_FROM_LE (channel_count);

      layer_mode = gimp_layer_mode_from_psp_blend_mode (blend_mode);
      if ((int) layer_mode == -1)
        {
          g_message ("Unsupported PSP layer blend mode %s "
                     "for layer %s, setting layer invisible",
                     blend_mode_name (blend_mode), layer_name);
          layer_mode = GIMP_LAYER_MODE_NORMAL_LEGACY;
          visibility = FALSE;
        }

      width = saved_image_rect[2] - saved_image_rect[0];
      height = saved_image_rect[3] - saved_image_rect[1];

      if ((width < 0) || (width > GIMP_MAX_IMAGE_SIZE)       /* w <= 2^18 */
          || (height < 0) || (height > GIMP_MAX_IMAGE_SIZE)  /* h <= 2^18 */
          || ((width / 256) * (height / 256) >= 8192))       /* w * h < 2^29 */
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Invalid layer dimensions: %dx%d"),
                       width, height);
          g_free (layer_name);
          return NULL;
        }

      IFDBG(2) g_message
        ("layer: %s %dx%d (%dx%d) @%d,%d opacity %d blend_mode %s "
         "%d bitmaps %d channels",
         layer_name,
         image_rect[2] - image_rect[0], image_rect[3] - image_rect[1],
         width, height,
         image_rect[0]+saved_image_rect[0], image_rect[1]+saved_image_rect[1],
         opacity, blend_mode_name (blend_mode),
         bitmap_count, channel_count);

      IFDBG(2) g_message
        ("mask %dx%d (%dx%d) @%d,%d",
         mask_rect[2] - mask_rect[0],
         mask_rect[3] - mask_rect[1],
         saved_mask_rect[2] - saved_mask_rect[0],
         saved_mask_rect[3] - saved_mask_rect[1],
         saved_mask_rect[0], saved_mask_rect[1]);

      if (width == 0)
        {
          width++;
          null_layer = TRUE;
        }
      if (height == 0)
        {
          height++;
          null_layer = TRUE;
        }

      if (ia->base_type == GIMP_RGB)
        if (bitmap_count == 1)
          drawable_type = GIMP_RGB_IMAGE, bytespp = 3;
        else
          drawable_type = GIMP_RGBA_IMAGE, bytespp = 4;
      else if (ia->base_type == GIMP_GRAY)
        if (bitmap_count == 1)
          drawable_type = GIMP_GRAY_IMAGE, bytespp = 1;
        else
          drawable_type = GIMP_GRAYA_IMAGE, bytespp = 2;
      else
        if (bitmap_count == 1)
          drawable_type = GIMP_INDEXED_IMAGE, bytespp = 1;
        else
          drawable_type = GIMP_INDEXEDA_IMAGE, bytespp = 2;
      bytespp *= ia->bytes_per_sample;

      layer = gimp_layer_new (image, layer_name,
                              width, height,
                              drawable_type,
                              100.0 * opacity / 255.0,
                              layer_mode);
      g_free (layer_name);
      if (! layer)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error creating layer"));
          return NULL;
        }

      gimp_image_insert_layer (image, layer, NULL, -1);

      if (image_rect[0] != 0 || image_rect[1] != 0 || saved_image_rect[0] != 0 || saved_image_rect[1] != 0)
        gimp_layer_set_offsets (layer,
                                image_rect[0] + saved_image_rect[0], image_rect[1] + saved_image_rect[1]);

      if (! visibility)
        gimp_item_set_visible (GIMP_ITEM (layer), FALSE);

      gimp_layer_set_lock_alpha (layer, transparency_protected);

      if (can_handle_layer)
        {
          pixel = g_malloc0 (height * width * bytespp);
          if (null_layer)
            {
              pixels = NULL;
            }
          else
            {
              pixels = g_new (guchar *, height);
              for (i = 0; i < height; i++)
                pixels[i] = pixel + width * bytespp * i;
            }

          buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

          /* Read the layer channel sub-blocks */
          while (ftell (f) < sub_block_start + sub_total_len)
            {
              sub_id = read_block_header (f, &channel_init_len,
                                          &channel_total_len, error);
              if (sub_id == -1)
                {
                  gimp_image_delete (image);
                  return NULL;
                }

              if (sub_id != PSP_CHANNEL_BLOCK)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Invalid layer sub-block %s, should be CHANNEL"),
                              block_name (sub_id));
                  return NULL;
                }

              channel_start = ftell (f);
              chunk_len = channel_init_len; /* init chunk_len for psp_ver_major == 3 */
              if ((psp_ver_major >= 4
                  && (fread (&chunk_len, 4, 1, f) < 1
                  || ((chunk_len = GUINT32_FROM_LE (chunk_len)) < 16)))
                  || fread (&compressed_len, 4, 1, f) < 1
                  || fread (&uncompressed_len, 4, 1, f) < 1
                  || fread (&bitmap_type, 2, 1, f) < 1
                  || fread (&channel_type, 2, 1, f) < 1)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Error reading channel information chunk"));
                  return NULL;
                }

              compressed_len = GUINT32_FROM_LE (compressed_len);
              uncompressed_len = GUINT32_FROM_LE (uncompressed_len);
              bitmap_type = GUINT16_FROM_LE (bitmap_type);
              channel_type = GUINT16_FROM_LE (channel_type);

              if (bitmap_type > PSP_DIB_USER_MASK)
                {
                  g_message ("Conversion of bitmap type %d is not supported.", bitmap_type);
                }
              else if (bitmap_type == PSP_DIB_USER_MASK)
                {
                  /* FIXME: Add as layer mask */
                  g_message ("Conversion of layer mask is not supported");
                }
              else
                {
                  if (channel_type > PSP_CHANNEL_BLUE)
                    {
                      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                  _("Invalid channel type %d in channel information chunk"),
                                  channel_type);
                      return NULL;
                    }

                  IFDBG(2) g_message ("channel: %s %s %d (%d) bytes %d bytespp",
                                      bitmap_type_name (bitmap_type),
                                      channel_type_name (channel_type),
                                      uncompressed_len, compressed_len,
                                      bytespp);

                  if (bitmap_type == PSP_DIB_TRANS_MASK || channel_type == PSP_CHANNEL_COMPOSITE)
                    offset = bytespp - ia->bytes_per_sample;
                  else
                    offset = (channel_type - PSP_CHANNEL_RED) * ia->bytes_per_sample;

                  if (!null_layer)
                    {
                      if (try_fseek (f, channel_start + chunk_len, SEEK_SET, error) < 0)
                        {
                          return NULL;
                        }

                      if (read_channel_data (f, ia, pixels, bytespp, offset,
                                            buffer, compressed_len, error) == -1)
                        {
                          return NULL;
                        }
                    }
                }
              if (try_fseek (f, channel_start + channel_total_len, SEEK_SET, error) < 0)
                {
                  return NULL;
                }
            }

          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                          NULL, pixel, GEGL_AUTO_ROWSTRIDE);

          g_object_unref (buffer);

          g_free (pixels);
          g_free (pixel);
          if (psp_ver_major >= 4)
            {
              if (try_fseek (f, sub_block_start + sub_total_len, SEEK_SET, error) < 0)
                {
                  return NULL;
                }
            }
        }
      else
        {
          /* Can't handle this type of layer, skip the data so we can read the next layer. */
          if (psp_ver_major >= 4)
            {
              if (try_fseek (f, sub_block_start + sub_total_len, SEEK_SET, error) < 0)
                {
                  return NULL;
                }
            }
        }
    }

  if (try_fseek (f, block_start + total_len, SEEK_SET, error) < 0)
    {
      return NULL;
    }

  return layer;
}

static gint
read_tube_block (FILE      *f,
                 GimpImage *image,
                 guint      total_len,
                 PSPimage  *ia,
                 GError   **error)
{
  guint16            version;
  guchar             name[514];
  guint32            step_size, column_count, row_count, cell_count;
  guint32            placement_mode, selection_mode;
  guint32            chunk_len;
  gint               i;
  GimpPixPipeParams  params;
  GimpParasite      *pipe_parasite;
  gchar             *parasite_text;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gimp_pixpipe_params_init (&params);
  G_GNUC_END_IGNORE_DEPRECATIONS

  if (psp_ver_major >= 4)
    {
      name[0] = 0;
      if (fread (&chunk_len, 4, 1, f) < 1
          || fread (&version, 2, 1, f) < 1
          || fread (&step_size, 4, 1, f) < 1
          || fread (&column_count, 4, 1, f) < 1
          || fread (&row_count, 4, 1, f) < 1
          || fread (&cell_count, 4, 1, f) < 1
          || fread (&placement_mode, 4, 1, f) < 1
          || fread (&selection_mode, 4, 1, f) < 1)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading tube data chunk"));
          return -1;
        }
    }
  else
    {
      chunk_len = 0;
      if (fread (&version, 2, 1, f) < 1
          || fread (name, 513, 1, f) < 1
          || fread (&step_size, 4, 1, f) < 1
          || fread (&column_count, 4, 1, f) < 1
          || fread (&row_count, 4, 1, f) < 1
          || fread (&cell_count, 4, 1, f) < 1
          || fread (&placement_mode, 4, 1, f) < 1
          || fread (&selection_mode, 4, 1, f) < 1)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading tube data chunk"));
          return -1;
        }
      name[513] = 0;
    }

  version = GUINT16_FROM_LE (version);
  params.step = GUINT32_FROM_LE (step_size);
  params.cols = GUINT32_FROM_LE (column_count);
  params.rows = GUINT32_FROM_LE (row_count);
  params.ncells = GUINT32_FROM_LE (cell_count);
  placement_mode = GUINT32_FROM_LE (placement_mode);
  selection_mode = GUINT32_FROM_LE (selection_mode);

  for (i = 1; i < params.cols; i++)
    gimp_image_add_vguide (image, (ia->width * i)/params.cols);
  for (i = 1; i < params.rows; i++)
    gimp_image_add_hguide (image, (ia->height * i)/params.rows);

  /* We use a parasite to pass in the tube (pipe) parameters in
   * case we will have any use of those, for instance in the gpb
   * plug-in that exports a GIMP image pipe.
   */
  params.dim = 1;
  params.cellwidth = ia->width / params.cols;
  params.cellheight = ia->height / params.rows;
  params.placement = (placement_mode == tpmRandom ? "random" :
                      (placement_mode == tpmConstant ? "constant" :
                       "default"));
  params.rank[0] = params.ncells;
  params.selection[0] = (selection_mode == tsmRandom ? "random" :
                         (selection_mode == tsmIncremental ? "incremental" :
                          (selection_mode == tsmAngular ? "angular" :
                           (selection_mode == tsmPressure ? "pressure" :
                            (selection_mode == tsmVelocity ? "velocity" :
                             "default")))));
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  parasite_text = gimp_pixpipe_params_build (&params);
  G_GNUC_END_IGNORE_DEPRECATIONS

  IFDBG(2) g_message ("parasite: %s", parasite_text);

  pipe_parasite = gimp_parasite_new ("gimp-brush-pipe-parameters",
                                     GIMP_PARASITE_PERSISTENT,
                                     strlen (parasite_text) + 1, parasite_text);
  gimp_image_attach_parasite (image, pipe_parasite);
  gimp_parasite_free (pipe_parasite);
  g_free (parasite_text);

  return 0;
}

static const gchar *
compression_name (gint compression)
{
  switch (compression)
    {
    case PSP_COMP_NONE:
      return "no compression";
    case PSP_COMP_RLE:
      return "RLE";
    case PSP_COMP_LZ77:
      return "LZ77";
    }
  g_assert_not_reached ();

  return NULL;
}

static gint
read_selection_block (FILE      *f,
                      GimpImage *image,
                      guint      total_len,
                      PSPimage  *ia,
                      GError   **error)
{
  gsize   current_location;
  gsize   file_size;
  guint32 chunk_size;
  guint32 rect[4];

  current_location = ftell (f);
  fseek (f, 0, SEEK_END);
  file_size = ftell (f);
  fseek (f, current_location, SEEK_SET);

  if (fread (&chunk_size, 4, 1, f) < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading selection chunk"));
      return -1;
    }
  chunk_size = GUINT32_FROM_LE (chunk_size);

  current_location = ftell (f);
  if (chunk_size > (file_size - current_location))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Invalid selection chunk size"));
      return -1;
    }

  if (fread (&rect, 16, 1, f) < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading selection chunk"));
      return -1;
    }

  swab_rect (rect);
  gimp_image_select_rectangle (image, GIMP_CHANNEL_OP_ADD,
                               rect[0], rect[1],
                               rect[2] - rect[0],
                               rect[3] - rect[1]);

  /* The file format specifies multiple selections, but
   * as of PSP 8 they only allow one. Skipping the remaining
   * information in the block. */
  total_len -= sizeof (guint32) + sizeof (rect);
  if (try_fseek (f, total_len, SEEK_SET, error) < 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading end of selection chunk"));
      return -1;
    }

  return 0;
}

static gint
read_extended_block (FILE      *f,
                     GimpImage *image,
                     guint      total_len,
                     PSPimage  *ia,
                     GError   **error)
{
  guchar   header[4];
  guint16  field_type;
  guint32  field_length;

  while (total_len > 0)
    {
      if (fread (header, 4, 1, f) < 1      ||
          fread (&field_type, 2, 1, f) < 1 ||
          fread (&field_length, 4, 1, f) < 1)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error reading extended block chunk header"));
          return -1;
        }

      if (memcmp (header, "~FL\0", 4) != 0)
        {
          g_print ("Header: %s\n", header);
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Invalid extended block chunk header"));
          return -1;
        }
      total_len -= sizeof (header) + sizeof (field_type) + sizeof (field_length);

      field_type   = GUINT16_FROM_LE (field_type);
      field_length = GUINT32_FROM_LE (field_length);

      if (field_length > total_len)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Invalid extended block chunk size"));
          return -1;
        }

      switch (field_type)
        {
        case PSP_XDATA_GRID:
          {
            GeglColor *color;
            guchar     rgb[4];
            guint32    h_spacing;
            guint32    v_spacing;
            guint16    unit; /* Unused */

            if (field_length != 14              ||
                fread (rgb, 4, 1, f) < 1        ||
                fread (&h_spacing, 4, 1, f) < 1 ||
                fread (&v_spacing, 4, 1, f) < 1 ||
                fread (&unit, 2, 1, f) < 1)
              {
                g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Error reading extended chunk grid data"));
                return -1;
              }

            h_spacing = GUINT32_FROM_LE (h_spacing);
            v_spacing = GUINT32_FROM_LE (v_spacing);
            gimp_image_grid_set_spacing (image, h_spacing, v_spacing);

            color = gegl_color_new (NULL);
            gegl_color_set_pixel (color, babl_format ("R'G'B' u8"), rgb);
            gimp_image_grid_set_foreground_color (image, color);
            gimp_image_grid_set_background_color (image, color);
            g_object_unref (color);
          }
          break;

        case PSP_XDATA_GUIDE:
          {
            guchar  rgb[4]; /* Unused */
            guint32 offset;
            guint16 orientation;

            if (field_length != 10           ||
                fread (rgb, 4, 1, f) < 1     ||
                fread (&offset, 4, 1, f) < 1 ||
                fread (&orientation, 2, 1, f) < 1)
              {
                g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Error reading extended chunk guide data"));
                return -1;
              }

            offset      = GUINT32_FROM_LE (offset);
            orientation = GUINT16_FROM_LE (orientation);

            if (orientation == keHorizontalGuide)
              {
                gimp_image_add_hguide (image, offset);
              }
            else if (orientation == keVerticalGuide)
              {
                gimp_image_add_vguide (image, offset);
              }
            else
              {
                g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                             _("Invalid guide orientation"));
                return -1;
              }
          }
          break;

        /* Not yet implemented */
        case PSP_XDATA_TRNS_INDEX:
        case PSP_XDATA_EXIF:
        case PSP_XDATA_IPTC:
        default:
          if (try_fseek (f, field_length, SEEK_CUR, error) < 0)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Error reading extended block chunk"));
              return -1;
            }
          break;
        }
      total_len -= field_length;
    }

  return 0;
}

static gint
read_colorprofile_block (FILE      *f,
                         GimpImage *image,
                         guint      total_len,
                         PSPimage  *ia,
                         GError   **error)
{
  guint             psp_header_size;
  guint             icc_profile_size;
  gchar            *icc_profile;
  GimpColorProfile *profile;

  if (fread (&psp_header_size, 4, 1, f) < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading colorprofile chunk"));
      return -1;
    }
  psp_header_size = GUINT32_FROM_LE (psp_header_size);

  /* The next part is a 2 byte length value, followed by PSP's
   * internal name for the profile. We'll skip most of it,
   * except for the last 4 bytes which is the profile size */
  if (try_fseek (f, psp_header_size - 8, SEEK_CUR, error) < 0 ||
      fread (&icc_profile_size, 4, 1, f) < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading colorprofile chunk"));
      return -1;
    }
  icc_profile_size = GUINT32_FROM_LE (icc_profile_size);

  /* The rest of the code references load_resource_1039 ()
   * in psd-image-res-load.c */
  icc_profile = g_malloc (icc_profile_size);
  if (fread (icc_profile, icc_profile_size, 1, f) < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading colorprofile chunk"));
      return -1;
    }

  profile = gimp_color_profile_new_from_icc_profile ((guint8 *) icc_profile,
                                                     icc_profile_size,
                                                     NULL);
  gimp_image_set_color_profile (image, profile);

  g_object_unref (profile);
  g_free (icc_profile);

  return 0;
}

/* The main function for loading PSP-images
 */
static GimpImage *
load_image (GFile   *file,
            GError **error)
{
  FILE      *f;
  GStatBuf   st;
  char       buf[32];
  PSPimage   ia;
  guint32    block_init_len, block_total_len;
  long       block_start;
  PSPBlockID id = -1;
  gint       block_number;

  GimpImage *image = NULL;

  if (g_stat (g_file_peek_path (file), &st) == -1)
    {
      return NULL;
    }

  f = g_fopen (g_file_peek_path (file), "rb");

  if (! f)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  /* Read the PSP File Header and determine file version */
  if (fread (buf, 32, 1, f) < 1
      || fread (&psp_ver_major, 2, 1, f) < 1
      || fread (&psp_ver_minor, 2, 1, f) < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error reading file header."));
      goto error;
    }

  if (memcmp (buf, "Paint Shop Pro Image File\n\032\0\0\0\0\0", 32) != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Incorrect file signature."));
      goto error;
    }

  psp_ver_major = GUINT16_FROM_LE (psp_ver_major);
  psp_ver_minor = GUINT16_FROM_LE (psp_ver_minor);

  /* We don't have the documentation for file format versions before 3.0,
   * but newer versions should be mostly backwards compatible so that
   * we can still read the image and skip unknown parts safely.
   */
  if (psp_ver_major < 3)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported PSP file format version %d.%d."),
                   psp_ver_major, psp_ver_minor);
      goto error;
    }

  /* Read all the blocks */
  block_number = 0;

  IFDBG(3) g_message ("size = %d", (int)st.st_size);
  while (ftell (f) != st.st_size
         && (id = read_block_header (f, &block_init_len,
                                     &block_total_len, error)) != -1)
    {
      block_start = ftell (f);

      if (block_start + block_total_len > st.st_size)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Could not open '%s' for reading: %s"),
                       gimp_file_get_utf8_name (file),
                       _("invalid block size"));
          goto error;
        }

      if (id == PSP_IMAGE_BLOCK)
        {
          if (block_number != 0)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Duplicate General Image Attributes block."));
              goto error;
            }
          if (read_general_image_attribute_block (f, block_init_len,
                                                  block_total_len, &ia, error) == -1)
            {
              goto error;
            }

          IFDBG(2) g_message ("%d dpi %dx%d %s",
                              (int) ia.resolution,
                              ia.width, ia.height,
                              compression_name (ia.compression));

          image = gimp_image_new_with_precision (ia.width, ia.height,
                                                 ia.base_type, ia.precision);
          if (! image)
            {
              goto error;
            }

          gimp_image_set_resolution (image, ia.resolution, ia.resolution);
        }
      else
        {
          if (block_number == 0)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Missing General Image Attributes block."));
              goto error;
            }

          switch (id)
            {
            case PSP_CREATOR_BLOCK:
              if (read_creator_block (f, image, block_total_len, &ia, error) == -1)
                goto error;
              break;

            case PSP_COLOR_BLOCK:
              if (read_color_block (f, image, block_total_len, &ia, error) == -1)
                goto error;
              break;

            case PSP_LAYER_START_BLOCK:
              if (! read_layer_block (f, image, block_total_len, &ia, error))
                goto error;
              break;

            case PSP_SELECTION_BLOCK:
              if (read_selection_block (f, image, block_total_len, &ia,
                                        error) == -1)
                goto error;
              break;

            case PSP_ALPHA_BANK_BLOCK:
              break;            /* Not yet implemented */

            case PSP_THUMBNAIL_BLOCK:
              break;            /* No use for it */

            case PSP_EXTENDED_DATA_BLOCK:
              if (read_extended_block (f, image, block_total_len, &ia,
                                       error) == -1)
                goto error;
              break;

            case PSP_TUBE_BLOCK:
              if (read_tube_block (f, image, block_total_len, &ia, error) == -1)
                goto error;
              break;

            case PSP_COMPOSITE_IMAGE_BANK_BLOCK:
              break;            /* Not yet implemented */

            case PSP_TABLE_BANK_BLOCK:
              break;            /* Not yet implemented */

            case PSP_BRUSH_BLOCK:
              break;            /* Not yet implemented */

            case PSP_ART_MEDIA_BLOCK:
            case PSP_ART_MEDIA_MAP_BLOCK:
            case PSP_ART_MEDIA_TILE_BLOCK:
            case PSP_ART_MEDIA_TEXTURE_BLOCK:
              break;            /* Not yet implemented */

            case PSP_COLORPROFILE_BLOCK:
              if (read_colorprofile_block (f, image, block_total_len, &ia,
                                           error) == -1)
                goto error;
              break;

            case PSP_LAYER_BLOCK:
            case PSP_CHANNEL_BLOCK:
            case PSP_ALPHA_CHANNEL_BLOCK:
            case PSP_ADJUSTMENT_EXTENSION_BLOCK:
            case PSP_VECTOR_EXTENSION_BLOCK:
            case PSP_SHAPE_BLOCK:
            case PSP_PAINTSTYLE_BLOCK:
            case PSP_COMPOSITE_ATTRIBUTES_BLOCK:
            case PSP_JPEG_BLOCK:
            case PSP_LINESTYLE_BLOCK:
            case PSP_TABLE_BLOCK:
            case PSP_PAPER_BLOCK:
            case PSP_PATTERN_BLOCK:
            case PSP_GRADIENT_BLOCK:
            case PSP_GROUP_EXTENSION_BLOCK:
            case PSP_MASK_EXTENSION_BLOCK:
            case PSP_RASTER_EXTENSION_BLOCK:
              g_message ("Sub-block %s should not occur "
                         "at main level of file",
                         block_name (id));
              break;

            default:
              g_message ("Unrecognized block id %d", id);
              break;
            }
        }

      if (block_start + block_total_len >= st.st_size)
        break;

      if (try_fseek (f, block_start + block_total_len, SEEK_SET, error) < 0)
        goto error;

      block_number++;
    }

  if (id == -1)
    {
    error:
      fclose (f);
      if (image)
        gimp_image_delete (image);
      return NULL;
    }

  fclose (f);

  return image;
}

static gint
export_image (GFile         *file,
              GimpImage     *image,
              gint           n_drawables,
              GList         *drawables,
              GObject       *config,
              GError       **error)
{
  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
               _("Exporting not implemented yet."));

  return FALSE;
}
