/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfont.c
 * Copyright (C) 2023 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <pango/pangofc-fontmap.h>

#include "gimp.h"

#include "gimpfont.h"

struct _GimpFont
{
  GimpResource parent_instance;
};

G_DEFINE_TYPE (GimpFont, gimp_font, GIMP_TYPE_RESOURCE);


static void  gimp_font_class_init (GimpFontClass *klass)
{
}

static void  gimp_font_init (GimpFont *font)
{
}

/**
 * gimp_font_get_pango_font_description:
 * @font: (transfer none): the [class@Gimp.Font]
 *
 * Returns a [class@Pango.Font.Description] representing @font.
 *
 * Returns: (transfer full): a %PangoFontDescription representing @font.
 *
 * Since: 3.0
 **/
PangoFontDescription *
gimp_font_get_pango_font_description (GimpFont *font)
{
  PangoFontDescription *desc          = NULL;
  gchar                *name          = NULL;
  gchar                *collection_id = NULL;
  gboolean              is_internal;

  is_internal = _gimp_resource_get_identifiers (GIMP_RESOURCE (font),
                                                &name, &collection_id);
  /* TODO: we can't create fonts from internal fonts right now, but it should
   * actually be possible because these are in fact alias to non-internal fonts.
   * See #9985.
   */
  if (! is_internal)
    {
      gchar *expanded_path;

      expanded_path = gimp_config_path_expand (collection_id, FALSE, NULL);
      if (expanded_path != NULL &&
          FcConfigAppFontAddFile (FcConfigGetCurrent (), (const FcChar8 *) expanded_path))
        desc = pango_font_description_from_string (name);

      g_free (expanded_path);
    }

  g_free (name);
  g_free (collection_id);

  return desc;
}
