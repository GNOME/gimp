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

#include "gegl/gimp-babl.h"

#include "gimp-memsize.h"
#include "gimpimage.h"
#include "gimpimage-undo-push.h"
#include "gimppalette.h"
#include "gimppalette-load.h"
#include "gimppalette-save.h"
#include "gimptagged.h"
#include "gimptempbuf.h"

#include "gimp-intl.h"


#define RGB_EPSILON 1e-6

enum
{
  ENTRY_CHANGED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void          gimp_palette_tagged_iface_init   (GimpTaggedInterface  *iface);

static void          gimp_palette_finalize            (GObject              *object);

static gint64        gimp_palette_get_memsize         (GimpObject           *object,
                                                       gint64               *gui_size);

static void          gimp_palette_get_preview_size    (GimpViewable         *viewable,
                                                       gint                  size,
                                                       gboolean              popup,
                                                       gboolean              dot_for_dot,
                                                       gint                 *width,
                                                       gint                 *height);
static gboolean      gimp_palette_get_popup_size      (GimpViewable         *viewable,
                                                       gint                  width,
                                                       gint                  height,
                                                       gboolean              dot_for_dot,
                                                       gint                 *popup_width,
                                                       gint                 *popup_height);
static GimpTempBuf * gimp_palette_get_new_preview     (GimpViewable         *viewable,
                                                       GimpContext          *context,
                                                       gint                  width,
                                                       gint                  height,
                                                       GeglColor            *fg_color);
static gchar       * gimp_palette_get_description     (GimpViewable         *viewable,
                                                       gchar               **tooltip);
static const gchar * gimp_palette_get_extension       (GimpData             *data);
static void          gimp_palette_copy                (GimpData             *data,
                                                       GimpData             *src_data);
static void          gimp_palette_real_entry_changed  (GimpPalette         *palette,
                                                       gint                 index);

static void          gimp_palette_entry_free          (GimpPaletteEntry     *entry);
static gint64        gimp_palette_entry_get_memsize   (GimpPaletteEntry     *entry,
                                                       gint64               *gui_size);
static gchar       * gimp_palette_get_checksum        (GimpTagged           *tagged);

static void          gimp_palette_image_space_updated (GimpImage            *image,
                                                       GimpPalette          *palette);
static void          gimp_palette_notify_image        (GimpPalette          *palette,
                                                       const GParamSpec     *unused,
                                                       gpointer              unused_user_data);


G_DEFINE_TYPE_WITH_CODE (GimpPalette, gimp_palette, GIMP_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_TAGGED,
                                                gimp_palette_tagged_iface_init))

#define parent_class gimp_palette_parent_class

static guint signals[LAST_SIGNAL] = { 0 };

static void
gimp_palette_class_init (GimpPaletteClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  signals[ENTRY_CHANGED] = g_signal_new ("entry-changed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_FIRST,
                                         G_STRUCT_OFFSET (GimpPaletteClass, entry_changed),
                                         NULL, NULL, NULL,
                                         G_TYPE_NONE, 1,
                                         G_TYPE_INT);

  object_class->finalize            = gimp_palette_finalize;

  gimp_object_class->get_memsize    = gimp_palette_get_memsize;

  viewable_class->default_icon_name = "gtk-select-color";
  viewable_class->default_name      = _("Palette");
  viewable_class->get_preview_size  = gimp_palette_get_preview_size;
  viewable_class->get_popup_size    = gimp_palette_get_popup_size;
  viewable_class->get_new_preview   = gimp_palette_get_new_preview;
  viewable_class->get_description   = gimp_palette_get_description;

  data_class->save                  = gimp_palette_save;
  data_class->get_extension         = gimp_palette_get_extension;
  data_class->copy                  = gimp_palette_copy;

  klass->entry_changed              = gimp_palette_real_entry_changed;
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
  palette->image     = NULL;
  palette->format    = NULL;

  g_signal_connect (palette, "notify::image",
                    G_CALLBACK (gimp_palette_notify_image),
                    NULL);
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

  g_clear_weak_pointer (&palette->image);

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
                              gint          height,
                              GeglColor    *fg_color G_GNUC_UNUSED)
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

          gegl_color_get_pixel (entry->color, babl_format ("R'G'B' u8"), &b[x * cell_size * 3]);

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
      g_set_weak_pointer (&standard_palette,
                          gimp_palette_new (context, "Standard"));

      gimp_data_clean (standard_palette);
      gimp_data_make_internal (standard_palette, "gimp-palette-standard");
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
  palette->format    = src_palette->format;
  /* Note: we don't copy the image variable on purpose (same as we don't
   * copy the one from the parent GimpData, nor the file), and we don't
   * want to keep syncing our format with changes in this image.
   */

  for (list = src_palette->colors; list; list = g_list_next (list))
    {
      GimpPaletteEntry *entry = list->data;

      /* Small validity check. */
      g_return_if_fail (! palette->format || palette->format == gegl_color_get_format (entry->color));
      gimp_palette_add_entry (palette, -1, entry->name, entry->color);
    }

  gimp_data_thaw (data);
}

static void
gimp_palette_real_entry_changed (GimpPalette *palette,
                                 gint         index)
{
  GimpImage *image = gimp_data_get_image (GIMP_DATA (palette));

  if (image != NULL)
    gimp_image_colormap_changed (image, index);
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

static void
gimp_palette_image_space_updated (GimpImage   *image,
                                  GimpPalette *palette)
{
  const Babl *space;
  const Babl *format;

  space  = gimp_image_get_layer_space (image);
  format = gimp_babl_format (GIMP_RGB, gimp_image_get_precision (image), FALSE, space);
  gimp_palette_restrict_format (palette, format, FALSE);
}

static void
gimp_palette_notify_image (GimpPalette      *palette,
                           const GParamSpec *unused,
                           gpointer          unused_user_data)
{
  GimpImage *image = gimp_data_get_image (GIMP_DATA (palette));

  if (palette->image == image)
    return;

  if (palette->image)
    g_signal_handlers_disconnect_by_func (palette->image,
                                          G_CALLBACK (gimp_palette_image_space_updated),
                                          palette);

  g_set_weak_pointer (&palette->image, image);

  if (image)
    {
      /* Note: I only connect to "precision-changed", and not
       * "profile-changed" changes which is handled in
       * gimp_image_convert_profile_colormap() with additional bpc
       * argument.
       * In current implementation of indexed images,
       * "precision-changed" should not happen because it is always
       * 8-bit non-linear.
       */
      g_signal_connect_object (image,
                               "precision-changed",
                               G_CALLBACK (gimp_palette_image_space_updated),
                               palette, 0);

      gimp_palette_image_space_updated (image, palette);
    }
  else
    {
      gimp_palette_restrict_format (palette, NULL, FALSE);
    }
}


/*  public functions  */

void
gimp_palette_restrict_format (GimpPalette *palette,
                              const Babl  *format,
                              gboolean     push_undo_if_image)
{
  gint n_colors;

  g_return_if_fail (GIMP_IS_PALETTE (palette));

  if (palette->format == format)
    return;

  if (push_undo_if_image && gimp_data_get_image (GIMP_DATA (palette)))
    gimp_image_undo_push_image_colormap (gimp_data_get_image (GIMP_DATA (palette)),
                                         C_("undo-type", "Change Colormap format restriction"));
  palette->format = format;

  if (palette->format == NULL)
    /* No more restriction. */
    return;

  /* Convert all colors to the new format. */
  n_colors = gimp_palette_get_n_colors (palette);
  for (gint i = 0; i < n_colors; i++)
    {
      GimpPaletteEntry *entry;
      guint8            pixel[40];

      entry = gimp_palette_get_entry (palette, i);
      gegl_color_get_pixel (entry->color, format, pixel);
      gegl_color_set_pixel (entry->color, format, pixel);
    }
}

const Babl *
gimp_palette_get_restriction (GimpPalette *palette)
{
  g_return_val_if_fail (GIMP_IS_PALETTE (palette), NULL);

  return palette->format;
}

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
  g_return_if_fail (GIMP_IS_PALETTE (palette));
  g_return_if_fail (entry != NULL);

  if (g_list_find (palette->colors, entry))
    {
      gint old_position = g_list_index (palette->colors, entry);

      palette->colors = g_list_remove (palette->colors,
                                       entry);
      palette->colors = g_list_insert (palette->colors,
                                       entry, position);

      if (! gimp_data_is_frozen (GIMP_DATA (palette)))
        for (gint i = MIN (position, old_position); i <= MAX (position, old_position); i++)
          g_signal_emit (palette, signals[ENTRY_CHANGED], 0, i);

      gimp_data_dirty (GIMP_DATA (palette));
    }
}

GimpPaletteEntry *
gimp_palette_add_entry (GimpPalette   *palette,
                        gint           position,
                        const gchar   *name,
                        GeglColor     *color)
{
  GimpPaletteEntry *entry;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  entry = g_slice_new0 (GimpPaletteEntry);

  entry->color = gegl_color_duplicate (color);
  entry->name  = g_strdup (name ? name : _("Untitled"));

  if (palette->format != NULL)
    {
      guint8 pixel[40];

      gegl_color_get_pixel (entry->color, palette->format, pixel);
      gegl_color_set_pixel (entry->color, palette->format, pixel);
    }

  if (position < 0 || position >= palette->n_colors)
    {
      palette->colors = g_list_append (palette->colors, entry);
    }
  else
    {
      palette->colors = g_list_insert (palette->colors, entry, position);
    }

  palette->n_colors += 1;

  if (! gimp_data_is_frozen (GIMP_DATA (palette)))
    for (gint i = position; i < palette->n_colors; i++)
      g_signal_emit (palette, signals[ENTRY_CHANGED], 0, i);

  gimp_data_dirty (GIMP_DATA (palette));

  return entry;
}

void
gimp_palette_delete_entry (GimpPalette      *palette,
                           GimpPaletteEntry *entry)
{
  g_return_if_fail (GIMP_IS_PALETTE (palette));
  g_return_if_fail (entry != NULL);

  if (g_list_find (palette->colors, entry))
    {
      gint old_position = g_list_index (palette->colors, entry);

      gimp_palette_entry_free (entry);

      palette->colors = g_list_remove (palette->colors, entry);

      palette->n_colors--;

      if (! gimp_data_is_frozen (GIMP_DATA (palette)))
        for (gint i = old_position; i < palette->n_colors; i++)
          g_signal_emit (palette, signals[ENTRY_CHANGED], 0, i);

      gimp_data_dirty (GIMP_DATA (palette));
    }
}

gboolean
gimp_palette_set_entry (GimpPalette   *palette,
                        gint           position,
                        const gchar   *name,
                        GeglColor     *color)
{
  GimpPaletteEntry *entry;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), FALSE);
  g_return_val_if_fail (GEGL_IS_COLOR (color), FALSE);

  entry = gimp_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  g_clear_object (&entry->color);
  entry->color = gegl_color_duplicate (color);
  if (palette->format != NULL)
    {
      guint8 pixel[40];

      gegl_color_get_pixel (entry->color, palette->format, pixel);
      gegl_color_set_pixel (entry->color, palette->format, pixel);
    }

  if (entry->name)
    g_free (entry->name);

  entry->name = g_strdup (name);

  if (! gimp_data_is_frozen (GIMP_DATA (palette)))
    g_signal_emit (palette, signals[ENTRY_CHANGED], 0, position);

  gimp_data_dirty (GIMP_DATA (palette));

  return TRUE;
}

gboolean
gimp_palette_set_entry_color (GimpPalette   *palette,
                              gint           position,
                              GeglColor     *color,
                              gboolean       push_undo_if_image)
{
  GimpPaletteEntry *entry;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), FALSE);
  g_return_val_if_fail (GEGL_IS_COLOR (color), FALSE);

  entry = gimp_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  if (push_undo_if_image && gimp_data_get_image (GIMP_DATA (palette)))
    gimp_image_undo_push_image_colormap (gimp_data_get_image (GIMP_DATA (palette)),
                                         C_("undo-type", "Change Colormap entry"));

  g_clear_object (&entry->color);
  entry->color = gegl_color_duplicate (color);
  if (palette->format != NULL)
    {
      guint8 pixel[40];

      gegl_color_get_pixel (entry->color, palette->format, pixel);
      gegl_color_set_pixel (entry->color, palette->format, pixel);
    }

  if (! gimp_data_is_frozen (GIMP_DATA (palette)))
    g_signal_emit (palette, signals[ENTRY_CHANGED], 0, position);

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

  if (! gimp_data_is_frozen (GIMP_DATA (palette)))
    g_signal_emit (palette, signals[ENTRY_CHANGED], 0, position);

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

gint
gimp_palette_get_entry_position (GimpPalette      *palette,
                                 GimpPaletteEntry *entry)
{
  g_return_val_if_fail (GIMP_IS_PALETTE (palette), -1);
  g_return_val_if_fail (entry != NULL, -1);

  return g_list_index (palette->colors, entry);
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
                         GeglColor        *color,
                         GimpPaletteEntry *start_from)
{
  GimpPaletteEntry *entry;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  if (! start_from)
    {
      GList *list;

      /* search from the start */

      for (list = palette->colors; list; list = g_list_next (list))
        {
          entry = (GimpPaletteEntry *) list->data;
          if (gimp_color_is_perceptually_identical (entry->color, color))
            return entry;
        }
    }
  else if (gimp_color_is_perceptually_identical (start_from->color, color))
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
              if (gimp_color_is_perceptually_identical (entry->color, color))
                return entry;

              next = next->next;
            }

          if (prev)
            {
              entry = (GimpPaletteEntry *) prev->data;
              if (gimp_color_is_perceptually_identical (entry->color, color))
                return entry;

              prev = prev->prev;
            }
        }
    }

  return NULL;
}

guchar *
gimp_palette_get_colormap (GimpPalette *palette,
                           const Babl  *format,
                           gint        *n_colors)
{
  guchar *colormap = NULL;
  gint    bpp;

  g_return_val_if_fail (GIMP_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (n_colors != NULL, NULL);

  bpp = babl_format_get_bytes_per_pixel (format);

  *n_colors = gimp_palette_get_n_colors (palette);

  if (*n_colors > 0)
    {
      guchar *p;

      colormap = g_new0 (guchar, bpp * *n_colors);
      p        = colormap;

      for (gint i = 0; i < *n_colors; i++)
        {
          GimpPaletteEntry *entry = gimp_palette_get_entry (palette, i);

          gegl_color_get_pixel (entry->color, format, p);
          p += bpp;
        }
    }

  return colormap;
}

void
gimp_palette_set_colormap (GimpPalette  *palette,
                           const Babl   *format,
                           const guchar *colormap,
                           gint          n_colors,
                           gboolean      push_undo_if_image)
{
  GimpPaletteEntry *entry;
  GeglColor        *color;
  gchar             name[64];
  gint              bpp;
  gint              i;

  g_return_if_fail (GIMP_IS_PALETTE (palette));
  g_return_if_fail (format != NULL);
  g_return_if_fail (n_colors > 0);

  if (push_undo_if_image && gimp_data_get_image (GIMP_DATA (palette)))
    gimp_image_undo_push_image_colormap (gimp_data_get_image (GIMP_DATA (palette)),
                                         C_("undo-type", "Set Colormap"));

  if (gimp_data_get_image (GIMP_DATA (palette)))
    n_colors = MIN (n_colors, 256);

  gimp_data_freeze (GIMP_DATA (palette));

  while ((entry = gimp_palette_get_entry (palette, 0)))
    gimp_palette_delete_entry (palette, entry);

  bpp = babl_format_get_bytes_per_pixel (format);

  color = gegl_color_new (NULL);
  for (i = 0; i < n_colors; i++)
    {
      gegl_color_set_pixel (color, format, &colormap[i * bpp]);
      g_snprintf (name, sizeof (name), "#%d", i);
      gimp_palette_add_entry (palette, i, name, color);
    }
  g_object_unref (color);

  gimp_data_thaw (GIMP_DATA (palette));

  if (! gimp_data_is_frozen (GIMP_DATA (palette)))
    for (gint i = 0; i < n_colors; i++)
      g_signal_emit (palette, signals[ENTRY_CHANGED], 0, i);
}


/*  private functions  */

static void
gimp_palette_entry_free (GimpPaletteEntry *entry)
{
  g_return_if_fail (entry != NULL);

  g_free (entry->name);
  g_clear_object (&entry->color);

  g_slice_free (GimpPaletteEntry, entry);
}

static gint64
gimp_palette_entry_get_memsize (GimpPaletteEntry *entry,
                                gint64           *gui_size)
{
  gint64 memsize = sizeof (GimpPaletteEntry);

  memsize += gimp_string_get_memsize (entry->name);
  /* the GeglColor is missing here */

  return memsize;
}
