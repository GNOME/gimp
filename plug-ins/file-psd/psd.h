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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PSD_H__
#define __PSD_H__

/* Temporary disable of save functionality */
#ifdef PSD_SAVE
#undef PSD_SAVE
#else
/* #define PSD_SAVE */
#endif

/* Set to the level of debugging output you want, 0 for none.
 *   Setting higher than 2 will result in a very large amount of debug
 *   output being produced. */
#define PSD_DEBUG 0
#define IFDBG(level) if (PSD_DEBUG >= level)

/* Set to FALSE to supress pop-up warnings about lossy file conversions */
#define CONVERSION_WARNINGS             FALSE

#define LOAD_PROC                       "file-psd-load"
#define LOAD_THUMB_PROC                 "file-psd-load-thumb"
#define SAVE_PROC                       "file-psd-save"
#define PLUG_IN_BINARY                  "file-psd"
#define PLUG_IN_ROLE                    "gimp-file-psd"

#define DECODE_XMP_PROC                 "plug-in-metadata-decode-xmp"

#define GIMP_PARASITE_COMMENT           "gimp-comment"
#define GIMP_PARASITE_ICC_PROFILE       "icc-profile"
#define GIMP_PARASITE_EXIF              "exif-data"
#define GIMP_PARASITE_IPTC              "iptc-data"

#define METADATA_PARASITE               "gimp-metadata"
#define METADATA_MARKER                 "GIMP_XMP_1"

#define PSD_PARASITE_DUOTONE_DATA       "psd-duotone-data"

/* Copied from app/base/gimpimage-quick-mask.h - internal identifier for quick mask channel */
#define GIMP_IMAGE_QUICK_MASK_NAME      "Qmask"

#define MAX_RAW_SIZE    0               /* FIXME all images are raw if 0 */

/* PSD spec defines */
#define MAX_CHANNELS    56              /* Photoshop CS to CS3 support 56 channels */

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

/* Vector mask */
#define PSD_LMSK_VMASK          "vmsk"          /* Vector mask setting (PS6) */

/* Parasites */
#define PSD_LPAR_ANNOTATE       "Anno"          /* Annotation (PS6) */

/* Other */
#define PSD_LOTH_SECTION        "lsct"          /* Section divider setting - Layer goups (PS6) */
#define PSD_LOTH_PATTERN        "Patt"          /* Patterns (PS6) */
#define PSD_LOTH_GRADIENT       "grdm"          /* Gradient settings (PS6) */
#define PSD_LOTH_RESTRICT       "brst"          /* Channel blending restirction setting (PS6) */
#define PSD_LOTH_FOREIGN_FX     "ffxi"          /* Foreign effect ID (PS6) */
#define PSD_LOTH_PATT_DATA      "shpa"          /* Pattern data (PS6) */
#define PSD_LOTH_META_DATA      "shmd"          /* Meta data setting (PS6) */
#define PSD_LOTH_LAYER_DATA     "layr"          /* Layer data (PS6) */

/* Effects layer resource IDs */
#define PSD_LFX_COMMON          "cmnS"          /* Effects layer - common state (PS5) */
#define PSD_LFX_DROP_SDW        "dsdw"          /* Effects layer - drop shadow (PS5) */
#define PSD_LFX_INNER_SDW       "isdw"          /* Effects layer - inner shadow (PS5) */
#define PSD_LFX_OUTER_GLW       "oglw"          /* Effects layer - outer glow (PS5) */
#define PSD_LFX_INNER_GLW       "iglw"          /* Effects layer - inner glow (PS5) */
#define PSD_LFX_BEVEL           "bevl"          /* Effects layer - bevel (PS5) */

/* PSD spec enums */

/* Image colour modes */
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

/* Image colour spaces */
typedef enum {
  PSD_CS_RGB       = 0,                 /* RGB */
  PSD_CS_HSB       = 1,                 /* Hue, Saturation, Brightness */
  PSD_CS_CMYK      = 2,                 /* CMYK */
  PSD_CS_PANTONE   = 3,                 /* Pantone matching system (Lab)*/
  PSD_CS_FOCOLTONE = 4,                 /* Focoltone colour system (CMYK)*/
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
  PSD_PS2_COLOR_TAB     = 1003,         /* 0x03eb - Obsolete - ps 2.0 indexed colour table */
  PSD_RESN_INFO         = 1005,         /* 0x03ed - ResolutionInfo structure */
  PSD_ALPHA_NAMES       = 1006,         /* 0x03ee - Alpha channel names */
  PSD_DISPLAY_INFO      = 1007,         /* 0x03ef - DisplayInfo structure */
  PSD_CAPTION           = 1008,         /* 0x03f0 - Optional - Caption string */
  PSD_BORDER_INFO       = 1009,         /* 0x03f1 - Border info */
  PSD_BACKGROUND_COL    = 1010,         /* 0x03f2 - Background colour */
  PSD_PRINT_FLAGS       = 1011,         /* 0x03f3 - Print flags */
  PSD_GREY_HALFTONE     = 1012,         /* 0x03f4 - Greyscale and multichannel halftoning info */
  PSD_COLOR_HALFTONE    = 1013,         /* 0x03f5 - Colour halftoning info */
  PSD_DUOTONE_HALFTONE  = 1014,         /* 0x03f6 - Duotone halftoning info */
  PSD_GREY_XFER         = 1015,         /* 0x03f7 - Greyscale and multichannel transfer functions */
  PSD_COLOR_XFER        = 1016,         /* 0x03f8 - Colour transfer functions */
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
  PSD_GLOBAL_ANGLE      = 1037,         /* 0x040d - Global angle */
  PSD_COLOR_SAMPLER     = 1038,         /* 0x040e - Colour samplers resource */
  PSD_ICC_PROFILE       = 1039,         /* 0x040f - ICC Profile */
  PSD_WATERMARK         = 1040,         /* 0x0410 - Watermark */
  PSD_ICC_UNTAGGED      = 1041,         /* 0x0411 - Do not use ICC profile flag */
  PSD_EFFECTS_VISIBLE   = 1042,         /* 0x0412 - Show / hide all effects layers */
  PSD_SPOT_HALFTONE     = 1043,         /* 0x0413 - Spot halftone */
  PSD_DOC_IDS           = 1044,         /* 0x0414 - Document specific IDs */
  PSD_ALPHA_NAMES_UNI   = 1045,         /* 0x0415 - Unicode alpha names */
  PSD_IDX_COL_TAB_CNT   = 1046,         /* 0x0416 - Indexed colour table count */
  PSD_IDX_TRANSPARENT   = 1047,         /* 0x0417 - Index of transparent colour (if any) */
  PSD_GLOBAL_ALT        = 1049,         /* 0x0419 - Global altitude */
  PSD_SLICES            = 1050,         /* 0x041a - Slices */
  PSD_WORKFLOW_URL_UNI  = 1051,         /* 0x041b - Workflow URL - Unicode string */
  PSD_JUMP_TO_XPEP      = 1052,         /* 0x041c - Jump to XPEP (?) */
  PSD_ALPHA_ID          = 1053,         /* 0x041d - Alpha IDs */
  PSD_URL_LIST_UNI      = 1054,         /* 0x041e - URL list - unicode */
  PSD_VERSION_INFO      = 1057,         /* 0x0421 - Version info */
  PSD_EXIF_DATA         = 1058,         /* 0x0422 - Exif data block */
  PSD_XMP_DATA          = 1060,         /* 0x0424 - XMP data block */
  PSD_PATH_INFO_FIRST   = 2000,         /* 0x07d0 - First path info block */
  PSD_PATH_INFO_LAST    = 2998,         /* 0x0bb6 - Last path info block */
  PSD_CLIPPING_PATH     = 2999,         /* 0x0bb7 - Name of clipping path */
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


/* Apple colour space data structures */

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
}CMRGBColor;

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
}CMHSVColor;

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
}CMCMYKColor;

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


/* Image resolution data */
typedef struct {
  Fixed         hRes;                   /* Horizontal resolution pixels/inch */
  gint16        hResUnit;               /* Horizontal display resolution unit */
  gint16        widthUnit;              /* Width unit ?? */
  Fixed         vRes;                   /* Vertical resolution pixels/inch */
  gint16        vResUnit;               /* Vertical display resolution unit */
  gint16        heightUnit;             /* Height unit ?? */
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

/* Channel display info data */
typedef struct {
  gint16        colorSpace;             /* Colour space from  PSDColorSpace */
  guint16       color[4];               /* 4 * 16 bit color components */
  gint16        opacity;                /* Opacity 0 to 100 */
  gchar         kind;                   /* Selected = 0, Protected = 1 */
  gchar         padding;                /* Padding */
} DisplayInfo;

/* PSD Channel length info data structure */
typedef struct
{
  gint16        channel_id;             /* Channel ID */
  guint32       data_len;               /* Layer left */
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
  gboolean      invert;                 /* Invert mask on blending */
} MaskFlags;

/* PSD Layer mask data (length 20) */
typedef struct
{
  gint32                top;                    /* Layer top */
  gint32                left;                   /* Layer left */
  gint32                bottom;                 /* Layer bottom */
  gint32                right;                  /* Layer right */
  guchar                def_color;              /* Default background colour */
  guchar                flags;                  /* Layer flags */
  guchar                extra_def_color;        /* Real default background colour */
  guchar                extra_flags;            /* Real layer flags */
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

/* PSD Layer data structure */
typedef struct
{
  gboolean              drop;                   /* Do not add layer to GIMP image */
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
  guchar                flags;                  /* Layer flags */
  guchar                filler;                 /* Filler */
  guint32               extra_len;              /* Extra data length */
  gchar                *name;                   /* Layer name */
  guint32               mask_len;               /* Layer mask data length */
  LayerMask             layer_mask;             /* Layer mask data */
  LayerMaskExtra        layer_mask_extra;       /* Layer mask extra data */
  LayerFlags            layer_flags;            /* Layer flags */
  guint32               id;                     /* Layer ID (Tattoo) */
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
  GimpRGB       gimp_color;             /* Gimp RGB color */
  gint16        opacity;                /* Opacity */
  guchar        ps_kind;                /* PS type flag */
  gint16        ps_cspace;              /* PS colour space */
  CMColor       ps_color;               /* PS colour */
} PSDchanneldata;

/* PSD Image Resource data structure */
typedef struct
{
  gchar         type[4];                /* Image resource type */
  gint16        id;                     /* Image resource ID */
  gchar         name[256];              /* Image resource name (pascal string) */
  gint32        data_start;             /* Image resource data start */
  gint32        data_len;               /* Image resource data length */
} PSDimageres;

/* PSD Layer Resource data structure */
typedef struct
{
  gchar         sig[4];                 /* Layer resource signature */
  gchar         key[4];                 /* Layer resource key */
  gint32        data_start;             /* Layer resource data start */
  gint32        data_len;               /* Layer resource data length */
} PSDlayerres;

/* PSD File data structures */
typedef struct
{
  guint16               channels;               /* Number of channels: 1- 56 */
  gboolean              transparency;           /* Image has merged transparency alpha channel */
  guint32               rows;                   /* Number of rows: 1 - 30000 */
  guint32               columns;                /* Number of columns: 1 - 30000 */
  guint16               bps;                    /* Bits per channel: 1, 8 or 16 */
  guint16               color_mode;             /* Image colour mode: {PSDColorMode} */
  GimpImageBaseType     base_type;              /* Image base colour mode: (GIMP) */
  guint16               comp_mode;              /* Merged image compression mode */
  guchar               *color_map;              /* Colour map data */
  guint32               color_map_len;          /* Colour map data length */
  guint32               color_map_entries;      /* Colour map number of entries */
  guint32               image_res_start;        /* Image resource block start address */
  guint32               image_res_len;          /* Image resource block length */
  guint32               mask_layer_start;       /* Mask & layer block start address */
  guint32               mask_layer_len;         /* Mask & layer block length */
  gint16                num_layers;             /* Number of layers */
  guint32               layer_data_start;       /* Layer pixel data start */
  guint32               layer_data_len;         /* Layer pixel data length */
  guint32               merged_image_start;     /* Merged image pixel data block start address */
  guint32               merged_image_len;       /* Merged image pixel data block length */
  gboolean              no_icc;                 /* Do not use ICC profile */
  guint16               layer_state;            /* Active layer number counting from bottom up */
  GPtrArray            *alpha_names;            /* Alpha channel names */
  PSDchanneldata      **alpha_display_info;     /* Alpha channel display info */
  guint16               alpha_display_count;    /* Number of alpha channel display info recs */
  guint32              *alpha_id;               /* Alpha channel ids (tattoos) */
  guint16               alpha_id_count;         /* Number of alpha channel id items */
  guint16               quick_mask_id;          /* Channel number containing quick mask */
} PSDimage;

/* Public functions */


#endif /* __PSD_H__ */
