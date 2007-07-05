/* GIMP plug-in to load and save Paint Shop Pro files (.PSP and .TUB)
 *
 * Copyright (C) 1999 Tor Lillqvist
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 *
 * Work in progress! Doesn't handle saving yet.
 *
 * For a copy of the PSP file format documentation, surf to
 * http://www.jasc.com.
 *
 */

#define LOAD_PROC      "file-psp-load"
#define SAVE_PROC      "file-psp-save"
#define PLUG_IN_BINARY "psp"

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
  PSP_IMAGE_BLOCK = 0,		/* General Image Attributes Block (main) */
  PSP_CREATOR_BLOCK,		/* Creator Data Block (main) */
  PSP_COLOR_BLOCK,		/* Color Palette Block (main and sub) */
  PSP_LAYER_START_BLOCK,	/* Layer Bank Block (main) */
  PSP_LAYER_BLOCK,		/* Layer Block (sub) */
  PSP_CHANNEL_BLOCK,		/* Channel Block (sub) */
  PSP_SELECTION_BLOCK,		/* Selection Block (main) */
  PSP_ALPHA_BANK_BLOCK,		/* Alpha Bank Block (main) */
  PSP_ALPHA_CHANNEL_BLOCK,	/* Alpha Channel Block (sub) */
  PSP_THUMBNAIL_BLOCK,		/* Thumbnail Block (main) */
  PSP_EXTENDED_DATA_BLOCK,	/* Extended Data Block (main) */
  PSP_TUBE_BLOCK		/* Picture Tube Data Block (main) */
} PSPBlockID;

/* Bitmap type.
 */
typedef enum {
  PSP_DIB_IMAGE = 0,		/* Layer color bitmap */
  PSP_DIB_TRANS_MASK,		/* Layer transparency mask bitmap */
  PSP_DIB_USER_MASK,		/* Layer user mask bitmap */
  PSP_DIB_SELECTION,		/* Selection mask bitmap */
  PSP_DIB_ALPHA_MASK,		/* Alpha channel mask bitmap */
  PSP_DIB_THUMBNAIL		/* Thumbnail bitmap */
} PSPDIBType;

/* Channel types.
 */
typedef enum {
  PSP_CHANNEL_COMPOSITE = 0,	/* Channel of single channel bitmap */
  PSP_CHANNEL_RED,		/* Red channel of 24 bit bitmap */
  PSP_CHANNEL_GREEN,		/* Green channel of 24 bit bitmap */
  PSP_CHANNEL_BLUE		/* Blue channel of 24 bit bitmap */
} PSPChannelType;

/* Possible metrics used to measure resolution.
 */
typedef enum {
  PSP_METRIC_UNDEFINED = 0,	/* Metric unknown */
  PSP_METRIC_INCH,		/* Resolution is in inches */
  PSP_METRIC_CM			/* Resolution is in centimeters */
} PSP_METRIC;


/* Possible types of compression.
 */
typedef enum {
  PSP_COMP_NONE = 0,		/* No compression */
  PSP_COMP_RLE,			/* RLE compression */
  PSP_COMP_LZ77			/* LZ77 compression */
} PSPCompression;

/* Picture tube placement mode.
 */
typedef enum {
  tpmRandom,			/* Place tube images in random intervals */
  tpmConstant			/* Place tube images in constant intervals */
} TubePlacementMode;

/* Picture tube selection mode.
 */
typedef enum {
  tsmRandom,			/* Randomly select the next image in  */
				/* tube to display */
  tsmIncremental,		/* Select each tube image in turn */
  tsmAngular,			/* Select image based on cursor direction */
  tsmPressure,			/* Select image based on pressure  */
				/* (from pressure-sensitive pad) */
  tsmVelocity			/* Select image based on cursor speed */
} TubeSelectionMode;

/* Extended data field types.
 */
typedef enum {
  PSP_XDATA_TRNS_INDEX = 0	/* Transparency index field */
} PSPExtendedDataID;

/* Creator field types.
 */
typedef enum {
  PSP_CRTR_FLD_TITLE = 0,	/* Image document title field */
  PSP_CRTR_FLD_CRT_DATE,	/* Creation date field */
  PSP_CRTR_FLD_MOD_DATE,	/* Modification date field */
  PSP_CRTR_FLD_ARTIST,		/* Artist name field */
  PSP_CRTR_FLD_CPYRGHT,		/* Copyright holder name field */
  PSP_CRTR_FLD_DESC,		/* Image document description field */
  PSP_CRTR_FLD_APP_ID,		/* Creating app id field */
  PSP_CRTR_FLD_APP_VER		/* Creating app version field */
} PSPCreatorFieldID;

/* Creator application identifiers.
 */
typedef enum {
  PSP_CREATOR_APP_UNKNOWN = 0,	/* Creator application unknown */
  PSP_CREATOR_APP_PAINT_SHOP_PRO /* Creator is Paint Shop Pro */
} PSPCreatorAppID;

/* Layer types.
 */
typedef enum {
  PSP_LAYER_NORMAL = 0,		/* Normal layer */
  PSP_LAYER_FLOATING_SELECTION	/* Floating selection layer */
} PSPLayerType;

/* Truth values.
 */
#if 0				/* FALSE and TRUE taken by GLib */
typedef enum {
  FALSE = 0,
  TRUE
} PSP_BOOLEAN;
#else
typedef gboolean PSP_BOOLEAN;
#endif

/* End of cut&paste from psp spec */

/* The following have been reverse engineered.
 * If a new version of the spec becomes available,
 * change to use the type and constant names from it.
 */
typedef enum {
  PSP_BLEND_NORMAL = 0,
  PSP_BLEND_DARKEN,
  PSP_BLEND_LIGHTEN,
  PSP_BLEND_HUE,
  PSP_BLEND_SATURATION,
  PSP_BLEND_COLOR,
  PSP_BLEND_LUMINANCE,
  PSP_BLEND_MULTIPLY,
  PSP_BLEND_SCREEN,
  PSP_BLEND_DISSOLVE,
  PSP_BLEND_OVERLAY,
  PSP_BLEND_HARD_LIGHT,
  PSP_BLEND_SOFT_LIGHT,
  PSP_BLEND_DIFFERENCE,
  PSP_BLEND_DODGE,
  PSP_BLEND_BURN,
  PSP_BLEND_EXCLUSION
} PSPLayerBlendModes;

/* End of reverse engineered types */

/* We store the various PSP data in own structures.
 * We cannot use structs intended to be direct copies of the file block
 * headers because of struct alignment issues.
 */
typedef struct
{
  guint32 width, height;
  gdouble resolution;
  guchar metric;
  guint16 compression;
  guint16 depth;
  guchar greyscale;
  guint32 active_layer;
  guint16 layer_count;
} PSPimage;

/* Declare some local functions.
 */
static void   query      (void);
static void   run        (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);
static gint32 load_image (const gchar      *filename);
static gint   save_image (const gchar      *filename,
			  gint32            image_ID,
			  gint32            drawable_ID);

/* Various local variables...
 */
const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

/* Save info  */
typedef struct
{
  PSPCompression compression;
} PSPSaveVals;

static PSPSaveVals psvals =
{
  PSP_COMP_LZ77
};

static guint16 major, minor;


MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

#if 0
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32,    "compression",  "Specify 0 for no compression, 1 for RLE, and 2 for LZ77" }
  };
#endif

  gimp_install_procedure (LOAD_PROC,
                          "loads images from the Paint Shop Pro PSP file format",
                          "This plug-in loads and saves images in "
			  "Paint Shop Pro's native PSP format. "
			  "Vector layers aren't handled. Saving isn't "
			  "yet implemented.",
			  "Tor Lillqvist",
                          "Tor Lillqvist",
                          "1999",
                          N_("Paint Shop Pro image"),
			  NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-psp");
  gimp_register_magic_load_handler (LOAD_PROC,
				    "psp,tub",
				    "",
				    "0,string,Paint\\040Shop\\040Pro\\040Image\\040File\n\032");

  /* Removed until Saving is implemented -- njl195@zepler.org */
#if 0
  gimp_install_procedure (SAVE_PROC,
                          "saves images in the Paint Shop Pro PSP file format",
                          "This plug-in loads and saves images in "
			  "Paint Shop Pro's native PSP format. "
			  "Vector layers aren't handled. Saving isn't "
			  "yet implemented.",
                          "Tor Lillqvist",
                          "Tor Lillqvist",
                          "1999",
                          N_("Paint Shop Pro image"),
			  "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_save_handler (SAVE_PROC, "psp,tub", "");
#endif
}

static gboolean
save_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  gint       run;

  dialog = gimp_dialog_new (_("Save as PSP"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, SAVE_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  /*  file save type  */
  frame = gimp_int_radio_group_new (TRUE, _("Data Compression"),
				    G_CALLBACK (gimp_radio_button_update),
				    &psvals.compression, psvals.compression,

				    _("None"), PSP_COMP_NONE, NULL,
				    _("RLE"),  PSP_COMP_RLE,  NULL,
				    _("LZ77"), PSP_COMP_LZ77, NULL,

				    NULL);

  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static gchar *
block_name (gint id)
{
  static gchar *block_names[] =
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
    "TUBE"
  };
  static gchar *err_name = NULL;

  if (id >= 0 && id <= PSP_TUBE_BLOCK)
    return block_names[id];

  g_free (err_name);
  err_name = g_strdup_printf ("id=%d", id);

  return err_name;
}

static gint
read_block_header (FILE    *f,
		   guint32 *init_len,
		   guint32 *total_len)
{
  guchar buf[4];
  guint16 id;
  long header_start;
  guint32 len;

  IFDBG(3) header_start = ftell (f);

  if (fread (buf, 4, 1, f) < 1
      || fread (&id, 2, 1, f) < 1
      || fread (&len, 4, 1, f) < 1
      || (major < 4 && fread (total_len, 4, 1, f) < 1))
    {
      g_message ("Error reading block header");
      return -1;
    }
  if (memcmp (buf, "~BK\0", 4) != 0)
    {
      IFDBG(3)
	g_message ("Invalid block header at %ld", header_start);
      else
	g_message ("Invalid block header");
      return -1;
    }

  IFDBG(3) g_message ("%s at %ld", block_name (id), header_start);

  if (major < 4)
    {
      *init_len = GUINT32_FROM_LE (len);
      *total_len = GUINT32_FROM_LE (*total_len);
    }
  else
    {
      /* Version 4.0 seems to have dropped the initial data chunk length
       * field.
       */
      *init_len = 0xDEADBEEF;	/* Intentionally bogus, should not be used */
      *total_len = GUINT32_FROM_LE (len);
    }

  return GUINT16_FROM_LE (id);
}

static gint
read_general_image_attribute_block (FILE     *f,
				    guint     init_len,
				    guint     total_len,
				    PSPimage *ia)
{
  gchar buf[6];
  guint64 res;

  if (init_len < 38 || total_len < 38)
    {
      g_message ("Invalid general image attribute chunk size");
      return -1;
    }

  if (major >= 4)
    fseek (f, 4, SEEK_CUR);

  if (fread (&ia->width, 4, 1, f) < 1
      || fread (&ia->height, 4, 1, f) < 1
      || fread (&res, 8, 1, f) < 1
      || fread (&ia->metric, 1, 1, f) < 1
      || fread (&ia->compression, 2, 1, f) < 1
      || fread (&ia->depth, 2, 1, f) < 1
      || fread (buf, 2+4, 1, f) < 1 /* Skip plane and colour count */
      || fread (&ia->greyscale, 1, 1, f) < 1
      || fread (buf, 4, 1, f) < 1 /* Skip total image size */
      || fread (&ia->active_layer, 4, 1, f) < 1
      || fread (&ia->layer_count, 2, 1, f) < 1)
    {
      g_message ("Error reading general image attribute block");
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
      g_message ("Unknown compression type %d", ia->compression);
      return -1;
    }

  ia->depth = GUINT16_FROM_LE (ia->depth);
  if (ia->depth != 24)
    {
      g_message ("Unsupported bit depth %d", ia->depth);
      return -1;
    }

  ia->active_layer = GUINT32_FROM_LE (ia->active_layer);
  ia->layer_count = GUINT16_FROM_LE (ia->layer_count);

  return 0;
}

static gint
try_fseek (FILE  *f,
	   glong  pos,
	   gint   whence)
{
  if (fseek (f, pos, whence) < 0)
    {
      g_message ("Seek error: %s", g_strerror (errno));
      fclose (f);
      return -1;
    }
  return 0;
}

static gint
read_creator_block (FILE     *f,
		    gint      image_ID,
		    guint     total_len,
		    PSPimage *ia)
{
  long data_start;
  guchar buf[4];
  guint16 keyword;
  guint32 length;
  gchar *string;
  gchar *title = NULL, *artist = NULL, *copyright = NULL, *description = NULL;
  guint32 dword;
  guint32 cdate = 0, mdate = 0, appid, appver;
  GString *comment;
  GimpParasite *comment_parasite;

  data_start = ftell (f);
  comment = g_string_new (NULL);

  while (ftell (f) < data_start + total_len)
    {
      if (fread (buf, 4, 1, f) < 1
	  || fread (&keyword, 2, 1, f) < 1
	  || fread (&length, 4, 1, f) < 1)
	{
	  g_message ("Error reading creator keyword chunk");
	  return -1;
	}
      if (memcmp (buf, "~FL\0", 4) != 0)
	{
	  g_message ("Invalid keyword chunk header");
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
	      g_message ("Error reading creator keyword data");
	      return -1;
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
	      g_message ("Error reading creator keyword data");
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
	  if (try_fseek (f, length, SEEK_CUR) < 0)
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
      gimp_image_parasite_attach(image_ID, comment_parasite);
      gimp_parasite_free (comment_parasite);
    }

  g_string_free (comment, FALSE);

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

static GimpLayerModeEffects
gimp_layer_mode_from_psp_blend_mode (PSPLayerBlendModes mode)
{
  switch (mode)
    {
    case PSP_BLEND_NORMAL:
      return GIMP_NORMAL_MODE;
    case PSP_BLEND_DARKEN:
      return GIMP_DARKEN_ONLY_MODE;
    case PSP_BLEND_LIGHTEN:
      return GIMP_LIGHTEN_ONLY_MODE;
    case PSP_BLEND_HUE:
      return GIMP_HUE_MODE;
    case PSP_BLEND_SATURATION:
      return GIMP_SATURATION_MODE;
    case PSP_BLEND_COLOR:
      return GIMP_COLOR_MODE;
    case PSP_BLEND_LUMINANCE:
      return GIMP_VALUE_MODE;	/* ??? */
    case PSP_BLEND_MULTIPLY:
      return GIMP_MULTIPLY_MODE;
    case PSP_BLEND_SCREEN:
      return GIMP_SCREEN_MODE;
    case PSP_BLEND_DISSOLVE:
      return GIMP_DISSOLVE_MODE;
    case PSP_BLEND_OVERLAY:
      return GIMP_OVERLAY_MODE;
    case PSP_BLEND_HARD_LIGHT:
    case PSP_BLEND_SOFT_LIGHT:
      return -1;
    case PSP_BLEND_DIFFERENCE:
      return GIMP_DIFFERENCE_MODE;
    case PSP_BLEND_DODGE:
    case PSP_BLEND_BURN:
    case PSP_BLEND_EXCLUSION:
      return -1;		/* ??? */
    }
  return -1;
}

static gchar *
blend_mode_name (PSPLayerBlendModes mode)
{
  static gchar *blend_mode_names[] =
  {
    "NORMAL",
    "DARKEN",
    "LIGHTEN",
    "HUE",
    "SATURATION",
    "COLOR",
    "LUMINANCE",
    "MULTIPLY",
    "SCREEN",
    "DISSOLVE",
    "OVERLAY",
    "HARD_LIGHT",
    "SOFT_LIGHT",
    "DIFFERENCE",
    "DODGE",
    "BURN",
    "EXCLUSION"
  };
  static gchar *err_name = NULL;

  if (mode >= 0 && mode <= PSP_BLEND_EXCLUSION)
    return blend_mode_names[mode];

  g_free (err_name);
  err_name = g_strdup_printf ("unknown layer blend mode %d", mode);

  return err_name;
}

static gchar *
bitmap_type_name (gint type)
{
  static gchar *bitmap_type_names[] =
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

static gchar *
channel_type_name (gint type)
{
  static char *channel_type_names[] =
  {
    "COMPOSITE",
    "RED",
    "GREEN",
    "BLUE"
  };
  static gchar *err_name = NULL;

  if (type >= 0 && type <= PSP_DIB_THUMBNAIL)
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

static int
read_channel_data (FILE       *f,
		   PSPimage   *ia,
		   guchar    **pixels,
		   guint       bytespp,
		   guint       offset,
		   GimpDrawable  *drawable,
		   guint32     compressed_len)
{
  gint i, y, width = drawable->width, height = drawable->height;
  gint npixels = width * height;
  guchar *buf, *p, *q, *endq;
  guchar *buf2 = NULL;  /* please the compiler */
  guchar runcount, byte;
  z_stream zstream;

  switch (ia->compression)
    {
    case PSP_COMP_NONE:
      if (bytespp == 1)
	{
	  if ((width % 4) == 0)
	    fread (pixels[0], height * width, 1, f);
	  else
	    {
	      for (y = 0; y < height; y++)
		{
		  fread (pixels[y], width, 1, f);
		  fseek (f, 4 - (width % 4), SEEK_CUR);
		}
	    }
	}
      else
	{
	  buf = g_malloc (width);
	  for (y = 0; y < height; y++)
	    {
	      fread (buf, width, 1, f);
	      if (width % 4)
		fseek (f, 4 - (width % 4), SEEK_CUR);
	      p = buf;
	      q = pixels[y] + offset;
	      for (i = 0; i < width; i++)
		{
		  *q = *p++;
		  q += bytespp;
		}
	    }
	  g_free (buf);
	}
      break;

    case PSP_COMP_RLE:
      q = pixels[0] + offset;
      endq = q + npixels * bytespp;
      buf = g_malloc (127);
      while (q < endq)
	{
	  p = buf;
	  fread (&runcount, 1, 1, f);
	  if (runcount > 128)
	    {
	      runcount -= 128;
	      fread (&byte, 1, 1, f);
	      memset (buf, byte, runcount);
	    }
	  else
	    fread (buf, runcount, 1, f);
	  if (bytespp == 1)
	    {
	      memmove (q, buf, runcount);
	      q += runcount;
	    }
	  else
	    {
	      p = buf;
	      for (i = 0; i < runcount; i++)
		{
		  *q = *p++;
		  q += bytespp;
		}
	    }
	}
      g_free (buf);
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
	  g_message ("zlib error");
	  return -1;
	}
      if (bytespp == 1)
	zstream.next_out = pixels[0];
      else
	{
	  buf2 = g_malloc (npixels);
	  zstream.next_out = buf2;
	}
      zstream.avail_out = npixels;
      if (inflate (&zstream, Z_FINISH) != Z_STREAM_END)
	{
	  g_message ("zlib error");
	  inflateEnd (&zstream);
	  return -1;
	}
      inflateEnd (&zstream);
      g_free (buf);

      if (bytespp > 1)
	{
	  p = buf2;
	  q = pixels[0] + offset;
	  for (i = 0; i < npixels; i++)
	    {
	      *q = *p++;
	      q += bytespp;
	    }
	  g_free (buf2);
	}
      break;
    }

  return 0;
}

static gint
read_layer_block (FILE     *f,
		  gint      image_ID,
		  guint     total_len,
		  PSPimage *ia)
{
  gint i;
  long block_start, sub_block_start, channel_start;
  gint sub_id;
  guint32 sub_init_len, sub_total_len;
  gchar *name = NULL;
  guint16 namelen;
  guchar type, opacity, blend_mode, visibility, transparency_protected;
  guchar link_group_id, mask_linked, mask_disabled;
  guint32 image_rect[4], saved_image_rect[4], mask_rect[4], saved_mask_rect[4];
  gboolean null_layer = FALSE;
  guint16 bitmap_count, channel_count;
  GimpImageType drawable_type;
  guint32 layer_ID = 0;
  GimpLayerModeEffects layer_mode;
  guint32 channel_init_len, channel_total_len;
  guint32 compressed_len, uncompressed_len;
  guint16 bitmap_type, channel_type;
  gint width, height, bytespp, offset;
  guchar **pixels, *pixel;
  GimpDrawable *drawable;
  GimpPixelRgn pixel_rgn;

  block_start = ftell (f);

  while (ftell (f) < block_start + total_len)
    {
      /* Read the layer sub-block header */
      sub_id = read_block_header (f, &sub_init_len, &sub_total_len);
      if (sub_id == -1)
        return -1;

      if (sub_id != PSP_LAYER_BLOCK)
	{
	  g_message ("Invalid layer sub-block %s, should be LAYER",
		     block_name (sub_id));
	  return -1;
	}

      sub_block_start = ftell (f);

      /* Read layer information chunk */
      if (major >= 4)
	{
	  if (fseek (f, 4, SEEK_CUR) < 0
	      || fread (&namelen, 2, 1, f) < 1
	      || ((namelen = GUINT16_FROM_LE (namelen)) && FALSE)
	      || (name = g_malloc (namelen + 1)) == NULL
	      || fread (name, namelen, 1, f) < 1
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
	      || fseek (f, 47, SEEK_CUR) < 0
	      || fread (&bitmap_count, 2, 1, f) < 1
	      || fread (&channel_count, 2, 1, f) < 1)
	    {
	      g_message ("Error reading layer information chunk");
              g_free (name);
	      return -1;
	    }

	  name[namelen] = 0;
	  type = PSP_LAYER_NORMAL; /* ??? */
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
	      g_message ("Error reading layer information chunk");
	      g_free (name);
	      return -1;
	    }
	}

      if (type == PSP_LAYER_FLOATING_SELECTION)
	g_message ("Floating selection restored as normal layer");

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
		     blend_mode_name (blend_mode), name);
	  layer_mode = GIMP_NORMAL_MODE;
	  visibility = FALSE;
	}

      width = saved_image_rect[2] - saved_image_rect[0];
      height = saved_image_rect[3] - saved_image_rect[1];

      if ((width < 0) || (width > GIMP_MAX_IMAGE_SIZE)       /* w <= 2^18 */
          || (height < 0) || (height > GIMP_MAX_IMAGE_SIZE)  /* h <= 2^18 */
          || ((width / 256) * (height / 256) >= 8192))       /* w * h < 2^29 */
        {
          g_message ("Invalid layer dimensions: %dx%d", width, height);
          return -1;
        }

      IFDBG(2) g_message
	("layer: %s %dx%d (%dx%d) @%d,%d opacity %d blend_mode %s "
	 "%d bitmaps %d channels",
	 name,
	 image_rect[2] - image_rect[0], image_rect[3] - image_rect[1],
	 width, height,
	 saved_image_rect[0], saved_image_rect[1],
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

      if (ia->greyscale)
	if (!null_layer && bitmap_count == 1)
	  drawable_type = GIMP_GRAY_IMAGE, bytespp = 1;
	else
	  drawable_type = GIMP_GRAYA_IMAGE, bytespp = 1;
      else
	if (!null_layer && bitmap_count == 1)
	  drawable_type = GIMP_RGB_IMAGE, bytespp = 3;
	else
	  drawable_type = GIMP_RGBA_IMAGE, bytespp = 4;

      layer_ID = gimp_layer_new (image_ID, name,
				 width, height,
				 drawable_type,
				 100.0 * opacity / 255.0,
				 layer_mode);
      if (layer_ID == -1)
	{
	  g_message ("Error creating layer");
	  return -1;
	}

      g_free (name);

      gimp_image_add_layer (image_ID, layer_ID, -1);

      if (saved_image_rect[0] != 0 || saved_image_rect[1] != 0)
	gimp_layer_set_offsets (layer_ID,
				saved_image_rect[0], saved_image_rect[1]);

      if (!visibility)
	gimp_drawable_set_visible (layer_ID, FALSE);

      gimp_layer_set_lock_alpha (layer_ID, transparency_protected);

      if (major < 4)
	if (try_fseek (f, sub_block_start + sub_init_len, SEEK_SET) < 0)
	  {
	    return -1;
	  }

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

      drawable = gimp_drawable_get (layer_ID);
      gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
			   width, height, TRUE, FALSE);

      gimp_tile_cache_size (gimp_tile_height () * width * bytespp);

      /* Read the layer channel sub-blocks */
      while (ftell (f) < sub_block_start + sub_total_len)
	{
	  sub_id = read_block_header (f, &channel_init_len,
				      &channel_total_len);
	  if (sub_id == -1)
	    {
	      gimp_image_delete (image_ID);
	      return -1;
	    }

	  if (sub_id != PSP_CHANNEL_BLOCK)
	    {
	      g_message ("Invalid layer sub-block %s, should be CHANNEL",
			 block_name (sub_id));
	      return -1;
	    }

	  channel_start = ftell (f);

	  if (major == 4)
	    fseek (f, 4, SEEK_CUR); /* Unknown field */

	  if (fread (&compressed_len, 4, 1, f) < 1
	      || fread (&uncompressed_len, 4, 1, f) < 1
	      || fread (&bitmap_type, 2, 1, f) < 1
	      || fread (&channel_type, 2, 1, f) < 1)
	    {
	      g_message ("Error reading channel information chunk");
	      return -1;
	    }

	  compressed_len = GUINT32_FROM_LE (compressed_len);
	  uncompressed_len = GUINT32_FROM_LE (uncompressed_len);
	  bitmap_type = GUINT16_FROM_LE (bitmap_type);
	  channel_type = GUINT16_FROM_LE (channel_type);

	  if (bitmap_type > PSP_DIB_USER_MASK)
	    {
	      g_message ("Invalid bitmap type %d in channel information chunk",
			 bitmap_type);
	      return -1;
	    }

	  if (channel_type > PSP_CHANNEL_BLUE)
	    {
	      g_message ("Invalid channel type %d in channel information chunk",
			 channel_type);
	      return -1;
	    }

	  IFDBG(2) g_message ("channel: %s %s %d (%d) bytes %d bytespp",
			      bitmap_type_name (bitmap_type),
			      channel_type_name (channel_type),
			      uncompressed_len, compressed_len,
			      bytespp);

	  if (bitmap_type == PSP_DIB_TRANS_MASK)
	    offset = 3;
	  else
	    offset = channel_type - PSP_CHANNEL_RED;

	  if (major < 4)
	    if (try_fseek (f, channel_start + channel_init_len, SEEK_SET) < 0)
	      {
		return -1;
	      }

	  if (!null_layer)
	    if (read_channel_data (f, ia, pixels, bytespp,
				   offset, drawable, compressed_len) == -1)
	      {
		return -1;
	      }

	  if (try_fseek (f, channel_start + channel_total_len, SEEK_SET) < 0)
	    {
	      return -1;
	    }
	}

      gimp_pixel_rgn_set_rect (&pixel_rgn, pixel, 0, 0, width, height);

      gimp_drawable_flush (drawable);
      gimp_drawable_detach (drawable);

      g_free (pixels);
      g_free (pixel);
    }

  if (try_fseek (f, block_start + total_len, SEEK_SET) < 0)
    {
      return -1;
    }

  return layer_ID;
}

static gint
read_tube_block (FILE     *f,
		 gint      image_ID,
		 guint     total_len,
		 PSPimage *ia)
{
  guint16 version;
  guchar name[514];
  guint32 step_size, column_count, row_count, cell_count;
  guint32 placement_mode, selection_mode;
  gint i;
  GimpPixPipeParams params;
  GimpParasite *pipe_parasite;
  gchar *parasite_text;

  gimp_pixpipe_params_init (&params);

  if (fread (&version, 2, 1, f) < 1
      || fread (name, 513, 1, f) < 1
      || fread (&step_size, 4, 1, f) < 1
      || fread (&column_count, 4, 1, f) < 1
      || fread (&row_count, 4, 1, f) < 1
      || fread (&cell_count, 4, 1, f) < 1
      || fread (&placement_mode, 4, 1, f) < 1
      || fread (&selection_mode, 4, 1, f) < 1)
    {
      g_message ("Error reading tube data chunk");
      return -1;
    }

  name[513] = 0;
  version = GUINT16_FROM_LE (version);
  params.step = GUINT32_FROM_LE (step_size);
  params.cols = GUINT32_FROM_LE (column_count);
  params.rows = GUINT32_FROM_LE (row_count);
  params.ncells = GUINT32_FROM_LE (cell_count);
  placement_mode = GUINT32_FROM_LE (placement_mode);
  selection_mode = GUINT32_FROM_LE (selection_mode);

  for (i = 1; i < column_count; i++)
    gimp_image_add_vguide (image_ID, (ia->width * i)/column_count);
  for (i = 1; i < row_count; i++)
    gimp_image_add_hguide (image_ID, (ia->height * i)/row_count);

  /* We use a parasite to pass in the tube (pipe) parameters in
   * case we will have any use of those, for instance in the gpb
   * plug-in that saves a GIMP image pipe.
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
  parasite_text = gimp_pixpipe_params_build (&params);

  IFDBG(2) g_message ("parasite: %s", parasite_text);

  pipe_parasite = gimp_parasite_new ("gimp-brush-pipe-parameters",
				     GIMP_PARASITE_PERSISTENT,
				     strlen (parasite_text) + 1, parasite_text);
  gimp_image_parasite_attach (image_ID, pipe_parasite);
  gimp_parasite_free (pipe_parasite);
  g_free (parasite_text);

  return 0;
}

static gchar *
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

static gint32
load_image (const gchar *filename)
{
  FILE *f;
  struct stat st;
  char buf[32];
  PSPimage ia;
  guint32 block_init_len, block_total_len;
  long block_start;
  PSPBlockID id = -1;
  gint block_number;

  gint32 image_ID = -1;

  if (g_stat (filename, &st) == -1)
    return -1;

  f = g_fopen (filename, "rb");
  if (f == NULL)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  /* Read thePSP File Header */
  if (fread (buf, 32, 1, f) < 1
      || fread (&major, 2, 1, f) < 1
      || fread (&minor, 2, 1, f) < 1)
    {
      g_message ("Error reading file header");
      goto error;
    }

  if (memcmp (buf, "Paint Shop Pro Image File\n\032\0\0\0\0\0", 32) != 0)
    {
      g_message ("Incorrect file signature");
      goto error;
    }

  major = GUINT16_FROM_LE (major);
  minor = GUINT16_FROM_LE (minor);

  /* I only have the documentation for file format version 3.0,
   * but PSP 6 writes version 4.0. Let's hope it's backwards compatible.
   * Earlier versions probably don't have all the fields I expect
   * so don't accept those.
   */
  if (major < 3)
    {
      g_message ("Unsupported PSP file format version "
		 "%d.%d, only knows 3.0 (and later?)",
		 major, minor);
      goto error;
    }
  else if (major == 3)
    ; /* OK */
  else if (major == 4 && minor == 0)
    g_message ("Warning: PSP file format version "
	       "4.0. Support for this format version "
	       "is based on reverse engineering, "
	       "as no documentation has been made available");
  else
    {
      g_message ("Unsupported PSP file format version %d.%d",
		 major, minor);
      goto error;
    }

  /* Read all the blocks */
  block_number = 0;

  IFDBG(3) g_message ("size = %d", (int)st.st_size);
  while (ftell (f) != st.st_size
	 && (id = read_block_header (f, &block_init_len,
				     &block_total_len)) != -1)
    {
      block_start = ftell (f);

      if (id == PSP_IMAGE_BLOCK)
	{
	  if (block_number != 0)
	    {
	      g_message ("Duplicate General Image Attributes block");
              goto error;
	    }
	  if (read_general_image_attribute_block (f, block_init_len,
						  block_total_len, &ia) == -1)
            {
              goto error;
            }

	  IFDBG(2) g_message ("%d dpi %dx%d %s",
			      (int) ia.resolution,
			      ia.width, ia.height,
			      compression_name (ia.compression));

	  image_ID = gimp_image_new (ia.width, ia.height,
				     ia.greyscale ? GIMP_GRAY : GIMP_RGB);
	  if (image_ID == -1)
            {
              goto error;
            }

	  gimp_image_set_filename (image_ID, filename);

	  gimp_image_set_resolution (image_ID, ia.resolution, ia.resolution);
	}
      else
	{
	  if (block_number == 0)
	    {
	      g_message ("Missing General Image Attributes block");
              goto error;
	    }

	  switch (id)
	    {
	    case PSP_CREATOR_BLOCK:
	      if (read_creator_block (f, image_ID, block_total_len, &ia) == -1)
                goto error;
	      break;

	    case PSP_COLOR_BLOCK:
	      break;		/* Not yet implemented */

	    case PSP_LAYER_START_BLOCK:
	      if (read_layer_block (f, image_ID, block_total_len, &ia) == -1)
		goto error;
	      break;

	    case PSP_SELECTION_BLOCK:
	      break;		/* Not yet implemented */

	    case PSP_ALPHA_BANK_BLOCK:
	      break;		/* Not yet implemented */

	    case PSP_THUMBNAIL_BLOCK:
	      break;		/* No use for it */

	    case PSP_EXTENDED_DATA_BLOCK:
	      break;		/* Not yet implemented */

	    case PSP_TUBE_BLOCK:
	      if (read_tube_block (f, image_ID, block_total_len, &ia) == -1)
		goto error;
	      break;

	    case PSP_LAYER_BLOCK:
	    case PSP_CHANNEL_BLOCK:
	    case PSP_ALPHA_CHANNEL_BLOCK:
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

      if (try_fseek (f, block_start + block_total_len, SEEK_SET) < 0)
        goto error;

      block_number++;
    }

  if (id == -1)
    {
    error:
      fclose (f);
      if (image_ID != -1)
        gimp_image_delete (image_ID);
      return -1;
    }

  fclose (f);

  return image_ID;
}

static gint
save_image (const gchar *filename,
	    gint32       image_ID,
	    gint32       drawable_ID)
{
  g_message ("Saving not implemented yet");

  return FALSE;
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  gint32            drawable_ID;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[1].type         = GIMP_PDB_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_ui_init (PLUG_IN_BINARY, FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "PSP",
				      GIMP_EXPORT_CAN_HANDLE_RGB     |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                      GIMP_EXPORT_CAN_HANDLE_LAYERS);
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }
	  break;
	default:
	  break;
	}

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data (SAVE_PROC, &psvals);

	  /*  First acquire information with a dialog  */
	  if (! save_dialog ())
	    status = GIMP_PDB_CANCEL;
	  break;

	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 6)
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
	  else
	    {
	      psvals.compression = (param[5].data.d_int32) ? TRUE : FALSE;

	      if (param[5].data.d_int32 < 0 ||
		  param[5].data.d_int32 > PSP_COMP_LZ77)
		status = GIMP_PDB_CALLING_ERROR;
	    }

	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_get_data (SAVE_PROC, &psvals);
	  break;

	default:
	  break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  if (save_image (param[3].data.d_string, image_ID, drawable_ID))
	    {
	      gimp_set_data (SAVE_PROC, &psvals, sizeof (PSPSaveVals));
	    }
	  else
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
	    }
	}

      if (export == GIMP_EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}
