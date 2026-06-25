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

#include "core-types.h"

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
