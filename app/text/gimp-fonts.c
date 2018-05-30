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
#include "core/gimp-gui.h"
#include "core/gimp-parallel.h"
#include "core/gimpasync.h"
#include "core/gimpasyncset.h"
#include "core/gimpcancelable.h"
#include "core/gimpuncancelablewaitable.h"
#include "core/gimpwaitable.h"

#include "gimp-fonts.h"
#include "gimpfontlist.h"

#include "gimp-intl.h"

#define CONF_FNAME "fonts.conf"


static gboolean   gimp_fonts_load_fonts_conf       (FcConfig      *config,
                                                    GFile         *fonts_conf);
static void       gimp_fonts_add_directories       (Gimp          *gimp,
                                                    FcConfig      *config,
                                                    GList         *path,
                                                    GError       **error);
static void       gimp_fonts_recursive_add_fontdir (FcConfig      *config,
                                                    GFile         *path,
                                                    GError       **error);
static void       gimp_fonts_notify_font_path      (GObject       *gobject,
                                                    GParamSpec    *pspec,
                                                    Gimp          *gimp);


void
gimp_fonts_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->fonts = gimp_font_list_new (72.0, 72.0);
  gimp_object_set_name (GIMP_OBJECT (gimp->fonts), "fonts");

  gimp->fonts_async_set = gimp_async_set_new ();
}

void
gimp_fonts_set_config (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_connect (gimp->config, "notify::font-path",
                    G_CALLBACK (gimp_fonts_notify_font_path),
                    gimp);
}

void
gimp_fonts_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->fonts_async_set)
    {
      gimp_cancelable_cancel (GIMP_CANCELABLE (gimp->fonts_async_set));

      g_clear_object (&gimp->fonts_async_set);
    }

  if (gimp->fonts)
    {
      if (gimp->config)
        g_signal_handlers_disconnect_by_func (gimp->config,
                                              G_CALLBACK (gimp_fonts_notify_font_path),
                                              gimp);

      g_clear_object (&gimp->fonts);
    }
}

static void
gimp_fonts_load_async (GimpAsync *async,
                       FcConfig  *config)
{
  if (FcConfigBuildFonts (config))
    {
      gimp_async_finish (async, config);
    }
  else
    {
      FcConfigDestroy (config);

      gimp_async_abort (async);
    }
}

static void
gimp_fonts_load_async_callback (GimpAsync *async,
                                Gimp      *gimp)
{
  if (gimp_async_is_canceled (async))
    return;

  if (gimp_async_is_finished (async))
    {
      FcConfig *config = gimp_async_get_result (async);

      FcConfigSetCurrent (config);

      gimp_font_list_restore (GIMP_FONT_LIST (gimp->fonts));
    }
}

void
gimp_fonts_load (Gimp    *gimp,
                 GError **error)
{
  FcConfig  *config;
  GFile     *fonts_conf;
  GList     *path;
  GimpAsync *async;

  g_return_if_fail (GIMP_IS_FONT_LIST (gimp->fonts));
  g_return_if_fail (GIMP_IS_ASYNC_SET (gimp->fonts_async_set));

  if (! gimp_async_set_is_empty (gimp->fonts_async_set))
    {
      /* font loading is already in progress */
      return;
    }

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
  gimp_fonts_add_directories (gimp, config, path, error);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /* We perform font cache initialization in a separate thread, so
   * in the case a cache rebuild is to be done it will not block
   * the UI.
   */
  async = gimp_parallel_run_async (
    TRUE,
    (GimpParallelRunAsyncFunc) gimp_fonts_load_async,
    config);

  gimp_async_add_callback (async,
                           (GimpAsyncCallback) gimp_fonts_load_async_callback,
                           gimp);

  gimp_async_set_add (gimp->fonts_async_set, async);

  g_object_unref (async);

 cleanup:
  gimp_container_thaw (GIMP_CONTAINER (gimp->fonts));
  gimp_unset_busy (gimp);
}

void
gimp_fonts_reset (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_ASYNC_SET (gimp->fonts_async_set));

  if (gimp->no_fonts)
    return;

  /* if font loading is in progress in another thread, do nothing.  calling
   * FcInitReinitialize() while loading takes place is unsafe.
   */
  if (! gimp_async_set_is_empty (gimp->fonts_async_set))
    return;

  /* Reinit the library with defaults. */
  FcInitReinitialize ();
}

gboolean
gimp_fonts_wait (Gimp    *gimp,
                 GError **error)
{
  GimpWaitable *waitable;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (GIMP_IS_FONT_LIST (gimp->fonts), FALSE);
  g_return_val_if_fail (GIMP_IS_ASYNC_SET (gimp->fonts_async_set), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* don't allow cancellation for now */
  waitable = gimp_uncancelable_waitable_new (
    GIMP_WAITABLE (gimp->fonts_async_set));

  gimp_wait (gimp, waitable, _("Loading fonts (this may take a while...)"));

  g_object_unref (waitable);

  return TRUE;
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
gimp_fonts_add_directories (Gimp      *gimp,
                            FcConfig  *config,
                            GList     *path,
                            GError   **error)
{
  GList *list;

  g_return_if_fail (config != NULL);

  for (list = path; list; list = list->next)
    {
      /* The configured directories must exist or be created. */
      g_file_make_directory_with_parents (list->data, NULL, NULL);

      /* Do not use FcConfigAppFontAddDir(). Instead use
       * FcConfigAppFontAddFile() with our own recursive loop.
       * Otherwise, when some fonts fail to load (e.g. permission
       * issues), we end up in weird situations where the fonts are in
       * the list, but are unusable and output many errors.
       * See bug 748553.
       */
      gimp_fonts_recursive_add_fontdir (config, list->data, error);
    }
  if (error && *error)
    {
      gchar *font_list = g_strdup ((*error)->message);

      g_clear_error (error);
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Some fonts failed to load:\n%s"), font_list);
      g_free (font_list);
    }
}

static void
gimp_fonts_recursive_add_fontdir (FcConfig  *config,
                                  GFile     *file,
                                  GError   **error)
{
  GFileEnumerator *enumerator;

  g_return_if_fail (config != NULL);

  enumerator = g_file_enumerate_children (file,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                          G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);
  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFileType  file_type;
          GFile     *child;

          if (g_file_info_get_is_hidden (info))
            {
              g_object_unref (info);
              continue;
            }

          file_type = g_file_info_get_file_type (info);
          child     = g_file_enumerator_get_child (enumerator, info);

          if (file_type == G_FILE_TYPE_DIRECTORY)
            {
              gimp_fonts_recursive_add_fontdir (config, child, error);
            }
          else if (file_type == G_FILE_TYPE_REGULAR)
            {
              gchar *path = g_file_get_path (child);
#ifdef G_OS_WIN32
              gchar *tmp = g_win32_locale_filename_from_utf8 (path);

              g_free (path);
              /* XXX: g_win32_locale_filename_from_utf8() may return
               * NULL. So we need to check that path is not NULL before
               * trying to load with fontconfig.
               */
              path = tmp;
#endif

              if (! path ||
                  FcFalse == FcConfigAppFontAddFile (config, (const FcChar8 *) path))
                {
                  g_printerr ("%s: adding font file '%s' failed.\n",
                              G_STRFUNC, path);
                  if (error)
                    {
                      if (*error)
                        {
                          gchar *current_message = g_strdup ((*error)->message);

                          g_clear_error (error);
                          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                       "%s\n- %s", current_message, path);
                          g_free (current_message);
                        }
                      else
                        {
                          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                       "- %s", path);
                        }
                    }
                }
              g_free (path);
            }

          g_object_unref (child);
          g_object_unref (info);
        }
    }
  else
    {
      if (error)
        {
          gchar *path = g_file_get_path (file);

          if (*error)
            {
              gchar *current_message = g_strdup ((*error)->message);

              g_clear_error (error);
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           "%s\n- %s%s", current_message, path,
                           G_DIR_SEPARATOR_S);
              g_free (current_message);
            }
          else
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           "- %s%s", path, G_DIR_SEPARATOR_S);
            }
          g_free (path);
        }
    }
}

static void
gimp_fonts_notify_font_path (GObject    *gobject,
                             GParamSpec *pspec,
                             Gimp       *gimp)
{
  GError *error = NULL;

  gimp_fonts_load (gimp, &error);

  if (error)
    {
      gimp_message_literal (gimp, NULL,
                            GIMP_MESSAGE_INFO,
                            error->message);
      g_error_free (error);
    }
}
