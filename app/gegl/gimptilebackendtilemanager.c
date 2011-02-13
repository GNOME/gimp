/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptilebackendtilemanager.c
 * Copyright (C) 2011 Øyvind Kolås <pippin@gimp.org>
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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gegl-buffer.h>

#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "base/tile.h"

#include "gimptilebackendtilemanager.h"


typedef struct _RamEntry RamEntry;

struct _RamEntry
{
  gint    x;
  gint    y;
  gint    z;
  guchar *offset;
};


static void     gimp_tile_backend_tile_manager_finalize     (GObject         *object);
static void     gimp_tile_backend_tile_manager_set_property (GObject         *object,
                                                             guint            property_id,
                                                             const GValue    *value,
                                                             GParamSpec      *pspec);
static void     gimp_tile_backend_tile_manager_get_property (GObject         *object,
                                                             guint            property_id,
                                                             GValue          *value,
                                                             GParamSpec      *pspec);
static gpointer gimp_tile_backend_tile_manager_command      (GeglTileSource  *tile_store,
                                                             GeglTileCommand  command,
                                                             gint             x,
                                                             gint             y,
                                                             gint             z,
                                                             gpointer         data);

static void       dbg_alloc         (int                         size);
static void       dbg_dealloc       (int                         size);
static RamEntry * lookup_entry      (GimpTileBackendTileManager *self,
                                     gint                        x,
                                     gint                        y,
                                     gint                        z);
static guint      hash_func         (gconstpointer               key);
static gboolean   equal_func        (gconstpointer               a,
                                     gconstpointer               b);
static void       ram_entry_read    (GimpTileBackendTileManager *ram,
                                     gint                        x,
                                     gint                        y,
                                     gint                        z,
                                     gfloat                      w,
                                     gfloat                      h,
                                     guchar                     *dest);
static void       ram_entry_write   (GimpTileBackendTileManager *ram,
                                     RamEntry                   *entry,
                                     guchar                     *source);
static RamEntry * ram_entry_new     (GimpTileBackendTileManager *ram);
static void       ram_entry_destroy (RamEntry                   *entry,
                                     GimpTileBackendTileManager *ram);


G_DEFINE_TYPE (GimpTileBackendTileManager, gimp_tile_backend_tile_manager,
               GEGL_TYPE_TILE_BACKEND)

#define parent_class gimp_tile_backend_tile_manager_parent_class


static void
gimp_tile_backend_tile_manager_class_init (GimpTileBackendTileManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_tile_backend_tile_manager_finalize;
  object_class->get_property = gimp_tile_backend_tile_manager_get_property;
  object_class->set_property = gimp_tile_backend_tile_manager_set_property;
}

static void
gimp_tile_backend_tile_manager_init (GimpTileBackendTileManager *backend)
{
  GeglTileSource *source = GEGL_TILE_SOURCE (backend);

  source->command  = gimp_tile_backend_tile_manager_command;

  backend->entries = g_hash_table_new (hash_func, equal_func);
}

static void
gimp_tile_backend_tile_manager_finalize (GObject *object)
{
  GimpTileBackendTileManager *backend = GIMP_TILE_BACKEND_TILE_MANAGER (object);

  g_hash_table_unref (backend->entries);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tile_backend_tile_manager_set_property (GObject       *object,
                                             guint          property_id,
                                             const GValue *value,
                                             GParamSpec    *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tile_backend_tile_manager_get_property (GObject    *object,
                                             guint       property_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gpointer
gimp_tile_backend_tile_manager_command (GeglTileSource  *tile_store,
                                        GeglTileCommand  command,
                                        gint             x,
                                        gint             y,
                                        gint             z,
                                        gpointer         data)
{
  GimpTileBackendTileManager *backend_tm;
  GeglTileBackend            *backend;

  backend_tm = GIMP_TILE_BACKEND_TILE_MANAGER (tile_store);
  backend    = GEGL_TILE_BACKEND (tile_store);

  switch (command)
    {
    case GEGL_TILE_GET:
      {
        GeglTile *tile;
        gint      tile_size = gegl_tile_backend_get_tile_size (backend);

        tile = gegl_tile_new (tile_size);

        ram_entry_read (backend_tm, x, y, z, TILE_WIDTH, TILE_HEIGHT,
                        gegl_tile_get_data (tile));

        return tile;
      }

    case GEGL_TILE_SET:
      {
        GeglTile *tile  = data;
        RamEntry *entry = lookup_entry (backend_tm, x, y, z);

        if (! entry)
          {
            entry    = ram_entry_new (backend_tm);
            entry->x = x;
            entry->y = y;
            entry->z = z;

            g_hash_table_insert (backend_tm->entries, entry, entry);
          }

        ram_entry_write (backend_tm, entry, gegl_tile_get_data (tile));

        gegl_tile_mark_as_stored (tile);

        return NULL;
      }

    case GEGL_TILE_IDLE:
      return NULL;

    case GEGL_TILE_VOID:
      {
        RamEntry *entry = lookup_entry (backend_tm, x, y, z);

        if (entry)
          ram_entry_destroy (entry, backend_tm);

        return NULL;
      }

    case GEGL_TILE_EXIST:
      {
        RamEntry *entry = lookup_entry (backend_tm, x, y, z);

        return (gpointer) (entry != NULL);
      }

    default:
      g_assert (command < GEGL_TILE_LAST_COMMAND && command >= 0);
    }

  return NULL;
}

static gint allocs                 = 0;
static gint ram_size               = 0;
static gint peak_allocs            = 0;
static gint peak_tile_manager_size = 0;

static void
dbg_alloc (gint size)
{
  allocs++;
  ram_size += size;
  if (allocs > peak_allocs)
    peak_allocs = allocs;
  if (ram_size > peak_tile_manager_size)
    peak_tile_manager_size = ram_size;
}

static void
dbg_dealloc (gint size)
{
  allocs--;
  ram_size -= size;
}

static RamEntry *
lookup_entry (GimpTileBackendTileManager *self,
              gint                        x,
              gint                        y,
              gint                        z)
{
  RamEntry key;

  key.x      = x;
  key.y      = y;
  key.z      = z;
  key.offset = 0;

  return g_hash_table_lookup (self->entries, &key);
}

static guint
hash_func (gconstpointer key)
{
  const RamEntry *e = key;
  guint           hash;
  gint            srcA = e->x;
  gint            srcB = e->y;
  gint            srcC = e->z;
  gint            i;

  /* interleave the 10 least significant bits of all coordinates,
   * this gives us Z-order / morton order of the space and should
   * work well as a hash
   */
  hash = 0;

#define ADD_BIT(bit)    do { hash |= (((bit) != 0) ? 1 : 0); hash <<= 1; \
      }                                                                 \
      while (0)

  for (i = 9; i >= 0; i--)
    {
      ADD_BIT (srcA & (1 << i));
      ADD_BIT (srcB & (1 << i));
      ADD_BIT (srcC & (1 << i));
    }

#undef ADD_BIT

  return hash;
}

static gboolean
equal_func (gconstpointer a,
            gconstpointer b)
{
  const RamEntry *ea = a;
  const RamEntry *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z)
    return TRUE;

  return FALSE;
}

static gfloat
mandel_calc (gfloat x,
             gfloat y)
{
  gfloat fCReal = x;
  gfloat fCImg  = y;
  gfloat fZReal = fCReal;
  gfloat fZImg  = fCImg;
#define MAX_ITER 100

  gint n;

  for (n=0; n< MAX_ITER; n++)
    {
      gfloat fZRealSquared = fZReal * fZReal;
      gfloat fZImgSquared = fZImg * fZImg;

      if (fZRealSquared + fZImgSquared > 4)
        return 1.0*n/(MAX_ITER);

/*              -- z = z^2 + c*/
      fZImg = 2 * fZReal * fZImg + fCImg;
      fZReal = fZRealSquared - fZImgSquared + fCReal;
    }
  return 1.0;
}

static void
ram_entry_read (GimpTileBackendTileManager *ram,
                gint                        x,
                gint                        y,
                gint                        z,
                gfloat                      w,
                gfloat                      h,
                guchar                     *dest)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (ram);
  Babl            *format  = gegl_tile_backend_get_format (backend);
  gint             u, v;
  gint             i = 0;

  g_printerr ("%s\n", babl_get_name (format));
  g_printerr ("READ %i %i %i\n", x, y, z);

  for (v = 0; v < h; v++)
    for (u = 0; u < w; u++)
      {
        float a   = ((u + x * w) * powf (2,z) - 2 * w);
        float b   = ((v + y * h) * powf (2,z) - 1 * h);
        float val =
          mandel_calc (a / w,
                       b / h);
        dest[i*4+0] = val * 255;
        dest[i*4+1] = val * 255;
        dest[i*4+2] = val * 255;
        dest[i*4+3] = 255;
        i++;
      }
}

static void
ram_entry_write (GimpTileBackendTileManager *ram,
                 RamEntry                   *entry,
                 guchar                     *source)
{
  g_printerr ("WRITE %i %i %i\n", entry->x, entry->y, entry->z);
  //memcpy (entry->offset, source, tile_size);
}

static RamEntry *
ram_entry_new (GimpTileBackendTileManager *ram)
{
  gint tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (ram));
  RamEntry *self = g_slice_new (RamEntry);

  self->offset = g_malloc (tile_size);
  dbg_alloc (tile_size);

  return self;
}

static void
ram_entry_destroy (RamEntry           *entry,
                   GimpTileBackendTileManager *ram)
{
  gint tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (ram));
  g_free (entry->offset);
  g_hash_table_remove (ram->entries, entry);

  dbg_dealloc (tile_size);
  g_slice_free (RamEntry, entry);
}

GeglTileBackend *
gimp_tile_backend_tile_manager_new (void)
{
  GimpTileBackendTileManager *ret =
    g_object_new (GIMP_TYPE_TILE_BACKEND_TILE_MANAGER,
                  "tile-width",  TILE_WIDTH,
                  "tile-height", TILE_HEIGHT,
                  "format", babl_format ("RGBA u8"),
                  NULL);
  GeglRectangle rect = { 100,100, 200, 200 };

  gegl_tile_backend_set_extent (GEGL_TILE_BACKEND (ret), &rect);

  return GEGL_TILE_BACKEND (ret);
}

void
gimp_tile_backend_tile_manager_stats (void)
{
  g_warning ("leaked: %i chunks (%f mb)  peak: %i (%i bytes %fmb))",
             allocs, ram_size / 1024 / 1024.0, peak_allocs,
             peak_tile_manager_size, peak_tile_manager_size / 1024 / 1024.0);
}
