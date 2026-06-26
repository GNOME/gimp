/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsavable.c
 * Copyright (C) 2026 Jehan
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpdrawable.h"
#include "gimppalette.h"
#include "gimpsavable.h"


G_DEFINE_INTERFACE (GimpSavable, gimp_savable, G_TYPE_OBJECT)


static void
gimp_savable_default_init (GimpSavableInterface *iface)
{
}


/*  public functions  */

void
gimp_savable_save (GimpSavable   *savable,
                   GOutputStream *output,
                   gint           n_ident,
                   GHashTable    *icc_references)
{
  GimpSavableInterface *savable_iface;

  g_return_if_fail (GIMP_IS_SAVABLE (savable));

  savable_iface = GIMP_SAVABLE_GET_IFACE (savable);

  if (savable_iface->save)
    savable_iface->save (savable, output, n_ident, icc_references);
}

void
gimp_savable_format_save (const Babl    *format,
                          GOutputStream *output,
                          gint           n_indent,
                          GHashTable    *space_references)
{
  const Babl *space;
  gchar      *encoding;

  g_return_if_fail (space_references != NULL);

  encoding = g_markup_escape_text (babl_format_get_encoding (format), -1);
  g_output_stream_printf (output, NULL, NULL, NULL, "%*c<format encoding='%s'>\n", n_indent, ' ', encoding);

  space = babl_format_get_space (format);
  gimp_savable_space_save (space, output, n_indent + 2, space_references, 0);

  g_output_stream_printf (output, NULL, NULL, NULL, "%*c</format>\n", n_indent, ' ');

  g_free (encoding);
}

void
gimp_savable_space_save (const Babl    *space,
                         GOutputStream *output,
                         gint           n_indent,
                         GHashTable    *references,
                         gint           new_space_id)
{
  g_return_if_fail (references != NULL || new_space_id >= 0);

  if (space == babl_space ("sRGB") || space == NULL)
    {
      /* XXX Maybe I should actually also verify the format. A NULL
       * space on a CMYK or other models won't necessarily be sRGB.
       * Or maybe set name='default'?
       */
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c<space name='sRGB'/>\n", n_indent, ' ');
    }
  else if (references == NULL)
    {
      /* A NULL references is a special case where we are creating the
       * hash table of all used space in initialization step. This is
       * the only case where new_space_id is used.
       */
      int         icc_length = 0;
      const char *icc = babl_space_get_icc (space, &icc_length);
      gchar      *icc_b64;

      icc_b64 = g_base64_encode ((const guchar*) icc, icc_length);
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c<space id='space-%d'>\n", n_indent, ' ', new_space_id);
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c<icc>%s</icc>\n", n_indent + 2, ' ', icc_b64);
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c</space>\n", n_indent, ' ');

      g_free (icc_b64);
    }
  else
    {
      gpointer key;
      gpointer space_id;

      if (g_hash_table_lookup_extended (references, babl_get_name (space), &key, &space_id))
        g_output_stream_printf (output, NULL, NULL, NULL, "%*c<space idref='space-%d'>\n", n_indent, ' ',
                                GPOINTER_TO_UINT (space_id));
      else
        g_critical ("%s: the space was not previously stored: %s", G_STRFUNC, babl_get_name (space));
    }
}

void
gimp_savable_color_save  (GeglColor     *color,
                          const gchar   *name,
                          const Babl    *restricted_format,
                          GOutputStream *output,
                          gint           n_indent,
                          GHashTable    *icc_references)
{
  const Babl *format;
  gchar      *pixel_b64;
  guint8      pixel[40];
  gint        bpp;

  g_return_if_fail (icc_references != NULL);

  if (name != NULL)
    g_output_stream_printf (output, NULL, NULL, NULL, "%*c<color name='%s'>\n", n_indent, ' ', name);
  else
    g_output_stream_printf (output, NULL, NULL, NULL, "%*c<color>\n", n_indent, ' ');

  /* XXX Do we want to make it possible to have a restricted format yet
   * store the colors in their source format (which may be higher
   * bit-depth)? If we were able to drop the restriction, we would not
   * lose precision data.
   */
  if (restricted_format)
    {
      format = restricted_format;
    }
  else
    {
      format = gegl_color_get_format (color);
      gimp_savable_format_save (format, output, n_indent + 2, icc_references);
    }

  gegl_color_get_pixel (color, format, pixel);
  bpp = babl_format_get_bytes_per_pixel (format);
  pixel_b64 = g_base64_encode ((const guchar*) pixel, bpp);
  g_output_stream_printf (output, NULL, NULL, NULL, "%*c<pixel>%s</pixel>\n", n_indent + 2, ' ', pixel_b64);

  g_output_stream_printf (output, NULL, NULL, NULL, "%*c</color>\n", n_indent, ' ');

  g_free (pixel_b64);
}

GHashTable *
gimp_savable_save_all_spaces (GimpImage     *image,
                              GOutputStream *output,
                              gint           n_indent)
{
  GHashTable *icc_refs;
  GList      *iter;
  const Babl *space;
  gint        icc_id = 0;

  icc_refs = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);

  /* Save the space of the image itself. */
  g_output_stream_printf (output, NULL, NULL, NULL, "%*c<formats>\n", n_indent, ' ');
  space = gimp_image_get_layer_space (image);
  if (space != NULL && space != babl_space ("sRGB"))
    {
      gimp_savable_space_save (space, output, 4, NULL, icc_id);
      g_hash_table_insert (icc_refs, (gpointer) babl_get_name (space), GINT_TO_POINTER (icc_id++));
    }

  /* Save the space of the layers (preparing a future where layers may
   * have their own model and space.
   */
  iter = gimp_image_get_layer_list (image);
  for (; iter; iter = iter->next)
    {
      GimpDrawable *drawable = iter->data;

      space = gimp_drawable_get_space (drawable);
      if (space != babl_space ("sRGB") &&
          ! g_hash_table_lookup_extended (icc_refs, babl_get_name (space), NULL, NULL))
        {
          gimp_savable_space_save (space, output, 4, NULL, icc_id);
          g_hash_table_insert (icc_refs, (gpointer) babl_get_name (space), GUINT_TO_POINTER (icc_id++));
        }
    }
  g_list_free (iter);

  /* Save the space of the palette colors (preparing a future where they
   * may have their own model and space.
   */
  if (gimp_image_get_colormap_palette (image))
    {
      GimpPalette *palette = gimp_image_get_colormap_palette (image);
      const Babl  *format;

      format = gimp_palette_get_restriction (palette);

      if (format == NULL)
        {
          iter = gimp_palette_get_colors (palette);
          for (; iter; iter = iter->next)
            {
              GimpPaletteEntry *entry = iter->data;

              format = gegl_color_get_format (entry->color);
              space = babl_format_get_space (format);
              if (space != babl_space ("sRGB") &&
                  ! g_hash_table_lookup_extended (icc_refs, babl_get_name (space), NULL, NULL))
                {
                  gimp_savable_space_save (space, output, 4, NULL, icc_id);
                  g_hash_table_insert (icc_refs, (gpointer) babl_get_name (space), GUINT_TO_POINTER (icc_id++));
                }
            }
        }
      else
        {
          space = babl_format_get_space (format);
          if (space != babl_space ("sRGB") &&
              ! g_hash_table_lookup_extended (icc_refs, babl_get_name (space), NULL, NULL))
            {
              gimp_savable_space_save (space, output, 4, NULL, icc_id);
              g_hash_table_insert (icc_refs, (gpointer) babl_get_name (space), GUINT_TO_POINTER (icc_id++));
            }
        }
    }
  g_output_stream_printf (output, NULL, NULL, NULL, "%*c</formats>\n", n_indent, ' ');

  return icc_refs;
}

void
gimp_savable_unit_save (GimpUnit      *unit,
                        GOutputStream *output,
                        gint           n_indent)
{
  if (gimp_unit_is_built_in (unit))
    {
      GEnumClass *enum_class;
      GEnumValue *enum_value;
      guint32     uid = gimp_unit_get_id (unit);

      g_return_if_fail (uid > GIMP_UNIT_PIXEL && uid < GIMP_UNIT_END);

      enum_class = g_type_class_ref (GIMP_TYPE_UNIT_ID);
      enum_value = g_enum_get_value (enum_class, uid);
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c<unit built-in='%s'/>\n",
                              n_indent, ' ', enum_value->value_nick);
      g_type_class_unref (enum_class);
    }
  else
    {
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c<unit>\n", n_indent, ' ');
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c<factor>%f</factor>\n",
                              n_indent + 2, ' ', gimp_unit_get_factor (unit));
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c<digits>%d</digits>\n",
                              n_indent + 2, ' ', gimp_unit_get_digits (unit));
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c<name>%s</name>\n",
                              n_indent + 2, ' ', gimp_unit_get_name (unit));
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c<symbol>%s</symbol>\n",
                              n_indent + 2, ' ', gimp_unit_get_symbol (unit));
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c<abbrev>%s</abbrev>\n",
                              n_indent + 2, ' ', gimp_unit_get_abbreviation (unit));
      g_output_stream_printf (output, NULL, NULL, NULL, "%*c</unit>\n", n_indent, ' ');
    }
}

void
gimp_savable_metadata_save (GimpMetadata  *metadata,
                            GOutputStream *output,
                            gint           n_indent)
{
  gchar *meta_string;
  gchar *escaped;

  meta_string = gimp_metadata_serialize (metadata);
  escaped     = g_markup_escape_text (meta_string, -1);
  if (meta_string)
    g_output_stream_printf (output, NULL, NULL, NULL, "%*c<metadata>%s</metadata\n",
                            n_indent, ' ', escaped);

  g_free (meta_string);
  g_free (escaped);
}
