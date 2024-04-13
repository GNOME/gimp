/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP PSD Plug-in
 * Copyright 2007 by John Marshall
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

#ifndef __PSD_H__
#define __PSD_H__


/* Set to the level of debugging output you want, 0 for none.
 *   Setting higher than 2 will result in a very large amount of debug
 *   output being produced. */
#define PSD_DEBUG 3
#define IFDBG(level) if (PSD_DEBUG >= level)

/* Set to FALSE to suppress pop-up warnings about lossy blend mode conversions */
#define CONVERSION_WARNINGS             FALSE

#define LOAD_PROC                       "file-psd-load"
#define LOAD_MERGED_PROC                "file-psd-load-merged"
#define LOAD_THUMB_PROC                 "file-psd-load-thumb"
#define EXPORT_PROC                     "file-psd-export"
#define LOAD_METADATA_PROC              "file-psd-load-metadata"
#define PLUG_IN_BINARY                  "file-psd"
#define PLUG_IN_ROLE                    "gimp-file-psd"

#define GIMP_PARASITE_COMMENT           "gimp-comment"

#define PSD_PARASITE_DUOTONE_DATA       "psd-duotone-data"
#define PSD_PARASITE_CLIPPING_PATH      "psd-clipping-path"
#define PSD_PARASITE_PATH_FLATNESS      "psd-path-flatness"

/* Copied from app/base/gimpimage-quick-mask.h - internal identifier for quick mask channel */
#define GIMP_IMAGE_QUICK_MASK_NAME      "Qmask"

#define MAX_RAW_SIZE    0               /* FIXME all images are raw if 0 */

/* PSD spec defines */
/* Although the spec still says 56 there is a test image with more so let's go a little higher to 99 */
#define MAX_CHANNELS    99              /* Photoshop CS to CS3 support 56 channels */

/* PSD spec constants */

/* Layer resource IDs */

/* Adjustment layer IDs */
#define PSD_LADJ_LEVEL          "levl"          /* Adjustment layer - levels (PS4) */
#define PSD_LADJ_CURVE          "curv"          /* Adjustment layer - curves (PS4) */
#define PSD_LADJ_BRIGHTNESS     "brit"          /* Adjustment layer - brightness/contrast (PS4) */
#define PSD_LADJ_BALANCE        "blnc"          /* Adjustment layer - color balance (PS4) */
#define PSD_LADJ_BLACK_WHITE    "blwh"          /* Adjustment layer - black & white (PS10) */
#define PSD_LADJ_HUE            "hue "          /* Adjustment layer - old hue/saturation (PS4) */
#define PSD_LADJ_HUE2           "hue2"          /* Adjustment layer - hue/saturation (PS5) */
#define PSD_LADJ_SELECTIVE      "selc"          /* Adjustment layer - selective color (PS4) */
#define PSD_LADJ_MIXER          "mixr"          /* Adjustment layer - channel mixer (PS9) */
#define PSD_LADJ_GRAD_MAP       "grdm"          /* Adjustment layer - gradient map (PS9) */
#define PSD_LADJ_PHOTO_FILT     "phfl"          /* Adjustment layer - photo filter (PS9) */
#define PSD_LADJ_EXPOSURE       "expA"          /* Adjustment layer - exposure (PS10) */
#define PSD_LADJ_INVERT         "nvrt"          /* Adjustment layer - invert (PS4) */
#define PSD_LADJ_THRESHOLD      "thrs"          /* Adjustment layer - threshold (PS4) */
#define PSD_LADJ_POSTERIZE      "post"          /* Adjustment layer - posterize (PS4) */
#define PSD_LADJ_VIBRANCE       "vibA"          /* Adjustment layer - vibrance (PS10) */
#define PSD_LADJ_COLOR_LOOKUP   "clrL"          /* Adjustment layer - color lookup (PS13) */

/* Fill Layer IDs */
#define PSD_LFIL_SOLID          "SoCo"          /* Solid color sheet setting (PS6) */
#define PSD_LFIL_PATTERN        "PtFl"          /* Pattern fill setting (PS6) */
#define PSD_LFIL_GRADIENT       "GdFl"          /* Gradient fill setting (PS6) */

/* Effects Layer IDs */
#define PSD_LFX_FX              "lrFX"          /* Effects layer info (PS5) */
#define PSD_LFX_FX2             "lfx2"          /* Object based effects layer info (PS6) */

/* Type Tool Layers */
#define PSD_LTYP_TYPE           "tySh"          /* Type tool layer (PS5) */
#define PSD_LTYP_TYPE2          "TySh"          /* Type tool object setting (PS6) */

/* Layer Properties */
#define PSD_LPRP_UNICODE        "luni"          /* Unicode layer name (PS5) */
#define PSD_LPRP_SOURCE         "lnsr"          /* Layer name source setting (PS6) */
#define PSD_LPRP_ID             "lyid"          /* Layer ID (PS5) */
#define PSD_LPRP_BLEND_CLIP     "clbl"          /* Blend clipping elements (PS6) */
#define PSD_LPRP_BLEND_INT      "infx"          /* Blend interior elements (PS6) */
#define PSD_LPRP_KNOCKOUT       "knko"          /* Knockout setting (PS6) */
#define PSD_LPRP_PROTECT        "lspf"          /* Protected setting (PS6) */
#define PSD_LPRP_COLOR          "lclr"          /* Sheet color setting (PS6) */
#define PSD_LPRP_REF_POINT      "fxrp"          /* Reference point (PS6) */
#define PSD_LPRP_VERSION        "lyvr"          /* Layer version (PS7) */

/* Vector mask */
#define PSD_LMSK_VMASK          "vmsk"          /* Vector mask setting (PS6) */

/* Parasites */
#define PSD_LPAR_ANNOTATE       "Anno"          /* Annotation (PS6) */

/* Other */
#define PSD_LOTH_SECTION        "lsct"          /* Section divider setting - Layer groups (PS6) */
#define PSD_LOTH_SECTION2       "lsdk"          /* Nested section divider setting - Layer groups (CS5) */
#define PSD_LOTH_PATTERN        "Patt"          /* Patterns (PS6) */
#define PSD_LOTH_PATTERN_2      "Pat2"          /* Patterns 2nd key (PS6) */
#define PSD_LOTH_PATTERN_3      "Pat3"          /* Patterns 3rd key (PS6) */
#define PSD_LOTH_GRADIENT       "grdm"          /* Gradient settings (PS6) */
#define PSD_LOTH_RESTRICT       "brst"          /* Channel blending restriction setting (PS6) */
#define PSD_LOTH_FOREIGN_FX     "ffxi"          /* Foreign effect ID (PS6) */
#define PSD_LOTH_PATT_DATA      "shpa"          /* Pattern data (PS6) */
#define PSD_LOTH_META_DATA      "shmd"          /* Meta data setting (PS6) */
#define PSD_LOTH_LAYER_DATA     "layr"          /* Layer data (PS6) */
#define PSD_LOTH_CONTENT_GEN    "CgEd"          /* Content generator extra data (PS12) */
#define PSD_LOTH_TEXT_ENGINE    "Txt2"          /* Text engine data (PS10) */
#define PSD_LOTH_PATH_NAME      "pths"          /* Unicode path name (PS13) */
#define PSD_LOTH_ANIMATION_FX   "anFX"          /* Animation effects (PS13) */
#define PSD_LOTH_FILTER_MASK    "FMsk"          /* Filter mask (PS10) */
#define PSD_LOTH_VECTOR_STROKE  "vscg"          /* Vector stroke data (PS13) */
#define PSD_LOTH_ALIGN_RENDER   "sn2P"          /* Aligned rendering flag (?) */
#define PSD_LOTH_USER_MASK      "LMsk"          /* User mask (?) */

/* Effects layer resource IDs */
#define PSD_LFX_COMMON          "cmnS"          /* Effects layer - common state (PS5) */
#define PSD_LFX_DROP_SDW        "dsdw"          /* Effects layer - drop shadow (PS5) */
#define PSD_LFX_INNER_SDW       "isdw"          /* Effects layer - inner shadow (PS5) */
#define PSD_LFX_OUTER_GLW       "oglw"          /* Effects layer - outer glow (PS5) */
#define PSD_LFX_INNER_GLW       "iglw"          /* Effects layer - inner glow (PS5) */
#define PSD_LFX_BEVEL           "bevl"          /* Effects layer - bevel (PS5) */

/* Placed Layer */
#define PSD_LPL_PLACE_LAYER     "PlLd"          /* Placed layer (?) (based on PSD files, not specification) */
#define PSD_LPL_PLACE_LAYER_NEW "SoLd"          /* Placed layer (PS10) */
#define PSD_SMART_OBJECT_LAYER  "SoLE"          /* Smart Object Layer (CC2015) */

/* Linked Layer */
#define PSD_LLL_LINKED_LAYER     "lnkD"         /* Linked layer (?) */
#define PSD_LLL_LINKED_LAYER_2   "lnk2"         /* Linked layer 2nd key */
#define PSD_LLL_LINKED_LAYER_3   "lnk3"         /* Linked layer 3rd key */
#define PSD_LLL_LINKED_LAYER_EXT "lnkE"         /* Linked layer external */

/* Merged Transparency */
#define PSD_LMT_MERGE_TRANS     "Mtrn"          /* Merged transparency save flag (?) */
#define PSD_LMT_MERGE_TRANS_16  "Mt16"          /* Merged transparency save flag 2 */
#define PSD_LMT_MERGE_TRANS_32  "Mt32"          /* Merged transparency save flag 3 */

/* Filter Effects */
#define PSD_LFFX_FILTER_FX      "FXid"          /* Filter effects (?) */
#define PSD_LFFX_FILTER_FX_2    "FEid"          /* Filter effects 2 */

/* PSD spec enums */

/* Image color modes */
typedef enum {
  PSD_BITMAP       = 0,                 /* Bitmap image */
  PSD_GRAYSCALE    = 1,                 /* Greyscale image */
  PSD_INDEXED      = 2,                 /* Indexed image */
  PSD_RGB          = 3,                 /* RGB image */
  PSD_CMYK         = 4,                 /* CMYK */
  PSD_MULTICHANNEL = 7,                 /* Multichannel image*/
  PSD_DUOTONE      = 8,                 /* Duotone image*/
  PSD_LAB          = 9                  /* L*a*b image */
} PSDColorMode;

/* Image color spaces */
typedef enum {
  PSD_CS_RGB       = 0,                 /* RGB */
  PSD_CS_HSB       = 1,                 /* Hue, Saturation, Brightness */
  PSD_CS_CMYK      = 2,                 /* CMYK */
  PSD_CS_PANTONE   = 3,                 /* Pantone matching system (Lab)*/
  PSD_CS_FOCOLTONE = 4,                 /* Focoltone color system (CMYK)*/
  PSD_CS_TRUMATCH  = 5,                 /* Trumatch color (CMYK)*/
  PSD_CS_TOYO      = 6,                 /* Toyo 88 colorfinder 1050 (Lab)*/
  PSD_CS_LAB       = 7,                 /* L*a*b*/
  PSD_CS_GRAYSCALE = 8,                 /* Grey scale */
  PSD_CS_HKS       = 10,                /* HKS colors (CMYK)*/
  PSD_CS_DIC       = 11,                /* DIC color guide (Lab)*/
  PSD_CS_ANPA      = 3000,              /* Anpa color (Lab)*/
} PSDColorSpace;

/* Image Resource IDs */
typedef enum {
  PSD_PS2_IMAGE_INFO    = 1000,         /* 0x03e8 - Obsolete - ps 2.0 image info */
  PSD_MAC_PRINT_INFO    = 1001,         /* 0x03e9 - Optional - Mac print manager print info record */
  PSD_PS2_COLOR_TAB     = 1003,         /* 0x03eb - Obsolete - ps 2.0 indexed color table */
  PSD_RESN_INFO         = 1005,         /* 0x03ed - ResolutionInfo structure */
  PSD_ALPHA_NAMES       = 1006,         /* 0x03ee - Alpha channel names */
  PSD_DISPLAY_INFO      = 1007,         /* 0x03ef - Superceded by PSD_DISPLAY_INFO_NEW for ps CS3 and higher - DisplayInfo structure */
  PSD_CAPTION           = 1008,         /* 0x03f0 - Optional - Caption string */
  PSD_BORDER_INFO       = 1009,         /* 0x03f1 - Border info */
  PSD_BACKGROUND_COL    = 1010,         /* 0x03f2 - Background color */
  PSD_PRINT_FLAGS       = 1011,         /* 0x03f3 - Print flags */
  PSD_GREY_HALFTONE     = 1012,         /* 0x03f4 - Greyscale and multichannel halftoning info */
  PSD_COLOR_HALFTONE    = 1013,         /* 0x03f5 - Color halftoning info */
  PSD_DUOTONE_HALFTONE  = 1014,         /* 0x03f6 - Duotone halftoning info */
  PSD_GREY_XFER         = 1015,         /* 0x03f7 - Greyscale and multichannel transfer functions */
  PSD_COLOR_XFER        = 1016,         /* 0x03f8 - Color transfer functions */
  PSD_DUOTONE_XFER      = 1017,         /* 0x03f9 - Duotone transfer functions */
  PSD_DUOTONE_INFO      = 1018,         /* 0x03fa - Duotone image information */
  PSD_EFFECTIVE_BW      = 1019,         /* 0x03fb - Effective black & white values for dot range */
  PSD_OBSOLETE_01       = 1020,         /* 0x03fc - Obsolete */
  PSD_EPS_OPT           = 1021,         /* 0x03fd - EPS options */
  PSD_QUICK_MASK        = 1022,         /* 0x03fe - Quick mask info */
  PSD_OBSOLETE_02       = 1023,         /* 0x03ff - Obsolete */
  PSD_LAYER_STATE       = 1024,         /* 0x0400 - Layer state info */
  PSD_WORKING_PATH      = 1025,         /* 0x0401 - Working path (not saved) */
  PSD_LAYER_GROUP       = 1026,         /* 0x0402 - Layers group info */
  PSD_OBSOLETE_03       = 1027,         /* 0x0403 - Obsolete */
  PSD_IPTC_NAA_DATA     = 1028,         /* 0x0404 - IPTC-NAA record (IMV4.pdf) */
  PSD_IMAGE_MODE_RAW    = 1029,         /* 0x0405 - Image mode for raw format files */
  PSD_JPEG_QUAL         = 1030,         /* 0x0406 - JPEG quality */
  PSD_GRID_GUIDE        = 1032,         /* 0x0408 - Grid & guide info */
  PSD_THUMB_RES         = 1033,         /* 0x0409 - Thumbnail resource */
  PSD_COPYRIGHT_FLG     = 1034,         /* 0x040a - Copyright flag */
  PSD_URL               = 1035,         /* 0x040b - URL string */
  PSD_THUMB_RES2        = 1036,         /* 0x040c - Thumbnail resource */
  PSD_GLOBAL_ANGLE      = 1037,         /* 0x040d - Superceded by PSD_NEW_COLOR_SAMPLER for ps CS3 and higher - Global angle */
  PSD_COLOR_SAMPLER     = 1038,         /* 0x040e - Superceded by PSD_NEW_COLOR_SAMPLER for ps CS3 and higher - Color samplers resource */
  PSD_ICC_PROFILE       = 1039,         /* 0x040f - ICC Profile */
  PSD_WATERMARK         = 1040,         /* 0x0410 - Watermark */
  PSD_ICC_UNTAGGED      = 1041,         /* 0x0411 - Do not use ICC profile flag */
  PSD_EFFECTS_VISIBLE   = 1042,         /* 0x0412 - Show / hide all effects layers */
  PSD_SPOT_HALFTONE     = 1043,         /* 0x0413 - Spot halftone */
  PSD_DOC_IDS           = 1044,         /* 0x0414 - Document specific IDs */
  PSD_ALPHA_NAMES_UNI   = 1045,         /* 0x0415 - Unicode alpha names */
  PSD_IDX_COL_TAB_CNT   = 1046,         /* 0x0416 - Indexed color table count */
  PSD_IDX_TRANSPARENT   = 1047,         /* 0x0417 - Index of transparent color (if any) */
  PSD_GLOBAL_ALT        = 1049,         /* 0x0419 - Global altitude */
  PSD_SLICES            = 1050,         /* 0x041a - Slices */
  PSD_WORKFLOW_URL_UNI  = 1051,         /* 0x041b - Workflow URL - Unicode string */
  PSD_JUMP_TO_XPEP      = 1052,         /* 0x041c - Jump to XPEP (?) */
  PSD_ALPHA_ID          = 1053,         /* 0x041d - Alpha IDs */
  PSD_URL_LIST_UNI      = 1054,         /* 0x041e - URL list - unicode */
  PSD_VERSION_INFO      = 1057,         /* 0x0421 - Version info */
  PSD_EXIF_DATA         = 1058,         /* 0x0422 - Exif data block 1 */
  PSD_EXIF_DATA_3       = 1059,         /* 0X0423 - Exif data block 3 (?) */
  PSD_XMP_DATA          = 1060,         /* 0x0424 - XMP data block */
  PSD_CAPTION_DIGEST    = 1061,         /* 0x0425 - Caption digest */
  PSD_PRINT_SCALE       = 1062,         /* 0x0426 - Print scale */
  PSD_PIXEL_AR          = 1064,         /* 0x0428 - Pixel aspect ratio */
  PSD_LAYER_COMPS       = 1065,         /* 0x0429 - Layer comps */
  PSD_ALT_DUOTONE_COLOR = 1066,         /* 0x042A - Alternative Duotone colors */
  PSD_ALT_SPOT_COLOR    = 1067,         /* 0x042B - Alternative Spot colors */
  PSD_LAYER_SELECT_ID   = 1069,         /* 0x042D - Layer selection ID */
  PSD_HDR_TONING_INFO   = 1070,         /* 0x042E - HDR toning information */
  PSD_PRINT_INFO_SCALE  = 1071,         /* 0x042F - Print scale */
  PSD_LAYER_GROUP_E_ID  = 1072,         /* 0x0430 - Layer group(s) enabled ID */
  PSD_COLOR_SAMPLER_NEW = 1073,         /* 0x0431 - Color sampler resource for ps CS3 and higher PSD files */
  PSD_MEASURE_SCALE     = 1074,         /* 0x0432 - Measurement scale */
  PSD_TIMELINE_INFO     = 1075,         /* 0x0433 - Timeline information */
  PSD_SHEET_DISCLOSE    = 1076,         /* 0x0434 - Sheet discloser */
  PSD_DISPLAY_INFO_NEW  = 1077,         /* 0x0435 - DisplayInfo structure for ps CS3 and higher PSD files */
  PSD_ONION_SKINS       = 1078,         /* 0x0436 - Onion skins */
  PSD_COUNT_INFO        = 1080,         /* 0x0438 - Count information*/
  PSD_PRINT_INFO        = 1082,         /* 0x043A - Print information added in ps CS5*/
  PSD_PRINT_STYLE       = 1083,         /* 0x043B - Print style */
  PSD_MAC_NSPRINTINFO   = 1084,         /* 0x043C - Mac NSPrintInfo*/
  PSD_WIN_DEVMODE       = 1085,         /* 0x043D - Windows DEVMODE */
  PSD_AUTO_SAVE_PATH    = 1086,         /* 0x043E - Auto save file path */
  PSD_AUTO_SAVE_FORMAT  = 1087,         /* 0x043F - Auto save format */
  PSD_PATH_INFO_FIRST   = 2000,         /* 0x07d0 - First path info block */
  PSD_PATH_INFO_LAST    = 2998,         /* 0x0bb6 - Last path info block */
  PSD_CLIPPING_PATH     = 2999,         /* 0x0bb7 - Name of clipping path */
  PSD_PLUGIN_R_FIRST    = 4000,         /* 0x0FA0 - First plugin resource */
  PSD_PLUGIN_R_LAST     = 4999,         /* 0x1387 - Last plugin resource */
  PSD_IMAGEREADY_VARS   = 7000,         /* 0x1B58 - Imageready variables */
  PSD_IMAGEREADY_DATA   = 7001,         /* 0x1B59 - Imageready data sets */
  PSD_LIGHTROOM_WORK    = 8000,         /* 0x1F40 - Lightroom workflow */
  PSD_PRINT_FLAGS_2     = 10000         /* 0x2710 - Print flags */
} PSDImageResID;

/* Display resolution units */
typedef enum {
  PSD_RES_INCH = 1,                     /* Pixels / inch */
  PSD_RES_CM = 2,                       /* Pixels / cm */
} PSDDisplayResUnit;

/* Width and height units */
typedef enum {
  PSD_UNIT_INCH         = 1,            /* inches */
  PSD_UNIT_CM           = 2,            /* cm */
  PSD_UNIT_POINT        = 3,            /* points  (72 points =   1 inch) */
  PSD_UNIT_PICA         = 4,            /* pica    ( 6 pica   =   1 inch) */
  PSD_UNIT_COLUMN       = 5,            /* columns ( column defined in ps prefs, default = 2.5 inches) */
} PSDUnit;

/* Thumbnail image data encoding */
typedef enum {
  kRawRGB               = 0,            /* RAW data format (never used?) */
  kJpegRGB              = 1             /* JPEG compression */
} PSDThumbFormat;

/* Path record types */
typedef enum {
  PSD_PATH_CL_LEN       = 0,            /* Closed sub-path length record */
  PSD_PATH_CL_LNK       = 1,            /* Closed sub-path Bezier knot, linked */
  PSD_PATH_CL_UNLNK     = 2,            /* Closed sub-path Bezier knot, unlinked */
  PSD_PATH_OP_LEN       = 3,            /* Open sub-path length record */
  PSD_PATH_OP_LNK       = 4,            /* Open sub-path Bezier knot, linked */
  PSD_PATH_OP_UNLNK     = 5,            /* Open sub-path Bezier knot, unlinked */
  PSD_PATH_FILL_RULE    = 6,            /* Path fill rule record */
  PSD_PATH_CLIPBOARD    = 7,            /* Path clipboard record */
  PSD_PATH_FILL_INIT    = 8             /* Path initial fill record */
} PSDpathtype;

/* Channel ID */
typedef enum {
  PSD_CHANNEL_EXTRA_MASK= -3,           /* User supplied extra layer mask */
  PSD_CHANNEL_MASK      = -2,           /* User supplied layer mask */
  PSD_CHANNEL_ALPHA     = -1,           /* Transparency mask */
  PSD_CHANNEL_RED       =  0,           /* Red channel data */
  PSD_CHANNEL_GREEN     =  1,           /* Green channel data */
  PSD_CHANNEL_BLUE      =  2            /* Blue channel data */
} PSDChannelID;

/* Clipping */
typedef enum {
  PSD_CLIPPING_BASE     = 0,            /* Base clipping */
  PSD_CLIPPING_NON_BASE = 1             /* Non-base clipping */
} PSDClipping;

/* Image compression mode */
typedef enum {
  PSD_COMP_RAW     = 0,                 /* Raw data */
  PSD_COMP_RLE,                         /* RLE compressed */
  PSD_COMP_ZIP,                         /* ZIP without prediction */
  PSD_COMP_ZIP_PRED                     /* ZIP with prediction */
} PSDCompressMode;

/* Vertical - horizontal selection */
typedef enum {
  PSD_VERTICAL     = 0,                 /* Vertical */
  PSD_HORIZONTAL   = 1                  /* Horizontal */
} VHSelect;


/* PSD spec data structures */

/* PSD field types */
typedef gint32  Fixed;                  /* Represents a fixed point implied decimal */


/* Apple color space data structures */

/* RGB Color Value
   A color value expressed in the RGB color space is composed of red, green,
   and blue component values. Each color component is expressed as a numeric
   value within the range of 0 to 65535.
*/
typedef struct
{
  guint16       red;
  guint16       green;
  guint16       blue;
} CMRGBColor;

/*  HSV Color Value
    A color value expressed in the HSV color space is composed of hue,
    saturation, and value component values. Each color component is
    expressed as a numeric value within the range of 0 to 65535 inclusive.
    The hue value represents a fraction of a circle in which red is
    positioned at 0.
*/

typedef struct
{
  guint16       hue;
  guint16       saturation;
  guint16       value;
} CMHSVColor;

/* CMYK Color Value
  A color value expressed in the CMYK color space is composed of cyan, magenta,
  yellow, and black component values. Each color component is expressed as a
  numeric value within the range of 0 to 65535 inclusive, with 0 representing
  100% ink (e.g. pure cyan = 0, 65535, 65535, 65535).
*/

typedef struct
{
  guint16       cyan;
  guint16       magenta;
  guint16       yellow;
  guint16       black;
} CMCMYKColor;

/* L*a*b* Color Value
   The first three values in the color data are, respectively, the colors
   lightness, a chrominance, and b chrominance components. The lightness
   component is a 16bit value ranging from 0 to 10000. The chrominance
   components are each 16bit values ranging from 12800 to 12700. Gray
   values are represented by chrominance components of 0 (e.g. pure white
   is defined as 10000, 0, 0).
*/
typedef struct
{
  guint16       L;
  gint16        a;
  gint16        b;
} CMLabColor;

/* Gray Color Value
  A color value expressed in the Gray color space is composed of a single component,
  gray, represented as a numeric value within the range of 0 to 10000.
*/
typedef struct
{
  guint16       gray;
} CMGrayColor ;

/* The color union is defined by the CMColor type definition.
 */
typedef union
{
  CMRGBColor        rgb;
  CMHSVColor        hsv;
  CMLabColor        Lab;
  CMCMYKColor       cmyk;
  CMGrayColor       gray;
} CMColor;

/* GIMP layer mode info */
typedef struct
{
  GimpLayerMode          mode;
  GimpLayerColorSpace    blend_space;
  GimpLayerColorSpace    composite_space;
  GimpLayerCompositeMode composite_mode;
} LayerModeInfo;

/* Image resolution data */
typedef struct {
  Fixed         hRes;                   /* Horizontal resolution pixels/inch */
  gint16        hResUnit;               /* Horizontal display resolution unit (1=pixels per inch, 2=pixels per cm) */
  gint16        widthUnit;              /* Display width unit (1=inches; 2=cm; 3=points; 4=picas; 5=columns) */
  Fixed         vRes;                   /* Vertical resolution pixels/inch */
  gint16        vResUnit;               /* Vertical display resolution unit */
  gint16        heightUnit;             /* Display height unit */
} ResolutionInfo;

/* Grid & guide header */
typedef struct {
  guint32       fVersion;               /* Version - always 1 for PS */
  guint32       fGridCycleV;            /* Vertical grid size */
  guint32       fGridCycleH;            /* Horizontal grid size */
  guint32       fGuideCount;            /* Number of guides */
} GuideHeader;

/* Guide resource block */
typedef struct {
  guint32       fLocation;              /* Guide position in Pixels * 100 */
  gchar         fDirection;             /* Guide orientation */
} GuideResource;

/* Thumbnail data */
typedef struct {
  gint32        format;                 /* Thumbnail image data format (1 = JPEG) */
  gint32        width;                  /* Thumbnail width in pixels */
  gint32        height;                 /* Thumbnail height in pixels */
  gint32        widthbytes;             /* Padded row bytes ((width * bitspixel + 31) / 32 * 4) */
  gint32        size;                   /* Total size (widthbytes * height * planes */
  gint32        compressedsize;         /* Size after compression for consistency */
  gint16        bitspixel;              /* Bits per pixel (always 24) */
  gint16        planes;                 /* Number of planes (always 1) */
} ThumbnailInfo;

/* Channel display info data for Adobe Photoshop CS2 and lower */
typedef struct {
  gint16        colorSpace;             /* Color space from PSDColorSpace */
  guint16       color[4];               /* 4 * 16 bit color components */
  gint16        opacity;                /* Opacity 0 to 100 */
  gchar         kind;                   /* Selected = 0, Protected = 1 */
  gchar         padding;                /* Padding */
} DisplayInfo;

/* Channel display info data for Adobe Photoshop CS3 and higher to support floating point colors */
typedef struct {
  gint16        colorSpace;             /* Color space from PSDColorSpace */
  guint16       color[4];               /* 4 * 16 bit color components */
  gint16        opacity;                /* Opacity 0 to 100 */
  gchar         mode;                   /* Alpha = 0, Inverted alpha = 1, Spot = 2 */
} DisplayInfoNew;

/* PSD Channel length info data structure */
typedef struct
{
  gint16        channel_id;             /* Channel ID */
  guint64       data_len;               /* Layer left */
} ChannelLengthInfo;

/* PSD Layer flags */
typedef struct
{
  gboolean      trans_prot;             /* Transparency protected */
  gboolean      visible;                /* Visible */
  gboolean      obsolete;               /* Obsolete */
  gboolean      bit4;                   /* Bit 4 in use */
  gboolean      irrelevant;             /* Pixel data irrelevant to image appearance */
} LayerFlags;

/* PSD Layer mask flags */
typedef struct
{
  gboolean      relative_pos;           /* Mask position recorded relative to layer */
  gboolean      disabled;               /* Mask disabled */
  gboolean      invert;                 /* Invert mask on blending (obsolete according to online specs) */
  gboolean      rendered;               /* User mask actually came from rendering other data */
  gboolean      params_present;         /* User and/or vector masks have parameters applied to them */
} MaskFlags;

/* PSD Slices */
typedef struct
{
  gint32        id;                     /* ID */
  gint32        groupid;                /* Group ID */
  gint32        origin;                 /* Origin */
  gint32        associatedid;           /* Associated Layer ID */
  gchar         *name;                  /* Name */
  gint32        type;                   /* Type */
  gint32        left;                   /* Position coordinates */
  gint32        top;
  gint32        right;
  gint32        bottom;
  gchar         *url;                   /* URL */
  gchar         *target;                /* Target */
  gchar         *message;               /* Message */
  gchar         *alttag;                /* Alt Tag */
  gchar         html;                   /* Boolean for if cell text is HTML */
  gchar         *celltext;              /* Cell text */
  gint32        horizontal;             /* Horizontal alignment */
  gint32        vertical;               /* Vertical alignment */
  gchar         alpha;                  /* Alpha */
  gchar         red;                    /* Red */
  gchar         green;                  /* Green */
  gchar         blue;                   /* Blue */
} PSDSlice;

/* PSD Layer mask data (length 20) */
typedef struct
{
  gint32                top;                    /* Layer top */
  gint32                left;                   /* Layer left */
  gint32                bottom;                 /* Layer bottom */
  gint32                right;                  /* Layer right */
  guchar                def_color;              /* Default background color */
  guchar                flags;                  /* Layer flags */
  guchar                mask_params;            /* Mask parameters. Only present if bit 4 of flags is set. */
  guchar                extra_flags;            /* Real layer flags */
  guchar                extra_def_color;        /* Real user mask background */
  MaskFlags             mask_flags;             /* Flags */
} LayerMask;

/* PSD Layer mask data (length 36) */
typedef struct
{
  gint32                top;                    /* Layer top */
  gint32                left;                   /* Layer left */
  gint32                bottom;                 /* Layer bottom */
  gint32                right;                  /* Layer right */
} LayerMaskExtra;

/* PSD text reading */
typedef struct
{
  gdouble               xx; /* Transform information */
  gdouble               xy;
  gdouble               yx;
  gdouble               yy;
  gdouble               tx;
  gdouble               ty;
  gchar                 *info; /* Text information */
} PSDText;

/* Partially or Unsupported Features */
typedef struct
{
  gboolean show_gui;
  gboolean duotone_mode;

  gboolean adjustment_layer;
  gboolean fill_layer;
  gboolean text_layer;
  gboolean linked_layer;
  gboolean vector_mask;
  gboolean smart_object;
  gboolean stroke;
  gboolean layer_effect;
  gboolean layer_comp;
  gboolean psd_metadata;
} PSDSupport;

/* PSD Layer data structure */
typedef struct
{
  gint32                top;                    /* Layer top */
  gint32                left;                   /* Layer left */
  gint32                bottom;                 /* Layer bottom */
  gint32                right;                  /* Layer right */
  guint16               num_channels;           /* Number of channels */
  ChannelLengthInfo    *chn_info;               /* Channel length info */
  gchar                 mode_key[4];            /* Blend mode key */
  gchar                 blend_mode[4];          /* Blend mode */
  guchar                opacity;                /* Opacity - 0 = transparent ... 255 = opaque */
  guchar                clipping;               /* Clipping */
  guchar                clipping_group_type;    /* Used to track group needed for clipping (1 = group start, 2 = group end) */
  guchar                flags;                  /* Layer flags */
  guchar                filler;                 /* Filler */
  guint64               extra_len;              /* Extra data length */
  gchar                *name;                   /* Layer name */
  guint64               mask_len;               /* Layer mask data length */
  LayerMask             layer_mask;             /* Layer mask data */
  LayerMaskExtra        layer_mask_extra;       /* Layer mask extra data */
  LayerFlags            layer_flags;            /* Layer flags */
  PSDText               text;                   /* PSD text */
  guint32               id;                     /* Layer ID (Tattoo) */
  guchar                group_type;             /* 0 -> not a group; 1 -> open folder; 2 -> closed folder; 3 -> end of group */
  guint16               color_tag[4];           /* 4 * 16 bit color components */

  PSDSupport           *unsupported_features;
} PSDlayer;

/* PSD Channel data structure */
typedef struct
{
  gint16        id;                     /* Channel ID */
  gchar        *name;                   /* Channel name */
  gchar        *data;                   /* Channel image data */
  guint32       rows;                   /* Channel rows */
  guint32       columns;                /* Channel columns */
} PSDchannel;

/* PSD Channel data structure */
typedef struct
{
  GeglColor    *gimp_color;             /* Gimp RGB color */
  gint16        opacity;                /* Opacity */
  guchar        ps_mode;                /* PS mode flag */
  guchar        ps_kind;                /* PS type flag */
  gint16        ps_cspace;              /* PS color space */
  CMColor       ps_color;               /* PS color */
} PSDchanneldata;

/* PSD Image Resource data structure */
typedef struct
{
  gchar         type[4];                /* Image resource type */
  gint16        id;                     /* Image resource ID */
  gchar         name[256];              /* Image resource name (pascal string) */
  guint64       data_start;             /* Image resource data start */
  guint64       data_len;               /* Image resource data length */
} PSDimageres;

/* PSD Layer Resource data structure */
typedef struct
{
  gchar         sig[4];                 /* Layer resource signature */
  gchar         key[4];                 /* Layer resource key */
  guint64       data_start;             /* Layer resource data start */
  guint64       data_len;               /* Layer resource data length */
  gboolean      ibm_pc_format;          /* If layers are saved in little endian format */
} PSDlayerres;

/* PSD File data structures */
typedef struct
{
  gboolean              merged_image_only;      /* Whether to load only the merged image data */

  guint16               version;                /* Version 1 (PSD) or 2 (PSB) */
  guint16               channels;               /* Number of channels: 1- 56 */
  gboolean              transparency;           /* Image has merged transparency alpha channel */
  guint32               rows;                   /* Number of rows: 1 - 30000 */
  guint32               columns;                /* Number of columns: 1 - 30000 */
  guint16               bps;                    /* Bits per sample: 1, 8, 16, or 32 */
  guint16               color_mode;             /* Image color mode: {PSDColorMode} */
  GimpImageBaseType     base_type;              /* Image base color mode: (GIMP) */
  guint16               comp_mode;              /* Merged image compression mode */
  guchar               *color_map;              /* Color map data */
  guint32               color_map_len;          /* Color map data length */
  guint32               color_map_entries;      /* Color map number of entries */
  guint64               image_res_start;        /* Image resource block start address */
  guint64               image_res_len;          /* Image resource block length */
  guint64               mask_layer_start;       /* Mask & layer block start address */
  guint64               mask_layer_len;         /* Mask & layer block length */
  gint16                num_layers;             /* Number of layers */
  guint64               layer_data_start;       /* Layer pixel data start */
  guint64               layer_data_len;         /* Layer pixel data length */
  guint64               merged_image_start;     /* Merged image pixel data block start address */
  guint64               merged_image_len;       /* Merged image pixel data block length */
  gboolean              no_icc;                 /* Do not use ICC profile */
  guint16               layer_state;            /* Active layer index counting from bottom up */
  GList                *layer_selection;        /* Selected layer IDs (GIMP layer tattoos) */
  GPtrArray            *alpha_names;            /* Alpha channel names */
  PSDchanneldata      **alpha_display_info;     /* Alpha channel display info */
  guint16               alpha_display_count;    /* Number of alpha channel display info recs */
  guint32              *alpha_id;               /* Alpha channel ids (tattoos) */
  guint16               alpha_id_count;         /* Number of alpha channel id items */
  guint16               quick_mask_id;          /* Channel number containing quick mask */

  GimpColorProfile     *cmyk_profile;
  gpointer              cmyk_transform;
  gpointer              cmyk_transform_alpha;

  gboolean              ibm_pc_format;          /* If layers are saved in little endian format */
  PSDSupport           *unsupported_features;
} PSDimage;

/* Public functions */


#endif /* __PSD_H__ */
