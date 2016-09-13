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
}

void
gimp_fonts_set_config (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_connect_swapped (gimp->config, "notify::font-path",
                            G_CALLBACK (gimp_fonts_load), gimp);
}

void
gimp_fonts_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->fonts)
    {
      g_signal_handlers_disconnect_by_func (gimp->config,
                                            G_CALLBACK (gimp_fonts_load),
                                            gimp);

      g_object_unref (gimp->fonts);
      gimp->fonts = NULL;
    }
}

typedef struct
{
  FcConfig  *config;
  GMutex     mutex;
  GCond      cond;
  gboolean   caching_complete : 1;
} GimpFontsLoadFuncData;

static void
gimp_fonts_load_func (FcConfig *config)
{
  if (! FcConfigBuildFonts (config))
    FcConfigDestroy (config);
  else
    FcConfigSetCurrent (config);
}

static void
gimp_fonts_load_thread (GimpFontsLoadFuncData *data)
{
  gimp_fonts_load_func (data->config);

  g_mutex_lock (&data->mutex);
  data->caching_complete = TRUE;
  g_cond_signal (&data->cond);
  g_mutex_unlock (&data->mutex);

  g_thread_exit (0);
}

void
gimp_fonts_load (Gimp               *gimp,
                 GimpInitStatusFunc  status_callback)
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

  if (status_callback)
    {
      gint64                 end_time;
      GThread               *cache_thread;
      GimpFontsLoadFuncData  data;

      /* We perform font cache initialization in a separate thread, so
       * in the case a cache rebuild is to be done it will not block
       * the UI.
       */
      data.config = config;
      g_mutex_init (&data.mutex);
      g_cond_init (&data.cond);
      data.caching_complete = FALSE;

      cache_thread = g_thread_new ("font-cacher",
                                   (GThreadFunc) gimp_fonts_load_thread,
                                   &data);

      g_mutex_lock (&data.mutex);

      end_time = g_get_monotonic_time () + 0.1 * G_TIME_SPAN_SECOND;
      while (! data.caching_complete)
        if (! g_cond_wait_until (&data.cond, &data.mutex, end_time))
          {
            status_callback (NULL, NULL, 0.6);

            end_time += 0.1 * G_TIME_SPAN_SECOND;
            continue;
          }

      g_mutex_unlock (&data.mutex);
      g_thread_join (cache_thread);

      g_mutex_clear (&data.mutex);
      g_cond_clear (&data.cond);
    }
  else
    {
      gimp_fonts_load_func (config);
    }

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
