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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimp-utils.h"
#include "gimppalette.h"
#include "gimppalette-load.h"
#include "gimppalette-save.h"

#include "gimp-intl.h"

#define EPSILON 1e-10

/*  local function prototypes  */

static void       gimp_palette_finalize         (GObject           *object);

static gint64     gimp_palette_get_memsize      (GimpObject        *object,
                                                 gint64            *gui_size);

static void       gimp_palette_get_preview_size (GimpViewable      *viewable,
                                                 gint               size,
                                                 gboolean           popup,
                                                 gboolean           dot_for_dot,
                                                 gint              *width,
                                                 gint              *height);
static gboolean   gimp_palette_get_popup_size   (GimpViewable      *viewable,
                                                 gint               width,
                                                 gint               height,
                                                 gboolean           dot_for_dot,
                                                 gint              *popup_width,
                                                 gint              *popup_height);
static TempBuf  * gimp_palette_get_new_preview  (GimpViewable      *viewable,
                                                 GimpContext       *context,
                                                 gint               width,
                                                 gint               height);
static gchar    * gimp_palette_get_description  (GimpViewable      *viewable,
                                                 gchar            **tooltip);
static gchar    * gimp_palette_get_extension    (GimpData          *data);
static GimpData * gimp_palette_duplicate        (GimpData          *data);

static void       gimp_palette_entry_free       (GimpPaletteEntry  *entry);
static gint64     gimp_palette_entry_get_memsize(GimpPaletteEntry  *entry,
                                                 gint64            *gui_size);


G_DEFINE_TYPE (GimpPalette, gimp_palette, GIMP_TYPE_DATA)

#define parent_class gimp_palette_parent_class


static void
gimp_palette_class_init (GimpPaletteClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  object_class->finalize           = gimp_palette_finalize;

  gimp_object_class->get_memsize   = gimp_palette_get_memsize;

  viewable_class->default_stock_id = "gtk-select-color";
  viewable_class->get_preview_size = gimp_palette_get_preview_size;
  viewable_class->get_popup_size   = gimp_palette_get_popup_size;
  viewable_class->get_new_preview  = gimp_palette_get_new_preview;
  viewable_class->get_description  = gimp_palette_get_description;

  data_class->save                 = gimp_palette_save;
  data_class->get_extension        = gimp_palette_get_extension;
  data_class->duplicate            = gimp_palette_duplicate;
}

static void
gimp_palette_init (GimpPalette *palette)
{
  palette->colors    = NULL;
  palette->n_colors  = 0;
  palette->n_columns = 0;
}

static void
gimp_palette_finalize (GObject *object)
{
  GimpPalette *palette = GIMP_PALETTE (object);

  if (palette->colors)
    {
      g_list_foreach (palette->colors, (GFunc) gimp_palette_entry_free, NULL);
      g_list_free (palette->colors);
      palette->colors = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_palette_get_memsize (GimpObject *object,
                          gint64     *gui_size)
{
  GimpPalette *palette = GIMP_PALETTE (object);
  gint64       memsize = 0;

  memsize += gimp_g_list_get_memsize_foreach (palette->colors,
                                              (GimpMemsizeFunc)
                                              gimp_palette_entry_get_memsize,
                                              gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_palette_get_preview_size (GimpViewable *viewable,
                               gint          size,
                               gboolean      popup,
                               gboolean      dot_for_dot,
                               gint         *width,
                               gint         *height)
{
  *width  = size;
  *height = size / 2;
}

static gboolean
gimp_palette_get_popup_size (GimpViewable *viewable,
                             gint          width,
                             gint          height,
                             gboolean      dot_for_dot,
                             gint         *popup_width,
                             gint         *popup_height)
{
  GimpPalette *palette = GIMP_PALETTE (viewable);
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

static TempBuf *
gimp_palette_get_new_preview (GimpViewable *viewable,
                              GimpContext  *context,
                              gint          width,
                              gint          height)
{
  GimpPalette *palette  = GIMP_PALETTE (viewable);
  TempBuf     *temp_buf;
  guchar      *buf;
  guchar      *b;
  GList       *list;
  guchar       white[3] = { 255, 255, 255 };
  gint         columns;
  gint         rows;
  gint         cell_size;
  gint         x, y;

  temp_buf = temp_buf_new (width, height, 3, 0, 0, white);

  if (palette->n_columns > 1)
    cell_size = MAX (4, width / palette->n_columns);
  else
    cell_size = 4;

  columns = width  / cell_size;
  rows    = height / cell_size;

  buf = temp_buf_data (temp_buf);
  b   = g_new (guchar, width * 3);

  list = palette->colors;

  for (y = 0; y < rows && list; y++)
    {
      gint i;

      memset (b, 255, width * 3);

      for (x = 0; x < columns && list; x++)
        {
          GimpPaletteEntry *entry = list->data;

          list = g_list_next (list);

          gimp_rgb_get_uchar (&entry->color,
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
gimp_palette_get_description (GimpViewable  *viewable,
                              gchar        **tooltip)
{
  GimpPalette *palette = GIMP_PALETTE (viewable);

  return g_strdup_printf ("%s (%d)",
                          GIMP_OBJECT (palette)->name,
                          palette->n_colors);
}

GimpData *
gimp_palette_new (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  return g_object_new (GIMP_TYPE_PALETTE,
                       "name", name,
                       NULL);
}

GimpData *
gimp_palette_get_standard (void)
{
  static GimpData *standard_palette = NULL;

  if (! standard_palette)
    {
      standard_palette = gimp_palette_new ("Standard");

      standard_palette->dirty = FALSE;
      gimp_data_make_internal (standard_palette);

      g_object_ref (standard_palette);
    }

  return standard_palette;
}

static gchar *
gimp_palette_get_extension (GimpData *data)
{
  return GIMP_PALETTE_FILE_EXTENSION;
}

static GimpData *
gimp_palette_duplicate (GimpData *data)
{
  GimpPalette *palette = GIMP_PALETTE (data);
  GimpPalette *new;
  GList       *list;

  new = g_object_new (GIMP_TYPE_PALETTE, NULL);

  new->n_columns = palette->n_columns;

  for (list = palette->colors; list; list = g_list_next (list))
    {
      GimpPaletteEntry *entry = list->data;

      gimp_palette_add_entry (new, -1, entry->name, &entry->color);
    }

  return GIMP_DATA (new);
}

GimpPaletteEntry *
gimp_palette_add_entry (GimpPalette   *palette,
                        gint           position,
                        const gchar   *name,
                        const GimpRGB *color)
{
  GimpPaletteEntry *entry;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (color != NULL, NULL);

  entry = g_slice_new0 (GimpPaletteEntry);

  entry->color = *color;
  entry->name  = g_strdup (name ? name : _("Untitled"));

  if (position < 0 || position >= palette->n_colors)
    {
      entry->position = palette->n_colors;
      palette->colors = g_list_append (palette->colors, entry);
    }
  else
    {
      GList *list;

      entry->position = position;
      palette->colors = g_list_insert (palette->colors, entry, position);

      /* renumber the displaced entries */
      for (list = g_list_nth (palette->colors, position + 1);
           list;
           list = g_list_next (list))
        {
          GimpPaletteEntry *entry2 = list->data;

          entry2->position += 1;
        }
    }

  palette->n_colors += 1;

  gimp_data_dirty (GIMP_DATA (palette));

  return entry;
}

void
gimp_palette_delete_entry (GimpPalette      *palette,
                           GimpPaletteEntry *entry)
{
  GList *list;
  gint   pos = 0;

  g_return_if_fail (GIMP_IS_PALETTE (palette));
  g_return_if_fail (entry != NULL);

  if (g_list_find (palette->colors, entry))
    {
      pos = entry->position;
      gimp_palette_entry_free (entry);

      palette->colors = g_list_remove (palette->colors, entry);

      palette->n_colors--;

      for (list = g_list_nth (palette->colors, pos);
           list;
           list = g_list_next (list))
        {
          entry = (GimpPaletteEntry *) list->data;

          entry->position = pos++;
        }

      gimp_data_dirty (GIMP_DATA (palette));
    }
}

void
gimp_palette_set_columns (GimpPalette *palette,
                          gint         columns)
{
  g_return_if_fail (GIMP_IS_PALETTE (palette));

  columns = CLAMP (columns, 0, 64);

  if (palette->n_columns != columns)
    {
      palette->n_columns = columns;

      gimp_data_dirty (GIMP_DATA (palette));
    }
}

gint
gimp_palette_get_columns  (GimpPalette *palette)
{
  g_return_val_if_fail (GIMP_IS_PALETTE (palette), 0);

  return palette->n_columns;
}

GimpPaletteEntry *
gimp_palette_find_entry (GimpPalette      *palette,
                         const GimpRGB    *color,
                         GimpPaletteEntry *start_from)
{
  GimpPaletteEntry *entry;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (color != NULL, NULL);
  g_return_val_if_fail (palette->n_colors > 0, NULL);

  if (! start_from)
    {
      GList *list;

      /* search from the start */

      for (list = palette->colors; list; list = g_list_next (list))
        {
          entry = (GimpPaletteEntry *) list->data;
          if (gimp_rgb_distance (&entry->color, color) < EPSILON)
            return entry;
        }
    }
  else if (gimp_rgb_distance (&start_from->color, color) < EPSILON)
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
              entry = (GimpPaletteEntry *) next->data;
              if (gimp_rgb_distance (&entry->color, color) < EPSILON)
                return entry;

              next = next->next;
            }

          if (prev)
            {
              entry = (GimpPaletteEntry *) prev->data;
              if (gimp_rgb_distance (&entry->color, color) < EPSILON)
                return entry;

              prev = prev->prev;
            }
        }
    }

  return NULL;
}


/*  private functions  */

static void
gimp_palette_entry_free (GimpPaletteEntry *entry)
{
  g_return_if_fail (entry != NULL);

  g_free (entry->name);

  g_slice_free (GimpPaletteEntry, entry);
}

static gint64
gimp_palette_entry_get_memsize (GimpPaletteEntry *entry,
                                gint64           *gui_size)
{
  gint64 memsize = sizeof (GimpPaletteEntry);

  if (entry->name)
    memsize += strlen (entry->name) + 1;

  return memsize;
}
