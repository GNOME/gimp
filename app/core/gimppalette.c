/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimp-memsize.h"
#include "gimppalette.h"
#include "gimppalette-load.h"
#include "gimppalette-save.h"
#include "gimptagged.h"
#include "gimptempbuf.h"

#include "gimp-intl.h"


#define RGB_EPSILON 1e-6


/*  local function prototypes  */

static void          gimp_palette_tagged_iface_init (GimpTaggedInterface  *iface);

static void          gimp_palette_finalize          (GObject              *object);

static gint64        gimp_palette_get_memsize       (GimpObject           *object,
                                                     gint64               *gui_size);

static void          gimp_palette_get_preview_size  (GimpViewable         *viewable,
                                                     gint                  size,
                                                     gboolean              popup,
                                                     gboolean              dot_for_dot,
                                                     gint                 *width,
                                                     gint                 *height);
static gboolean      gimp_palette_get_popup_size    (GimpViewable         *viewable,
                                                     gint                  width,
                                                     gint                  height,
                                                     gboolean              dot_for_dot,
                                                     gint                 *popup_width,
                                                     gint                 *popup_height);
static GimpTempBuf * gimp_palette_get_new_preview   (GimpViewable         *viewable,
                                                     GimpContext          *context,
                                                     gint                  width,
                                                     gint                  height);
static gchar       * gimp_palette_get_description   (GimpViewable         *viewable,
                                                     gchar               **tooltip);
static const gchar * gimp_palette_get_extension     (GimpData             *data);
static void          gimp_palette_copy              (GimpData             *data,
                                                     GimpData             *src_data);

static void          gimp_palette_entry_free        (GimpPaletteEntry     *entry);
static gint64        gimp_palette_entry_get_memsize (GimpPaletteEntry     *entry,
                                                     gint64               *gui_size);
static gchar       * gimp_palette_get_checksum      (GimpTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (GimpPalette, gimp_palette, GIMP_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_TAGGED,
                                                gimp_palette_tagged_iface_init))

#define parent_class gimp_palette_parent_class


static void
gimp_palette_class_init (GimpPaletteClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  object_class->finalize            = gimp_palette_finalize;

  gimp_object_class->get_memsize    = gimp_palette_get_memsize;

  viewable_class->default_icon_name = "gtk-select-color";
  viewable_class->get_preview_size  = gimp_palette_get_preview_size;
  viewable_class->get_popup_size    = gimp_palette_get_popup_size;
  viewable_class->get_new_preview   = gimp_palette_get_new_preview;
  viewable_class->get_description   = gimp_palette_get_description;

  data_class->save                  = gimp_palette_save;
  data_class->get_extension         = gimp_palette_get_extension;
  data_class->copy                  = gimp_palette_copy;
}

static void
gimp_palette_tagged_iface_init (GimpTaggedInterface *iface)
{
  iface->get_checksum = gimp_palette_get_checksum;
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
      g_list_free_full (palette->colors,
                        (GDestroyNotify) gimp_palette_entry_free);
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
  *height = 1 + size / 2;
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

static GimpTempBuf *
gimp_palette_get_new_preview (GimpViewable *viewable,
                              GimpContext  *context,
                              gint          width,
                              gint          height)
{
  GimpPalette *palette  = GIMP_PALETTE (viewable);
  GimpTempBuf *temp_buf;
  guchar      *buf;
  guchar      *b;
  GList       *list;
  gint         columns;
  gint         rows;
  gint         cell_size;
  gint         x, y;

  temp_buf = gimp_temp_buf_new (width, height, babl_format ("R'G'B' u8"));
  memset (gimp_temp_buf_get_data (temp_buf), 255, width * height * 3);

  if (palette->n_columns > 1)
    cell_size = MAX (4, width / palette->n_columns);
  else
    cell_size = 4;

  columns = width  / cell_size;
  rows    = height / cell_size;

  buf = gimp_temp_buf_get_data (temp_buf);
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
                          gimp_object_get_name (palette),
                          palette->n_colors);
}

GimpData *
gimp_palette_new (GimpContext *context,
                  const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  return g_object_new (GIMP_TYPE_PALETTE,
                       "name", name,
                       NULL);
}

GimpData *
gimp_palette_get_standard (GimpContext *context)
{
  static GimpData *standard_palette = NULL;

  if (! standard_palette)
    {
      standard_palette = gimp_palette_new (context, "Standard");

      gimp_data_clean (standard_palette);
      gimp_data_make_internal (standard_palette, "gimp-palette-standard");

      g_object_add_weak_pointer (G_OBJECT (standard_palette),
                                 (gpointer *) &standard_palette);
    }

  return standard_palette;
}

static const gchar *
gimp_palette_get_extension (GimpData *data)
{
  return GIMP_PALETTE_FILE_EXTENSION;
}

static void
gimp_palette_copy (GimpData *data,
                   GimpData *src_data)
{
  GimpPalette *palette     = GIMP_PALETTE (data);
  GimpPalette *src_palette = GIMP_PALETTE (src_data);
  GList       *list;

  gimp_data_freeze (data);

  if (palette->colors)
    {
      g_list_free_full (palette->colors,
                        (GDestroyNotify) gimp_palette_entry_free);
      palette->colors = NULL;
    }

  palette->n_colors  = 0;
  palette->n_columns = src_palette->n_columns;

  for (list = src_palette->colors; list; list = g_list_next (list))
    {
      GimpPaletteEntry *entry = list->data;

      gimp_palette_add_entry (palette, -1, entry->name, &entry->color);
    }

  gimp_data_thaw (data);
}

static gchar *
gimp_palette_get_checksum (GimpTagged *tagged)
{
  GimpPalette *palette         = GIMP_PALETTE (tagged);
  gchar       *checksum_string = NULL;

  if (palette->n_colors > 0)
    {
      GChecksum *checksum       = g_checksum_new (G_CHECKSUM_MD5);
      GList     *color_iterator = palette->colors;

      g_checksum_update (checksum, (const guchar *) &palette->n_colors, sizeof (palette->n_colors));
      g_checksum_update (checksum, (const guchar *) &palette->n_columns, sizeof (palette->n_columns));

      while (color_iterator)
        {
          GimpPaletteEntry *entry = (GimpPaletteEntry *) color_iterator->data;

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
gimp_palette_get_colors (GimpPalette *palette)
{
  g_return_val_if_fail (GIMP_IS_PALETTE (palette), NULL);

  return palette->colors;
}

gint
gimp_palette_get_n_colors (GimpPalette *palette)
{
  g_return_val_if_fail (GIMP_IS_PALETTE (palette), 0);

  return palette->n_colors;
}

void
gimp_palette_move_entry (GimpPalette      *palette,
                         GimpPaletteEntry *entry,
                         gint              position)
{
  GList *list;
  gint   pos = 0;

  g_return_if_fail (GIMP_IS_PALETTE (palette));
  g_return_if_fail (entry != NULL);

  if (g_list_find (palette->colors, entry))
    {
      pos = entry->position;

      if (entry->position == position)
        return;

      entry->position = position;
      palette->colors = g_list_remove (palette->colors,
                                       entry);
      palette->colors = g_list_insert (palette->colors,
                                       entry, position);

      if (pos < position)
        {
          for (list = g_list_nth (palette->colors, pos);
               list && pos < position;
               list = g_list_next (list))
            {
              entry = (GimpPaletteEntry *) list->data;

              entry->position = pos++;
            }
        }
      else
        {
          for (list = g_list_nth (palette->colors, position + 1);
               list && position < pos;
               list = g_list_next (list))
            {
              entry = (GimpPaletteEntry *) list->data;

              entry->position += 1;
              pos--;
            }
        }

      gimp_data_dirty (GIMP_DATA (palette));
    }
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

gboolean
gimp_palette_set_entry (GimpPalette   *palette,
                        gint           position,
                        const gchar   *name,
                        const GimpRGB *color)
{
  GimpPaletteEntry *entry;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  entry = gimp_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  entry->color = *color;

  if (entry->name)
    g_free (entry->name);

  entry->name = g_strdup (name);

  gimp_data_dirty (GIMP_DATA (palette));

  return TRUE;
}

gboolean
gimp_palette_set_entry_color (GimpPalette   *palette,
                              gint           position,
                              const GimpRGB *color)
{
  GimpPaletteEntry *entry;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  entry = gimp_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  entry->color = *color;

  gimp_data_dirty (GIMP_DATA (palette));

  return TRUE;
}

gboolean
gimp_palette_set_entry_name (GimpPalette *palette,
                             gint         position,
                             const gchar *name)
{
  GimpPaletteEntry *entry;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), FALSE);

  entry = gimp_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  if (entry->name)
    g_free (entry->name);

  entry->name = g_strdup (name);

  gimp_data_dirty (GIMP_DATA (palette));

  return TRUE;
}

GimpPaletteEntry *
gimp_palette_get_entry (GimpPalette *palette,
                        gint         position)
{
  g_return_val_if_fail (GIMP_IS_PALETTE (palette), NULL);

  return g_list_nth_data (palette->colors, position);
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
gimp_palette_get_columns (GimpPalette *palette)
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

  if (! start_from)
    {
      GList *list;

      /* search from the start */

      for (list = palette->colors; list; list = g_list_next (list))
        {
          entry = (GimpPaletteEntry *) list->data;
          if (gimp_rgb_distance (&entry->color, color) < RGB_EPSILON)
            return entry;
        }
    }
  else if (gimp_rgb_distance (&start_from->color, color) < RGB_EPSILON)
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
              if (gimp_rgb_distance (&entry->color, color) < RGB_EPSILON)
                return entry;

              next = next->next;
            }

          if (prev)
            {
              entry = (GimpPaletteEntry *) prev->data;
              if (gimp_rgb_distance (&entry->color, color) < RGB_EPSILON)
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

  memsize += gimp_string_get_memsize (entry->name);

  return memsize;
}
