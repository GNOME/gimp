#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define MAX_OPEN_SWAP_FILES  16

#include "tile_swap.h"


typedef struct _SwapFile     SwapFile;
typedef struct _DefSwapFile  DefSwapFile;
typedef struct _Gap          Gap;

struct _SwapFile
{
  char *filename;
  int swap_num;
  SwapFunc swap_func;
  gpointer user_data;
  int fd;
};

struct _DefSwapFile
{
  GList *gaps;
  long swap_file_end;
  off_t cur_position;
};

struct _Gap
{
  long start;
  long end;
};


static void  tile_swap_init    (void);
static guint tile_swap_hash    (int      *key);
static gint  tile_swap_compare (int      *a,
				int      *b);
static void  tile_swap_command (Tile     *tile,
				int       command);
static void  tile_swap_open    (SwapFile *swap_file);

static int   tile_swap_default        (int       fd,
				       Tile     *tile,
				       int       cmd,
				       gpointer  user_data);
static void  tile_swap_default_in     (DefSwapFile *def_swap_file,
				       int          fd,
				       Tile        *tile);
static void  tile_swap_default_out    (DefSwapFile *def_swap_file,
				       int          fd,
				       Tile        *tile);
static void  tile_swap_default_delete (DefSwapFile *def_swap_file,
				       int          fd,
				       Tile        *tile);
static long  tile_swap_find_offset    (DefSwapFile *def_swap_file,
				       int          fd,
				       int          bytes);
static void  tile_swap_resize         (DefSwapFile *def_swap_file,
				       int          fd,
				       long         new_size);
static Gap*  tile_swap_gap_new        (long         start,
				       long         end);
static void  tile_swap_gap_destroy    (Gap         *gap);


static int initialize = TRUE;
static GHashTable *swap_files = NULL;
static GList *open_swap_files = NULL;
static int nopen_swap_files = 0;
static int next_swap_num = 1;
static long swap_file_grow = 16 * TILE_WIDTH * TILE_HEIGHT * 4;

static gboolean seek_err_msg = TRUE, read_err_msg = TRUE, write_err_msg = TRUE;

static void
tile_swap_print_gaps (DefSwapFile *def_swap_file)
{
  GList *gaps;
  Gap *gap;

  gaps = def_swap_file->gaps;
  while (gaps)
    {
      gap = gaps->data;
      gaps = gaps->next;

      g_print ("  %6ld - %6ld\n", gap->start, gap->end);
    }
}

static void
tile_swap_exit1 (gpointer key,
		 gpointer value,
		 gpointer data)
{
  extern int tile_ref_count;
  SwapFile *swap_file;
  DefSwapFile *def_swap_file;

  if (tile_ref_count != 0)
    g_message ("tile ref count balance: %d\n", tile_ref_count);

  swap_file = value;
  if (swap_file->swap_func == tile_swap_default)
    {
      def_swap_file = swap_file->user_data;
      if (def_swap_file->swap_file_end != 0)
	{
	  g_message ("swap file not empty: \"%s\"\n", swap_file->filename);
	  tile_swap_print_gaps (def_swap_file);
	}

      unlink (swap_file->filename);
    }
}


void
tile_swap_exit ()
{
  if (swap_files)
    g_hash_table_foreach (swap_files, tile_swap_exit1, NULL);
}

int
tile_swap_add (char     *filename,
	       SwapFunc  swap_func,
	       gpointer  user_data)
{
  SwapFile *swap_file;
  DefSwapFile *def_swap_file;

  if (initialize)
    tile_swap_init ();

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
tile_swap_remove (int swap_num)
{
  SwapFile *swap_file;

  if (initialize)
    tile_swap_init ();

  swap_file = g_hash_table_lookup (swap_files, &swap_num);
  if (!swap_file)
    return;

  g_hash_table_remove (swap_files, &swap_num);

  if (swap_file->fd != -1)
    close (swap_file->fd);

  g_free (swap_file);
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
tile_swap_compress (int swap_num)
{
  tile_swap_command (NULL, SWAP_COMPRESS);
}

static void
tile_swap_init ()
{
  if (initialize)
    {
      initialize = FALSE;

      swap_files = g_hash_table_new ((GHashFunc) tile_swap_hash,
				     (GCompareFunc) tile_swap_compare);
    }
}

static guint
tile_swap_hash (int *key)
{
  return ((guint) *key);
}

static gint
tile_swap_compare (int *a,
		   int *b)
{
  return (*a == *b);
}

static void
tile_swap_command (Tile *tile,
		   int   command)
{
  SwapFile *swap_file;

  if (initialize)
    tile_swap_init ();

  do {
    swap_file = g_hash_table_lookup (swap_files, &tile->swap_num);
    if (!swap_file)
      {
	g_message ("could not find swap file for tile");
	return;
      }

    if (swap_file->fd == -1)
      {
	tile_swap_open (swap_file);

	if (swap_file->fd == -1)
	  return;
      }
  } while ((* swap_file->swap_func) (swap_file->fd, tile, command, swap_file->user_data));
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

  swap_file->fd = open (swap_file->filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
  if (swap_file->fd == -1)
    {
      g_message ("unable to open swap file...BAD THINGS WILL HAPPEN SOON");
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
tile_swap_default (int       fd,
		   Tile     *tile,
		   int       cmd,
		   gpointer  user_data)
{
  DefSwapFile *def_swap_file;

  def_swap_file = (DefSwapFile*) user_data;

  switch (cmd)
    {
    case SWAP_IN:
      tile_swap_default_in (def_swap_file, fd, tile);
      break;
    case SWAP_OUT:
      tile_swap_default_out (def_swap_file, fd, tile);
      break;
    case SWAP_DELETE:
      tile_swap_default_delete (def_swap_file, fd, tile);
      break;
    case SWAP_COMPRESS:
      g_message ("tile_swap_default: SWAP_COMPRESS: UNFINISHED");
      break;
    }

  return FALSE;
}

static void
tile_swap_default_in (DefSwapFile *def_swap_file,
		      int          fd,
		      Tile        *tile)
{
  int bytes;
  int err;
  int nleft;
  off_t offset;

  err = -1;

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

  bytes = tile_size (tile);
  tile_alloc (tile);

  nleft = bytes;
  while (nleft > 0)
    {
      do {
	err = read (fd, tile->data + bytes - nleft, nleft);
      } while ((err == -1) && ((errno == EAGAIN) || (errno == EINTR)));

      if (err <= 0)
	{
	  if (read_err_msg)
	    g_message ("unable to read tile data from disk: %d ( %d ) bytes read", err, nleft);
	  read_err_msg = FALSE;
	  return;
	}

      nleft -= err;
    }

  def_swap_file->cur_position += bytes;

  /*  Do not delete the swap from the file  */
  /*  tile_swap_default_delete (def_swap_file, fd, tile);  */

  read_err_msg = seek_err_msg = TRUE;
}

static void
tile_swap_default_out (DefSwapFile *def_swap_file,
		       int          fd,
		       Tile        *tile)
{
  int bytes;
  int rbytes;
  int err;
  int nleft;
  off_t offset;

  bytes = TILE_WIDTH * TILE_HEIGHT * tile->bpp;
  rbytes = tile_size (tile);

  /*  If there is already a valid swap_offset, use it  */
  if (tile->swap_offset == -1)
    tile->swap_offset = tile_swap_find_offset (def_swap_file, fd, bytes);

  if (def_swap_file->cur_position != tile->swap_offset)
    {
      def_swap_file->cur_position = tile->swap_offset;

      offset = lseek (fd, tile->swap_offset, SEEK_SET);
      if (offset == -1)
	{
	  if (seek_err_msg)
	    g_message ("unable to seek to tile location on disk: %d", errno);
	  seek_err_msg = FALSE;
	  return;
	}
    }

  nleft = rbytes;
  while (nleft > 0)
    {
      err = write (fd, tile->data + rbytes - nleft, nleft);
      if (err <= 0)
	{
	  if (write_err_msg)
	    g_message ("unable to write tile data to disk: %d ( %d ) bytes written", err, nleft);
	  write_err_msg = FALSE;
	  return;
	}

      nleft -= err;
    }

  def_swap_file->cur_position += rbytes;

  g_free (tile->data);
  tile->data = NULL;

  write_err_msg = seek_err_msg = TRUE;
}

static void
tile_swap_default_delete (DefSwapFile *def_swap_file,
			  int          fd,
			  Tile        *tile)
{
  GList *tmp;
  GList *tmp2;
  Gap *gap;
  Gap *gap2;
  long start;
  long end;

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
		  def_swap_file->gaps = g_list_remove_link (def_swap_file->gaps, tmp);
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
		  def_swap_file->gaps = g_list_remove_link (def_swap_file->gaps, tmp);
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
		  int          fd,
		  long         new_size)
{
  if (def_swap_file->swap_file_end > new_size)
    ftruncate (fd, new_size);
  def_swap_file->swap_file_end = new_size;
}

static long
tile_swap_find_offset (DefSwapFile *def_swap_file,
		       int          fd,
		       int          bytes)
{
  GList *tmp;
  Gap *gap;
  long offset;

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
	      def_swap_file->gaps = g_list_remove_link (def_swap_file->gaps, tmp);
	      g_list_free (tmp);
	    }

	  return offset;
	}

      tmp = tmp->next;
    }

  offset = def_swap_file->swap_file_end;

  tile_swap_resize (def_swap_file, fd, def_swap_file->swap_file_end + swap_file_grow);

  if ((offset + bytes) < (def_swap_file->swap_file_end))
    {
      gap = tile_swap_gap_new (offset + bytes, def_swap_file->swap_file_end);
      def_swap_file->gaps = g_list_append (def_swap_file->gaps, gap);
    }

  return offset;
}

static Gap*
tile_swap_gap_new (long start,
		   long end)
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
