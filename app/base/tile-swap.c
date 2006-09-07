/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#include <process.h>
#define getpid _getpid
#endif

#include "base-types.h"

#ifndef _O_BINARY
#define _O_BINARY 0
#endif
#ifndef _O_TEMPORARY
#define _O_TEMPORARY 0
#endif

#include "tile.h"
#include "tile-private.h"
#include "tile-swap.h"

#include "gimp-intl.h"

#define MAX_OPEN_SWAP_FILES  16

typedef struct _SwapFile      SwapFile;
typedef struct _DefSwapFile   DefSwapFile;
typedef struct _Gap           Gap;
typedef struct _AsyncSwapArgs AsyncSwapArgs;

struct _SwapFile
{
  gchar    *filename;
  gint      swap_num;
  SwapFunc  swap_func;
  gpointer  user_data;
  gint      fd;
};

struct _DefSwapFile
{
  GList *gaps;
  off_t  swap_file_end;
  off_t  cur_position;
};

struct _Gap
{
  off_t start;
  off_t end;
};

struct _AsyncSwapArgs
{
  DefSwapFile *def_swap_file;
  gint         fd;
  Tile        *tile;
};


static guint   tile_swap_hash             (gint        *key);
static gint    tile_swap_compare          (gint        *a,
                                           gint        *b);
static void    tile_swap_command          (Tile        *tile,
                                           gint         command);
static void    tile_swap_open             (SwapFile    *swap_file);

static gint    tile_swap_default          (gint         fd,
                                           Tile        *tile,
                                           gint         cmd,
                                           gpointer     user_data);
static void    tile_swap_default_in       (DefSwapFile *def_swap_file,
                                           gint         fd,
                                           Tile        *tile);
static void    tile_swap_default_in_async (DefSwapFile *def_swap_file,
                                           gint         fd,
                                           Tile        *tile);
static void    tile_swap_default_out      (DefSwapFile *def_swap_file,
                                           gint         fd,
                                           Tile        *tile);
static void    tile_swap_default_delete   (DefSwapFile *def_swap_file,
                                           gint         fd,
                                           Tile        *tile);
static off_t   tile_swap_find_offset      (DefSwapFile *def_swap_file,
                                           gint         fd,
                                           off_t        bytes);
static void    tile_swap_resize           (DefSwapFile *def_swap_file,
                                           gint         fd,
                                           off_t        new_size);
static Gap   * tile_swap_gap_new          (off_t        start,
                                           off_t        end);
static void    tile_swap_gap_destroy      (Gap         *gap);


static gboolean      initialized      = FALSE;
static GHashTable  * swap_files       = NULL;
static GList       * open_swap_files  = NULL;
static gint          nopen_swap_files = 0;
static gint          next_swap_num    = 1;
static const off_t   swap_file_grow   = 1024 * TILE_WIDTH * TILE_HEIGHT * 4;

static gboolean      seek_err_msg     = TRUE;
static gboolean      read_err_msg     = TRUE;
static gboolean      write_err_msg    = TRUE;


#ifdef GIMP_UNSTABLE
static void
tile_swap_print_gaps (DefSwapFile *def_swap_file)
{
  GList *gaps;
  Gap   *gap;

  gaps = def_swap_file->gaps;
  while (gaps)
    {
      gap = gaps->data;
      gaps = gaps->next;

      g_print ("  %"G_GINT64_FORMAT" - %"G_GINT64_FORMAT"\n",
               gap->start, gap->end);
    }
}
#endif

static void
tile_swap_exit1 (gpointer key,
                 gpointer value,
                 gpointer data)
{
  extern gint  tile_ref_count;
  SwapFile    *swap_file;
  DefSwapFile *def_swap_file;

  if (tile_ref_count != 0)
    g_warning ("tile ref count balance: %d\n", tile_ref_count);

  swap_file = value;
  if (swap_file->swap_func == tile_swap_default)
    {
      def_swap_file = swap_file->user_data;

#ifdef GIMP_UNSTABLE
      if (def_swap_file->swap_file_end != 0)
        {
          g_warning ("swap file not empty: \"%s\"\n",
                     gimp_filename_to_utf8 (swap_file->filename));
          tile_swap_print_gaps (def_swap_file);
        }
#endif

#ifdef G_OS_WIN32
      /* should close before unlink */
      if (swap_file->fd > 0)
        {
          close (swap_file->fd);
          swap_file->fd = -1;
        }
#endif
      g_unlink (swap_file->filename);
    }
}

void
tile_swap_init (const gchar *path)
{
  gchar *swapfile;
  gchar *swapdir;
  gchar *swappath;

  g_return_if_fail (path != NULL);
  g_return_if_fail (! initialized);

  initialized = TRUE;

  swap_files = g_hash_table_new ((GHashFunc) tile_swap_hash,
                                 (GCompareFunc) tile_swap_compare);

  swapdir  = gimp_config_path_expand (path, TRUE, NULL);
  swapfile = g_strdup_printf ("gimpswap.%lu", (unsigned long) getpid ());

  swappath = g_build_filename (swapdir, swapfile, NULL);

  g_free (swapfile);

  /*  create the swap directory if it doesn't exist */
  if (! g_file_test (swapdir, G_FILE_TEST_EXISTS))
    g_mkdir_with_parents (swapdir,
                          S_IRUSR | S_IXUSR | S_IWUSR |
                          S_IRGRP | S_IXGRP |
                          S_IROTH | S_IXOTH);

  g_free (swapdir);

  tile_swap_add (swappath, NULL, NULL);

  g_free (swappath);
}

void
tile_swap_exit (void)
{
  g_return_if_fail (initialized);

#ifdef HINTS_SANITY
  extern int tile_exist_peak;

  g_printerr ("Tile exist peak was %d Tile structs (%d bytes)",
              tile_exist_peak, tile_exist_peak * sizeof(Tile));
#endif

  if (swap_files)
    g_hash_table_foreach (swap_files, tile_swap_exit1, NULL);
}

gint
tile_swap_add (gchar    *filename,
               SwapFunc  swap_func,
               gpointer  user_data)
{
  SwapFile    *swap_file;
  DefSwapFile *def_swap_file;

  g_return_val_if_fail (initialized, -1);

  swap_file = g_new (SwapFile, 1);
  swap_file->filename = g_strdup (filename);
  swap_file->swap_num = next_swap_num++;

  if (!swap_func)
    {
      swap_func = tile_swap_default;

      def_swap_file = g_new (DefSwapFile, 1);
      def_swap_file->gaps = NULL;
      def_swap_file->swap_file_end = 0;
      def_swap_file->cur_position = 0;

      user_data = def_swap_file;
    }

  swap_file->swap_func = swap_func;
  swap_file->user_data = user_data;
  swap_file->fd = -1;

  g_hash_table_insert (swap_files, &swap_file->swap_num, swap_file);

  return swap_file->swap_num;
}

void
tile_swap_remove (gint swap_num)
{
  SwapFile *swap_file;

  g_return_if_fail (initialized);

  swap_file = g_hash_table_lookup (swap_files, &swap_num);
  if (!swap_file)
    return;

  g_hash_table_remove (swap_files, &swap_num);

  if (swap_file->fd != -1)
    close (swap_file->fd);

  g_free (swap_file);
}

void
tile_swap_in_async (Tile *tile)
{
  if (tile->swap_offset == -1)
    return;

  tile_swap_command (tile, SWAP_IN_ASYNC);
}

void
tile_swap_in (Tile *tile)
{
  if (tile->swap_offset == -1)
    {
      tile_alloc (tile);
      return;
    }

  tile_swap_command (tile, SWAP_IN);
}

void
tile_swap_out (Tile *tile)
{
  tile_swap_command (tile, SWAP_OUT);
}

void
tile_swap_delete (Tile *tile)
{
  tile_swap_command (tile, SWAP_DELETE);
}

void
tile_swap_compress (gint swap_num)
{
  tile_swap_command (NULL, SWAP_COMPRESS);
}

/* check if we can open a swap file */
gboolean
tile_swap_test (void)
{
  SwapFile *swap_file;
  int       swap_num = 1;

  g_return_val_if_fail (initialized, FALSE);

  swap_file = g_hash_table_lookup (swap_files, &swap_num);
  g_assert (swap_file->fd == -1);

  /* make sure this duplicates the open() call from tile_swap_open() */
  swap_file->fd = g_open (swap_file->filename,
                          O_CREAT | O_RDWR | _O_BINARY | _O_TEMPORARY,
                          S_IRUSR | S_IWUSR);

  if (swap_file->fd != -1)
    {
      close (swap_file->fd);
      swap_file->fd = -1;
      g_unlink (swap_file->filename);

      return TRUE;
    }

  return FALSE;
}

static guint
tile_swap_hash (gint *key)
{
  return (guint) *key;
}

static gint
tile_swap_compare (gint *a,
                   gint *b)
{
  return (*a == *b);
}

static void
tile_swap_command (Tile *tile,
                   gint  command)
{
  SwapFile *swap_file;

  g_return_if_fail (initialized);

  do
    {
      swap_file = g_hash_table_lookup (swap_files, &tile->swap_num);
      if (!swap_file)
        {
          g_warning ("could not find swap file for tile");
          return;
        }

      if (swap_file->fd == -1)
        {
          tile_swap_open (swap_file);

          if (swap_file->fd == -1)
            return;
        }
    }
  while ((* swap_file->swap_func) (swap_file->fd,
                                   tile, command, swap_file->user_data));
}

static void
tile_swap_open (SwapFile *swap_file)
{
  SwapFile *tmp;

  if (swap_file->fd != -1)
    return;

  if (nopen_swap_files == MAX_OPEN_SWAP_FILES)
    {
      tmp = open_swap_files->data;
      close (tmp->fd);
      tmp->fd = -1;

      open_swap_files = g_list_remove (open_swap_files, open_swap_files->data);
      nopen_swap_files -= 1;
    }

  /* duplicate this open() call in tile_swap_test() */
  swap_file->fd = g_open (swap_file->filename,
                          O_CREAT | O_RDWR | _O_BINARY | _O_TEMPORARY,
                          S_IRUSR | S_IWUSR);

  if (swap_file->fd == -1)
    {
      g_message (_("Unable to open swap file. The Gimp has run out of memory "
                   "and cannot use the swap file. Some parts of your images "
                   "may be corrupted. Try to save your work using different "
                   "filenames, restart the Gimp and check the location of the "
                   "swap directory in your Preferences."));

      return;
    }

  open_swap_files = g_list_append (open_swap_files, swap_file);
  nopen_swap_files += 1;
}

/* The actual swap file code. The swap file consists of tiles
 *  which have been moved out to disk in order to conserve memory.
 *  The swap file format is free form. Any tile in memory may
 *  end up anywhere on disk.
 * An actual tile in the swap file consists only of the tile data.
 *  The offset of the tile on disk is stored in the tile data structure
 *  in memory.
 */

static int
tile_swap_default (gint      fd,
                   Tile     *tile,
                   gint      cmd,
                   gpointer  user_data)
{
  DefSwapFile *def_swap_file;

  def_swap_file = (DefSwapFile*) user_data;

  switch (cmd)
    {
    case SWAP_IN:
      tile_swap_default_in (def_swap_file, fd, tile);
      break;
    case SWAP_IN_ASYNC:
      tile_swap_default_in_async (def_swap_file, fd, tile);
      break;
    case SWAP_OUT:
      tile_swap_default_out (def_swap_file, fd, tile);
      break;
    case SWAP_DELETE:
      tile_swap_default_delete (def_swap_file, fd, tile);
      break;
    case SWAP_COMPRESS:
      g_warning ("tile_swap_default: SWAP_COMPRESS: UNFINISHED");
      break;
    }

  return FALSE;
}

static void
tile_swap_default_in_async (DefSwapFile *def_swap_file,
                            gint         fd,
                            Tile        *tile)
{
  /* ignore; it's only a hint anyway */
  /* this could be changed to call out to another program that
   * tries to make the OS read the data in from disk.
   */
}

/* NOTE: if you change this function, check to see if your changes
 * apply to tile_swap_in_attempt() near the end of the file.  The
 * difference is that this version makes guarantees about what it
 * provides, but tile_swap_in_attempt() just tries and gives up if
 * anything goes wrong.
 *
 * I'm not sure that it is worthwhile to try to pull out common
 * bits; I think the two functions are (at least for now) different
 * enough to keep as two functions.
 *
 * N.B. the mutex on the tile must already have been locked on entry
 * to this function.  DO NOT LOCK IT HERE.
 */
static void
tile_swap_default_in (DefSwapFile *def_swap_file,
                      gint         fd,
                      Tile        *tile)
{
  gint  err;
  gint  nleft;
  off_t offset;

  err = -1;

  if (tile->data)
    return;

  if (def_swap_file->cur_position != tile->swap_offset)
    {
      def_swap_file->cur_position = tile->swap_offset;

      offset = lseek (fd, tile->swap_offset, SEEK_SET);
      if (offset == -1)
        {
          if (seek_err_msg)
            g_message ("unable to seek to tile location on disk: %d", err);
          seek_err_msg = FALSE;
          return;
        }
    }

  tile_alloc (tile);

  nleft = tile->size;
  while (nleft > 0)
    {
      do
        {
          err = read (fd, tile->data + tile->size - nleft, nleft);
        }
      while ((err == -1) && ((errno == EAGAIN) || (errno == EINTR)));

      if (err <= 0)
        {
          if (read_err_msg)
            g_message ("unable to read tile data from disk: "
                       "%s (%d/%d bytes read)",
                       g_strerror (errno), err, nleft);
          read_err_msg = FALSE;
          return;
        }

      nleft -= err;
    }

  def_swap_file->cur_position += tile->size;

  /*  Do not delete the swap from the file  */
  /*  tile_swap_default_delete (def_swap_file, fd, tile);  */

  read_err_msg = seek_err_msg = TRUE;
}

static void
tile_swap_default_out (DefSwapFile *def_swap_file,
                       int          fd,
                       Tile        *tile)
{
  gint  bytes;
  gint  err;
  gint  nleft;
  off_t offset;
  off_t newpos;

  bytes = TILE_WIDTH * TILE_HEIGHT * tile->bpp;

  /*  If there is already a valid swap_offset, use it  */
  if (tile->swap_offset == -1)
    newpos = tile_swap_find_offset (def_swap_file, fd, bytes);
  else
    newpos = tile->swap_offset;

  if (def_swap_file->cur_position != newpos)
    {
      offset = lseek (fd, newpos, SEEK_SET);
      if (offset == -1)
        {
          if (seek_err_msg)
            g_message ("unable to seek to tile location on disk: %s",
                       g_strerror (errno));
          seek_err_msg = FALSE;
          return;
        }
      def_swap_file->cur_position = newpos;
    }

  nleft = tile->size;
  while (nleft > 0)
    {
      err = write (fd, tile->data + tile->size - nleft, nleft);
      if (err <= 0)
        {
          if (write_err_msg)
            g_message ("unable to write tile data to disk: "
                       "%s (%d/%d bytes written)",
                       g_strerror (errno), err, nleft);
          write_err_msg = FALSE;
          return;
        }

      nleft -= err;
    }

  def_swap_file->cur_position += tile->size;

  /* Do NOT free tile->data because we may be pre-swapping.
   * tile->data is freed in tile_cache_zorch_next
   */
  tile->dirty = FALSE;
  tile->swap_offset = newpos;

  write_err_msg = seek_err_msg = TRUE;
}

static void
tile_swap_default_delete (DefSwapFile *def_swap_file,
                          gint         fd,
                          Tile        *tile)
{
  GList *tmp;
  GList *tmp2;
  Gap   *gap;
  Gap   *gap2;
  off_t  start;
  off_t  end;

  if (tile->swap_offset == -1)
    return;

  start = tile->swap_offset;
  end = start + TILE_WIDTH * TILE_HEIGHT * tile->bpp;
  tile->swap_offset = -1;

  tmp = def_swap_file->gaps;
  while (tmp)
    {
      gap = tmp->data;

      if (end == gap->start)
        {
          gap->start = start;

          if (tmp->prev)
            {
              gap2 = tmp->prev->data;
              if (gap->start == gap2->end)
                {
                  gap2->end = gap->end;
                  tile_swap_gap_destroy (gap);
                  def_swap_file->gaps =
                    g_list_remove_link (def_swap_file->gaps, tmp);
                  g_list_free (tmp);
                }
            }
          break;
        }
      else if (start == gap->end)
        {
          gap->end = end;

          if (tmp->next)
            {
              gap2 = tmp->next->data;
              if (gap->end == gap2->start)
                {
                  gap2->start = gap->start;
                  tile_swap_gap_destroy (gap);
                  def_swap_file->gaps =
                    g_list_remove_link (def_swap_file->gaps, tmp);
                  g_list_free (tmp);
                }
            }
          break;
        }
      else if (end < gap->start)
        {
          gap = tile_swap_gap_new (start, end);

          tmp2 = g_list_alloc ();
          tmp2->data = gap;
          tmp2->next = tmp;
          tmp2->prev = tmp->prev;
          if (tmp->prev)
            tmp->prev->next = tmp2;
          tmp->prev = tmp2;

          if (tmp == def_swap_file->gaps)
            def_swap_file->gaps = tmp2;
          break;
        }
      else if (!tmp->next)
        {
          gap = tile_swap_gap_new (start, end);
          tmp->next = g_list_alloc ();
          tmp->next->data = gap;
          tmp->next->prev = tmp;
          break;
        }

      tmp = tmp->next;
    }

  if (!def_swap_file->gaps)
    {
      gap = tile_swap_gap_new (start, end);
      def_swap_file->gaps = g_list_append (def_swap_file->gaps, gap);
    }

  tmp = g_list_last (def_swap_file->gaps);
  gap = tmp->data;

  if (gap->end == def_swap_file->swap_file_end)
    {
      tile_swap_resize (def_swap_file, fd, gap->start);
      tile_swap_gap_destroy (gap);
      def_swap_file->gaps = g_list_remove_link (def_swap_file->gaps, tmp);
      g_list_free (tmp);
    }
}

static void
tile_swap_resize (DefSwapFile *def_swap_file,
                  gint         fd,
                  off_t        new_size)
{
  if (def_swap_file->swap_file_end > new_size)
    {
      if (ftruncate (fd, new_size) != 0)
        {
          g_message ("Failed to resize swap file: %s", g_strerror (errno));
          return;
        }
    }

  def_swap_file->swap_file_end = new_size;
}

static off_t
tile_swap_find_offset (DefSwapFile *def_swap_file,
                       gint         fd,
                       off_t        bytes)
{
  GList *tmp;
  Gap   *gap;
  off_t  offset;

  tmp = def_swap_file->gaps;
  while (tmp)
    {
      gap = tmp->data;

      if ((gap->end - gap->start) >= bytes)
        {
          offset = gap->start;
          gap->start += bytes;

          if (gap->start == gap->end)
            {
              tile_swap_gap_destroy (gap);
              def_swap_file->gaps =
                g_list_remove_link (def_swap_file->gaps, tmp);
              g_list_free (tmp);
            }

          return offset;
        }

      tmp = tmp->next;
    }

  offset = def_swap_file->swap_file_end;

  tile_swap_resize (def_swap_file, fd,
                    def_swap_file->swap_file_end + swap_file_grow);

  if ((offset + bytes) < (def_swap_file->swap_file_end))
    {
      gap = tile_swap_gap_new (offset + bytes, def_swap_file->swap_file_end);
      def_swap_file->gaps = g_list_append (def_swap_file->gaps, gap);
    }

  return offset;
}

static Gap *
tile_swap_gap_new (off_t start,
                   off_t end)
{
  Gap *gap;

  gap = g_new (Gap, 1);
  gap->start = start;
  gap->end = end;

  return gap;
}

static void
tile_swap_gap_destroy (Gap *gap)
{
  g_free (gap);
}

