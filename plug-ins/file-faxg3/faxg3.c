/* This is a plugin for GIMP.
 *
 * Copyright (C) 1997 Jochen Friedrich
 * Parts Copyright (C) 1995 Gert Doering
 * Parts Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <errno.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <fcntl.h>

#include <glib.h> /* For G_OS_WIN32 */
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <libgimp/gimp.h>

#include "g3.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC "file-faxg3-load"
#define VERSION   "0.6"


typedef struct _Faxg3      Faxg3;
typedef struct _Faxg3Class Faxg3Class;

struct _Faxg3
{
  GimpPlugIn      parent_instance;
};

struct _Faxg3Class
{
  GimpPlugInClass parent_class;
};


#define FAXG3_TYPE  (faxg3_get_type ())
#define FAXG3 (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FAXG3_TYPE, Faxg3))

GType                   faxg3_get_type         (void) G_GNUC_CONST;

static GList          * faxg3_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * faxg3_create_procedure (GimpPlugIn            *plug_in,
                                                const gchar           *name);

static GimpValueArray * faxg3_load             (GimpProcedure         *procedure,
                                                GimpRunMode            run_mode,
                                                GFile                 *file,
                                                GimpMetadata          *metadata,
                                                GimpMetadataLoadFlags *flags,
                                                GimpProcedureConfig   *config,
                                                gpointer               run_data);

static GimpImage      * load_image             (GFile                 *file,
                                                GError               **error);

static GimpImage      *  emitgimp              (gint                   hcol,
                                                gint                   row,
                                                const gchar           *bitmap,
                                                gint                   bperrow,
                                                GFile                 *file,
                                                GError               **error);


G_DEFINE_TYPE (Faxg3, faxg3, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FAXG3_TYPE)
DEFINE_STD_SET_I18N


static void
faxg3_class_init (Faxg3Class *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = faxg3_query_procedures;
  plug_in_class->create_procedure = faxg3_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
faxg3_init (Faxg3 *faxg3)
{
}

static GList *
faxg3_query_procedures (GimpPlugIn *plug_in)
{
  return  g_list_append (NULL, g_strdup (LOAD_PROC));
}

static GimpProcedure *
faxg3_create_procedure (GimpPlugIn  *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           faxg3_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("G3 fax image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads g3 fax files",
                                        "This plug-in loads Fax G3 Image files.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Jochen Friedrich",
                                      "Jochen Friedrich, Gert Doering, "
                                      "Spencer Kimball & Peter Mattis",
                                      NULL);

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/g3-fax");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "g3");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "4,string,Research");
    }

  return procedure;
}

static GimpValueArray *
faxg3_load (GimpProcedure         *procedure,
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

#ifdef DEBUG
void
putbin (unsigned long d)
{
  unsigned long i = 0x80000000;

  while (i != 0)
    {
      putc((d & i) ? '1' : '0', stderr);
      i >>= 1;
    }
  putc('\n', stderr);
}
#endif

static int byte_tab[256];
/* static int o_stretch; */             /* -stretch: double each line */
/* static int o_stretch_force=-1; */    /* -1: guess from filename */
/* static int o_lj; */                  /* -l: LJ output */
/* static int o_turn; */                /* -t: turn 90 degrees right */

struct g3_tree * black, * white;

#define CHUNK 2048;
static  char rbuf[2048];        /* read buffer */
static  int  rp;                /* read pointer */
static  int  rs;                /* read buffer size */

#define MAX_ROWS 4300
#define MAX_COLS 1728           /* !! FIXME - command line parameter */


static GimpImage *
load_image (GFile   *file,
            GError **error)
{
  int             data;
  int             hibit;
  struct g3_tree *p;
  int             nr_pels;
  int             fd;
  int             color;
  int             i, rr, rsize;
  int             cons_eol;
  int             last_eol_row;

  GimpImage      *image   = NULL;
  gint            bperrow = MAX_COLS/8;  /* bytes per bit row */
  gchar          *bitmap;                /* MAX_ROWS by (bperrow) bytes */
  gchar          *bp;                    /* bitmap pointer */
  gint            row;
  gint            max_rows;              /* max. rows allocated */
  gint            col, hcol;             /* column, highest column ever used */

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  /* initialize lookup trees */
  build_tree (&white, t_white);
  build_tree (&white, m_white);
  build_tree (&black, t_black);
  build_tree (&black, m_black);

  init_byte_tab (0, byte_tab);

  fd = g_open (g_file_peek_path (file), O_RDONLY | _O_BINARY, 0);

  if (fd < 0)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  hibit = 0;
  data = 0;

  cons_eol = 0; /* consecutive EOLs read - zero yet */
  last_eol_row = 0;

  color = 0; /* start with white */
  rr = 0;

  rsize = lseek (fd, 0L, SEEK_END);
  lseek (fd, 0L, 0);

  rs = read (fd, rbuf, sizeof (rbuf));
  if (rs < 0)
    {
      perror ("read");
      close (fd);
      gimp_quit ();
    }

  rr += rs;
  gimp_progress_update ((float) rr / rsize / 2.0);

                        /* skip GhostScript header */
  rp = (rs >= 64 && strcmp (rbuf + 1, "PC Research, Inc") == 0) ? 64 : 0;

  /* initialize bitmap */

  row = col = hcol = 0;

  bitmap = g_new0 (gchar, (max_rows = MAX_ROWS) * MAX_COLS / 8);
  if (! bitmap)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not create buffer to process image data."));
      return NULL;
    }

  bp = &bitmap[row * MAX_COLS / 8];

  while (rs > 0 && cons_eol < 10)        /* i.e., while (!EOF) */
    {
#ifdef DEBUG
      g_printerr ("hibit=%2d, data=", hibit);
      putbin (data);
#endif

      while (hibit < 20)
        {
          data |= (byte_tab[(int) (unsigned char) rbuf[rp++]] << hibit);
          hibit += 8;

          if (rp >= rs)
            {
              rs = read (fd, rbuf, sizeof (rbuf));
              if (rs < 0)
                { perror ("read2");
                  break;
                }
              rr += rs;
              gimp_progress_update ((float) rr / rsize / 2.0);
              rp = 0;
              if (rs == 0)
                goto do_write;
            }

#ifdef DEBUG
          g_printerr ("hibit=%2d, data=", hibit);
          putbin (data);
#endif
        }

      if (color == 0) /* white */
        p = white->nextb[data & BITM];
      else /* black */
        p = black->nextb[data & BITM];

      while (p != NULL && ! (p->nr_bits))
        {
          data >>= FBITS;
          hibit -= FBITS;
          p = p->nextb[data & BITM];
        }

      if (p == NULL) /* invalid code */
        {
          g_printerr ("invalid code, row=%d, col=%d, file offset=%lx, skip to eol\n",
                      row, col, (unsigned long) lseek (fd, 0, 1) - rs + rp);

          while ((data & 0x03f) != 0)
            {
              data >>= 1; hibit--;

              if ( hibit < 20 )
                {
                  data |= (byte_tab[(int) (unsigned char) rbuf[rp++]] << hibit);
                  hibit += 8;

                  if (rp >= rs) /* buffer underrun */
                    {
                      rs = read (fd, rbuf, sizeof (rbuf));

                      if (rs < 0)
                        { perror ("read4");
                          break;
                        }

                      rr += rs;
                      gimp_progress_update ((float) rr / rsize / 2.0);
                      rp = 0;
                      if (rs == 0)
                        goto do_write;
                    }
                }
            }
          nr_pels = -1;         /* handle as if eol */
        }
      else /* p != NULL <-> valid code */
        {
          data >>= p->nr_bits;
          hibit -= p->nr_bits;

          nr_pels = ((struct g3_leaf *) p)->nr_pels;
#ifdef DEBUG
          g_printerr ("PELs: %d (%c)\n", nr_pels, '0' + color);
#endif
        }

        /* handle EOL (including fill bits) */
      if (nr_pels == -1)
        {
#ifdef DEBUG
          g_printerr ("hibit=%2d, data=", hibit);
          putbin (data);
#endif
          /* skip filler 0bits -> seek for "1"-bit */
          while ((data & 0x01) != 1)
            {
              if ((data & 0xf) == 0) /* nibble optimization */
                {
                  hibit-= 4;
                  data >>= 4;
                }
              else
                {
                  hibit--;
                  data >>= 1;
                }

              /* fill higher bits */
              if (hibit < 20)
                {
                  data |= ( byte_tab[(int) (unsigned char) rbuf[ rp++]] << hibit);
                  hibit += 8;

                  if (rp >= rs) /* buffer underrun */
                    {
                      rs = read (fd, rbuf, sizeof (rbuf));
                      if ( rs < 0 )
                        {
                          perror ("read3");
                          break;
                        }
                      rr += rs;
                      gimp_progress_update ((float) rr / rsize / 2.0);
                      rp = 0;
                      if (rs == 0)
                        goto do_write;
                    }
                }
#ifdef DEBUG
              g_printerr ("hibit=%2d, data=", hibit );
              putbin(data);
#endif
            } /* end skip 0bits */
          hibit--;
          data >>=1;

          color = 0;

          if (col == 0)
            {
              if (last_eol_row != row)
                {
                  cons_eol++; /* consecutive EOLs */
                  last_eol_row = row;
                }
            }
          else
            {
              if (col > hcol && col <= MAX_COLS)
                hcol = col;
              row++;

              /* bitmap memory full? make it larger! */
              if (row >= max_rows)
                {
                  gchar *p = g_try_realloc (bitmap,
                                            (max_rows += 500) * MAX_COLS / 8);
                  if (p == NULL)
                    {
                      perror ("realloc() failed, page truncated");
                      rs = 0;
                    }
                  else
                    {
                      bitmap = p;
                      memset (&bitmap[ row * MAX_COLS / 8 ], 0,
                              (max_rows - row) * MAX_COLS / 8);
                    }
                }

              col=0; bp = &bitmap[row * MAX_COLS / 8];
              cons_eol = 0;
            }
        }
      else /* not eol */
        {
          if (col + nr_pels > MAX_COLS)
            nr_pels = MAX_COLS - col;

          if (color == 0) /* white */
            {
              col += nr_pels;
            }
          else /* black */
            {
              register int bit = (0x80 >> (col & 07));
              register char *w = & bp[col >> 3];

              for (i = nr_pels; i > 0; i--)
                {
                  *w |= bit;
                  bit >>=1;
                  if (bit == 0)
                    {
                      bit = 0x80;
                      w++;
                    }
                  col++;
                }
            }

          if (nr_pels < 64)
            color = !color; /* terminating code */
        }
    } /* end main loop */

 do_write: /* write pbm (or whatever) file */

  if (fd != 0)
    close (fd); /* close input file */

#ifdef DEBUG
  g_printerr ("consecutive EOLs: %d, max columns: %d\n", cons_eol, hcol);
#endif

  image = emitgimp (hcol, row, bitmap, bperrow, file, error);

  g_free (bitmap);

  return image;
}

/* hcol is the number of columns, row the number of rows
 * bperrow is the number of bytes actually used by hcol, which may
 * be greater than (hcol+7)/8 [in case of an unscaled g3 image less
 * than 1728 pixels wide]
 */

static GimpImage *
emitgimp (gint         hcol,
          gint         row,
          const gchar *bitmap,
          gint         bperrow,
          GFile       *file,
          GError     **error)
{
  GeglBuffer *buffer;
  GimpImage  *image;
  GimpLayer  *layer;
  guchar     *buf;
  guchar      tmp;
  gint        x, y;
  gint        xx, yy;
  gint        tile_height;

  /* initialize */

  tmp = 0;

#ifdef DEBUG
  g_printerr ("emit gimp: %d x %d\n", hcol, row);
#endif

  if (hcol > GIMP_MAX_IMAGE_SIZE || hcol <= 0 ||
      row > GIMP_MAX_IMAGE_SIZE || row <= 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Invalid image dimensions (%d x %d). "
                     "Image may be corrupt."),
                   hcol, row);
      return NULL;
    }

  image = gimp_image_new (hcol, row, GIMP_GRAY);
  if (! image)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not create image."));
      return NULL;
    }

  layer = gimp_layer_new (image, _("Background"),
                          hcol,
                          row,
                          GIMP_GRAY_IMAGE,
                          100,
                          gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, NULL, 0);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  tile_height = gimp_tile_height ();
#ifdef DEBUG
  g_printerr ("tile height: %d\n", tile_height);
#endif

  buf = g_new (guchar, hcol * tile_height);
  if (! buf)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not create buffer to process image data."));
      g_object_unref (buffer);
      gimp_image_delete(image);
      return NULL;
    }

  xx = 0;
  yy = 0;

  for (y = 0; y < row; y++)
    {
      for (x = 0; x < hcol; x++)
        {
          if ((x & 7) == 0)
            tmp = bitmap[y * bperrow + (x >> 3)];

          buf[xx++] = tmp&(128 >> (x & 7)) ? 0 : 255;
        }

      if ((y - yy) == tile_height - 1)
        {
#ifdef DEBUG
          g_printerr ("update tile height: %d\n", tile_height);
#endif

          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, yy, hcol, tile_height), 0,
                           NULL, buf, GEGL_AUTO_ROWSTRIDE);

          gimp_progress_update (0.5 + (float) y / row / 2.0);

          xx = 0;
          yy += tile_height;
        }
    }

  if (row - yy)
    {
#ifdef DEBUG
      g_printerr ("update rest: %d\n", row-yy);
#endif

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, yy, hcol, row - yy), 0,
                       NULL, buf, GEGL_AUTO_ROWSTRIDE);
    }

  gimp_progress_update (1.0);

  g_free (buf);

  g_object_unref (buffer);

  return image;
}
