/* GIMP - The GNU Image Manipulation Program
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
#include <windows.h>
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
#include "tile-rowhints.h"
#include "tile-swap.h"
#include "tile-private.h"

#include "gimp-intl.h"


typedef enum
{
  SWAP_IN = 1,
  SWAP_OUT,
  SWAP_DELETE
} SwapCommand;

typedef gint (* SwapFunc) (gint         fd,
                           Tile        *tile,
                           SwapCommand  cmd);



#define MAX_OPEN_SWAP_FILES  16


typedef struct _SwapFile     SwapFile;
typedef struct _SwapFileGap  SwapFileGap;

struct _SwapFile
{
  gchar   *filename;
  gint     fd;
  GList   *gaps;
  gint64   swap_file_end;
  gint64   cur_position;
};

struct _SwapFileGap
{
  gint64 start;
  gint64 end;
};


static void          tile_swap_command        (Tile        *tile,
                                               gint         command);
static void          tile_swap_default_in     (SwapFile    *swap_file,
                                               Tile        *tile);
static void          tile_swap_default_out    (SwapFile    *swap_file,
                                               Tile        *tile);
static void          tile_swap_default_delete (SwapFile    *swap_file,
                                               Tile        *tile);

static gint64        tile_swap_find_offset    (SwapFile    *swap_file,
                                               gint64       bytes);
static void          tile_swap_open           (SwapFile    *swap_file);
static void          tile_swap_resize         (SwapFile    *swap_file,
                                               gint64       new_size);
static SwapFileGap * tile_swap_gap_new        (gint64       start,
                                               gint64       end);
static void          tile_swap_gap_destroy    (SwapFileGap *gap);


static SwapFile     * gimp_swap_file   = NULL;

static const gint64   swap_file_grow   = 1024 * TILE_WIDTH * TILE_HEIGHT * 4;

static gboolean       seek_err_msg     = TRUE;
static gboolean       read_err_msg     = TRUE;
static gboolean       write_err_msg    = TRUE;


#ifdef G_OS_WIN32

#define LARGE_SEEK(f, o, w) _lseeki64 (f, o, w)
#define LARGE_TRUNCATE(f, s) win32_large_truncate (f, s)

static gint
win32_large_truncate (gint   fd,
		      gint64 size)
{
  if (LARGE_SEEK (fd, size, SEEK_SET) == size &&
      SetEndOfFile ((HANDLE) _get_osfhandle (fd)))
    return 0;
  else
    return -1;
}

#else

#define LARGE_SEEK(f, o, t) lseek (f, o, t)
#define LARGE_TRUNCATE(f, s) ftruncate (f, s)

#endif


#ifdef GIMP_UNSTABLE
static void
tile_swap_print_gaps (SwapFile *swap_file)
{
  GList *list;

  for (list = swap_file->gaps; list; list = list->next)
    {
      SwapFileGap *gap = list->data;

      g_print ("  %"G_GINT64_FORMAT" - %"G_GINT64_FORMAT"\n",
               gap->start, gap->end);
    }
}
#endif

void
tile_swap_init (const gchar *path)
{
  gchar *basename;
  gchar *dirname;

  g_return_if_fail (gimp_swap_file == NULL);
  g_return_if_fail (path != NULL);

  dirname  = gimp_config_path_expand (path, TRUE, NULL);
  basename = g_strdup_printf ("gimpswap.%lu", (unsigned long) getpid ());

  /*  create the swap directory if it doesn't exist */
  if (! g_file_test (dirname, G_FILE_TEST_EXISTS))
    g_mkdir_with_parents (dirname,
                          S_IRUSR | S_IXUSR | S_IWUSR |
                          S_IRGRP | S_IXGRP |
                          S_IROTH | S_IXOTH);

  gimp_swap_file = g_slice_new (SwapFile);

  gimp_swap_file->filename      = g_build_filename (dirname, basename, NULL);
  gimp_swap_file->gaps          = NULL;
  gimp_swap_file->swap_file_end = 0;
  gimp_swap_file->cur_position  = 0;
  gimp_swap_file->fd            = -1;

  g_free (basename);
  g_free (dirname);
}

void
tile_swap_exit (void)
{
#ifdef HINTS_SANITY
  extern int tile_exist_peak;

  g_printerr ("Tile exist peak was %d Tile structs (%d bytes)",
              tile_exist_peak, tile_exist_peak * sizeof(Tile));
#endif

  if (tile_global_refcount () != 0)
    g_warning ("tile ref count balance: %d\n", tile_global_refcount ());

  g_return_if_fail (gimp_swap_file != NULL);

#ifdef GIMP_UNSTABLE
  if (gimp_swap_file->swap_file_end != 0)
    {
      g_warning ("swap file not empty: \"%s\"\n",
                 gimp_filename_to_utf8 (gimp_swap_file->filename));
      tile_swap_print_gaps (gimp_swap_file);
    }
#endif

#ifdef G_OS_WIN32
  /* should close before unlink */
  if (gimp_swap_file->fd > 0)
    {
      close (gimp_swap_file->fd);
      gimp_swap_file->fd = -1;
    }
#endif
  g_unlink (gimp_swap_file->filename);

  g_free (gimp_swap_file->filename);
  g_slice_free (SwapFile, gimp_swap_file);

  gimp_swap_file = NULL;
}

/* check if we can open a swap file */
gboolean
tile_swap_test (void)
{
  g_return_val_if_fail (gimp_swap_file != NULL, FALSE);

  /* make sure this duplicates the open() call from tile_swap_open() */
  gimp_swap_file->fd = g_open (gimp_swap_file->filename,
                               O_CREAT | O_RDWR | _O_BINARY | _O_TEMPORARY,
                               S_IRUSR | S_IWUSR);

  if (gimp_swap_file->fd != -1)
    {
      close (gimp_swap_file->fd);
      gimp_swap_file->fd = -1;
      g_unlink (gimp_swap_file->filename);

      return TRUE;
    }

  return FALSE;
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

static void
tile_swap_command (Tile *tile,
                   gint  command)
{
  if (gimp_swap_file->fd == -1)
    {
      tile_swap_open (gimp_swap_file);

      if (G_UNLIKELY (gimp_swap_file->fd == -1))
        return;
    }

  switch (command)
    {
    case SWAP_IN:
      tile_swap_default_in (gimp_swap_file, tile);
      break;
    case SWAP_OUT:
      tile_swap_default_out (gimp_swap_file, tile);
      break;
    case SWAP_DELETE:
      tile_swap_default_delete (gimp_swap_file, tile);
      break;
    }
}

/* The actual swap file code. The swap file consists of tiles
 *  which have been moved out to disk in order to conserve memory.
 *  The swap file format is free form. Any tile in memory may
 *  end up anywhere on disk.
 * An actual tile in the swap file consists only of the tile data.
 *  The offset of the tile on disk is stored in the tile data structure
 *  in memory.
 */

static void
tile_swap_default_in (SwapFile *swap_file,
                      Tile     *tile)
{
  gint   nleft;
  gint64 offset;

  if (tile->data)
    return;

  if (swap_file->cur_position != tile->swap_offset)
    {
      swap_file->cur_position = tile->swap_offset;

      offset = LARGE_SEEK (swap_file->fd, tile->swap_offset, SEEK_SET);
      if (offset == -1)
        {
          if (seek_err_msg)
            g_message ("unable to seek to tile location on disk: %s",
                       g_strerror (errno));
          seek_err_msg = FALSE;
          return;
        }
    }

  tile_alloc (tile);

  nleft = tile->size;
  while (nleft > 0)
    {
      gint err;

      do
        {
          err = read (swap_file->fd, tile->data + tile->size - nleft, nleft);
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

  swap_file->cur_position += tile->size;

  /*  Do not delete the swap from the file  */
  /*  tile_swap_default_delete (swap_file, fd, tile);  */

  read_err_msg = seek_err_msg = TRUE;
}

static void
tile_swap_default_out (SwapFile *swap_file,
                       Tile     *tile)
{
  gint   bytes;
  gint   nleft;
  gint64 offset;
  gint64 newpos;

  bytes = TILE_WIDTH * TILE_HEIGHT * tile->bpp;

  /*  If there is already a valid swap_offset, use it  */
  if (tile->swap_offset == -1)
    newpos = tile_swap_find_offset (swap_file, bytes);
  else
    newpos = tile->swap_offset;

  if (swap_file->cur_position != newpos)
    {
      offset = LARGE_SEEK (swap_file->fd, newpos, SEEK_SET);

      if (offset == -1)
        {
          if (seek_err_msg)
            g_message ("unable to seek to tile location on disk: %s",
                       g_strerror (errno));
          seek_err_msg = FALSE;
          return;
        }

      swap_file->cur_position = newpos;
    }

  nleft = tile->size;
  while (nleft > 0)
    {
      gint err = write (swap_file->fd, tile->data + tile->size - nleft, nleft);

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

  swap_file->cur_position += tile->size;

  /* Do NOT free tile->data because we may be pre-swapping.
   * tile->data is freed in tile_cache_zorch_next
   */
  tile->dirty = FALSE;
  tile->swap_offset = newpos;

  write_err_msg = seek_err_msg = TRUE;
}

static void
tile_swap_default_delete (SwapFile *swap_file,
                          Tile     *tile)
{
  SwapFileGap *gap;
  SwapFileGap *gap2;
  GList       *tmp;
  GList       *tmp2;
  gint64       start;
  gint64       end;

  if (tile->swap_offset == -1)
    return;

  start = tile->swap_offset;
  end = start + TILE_WIDTH * TILE_HEIGHT * tile->bpp;
  tile->swap_offset = -1;

  tmp = swap_file->gaps;
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
                  swap_file->gaps =
                    g_list_remove_link (swap_file->gaps, tmp);
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
                  swap_file->gaps =
                    g_list_remove_link (swap_file->gaps, tmp);
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

          if (tmp == swap_file->gaps)
            swap_file->gaps = tmp2;
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

  if (!swap_file->gaps)
    {
      gap = tile_swap_gap_new (start, end);
      swap_file->gaps = g_list_append (swap_file->gaps, gap);
    }

  tmp = g_list_last (swap_file->gaps);
  gap = tmp->data;

  if (gap->end == swap_file->swap_file_end)
    {
      tile_swap_resize (swap_file, gap->start);
      tile_swap_gap_destroy (gap);
      swap_file->gaps = g_list_remove_link (swap_file->gaps, tmp);
      g_list_free (tmp);
    }
}

static void
tile_swap_open (SwapFile *swap_file)
{
  g_return_if_fail (swap_file->fd == -1);

  /* duplicate this open() call in tile_swap_test() */
  swap_file->fd = g_open (swap_file->filename,
                          O_CREAT | O_RDWR | _O_BINARY | _O_TEMPORARY,
                          S_IRUSR | S_IWUSR);

  if (swap_file->fd == -1)
    g_message (_("Unable to open swap file. GIMP has run out of memory "
                 "and cannot use the swap file. Some parts of your images "
                 "may be corrupted. Try to save your work using different "
                 "filenames, restart GIMP and check the location of the "
                 "swap directory in your Preferences."));
}

static void
tile_swap_resize (SwapFile *swap_file,
                  gint64    new_size)
{
  if (swap_file->swap_file_end > new_size)
    {
      if (LARGE_TRUNCATE (swap_file->fd, new_size) != 0)
        {
          g_message (_("Failed to resize swap file: %s"), g_strerror (errno));
          return;
        }
    }

  swap_file->swap_file_end = new_size;
}

static gint64
tile_swap_find_offset (SwapFile *swap_file,
                       gint64    bytes)
{
  SwapFileGap *gap;
  GList       *tmp;
  gint64       offset;

  tmp = swap_file->gaps;
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
              swap_file->gaps = g_list_remove_link (swap_file->gaps, tmp);
              g_list_free (tmp);
            }

          return offset;
        }

      tmp = tmp->next;
    }

  offset = swap_file->swap_file_end;

  tile_swap_resize (swap_file, swap_file->swap_file_end + swap_file_grow);

  if ((offset + bytes) < (swap_file->swap_file_end))
    {
      gap = tile_swap_gap_new (offset + bytes, swap_file->swap_file_end);
      swap_file->gaps = g_list_append (swap_file->gaps, gap);
    }

  return offset;
}

static SwapFileGap *
tile_swap_gap_new (gint64 start,
                   gint64 end)
{
  SwapFileGap *gap = g_slice_new (SwapFileGap);

  gap->start = start;
  gap->end   = end;

  return gap;
}

static void
tile_swap_gap_destroy (SwapFileGap *gap)
{
  g_slice_free (SwapFileGap, gap);
}
