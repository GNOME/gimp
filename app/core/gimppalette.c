/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "ligma-memsize.h"
#include "ligmapalette.h"
#include "ligmapalette-load.h"
#include "ligmapalette-save.h"
#include "ligmatagged.h"
#include "ligmatempbuf.h"

#include "ligma-intl.h"


#define RGB_EPSILON 1e-6


/*  local function prototypes  */

static void          ligma_palette_tagged_iface_init (LigmaTaggedInterface  *iface);

static void          ligma_palette_finalize          (GObject              *object);

static gint64        ligma_palette_get_memsize       (LigmaObject           *object,
                                                     gint64               *gui_size);

static void          ligma_palette_get_preview_size  (LigmaViewable         *viewable,
                                                     gint                  size,
                                                     gboolean              popup,
                                                     gboolean              dot_for_dot,
                                                     gint                 *width,
                                                     gint                 *height);
static gboolean      ligma_palette_get_popup_size    (LigmaViewable         *viewable,
                                                     gint                  width,
                                                     gint                  height,
                                                     gboolean              dot_for_dot,
                                                     gint                 *popup_width,
                                                     gint                 *popup_height);
static LigmaTempBuf * ligma_palette_get_new_preview   (LigmaViewable         *viewable,
                                                     LigmaContext          *context,
                                                     gint                  width,
                                                     gint                  height);
static gchar       * ligma_palette_get_description   (LigmaViewable         *viewable,
                                                     gchar               **tooltip);
static const gchar * ligma_palette_get_extension     (LigmaData             *data);
static void          ligma_palette_copy              (LigmaData             *data,
                                                     LigmaData             *src_data);

static void          ligma_palette_entry_free        (LigmaPaletteEntry     *entry);
static gint64        ligma_palette_entry_get_memsize (LigmaPaletteEntry     *entry,
                                                     gint64               *gui_size);
static gchar       * ligma_palette_get_checksum      (LigmaTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (LigmaPalette, ligma_palette, LIGMA_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_TAGGED,
                                                ligma_palette_tagged_iface_init))

#define parent_class ligma_palette_parent_class


static void
ligma_palette_class_init (LigmaPaletteClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaDataClass     *data_class        = LIGMA_DATA_CLASS (klass);

  object_class->finalize            = ligma_palette_finalize;

  ligma_object_class->get_memsize    = ligma_palette_get_memsize;

  viewable_class->default_icon_name = "gtk-select-color";
  viewable_class->get_preview_size  = ligma_palette_get_preview_size;
  viewable_class->get_popup_size    = ligma_palette_get_popup_size;
  viewable_class->get_new_preview   = ligma_palette_get_new_preview;
  viewable_class->get_description   = ligma_palette_get_description;

  data_class->save                  = ligma_palette_save;
  data_class->get_extension         = ligma_palette_get_extension;
  data_class->copy                  = ligma_palette_copy;
}

static void
ligma_palette_tagged_iface_init (LigmaTaggedInterface *iface)
{
  iface->get_checksum = ligma_palette_get_checksum;
}

static void
ligma_palette_init (LigmaPalette *palette)
{
  palette->colors    = NULL;
  palette->n_colors  = 0;
  palette->n_columns = 0;
}

static void
ligma_palette_finalize (GObject *object)
{
  LigmaPalette *palette = LIGMA_PALETTE (object);

  if (palette->colors)
    {
      g_list_free_full (palette->colors,
                        (GDestroyNotify) ligma_palette_entry_free);
      palette->colors = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_palette_get_memsize (LigmaObject *object,
                          gint64     *gui_size)
{
  LigmaPalette *palette = LIGMA_PALETTE (object);
  gint64       memsize = 0;

  memsize += ligma_g_list_get_memsize_foreach (palette->colors,
                                              (LigmaMemsizeFunc)
                                              ligma_palette_entry_get_memsize,
                                              gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_palette_get_preview_size (LigmaViewable *viewable,
                               gint          size,
                               gboolean      popup,
                               gboolean      dot_for_dot,
                               gint         *width,
                               gint         *height)
{
  *width  = size;
  *height = 1 + size / 2;
}

static gboolean
ligma_palette_get_popup_size (LigmaViewable *viewable,
                             gint          width,
                             gint          height,
                             gboolean      dot_for_dot,
                             gint         *popup_width,
                             gint         *popup_height)
{
  LigmaPalette *palette = LIGMA_PALETTE (viewable);
  gint         p_width;
  gint         p_height;

  if (! palette->n_colors)
    return FALSE;

  if (palette->n_columns)
    p_width = palette->n_columns;
  else
    p_width = MIN (palette->n_colors, 16);

  p_height = MAX (1, palette->n_colors / p_width);

  if (p_width * 4 > width || p_height * 4 > height)
    {
      *popup_width  = p_width  * 4;
      *popup_height = p_height * 4;

      return TRUE;
    }

  return FALSE;
}

static LigmaTempBuf *
ligma_palette_get_new_preview (LigmaViewable *viewable,
                              LigmaContext  *context,
                              gint          width,
                              gint          height)
{
  LigmaPalette *palette  = LIGMA_PALETTE (viewable);
  LigmaTempBuf *temp_buf;
  guchar      *buf;
  guchar      *b;
  GList       *list;
  gint         columns;
  gint         rows;
  gint         cell_size;
  gint         x, y;

  temp_buf = ligma_temp_buf_new (width, height, babl_format ("R'G'B' u8"));
  memset (ligma_temp_buf_get_data (temp_buf), 255, width * height * 3);

  if (palette->n_columns > 1)
    cell_size = MAX (4, width / palette->n_columns);
  else
    cell_size = 4;

  columns = width  / cell_size;
  rows    = height / cell_size;

  buf = ligma_temp_buf_get_data (temp_buf);
  b   = g_new (guchar, width * 3);

  list = palette->colors;

  for (y = 0; y < rows && list; y++)
    {
      gint i;

      memset (b, 255, width * 3);

      for (x = 0; x < columns && list; x++)
        {
          LigmaPaletteEntry *entry = list->data;

          list = g_list_next (list);

          ligma_rgb_get_uchar (&entry->color,
                              &b[x * cell_size * 3 + 0],
                              &b[x * cell_size * 3 + 1],
                              &b[x * cell_size * 3 + 2]);

          for (i = 1; i < cell_size; i++)
            {
              b[(x * cell_size + i) * 3 + 0] = b[(x * cell_size) * 3 + 0];
              b[(x * cell_size + i) * 3 + 1] = b[(x * cell_size) * 3 + 1];
              b[(x * cell_size + i) * 3 + 2] = b[(x * cell_size) * 3 + 2];
            }
        }

      for (i = 0; i < cell_size; i++)
        memcpy (buf + ((y * cell_size + i) * width) * 3, b, width * 3);
    }

  g_free (b);

  return temp_buf;
}

static gchar *
ligma_palette_get_description (LigmaViewable  *viewable,
                              gchar        **tooltip)
{
  LigmaPalette *palette = LIGMA_PALETTE (viewable);

  return g_strdup_printf ("%s (%d)",
                          ligma_object_get_name (palette),
                          palette->n_colors);
}

LigmaData *
ligma_palette_new (LigmaContext *context,
                  const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  return g_object_new (LIGMA_TYPE_PALETTE,
                       "name", name,
                       NULL);
}

LigmaData *
ligma_palette_get_standard (LigmaContext *context)
{
  static LigmaData *standard_palette = NULL;

  if (! standard_palette)
    {
      standard_palette = ligma_palette_new (context, "Standard");

      ligma_data_clean (standard_palette);
      ligma_data_make_internal (standard_palette, "ligma-palette-standard");

      g_object_add_weak_pointer (G_OBJECT (standard_palette),
                                 (gpointer *) &standard_palette);
    }

  return standard_palette;
}

static const gchar *
ligma_palette_get_extension (LigmaData *data)
{
  return LIGMA_PALETTE_FILE_EXTENSION;
}

static void
ligma_palette_copy (LigmaData *data,
                   LigmaData *src_data)
{
  LigmaPalette *palette     = LIGMA_PALETTE (data);
  LigmaPalette *src_palette = LIGMA_PALETTE (src_data);
  GList       *list;

  ligma_data_freeze (data);

  if (palette->colors)
    {
      g_list_free_full (palette->colors,
                        (GDestroyNotify) ligma_palette_entry_free);
      palette->colors = NULL;
    }

  palette->n_colors  = 0;
  palette->n_columns = src_palette->n_columns;

  for (list = src_palette->colors; list; list = g_list_next (list))
    {
      LigmaPaletteEntry *entry = list->data;

      ligma_palette_add_entry (palette, -1, entry->name, &entry->color);
    }

  ligma_data_thaw (data);
}

static gchar *
ligma_palette_get_checksum (LigmaTagged *tagged)
{
  LigmaPalette *palette         = LIGMA_PALETTE (tagged);
  gchar       *checksum_string = NULL;

  if (palette->n_colors > 0)
    {
      GChecksum *checksum       = g_checksum_new (G_CHECKSUM_MD5);
      GList     *color_iterator = palette->colors;

      g_checksum_update (checksum, (const guchar *) &palette->n_colors, sizeof (palette->n_colors));
      g_checksum_update (checksum, (const guchar *) &palette->n_columns, sizeof (palette->n_columns));

      while (color_iterator)
        {
          LigmaPaletteEntry *entry = (LigmaPaletteEntry *) color_iterator->data;

          g_checksum_update (checksum, (const guchar *) &entry->color, sizeof (entry->color));
          if (entry->name)
            g_checksum_update (checksum, (const guchar *) entry->name, strlen (entry->name));

          color_iterator = g_list_next (color_iterator);
        }

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }

  return checksum_string;
}


/*  public functions  */

GList *
ligma_palette_get_colors (LigmaPalette *palette)
{
  g_return_val_if_fail (LIGMA_IS_PALETTE (palette), NULL);

  return palette->colors;
}

gint
ligma_palette_get_n_colors (LigmaPalette *palette)
{
  g_return_val_if_fail (LIGMA_IS_PALETTE (palette), 0);

  return palette->n_colors;
}

void
ligma_palette_move_entry (LigmaPalette      *palette,
                         LigmaPaletteEntry *entry,
                         gint              position)
{
  g_return_if_fail (LIGMA_IS_PALETTE (palette));
  g_return_if_fail (entry != NULL);

  if (g_list_find (palette->colors, entry))
    {
      palette->colors = g_list_remove (palette->colors,
                                       entry);
      palette->colors = g_list_insert (palette->colors,
                                       entry, position);

      ligma_data_dirty (LIGMA_DATA (palette));
    }
}

LigmaPaletteEntry *
ligma_palette_add_entry (LigmaPalette   *palette,
                        gint           position,
                        const gchar   *name,
                        const LigmaRGB *color)
{
  LigmaPaletteEntry *entry;

  g_return_val_if_fail (LIGMA_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (color != NULL, NULL);

  entry = g_slice_new0 (LigmaPaletteEntry);

  entry->color = *color;
  entry->name  = g_strdup (name ? name : _("Untitled"));

  if (position < 0 || position >= palette->n_colors)
    {
      palette->colors = g_list_append (palette->colors, entry);
    }
  else
    {
      palette->colors = g_list_insert (palette->colors, entry, position);
    }

  palette->n_colors += 1;

  ligma_data_dirty (LIGMA_DATA (palette));

  return entry;
}

void
ligma_palette_delete_entry (LigmaPalette      *palette,
                           LigmaPaletteEntry *entry)
{
  g_return_if_fail (LIGMA_IS_PALETTE (palette));
  g_return_if_fail (entry != NULL);

  if (g_list_find (palette->colors, entry))
    {
      ligma_palette_entry_free (entry);

      palette->colors = g_list_remove (palette->colors, entry);

      palette->n_colors--;

      ligma_data_dirty (LIGMA_DATA (palette));
    }
}

gboolean
ligma_palette_set_entry (LigmaPalette   *palette,
                        gint           position,
                        const gchar   *name,
                        const LigmaRGB *color)
{
  LigmaPaletteEntry *entry;

  g_return_val_if_fail (LIGMA_IS_PALETTE (palette), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  entry = ligma_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  entry->color = *color;

  if (entry->name)
    g_free (entry->name);

  entry->name = g_strdup (name);

  ligma_data_dirty (LIGMA_DATA (palette));

  return TRUE;
}

gboolean
ligma_palette_set_entry_color (LigmaPalette   *palette,
                              gint           position,
                              const LigmaRGB *color)
{
  LigmaPaletteEntry *entry;

  g_return_val_if_fail (LIGMA_IS_PALETTE (palette), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  entry = ligma_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  entry->color = *color;

  ligma_data_dirty (LIGMA_DATA (palette));

  return TRUE;
}

gboolean
ligma_palette_set_entry_name (LigmaPalette *palette,
                             gint         position,
                             const gchar *name)
{
  LigmaPaletteEntry *entry;

  g_return_val_if_fail (LIGMA_IS_PALETTE (palette), FALSE);

  entry = ligma_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  if (entry->name)
    g_free (entry->name);

  entry->name = g_strdup (name);

  ligma_data_dirty (LIGMA_DATA (palette));

  return TRUE;
}

LigmaPaletteEntry *
ligma_palette_get_entry (LigmaPalette *palette,
                        gint         position)
{
  g_return_val_if_fail (LIGMA_IS_PALETTE (palette), NULL);

  return g_list_nth_data (palette->colors, position);
}

gint
ligma_palette_get_entry_position (LigmaPalette      *palette,
                                 LigmaPaletteEntry *entry)
{
  g_return_val_if_fail (LIGMA_IS_PALETTE (palette), -1);
  g_return_val_if_fail (entry != NULL, -1);

  return g_list_index (palette->colors, entry);
}

void
ligma_palette_set_columns (LigmaPalette *palette,
                          gint         columns)
{
  g_return_if_fail (LIGMA_IS_PALETTE (palette));

  columns = CLAMP (columns, 0, 64);

  if (palette->n_columns != columns)
    {
      palette->n_columns = columns;

      ligma_data_dirty (LIGMA_DATA (palette));
    }
}

gint
ligma_palette_get_columns (LigmaPalette *palette)
{
  g_return_val_if_fail (LIGMA_IS_PALETTE (palette), 0);

  return palette->n_columns;
}

LigmaPaletteEntry *
ligma_palette_find_entry (LigmaPalette      *palette,
                         const LigmaRGB    *color,
                         LigmaPaletteEntry *start_from)
{
  LigmaPaletteEntry *entry;

  g_return_val_if_fail (LIGMA_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (color != NULL, NULL);

  if (! start_from)
    {
      GList *list;

      /* search from the start */

      for (list = palette->colors; list; list = g_list_next (list))
        {
          entry = (LigmaPaletteEntry *) list->data;
          if (ligma_rgb_distance (&entry->color, color) < RGB_EPSILON)
            return entry;
        }
    }
  else if (ligma_rgb_distance (&start_from->color, color) < RGB_EPSILON)
    {
      return start_from;
    }
  else
    {
      GList *old = g_list_find (palette->colors, start_from);
      GList *next;
      GList *prev;

      g_return_val_if_fail (old != NULL, NULL);

      next = old->next;
      prev = old->prev;

      /* proximity-based search */

      while (next || prev)
        {
          if (next)
            {
              entry = (LigmaPaletteEntry *) next->data;
              if (ligma_rgb_distance (&entry->color, color) < RGB_EPSILON)
                return entry;

              next = next->next;
            }

          if (prev)
            {
              entry = (LigmaPaletteEntry *) prev->data;
              if (ligma_rgb_distance (&entry->color, color) < RGB_EPSILON)
                return entry;

              prev = prev->prev;
            }
        }
    }

  return NULL;
}


/*  private functions  */

static void
ligma_palette_entry_free (LigmaPaletteEntry *entry)
{
  g_return_if_fail (entry != NULL);

  g_free (entry->name);

  g_slice_free (LigmaPaletteEntry, entry);
}

static gint64
ligma_palette_entry_get_memsize (LigmaPaletteEntry *entry,
                                gint64           *gui_size)
{
  gint64 memsize = sizeof (LigmaPaletteEntry);

  memsize += ligma_string_get_memsize (entry->name);

  return memsize;
}
