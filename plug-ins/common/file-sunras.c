/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * SUN raster reading and writing code Copyright (C) 1996 Peter Kirchgessner
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

/* This program was written using pages 625-629 of the book
 * "Encyclopedia of Graphics File Formats", Murray/van Ryper,
 * O'Reilly & Associates Inc.
 * Bug reports or suggestions should be e-mailed to peter@kirchgessner.net
 */

/* Event history:
 * V 1.00, PK, 25-Jul-96: First try
 * V 1.90, PK, 15-Mar-97: Upgrade to work with GIMP V0.99
 * V 1.91, PK, 05-Apr-97: Return all arguments, even in case of an error
 * V 1.92, PK, 18-May-97: Ignore EOF-error on reading image data
 * V 1.93, PK, 05-Oct-97: Parse rc file
 * V 1.94, PK, 12-Oct-97: No progress bars for non-interactive mode
 * V 1.95, nn, 20-Dec-97: Initialize some variable
 * V 1.96, PK, 21-Nov-99: Internationalization
 * V 1.97, PK, 20-Dec-00: Recognize extensions .rs and .ras too
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-sunras-load"
#define EXPORT_PROC    "file-sunras-export"
#define PLUG_IN_BINARY "file-sunras"
#define PLUG_IN_ROLE   "gimp-file-sunras"


typedef int WRITE_FUN(void*,size_t,size_t,FILE*);

typedef gulong  L_CARD32;
typedef gushort L_CARD16;
typedef guchar  L_CARD8;

/* Fileheader of SunRaster files */
typedef struct
{
  L_CARD32 l_ras_magic;    /* Magic Number */
  L_CARD32 l_ras_width;    /* Width */
  L_CARD32 l_ras_height;   /* Height */
  L_CARD32 l_ras_depth;    /* Number of bits per pixel (1,8,24,32) */
  L_CARD32 l_ras_length;   /* Length of image data (but may also be 0) */
  L_CARD32 l_ras_type;     /* Encoding */
  L_CARD32 l_ras_maptype;  /* Type of colormap */
  L_CARD32 l_ras_maplength;/* Number of bytes for colormap */
} L_SUNFILEHEADER;

/* Sun-raster magic */
#define RAS_MAGIC 0x59a66a95

#define RAS_TYPE_STD 1    /* Standard uncompressed format */
#define RAS_TYPE_RLE 2    /* Runlength compression format */

typedef struct
{
  gint val;   /* The value that is to be repeated */
  gint n;     /* How many times it is repeated */
} RLEBUF;


typedef struct _Sunras      Sunras;
typedef struct _SunrasClass SunrasClass;

struct _Sunras
{
  GimpPlugIn      parent_instance;
};

struct _SunrasClass
{
  GimpPlugInClass parent_class;
};


#define SUNRAS_TYPE  (sunras_get_type ())
#define SUNRAS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SUNRAS_TYPE, Sunras))

GType                   sunras_get_type         (void) G_GNUC_CONST;

static GList          * sunras_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * sunras_create_procedure (GimpPlugIn            *plug_in,
                                                 const gchar           *name);

static GimpValueArray * sunras_load             (GimpProcedure         *procedure,
                                                 GimpRunMode            run_mode,
                                                 GFile                 *file,
                                                 GimpMetadata          *metadata,
                                                 GimpMetadataLoadFlags *flags,
                                                 GimpProcedureConfig   *config,
                                                 gpointer               run_data);
static GimpValueArray * sunras_export           (GimpProcedure         *procedure,
                                                 GimpRunMode            run_mode,
                                                 GimpImage             *image,
                                                 GFile                 *file,
                                                 GimpExportOptions     *options,
                                                 GimpMetadata          *metadata,
                                                 GimpProcedureConfig   *config,
                                                 gpointer               run_data);

static GimpImage      * load_image              (GFile                 *file,
                                                 GError               **error);
static gboolean         export_image            (GFile                 *file,
                                                 GimpImage             *image,
                                                 GimpDrawable          *drawable,
                                                 GObject               *config,
                                                 GError               **error);

static void             set_color_table         (GimpImage             *image,
                                                 L_SUNFILEHEADER       *sunhdr,
                                                 const guchar          *suncolmap);
static GimpImage      * create_new_image        (GFile                 *file,
                                                 guint                  width,
                                                 guint                  height,
                                                 GimpImageBaseType      type,
                                                 GimpLayer            **layer,
                                                 GeglBuffer           **buffer);

static GimpImage      * load_sun_d1             (GFile                 *file,
                                                 FILE                  *ifp,
                                                 L_SUNFILEHEADER       *sunhdr,
                                                 guchar                *suncolmap);
static GimpImage      * load_sun_d8             (GFile                 *file,
                                                 FILE                  *ifp,
                                                 L_SUNFILEHEADER       *sunhdr,
                                                 guchar                *suncolmap);
static GimpImage      * load_sun_d24            (GFile                 *file,
                                                 FILE                  *ifp,
                                                 L_SUNFILEHEADER       *sunhdr,
                                                 guchar                *suncolmap);
static GimpImage      * load_sun_d32            (GFile                 *file,
                                                 FILE                  *ifp,
                                                 L_SUNFILEHEADER       *sunhdr,
                                                 guchar                *suncolmap);

static L_CARD32         read_card32             (FILE                  *ifp,
                                                 int                   *err);

static void             write_card32            (FILE                  *ofp,
                                                 L_CARD32               c);

static void             byte2bit                (guchar                *byteline,
                                                 int                    width,
                                                 guchar                *bitline,
                                                 gboolean               invert);

static void             rle_startread           (FILE                  *ifp);
static int              rle_fread               (char                  *ptr,
                                                 int                    sz,
                                                 int                    nelem,
                                                 FILE                  *ifp);
static int              rle_fgetc               (FILE                  *ifp);
#define rle_getc(fp) ((rlebuf.n > 0) ? (rlebuf.n)--,rlebuf.val : rle_fgetc (fp))

static void             rle_startwrite          (FILE                  *ofp);
static int              rle_fwrite              (char                  *ptr,
                                                 int                    sz,
                                                 int                    nelem,
                                                 FILE                  *ofp);
static int              rle_fputc               (int                    val,
                                                 FILE                  *ofp);
static int              rle_putrun              (int                    n,
                                                 int                    val,
                                                 FILE                  *ofp);
static void             rle_endwrite            (FILE                  *ofp);
#define rle_putc rle_fputc

static void             read_sun_header         (FILE                  *ifp,
                                                 L_SUNFILEHEADER       *sunhdr);
static void             write_sun_header        (FILE                  *ofp,
                                                 L_SUNFILEHEADER       *sunhdr);
static void             read_sun_cols           (FILE                  *ifp,
                                                 L_SUNFILEHEADER       *sunhdr,
                                                 guchar                *colormap);
static void             write_sun_cols          (FILE                  *ofp,
                                                 L_SUNFILEHEADER       *sunhdr,
                                                 guchar                *colormap);

static gint             save_index              (FILE                  *ofp,
                                                 GimpImage             *image,
                                                 GimpDrawable          *drawable,
                                                 gboolean               grey,
                                                 gint                   rle);
static gint             save_rgb                (FILE                  *ofp,
                                                 GimpImage             *image,
                                                 GimpDrawable          *drawable,
                                                 gint                   rle);

static gboolean         save_dialog             (GimpImage             *image,
                                                 GimpProcedure         *procedure,
                                                 GObject               *config);

/* Portability kludge */
static int              my_fwrite               (void                  *ptr,
                                                 int                    size,
                                                 int                    nmemb,
                                                 FILE                  *stream);


G_DEFINE_TYPE (Sunras, sunras, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SUNRAS_TYPE)
DEFINE_STD_SET_I18N


static int    read_msb_first = 1;
static RLEBUF rlebuf;


static void
sunras_class_init (SunrasClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = sunras_query_procedures;
  plug_in_class->create_procedure = sunras_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
sunras_init (Sunras *sunras)
{
}

static GList *
sunras_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
sunras_create_procedure (GimpPlugIn  *plug_in,
                         const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           sunras_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("SUN Rasterfile image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load file of the SunRaster file format"),
                                        _("Load file of the SunRaster file format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner",
                                      "1996");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-sun-raster");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "im1,im8,im24,im32,rs,ras,sun");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,long,0x59a66a95");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, sunras_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB, GRAY, INDEXED");

      gimp_procedure_set_menu_label (procedure, _("SUN Rasterfile image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Export file in the SunRaster file "
                                          "format"),
                                        _("SUNRAS exporting handles all image "
                                          "types except those with alpha "
                                          "channels."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Peter Kirchgessner",
                                      "Peter Kirchgessner",
                                      "1996");

      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("SUNRAS"));
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-sun-raster");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "im1,im8,im24,im32,rs,ras,sun");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB  |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED,
                                              NULL, NULL, NULL);

      gimp_procedure_add_choice_argument (procedure, "rle",
                                          _("_Data Formatting"),
                                          _("Use standard or Run-Length Encoded output"),
                                           gimp_choice_new_with_values ("standard", 0, _("Standard"),           NULL,
                                                                        "rle",      1, _("Run-Length Encoding"), NULL,
                                                                        NULL),
                                          "rle",
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
sunras_load (GimpProcedure         *procedure,
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
sunras_export (GimpProcedure        *procedure,
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

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (image, procedure, G_OBJECT (config)))
        status = GIMP_PDB_CANCEL;
    }

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! export_image (file, image, drawables->data, G_OBJECT (config),
                          &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }


  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpImage *
load_image (GFile   *file,
            GError **error)
{
  GimpImage       *image;
  FILE            *ifp;
  L_SUNFILEHEADER  sunhdr;
  guchar          *suncolmap = NULL;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  ifp = g_fopen (g_file_peek_path (file), "rb");

  if (! ifp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  read_msb_first = 1;   /* SUN raster is always most significant byte first */

  read_sun_header (ifp, &sunhdr);
  if (sunhdr.l_ras_magic != RAS_MAGIC)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' as SUN-raster-file"),
                   gimp_file_get_utf8_name (file));
      fclose (ifp);
      return NULL;
    }

  if (sunhdr.l_ras_type > 5)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s",
                   _("The type of this SUN-rasterfile is not supported"));
      fclose (ifp);
      return NULL;
    }

  if (sunhdr.l_ras_maplength > (256 * 3))
    {
      g_message ("Map lengths greater than 256 entries are unsupported by GIMP.");
      gimp_quit ();
    }

  /* Is there a RGB colormap ? */
  if ((sunhdr.l_ras_maptype == 1) && (sunhdr.l_ras_maplength > 0))
    {
      suncolmap = g_new (guchar, sunhdr.l_ras_maplength);

      read_sun_cols (ifp, &sunhdr, suncolmap);
#ifdef DEBUG
      {
        int j, ncols;
        printf ("File %s\n", g_file_peek_path (file));
        ncols = sunhdr.l_ras_maplength/3;
        for (j=0; j < ncols; j++)
          printf ("Entry 0x%08x: 0x%04x,  0x%04x, 0x%04x\n",
                  j,suncolmap[j],suncolmap[j+ncols],suncolmap[j+2*ncols]);
      }
#endif
      if (sunhdr.l_ras_magic != RAS_MAGIC)
        {
          g_message (_("Could not read color entries from '%s'"),
                     gimp_file_get_utf8_name (file));
          fclose (ifp);
          g_free (suncolmap);
          return NULL;
        }
    }
  else if (sunhdr.l_ras_maplength > 0)
    {
      g_message (_("Type of colormap not supported"));
      fseek (ifp, (sizeof (L_SUNFILEHEADER)/sizeof (L_CARD32))
             *4 + sunhdr.l_ras_maplength, SEEK_SET);
    }

  if (sunhdr.l_ras_width <= 0)
    {
      g_message (_("'%s':\nNo image width specified"),
                 gimp_file_get_utf8_name (file));
      fclose (ifp);
      return NULL;
    }

  if (sunhdr.l_ras_width > GIMP_MAX_IMAGE_SIZE)
    {
      g_message (_("'%s':\nImage width is larger than GIMP can handle"),
                 gimp_file_get_utf8_name (file));
      fclose (ifp);
      return NULL;
    }

  if (sunhdr.l_ras_height <= 0)
    {
      g_message (_("'%s':\nNo image height specified"),
                 gimp_file_get_utf8_name (file));
      fclose (ifp);
      return NULL;
    }

  if (sunhdr.l_ras_height > GIMP_MAX_IMAGE_SIZE)
    {
      g_message (_("'%s':\nImage height is larger than GIMP can handle"),
                 gimp_file_get_utf8_name (file));
      fclose (ifp);
      return NULL;
    }

  switch (sunhdr.l_ras_depth)
    {
    case 1:    /* bitmap */
      image = load_sun_d1 (file, ifp, &sunhdr, suncolmap);
      break;

    case 8:    /* 256 colors */
      image = load_sun_d8 (file, ifp, &sunhdr, suncolmap);
      break;

    case 24:   /* True color */
      image = load_sun_d24 (file, ifp, &sunhdr, suncolmap);
      break;

    case 32:   /* True color with extra byte */
      image = load_sun_d32 (file, ifp, &sunhdr, suncolmap);
      break;

    default:
      image = NULL;
      break;
    }
  gimp_progress_update (1.0);

  fclose (ifp);

  g_free (suncolmap);

  if (! image)
    {
      g_message (_("This image depth is not supported"));
      return NULL;
    }

  return image;
}

static gboolean
export_image (GFile         *file,
              GimpImage     *image,
              GimpDrawable  *drawable,
              GObject       *config,
              GError       **error)
{
  FILE          *ofp;
  GimpImageType  drawable_type;
  gint           rle;
  gboolean       retval;

  drawable_type = gimp_drawable_type (drawable);

  rle = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                             "rle");

  /*  Make sure we're not exporting an image with an alpha channel  */
  if (gimp_drawable_has_alpha (drawable))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("SUNRAS export cannot handle images with alpha channels"));
      return FALSE;
    }

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
    case GIMP_GRAY_IMAGE:
    case GIMP_RGB_IMAGE:
      break;
    default:
      g_message (_("Can't operate on unknown image types"));
      return FALSE;
      break;
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  /* Open the output file. */
  ofp = g_fopen (g_file_peek_path (file), "wb");

  if (! ofp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  if (drawable_type == GIMP_INDEXED_IMAGE)
    {
      retval = save_index (ofp, image, drawable, FALSE, rle);
    }
  else if (drawable_type == GIMP_GRAY_IMAGE)
    {
      retval = save_index (ofp, image, drawable, TRUE, rle);
    }
  else if (drawable_type == GIMP_RGB_IMAGE)
    {
      retval = save_rgb (ofp, image, drawable, rle);
    }
  else
    {
      retval = FALSE;
    }

  fclose (ofp);

  return retval;
}

static L_CARD32
read_card32 (FILE *ifp,
             gint *err)
{
  L_CARD32 c;

  if (read_msb_first)
    {
      c = (((L_CARD32)(getc (ifp))) << 24);
      c |= (((L_CARD32)(getc (ifp))) << 16);
      c |= (((L_CARD32)(getc (ifp))) << 8);
      c |= ((L_CARD32)(*err = getc (ifp)));
    }
  else
    {
      c = ((L_CARD32)(getc (ifp)));
      c |= (((L_CARD32)(getc (ifp))) << 8);
      c |= (((L_CARD32)(getc (ifp))) << 16);
      c |= (((L_CARD32)(*err = getc (ifp))) << 24);
    }

  *err = (*err < 0);

  return c;
}


static void
write_card32 (FILE     *ofp,
              L_CARD32  c)
{
  putc ((int)((c >> 24) & 0xff), ofp);
  putc ((int)((c >> 16) & 0xff), ofp);
  putc ((int)((c >> 8) & 0xff), ofp);
  putc ((int)((c) & 0xff), ofp);
}


/* Convert n bytes of 0/1 to a line of bits */
static void
byte2bit (guchar   *byteline,
          gint      width,
          guchar   *bitline,
          gboolean  invert)
{
  guchar bitval;
  guchar rest[8];

  while (width >= 8)
    {
      bitval = 0;
      if (*(byteline++)) bitval |= 0x80;
      if (*(byteline++)) bitval |= 0x40;
      if (*(byteline++)) bitval |= 0x20;
      if (*(byteline++)) bitval |= 0x10;
      if (*(byteline++)) bitval |= 0x08;
      if (*(byteline++)) bitval |= 0x04;
      if (*(byteline++)) bitval |= 0x02;
      if (*(byteline++)) bitval |= 0x01;
      *(bitline++) = invert ? ~bitval : bitval;
      width -= 8;
    }
  if (width > 0)
    {
      memset (rest, 0, 8);
      memcpy (rest, byteline, width);
      bitval = 0;
      byteline = rest;
      if (*(byteline++)) bitval |= 0x80;
      if (*(byteline++)) bitval |= 0x40;
      if (*(byteline++)) bitval |= 0x20;
      if (*(byteline++)) bitval |= 0x10;
      if (*(byteline++)) bitval |= 0x08;
      if (*(byteline++)) bitval |= 0x04;
      if (*(byteline++)) bitval |= 0x02;
      *bitline = invert ? ~bitval : bitval;
    }
}


/* Start reading Runlength Encoded Data */
static void
rle_startread (FILE *ifp)
{
  /* Clear RLE-buffer */
  rlebuf.val = rlebuf.n = 0;
}


/* Read uncompressed elements from RLE-stream */
static gint
rle_fread (gchar *ptr,
           gint   sz,
           gint   nelem,
           FILE  *ifp)
{
  int elem_read, cnt, val, err = 0;

  for (elem_read = 0; elem_read < nelem; elem_read++)
    {
      for (cnt = 0; cnt < sz; cnt++)
        {
          val = rle_getc (ifp);

          if (val < 0)
            {
              err = 1;
              break;
            }

          *(ptr++) = (char)val;
        }

      if (err)
        break;
    }

  return elem_read;
}


/* Get one byte of uncompressed data from RLE-stream */
static gint
rle_fgetc (FILE *ifp)
{
  int flag, runcnt, runval;

  if (rlebuf.n > 0)    /* Something in the buffer ? */
    {
      (rlebuf.n)--;
      return rlebuf.val;
    }

  /* Nothing in the buffer. We have to read something */
  if ((flag = getc (ifp)) < 0) return -1;
  if (flag != 0x0080) return flag;    /* Single byte run ? */

  if ((runcnt = getc (ifp)) < 0) return -1;
  if (runcnt == 0) return 0x0080;     /* Single 0x80 ? */

  /* The run */
  if ((runval = getc (ifp)) < 0) return -1;
  rlebuf.n = runcnt;
  rlebuf.val = runval;

  return runval;
}


/* Start writing Runlength Encoded Data */
static void
rle_startwrite (FILE *ofp)
{
  /* Clear RLE-buffer */
  rlebuf.val = rlebuf.n = 0;
}


/* Write uncompressed elements to RLE-stream */
static gint
rle_fwrite (gchar *ptr,
            gint   sz,
            gint   nelem,
            FILE  *ofp)
{
  int     elem_write, cnt, val, err = 0;
  guchar *pixels = (unsigned char *)ptr;

  for (elem_write = 0; elem_write < nelem; elem_write++)
    {
      for (cnt = 0; cnt < sz; cnt++)
        {
          val = rle_fputc (*(pixels++), ofp);
          if (val < 0)
            {
              err = 1;
              break;
            }
        }

      if (err)
        break;
    }

  return elem_write;
}


/* Write uncompressed character to RLE-stream */
static gint
rle_fputc (gint  val,
           FILE *ofp)
{
  int retval;

  if (rlebuf.n == 0)    /* Nothing in the buffer ? Save the value */
    {
      rlebuf.n   = 1;
      rlebuf.val = val;

      return val;
    }

  /* Something in the buffer */

  if (rlebuf.val == val)   /* Same value in the buffer ? */
    {
      (rlebuf.n)++;
      if (rlebuf.n == 257) /* Can not be encoded in a single run ? */
        {
          retval = rle_putrun (256, rlebuf.val, ofp);
          if (retval < 0)
            return retval;

          rlebuf.n -= 256;
        }

      return val;
    }

  /* Something different in the buffer ? Write out the run */

  retval = rle_putrun (rlebuf.n, rlebuf.val, ofp);
  if (retval < 0)
    return retval;

  /* Save the new value */
  rlebuf.n   = 1;
  rlebuf.val = val;

  return val;
}


/* Write out a run with 0 < n < 257 */
static gint
rle_putrun (gint  n,
            gint  val,
            FILE *ofp)
{
  int retval, flag = 0x80;

  /* Useful to write a 3 byte run ? */
  if ((n > 2) || ((n == 2) && (val == flag)))
    {
      putc (flag, ofp);
      putc (n-1, ofp);
      retval = putc (val, ofp);
    }
  else if (n == 2) /* Write two single runs (could not be value 0x80) */
    {
      putc (val, ofp);
      retval = putc (val, ofp);
    }
  else  /* Write a single run */
    {
      if (val == flag)
        retval = putc (flag, ofp), putc (0x00, ofp);
      else
        retval = putc (val, ofp);
    }

  return (retval < 0) ? retval : val;
}


/* End writing Runlength Encoded Data */
static void
rle_endwrite (FILE *ofp)
{
  if (rlebuf.n > 0)
    {
      rle_putrun (rlebuf.n, rlebuf.val, ofp);
      rlebuf.val = rlebuf.n = 0; /* Clear RLE-buffer */
    }
}


static void
read_sun_header (FILE            *ifp,
                 L_SUNFILEHEADER *sunhdr)
{
  int       j, err;
  L_CARD32 *cp;

  cp = (L_CARD32 *)sunhdr;

  /* Read in all 32-bit values of the header and check for byte order */
  for (j = 0; j < sizeof (L_SUNFILEHEADER) / sizeof(sunhdr->l_ras_magic); j++)
    {
      *(cp++) = read_card32 (ifp, &err);
      if (err)
        break;
    }

  if (err)
    sunhdr->l_ras_magic = 0;  /* Not a valid SUN-raster file */
}


/* Write out a SUN-fileheader */

static void
write_sun_header (FILE            *ofp,
                  L_SUNFILEHEADER *sunhdr)
{
  int       j, hdr_entries;
  L_CARD32 *cp;

  hdr_entries = sizeof (L_SUNFILEHEADER) / sizeof(sunhdr->l_ras_magic);

  cp = (L_CARD32 *)sunhdr;

  /* Write out all 32-bit values of the header and check for byte order */
  for (j = 0; j < hdr_entries; j++)
    {
      write_card32 (ofp, *(cp++));
    }
}


/* Read the sun colormap */

static void
read_sun_cols (FILE            *ifp,
               L_SUNFILEHEADER *sunhdr,
               guchar          *colormap)
{
  int ncols, err = 0;

  /* Read in SUN-raster Colormap */
  ncols = sunhdr->l_ras_maplength / 3;
  if (ncols <= 0)
    err = 1;
  else
    err = (fread (colormap, 3, ncols, ifp) != ncols);

  if (err)
    sunhdr->l_ras_magic = 0;  /* Not a valid SUN-raster file */
}


/* Write a sun colormap */

static void
write_sun_cols (FILE            *ofp,
                L_SUNFILEHEADER *sunhdr,
                guchar          *colormap)
{
  int ncols;

  ncols = sunhdr->l_ras_maplength / 3;
  fwrite (colormap, 3, ncols, ofp);
}


/* Set a GIMP colortable using the sun colormap */

static void
set_color_table (GimpImage       *image,
                 L_SUNFILEHEADER *sunhdr,
                 const guchar    *suncolmap)
{
  guchar ColorMap[256 * 3];
  gint   ncols, j;

  ncols = sunhdr->l_ras_maplength / 3;
  if (ncols <= 0)
    return;

  for (j = 0; j < MIN (ncols, 256); j++)
    {
      ColorMap[j * 3 + 0] = suncolmap[j];
      ColorMap[j * 3 + 1] = suncolmap[j + ncols];
      ColorMap[j * 3 + 2] = suncolmap[j + 2 * ncols];
    }

#ifdef DEBUG
  printf ("Set GIMP colortable:\n");
  for (j = 0; j < ncols; j++)
    printf ("%3d: 0x%02x 0x%02x 0x%02x\n", j,
            ColorMap[j*3], ColorMap[j*3+1], ColorMap[j*3+2]);
#endif

  gimp_image_set_colormap (image, ColorMap, ncols);
}


/* Create an image. Sets layer, drawable and rgn. Returns image */
static GimpImage *
create_new_image (GFile              *file,
                  guint               width,
                  guint               height,
                  GimpImageBaseType   type,
                  GimpLayer         **layer,
                  GeglBuffer        **buffer)
{
  GimpImage     *image;
  GimpImageType  gdtype;

  switch (type)
    {
    case GIMP_RGB:
      gdtype = GIMP_RGB_IMAGE;
      break;
    case GIMP_GRAY:
      gdtype = GIMP_GRAY_IMAGE;
      break;
    case GIMP_INDEXED:
      gdtype = GIMP_INDEXED_IMAGE;
      break;
    default:
      g_warning ("Unsupported image type");
      return NULL;
    }

  image = gimp_image_new (width, height, type);

  *layer = gimp_layer_new (image, _("Background"), width, height,
                           gdtype,
                           100,
                           gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, *layer, NULL, 0);

  *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (*layer));

  return image;
}


/* Load SUN-raster-file with depth 1 */
static GimpImage *
load_sun_d1 (GFile           *file,
             FILE            *ifp,
             L_SUNFILEHEADER *sunhdr,
             guchar          *suncolmap)
{
  int               pix8;
  int               width, height, linepad, scan_lines, tile_height;
  int               i, j;
  guchar           *dest, *data;
  GimpImage        *image;
  GimpLayer        *layer;
  GeglBuffer       *buffer;
  guchar            bit2byte[256 * 8];
  L_SUNFILEHEADER   sun_bwhdr;
  guchar            sun_bwcolmap[6] = { 255,0,255,0,255,0 };
  int               err = 0;
  gboolean          rle = (sunhdr->l_ras_type == RAS_TYPE_RLE);

  width = sunhdr->l_ras_width;
  height = sunhdr->l_ras_height;

  image = create_new_image (file, width, height, GIMP_INDEXED,
                            &layer, &buffer);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width);

  if (suncolmap != NULL)   /* Set up the specified color map */
    {
      set_color_table (image, sunhdr, suncolmap);
    }
  else   /* No colormap available. Set up a dummy b/w-colormap */
    {      /* Copy the original header and simulate b/w-colormap */
      memcpy ((char *)&sun_bwhdr,(char *)sunhdr,sizeof (L_SUNFILEHEADER));
      sun_bwhdr.l_ras_maptype = 2;
      sun_bwhdr.l_ras_maplength = 6;
      set_color_table (image, &sun_bwhdr, sun_bwcolmap);
    }

  /* Get an array for mapping 8 bits in a byte to 8 bytes */
  dest = bit2byte;
  for (j = 0; j < 256; j++)
    for (i = 7; i >= 0; i--)
      *(dest++) = ((j & (1 << i)) != 0);

  linepad = (((sunhdr->l_ras_width+7)/8) % 2); /* Check for 16bit align */

  if (rle)
    rle_startread (ifp);

  dest = data;
  scan_lines = 0;

  for (i = 0; i < height; i++)
    {
      j = width;
      while (j >= 8)
        {
          pix8 = rle ? rle_getc (ifp) : getc (ifp);
          if (pix8 < 0) { err = 1; pix8 = 0; }

          memcpy (dest, bit2byte + pix8*8, 8);
          dest += 8;
          j -= 8;
        }

      if (j > 0)
        {
          pix8 = rle ? rle_getc (ifp) : getc (ifp);
          if (pix8 < 0) { err = 1; pix8 = 0; }

          memcpy (dest, bit2byte + pix8*8, j);
          dest += j;
        }

      if (linepad)
        err |= ((rle ? rle_getc (ifp) : getc (ifp)) < 0);

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

  if (err)
    g_message (_("EOF encountered on reading"));

  g_object_unref (buffer);

  return image;
}


/* Load SUN-raster-file with depth 8 */

static GimpImage *
load_sun_d8 (GFile           *file,
             FILE            *ifp,
             L_SUNFILEHEADER *sunhdr,
             guchar          *suncolmap)
{
  int         width, height, linepad, i, j;
  gboolean    grayscale;
  gint        ncols;
  int         scan_lines, tile_height;
  guchar     *dest, *data;
  GimpImage  *image;
  GimpLayer  *layer;
  GeglBuffer *buffer;
  int         err = 0;
  gboolean    rle = (sunhdr->l_ras_type == RAS_TYPE_RLE);

  width  = sunhdr->l_ras_width;
  height = sunhdr->l_ras_height;

  /* This could also be a grayscale image. Check it */
  ncols = sunhdr->l_ras_maplength / 3;

  grayscale = TRUE;  /* Also grayscale if no colormap present */

  if ((ncols > 0) && (suncolmap != NULL))
    {
      for (j = 0; j < ncols; j++)
        {
          if ((suncolmap[j]             != j) ||
              (suncolmap[j + ncols]     != j) ||
              (suncolmap[j + 2 * ncols] != j))
            {
              grayscale = FALSE;
              break;
            }
        }
    }

  image = create_new_image (file, width, height,
                            grayscale ? GIMP_GRAY : GIMP_INDEXED,
                            &layer, &buffer);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width);

  if (!grayscale)
    set_color_table (image, sunhdr, suncolmap);

  linepad = (sunhdr->l_ras_width % 2);

  if (rle)
    rle_startread (ifp);  /* Initialize RLE-buffer */

  dest = data;
  scan_lines = 0;

  for (i = 0; i < height; i++)
    {
      memset ((char *)dest, 0, width);
      err |= ((rle ? rle_fread ((char *)dest, 1, width, ifp)
               : fread ((char *)dest, 1, width, ifp)) != width);

      if (linepad)
        err |= ((rle ? rle_getc (ifp) : getc (ifp)) < 0);

      dest += width;
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

  if (err)
    g_message (_("EOF encountered on reading"));

  g_object_unref (buffer);

  return image;
}


/* Load SUN-raster-file with depth 24 */
static GimpImage *
load_sun_d24 (GFile            *file,
              FILE             *ifp,
              L_SUNFILEHEADER  *sunhdr,
              guchar           *suncolmap)
{
  guchar     *dest, blue;
  guchar     *data;
  int         width, height, linepad, tile_height, scan_lines;
  int         i, j;
  GimpImage  *image;
  GimpLayer  *layer;
  GeglBuffer *buffer;
  int         err = 0;
  gboolean    rle = (sunhdr->l_ras_type == RAS_TYPE_RLE);

  width  = sunhdr->l_ras_width;
  height = sunhdr->l_ras_height;

  image = create_new_image (file, width, height, GIMP_RGB,
                            &layer, &buffer);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width * 3);

  linepad = ((sunhdr->l_ras_width*3) % 2);

  if (rle)
    rle_startread (ifp);  /* Initialize RLE-buffer */

  dest = data;
  scan_lines = 0;

  for (i = 0; i < height; i++)
    {
      memset ((char *)dest, 0, 3*width);
      err |= ((rle ? rle_fread ((char *)dest, 3, width, ifp)
               : fread ((char *)dest, 3, width, ifp)) != width);

      if (linepad)
        err |= ((rle ? rle_getc (ifp) : getc (ifp)) < 0);

      if (sunhdr->l_ras_type == 3) /* RGB-format ? That is what GIMP wants */
        {
          dest += width * 3;
        }
      else                         /* We have BGR format. Correct it */
        {
          for (j = 0; j < width; j++)
            {
              blue = *dest;
              *dest = *(dest+2);
              *(dest+2) = blue;
              dest += 3;
            }
        }

      scan_lines++;

      if ((i % 20) == 0)
        gimp_progress_update ((double)(i + 1) / (double)height);

      if ((scan_lines == tile_height) || ((i + 1) == height))
        {
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i - scan_lines + 1,
                                                   width, scan_lines), 0,
                           NULL, data, GEGL_AUTO_ROWSTRIDE);
          scan_lines = 0;
          dest = data;
        }
    }

  g_free (data);

  if (err)
    g_message (_("EOF encountered on reading"));

  g_object_unref (buffer);

  return image;
}


/* Load SUN-raster-file with depth 32 */

static GimpImage *
load_sun_d32 (GFile           *file,
              FILE            *ifp,
              L_SUNFILEHEADER *sunhdr,
              guchar          *suncolmap)
{
  guchar     *dest, blue;
  guchar     *data;
  int         width, height, tile_height, scan_lines;
  int         i, j;
  GimpImage  *image;
  GimpLayer  *layer;
  GeglBuffer *buffer;
  int         err = 0;
  int         cerr;
  gboolean    rle = (sunhdr->l_ras_type == RAS_TYPE_RLE);

  width  = sunhdr->l_ras_width;
  height = sunhdr->l_ras_height;

  /* initialize */

  cerr = 0;

  image = create_new_image (file, width, height, GIMP_RGB,
                            &layer, &buffer);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width * 3);

  if (rle)
    rle_startread (ifp);  /* Initialize RLE-buffer */

  dest = data;
  scan_lines = 0;

  for (i = 0; i < height; i++)
    {
      if (rle)
        {
          for (j = 0; j < width; j++)
            {
              rle_getc (ifp);   /* Skip unused byte */
              *(dest++) = rle_getc (ifp);
              *(dest++) = rle_getc (ifp);
              *(dest++) = (cerr = (rle_getc (ifp)));
            }
        }
      else
        {
          for (j = 0; j < width; j++)
            {
              getc (ifp);   /* Skip unused byte */
              *(dest++) = getc (ifp);
              *(dest++) = getc (ifp);
              *(dest++) = (cerr = (getc (ifp)));
            }
        }
      err |= (cerr < 0);

      if (sunhdr->l_ras_type != 3) /* BGR format ? Correct it */
        {
          for (j = 0; j < width; j++)
            {
              dest -= 3;
              blue = *dest;
              *dest = *(dest+2);
              *(dest+2) = blue;
            }
          dest += width*3;
        }

      scan_lines++;

      if ((i % 20) == 0)
        gimp_progress_update ((double)(i + 1) / (double)height);

      if ((scan_lines == tile_height) || ((i + 1) == height))
        {
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i - scan_lines + 1,
                                                   width, scan_lines), 0,
                           NULL, data, GEGL_AUTO_ROWSTRIDE);
          scan_lines = 0;
          dest = data;
        }
    }

  g_free (data);

  if (err)
    g_message (_("EOF encountered on reading"));

  g_object_unref (buffer);

  return image;
}


static gboolean
save_index (FILE         *ofp,
            GimpImage    *image,
            GimpDrawable *drawable,
            gboolean      grey,
            gint          rle)
{
  int             height, width, linepad, i, j;
  int             ncols, bw, is_bw, is_wb, bpl;
  int             tile_height;
  long            tmp = 0;
  guchar         *cmap, *bwline = NULL;
  guchar         *data, *src;
  L_SUNFILEHEADER sunhdr;
  guchar          sun_colormap[256*3];
  static guchar   sun_bwmap[6] = { 0,   255, 0,   255, 0,   255 };
  static guchar   sun_wbmap[6] = { 255, 0,   255, 0,   255, 0   };
  unsigned char  *suncolmap = sun_colormap;
  GeglBuffer     *buffer;
  const Babl     *format;
  WRITE_FUN      *write_fun;

  buffer = gimp_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  tile_height = gimp_tile_height ();

  if (grey)
    format = babl_format ("Y' u8");
  else
    format = gegl_buffer_get_format (buffer);

  /* allocate a buffer for retrieving information from the buffer  */
  src = data = g_malloc (tile_height * width *
                         babl_format_get_bytes_per_pixel (format));

  /* Fill SUN-color map */
  if (grey)
    {
      ncols = 256;

      for (j = 0; j < ncols; j++)
        {
          suncolmap[j]             = j;
          suncolmap[j + ncols]     = j;
          suncolmap[j + ncols * 2] = j;
        }
    }
  else
    {
      cmap = gimp_image_get_colormap (image, NULL, &ncols);

      for (j = 0; j < ncols; j++)
        {
          suncolmap[j]             = *(cmap++);
          suncolmap[j + ncols]     = *(cmap++);
          suncolmap[j + ncols * 2] = *(cmap++);
        }
    }

  bw = (ncols == 2);   /* Maybe this is a two-color image */
  if (bw)
    {
      bwline = g_malloc ((width + 7) / 8);
      if (bwline == NULL)
        bw = 0;
    }

  is_bw = is_wb = 0;
  if (bw)    /* The Sun-OS imagetool generates index 0 for white and */
    {          /* index 1 for black. Do the same without colortable. */
      is_bw = (memcmp (suncolmap, sun_bwmap, 6) == 0);
      is_wb = (memcmp (suncolmap, sun_wbmap, 6) == 0);
    }

  /* Number of data bytes per line */
  bpl = bw ?  (width+7)/8 : width;
  linepad = bpl % 2;

  /* Fill in the SUN header */
  sunhdr.l_ras_magic  = RAS_MAGIC;
  sunhdr.l_ras_width  = width;
  sunhdr.l_ras_height = height;
  sunhdr.l_ras_depth  = bw ? 1 : 8;
  sunhdr.l_ras_length = (bpl + linepad) * height;
  sunhdr.l_ras_type   = rle ? RAS_TYPE_RLE : RAS_TYPE_STD;

  if (is_bw || is_wb)   /* No colortable for real b/w images */
    {
      sunhdr.l_ras_maptype = 0;   /* No colormap */
      sunhdr.l_ras_maplength = 0; /* Length of colormap */
    }
  else
    {
      sunhdr.l_ras_maptype = 1;   /* RGB colormap */
      sunhdr.l_ras_maplength = ncols*3; /* Length of colormap */
    }

  write_sun_header (ofp, &sunhdr);

  if (sunhdr.l_ras_maplength > 0)
    write_sun_cols (ofp, &sunhdr, suncolmap);

#define GET_INDEX_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines), 1.0, \
                     format, begin, \
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE); \
    src = begin; }

  if (rle)
    {
      write_fun = (WRITE_FUN *) &rle_fwrite;
      rle_startwrite (ofp);
    }
  else
    {
      write_fun = (WRITE_FUN *) &my_fwrite;
    }

  if (bw)  /* Two color image */
    {
      for (i = 0; i < height; i++)
        {
          if ((i % tile_height) == 0)
            GET_INDEX_TILE (data); /* Get more data */

          byte2bit (src, width, bwline, is_bw);
          (*write_fun) (bwline, bpl, 1, ofp);
          if (linepad)
            (*write_fun) ((char *)&tmp, linepad, 1, ofp);
          src += width;

          if ((i % 20) == 0)
            gimp_progress_update ((double) i / (double) height);
        }
    }
  else   /* Color or grey-image */
    {
      for (i = 0; i < height; i++)
        {
          if ((i % tile_height) == 0)
            GET_INDEX_TILE (data); /* Get more data */

          (*write_fun) ((char *)src, width, 1, ofp);
          if (linepad)
            (*write_fun) ((char *)&tmp, linepad, 1, ofp);
          src += width;

          if ((i % 20) == 0)
            gimp_progress_update ((double) i / (double) height);
        }
    }

#undef GET_INDEX_TILE

  if (rle)
    rle_endwrite (ofp);

  g_free (data);

  if (bwline)
    g_free (bwline);

  g_object_unref (buffer);

  if (ferror (ofp))
    {
      g_message (_("Write error occurred"));
      return FALSE;
    }

  return TRUE;
}


static gboolean
save_rgb (FILE         *ofp,
          GimpImage    *image,
          GimpDrawable *drawable,
          gint          rle)
{
  int              height, width, tile_height, linepad;
  int              i, j, bpp;
  guchar          *data, *src;
  L_SUNFILEHEADER  sunhdr;
  GeglBuffer      *buffer;
  const Babl      *format;

  buffer = gimp_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  tile_height = gimp_tile_height ();

  format = babl_format ("R'G'B' u8");

  /* allocate a buffer for retrieving information from the pixel region  */
  src = data = g_malloc (tile_height * width *
                         babl_format_get_bytes_per_pixel (format));

/* #define SUNRAS_32 */
#ifdef SUNRAS_32
  bpp = 4;
#else
  bpp = 3;
#endif
  linepad = (width * bpp) % 2;

  /* Fill in the SUN header */
  sunhdr.l_ras_magic     = RAS_MAGIC;
  sunhdr.l_ras_width     = width;
  sunhdr.l_ras_height    = height;
  sunhdr.l_ras_depth     = 8 * bpp;
  sunhdr.l_ras_length    = (width * bpp + linepad) * height;
  sunhdr.l_ras_type      = rle ? RAS_TYPE_RLE : RAS_TYPE_STD;
  sunhdr.l_ras_maptype   = 0;   /* No colormap */
  sunhdr.l_ras_maplength = 0; /* Length of colormap */

  write_sun_header (ofp, &sunhdr);

#define GET_RGB_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scan_lines), 1.0, \
                     format, begin, \
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE); \
    src = begin; }

  if (! rle)
    {
      for (i = 0; i < height; i++)
        {
          if ((i % tile_height) == 0)
            GET_RGB_TILE (data); /* Get more data */

          for (j = 0; j < width; j++)
            {
              if (bpp == 4) putc (0, ofp);   /* Dummy */
              putc (*(src + 2), ofp);        /* Blue */
              putc (*(src + 1), ofp);        /* Green */
              putc (*src,       ofp);        /* Red */
              src += 3;
            }

          for (j = 0; j < linepad; j++)
            putc (0, ofp);

          if ((i % 20) == 0)
            gimp_progress_update ((double) i / (double) height);
        }
    }
  else  /* Write runlength encoded */
    {
      rle_startwrite (ofp);

      for (i = 0; i < height; i++)
        {
          if ((i % tile_height) == 0)
            GET_RGB_TILE (data); /* Get more data */

          for (j = 0; j < width; j++)
            {
              if (bpp == 4) rle_putc (0, ofp);   /* Dummy */
              rle_putc (*(src + 2), ofp);        /* Blue */
              rle_putc (*(src + 1), ofp);        /* Green */
              rle_putc (*src,       ofp);        /* Red */
              src += 3;
            }

          for (j = 0; j < linepad; j++)
            rle_putc (0, ofp);

          if ((i % 20) == 0)
            gimp_progress_update ((double) i / (double) height);
        }

      rle_endwrite (ofp);
    }

#undef GET_RGB_TILE

  g_free (data);

  g_object_unref (buffer);

  if (ferror (ofp))
    {
      g_message (_("Write error occurred"));
      return FALSE;
    }

  return TRUE;
}


/*  Save interface functions  */

static gboolean
save_dialog (GimpImage     *image,
             GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  gboolean   run;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                    "rle", GIMP_TYPE_INT_RADIO_FRAME);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static int
my_fwrite (void *ptr,
           int   size,
           int   nmemb,
           FILE *stream)
{
  return fwrite (ptr, size, nmemb, stream);
}
