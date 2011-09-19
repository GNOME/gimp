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
#include <gegl-tile.h>

#include "libgimpmath/gimpmath.h"

#include "gimp-gegl-types.h"

#include "base/tile.h"
#include "base/tile-manager.h"

#include "gimptilebackendtilemanager.h"
#include "gimp-gegl-utils.h"


struct _GimpTileBackendTileManagerPrivate 
{
  GHashTable      *entries;
  TileManager     *tile_manager;
};


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

  g_type_class_add_private (klass, sizeof (GimpTileBackendTileManagerPrivate));
}

static void
gimp_tile_backend_tile_manager_init (GimpTileBackendTileManager *backend)
{
  GeglTileSource *source = GEGL_TILE_SOURCE (backend);

  backend->priv = G_TYPE_INSTANCE_GET_PRIVATE (backend,
                                               GIMP_TYPE_TILE_BACKEND_TILE_MANAGER,
                                               GimpTileBackendTileManagerPrivate);
  source->command  = gimp_tile_backend_tile_manager_command;

  backend->priv->entries = g_hash_table_new (hash_func, equal_func);
}

static void
gimp_tile_backend_tile_manager_finalize (GObject *object)
{
  GimpTileBackendTileManager *backend = GIMP_TILE_BACKEND_TILE_MANAGER (object);

  g_hash_table_unref (backend->priv->entries);

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
        gint      tile_size;
        Tile     *gimp_tile;
        gint      tile_stride;
        gint      gimp_tile_stride;
        int       row;

        gimp_tile = tile_manager_get_at (backend_tm->priv->tile_manager,
                                         x, y, TRUE, FALSE);

        g_return_val_if_fail (gimp_tile != NULL, NULL);

        tile_size        = gegl_tile_backend_get_tile_size (backend);
        tile_stride      = TILE_WIDTH * tile_bpp (gimp_tile);
        gimp_tile_stride = tile_ewidth (gimp_tile) * tile_bpp (gimp_tile);

        /* XXX: Point to Tile data directly instead of using memcpy */
        tile = gegl_tile_new (tile_size);
        for (row = 0; row < tile_eheight (gimp_tile); row++)
          {
            memcpy (gegl_tile_get_data (tile) + row * tile_stride,
                    tile_data_pointer (gimp_tile, 0, row),
                    gimp_tile_stride);
          }

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

            g_hash_table_insert (backend_tm->priv->entries, entry, entry);
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

        return GINT_TO_POINTER (entry != NULL);
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

  return g_hash_table_lookup (self->priv->entries, &key);
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
  g_hash_table_remove (ram->priv->entries, entry);

  dbg_dealloc (tile_size);
  g_slice_free (RamEntry, entry);
}

GeglTileBackend *
gimp_tile_backend_tile_manager_new (TileManager *tm)
{
  GeglTileBackend *ret;
  gint             width  = tile_manager_width (tm);
  gint             height = tile_manager_height (tm);
  gint             bpp    = tile_manager_bpp (tm);
  GeglRectangle    rect   = { 0, 0, width, height };

  ret = g_object_new (GIMP_TYPE_TILE_BACKEND_TILE_MANAGER,
                      "tile-width",  TILE_WIDTH,
                      "tile-height", TILE_HEIGHT,
                      "format",      gimp_bpp_to_babl_format (bpp, FALSE),
                      NULL);

  GIMP_TILE_BACKEND_TILE_MANAGER (ret)->priv->tile_manager = tile_manager_ref (tm);

  gegl_tile_backend_set_extent (ret, &rect);

  return ret;
}

void
gimp_tile_backend_tile_manager_stats (void)
{
  g_warning ("leaked: %i chunks (%f mb)  peak: %i (%i bytes %fmb))",
             allocs, ram_size / 1024 / 1024.0, peak_allocs,
             peak_tile_manager_size, peak_tile_manager_size / 1024 / 1024.0);
}
