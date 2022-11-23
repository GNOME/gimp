/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafontfactory.c
 * Copyright (C) 2003-2018 Michael Natterer <mitch@ligma.org>
 *
 * Partly based on code Copyright (C) Sven Neumann <sven@ligma.org>
 *                                    Manish Singh <yosh@ligma.org>
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
#include <pango/pangocairo.h>
#include <pango/pangofc-fontmap.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "text-types.h"

#include "core/ligma.h"
#include "core/ligma-parallel.h"
#include "core/ligmaasync.h"
#include "core/ligmaasyncset.h"
#include "core/ligmacancelable.h"
#include "core/ligmacontainer.h"

#include "ligmafont.h"
#include "ligmafontfactory.h"

#include "ligma-intl.h"


/* Use fontconfig directly for speed. We can use the pango stuff when/if
 * fontconfig/pango get more efficient.
 */
#define USE_FONTCONFIG_DIRECTLY

#ifdef USE_FONTCONFIG_DIRECTLY
#include <fontconfig/fontconfig.h>
#endif

#define CONF_FNAME "fonts.conf"


struct _LigmaFontFactoryPrivate
{
  gpointer foo; /* can't have an empty struct */
};

#define GET_PRIVATE(obj) (((LigmaFontFactory *) (obj))->priv)


static void       ligma_font_factory_data_init       (LigmaDataFactory *factory,
                                                     LigmaContext     *context);
static void       ligma_font_factory_data_refresh    (LigmaDataFactory *factory,
                                                     LigmaContext     *context);
static void       ligma_font_factory_data_save       (LigmaDataFactory *factory);
static void       ligma_font_factory_data_cancel     (LigmaDataFactory *factory);
static LigmaData * ligma_font_factory_data_duplicate  (LigmaDataFactory *factory,
                                                     LigmaData        *data);
static gboolean   ligma_font_factory_data_delete     (LigmaDataFactory *factory,
                                                     LigmaData        *data,
                                                     gboolean         delete_from_disk,
                                                     GError         **error);

static void       ligma_font_factory_load            (LigmaFontFactory *factory,
                                                     GError         **error);
static gboolean   ligma_font_factory_load_fonts_conf (FcConfig        *config,
                                                     GFile           *fonts_conf);
static void       ligma_font_factory_add_directories (FcConfig        *config,
                                                     GList           *path,
                                                     GError         **error);
static void       ligma_font_factory_recursive_add_fontdir
                                                    (FcConfig        *config,
                                                     GFile           *file,
                                                     GError         **error);
static void       ligma_font_factory_load_names      (LigmaContainer   *container,
                                                     PangoFontMap    *fontmap,
                                                     PangoContext    *context);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaFontFactory, ligma_font_factory,
                            LIGMA_TYPE_DATA_FACTORY)

#define parent_class ligma_font_factory_parent_class


static void
ligma_font_factory_class_init (LigmaFontFactoryClass *klass)
{
  LigmaDataFactoryClass *factory_class = LIGMA_DATA_FACTORY_CLASS (klass);

  factory_class->data_init      = ligma_font_factory_data_init;
  factory_class->data_refresh   = ligma_font_factory_data_refresh;
  factory_class->data_save      = ligma_font_factory_data_save;
  factory_class->data_cancel    = ligma_font_factory_data_cancel;
  factory_class->data_duplicate = ligma_font_factory_data_duplicate;
  factory_class->data_delete    = ligma_font_factory_data_delete;
}

static void
ligma_font_factory_init (LigmaFontFactory *factory)
{
  factory->priv = ligma_font_factory_get_instance_private (factory);
}

static void
ligma_font_factory_data_init (LigmaDataFactory *factory,
                             LigmaContext     *context)
{
  GError *error = NULL;

  ligma_font_factory_load (LIGMA_FONT_FACTORY (factory), &error);

  if (error)
    {
      ligma_message_literal (ligma_data_factory_get_ligma (factory), NULL,
                            LIGMA_MESSAGE_INFO,
                            error->message);
      g_error_free (error);
    }
}

static void
ligma_font_factory_data_refresh (LigmaDataFactory *factory,
                                LigmaContext     *context)
{
  GError *error = NULL;

  ligma_font_factory_load (LIGMA_FONT_FACTORY (factory), &error);

  if (error)
    {
      ligma_message_literal (ligma_data_factory_get_ligma (factory), NULL,
                            LIGMA_MESSAGE_INFO,
                            error->message);
      g_error_free (error);
    }
}

static void
ligma_font_factory_data_save (LigmaDataFactory *factory)
{
  /*  this is not "saving" but this functions is called at the right
   *  time at exit to reset the config
   */

  /* if font loading is in progress in another thread, do nothing.  calling
   * FcInitReinitialize() while loading takes place is unsafe.
   */
  if (! ligma_async_set_is_empty (ligma_data_factory_get_async_set (factory)))
    return;

  /* Reinit the library with defaults. */
  FcInitReinitialize ();
}

static void
ligma_font_factory_data_cancel (LigmaDataFactory *factory)
{
  LigmaAsyncSet *async_set = ligma_data_factory_get_async_set (factory);

  /* we can't really cancel font loading, so we just clear the async set and
   * return without waiting for loading to finish.  we also cancel the async
   * set beforehand, as a way to signal to
   * ligma_font_factory_load_async_callback() that loading was canceled and the
   * factory might be dead, and that it should just do nothing.
   */
  ligma_cancelable_cancel (LIGMA_CANCELABLE (async_set));
  ligma_async_set_clear (async_set);
}

static LigmaData *
ligma_font_factory_data_duplicate (LigmaDataFactory *factory,
                                  LigmaData        *data)
{
  return NULL;
}

static gboolean
ligma_font_factory_data_delete (LigmaDataFactory  *factory,
                               LigmaData         *data,
                               gboolean          delete_from_disk,
                               GError          **error)
{
  return TRUE;
}


/*  public functions  */

LigmaDataFactory *
ligma_font_factory_new (Ligma        *ligma,
                       const gchar *path_property_name)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (path_property_name != NULL, NULL);

  return g_object_new (LIGMA_TYPE_FONT_FACTORY,
                       "ligma",               ligma,
                       "data-type",          LIGMA_TYPE_FONT,
                       "path-property-name", path_property_name,
                       "get-standard-func",  ligma_font_get_standard,
                       NULL);
}


/*  private functions  */

static void
ligma_font_factory_load_async (LigmaAsync *async,
                              FcConfig  *config)
{
  if (FcConfigBuildFonts (config))
    {
      ligma_async_finish (async, config);
    }
  else
    {
      FcConfigDestroy (config);

      ligma_async_abort (async);
    }
}

static void
ligma_font_factory_load_async_callback (LigmaAsync       *async,
                                       LigmaFontFactory *factory)
{
  LigmaContainer *container;

  /* the operation was canceled and the factory might be dead (see
   * ligma_font_factory_data_cancel()).  bail.
   */
  if (ligma_async_is_canceled (async))
    return;

  container = ligma_data_factory_get_container (LIGMA_DATA_FACTORY (factory));

  if (ligma_async_is_finished (async))
    {
      FcConfig     *config = ligma_async_get_result (async);
      PangoFontMap *fontmap;
      PangoContext *context;

      FcConfigSetCurrent (config);

      fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
      if (! fontmap)
        g_error ("You are using a Pango that has been built against a cairo "
                 "that lacks the Freetype font backend");

      pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (fontmap),
                                           72.0 /* FIXME */);
      context = pango_font_map_create_context (fontmap);
      g_object_unref (fontmap);

      ligma_font_factory_load_names (container, PANGO_FONT_MAP (fontmap), context);
      g_object_unref (context);
      FcConfigDestroy (config);
    }

  ligma_container_thaw (container);
}

static void
ligma_font_factory_load (LigmaFontFactory  *factory,
                        GError          **error)
{
  LigmaContainer *container;
  Ligma          *ligma;
  LigmaAsyncSet  *async_set;
  FcConfig      *config;
  GFile         *fonts_conf;
  GList         *path;
  LigmaAsync     *async;

  async_set = ligma_data_factory_get_async_set (LIGMA_DATA_FACTORY (factory));

  if (! ligma_async_set_is_empty (async_set))
    {
      /* font loading is already in progress */
      return;
    }

  container = ligma_data_factory_get_container (LIGMA_DATA_FACTORY (factory));

  ligma = ligma_data_factory_get_ligma (LIGMA_DATA_FACTORY (factory));

  if (ligma->be_verbose)
    g_print ("Loading fonts\n");

  config = FcInitLoadConfig ();

  if (! config)
    return;

  fonts_conf = ligma_directory_file (CONF_FNAME, NULL);
  if (! ligma_font_factory_load_fonts_conf (config, fonts_conf))
    g_printerr ("%s: failed to read '%s'.\n",
                G_STRFUNC, g_file_peek_path (fonts_conf));
  g_object_unref (fonts_conf);

  fonts_conf = ligma_sysconf_directory_file (CONF_FNAME, NULL);
  if (! ligma_font_factory_load_fonts_conf (config, fonts_conf))
    g_printerr ("%s: failed to read '%s'.\n",
                G_STRFUNC, g_file_peek_path (fonts_conf));
  g_object_unref (fonts_conf);

  path = ligma_data_factory_get_data_path (LIGMA_DATA_FACTORY (factory));
  if (! path)
    return;

  ligma_container_freeze (container);
  ligma_container_clear (container);

  ligma_font_factory_add_directories (config, path, error);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /* We perform font cache initialization in a separate thread, so
   * in the case a cache rebuild is to be done it will not block
   * the UI.
   */
  async = ligma_parallel_run_async_independent_full (
    +10,
    (LigmaRunAsyncFunc) ligma_font_factory_load_async,
    config);

  ligma_async_add_callback_for_object (
    async,
    (LigmaAsyncCallback) ligma_font_factory_load_async_callback,
    factory,
    factory);

  ligma_async_set_add (async_set, async);

  g_object_unref (async);
}

static gboolean
ligma_font_factory_load_fonts_conf (FcConfig *config,
                                   GFile    *fonts_conf)
{
  gchar    *path = g_file_get_path (fonts_conf);
  gboolean  ret  = TRUE;

  if (! FcConfigParseAndLoad (config, (const guchar *) path, FcFalse))
    ret = FALSE;

  g_free (path);

  return ret;
}

static void
ligma_font_factory_add_directories (FcConfig  *config,
                                   GList     *path,
                                   GError   **error)
{
  GList *list;

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
      ligma_font_factory_recursive_add_fontdir (config, list->data, error);
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
ligma_font_factory_recursive_add_fontdir (FcConfig  *config,
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
              ligma_font_factory_recursive_add_fontdir (config, child, error);
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

      g_object_unref (enumerator);
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
ligma_font_factory_add_font (LigmaContainer        *container,
                            PangoContext         *context,
                            PangoFontDescription *desc)
{
  gchar *name;

  if (! desc)
    return;

  name = pango_font_description_to_string (desc);

  /* It doesn't look like pango_font_description_to_string() could ever
   * return NULL. But just to be double sure and avoid a segfault, I
   * check before validating the string.
   */
  if (name && strlen (name) > 0 &&
      g_utf8_validate (name, -1, NULL))
    {
      LigmaFont *font;

      font = g_object_new (LIGMA_TYPE_FONT,
                           "name",          name,
                           "pango-context", context,
                           NULL);

      ligma_container_add (container, LIGMA_OBJECT (font));
      g_object_unref (font);
    }

  g_free (name);
}

#ifdef USE_FONTCONFIG_DIRECTLY
/* We're really chummy here with the implementation. Oh well. */

/* This is copied straight from make_alias_description in pango, plus
 * the ligma_font_list_add_font bits.
 */
static void
ligma_font_factory_make_alias (LigmaContainer *container,
                              PangoContext  *context,
                              const gchar   *family,
                              gboolean       bold,
                              gboolean       italic)
{
  PangoFontDescription *desc = pango_font_description_new ();

  pango_font_description_set_family (desc, family);
  pango_font_description_set_style (desc,
                                    italic ?
                                    PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (desc,
                                     bold ?
                                     PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (desc, PANGO_STRETCH_NORMAL);

  ligma_font_factory_add_font (container, context, desc);

  pango_font_description_free (desc);
}

static void
ligma_font_factory_load_aliases (LigmaContainer *container,
                                PangoContext  *context)
{
  const gchar *families[] = { "Sans-serif", "Serif", "Monospace" };
  gint         i;

  for (i = 0; i < 3; i++)
    {
      ligma_font_factory_make_alias (container, context, families[i],
                                    FALSE, FALSE);
      ligma_font_factory_make_alias (container, context, families[i],
                                    TRUE,  FALSE);
      ligma_font_factory_make_alias (container, context, families[i],
                                    FALSE, TRUE);
      ligma_font_factory_make_alias (container, context, families[i],
                                    TRUE,  TRUE);
    }
}

static void
ligma_font_factory_load_names (LigmaContainer *container,
                              PangoFontMap  *fontmap,
                              PangoContext  *context)
{
  FcObjectSet *os;
  FcPattern   *pat;
  FcFontSet   *fontset;
  gint         i;

  os = FcObjectSetBuild (FC_FAMILY, FC_STYLE,
                         FC_SLANT, FC_WEIGHT, FC_WIDTH,
                         NULL);
  g_return_if_fail (os);

  pat = FcPatternCreate ();
  if (! pat)
    {
      FcObjectSetDestroy (os);
      g_critical ("%s: FcPatternCreate() returned NULL.", G_STRFUNC);
      return;
    }

  fontset = FcFontList (NULL, pat, os);

  FcPatternDestroy (pat);
  FcObjectSetDestroy (os);

  g_return_if_fail (fontset);

  for (i = 0; i < fontset->nfont; i++)
    {
      PangoFontDescription *desc;

      desc = pango_fc_font_description_from_pattern (fontset->fonts[i], FALSE);
      ligma_font_factory_add_font (container, context, desc);
      pango_font_description_free (desc);
    }

  /*  only create aliases if there is at least one font available  */
  if (fontset->nfont > 0)
    ligma_font_factory_load_aliases (container, context);

  FcFontSetDestroy (fontset);
}

#else  /* ! USE_FONTCONFIG_DIRECTLY */

static void
ligma_font_factory_load_names (LigmaContainer *container,
                              PangoFontMap  *fontmap,
                              PangoContext  *context)
{
  PangoFontFamily **families;
  PangoFontFace   **faces;
  gint              n_families;
  gint              n_faces;
  gint              i, j;

  pango_font_map_list_families (fontmap, &families, &n_families);

  for (i = 0; i < n_families; i++)
    {
      pango_font_family_list_faces (families[i], &faces, &n_faces);

      for (j = 0; j < n_faces; j++)
        {
          PangoFontDescription *desc;

          desc = pango_font_face_describe (faces[j]);
          ligma_font_factory_add_font (container, context, desc);
          pango_font_description_free (desc);
        }
    }

  g_free (families);
}

#endif /* USE_FONTCONFIG_DIRECTLY */
