/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * XWD reading and writing code Copyright (C) 1996 Peter Kirchgessner
 * (email: peter@kirchgessner.net, WWW: http://www.kirchgessner.net)
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
 *          2       |    1,...,32  |      32
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
#define EXPORT_PROC    "file-xwd-export"
#define PLUG_IN_BINARY "file-xwd"
#define PLUG_IN_ROLE   "gimp-file-xwd"


typedef guint32 L_CARD32;
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


typedef struct _Xwd      Xwd;
typedef struct _XwdClass XwdClass;

struct _Xwd
{
  GimpPlugIn      parent_instance;
};

struct _XwdClass
{
  GimpPlugInClass parent_class;
};


#define XWD_TYPE  (xwd_get_type ())
#define XWD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), XWD_TYPE, Xwd))

GType                   xwd_get_type         (void) G_GNUC_CONST;

static GList          * xwd_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * xwd_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static GimpValueArray * xwd_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);
static GimpValueArray * xwd_export           (GimpProcedure         *procedure,
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
                                              GimpDrawable          *drawable,
                                              GError               **error);
static GimpImage      * create_new_image     (GFile                 *file,
                                              guint                  width,
                                              guint                  height,
                                              GimpImageBaseType      type,
                                              GimpImageType          gdtype,
                                              GimpLayer            **layer,
                                              GeglBuffer           **buffer);

static int              set_pixelmap         (gint                   ncols,
                                              L_XWDCOLOR            *xwdcol,
                                              PIXEL_MAP             *pixelmap);
static gboolean         get_pixelmap         (L_CARD32               pixelval,
                                              PIXEL_MAP             *pixelmap,
                                              guchar                *red,
                                              guchar                *green,
                                              guchar                *glue);

static void             set_bw_color_table   (GimpImage             *image);
static void             set_color_table      (GimpImage             *image,
                                              L_XWDFILEHEADER       *xwdhdr,
                                              L_XWDCOLOR            *xwdcolmap);

static GimpImage      * load_xwd_f2_d1_b1    (GFile                 *file,
                                              FILE                  *ifp,
                                              L_XWDFILEHEADER       *xwdhdr,
                                              L_XWDCOLOR            *xwdcolmap,
                                              GError               **error);
static GimpImage      * load_xwd_f2_d8_b8    (GFile                 *file,
                                              FILE                  *ifp,
                                              L_XWDFILEHEADER       *xwdhdr,
                                              L_XWDCOLOR            *xwdcolmap,
                                              GError               **error);
static GimpImage      * load_xwd_f2_d16_b16  (GFile                 *file,
                                              FILE                  *ifp,
                                              L_XWDFILEHEADER       *xwdhdr,
                                              L_XWDCOLOR            *xwdcolmap,
                                              GError               **error);
static GimpImage      * load_xwd_f2_d24_b32  (GFile                 *file,
                                              FILE                  *ifp,
                                              L_XWDFILEHEADER       *xwdhdr,
                                              L_XWDCOLOR            *xwdcolmap,
                                              GError               **error);
static GimpImage      * load_xwd_f2_d32_b32  (GFile                 *file,
                                              FILE                  *ifp,
                                              L_XWDFILEHEADER       *xwdhdr,
                                              L_XWDCOLOR            *xwdcolmap,
                                              GError               **error);
static GimpImage      * load_xwd_f1_d24_b1   (GFile                 *file,
                                              FILE                  *ifp,
                                              L_XWDFILEHEADER       *xwdhdr,
                                              L_XWDCOLOR            *xwdcolmap,
                                              GError               **error);

static L_CARD32         read_card32          (FILE                  *ifp,
                                              gint                  *err);
static L_CARD16         read_card16          (FILE                  *ifp,
                                              gint                  *err);
static L_CARD8          read_card8           (FILE                  *ifp,
                                              gint                  *err);

static gboolean         write_card32         (GOutputStream         *output,
                                              L_CARD32               c,
                                              GError               **error);
static gboolean         write_card16         (GOutputStream         *output,
                                              L_CARD32               c,
                                              GError               **error);
static gboolean         write_card8          (GOutputStream         *output,
                                              L_CARD32               c,
                                              GError               **error);

static void             read_xwd_header      (FILE                  *ifp,
                                              L_XWDFILEHEADER       *xwdhdr);

static gboolean         write_xwd_header     (GOutputStream         *output,
                                              L_XWDFILEHEADER       *xwdhdr,
                                              GError               **error);

static void             read_xwd_cols        (FILE                  *ifp,
                                              L_XWDFILEHEADER       *xwdhdr,
                                              L_XWDCOLOR            *xwdcolmap,
                                              GError               **error);

static gboolean         write_xwd_cols       (GOutputStream         *output,
                                              L_XWDFILEHEADER       *xwdhdr,
                                              L_XWDCOLOR            *colormap,
                                              GError               **error);

static gint             save_index           (GOutputStream         *output,
                                              GimpImage             *image,
                                              GimpDrawable          *drawable,
                                              gboolean               gray,
                                              GError               **error);
static gint             save_rgb             (GOutputStream         *output,
                                              GimpImage             *image,
                                              GimpDrawable          *drawable,
                                              GError               **error);


G_DEFINE_TYPE (Xwd, xwd, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (XWD_TYPE)
DEFINE_STD_SET_I18N


static void
xwd_class_init (XwdClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = xwd_query_procedures;
  plug_in_class->create_procedure = xwd_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
xwd_init (Xwd *xwd)
{
}

static GList *
xwd_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
xwd_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           xwd_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("X window dump"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in the XWD (X Window Dump) "
                                        "format",
                                        "Loads files in the XWD (X Window Dump) "
                                        "format. XWD image files are produced "
                                        "by the program xwd. Xwd is an X Window "
                                        "System window dumping utility.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner",
                                      "1996");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-xwindowdump");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "xwd");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "4,long,0x00000007");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, xwd_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB, GRAY, INDEXED");

      gimp_procedure_set_menu_label (procedure, _("X window dump"));

      gimp_procedure_set_documentation (procedure,
                                        "Exports files in the XWD (X Window "
                                        "Dump) format",
                                        "XWD exporting handles all image "
                                        "types except those with alpha channels.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner",
                                      "1996");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-xwindowdump");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "xwd");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB  |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED,
                                              NULL, NULL, NULL);
    }

  return procedure;
}

static GimpValueArray *
xwd_load (GimpProcedure         *procedure,
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
xwd_export (GimpProcedure        *procedure,
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
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  export    = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);

  if (! export_image (file, image, drawables->data, &error))
    status = GIMP_PDB_EXECUTION_ERROR;

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpImage *
load_image (GFile   *file,
            GError **error)
{
  FILE            *ifp;
  gint             depth, bpp;
  GimpImage       *image = NULL;
  L_XWDFILEHEADER  xwdhdr;
  L_XWDCOLOR      *xwdcolmap = NULL;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  ifp = g_fopen (g_file_peek_path (file), "rb");

  if (! ifp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      goto out;
    }

  read_xwd_header (ifp, &xwdhdr);

#ifdef XWD_DEBUG
  /* Write info about header */
  g_printerr ("XWD header:\n\t"
              "Header size: %u, filer version: %u, image type: %u\n\t"
              "Depth: %u, Width: %u, Height: %u\n\t"
              "X-offset: %u, byte order: %u, bitmap unit: %u, bit order: %u\n\t"
              "bitmap pad: %u, bits per pixel: %u, bytes per line: %u\n\t"
              "visual class: %u, Masks: red %x, green %x, blue %x, bits per rgb: %u\n\t"
              "Number of colors: %u, color map entries: %u\n\t"
              "Window width: %u, height: %u, x: %u, y: %u, border width: %u"
              "\n", xwdhdr.l_header_size, xwdhdr.l_file_version, xwdhdr.l_pixmap_format,
              xwdhdr.l_pixmap_depth, xwdhdr.l_pixmap_width, xwdhdr.l_pixmap_height,
              xwdhdr.l_xoffset, xwdhdr.l_byte_order, xwdhdr.l_bitmap_unit,
              xwdhdr.l_bitmap_bit_order, xwdhdr.l_bitmap_pad, xwdhdr.l_bits_per_pixel,
              xwdhdr.l_bytes_per_line, xwdhdr.l_visual_class,
              xwdhdr.l_red_mask, xwdhdr.l_green_mask, xwdhdr.l_blue_mask,
              xwdhdr.l_bits_per_rgb, xwdhdr.l_ncolors, xwdhdr.l_colormap_entries,
              xwdhdr.l_window_width, xwdhdr.l_window_height,
              xwdhdr.l_window_x, xwdhdr.l_window_y, xwdhdr.l_window_bdrwidth);
#endif

  if (xwdhdr.l_file_version != 7)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read XWD header from '%s'"),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

#ifdef XWD_COL_WAIT_DEBUG
  {
    int k = 1;

    while (k)
      k = k;
  }
#endif

  /* Position to start of XWDColor structures */
  if (fseek (ifp, (long)xwdhdr.l_header_size, SEEK_SET) != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s':\nSeek error"),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  /* Guard against insanely huge color maps -- gimp_image_set_colormap() only
   * accepts colormaps with 0..256 colors anyway. */
  if (xwdhdr.l_colormap_entries > 256)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s':\nIllegal number of colormap entries: %u"),
                   gimp_file_get_utf8_name (file),
                   xwdhdr.l_colormap_entries);
      goto out;
    }

  if (xwdhdr.l_colormap_entries > 0)
    {
      if (xwdhdr.l_colormap_entries < xwdhdr.l_ncolors)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("'%s':\nNumber of colormap entries < number of colors"),
                       gimp_file_get_utf8_name (file));
          goto out;
        }

      xwdcolmap = g_new (L_XWDCOLOR, xwdhdr.l_colormap_entries);

      read_xwd_cols (ifp, &xwdhdr, xwdcolmap, error);
      if (error && *error)
        goto out;

#ifdef XWD_COL_DEBUG
      {
        int j;
        g_printf ("File %s\n", g_file_peek_path (file));
        for (j = 0; j < xwdhdr.l_colormap_entries; j++)
          g_printf ("Entry 0x%08lx: 0x%04lx,  0x%04lx, 0x%04lx, %d\n",
                    (long)xwdcolmap[j].l_pixel,(long)xwdcolmap[j].l_red,
                    (long)xwdcolmap[j].l_green,(long)xwdcolmap[j].l_blue,
                    (int)xwdcolmap[j].l_flags);
      }
#endif

      if (xwdhdr.l_file_version != 7)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Can't read color entries"));
          goto out;
        }
    }

  if (xwdhdr.l_pixmap_width <= 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s':\nNo image width specified"),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  if (xwdhdr.l_pixmap_width > GIMP_MAX_IMAGE_SIZE
      || xwdhdr.l_bytes_per_line > GIMP_MAX_IMAGE_SIZE * 3)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s':\nImage width is larger than GIMP can handle"),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  if (xwdhdr.l_pixmap_height <= 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s':\nNo image height specified"),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  if (xwdhdr.l_pixmap_height > GIMP_MAX_IMAGE_SIZE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s':\nImage height is larger than GIMP can handle"),
                   gimp_file_get_utf8_name (file));
      goto out;
    }

  depth = xwdhdr.l_pixmap_depth;
  bpp   = xwdhdr.l_bits_per_pixel;

  switch (xwdhdr.l_pixmap_format)
    {
    case 0:    /* Single plane bitmap */
      if ((depth == 1) && (bpp == 1))
        { /* Can be performed by format 2 loader */
          image = load_xwd_f2_d1_b1 (file, ifp, &xwdhdr, xwdcolmap, error);
        }
      break;

    case 1:    /* Single plane pixmap */
      if ((depth <= 24) && (bpp == 1))
        {
          image = load_xwd_f1_d24_b1 (file, ifp, &xwdhdr, xwdcolmap, error);
        }
      break;

    case 2:    /* Multiplane pixmaps */
      if ((depth == 1) && (bpp == 1))
        {
          image = load_xwd_f2_d1_b1 (file, ifp, &xwdhdr, xwdcolmap, error);
        }
      else if ((depth <= 8) && (bpp == 8))
        {
          image = load_xwd_f2_d8_b8 (file, ifp, &xwdhdr, xwdcolmap, error);
        }
      else if ((depth <= 16) && (bpp == 16))
        {
          if (xwdcolmap)
            image = load_xwd_f2_d16_b16 (file, ifp, &xwdhdr, xwdcolmap, error);
        }
      else if ((depth <= 24) && ((bpp == 24) || (bpp == 32)))
        {
          image = load_xwd_f2_d24_b32 (file, ifp, &xwdhdr, xwdcolmap, error);
        }
      else if ((depth <= 32) && (bpp == 32))
        {
          image = load_xwd_f2_d32_b32 (file, ifp, &xwdhdr, xwdcolmap, error);
        }
      break;
    }
  gimp_progress_update (1.0);

  if (! image && ! (error && *error))
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                 _("XWD-file %s has format %d, depth %d and bits per pixel %d. "
                   "Currently this is not supported."),
                 gimp_file_get_utf8_name (file),
                 (gint) xwdhdr.l_pixmap_format, depth, bpp);

out:
  if (ifp)
    fclose (ifp);

  if (xwdcolmap)
    g_free (xwdcolmap);

  return image;
}

static gboolean
export_image (GFile         *file,
              GimpImage     *image,
              GimpDrawable  *drawable,
              GError       **error)
{
  GOutputStream *output;
  GimpImageType  drawable_type;
  gboolean       success;

  drawable_type = gimp_drawable_type (drawable);

  /*  Make sure we're not exporting an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable))
    {
      g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Cannot export images with alpha channels."));
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

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file, NULL, FALSE, 0, NULL, error));
  if (! output)
    {
      g_prefix_error (error,
                      _("Could not open '%s' for writing: "),
                      gimp_file_get_utf8_name (file));
      return FALSE;
    }

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
      success = save_index (output, image, drawable, FALSE, error);
      break;

    case GIMP_GRAY_IMAGE:
      success = save_index (output, image, drawable, TRUE, error);
      break;

    case GIMP_RGB_IMAGE:
      success = save_rgb (output, image, drawable, error);
      break;

    default:
      success = FALSE;
      break;
    }

  if (success && ! g_output_stream_close (output, NULL, error))
    {
      g_prefix_error (error,
                      _("Error exporting '%s': "),
                      gimp_file_get_utf8_name (file));
      success = FALSE;
    }
  else if (! success)
    {
      GCancellable  *cancellable;

      cancellable = g_cancellable_new ();
      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);
    }

  g_object_unref (output);

  gimp_progress_update (1.0);

  return success;
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


static gboolean
write_card32 (GOutputStream  *output,
              L_CARD32        c,
              GError        **error)
{
  guchar buffer[4];

  buffer[0] = (c >> 24) & 0xff;
  buffer[1] = (c >> 16) & 0xff;
  buffer[2] = (c >>  8) & 0xff;
  buffer[3] = (c)       & 0xff;

  return g_output_stream_write_all (output, buffer, 4,
                                    NULL, NULL, error);
}

static gboolean
write_card16 (GOutputStream  *output,
              L_CARD32        c,
              GError        **error)
{
  guchar buffer[2];

  buffer[0] = (c >> 8) & 0xff;
  buffer[1] = (c)      & 0xff;

  return g_output_stream_write_all (output, buffer, 2,
                                    NULL, NULL, error);
}

static gboolean
write_card8 (GOutputStream  *output,
             L_CARD32        c,
             GError        **error)
{
  guchar buffer[1];

  buffer[0] = c & 0xff;

  return g_output_stream_write_all (output, buffer, 1,
                                    NULL, NULL, error);
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
static gboolean
write_xwd_header (GOutputStream    *output,
                  L_XWDFILEHEADER  *xwdhdr,
                  GError          **error)

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
      if (! write_card32 (output, *(cp++), error))
        return FALSE;
    }

  /* Add padding bytes after XWD header */
  for (j = 0; j < hdrpad; j++)
    if (! write_card8 (output, (L_CARD32) 0, error))
      return FALSE;

  return TRUE;
}


static void
read_xwd_cols (FILE             *ifp,
               L_XWDFILEHEADER  *xwdhdr,
               L_XWDCOLOR       *colormap,
               GError          **error)
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
  if (fseek (ifp, colmappos, SEEK_SET) != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Seek error"));
      return;
    }

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
  if (fseek (ifp, colmappos, SEEK_SET) !=0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Seek error"));
      return;
    }

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
  if (fseek (ifp, colmappos, SEEK_SET) != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Seek error"));
      return;
    }

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
      g_printf ("xwd: Warning. Error in XWD-color-structure (");

      if (flag_is_bad) g_printf ("flag");
      if (index_is_bad) g_printf ("index");

      g_printf (")\n");
    }

  if (err)
    xwdhdr->l_file_version = 0;  /* Not a valid XWD-file */
}

static gboolean
write_xwd_cols (GOutputStream    *output,
                L_XWDFILEHEADER  *xwdhdr,
                L_XWDCOLOR       *colormap,
                GError          **error)
{
  gint j;

  for (j = 0; j < xwdhdr->l_colormap_entries; j++)
    {
#ifdef CRAY
      if (! (write_card32 (output, (L_CARD32)0, error)                   &&
             write_card32 (output, colormap[j].l_pixel, error)           &&
             write_card32 (output, (L_CARD32)0, error)                   &&
             write_card32 (output, (L_CARD32)colormap[j].l_red, error)   &&
             write_card32 (output, (L_CARD32)0, error)                   &&
             write_card32 (output, (L_CARD32)colormap[j].l_green, error) &&
             write_card32 (output, (L_CARD32)0, error)                   &&
             write_card32 (output, (L_CARD32)colormap[j].l_blue, error)  &&
             write_card8  (output, (L_CARD32)colormap[j].l_flags, error) &&
             write_card8  (output, (L_CARD32)colormap[j].l_pad, error)   &&
             write_card16 (output, (L_CARD32)0, error)                   &&
             write_card32 (output, (L_CARD32)0, error)))
        {
          return FALSE;
        }
#else
#ifdef __alpha
      if (! (write_card32 (output, colormap[j].l_pixel, error)           &&
             write_card32 (output, (L_CARD32)0, error)                   &&
             write_card16 (output, (L_CARD32)colormap[j].l_red, error)   &&
             write_card16 (output, (L_CARD32)colormap[j].l_green, error) &&
             write_card16 (output, (L_CARD32)colormap[j].l_blue, error)  &&
             write_card8  (output, (L_CARD32)colormap[j].l_flags, error) &&
             write_card8  (output, (L_CARD32)colormap[j].l_pad, error)))
        {
          return FALSE;
        }
#else
      if (! (write_card32 (output, colormap[j].l_pixel, error)           &&
             write_card16 (output, (L_CARD32)colormap[j].l_red, error)   &&
             write_card16 (output, (L_CARD32)colormap[j].l_green, error) &&
             write_card16 (output, (L_CARD32)colormap[j].l_blue, error)  &&
             write_card8  (output, (L_CARD32)colormap[j].l_flags, error) &&
             write_card8  (output, (L_CARD32)colormap[j].l_pad, error)))
        {
          return FALSE;
        }
#endif
#endif
    }

  return TRUE;
}


/* Create a map for mapping up to 32 bit pixelvalues to RGB.
 * Returns number of colors kept in the map (up to 256)
 */
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
  g_printf ("Colors in pixelmap: %d\n",pixelmap->npixel);
  for (j=0; j<pixelmap->npixel; j++)
    g_printf ("Pixelvalue 0x%08lx, 0x%02x 0x%02x 0x%02x\n",
              pixelmap->pmap[j].pixel_val,
              pixelmap->pmap[j].red,pixelmap->pmap[j].green,
              pixelmap->pmap[j].blue);
  for (j=0; j<=MAPPERMASK; j++)
    g_printf ("0x%08lx: %d\n",(long)j,pixelmap->pixel_in_map[j]);
#endif

  return pixelmap->npixel;
}


/* Search a pixel value in the pixel map. Returns FALSE if the
 * pixelval was not found in map. Returns TRUE if found.
 */
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
set_bw_color_table (GimpImage *image)
{
  static guchar BWColorMap[2*3] = { 255, 255, 255, 0, 0, 0 };

#ifdef XWD_COL_DEBUG
  g_printf ("Set GIMP b/w-colortable:\n");
#endif

  gimp_image_set_colormap (image, BWColorMap, 2);
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
set_color_table (GimpImage       *image,
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
  g_printf ("Set GIMP colortable:\n");
  for (j = 0; j < 256; j++)
    g_printf ("%3d: 0x%02x 0x%02x 0x%02x\n", j,
              ColorMap[j*3], ColorMap[j*3+1], ColorMap[j*3+2]);
#endif

  gimp_image_set_colormap (image, ColorMap, 256);
}




/* Create an image. Sets layer, drawable and rgn. Returns image */
static GimpImage *
create_new_image (GFile               *file,
                  guint                width,
                  guint                height,
                  GimpImageBaseType    type,
                  GimpImageType        gdtype,
                  GimpLayer          **layer,
                  GeglBuffer         **buffer)
{
  GimpImage *image;

  image = gimp_image_new (width, height, type);

  *layer = gimp_layer_new (image, "Background", width, height,
                           gdtype,
                           100,
                           gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, *layer, NULL, 0);

  *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (*layer));

  return image;
}


/* Load XWD with pixmap_format 2, pixmap_depth 1, bits_per_pixel 1 */

static GimpImage *
load_xwd_f2_d1_b1 (GFile           *file,
                   FILE            *ifp,
                   L_XWDFILEHEADER *xwdhdr,
                   L_XWDCOLOR      *xwdcolmap,
                   GError         **error)
{
  register int     pix8;
  register guchar *dest, *src;
  guchar           c1, c2, c3, c4;
  gint             width, height, scan_lines, tile_height;
  gint             i, j, ncols;
  gchar           *temp;
  guchar           bit2byte[256 * 8];
  guchar          *data, *scanline;
  gint             err = 0;
  GimpImage       *image;
  GimpLayer       *layer;
  GeglBuffer      *buffer;

#ifdef XWD_DEBUG
  g_printf ("load_xwd_f2_d1_b1 (%s)\n", gimp_file_get_utf8_name (file));
#endif

  width  = xwdhdr->l_pixmap_width;
  height = xwdhdr->l_pixmap_height;

  image = create_new_image (file, width, height, GIMP_INDEXED,
                            GIMP_INDEXED_IMAGE, &layer, &buffer);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width);

  scanline = g_new (guchar, xwdhdr->l_bytes_per_line + 8);

  ncols = xwdhdr->l_colormap_entries;
  if (xwdhdr->l_ncolors < ncols)
    ncols = xwdhdr->l_ncolors;

  if (ncols < 2)
    set_bw_color_table (image);
  else
    set_color_table (image, xwdhdr, xwdcolmap);

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
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i - scan_lines + 1,
                                                   width, scan_lines), 0,
                           NULL, data, GEGL_AUTO_ROWSTRIDE);

          scan_lines = 0;
          dest = data;
        }
    }

  g_free (data);
  g_free (scanline);
  g_object_unref (buffer);

  if (err)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("EOF encountered on reading"));
      return NULL;
    }

  return image;
}


/* Load XWD with pixmap_format 2, pixmap_depth 8, bits_per_pixel 8 */

static GimpImage *
load_xwd_f2_d8_b8 (GFile           *file,
                   FILE            *ifp,
                   L_XWDFILEHEADER *xwdhdr,
                   L_XWDCOLOR      *xwdcolmap,
                   GError         **error)
{
  gint        width, height, linepad, tile_height, scan_lines;
  gint        i, j, ncols;
  gint        grayscale;
  guchar     *dest, *data;
  gint        err = 0;
  GimpImage  *image;
  GimpLayer  *layer;
  GeglBuffer *buffer;

#ifdef XWD_DEBUG
  g_printf ("load_xwd_f2_d8_b8 (%s)\n", gimp_file_get_utf8_name (file));
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

  image = create_new_image (file, width, height,
                            grayscale ? GIMP_GRAY : GIMP_INDEXED,
                            grayscale ? GIMP_GRAY_IMAGE : GIMP_INDEXED_IMAGE,
                            &layer, &buffer);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width);

  if (!grayscale)
    {
      ncols = xwdhdr->l_colormap_entries;
      if (xwdhdr->l_ncolors < ncols) ncols = xwdhdr->l_ncolors;
      if (ncols < 2)
        set_bw_color_table (image);
      else
        set_color_table (image, xwdhdr, xwdcolmap);
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
        if (getc (ifp) < 0)
          {
            err = 1;
            break;
          }
      if (err)
        break;

      scan_lines++;

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) (i + 1) / (gdouble) height);

      if ((scan_lines == tile_height) || ((i+1) == height))
        {
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i - scan_lines + 1,
                                                   width, scan_lines), 0,
                           NULL, data, GEGL_AUTO_ROWSTRIDE);

          scan_lines = 0;
          dest = data;
        }
    }

  g_free (data);
  g_object_unref (buffer);

  if (err)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("EOF encountered on reading"));
      return NULL;
    }

  return image;
}


/* Load XWD with pixmap_format 2, pixmap_depth up to 16, bits_per_pixel 16 */

static GimpImage *
load_xwd_f2_d16_b16 (GFile           *file,
                     FILE            *ifp,
                     L_XWDFILEHEADER *xwdhdr,
                     L_XWDCOLOR      *xwdcolmap,
                     GError         **error)
{
  register guchar *dest, lsbyte_first;
  gint             width, height, linepad, i, j, c0, c1, ncols;
  gint             red, green, blue, redval, greenval, blueval;
  gint             maxred, maxgreen, maxblue;
  gint             tile_height, scan_lines;
  guint32          redmask, greenmask, bluemask;
  guint            redshift, greenshift, blueshift;
  guint32          maxval;
  guchar          *ColorMap, *cm, *data;
  gint             err = 0;
  GimpImage       *image;
  GimpLayer       *layer;
  GeglBuffer      *buffer;

#ifdef XWD_DEBUG
  g_printf ("load_xwd_f2_d16_b16 (%s)\n", gimp_file_get_utf8_name (file));
#endif

  width  = xwdhdr->l_pixmap_width;
  height = xwdhdr->l_pixmap_height;

  image = create_new_image (file, width, height, GIMP_RGB,
                            GIMP_RGB_IMAGE, &layer, &buffer);

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
        if (getc (ifp) < 0)
          {
            err = 1;
            break;
          }
      if (err)
        break;

      scan_lines++;

      if ((i % 20) == 0)
        gimp_progress_update ((double)(i+1) / (double)height);

      if ((scan_lines == tile_height) || ((i+1) == height))
        {
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i - scan_lines + 1,
                                                   width, scan_lines), 0,
                           NULL, data, GEGL_AUTO_ROWSTRIDE);

          scan_lines = 0;
          dest = data;
        }
    }
  g_free (data);
  g_free (ColorMap);

  g_object_unref (buffer);

  if (err)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("EOF encountered on reading"));
      return NULL;
    }

  return image;
}


/* Load XWD with pixmap_format 2, pixmap_depth up to 24, bits_per_pixel 24/32 */

static GimpImage *
load_xwd_f2_d24_b32 (GFile            *file,
                     FILE             *ifp,
                     L_XWDFILEHEADER  *xwdhdr,
                     L_XWDCOLOR       *xwdcolmap,
                     GError          **error)
{
  register guchar *dest, lsbyte_first;
  gint             width, height, linepad, i, j, c0, c1, c2, c3;
  gint             tile_height, scan_lines;
  L_CARD32         pixelval;
  gint             red, green, blue, ncols;
  guint32          maxred, maxgreen, maxblue;
  guint32          redmask, greenmask, bluemask;
  guint            redshift, greenshift, blueshift;
  guchar           redmap[256], greenmap[256], bluemap[256];
  guchar          *data;
  PIXEL_MAP        pixel_map;
  gint             err = 0;
  GimpImage       *image;
  GimpLayer       *layer;
  GeglBuffer      *buffer;

#ifdef XWD_DEBUG
  g_printf ("load_xwd_f2_d24_b32 (%s)\n", gimp_file_get_utf8_name (file));
#endif

  /* issue #8082: depth and bits per pixel is 24, but 4 bytes are used per pixel */
  if (xwdhdr->l_bits_per_pixel == 24)
    {
      if (xwdhdr->l_bytes_per_line / xwdhdr->l_pixmap_width == 4 &&
          xwdhdr->l_bytes_per_line % xwdhdr->l_pixmap_width == 0)
        xwdhdr->l_bits_per_pixel = 32;
    }

  width  = xwdhdr->l_pixmap_width;
  height = xwdhdr->l_pixmap_height;

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

  if (maxred   == 0 || maxred   >= sizeof (redmap)   ||
      maxgreen == 0 || maxgreen >= sizeof (greenmap) ||
      maxblue  == 0 || maxblue  >= sizeof (bluemap))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("XWD-file %s is corrupt."),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  image = create_new_image (file, width, height, GIMP_RGB,
                            GIMP_RGB_IMAGE, &layer, &buffer);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width * 3);

  /* Set map-arrays for red, green, blue */
  for (red = 0; red <= maxred; red++)
    redmap[red] = (red * 255) / maxred;
  for (green = 0; green <= maxgreen; green++)
    greenmap[green] = (green * 255) / maxgreen;
  for (blue = 0; blue <= maxblue; blue++)
    bluemap[blue] = (blue * 255) / maxblue;

  ncols = xwdhdr->l_colormap_entries;
  if (xwdhdr->l_ncolors < ncols)
    ncols = xwdhdr->l_ncolors;

  set_pixelmap (ncols, xwdcolmap, &pixel_map);

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
            if (getc (ifp) < 0)
              {
                err = 1;
                break;
              }
          if (err)
            break;

          if ((i % 20) == 0)
            gimp_progress_update ((gdouble) (i + 1) / (gdouble) height);

          if ((scan_lines == tile_height) || ((i+1) == height))
            {
              gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i - scan_lines + 1,
                                                       width, scan_lines), 0,
                               NULL, data, GEGL_AUTO_ROWSTRIDE);

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
            if (getc (ifp) < 0)
              {
                err = 1;
                break;
              }
          if (err)
            break;

          if ((i % 20) == 0)
            gimp_progress_update ((gdouble) (i + 1) / (gdouble) height);

          if ((scan_lines == tile_height) || ((i+1) == height))
            {
              gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i - scan_lines + 1,
                                                       width, scan_lines), 0,
                               NULL, data, GEGL_AUTO_ROWSTRIDE);

              scan_lines = 0;
              dest = data;
            }
        }
    }

  g_free (data);

  g_object_unref (buffer);

  if (err)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("EOF encountered on reading"));
      return NULL;
    }

  return image;
}

/* Load XWD with pixmap_format 2, pixmap_depth up to 32, bits_per_pixel 32 */

static GimpImage *
load_xwd_f2_d32_b32 (GFile           *file,
                     FILE            *ifp,
                     L_XWDFILEHEADER *xwdhdr,
                     L_XWDCOLOR      *xwdcolmap,
                     GError         **error)
{
  register guchar *dest, lsbyte_first;
  gint             width, height, linepad, i, j, c0, c1, c2, c3;
  gint             tile_height, scan_lines;
  L_CARD32         pixelval;
  gint             red, green, blue, alpha, ncols;
  guint32          maxred, maxgreen, maxblue, maxalpha;
  guint32          redmask, greenmask, bluemask, alphamask;
  guint            redshift, greenshift, blueshift, alphashift;
  guchar           redmap[256], greenmap[256], bluemap[256], alphamap[256];
  guchar          *data;
  PIXEL_MAP        pixel_map;
  gint             err = 0;
  GimpImage       *image;
  GimpLayer       *layer;
  GeglBuffer      *buffer;

#ifdef XWD_DEBUG
  g_printf ("load_xwd_f2_d32_b32 (%s)\n", gimp_file_get_utf8_name (file));
#endif

  if (! xwdcolmap)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s':\nInvalid color map"),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  width  = xwdhdr->l_pixmap_width;
  height = xwdhdr->l_pixmap_height;

  image = create_new_image (file, width, height, GIMP_RGB,
                            GIMP_RGBA_IMAGE, &layer, &buffer);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width * 4);

  redmask   = xwdhdr->l_red_mask;
  greenmask = xwdhdr->l_green_mask;
  bluemask  = xwdhdr->l_blue_mask;

  if (redmask   == 0) redmask   = 0xff0000;
  if (greenmask == 0) greenmask = 0x00ff00;
  if (bluemask  == 0) bluemask  = 0x0000ff;
  alphamask = 0xffffffff & ~(redmask | greenmask | bluemask);

  /* How to shift RGB to be right aligned ? */
  /* (We rely on the the mask bits are grouped and not mixed) */
  redshift = greenshift = blueshift = alphashift = 0;

  while (((1 << redshift)   & redmask)   == 0) redshift++;
  while (((1 << greenshift) & greenmask) == 0) greenshift++;
  while (((1 << blueshift)  & bluemask)  == 0) blueshift++;
  while (((1 << alphashift) & alphamask) == 0) alphashift++;

  /* The bits_per_rgb may not be correct. Use redmask instead */

  maxred = 0; while (redmask >> (redshift + maxred)) maxred++;
  maxred = (1 << maxred) - 1;

  maxgreen = 0; while (greenmask >> (greenshift + maxgreen)) maxgreen++;
  maxgreen = (1 << maxgreen) - 1;

  maxblue = 0; while (bluemask >> (blueshift + maxblue)) maxblue++;
  maxblue = (1 << maxblue) - 1;

  maxalpha = 0; while (alphamask >> (alphashift + maxalpha)) maxalpha++;
  maxalpha = (1 << maxalpha) - 1;

  if (maxred   == 0 || maxred   >= sizeof (redmap)   ||
      maxgreen == 0 || maxgreen >= sizeof (greenmap) ||
      maxblue  == 0 || maxblue  >= sizeof (bluemap)  ||
      maxalpha == 0 || maxalpha >= sizeof (alphamap))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                    _("XWD-file %s is corrupt."),
                    gimp_file_get_utf8_name (file));
      return NULL;
    }

  /* Set map-arrays for red, green, blue */
  for (red = 0; red <= maxred; red++)
    redmap[red] = (red * 255) / maxred;
  for (green = 0; green <= maxgreen; green++)
    greenmap[green] = (green * 255) / maxgreen;
  for (blue = 0; blue <= maxblue; blue++)
    bluemap[blue] = (blue * 255) / maxblue;
  for (alpha = 0; alpha <= maxalpha; alpha++)
    alphamap[alpha] = (alpha * 255) / maxalpha;

  ncols = xwdhdr->l_colormap_entries;
  if (xwdhdr->l_ncolors < ncols)
    ncols = xwdhdr->l_ncolors;

  set_pixelmap (ncols, xwdcolmap, &pixel_map);

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
              /* FIXME: is it always transparent or encoded in an unknown way? */
              *(dest+3) = 0x00;
              dest += 4;
            }
          else
            {
              *(dest++) = redmap[(pixelval & redmask) >> redshift];
              *(dest++) = greenmap[(pixelval & greenmask) >> greenshift];
              *(dest++) = bluemap[(pixelval & bluemask) >> blueshift];
              *(dest++) = alphamap[(pixelval & alphamask) >> alphashift];
            }
        }
      scan_lines++;

      if (err)
        break;

      for (j = 0; j < linepad; j++)
        if (getc (ifp) < 0)
          {
            err = 1;
            break;
          }
      if (err)
        break;

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) (i + 1) / (gdouble) height);

      if ((scan_lines == tile_height) || ((i+1) == height))
        {
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i - scan_lines + 1,
                                                   width, scan_lines), 0,
                           NULL, data, GEGL_AUTO_ROWSTRIDE);

          scan_lines = 0;
          dest = data;
        }
    }

  g_free (data);

  g_object_unref (buffer);

  if (err)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("EOF encountered on reading"));
      return NULL;
    }

  return image;
}

/* Load XWD with pixmap_format 1, pixmap_depth up to 24, bits_per_pixel 1 */

static GimpImage *
load_xwd_f1_d24_b1 (GFile            *file,
                    FILE             *ifp,
                    L_XWDFILEHEADER  *xwdhdr,
                    L_XWDCOLOR       *xwdcolmap,
                    GError          **error)
{
  register guchar *dest, outmask, inmask, do_reverse;
  gint             width, height, i, j, plane, fromright;
  gint             tile_height, tile_start, tile_end;
  gint             indexed, bytes_per_pixel;
  gint             maxred, maxgreen, maxblue;
  gint             red, green, blue, ncols, standard_rgb;
  goffset          data_offset, plane_offset, tile_offset;
  guint32          redmask, greenmask, bluemask;
  guint            redshift, greenshift, blueshift;
  guint32          g;
  guchar           redmap[256], greenmap[256], bluemap[256];
  guchar           bit_reverse[256];
  guchar          *xwddata, *xwdin, *data;
  L_CARD32         pixelval;
  PIXEL_MAP        pixel_map;
  gint             err = 0;
  GimpImage       *image;
  GimpLayer       *layer;
  GeglBuffer      *buffer;

#ifdef XWD_DEBUG
  g_printf ("load_xwd_f1_d24_b1 (%s)\n", gimp_file_get_utf8_name (file));
#endif

  xwddata = g_malloc (xwdhdr->l_bytes_per_line);
  if (xwddata == NULL)
    return NULL;

  width           = xwdhdr->l_pixmap_width;
  height          = xwdhdr->l_pixmap_height;
  indexed         = (xwdhdr->l_pixmap_depth <= 8);
  bytes_per_pixel = (indexed ? 1 : 3);

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

      if (maxred   == 0 || maxred   >= sizeof (redmap)   ||
          maxgreen == 0 || maxgreen >= sizeof (greenmap) ||
          maxblue  == 0 || maxblue  >= sizeof (bluemap))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("XWD-file %s is corrupt."),
                       gimp_file_get_utf8_name (file));
          return NULL;
        }

      /* Set map-arrays for red, green, blue */
      for (red = 0; red <= maxred; red++)
        redmap[red] = (red * 255) / maxred;
      for (green = 0; green <= maxgreen; green++)
        greenmap[green] = (green * 255) / maxgreen;
      for (blue = 0; blue <= maxblue; blue++)
        bluemap[blue] = (blue * 255) / maxblue;
    }

  image = create_new_image (file, width, height,
                            indexed ? GIMP_INDEXED : GIMP_RGB,
                            indexed ? GIMP_INDEXED_IMAGE : GIMP_RGB_IMAGE,
                            &layer, &buffer);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width * bytes_per_pixel);

  ncols = xwdhdr->l_colormap_entries;
  if (xwdhdr->l_ncolors < ncols)
    ncols = xwdhdr->l_ncolors;

  if (indexed)
    {
      if (ncols < 2)
        set_bw_color_table (image);
      else
        set_color_table (image, xwdhdr, xwdcolmap);
    }
  else
    {
      set_pixelmap (ncols, xwdcolmap, &pixel_map);
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
          if (fseek (ifp, tile_offset, SEEK_SET) != 0)
            {
              err = 1;
              break;
            }

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
          if (err)
            break;
        }
      if (err)
        break;

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

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, tile_start,
                                               width, tile_end-tile_start+1), 0,
                       NULL, data, GEGL_AUTO_ROWSTRIDE);
    }

  g_free (data);
  g_free (xwddata);

  g_object_unref (buffer);

  if (err)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("EOF encountered on reading"));
      return NULL;
    }

  return image;
}


static gboolean
save_index (GOutputStream  *output,
            GimpImage      *image,
            GimpDrawable   *drawable,
            gboolean        gray,
            GError        **error)
{
  gint             height, width;
  gint             linepad;
  gint             tile_height;
  gint             i, j;
  gint             ncolors, vclass;
  gsize            tmp = 0;
  guchar          *data, *src, *cmap;
  L_XWDFILEHEADER  xwdhdr;
  L_XWDCOLOR       xwdcolmap[256];
  const Babl      *format;
  GeglBuffer      *buffer;
  gboolean         success = TRUE;

#ifdef XWD_DEBUG
  g_printf ("save_index ()\n");
#endif

  buffer      = gimp_drawable_get_buffer (drawable);
  width       = gegl_buffer_get_width  (buffer);
  height      = gegl_buffer_get_height (buffer);
  tile_height = gimp_tile_height ();

  if (gray)
    format = babl_format ("Y' u8");
  else
    format = gegl_buffer_get_format (buffer);

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = g_new (guchar,
                      tile_height * width *
                      babl_format_get_bytes_per_pixel (format));

  linepad = width % 4;
  if (linepad)
    linepad = 4 - linepad;

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
      cmap = gimp_image_get_colormap (image, NULL, &ncolors);

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

  success = (write_xwd_header (output, &xwdhdr, error) &&
             write_xwd_cols (output, &xwdhdr, xwdcolmap, error));
  if (! success)
    goto out;

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)   /* Get more data */
        {
          gint scan_lines = (i + tile_height - 1 < height) ? tile_height : (height - i);

          gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines), 1.0,
                           format, data,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          src = data;
        }

      success = g_output_stream_write_all (output, src, width,
                                           NULL, NULL, error);
      if (! success)
        goto out;

      if (linepad)
        {
          success = g_output_stream_write_all (output, &tmp, linepad,
                                               NULL, NULL, error);
          if (! success)
            goto out;
        }

      src += width;

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

 out:
  g_free (data);
  g_object_unref (buffer);

  return success;
}

static gboolean
save_rgb (GOutputStream  *output,
          GimpImage      *image,
          GimpDrawable   *drawable,
          GError        **error)
{
  gint             height, width;
  gint             linepad;
  gint             tile_height;
  gint             i;
  gsize            tmp = 0;
  guchar          *data, *src;
  L_XWDFILEHEADER  xwdhdr;
  const Babl      *format;
  GeglBuffer      *buffer;
  gboolean         success = TRUE;

#ifdef XWD_DEBUG
  g_printf ("save_rgb ()\n");
#endif

  buffer      = gimp_drawable_get_buffer (drawable);
  width       = gegl_buffer_get_width  (buffer);
  height      = gegl_buffer_get_height (buffer);
  tile_height = gimp_tile_height ();
  format      = babl_format ("R'G'B' u8");

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = g_new (guchar,
                      tile_height * width *
                      babl_format_get_bytes_per_pixel (format));

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

  success = write_xwd_header (output, &xwdhdr, error);
  if (! success)
    goto out;

  for (i = 0; i < height; i++)
    {
      if ((i % tile_height) == 0)   /* Get more data */
        {
          gint scan_lines = (i + tile_height - 1 < height) ? tile_height : (height - i);

          gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines), 1.0,
                           format, data,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          src = data;
        }

      success = g_output_stream_write_all (output, src, width * 3,
                                           NULL, NULL, error);
      if (! success)
        goto out;

      if (linepad)
        {
          success = g_output_stream_write_all (output, &tmp, linepad,
                                               NULL, NULL, error);
          if (! success)
            goto out;
        }

      src += width * 3;

      if ((i % 20) == 0)
        gimp_progress_update ((gdouble) i / (gdouble) height);
    }

 out:
  g_free (data);
  g_object_unref (buffer);

  return success;
}
