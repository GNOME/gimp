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
 * Returns a [struct@Pango.FontDescription] representing @font.
 *
 * Returns: (transfer full): a %PangoFontDescription representing @font.
 *
 * Since: 3.0
 **/
PangoFontDescription *
gimp_font_get_pango_font_description (GimpFont *font)
{
  static gboolean       fc_configs_loaded = FALSE;
  PangoFontDescription *desc              = NULL;

  if (!fc_configs_loaded)
    {
      gchar  *config;
      gchar  *sysconfig;
      gchar **fonts_renaming_config;
      gchar **dirs;

      FcConfigSetCurrent (FcInitLoadConfig ());

      config = _gimp_fonts_get_custom_configs (&sysconfig, &fonts_renaming_config, &dirs);

      FcConfigParseAndLoad (FcConfigGetCurrent (),
                            (const guchar *)config ,
                            FcFalse);

      FcConfigParseAndLoad (FcConfigGetCurrent (),
                            (const guchar *)sysconfig ,
                            FcFalse);

      for (int i = 0; dirs[i] != NULL; ++i)
        if (dirs[i])
          FcConfigAppFontAddDir (FcConfigGetCurrent (), (const FcChar8 *)dirs[i]);

      for (int i = 0; fonts_renaming_config[i] != NULL; ++i)
        if (fonts_renaming_config[i])
          FcConfigParseAndLoadFromMemory (FcConfigGetCurrent (), (const FcChar8 *)fonts_renaming_config[i], FcTrue);

      g_strfreev (fonts_renaming_config);
      g_free (sysconfig);
      g_free (config);
      g_strfreev (dirs);

      fc_configs_loaded = TRUE;
    }

  desc = pango_font_description_from_string (_gimp_font_get_lookup_name (font));

  return desc;
}
