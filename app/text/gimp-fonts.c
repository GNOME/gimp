/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
 *
 * text.c
 * Copyright (C) 2003 Manish Singh <yosh@gimp.org>
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

#include <gio/gio.h>

#include <fontconfig/fontconfig.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "text-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"

#include "gimp-fonts.h"
#include "gimpfontlist.h"


#define CONF_FNAME "fonts.conf"


static gboolean gimp_fonts_load_fonts_conf (FcConfig *config,
                                            GFile    *fonts_conf);
static void     gimp_fonts_add_directories (FcConfig *config,
                                            GList    *path);


void
gimp_fonts_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->fonts = gimp_font_list_new (72.0, 72.0);
  gimp_object_set_name (GIMP_OBJECT (gimp->fonts), "fonts");

  g_signal_connect_swapped (gimp->config, "notify::font-path",
                            G_CALLBACK (gimp_fonts_load), gimp);
}

void
gimp_fonts_load (Gimp *gimp)
{
  FcConfig *config;
  GFile    *fonts_conf;
  GList    *path;

  g_return_if_fail (GIMP_IS_FONT_LIST (gimp->fonts));

  gimp_set_busy (gimp);

  if (gimp->be_verbose)
    g_print ("Loading fonts\n");

  gimp_container_freeze (GIMP_CONTAINER (gimp->fonts));

  gimp_container_clear (GIMP_CONTAINER (gimp->fonts));

  config = FcInitLoadConfig ();

  if (! config)
    goto cleanup;

  fonts_conf = gimp_directory_file (CONF_FNAME, NULL);
  if (! gimp_fonts_load_fonts_conf (config, fonts_conf))
    goto cleanup;

  fonts_conf = gimp_sysconf_directory_file (CONF_FNAME, NULL);
  if (! gimp_fonts_load_fonts_conf (config, fonts_conf))
    goto cleanup;

  path = gimp_config_path_expand_to_files (gimp->config->font_path, FALSE);
  gimp_fonts_add_directories (config, path);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  if (! FcConfigBuildFonts (config))
    {
      FcConfigDestroy (config);
      goto cleanup;
    }

  FcConfigSetCurrent (config);

  gimp_font_list_restore (GIMP_FONT_LIST (gimp->fonts));

 cleanup:
  gimp_container_thaw (GIMP_CONTAINER (gimp->fonts));
  gimp_unset_busy (gimp);
}

void
gimp_fonts_reset (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->no_fonts)
    return;

  /* Reinit the library with defaults. */
  FcInitReinitialize ();
}

static gboolean
gimp_fonts_load_fonts_conf (FcConfig *config,
                            GFile    *fonts_conf)
{
  gchar    *path = g_file_get_path (fonts_conf);
  gboolean  ret  = TRUE;

  if (! FcConfigParseAndLoad (config, (const guchar *) path, FcFalse))
    {
      FcConfigDestroy (config);
      ret = FALSE;
    }

  g_free (path);
  g_object_unref (fonts_conf);

  return ret;
}

static void
gimp_fonts_add_directories (FcConfig *config,
                            GList    *path)
{
  GList *list;

  g_return_if_fail (config != NULL);

  for (list = path; list; list = list->next)
    {
      gchar *dir = g_file_get_path (list->data);
#ifdef G_OS_WIN32
      gchar *tmp = g_win32_locale_filename_from_utf8 (dir);

      g_free (dir);
      dir = tmp;
#endif

      FcConfigAppFontAddDir (config, (const FcChar8 *) dir);

      g_free (dir);
    }
}
