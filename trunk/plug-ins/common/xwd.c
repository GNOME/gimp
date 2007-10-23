/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * XWD reading and writing code Copyright (C) 1996 Peter Kirchgessner
 * (email: peter@kirchgessner.net, WWW: http://www.kirchgessner.net)
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
 *
 */

/*
 * XWD-input/output was written by Peter Kirchgessner (peter@kirchgessner.net)
 * Examples from mainly used UNIX-systems have been used for testing.
 * If a file does not work, please return a small (!!) compressed example.
 * Currently the following formats are supported:
 *    pixmap_format | pixmap_depth | bits_per_pixel
 *    ---------------------------------------------
 *          0       |        1     |       1
 *          1       |    1,...,24  |       1
 *          2       |        1     |       1
 *          2       |    1,...,8   |       8
 *          2       |    1,...,16  |      16
 *          2       |    1,...,24  |      24
 *          2       |    1,...,24  |      32
 */
/* Event history:
 * PK = Peter Kirchgessner, ME = Mattias Engdegård
 * V 1.00, PK, xx-Aug-96: First try
 * V 1.01, PK, 03-Sep-96: Check for bitmap_bit_order
 * V 1.90, PK, 17-Mar-97: Upgrade to work with GIMP V0.99
 *                        Use visual class 3 to write indexed image
 *                        Set gimp b/w-colormap if no xwdcolormap present
 * V 1.91, PK, 05-Apr-97: Return all arguments, even in case of an error
 * V 1.92, PK, 12-Oct-97: No progress bars for non-interactive mode
 * V 1.93, PK, 11-Apr-98: Fix problem with overwriting memory
 * V 1.94, ME, 27-Feb-00: Remove superfluous little-endian support (format is
                          specified as big-endian). Trim magic header
 * V 1.95, PK, 02-Jul-01: Fix problem with 8 bit image
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-xwd-load"
#define SAVE_PROC      "file-xwd-save"
#define PLUG_IN_BINARY "xwd"


typedef gulong  L_CARD32;
typedef gushort L_CARD16;
typedef guchar  L_CARD8;

typedef struct
{
  L_CARD32 l_header_size;    /* Header size */

  L_CARD32 l_file_version;   /* File version (7) */
  L_CARD32 l_pixmap_format;  /* Image type */
  L_CARD32 l_pixmap_depth;   /* Number of planes */
  L_CARD32 l_pixmap_width;   /* Image width */
  L_CARD32 l_pixmap_height;  /* Image height */
  L_CARD32 l_xoffset;        /* x-offset (0 ?) */
  L_CARD32 l_byte_order;     /* Byte ordering */

  L_CARD32 l_bitmap_unit;
  L_CARD32 l_bitmap_bit_order; /* Bit order */
  L_CARD32 l_bitmap_pad;
  L_CARD32 l_bits_per_pixel; /* Number of bits per pixel */

  L_CARD32 l_bytes_per_line; /* Number of bytes per scanline */
  L_CARD32 l_visual_class;   /* Visual class */
  L_CARD32 l_red_mask;       /* Red mask */
  L_CARD32 l_green_mask;     /* Green mask */
  L_CARD32 l_blue_mask;      /* Blue mask */
  L_CARD32 l_bits_per_rgb;   /* Number of bits per RGB-part */
  L_CARD32 l_colormap_entries; /* Number of colors in color table (?) */
  L_CARD32 l_ncolors;        /* Number of xwdcolor structures */
  L_CARD32 l_window_width;   /* Window width */
  L_CARD32 l_window_height;  /* Window height */
  L_CARD32 l_window_x;       /* Window position x */
  L_CARD32 l_window_y;       /* Window position y */
  L_CARD32 l_window_bdrwidth;/* Window border width */
} L_XWDFILEHEADER;

typedef struct
{
  L_CARD32 l_pixel;          /* Color index */
  L_CARD16 l_red, l_green, l_blue;  /* RGB-values */
  L_CARD8  l_flags, l_pad;
} L_XWDCOLOR;


/* Some structures for mapping up to 32bit-pixel */
/* values which are kept in the XWD-Colormap */

#define MAPPERBITS 12
#define MAPPERMASK ((1 << MAPPERBITS)-1)

typedef struct
{
  L_CARD32 pixel_val;
  guchar   red;
  guchar   green;
  guchar   blue;
} PMAP;

typedef struct
{
  gint   npixel; /* Number of pixel values in map */
  guchar pixel_in_map[1 << MAPPERBITS];
  PMAP   pmap[256];
} PIXEL_MAP;

#define XWDHDR_PAD   0  /* Total number of padding bytes for XWD header */
#define XWDCOL_PAD   0  /* Total number of padding bytes for each XWD color */

/* Declare some local functions.
 */
static void   query               (void);
static void   run                 (const gchar      *name,
                                   gint              nparams,
                                   const GimpParam  *param,
                                   gint             *nreturn_vals,
                                   GimpParam       **return_vals);

static gint32 load_image          (const gchar      *filename);
static gint   save_image          (const gchar      *filename,
                                   gint32            image_ID,
                                   gint32            drawable_ID);
static gint32 create_new_image    (const gchar      *filename,
                                   guint             width,
                                   guint             height,
                                   GimpImageBaseType type,
                                   gint32           *layer_ID,
                                   GimpDrawable    **drawable,
                                   GimpPixelRgn     *pixel_rgn);

static int      set_pixelmap      (gint,
                                   L_XWDCOLOR *,
                                   PIXEL_MAP *);
static gboolean get_pixelmap      (L_CARD32,
                                   PIXEL_MAP *,
                                   guchar *,
                                   guchar *,
                                   guchar *);
static void   set_bw_color_table  (gint32);
static void   set_color_table     (gint32,
                                   L_XWDFILEHEADER *,
                                   L_XWDCOLOR *);

static gint32 load_xwd_f2_d1_b1   (const gchar *,
                                   FILE *,
                                   L_XWDFILEHEADER *,
                                   L_XWDCOLOR *);
static gint32 load_xwd_f2_d8_b8   (const gchar *,
                                   FILE *,
                                   L_XWDFILEHEADER *,
                                   L_XWDCOLOR *);
static gint32 load_xwd_f2_d16_b16 (const gchar *,
                                   FILE *,
                                   L_XWDFILEHEADER *,
                                   L_XWDCOLOR *);
static gint32 load_xwd_f2_d24_b32 (const gchar *,
                                   FILE *,
                                   L_XWDFILEHEADER *,
                                   L_XWDCOLOR *);
static gint32 load_xwd_f1_d24_b1  (const gchar *,
                                   FILE *,
                                   L_XWDFILEHEADER *,
                                   L_XWDCOLOR *);

static L_CARD32 read_card32  (FILE *,
                              gint *);
static L_CARD16 read_card16  (FILE *,
                              gint *);
static L_CARD8  read_card8   (FILE *,
                              gint *);

static void write_card32     (FILE *,
                              L_CARD32);
static void write_card16     (FILE *,
                              L_CARD32);
static void write_card8      (FILE *,
                              L_CARD32);

static void read_xwd_header  (FILE *,
                              L_XWDFILEHEADER *);
static void write_xwd_header (FILE *,
                              L_XWDFILEHEADER *);
static void read_xwd_cols    (FILE *,
                              L_XWDFILEHEADER *,
                              L_XWDCOLOR *);
static void write_xwd_cols   (FILE *,
                              L_XWDFILEHEADER *,
                              L_XWDCOLOR *);

static gint save_index       (FILE *,
                              gint32,
                              gint32,
                              gint);
static gint save_rgb         (FILE *,
                              gint32,
                              gint32);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


/* The run mode */
static GimpRunMode l_run_mode;


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
    { GIMP_PDB_IMAGE,  "image",        "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files in the XWD (X Window Dump) format",
                          "Loads files in the XWD (X Window Dump) format. "
                          "XWD image files are produced by the program xwd. "
                          "Xwd is an X Window System window dumping utility.",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          "1996",
                          N_("X window dump"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-xwindowdump");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "xwd",
                                    "",
                                    "4,long,0x00000007");

  gimp_install_procedure (SAVE_PROC,
                          "Saves files in the XWD (X Window Dump) format",
                          "XWD saving handles all image types except "
                          "those with alpha channels.",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          "1996",
                          N_("X window dump"),
                          "RGB, GRAY, INDEXED",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-xwindowdump");
  gimp_register_save_handler (SAVE_PROC, "xwd", "");
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

  l_run_mode = run_mode = param[0].data.d_int32;

  INIT_I18N ();

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
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          export = gimp_export_image (&image_ID, &drawable_ID, "XWD",
                                      (GIMP_EXPORT_CAN_HANDLE_RGB |
                                       GIMP_EXPORT_CAN_HANDLE_GRAY |
                                       GIMP_EXPORT_CAN_HANDLE_INDEXED));
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
        case GIMP_RUN_WITH_LAST_VALS:
          /* No additional data to retrieve */
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 5)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (! save_image (param[3].data.d_string, image_ID, drawable_ID))
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CANCEL;
    }

  values[0].data.d_status = status;
}

static gint32
load_image (const gchar *filename)
{
  FILE            *ifp;
  gint             depth, bpp;
  gint32           image_ID;
  L_XWDFILEHEADER  xwdhdr;
  L_XWDCOLOR      *xwdcolmap = NULL;

  ifp = g_fopen (filename, "rb");
  if (!ifp)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  read_xwd_header (ifp, &xwdhdr);
  if (xwdhdr.l_file_version != 7)
    {
      g_message (_("Could not read XWD header from '%s'"),
                 gimp_filename_to_utf8 (filename));
      fclose (ifp);
      return -1;
    }

#ifdef XWD_COL_WAIT_DEBUG
  {
    int k = 1;

    while (k)
      k = k;
  }
#endif

  /* Position to start of XWDColor structures */
  fseek (ifp, (long)xwdhdr.l_header_size, SEEK_SET);

  if (xwdhdr.l_colormap_entries > 0)
    {
      xwdcolmap = g_new (L_XWDCOLOR, xwdhdr.l_colormap_entries);

      read_xwd_cols (ifp, &xwdhdr, xwdcolmap);

#ifdef XWD_COL_DEBUG
      {
        int j;
        printf ("File %s\n",filename);
        for (j = 0; j < xwdhdr.l_colormap_entries; j++)
          printf ("Entry 0x%08lx: 0x%04lx,  0x%04lx, 0x%04lx, %d\n",
                  (long)xwdcolmap[j].l_pixel,(long)xwdcolmap[j].l_red,
                  (long)xwdcolmap[j].l_green,(long)xwdcolmap[j].l_blue,
                  (int)xwdcolmap[j].l_flags);
      }
#endif

      if (xwdhdr.l_file_version != 7)
        {
          g_message (_("Can't read color entries"));
          g_free (xwdcolmap);
          fclose (ifp);
          return (-1);
        }
    }

  if (xwdhdr.l_pixmap_width <= 0)
    {
      g_message (_("'%s':\nNo image width specified"),
                 gimp_filename_to_utf8 (filename));
      fclose (ifp);
      return (-1);
    }

  if (xwdhdr.l_pixmap_width > GIMP_MAX_IMAGE_SIZE
      || xwdhdr.l_bytes_per_line > GIMP_MAX_IMAGE_SIZE * 3)
    {
      g_message (_("'%s':\nImage width is larger than GIMP can handle"),
                 gimp_filename_to_utf8 (filename));
      fclose (ifp);
      return (-1);
    }

  if (xwdhdr.l_pixmap_height <= 0)
    {
      g_message (_("'%s':\nNo image height specified"),
                 gimp_filename_to_utf8 (filename));
      fclose (ifp);
      return (-1);
    }

  if (xwdhdr.l_pixmap_height > GIMP_MAX_IMAGE_SIZE)
    {
      g_message (_("'%s':\nImage height is larger than GIMP can handle"),
                 gimp_filename_to_utf8 (filename));
      fclose (ifp);
      return (-1);
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  depth = xwdhdr.l_pixmap_depth;
  bpp   = xwdhdr.l_bits_per_pixel;

  image_ID = -1;
  switch (xwdhdr.l_pixmap_format)
    {
    case 0:    /* Single plane bitmap */
      if ((depth == 1) && (bpp == 1))
        { /* Can be performed by format 2 loader */
          image_ID = load_xwd_f2_d1_b1 (filename, ifp, &xwdhdr, xwdcolmap);
        }
      break;

    case 1:    /* Single plane pixmap */
      if ((depth <= 24) && (bpp == 1))
        {
          image_ID = load_xwd_f1_d24_b1 (filename, ifp, &xwdhdr, xwdcolmap);
        }
      break;

    case 2:    /* Multiplane pixmaps */
      if ((depth == 1) && (bpp == 1))
        {
          image_ID = load_xwd_f2_d1_b1 (filename, ifp, &xwdhdr, xwdcolmap);
        }
      else if ((depth <= 8) && (bpp == 8))
        {
          image_ID = load_xwd_f2_d8_b8 (filename, ifp, &xwdhdr, xwdcolmap);
        }
      else if ((depth <= 16) && (bpp == 16))
        {
          image_ID = load_xwd_f2_d16_b16 (filename, ifp, &xwdhdr, xwdcolmap);
        }
      else if ((depth <= 24) && ((bpp == 24) || (bpp == 32)))
        {
          image_ID = load_xwd_f2_d24_b32 (filename, ifp, &xwdhdr, xwdcolmap);
        }
      break;
    }

  fclose (ifp);

  if (xwdcolmap)
    g_free (xwdcolmap);

  if (image_ID == -1)
    g_message (_("XWD-file %s has format %d, depth %d and bits per pixel %d. "
                 "Currently this is not supported."),
               gimp_filename_to_utf8 (filename),
               (gint) xwdhdr.l_pixmap_format, depth, bpp);

  return image_ID;
}

static gint
save_image (const gchar *filename,
            gint32       image_ID,
            gint32       drawable_ID)

{
  FILE          *ofp;
  GimpImageType  drawable_type;
  gint           retval;

  drawable_type = gimp_drawable_type (drawable_ID);

  /*  Make sure we're not saving an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable_ID))
    {
      g_message (_("Cannot save images with alpha channels."));
      return FALSE;
    }

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
    case GIMP_GRAY_IMAGE:
    case GIMP_RGB_IMAGE:
      break;
    default:
      g_message (_("Cannot operate on unknown image types."));
      return FALSE;
      break;
    }

  /* Open the output file. */
  ofp = g_fopen (filename, "wb");
  if (!ofp)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (drawable_type == GIMP_INDEXED_IMAGE)
    retval = save_index (ofp, image_ID, drawable_ID, 0);
  else if (drawable_type == GIMP_GRAY_IMAGE)
    retval = save_index (ofp, image_ID, drawable_ID, 1);
  else if (drawable_type == GIMP_RGB_IMAGE)
    retval = save_rgb (ofp, image_ID, drawable_ID);
  else
    retval = FALSE;

  fclose (ofp);

  return retval;
}


static L_CARD32
read_card32 (FILE *ifp,
             int  *err)

{
  L_CARD32 c;

  c =  (((L_CARD32) (getc (ifp))) << 24);
  c |= (((L_CARD32) (getc (ifp))) << 16);
  c |= (((L_CARD32) (getc (ifp))) << 8);
  c |= ((L_CARD32) (*err = getc (ifp)));

  *err = (*err < 0);

  return c;
}


static L_CARD16
read_card16 (FILE *ifp,
             int  *err)

{
  L_CARD16 c;

  c =  (((L_CARD16) (getc (ifp))) << 8);
  c |= ((L_CARD16) (*err = getc (ifp)));

  *err = (*err < 0);

  return c;
}


static L_CARD8
read_card8 (FILE *ifp,
            int  *err)
{
  L_CARD8 c;

  c = ((L_CARD8) (*err = getc (ifp)));

  *err = (*err < 0);

  return c;
}

static void
write_card32 (FILE     *ofp,
              L_CARD32  c)
{
  putc ((int) ((c >> 24) & 0xff), ofp);
  putc ((int) ((c >> 16) & 0xff), ofp);
  putc ((int) ((c >> 8) & 0xff), ofp);
  putc ((int) ((c) & 0xff), ofp);
}


static void
write_card16 (FILE     *ofp,
              L_CARD32  c)
{
  putc ((int) ((c >> 8) & 0xff), ofp);
  putc ((int) ((c) & 0xff), ofp);
}


static void
write_card8 (FILE     *ofp,
             L_CARD32  c)
{
  putc ((int) ((c) & 0xff), ofp);
}


static void
read_xwd_header (FILE            *ifp,
                 L_XWDFILEHEADER *xwdhdr)
{
  gint      j, err;
  L_CARD32 *cp;

  cp = (L_CARD32 *) xwdhdr;

  /* Read in all 32-bit values of the header and check for byte order */
  for (j = 0; j < sizeof (L_XWDFILEHEADER) / sizeof(xwdhdr->l_file_version); j++)
    {
      *(cp++) = read_card32 (ifp, &err);

      if (err)
        break;
    }

  if (err)
    xwdhdr->l_file_version = 0;  /* Not a valid XWD-file */
}


/* Write out an XWD-fileheader. The header size is calculated here */
static void
write_xwd_header (FILE            *ofp,
                  L_XWDFILEHEADER *xwdhdr)

{
  gint      j, hdrpad, hdr_entries;
  L_CARD32 *cp;

  hdrpad = XWDHDR_PAD;
  hdr_entries = sizeof (L_XWDFILEHEADER) / sizeof(xwdhdr->l_file_version);
  xwdhdr->l_header_size = hdr_entries * 4 + hdrpad;

  cp = (L_CARD32 *) xwdhdr;

  /* Write out all 32-bit values of the header and check for byte order */
  for (j = 0; j < sizeof (L_XWDFILEHEADER) / sizeof(xwdhdr->l_file_version); j++)
    {
      write_card32 (ofp, *(cp++));
    }

  /* Add padding bytes after XWD header */
  for (j = 0; j < hdrpad; j++)
    write_card8 (ofp, (L_CARD32) 0);
}


static void
read_xwd_cols (FILE            *ifp,
               L_XWDFILEHEADER *xwdhdr,
               L_XWDCOLOR      *colormap)
{
  gint  j, err = 0;
  gint  flag_is_bad, index_is_bad;
  gint  indexed = (xwdhdr->l_pixmap_depth <= 8);
  glong colmappos = ftell (ifp);

  /* Read in XWD-Color structures (the usual case) */
  flag_is_bad = index_is_bad = 0;
  for (j = 0; j < xwdhdr->l_ncolors; j++)
    {
      colormap[j].l_pixel = read_card32 (ifp, &err);

      colormap[j].l_red   = read_card16 (ifp, &err);
      colormap[j].l_green = read_card16 (ifp, &err);
      colormap[j].l_blue  = read_card16 (ifp, &err);

      colormap[j].l_flags = read_card8 (ifp, &err);
      colormap[j].l_pad   = read_card8 (ifp, &err);

      if (indexed && (colormap[j].l_pixel > 255))
        index_is_bad++;

      if (err)
        break;
    }

  if (err)        /* Not a valid XWD-file ? */
    xwdhdr->l_file_version = 0;
  if (err || ((flag_is_bad == 0) && (index_is_bad == 0)))
    return;

  /* Read in XWD-Color structures (with 4 bytes inserted infront of RGB) */
  fseek (ifp, colmappos, SEEK_SET);
  flag_is_bad = index_is_bad = 0;
  for (j = 0; j < xwdhdr->l_ncolors; j++)
    {
      colormap[j].l_pixel = read_card32 (ifp, &err);

      read_card32 (ifp, &err);  /* Empty bytes on Alpha OSF */

      colormap[j].l_red   = read_card16 (ifp, &err);
      colormap[j].l_green = read_card16 (ifp, &err);
      colormap[j].l_blue  = read_card16 (ifp, &err);

      colormap[j].l_flags = read_card8 (ifp, &err);
      colormap[j].l_pad   = read_card8 (ifp, &err);

      if (indexed && (colormap[j].l_pixel > 255))
        index_is_bad++;

      if (err)
        break;
    }

  if (err)        /* Not a valid XWD-file ? */
    xwdhdr->l_file_version = 0;
  if (err || ((flag_is_bad == 0) && (index_is_bad == 0)))
    return;

  /* Read in XWD-Color structures (with 2 bytes inserted infront of RGB) */
  fseek (ifp, colmappos, SEEK_SET);
  flag_is_bad = index_is_bad = 0;
  for (j = 0; j < xwdhdr->l_ncolors; j++)
    {
      colormap[j].l_pixel = read_card32 (ifp, &err);

      read_card16 (ifp, &err);  /* Empty bytes (from where ?) */

      colormap[j].l_red   = read_card16 (ifp, &err);
      colormap[j].l_green = read_card16 (ifp, &err);
      colormap[j].l_blue  = read_card16 (ifp, &err);

      colormap[j].l_flags = read_card8 (ifp, &err);
      colormap[j].l_pad   = read_card8 (ifp, &err);

      /* if ((colormap[j].l_flags == 0) || (colormap[j].l_flags > 7))
         flag_is_bad++; */

      if (indexed && (colormap[j].l_pixel > 255))
        index_is_bad++;

      if (err)
        break;
    }

  if (err)        /* Not a valid XWD-file ? */
    xwdhdr->l_file_version = 0;
  if (err || ((flag_is_bad == 0) && (index_is_bad == 0)))
    return;

  /* Read in XWD-Color structures (every value is 8 bytes from a CRAY) */
  fseek (ifp, colmappos, SEEK_SET);
  flag_is_bad = index_is_bad = 0;
  for (j = 0; j < xwdhdr->l_ncolors; j++)
    {
      read_card32 (ifp, &err);
      colormap[j].l_pixel = read_card32 (ifp, &err);

      read_card32 (ifp, &err);
      colormap[j].l_red   = read_card32 (ifp, &err);
      read_card32 (ifp, &err);
      colormap[j].l_green = read_card32 (ifp, &err);
      read_card32 (ifp, &err);
      colormap[j].l_blue  = read_card32 (ifp, &err);

      /* The flag byte is kept in the first byte */
      colormap[j].l_flags = read_card8 (ifp, &err);
      colormap[j].l_pad   = read_card8 (ifp, &err);
      read_card16 (ifp, &err);
      read_card32 (ifp, &err);

      if (indexed && (colormap[j].l_pixel > 255))
        index_is_bad++;

       if (err)
         break;
    }

  if (flag_is_bad || index_is_bad)
    {
      printf ("xwd: Warning. Error in XWD-color-structure (");

      if (flag_is_bad) printf ("flag");
      if (index_is_bad) printf ("index");

      printf (")\n");
    }

  if (err)
    xwdhdr->l_file_version = 0;  /* Not a valid XWD-file */
}


static void
write_xwd_cols (FILE            *ofp,
                L_XWDFILEHEADER *xwdhdr,
                L_XWDCOLOR      *colormap)

{
  int j;

  for (j = 0; j < xwdhdr->l_colormap_entries; j++)
    {
#ifdef CRAY
      write_card32 (ofp, (L_CARD32)0);
      write_card32 (ofp, colormap[j].l_pixel);
      write_card32 (ofp, (L_CARD32)0);
      write_card32 (ofp, (L_CARD32)colormap[j].l_red);
      write_card32 (ofp, (L_CARD32)0);
      write_card32 (ofp, (L_CARD32)colormap[j].l_green);
      write_card32 (ofp, (L_CARD32)0);
      write_card32 (ofp, (L_CARD32)colormap[j].l_blue);
      write_card8  (ofp, (L_CARD32)colormap[j].l_flags);
      write_card8  (ofp, (L_CARD32)colormap[j].l_pad);
      write_card16 (ofp, (L_CARD32)0);
      write_card32 (ofp, (L_CARD32)0);
#else
#ifdef __alpha
      write_card32 (ofp, colormap[j].l_pixel);
      write_card32 (ofp, (L_CARD32)0);
      write_card16 (ofp, (L_CARD32)colormap[j].l_red);
      write_card16 (ofp, (L_CARD32)colormap[j].l_green);
      write_card16 (ofp, (L_CARD32)colormap[j].l_blue);
      write_card8  (ofp, (L_CARD32)colormap[j].l_flags);
      write_card8  (ofp, (L_CARD32)colormap[j].l_pad);
#else
      write_card32 (ofp, colormap[j].l_pixel);
      write_card16 (ofp, (L_CARD32)colormap[j].l_red);
      write_card16 (ofp, (L_CARD32)colormap[j].l_green);
      write_card16 (ofp, (L_CARD32)colormap[j].l_blue);
      write_card8  (ofp, (L_CARD32)colormap[j].l_flags);
      write_card8  (ofp, (L_CARD32)colormap[j].l_pad);
#endif
#endif
    }
}


/* Create a map for mapping up to 32 bit pixelvalues to RGB. */
/* Returns number of colors kept in the map (up to 256) */

static gint
set_pixelmap (int         ncols,
              L_XWDCOLOR *xwdcol,
              PIXEL_MAP  *pixelmap)

{
  gint     i, j, k, maxcols;
  L_CARD32 pixel_val;

  memset ((gchar *) pixelmap, 0, sizeof (PIXEL_MAP));

  maxcols = 0;

  for (j = 0; j < ncols; j++) /* For each entry of the XWD colormap */
    {
      pixel_val = xwdcol[j].l_pixel;
      for (k = 0; k < maxcols; k++)  /* Where to insert in list ? */
        {
          if (pixel_val <= pixelmap->pmap[k].pixel_val)
            break;
        }
      if ((k < maxcols) && (pixel_val == pixelmap->pmap[k].pixel_val))
        break;   /* It was already in list */

      if (k >= 256)
        break;

      if (k < maxcols)   /* Must move entries to the back ? */
        {
          for (i = maxcols-1; i >= k; i--)
            memcpy ((char *)&(pixelmap->pmap[i+1]),(char *)&(pixelmap->pmap[i]),
                    sizeof (PMAP));
        }
      pixelmap->pmap[k].pixel_val = pixel_val;
      pixelmap->pmap[k].red = xwdcol[j].l_red >> 8;
      pixelmap->pmap[k].green = xwdcol[j].l_green >> 8;
      pixelmap->pmap[k].blue = xwdcol[j].l_blue >> 8;
      pixelmap->pixel_in_map[pixel_val & MAPPERMASK] = 1;
      maxcols++;
    }
  pixelmap->npixel = maxcols;
#ifdef XWD_COL_DEBUG
  printf ("Colors in pixelmap: %d\n",pixelmap->npixel);
  for (j=0; j<pixelmap->npixel; j++)
    printf ("Pixelvalue 0x%08lx, 0x%02x 0x%02x 0x%02x\n",
            pixelmap->pmap[j].pixel_val,
            pixelmap->pmap[j].red,pixelmap->pmap[j].green,
            pixelmap->pmap[j].blue);
  for (j=0; j<=MAPPERMASK; j++)
    printf ("0x%08lx: %d\n",(long)j,pixelmap->pixel_in_map[j]);
#endif

  return pixelmap->npixel;
}


/* Search a pixel value in the pixel map. Returns FALSE if the */
/* pixelval was not found in map. Returns TRUE if found. */

static gboolean
get_pixelmap (L_CARD32   pixelval,
              PIXEL_MAP *pixelmap,
              guchar    *red,
              guchar    *green,
              guchar    *blue)

{
  register PMAP *low, *high, *middle;

  if (pixelmap->npixel == 0)
    return FALSE;

  if (!(pixelmap->pixel_in_map[pixelval & MAPPERMASK]))
    return FALSE;

  low =  &(pixelmap->pmap[0]);
  high = &(pixelmap->pmap[pixelmap->npixel-1]);

  /* Do a binary search on the array */
  while (low < high)
    {
      middle = low + ((high - low)/2);
      if (pixelval <= middle->pixel_val)
        high = middle;
      else
        low = middle+1;
    }

  if (pixelval == low->pixel_val)
    {
      *red = low->red; *green = low->green; *blue = low->blue;
      return TRUE;
    }

  return FALSE;
}


static void
set_bw_color_table (gint32 image_ID)
{
  static guchar BWColorMap[2*3] = { 255, 255, 255, 0, 0, 0 };

#ifdef XWD_COL_DEBUG
  printf ("Set GIMP b/w-colortable:\n");
#endif

  gimp_image_set_colormap (image_ID, BWColorMap, 2);
}


/* Initialize an 8-bit colortable from the mask-values of the XWD-header */
static void
init_color_table256 (L_XWDFILEHEADER *xwdhdr,
                     guchar          *ColorMap)

{
  gint i, j, k, cuind;
  gint redshift, greenshift, blueshift;
  gint maxred, maxgreen, maxblue;

  /* Assume: the bit masks for red/green/blue are grouped together
   * Example: redmask = 0xe0, greenmask = 0x1c, bluemask = 0x03
   * We need to know where to place the RGB-values (shifting)
   * and the maximum value for each component.
   */
  redshift = greenshift = blueshift = 0;
  if ((maxred = xwdhdr->l_red_mask) == 0)
    return;

  /* Shift the redmask to the rightmost bit position to get
   * maximum value for red.
   */
  while ((maxred & 1) == 0)
    {
      redshift++;
      maxred >>= 1;
    }

  if ((maxgreen = xwdhdr->l_green_mask) == 0)
    return;

  while ((maxgreen & 1) == 0)
    {
      greenshift++;
      maxgreen >>= 1;
    }

  if ((maxblue = xwdhdr->l_blue_mask) == 0)
    return;

  while ((maxblue & 1) == 0)
    {
      blueshift++;
      maxblue >>= 1;
    }

  memset ((gchar *) ColorMap, 0, 256 * 3);

  for (i = 0; i <= maxred; i++)
    for (j = 0; j <= maxgreen; j++)
      for (k = 0; k <= maxblue; k++)
        {
          cuind = (i << redshift) | (j << greenshift) | (k << blueshift);

          if (cuind < 256)
            {
              ColorMap[cuind*3]   = (i * 255)/maxred;
              ColorMap[cuind*3+1] = (j * 255)/maxgreen;
              ColorMap[cuind*3+2] = (k * 255)/maxblue;
            }
        }
}


static void
set_color_table (gint32           image_ID,
                 L_XWDFILEHEADER *xwdhdr,
                 L_XWDCOLOR      *xwdcolmap)

{
  gint   ncols, i, j;
  guchar ColorMap[256 * 3];

  ncols = xwdhdr->l_colormap_entries;
  if (xwdhdr->l_ncolors < ncols)
    ncols = xwdhdr->l_ncolors;
  if (ncols <= 0)
    return;
  if (ncols > 256)
    ncols = 256;

  /* Initialize color table for all 256 entries from mask-values */
  init_color_table256 (xwdhdr, ColorMap);

  for (j = 0; j < ncols; j++)
    {
      i = xwdcolmap[j].l_pixel;
      if ((i >= 0) && (i < 256))
        {
          ColorMap[i*3] = (xwdcolmap[j].l_red) >> 8;
          ColorMap[i*3+1] = (xwdcolmap[j].l_green) >> 8;
          ColorMap[i*3+2] = (xwdcolmap[j].l_blue) >> 8;
        }
    }

#ifdef XWD_COL_DEBUG
  printf ("Set GIMP colortable:\n");
  for (j = 0; j < 256; j++)
    printf ("%3d: 0x%02x 0x%02x 0x%02x\n", j,
            ColorMap[j*3], ColorMap[j*3+1], ColorMap[j*3+2]);
#endif

  gimp_image_set_colormap (image_ID, ColorMap, 256);
}




/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static gint32
create_new_image (const gchar         *filename,
                  guint                width,
                  guint                height,
                  GimpImageBaseType    type,
                  gint32              *layer_ID,
                  GimpDrawable       **drawable,
                  GimpPixelRgn        *pixel_rgn)
{
  gint32        image_ID;
  GimpImageType gdtype;

  switch (type)
    {
    case GIMP_GRAY:
      gdtype = GIMP_GRAY_IMAGE;
      break;
    case GIMP_INDEXED:
      gdtype = GIMP_INDEXED_IMAGE;
      break;
    case GIMP_RGB:
      gdtype = GIMP_RGB_IMAGE;
      break;
    default:
      g_warning ("Unknown image type");
      return -1;
    }

  image_ID = gimp_image_new (width, height, type);
  gimp_image_set_filename (image_ID, filename);

  *layer_ID = gimp_layer_new (image_ID, "Background", width, height,
                              gdtype, 100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (image_ID, *layer_ID, 0);

  *drawable = gimp_drawable_get (*layer_ID);
  gimp_pixel_rgn_init (pixel_rgn, *drawable, 0, 0, (*drawable)->width,
                       (*drawable)->height, TRUE, FALSE);

  return image_ID;
}


/* Load XWD with pixmap_format 2, pixmap_depth 1, bits_per_pixel 1 */

static gint32
load_xwd_f2_d1_b1 (const gchar     *filename,
                   FILE            *ifp,
                   L_XWDFILEHEADER *xwdhdr,
                   L_XWDCOLOR      *xwdcolmap)
{
  register int     pix8;
  register guchar *dest, *src;
  guchar           c1, c2, c3, c4;
  gint             width, height, linepad, scan_lines, tile_height;
  gint             i, j, ncols;
  gchar           *temp;
  guchar           bit2byte[256 * 8];
  guchar          *data, *scanline;
  gint             err = 0;
  gint32           layer_ID, image_ID;
  GimpPixelRgn     pixel_rgn;
  GimpDrawable    *drawable;

#ifdef XWD_DEBUG
  printf ("load_xwd_f2_d1_b1 (%s)\n", filename);
#endif

  width  = xwdhdr->l_pixmap_width;
  height = xwdhdr->l_pixmap_height;

  image_ID = create_new_image (filename, width, height, GIMP_INDEXED,
                               &layer_ID, &drawable, &pixel_rgn);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width);

  scanline = g_new (guchar, xwdhdr->l_bytes_per_line + 8);

  ncols = xwdhdr->l_colormap_entries;
  if (xwdhdr->l_ncolors < ncols)
    ncols = xwdhdr->l_ncolors;

  if (ncols < 2)
    set_bw_color_table (image_ID);
  else
    set_color_table (image_ID, xwdhdr, xwdcolmap);

  temp = (gchar *) bit2byte;

  /* Get an array for mapping 8 bits in a byte to 8 bytes */
  if (!xwdhdr->l_bitmap_bit_order)
    {
      for (j = 0; j < 256; j++)
        for (i = 0; i < 8; i++)
          *(temp++) = ((j & (1 << i)) != 0);
    }
  else
    {
      for (j = 0; j < 256; j++)
        for (i = 7; i >= 0; i--)
          *(temp++) = ((j & (1 << i)) != 0);
    }

  linepad = xwdhdr->l_bytes_per_line - (xwdhdr->l_pixmap_width+7)/8;
  if (linepad < 0)
    linepad = 0;

  dest = data;
  scan_lines = 0;

  for (i = 0; i < height; i++)
    {
      if (fread (scanline, xwdhdr->l_bytes_per_line, 1, ifp) != 1)
        {
          err = 1;
          break;
        }

      /* Need to check byte order ? */
      if (xwdhdr->l_bitmap_bit_order != xwdhdr->l_byte_order)
        {
          src = scanline;
          switch (xwdhdr->l_bitmap_unit)
            {
            case 16:
              j = xwdhdr->l_bytes_per_line;
              while (j > 0)
                {
                  c1 = src[0]; c2 = src[1];
                  *(src++) = c2; *(src++) = c1;
                  j -= 2;
                }
              break;

            case 32:
              j = xwdhdr->l_bytes_per_line;
              while (j > 0)
                {
                  c1 = src[0]; c2 = src[1]; c3 = src[2]; c4 = src[3];
                  *(src++) = c4; *(src++) = c3; *(src++) = c2; *(src++) = c1;
                  j -= 4;
                }
              break;
            }
        }
      src = scanline;
      j = width;
      while (j >= 8)
        {
          pix8 = *(src++);
          memcpy (dest, bit2byte + pix8*8, 8);
          dest += 8;
          j -= 8;
        }
      if (j > 0)
        {
          pix8 = *(src++);
          memcpy (dest, bit2byte + pix8*8, j);
          dest += j;
        }

      scan_lines++;

      if ((i % 20) == 0)
        gimp_progress_update ((double)(i+1) / (double)height);

      if ((scan_lines == tile_height) || ((i+1) == height))
        {
          gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, i-scan_lines+1,
                                   width, scan_lines);
          scan_lines = 0;
          dest = data;
        }
      if (err) break;
    }

  g_free (data);
  g_free (scanline);

  if (err)
    g_message (_("EOF encountered on reading"));

  gimp_drawable_flush (drawable);

  return err ? -1 : image_ID;
}


/* Load XWD with pixmap_format 2, pixmap_depth 8, bits_per_pixel 8 */

static gint32
load_xwd_f2_d8_b8 (const gchar     *filename,
                   FILE            *ifp,
                   L_XWDFILEHEADER *xwdhdr,
                   L_XWDCOLOR      *xwdcolmap)
{
  gint          width, height, linepad, tile_height, scan_lines;
  gint          i, j, ncols;
  gint          grayscale;
  guchar       *dest, *data;
  gint          err = 0;
  gint32        layer_ID, image_ID;
  GimpPixelRgn  pixel_rgn;
  GimpDrawable *drawable;

#ifdef XWD_DEBUG
  printf ("load_xwd_f2_d8_b8 (%s)\n", filename);
#endif

  width  = xwdhdr->l_pixmap_width;
  height = xwdhdr->l_pixmap_height;

  /* This could also be a grayscale image. Check it */
  grayscale = 0;
  if ((xwdhdr->l_ncolors == 256) && (xwdhdr->l_colormap_entries == 256))
    {
      for (j = 0; j < 256; j++)
        {
          if ((xwdcolmap[j].l_pixel != j)
              || ((xwdcolmap[j].l_red >> 8) != j)
              || ((xwdcolmap[j].l_green >> 8) != j)
              || ((xwdcolmap[j].l_blue >> 8) != j))
            break;
        }

      grayscale = (j == 256);
    }

  image_ID = create_new_image (filename, width, height,
                               grayscale ? GIMP_GRAY : GIMP_INDEXED,
                               &layer_ID, &drawable, &pixel_rgn);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width);

  if (!grayscale)
    {
      ncols = xwdhdr->l_colormap_entries;
      if (xwdhdr->l_ncolors < ncols) ncols = xwdhdr->l_ncolors;
      if (ncols < 2)
        set_bw_color_table (image_ID);
      else
        set_color_table (image_ID, xwdhdr, xwdcolmap);
    }

  linepad = xwdhdr->l_bytes_per_line - xwdhdr->l_pixmap_width;
  if (linepad < 0)
    linepad = 0;

  dest = data;
  scan_lines = 0;

  for (i = 0; i < height; i++)
    {
      if (fread (dest, 1, width, ifp) != width)
        {
          err = 1;
          break;
        }
      dest += width;

      for (j = 0; j < linepad; j++)
        getc (ifp);

      scan_lines++;

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) (i + 1) / (gdouble) height);

      if ((scan_lines == tile_height) || ((i+1) == height))
        {
          gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, i-scan_lines+1,
                                   width, scan_lines);
          scan_lines = 0;
          dest = data;
        }
    }

  g_free (data);

  if (err)
    g_message (_("EOF encountered on reading"));

  gimp_drawable_flush (drawable);

  return err ? -1 : image_ID;
}


/* Load XWD with pixmap_format 2, pixmap_depth up to 16, bits_per_pixel 16 */

static gint32
load_xwd_f2_d16_b16 (const gchar     *filename,
                     FILE            *ifp,
                     L_XWDFILEHEADER *xwdhdr,
                     L_XWDCOLOR      *xwdcolmap)
{
  register guchar *dest, lsbyte_first;
  gint             width, height, linepad, i, j, c0, c1, ncols;
  gint             red, green, blue, redval, greenval, blueval;
  gint             maxred, maxgreen, maxblue;
  gint             tile_height, scan_lines;
  gulong           redmask, greenmask, bluemask;
  guint            redshift, greenshift, blueshift;
  gulong           maxval;
  guchar          *ColorMap, *cm, *data;
  gint             err = 0;
  gint32           layer_ID, image_ID;
  GimpPixelRgn     pixel_rgn;
  GimpDrawable    *drawable;

#ifdef XWD_DEBUG
  printf ("load_xwd_f2_d16_b16 (%s)\n", filename);
#endif

  width  = xwdhdr->l_pixmap_width;
  height = xwdhdr->l_pixmap_height;

  image_ID = create_new_image (filename, width, height, GIMP_RGB,
                               &layer_ID, &drawable, &pixel_rgn);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width * 3);

  /* Get memory for mapping 16 bit XWD-pixel to GIMP-RGB */
  maxval = 0x10000 * 3;
  ColorMap = g_new0 (guchar, maxval);

  redmask   = xwdhdr->l_red_mask;
  greenmask = xwdhdr->l_green_mask;
  bluemask  = xwdhdr->l_blue_mask;

  /* How to shift RGB to be right aligned ? */
  /* (We rely on the the mask bits are grouped and not mixed) */
  redshift = greenshift = blueshift = 0;

  while (((1 << redshift)   & redmask)   == 0) redshift++;
  while (((1 << greenshift) & greenmask) == 0) greenshift++;
  while (((1 << blueshift)  & bluemask)  == 0) blueshift++;

  /* The bits_per_rgb may not be correct. Use redmask instead */
  maxred = 0; while (redmask >> (redshift + maxred)) maxred++;
  maxred = (1 << maxred) - 1;

  maxgreen = 0; while (greenmask >> (greenshift + maxgreen)) maxgreen++;
  maxgreen = (1 << maxgreen) - 1;

  maxblue = 0; while (bluemask >> (blueshift + maxblue)) maxblue++;
  maxblue = (1 << maxblue) - 1;

  /* Built up the array to map XWD-pixel value to GIMP-RGB */
  for (red = 0; red <= maxred; red++)
    {
      redval = (red * 255) / maxred;
      for (green = 0; green <= maxgreen; green++)
        {
          greenval = (green * 255) / maxgreen;
          for (blue = 0; blue <= maxblue; blue++)
            {
              blueval = (blue * 255) / maxblue;
              cm = ColorMap + ((red << redshift) + (green << greenshift)
                               + (blue << blueshift)) * 3;
              *(cm++) = redval;
              *(cm++) = greenval;
              *cm = blueval;
            }
        }
    }

  /* Now look what was written to the XWD-Colormap */

  ncols = xwdhdr->l_colormap_entries;
  if (xwdhdr->l_ncolors < ncols)
    ncols = xwdhdr->l_ncolors;

  for (j = 0; j < ncols; j++)
    {
      cm = ColorMap + xwdcolmap[j].l_pixel * 3;
      *(cm++) = (xwdcolmap[j].l_red >> 8);
      *(cm++) = (xwdcolmap[j].l_green >> 8);
      *cm = (xwdcolmap[j].l_blue >> 8);
    }

  /* What do we have to consume after a line has finished ? */
  linepad =   xwdhdr->l_bytes_per_line
    - (xwdhdr->l_pixmap_width*xwdhdr->l_bits_per_pixel)/8;
  if (linepad < 0) linepad = 0;

  lsbyte_first = (xwdhdr->l_byte_order == 0);

  dest = data;
  scan_lines = 0;

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
        {
          c0 = getc (ifp);
          c1 = getc (ifp);
          if (c1 < 0)
            {
              err = 1;
              break;
            }

          if (lsbyte_first)
            c0 = c0 | (c1 << 8);
          else
            c0 = (c0 << 8) | c1;

          cm = ColorMap + c0 * 3;
          *(dest++) = *(cm++);
          *(dest++) = *(cm++);
          *(dest++) = *cm;
        }

      if (err)
        break;

      for (j = 0; j < linepad; j++)
        getc (ifp);

      scan_lines++;

      if ((i % 20) == 0)
        gimp_progress_update ((double)(i+1) / (double)height);

      if ((scan_lines == tile_height) || ((i+1) == height))
        {
          gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, i-scan_lines+1,
                                   width, scan_lines);
          scan_lines = 0;
          dest = data;
        }
    }
  g_free (data);
  g_free (ColorMap);

  if (err)
    g_message (_("EOF encountered on reading"));

  gimp_drawable_flush (drawable);

  return err ? -1 : image_ID;
}


/* Load XWD with pixmap_format 2, pixmap_depth up to 24, bits_per_pixel 24/32 */

static gint32
load_xwd_f2_d24_b32 (const gchar     *filename,
                     FILE            *ifp,
                     L_XWDFILEHEADER *xwdhdr,
                     L_XWDCOLOR      *xwdcolmap)
{
  register guchar *dest, lsbyte_first;
  gint             width, height, linepad, i, j, c0, c1, c2, c3;
  gint             tile_height, scan_lines;
  L_CARD32         pixelval;
  gint             red, green, blue, ncols;
  gint             maxred, maxgreen, maxblue;
  gulong           redmask, greenmask, bluemask;
  guint            redshift, greenshift, blueshift;
  guchar           redmap[256], greenmap[256], bluemap[256];
  guchar          *data;
  PIXEL_MAP        pixel_map;
  gint             err = 0;
  gint32           layer_ID, image_ID;
  GimpPixelRgn     pixel_rgn;
  GimpDrawable    *drawable;

#ifdef XWD_DEBUG
  printf ("load_xwd_f2_d24_b32 (%s)\n", filename);
#endif

  width  = xwdhdr->l_pixmap_width;
  height = xwdhdr->l_pixmap_height;

  image_ID = create_new_image (filename, width, height, GIMP_RGB,
                               &layer_ID, &drawable, &pixel_rgn);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width * 3);

  redmask   = xwdhdr->l_red_mask;
  greenmask = xwdhdr->l_green_mask;
  bluemask  = xwdhdr->l_blue_mask;

  if (redmask   == 0) redmask   = 0xff0000;
  if (greenmask == 0) greenmask = 0x00ff00;
  if (bluemask  == 0) bluemask  = 0x0000ff;

  /* How to shift RGB to be right aligned ? */
  /* (We rely on the the mask bits are grouped and not mixed) */
  redshift = greenshift = blueshift = 0;

  while (((1 << redshift)   & redmask)   == 0) redshift++;
  while (((1 << greenshift) & greenmask) == 0) greenshift++;
  while (((1 << blueshift)  & bluemask)  == 0) blueshift++;

  /* The bits_per_rgb may not be correct. Use redmask instead */

  maxred = 0; while (redmask >> (redshift + maxred)) maxred++;
  maxred = (1 << maxred) - 1;

  maxgreen = 0; while (greenmask >> (greenshift + maxgreen)) maxgreen++;
  maxgreen = (1 << maxgreen) - 1;

  maxblue = 0; while (bluemask >> (blueshift + maxblue)) maxblue++;
  maxblue = (1 << maxblue) - 1;

  /* Set map-arrays for red, green, blue */
  for (red = 0; red <= maxred; red++)
    redmap[red] = (red * 255) / maxred;
  for (green = 0; green <= maxgreen; green++)
    greenmap[green] = (green * 255) / maxgreen;
  for (blue = 0; blue <= maxblue; blue++)
    bluemap[blue] = (blue * 255) / maxblue;

  ncols = xwdhdr->l_colormap_entries;
  if (xwdhdr->l_ncolors < ncols) ncols = xwdhdr->l_ncolors;

  ncols = set_pixelmap (ncols, xwdcolmap, &pixel_map);

  /* What do we have to consume after a line has finished ? */
  linepad =   xwdhdr->l_bytes_per_line
    - (xwdhdr->l_pixmap_width*xwdhdr->l_bits_per_pixel)/8;
  if (linepad < 0) linepad = 0;

  lsbyte_first = (xwdhdr->l_byte_order == 0);

  dest = data;
  scan_lines = 0;

  if (xwdhdr->l_bits_per_pixel == 32)
    {
      for (i = 0; i < height; i++)
        {
          for (j = 0; j < width; j++)
            {
              c0 = getc (ifp);
              c1 = getc (ifp);
              c2 = getc (ifp);
              c3 = getc (ifp);
              if (c3 < 0)
                {
                  err = 1;
                  break;
                }
              if (lsbyte_first)
                pixelval = c0 | (c1 << 8) | (c2 << 16) | (c3 << 24);
              else
                pixelval = (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;

              if (get_pixelmap (pixelval, &pixel_map, dest, dest+1, dest+2))
                {
                  dest += 3;
                }
              else
                {
                  *(dest++) = redmap[(pixelval & redmask) >> redshift];
                  *(dest++) = greenmap[(pixelval & greenmask) >> greenshift];
                  *(dest++) = bluemap[(pixelval & bluemask) >> blueshift];
                }
            }
          scan_lines++;

          if (err)
            break;

          for (j = 0; j < linepad; j++)
            getc (ifp);

          if ((i % 20) == 0)
            gimp_progress_update ((gdouble) (i + 1) / (gdouble) height);

          if ((scan_lines == tile_height) || ((i+1) == height))
            {
              gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, i-scan_lines+1,
                                       width, scan_lines);
              scan_lines = 0;
              dest = data;
            }
        }
    }
  else    /* 24 bits per pixel */
    {
      for (i = 0; i < height; i++)
        {
          for (j = 0; j < width; j++)
            {
              c0 = getc (ifp);
              c1 = getc (ifp);
              c2 = getc (ifp);
              if (c2 < 0)
                {
                  err = 1;
                  break;
                }
              if (lsbyte_first)
                pixelval = c0 | (c1 << 8) | (c2 << 16);
              else
                pixelval = (c0 << 16) | (c1 << 8) | c2;

              if (get_pixelmap (pixelval, &pixel_map, dest, dest+1, dest+2))
                {
                  dest += 3;
                }
              else
                {
                  *(dest++) = redmap[(pixelval & redmask) >> redshift];
                  *(dest++) = greenmap[(pixelval & greenmask) >> greenshift];
                  *(dest++) = bluemap[(pixelval & bluemask) >> blueshift];
                }
            }
          scan_lines++;

          if (err)
            break;

          for (j = 0; j < linepad; j++)
            getc (ifp);

          if ((i % 20) == 0)
            gimp_progress_update ((gdouble) (i + 1) / (gdouble) height);

          if ((scan_lines == tile_height) || ((i+1) == height))
            {
              gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, i-scan_lines+1,
                                       width, scan_lines);
              scan_lines = 0;
              dest = data;
            }
        }
    }

  g_free (data);

  if (err)
    g_message (_("EOF encountered on reading"));

  gimp_drawable_flush (drawable);

  return err ? -1 : image_ID;
}


/* Load XWD with pixmap_format 1, pixmap_depth up to 24, bits_per_pixel 1 */

static gint32
load_xwd_f1_d24_b1 (const gchar     *filename,
                    FILE            *ifp,
                    L_XWDFILEHEADER *xwdhdr,
                    L_XWDCOLOR      *xwdcolmap)
{
  register guchar *dest, outmask, inmask, do_reverse;
  gint             width, height, linepad, i, j, plane, fromright;
  gint             tile_height, tile_start, tile_end;
  gint             indexed, bytes_per_pixel;
  gint             maxred, maxgreen, maxblue;
  gint             red, green, blue, ncols, standard_rgb;
  glong            data_offset, plane_offset, tile_offset;
  gulong           redmask, greenmask, bluemask;
  guint            redshift, greenshift, blueshift;
  gulong           g;
  guchar           redmap[256], greenmap[256], bluemap[256];
  guchar           bit_reverse[256];
  guchar          *xwddata, *xwdin, *data;
  L_CARD32         pixelval;
  PIXEL_MAP        pixel_map;
  gint             err = 0;
  gint32           layer_ID, image_ID;
  GimpPixelRgn     pixel_rgn;
  GimpDrawable    *drawable;

#ifdef XWD_DEBUG
  printf ("load_xwd_f1_d24_b1 (%s)\n", filename);
#endif

  xwddata = g_malloc (xwdhdr->l_bytes_per_line);
  if (xwddata == NULL)
    return -1;

  width           = xwdhdr->l_pixmap_width;
  height          = xwdhdr->l_pixmap_height;
  indexed         = (xwdhdr->l_pixmap_depth <= 8);
  bytes_per_pixel = (indexed ? 1 : 3);

  image_ID = create_new_image (filename, width, height,
                               indexed ? GIMP_INDEXED : GIMP_RGB,
                               &layer_ID, &drawable, &pixel_rgn);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width * bytes_per_pixel);

  linepad =   xwdhdr->l_bytes_per_line
    - (xwdhdr->l_pixmap_width+7)/8;
  if (linepad < 0)
    linepad = 0;

  for (j = 0; j < 256; j++)   /* Create an array for reversing bits */
    {
      inmask = 0;
      for (i = 0; i < 8; i++)
        {
          inmask <<= 1;
          if (j & (1 << i)) inmask |= 1;
        }
      bit_reverse[j] = inmask;
    }

  redmask   = xwdhdr->l_red_mask;
  greenmask = xwdhdr->l_green_mask;
  bluemask  = xwdhdr->l_blue_mask;

  if (redmask   == 0) redmask   = 0xff0000;
  if (greenmask == 0) greenmask = 0x00ff00;
  if (bluemask  == 0) bluemask  = 0x0000ff;

  standard_rgb =    (redmask == 0xff0000) && (greenmask == 0x00ff00)
    && (bluemask == 0x0000ff);
  redshift = greenshift = blueshift = 0;

  if (!standard_rgb)   /* Do we need to re-map the pixel-values ? */
    {
      /* How to shift RGB to be right aligned ? */
      /* (We rely on the the mask bits are grouped and not mixed) */

      while (((1 << redshift)   & redmask)   == 0) redshift++;
      while (((1 << greenshift) & greenmask) == 0) greenshift++;
      while (((1 << blueshift)  & bluemask)  == 0) blueshift++;

      /* The bits_per_rgb may not be correct. Use redmask instead */

      maxred = 0; while (redmask >> (redshift + maxred)) maxred++;
      maxred = (1 << maxred) - 1;

      maxgreen = 0; while (greenmask >> (greenshift + maxgreen)) maxgreen++;
      maxgreen = (1 << maxgreen) - 1;

      maxblue = 0; while (bluemask >> (blueshift + maxblue)) maxblue++;
      maxblue = (1 << maxblue) - 1;

      /* Set map-arrays for red, green, blue */
      for (red = 0; red <= maxred; red++)
        redmap[red] = (red * 255) / maxred;
      for (green = 0; green <= maxgreen; green++)
        greenmap[green] = (green * 255) / maxgreen;
      for (blue = 0; blue <= maxblue; blue++)
        bluemap[blue] = (blue * 255) / maxblue;
    }

  ncols = xwdhdr->l_colormap_entries;
  if (xwdhdr->l_ncolors < ncols)
    ncols = xwdhdr->l_ncolors;

  if (indexed)
    {
      if (ncols < 2)
        set_bw_color_table (image_ID);
      else
        set_color_table (image_ID, xwdhdr, xwdcolmap);
    }
  else
    {
      ncols = set_pixelmap (ncols, xwdcolmap, &pixel_map);
    }

  do_reverse = !xwdhdr->l_bitmap_bit_order;

  /* This is where the image data starts within the file */
  data_offset = ftell (ifp);

  for (tile_start = 0; tile_start < height; tile_start += tile_height)
    {
      memset (data, 0, width*tile_height*bytes_per_pixel);

      tile_end = tile_start + tile_height - 1;
      if (tile_end >= height)
        tile_end = height - 1;

      for (plane = 0; plane < xwdhdr->l_pixmap_depth; plane++)
        {
          dest = data;    /* Position to start of tile within the plane */
          plane_offset = data_offset + plane*height*xwdhdr->l_bytes_per_line;
          tile_offset = plane_offset + tile_start*xwdhdr->l_bytes_per_line;
          fseek (ifp, tile_offset, SEEK_SET);

          /* Place the last plane at the least significant bit */

          if (indexed)   /* Only 1 byte per pixel */
            {
              fromright = xwdhdr->l_pixmap_depth-1-plane;
              outmask = (1 << fromright);
            }
          else           /* 3 bytes per pixel */
            {
              fromright = xwdhdr->l_pixmap_depth-1-plane;
              dest += 2 - fromright/8;
              outmask = (1 << (fromright % 8));
            }

          for (i = tile_start; i <= tile_end; i++)
            {
              if (fread (xwddata,xwdhdr->l_bytes_per_line,1,ifp) != 1)
                {
                  err = 1;
                  break;
                }
              xwdin = xwddata;

              /* Handle bitmap unit */
              if (xwdhdr->l_bitmap_unit == 16)
                {
                  if (xwdhdr->l_bitmap_bit_order != xwdhdr->l_byte_order)
                    {
                      j = xwdhdr->l_bytes_per_line/2;
                      while (j--)
                        {
                          inmask = xwdin[0]; xwdin[0] = xwdin[1]; xwdin[1] = inmask;
                          xwdin += 2;
                        }
                      xwdin = xwddata;
                    }
                }
              else if (xwdhdr->l_bitmap_unit == 32)
                {
                  if (xwdhdr->l_bitmap_bit_order != xwdhdr->l_byte_order)
                    {
                      j = xwdhdr->l_bytes_per_line/4;
                      while (j--)
                        {
                          inmask = xwdin[0]; xwdin[0] = xwdin[3]; xwdin[3] = inmask;
                          inmask = xwdin[1]; xwdin[1] = xwdin[2]; xwdin[2] = inmask;
                          xwdin += 4;
                        }
                      xwdin = xwddata;
                    }
                }

              g = inmask = 0;
              for (j = 0; j < width; j++)
                {
                  if (!inmask)
                    {
                      g = *(xwdin++);
                      if (do_reverse)
                        g = bit_reverse[g];
                      inmask = 0x80;
                    }

                  if (g & inmask)
                    *dest |= outmask;
                  dest += bytes_per_pixel;

                  inmask >>= 1;
                }
            }
        }

      /* For indexed images, the mapping to colors is done by the color table. */
      /* Otherwise we must do the mapping by ourself. */
      if (!indexed)
        {
          dest = data;
          for (i = tile_start; i <= tile_end; i++)
            {
              for (j = 0; j < width; j++)
                {
                  pixelval = (*dest << 16) | (*(dest+1) << 8) | *(dest+2);

                  if (get_pixelmap (pixelval, &pixel_map, dest, dest+1, dest+2)
                      || standard_rgb)
                    {
                      dest += 3;
                    }
                  else   /* We have to map RGB to 0,...,255 */
                    {
                      *(dest++) = redmap[(pixelval & redmask) >> redshift];
                      *(dest++) = greenmap[(pixelval & greenmask) >> greenshift];
                      *(dest++) = bluemap[(pixelval & bluemask) >> blueshift];
                    }
                }
            }
        }

      gimp_progress_update ((gdouble) tile_end / (gdouble) height);

      gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, tile_start,
                               width, tile_end-tile_start+1);
    }

  g_free (data);
  g_free (xwddata);

  if (err)
    g_message (_("EOF encountered on reading"));

  gimp_drawable_flush (drawable);

  return err ? -1 : image_ID;
}


static gint
save_index (FILE    *ofp,
	    gint32   image_ID,
	    gint32   drawable_ID,
	    gint     gray)
{
  gint             height, width, linepad, tile_height, i, j;
  gint             ncolors, vclass;
  glong            tmp = 0;
  guchar          *data, *src, *cmap;
  L_XWDFILEHEADER  xwdhdr;
  L_XWDCOLOR       xwdcolmap[256];
  GimpPixelRgn     pixel_rgn;
  GimpDrawable    *drawable;
  GimpImageType    drawable_type;

#ifdef XWD_DEBUG
  printf ("save_index ()\n");
#endif

  drawable      = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width         = drawable->width;
  height        = drawable->height;
  tile_height   = gimp_tile_height ();

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = g_new (guchar, tile_height * width * drawable->bpp);

  linepad = width % 4;
  if (linepad) linepad = 4 - linepad;

  /* Fill XWD-color map */
  if (gray)
    {
      vclass = 0;
      ncolors = 256;

      for (j = 0; j < ncolors; j++)
	{
	  xwdcolmap[j].l_pixel = j;
	  xwdcolmap[j].l_red   = (j << 8) | j;
	  xwdcolmap[j].l_green = (j << 8) | j;
	  xwdcolmap[j].l_blue  = (j << 8) | j;
	  xwdcolmap[j].l_flags = 7;
	  xwdcolmap[j].l_pad = 0;
	}
    }
  else
    {
      vclass = 3;
      cmap = gimp_image_get_colormap (image_ID, &ncolors);

      for (j = 0; j < ncolors; j++)
	{
	  xwdcolmap[j].l_pixel = j;
	  xwdcolmap[j].l_red   = ((*cmap) << 8) | *cmap; cmap++;
	  xwdcolmap[j].l_green = ((*cmap) << 8) | *cmap; cmap++;
	  xwdcolmap[j].l_blue  = ((*cmap) << 8) | *cmap; cmap++;
	  xwdcolmap[j].l_flags = 7;
	  xwdcolmap[j].l_pad = 0;
	}
    }

  /* Fill in the XWD header (header_size is evaluated by write_xwd_hdr ()) */
  xwdhdr.l_header_size      = 0;
  xwdhdr.l_file_version     = 7;
  xwdhdr.l_pixmap_format    = 2;
  xwdhdr.l_pixmap_depth     = 8;
  xwdhdr.l_pixmap_width     = width;
  xwdhdr.l_pixmap_height    = height;
  xwdhdr.l_xoffset          = 0;
  xwdhdr.l_byte_order       = 1;
  xwdhdr.l_bitmap_unit      = 32;
  xwdhdr.l_bitmap_bit_order = 1;
  xwdhdr.l_bitmap_pad       = 32;
  xwdhdr.l_bits_per_pixel   = 8;
  xwdhdr.l_bytes_per_line   = width + linepad;
  xwdhdr.l_visual_class     = vclass;
  xwdhdr.l_red_mask         = 0x000000;
  xwdhdr.l_green_mask       = 0x000000;
  xwdhdr.l_blue_mask        = 0x000000;
  xwdhdr.l_bits_per_rgb     = 8;
  xwdhdr.l_colormap_entries = ncolors;
  xwdhdr.l_ncolors          = ncolors;
  xwdhdr.l_window_width     = width;
  xwdhdr.l_window_height    = height;
  xwdhdr.l_window_x         = 64;
  xwdhdr.l_window_y         = 64;
  xwdhdr.l_window_bdrwidth  = 0;

  write_xwd_header (ofp, &xwdhdr);
  write_xwd_cols (ofp, &xwdhdr, xwdcolmap);

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)   /* Get more data */
	{
          gint scan_lines = (i + tile_height - 1 < height) ? tile_height : (height - i);

          gimp_pixel_rgn_get_rect (&pixel_rgn, data, 0, i, width, scan_lines);
          src = data;
	}

      fwrite (src, width, 1, ofp);

      if (linepad)
        fwrite ((char *)&tmp, linepad, 1, ofp);

      src += width;

      if ((i % 20) == 0)
	gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp))
    {
      g_message (_("Error during writing indexed/gray image"));
      return FALSE;
    }

  return TRUE;
}


static gint
save_rgb (FILE   *ofp,
          gint32  image_ID,
          gint32  drawable_ID)
{
  gint             height, width, linepad, tile_height, i;
  glong            tmp = 0;
  guchar          *data, *src;
  L_XWDFILEHEADER  xwdhdr;
  GimpPixelRgn     pixel_rgn;
  GimpDrawable    *drawable;
  GimpImageType    drawable_type;

#ifdef XWD_DEBUG
  printf ("save_rgb ()\n");
#endif

  drawable      = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  width         = drawable->width;
  height        = drawable->height;
  tile_height   = gimp_tile_height ();

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = g_new (guchar, tile_height * width * drawable->bpp);

  linepad = (width * 3) % 4;
  if (linepad)
    linepad = 4 - linepad;

  /* Fill in the XWD header (header_size is evaluated by write_xwd_hdr ()) */
  xwdhdr.l_header_size      = 0;
  xwdhdr.l_file_version     = 7;
  xwdhdr.l_pixmap_format    = 2;
  xwdhdr.l_pixmap_depth     = 24;
  xwdhdr.l_pixmap_width     = width;
  xwdhdr.l_pixmap_height    = height;
  xwdhdr.l_xoffset          = 0;
  xwdhdr.l_byte_order       = 1;

  xwdhdr.l_bitmap_unit      = 32;
  xwdhdr.l_bitmap_bit_order = 1;
  xwdhdr.l_bitmap_pad       = 32;
  xwdhdr.l_bits_per_pixel   = 24;

  xwdhdr.l_bytes_per_line   = width * 3 + linepad;
  xwdhdr.l_visual_class     = 5;
  xwdhdr.l_red_mask         = 0xff0000;
  xwdhdr.l_green_mask       = 0x00ff00;
  xwdhdr.l_blue_mask        = 0x0000ff;
  xwdhdr.l_bits_per_rgb     = 8;
  xwdhdr.l_colormap_entries = 0;
  xwdhdr.l_ncolors          = 0;
  xwdhdr.l_window_width     = width;
  xwdhdr.l_window_height    = height;
  xwdhdr.l_window_x         = 64;
  xwdhdr.l_window_y         = 64;
  xwdhdr.l_window_bdrwidth  = 0;

  write_xwd_header (ofp, &xwdhdr);

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)   /* Get more data */
	{
          gint scan_lines = (i + tile_height - 1 < height) ? tile_height : (height - i);

          gimp_pixel_rgn_get_rect (&pixel_rgn, data, 0, i, width, scan_lines);
          src = data;
	}

      fwrite (src, width * 3, 1, ofp);

      if (linepad)
        fwrite ((char *)&tmp, linepad, 1, ofp);

      src += width * 3;

      if ((i % 20) == 0)
	gimp_progress_update ((gdouble) i / (gdouble) height);
    }

  g_free (data);

  gimp_drawable_detach (drawable);

  if (ferror (ofp))
    {
      g_message (_("Error during writing rgb image"));
      return FALSE;
    }

  return TRUE;
}
