/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfontfactory.c
 * Copyright (C) 2003-2018 Michael Natterer <mitch@gimp.org>
 *
 * Partly based on code Copyright (C) Sven Neumann <sven@gimp.org>
 *                                    Manish Singh <yosh@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "text-types.h"

#include "core/gimp.h"
#include "core/gimp-parallel.h"
#include "core/gimpasync.h"
#include "core/gimpasyncset.h"
#include "core/gimpcancelable.h"
#include "core/gimpcontainer.h"

#include "gimpfont.h"
#include "gimpfontfactory.h"

#include "gimp-intl.h"


#include <fontconfig/fontconfig.h>

#define CONF_FNAME "fonts.conf"


struct _GimpFontFactoryPrivate
{
  gpointer foo; /* can't have an empty struct */
};

#define GET_PRIVATE(obj) (((GimpFontFactory *) (obj))->priv)


static void       gimp_font_factory_data_init       (GimpDataFactory *factory,
                                                     GimpContext     *context);
static void       gimp_font_factory_data_refresh    (GimpDataFactory *factory,
                                                     GimpContext     *context);
static void       gimp_font_factory_data_save       (GimpDataFactory *factory);
static void       gimp_font_factory_data_cancel     (GimpDataFactory *factory);
static GimpData * gimp_font_factory_data_duplicate  (GimpDataFactory *factory,
                                                     GimpData        *data);
static gboolean   gimp_font_factory_data_delete     (GimpDataFactory *factory,
                                                     GimpData        *data,
                                                     gboolean         delete_from_disk,
                                                     GError         **error);
static void       gimp_font_factory_finalize        (GObject         *object);

static void       gimp_font_factory_load            (GimpFontFactory *factory,
                                                     GError         **error);
static gboolean   gimp_font_factory_load_fonts_conf (FcConfig        *config,
                                                     GFile           *fonts_conf);
static void       gimp_font_factory_add_directories (GimpFontFactory *factory,
                                                     FcConfig        *config,
                                                     GList           *path,
                                                     GError         **error);
static void       gimp_font_factory_recursive_add_fontdir
                                                    (FcConfig        *config,
                                                     GFile           *file,
                                                     GError         **error);
static void       gimp_font_factory_load_names      (GimpFontFactory *container,
                                                     PangoFontMap    *fontmap,
                                                     PangoContext    *context);


G_DEFINE_TYPE_WITH_PRIVATE (GimpFontFactory, gimp_font_factory,
                            GIMP_TYPE_DATA_FACTORY)

#define parent_class gimp_font_factory_parent_class


static void
gimp_font_factory_class_init (GimpFontFactoryClass *klass)
{
  GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
  GimpDataFactoryClass *factory_class = GIMP_DATA_FACTORY_CLASS (klass);

  object_class->finalize        = gimp_font_factory_finalize;

  factory_class->data_init      = gimp_font_factory_data_init;
  factory_class->data_refresh   = gimp_font_factory_data_refresh;
  factory_class->data_save      = gimp_font_factory_data_save;
  factory_class->data_cancel    = gimp_font_factory_data_cancel;
  factory_class->data_duplicate = gimp_font_factory_data_duplicate;
  factory_class->data_delete    = gimp_font_factory_data_delete;
}

static void
gimp_font_factory_init (GimpFontFactory *factory)
{
  factory->priv = gimp_font_factory_get_instance_private (factory);
}

static void
gimp_font_factory_data_init (GimpDataFactory *factory,
                             GimpContext     *context)
{
  GError *error = NULL;

  gimp_font_factory_load (GIMP_FONT_FACTORY (factory), &error);

  if (error)
    {
      gimp_message_literal (gimp_data_factory_get_gimp (factory), NULL,
                            GIMP_MESSAGE_INFO,
                            error->message);
      g_error_free (error);
    }
}

static void
gimp_font_factory_data_refresh (GimpDataFactory *factory,
                                GimpContext     *context)
{
  GError *error = NULL;

  gimp_font_factory_load (GIMP_FONT_FACTORY (factory), &error);

  if (error)
    {
      gimp_message_literal (gimp_data_factory_get_gimp (factory), NULL,
                            GIMP_MESSAGE_INFO,
                            error->message);
      g_error_free (error);
    }
}

static void
gimp_font_factory_data_save (GimpDataFactory *factory)
{
  /*  this is not "saving" but this functions is called at the right
   *  time at exit to reset the config
   */

  /* if font loading is in progress in another thread, do nothing.  calling
   * FcInitReinitialize() while loading takes place is unsafe.
   */
  if (! gimp_async_set_is_empty (gimp_data_factory_get_async_set (factory)))
    return;

  /* Reinit the library with defaults. */
  FcInitReinitialize ();
}

static void
gimp_font_factory_data_cancel (GimpDataFactory *factory)
{
  GimpAsyncSet *async_set = gimp_data_factory_get_async_set (factory);

  /* we can't really cancel font loading, so we just clear the async set and
   * return without waiting for loading to finish.  we also cancel the async
   * set beforehand, as a way to signal to
   * gimp_font_factory_load_async_callback() that loading was canceled and the
   * factory might be dead, and that it should just do nothing.
   */
  gimp_cancelable_cancel (GIMP_CANCELABLE (async_set));
  gimp_async_set_clear (async_set);
}

static GimpData *
gimp_font_factory_data_duplicate (GimpDataFactory *factory,
                                  GimpData        *data)
{
  return NULL;
}

static gboolean
gimp_font_factory_data_delete (GimpDataFactory  *factory,
                               GimpData         *data,
                               gboolean          delete_from_disk,
                               GError          **error)
{
  return TRUE;
}

static void
gimp_font_factory_finalize (GObject  *object)
{
  GimpFontFactory *font_factory = GIMP_FONT_FACTORY(object);

  g_free (font_factory->fonts_renaming_config);
  g_free (font_factory->sysconf);
  g_free (font_factory->conf);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

GimpDataFactory *
gimp_font_factory_new (Gimp        *gimp,
                       const gchar *path_property_name)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (path_property_name != NULL, NULL);

  return g_object_new (GIMP_TYPE_FONT_FACTORY,
                       "gimp",               gimp,
                       "data-type",          GIMP_TYPE_FONT,
                       "path-property-name", path_property_name,
                       "get-standard-func",  gimp_font_get_standard,
                       "unique-names",       FALSE,
                       NULL);
}

GList *
gimp_font_factory_get_custom_fonts_dirs (GimpFontFactory  *factory)
{
  return gimp_data_factory_get_data_path (GIMP_DATA_FACTORY (factory));
}

void
gimp_font_factory_get_custom_config_path (GimpFontFactory  *factory,
                                          gchar           **conf,
                                          gchar           **sysconf)
{
  *conf    = factory->conf;
  *sysconf = factory->sysconf;
}

gchar *
gimp_font_factory_get_fonts_renaming_config (GimpFontFactory  *factory)
{
  return factory->fonts_renaming_config;
}

/*  private functions  */

static void
gimp_font_factory_load_async (GimpAsync *async,
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
gimp_font_factory_load_async_callback (GimpAsync       *async,
                                       GimpFontFactory *factory)
{
  GimpContainer *container;

  /* the operation was canceled and the factory might be dead (see
   * gimp_font_factory_data_cancel()).  bail.
   */
  if (gimp_async_is_canceled (async))
    return;

  container = gimp_data_factory_get_container (GIMP_DATA_FACTORY (factory));

  if (gimp_async_is_finished (async))
    {
      FcConfig     *config = gimp_async_get_result (async);
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

      gimp_font_factory_load_names (factory, PANGO_FONT_MAP (fontmap), context);
      g_object_unref (context);
      FcConfigDestroy (config);
    }

  gimp_container_thaw (container);
}

static void
gimp_font_factory_load (GimpFontFactory  *factory,
                        GError          **error)
{
  GimpContainer *container;
  Gimp          *gimp;
  GimpAsyncSet  *async_set;
  FcConfig      *config;
  GFile         *fonts_conf;
  GList         *path;
  GimpAsync     *async;

  async_set = gimp_data_factory_get_async_set (GIMP_DATA_FACTORY (factory));

  if (! gimp_async_set_is_empty (async_set))
    {
      /* font loading is already in progress */
      return;
    }

  container = gimp_data_factory_get_container (GIMP_DATA_FACTORY (factory));

  gimp = gimp_data_factory_get_gimp (GIMP_DATA_FACTORY (factory));

  if (gimp->be_verbose)
    g_print ("Loading fonts\n");

  config = FcInitLoadConfig ();

  if (! config)
    return;

  fonts_conf = gimp_directory_file (CONF_FNAME, NULL);
  if (! gimp_font_factory_load_fonts_conf (config, fonts_conf))
    {
      g_printerr ("%s: failed to read '%s'.\n",
                  G_STRFUNC, g_file_peek_path (fonts_conf));
    }
  else
    {
      g_free (factory->conf);
      factory->conf = g_file_get_path (fonts_conf);
    }

  g_object_unref (fonts_conf);

  fonts_conf = gimp_sysconf_directory_file (CONF_FNAME, NULL);
  if (! gimp_font_factory_load_fonts_conf (config, fonts_conf))
    {
      g_printerr ("%s: failed to read '%s'.\n",
                  G_STRFUNC, g_file_peek_path (fonts_conf));
    }
  else
    {
      g_free (factory->sysconf);
      factory->sysconf = g_file_get_path (fonts_conf);
    }

  g_object_unref (fonts_conf);

  path = gimp_data_factory_get_data_path (GIMP_DATA_FACTORY (factory));
  if (! path)
    return;

  gimp_container_freeze (container);
  gimp_container_clear (container);

  gimp_font_factory_add_directories (factory, config, path, error);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /* We perform font cache initialization in a separate thread, so
   * in the case a cache rebuild is to be done it will not block
   * the UI.
   */
  async = gimp_parallel_run_async_independent_full (
    +10,
    (GimpRunAsyncFunc) gimp_font_factory_load_async,
    config);

  gimp_async_add_callback_for_object (
    async,
    (GimpAsyncCallback) gimp_font_factory_load_async_callback,
    factory,
    factory);

  gimp_async_set_add (async_set, async);

  g_object_unref (async);
}

static gboolean
gimp_font_factory_load_fonts_conf (FcConfig *config,
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
gimp_font_factory_add_directories (GimpFontFactory *factory,
                                   FcConfig        *config,
                                   GList           *path,
                                   GError         **error)
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
      gimp_font_factory_recursive_add_fontdir (config, list->data, error);
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
gimp_font_factory_recursive_add_fontdir (FcConfig  *config,
                                         GFile     *file,
                                         GError   **error)
{
  GFileEnumerator *enumerator;
  GError          *file_error = NULL;

  g_return_if_fail (config != NULL);

  enumerator = g_file_enumerate_children (file,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                          G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, &file_error);
  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFileType  file_type;
          GFile     *child;

          if (g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN))
            {
              g_object_unref (info);
              continue;
            }

          file_type = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE);
          child     = g_file_enumerator_get_child (enumerator, info);

          if (file_type == G_FILE_TYPE_DIRECTORY)
            {
              gimp_font_factory_recursive_add_fontdir (config, child, error);
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

          /* The font directories are supposed to exist since we create
           * them in gimp_font_factory_add_directories() when they
           * aren't already there.
           * Yet there are cases where empty folders can be deleted and
           * system paths are read-only. This happens for instance for
           * MSIX (see #11401).
           * So as a special exception, we ignore the case where the
           * folders don't exist, but we still warn for all other types
           * of errors.
           */
          if (! file_error || file_error->code != G_IO_ERROR_NOT_FOUND)
            {
              if (*error)
                {
                  gchar *current_message = g_strdup ((*error)->message);

                  g_clear_error (error);
                  if (file_error != NULL)
                    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                 "%s\n- %s%s (%s)", current_message, path,
                                 G_DIR_SEPARATOR_S, file_error->message);
                  else
                    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                 "%s\n- %s%s", current_message, path,
                                 G_DIR_SEPARATOR_S);
                  g_free (current_message);
                }
              else
                {
                  if (file_error != NULL)
                    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                 "- %s%s (%s)", path, G_DIR_SEPARATOR_S,
                                 file_error->message);
                  else
                    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                 "- %s%s", path, G_DIR_SEPARATOR_S);
                }
            }

          g_free (path);
        }
    }

  g_clear_error (&file_error);
}

static void
gimp_font_factory_add_font (GimpContainer        *container,
                            PangoContext         *context,
                            PangoFontDescription *desc,
                            const gchar          *full_name,
                            const gchar          *path,
                            gpointer              font_info[])
{
  gchar *name = (gchar *) full_name;

  if (! desc && ! full_name)
    return;

  if (! full_name)
    name = pango_font_description_to_string (desc);

  /* It doesn't look like pango_font_description_to_string() could ever
   * return NULL. But just to be double sure and avoid a segfault, I
   * check before validating the string.
   */
  if (name && strlen (name) > 0 &&
      g_utf8_validate (name, -1, NULL))
    {
      GimpFont *font;

      font = g_object_new (GIMP_TYPE_FONT,
                           "name",          name,
                           "pango-context", context,
                           NULL);
      gimp_font_set_lookup_name (font, pango_font_description_to_string (desc));

      if (font_info != NULL)
        gimp_font_set_font_info (font, font_info);

      if (path != NULL)
        {
          GFile *file;

          file = g_file_new_for_path (path);
          gimp_data_set_file (GIMP_DATA (font), file, FALSE, FALSE);

          g_object_unref (file);
        }
      else
        {
          gimp_data_make_internal (GIMP_DATA (font), "gimp-font-standard-alias");
        }

      gimp_container_add (container, GIMP_OBJECT (font));
      g_object_unref (font);
    }

  if (!full_name)
    g_free (name);
}

/* We're really chummy here with the implementation. Oh well. */

/* This is copied straight from make_alias_description in pango, plus
 * the gimp_font_list_add_font bits.
 */
static void
gimp_font_factory_make_alias (GimpContainer *container,
                              PangoContext  *context,
                              const gchar   *family,
                              gboolean       bold,
                              gboolean       italic)
{
  FcPattern            *fcpattern;
  PangoFontDescription *desc        = pango_font_description_new ();
  gchar                *desc_str    = NULL;
  gchar                *style       = NULL;
  gchar                *psname      = NULL;
  gchar                *fullname    = NULL;
  gchar                *file        = NULL;
  gint                  index       = -1;
  gint                  weight      = -1;
  gint                  width       = -1;
  gint                  slant       = -1;
  gint                  fontversion = -1;
  gpointer              font_info[PROPERTIES_COUNT];

  pango_font_description_set_family (desc, family);
  pango_font_description_set_style (desc,
                                    italic ?
                                    PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (desc,
                                     bold ?
                                     PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (desc, PANGO_STRETCH_NORMAL);

  fcpattern = pango_fc_font_get_pattern (PANGO_FC_FONT (pango_context_load_font (context, desc)));

  /* this is for backward compatibility*/
  desc_str = pango_font_description_to_string (desc);

  FcPatternGetString  (fcpattern, FC_FILE,            0, (FcChar8 **) &file);
  FcPatternGetString  (fcpattern, FC_FULLNAME,        0, (FcChar8 **) &fullname);
  FcPatternGetString  (fcpattern, FC_POSTSCRIPT_NAME, 0, (FcChar8 **) &psname);
  FcPatternGetString  (fcpattern, FC_STYLE,           0, (FcChar8 **) &style);
  FcPatternGetInteger (fcpattern, FC_WEIGHT,          0,              &weight);
  FcPatternGetInteger (fcpattern, FC_WIDTH,           0,              &width);
  FcPatternGetInteger (fcpattern, FC_INDEX,           0,              &index);
  FcPatternGetInteger (fcpattern, FC_SLANT,           0,              &slant);
  FcPatternGetInteger (fcpattern, FC_FONTVERSION,     0,              &fontversion);

  font_info[PROP_DESC]        = (gpointer)  desc_str;
  font_info[PROP_FULLNAME]    = (gpointer)  fullname;
  font_info[PROP_FAMILY]      = (gpointer)  family;
  font_info[PROP_STYLE]       = (gpointer)  style;
  font_info[PROP_PSNAME]      = (gpointer)  psname;
  font_info[PROP_WEIGHT]      = (gpointer) &weight;
  font_info[PROP_WIDTH]       = (gpointer) &width;
  font_info[PROP_INDEX]       = (gpointer) &index;
  font_info[PROP_SLANT]       = (gpointer) &slant;
  font_info[PROP_FONTVERSION] = (gpointer) &fontversion;
  font_info[PROP_FILE]        = (gpointer)  file;

  /* This might be the only valid time where a NULL path is valid. Though I do
   * wonder if really these aliases are the right thing to do. Generic aliases
   * are the best way to have differing text renders over time (and that's not
   * something to be wished for). XXX
   */
  gimp_font_factory_add_font (container, context, desc, NULL, NULL, font_info);

  g_free (desc_str);
  pango_font_description_free (desc);
}

static void
gimp_font_factory_load_aliases (GimpContainer *container,
                                PangoContext  *context)
{
  const gchar *families[] = { "Sans-serif", "Serif", "Monospace" };
  gint         i;

  for (i = 0; i < 3; i++)
    {
      gimp_font_factory_make_alias (container, context, families[i],
                                    FALSE, FALSE);
      gimp_font_factory_make_alias (container, context, families[i],
                                    TRUE,  FALSE);
      gimp_font_factory_make_alias (container, context, families[i],
                                    FALSE, TRUE);
      gimp_font_factory_make_alias (container, context, families[i],
                                    TRUE,  TRUE);
    }
}

static void
gimp_font_factory_load_names (GimpFontFactory *factory,
                              PangoFontMap    *fontmap,
                              PangoContext    *context)
{
  GimpContainer *container;
  FcObjectSet   *os;
  FcPattern     *pat;
  FcFontSet     *fontset;
  GString       *ignored_fonts;
  GString       *global_xml = g_string_new ("<fontconfig>\n");
  gint           n_ignored  = 0;
  gint           i;

  container = gimp_data_factory_get_container (GIMP_DATA_FACTORY (factory));

  os = FcObjectSetBuild (FC_FAMILY,
                         FC_STYLE,
                         FC_POSTSCRIPT_NAME,
                         FC_FULLNAME,
                         FC_FILE,
                         FC_WEIGHT,
                         FC_SLANT,
                         FC_WIDTH,
                         FC_INDEX,
                         FC_FONTVERSION,
                         FC_FONTFORMAT,
                         NULL);
  g_return_if_fail (os);

  pat = FcPatternCreate ();
  if (! pat)
    {
      FcObjectSetDestroy (os);
      g_critical ("%s: FcPatternCreate() returned NULL.", G_STRFUNC);
      return;
    }

  fontset       = FcFontList (NULL, pat, os);
  ignored_fonts = g_string_new (NULL);

  FcPatternDestroy (pat);
  FcObjectSetDestroy (os);

  g_return_if_fail (fontset);

  for (i = 0; i < fontset->nfont; i++)
    {
      PangoFontDescription *pfd;
      GString              *xml;
      GString              *xml_bold_variant;
      gchar                *family           = NULL;
      gchar                *style            = NULL;
      gchar                *psname           = NULL;
      gchar                *newname          = NULL;
      gchar                *escaped_fullname = NULL;
      gchar                *fullname         = NULL;
      gchar                *fullname2        = NULL;
      gchar                *escaped_file     = NULL;
      gchar                *file             = NULL;
      hb_blob_t            *blob             = NULL;
      gint                  index            = -1;
      gint                  weight           = -1;
      gint                  width            = -1;
      gint                  slant            = -1;
      gint                  fontversion      = -1;
      gpointer              font_info[PROPERTIES_COUNT];
      PangoFontDescription *pattern_pfd;
      gchar                *pattern_pfd_desc;

      FcPatternGetString (fontset->fonts[i], FC_FILE, 0, (FcChar8 **) &file);

      blob = hb_blob_create_from_file_or_fail (file);
      /*
       * Pango doesn't support non SFNT fonts because harfbuzz doesn't support them.
       * woff and woff2, not supported by pango (because they are not yet supported by harfbuzz).
       * pcf,pcf.gz are bitmap font formats, not supported by pango (because of harfbuzz).
       * afm, pfm, pfb are type1 font formats, not supported by pango (because of harfbuzz).
       */
      if (blob == NULL || hb_face_count (blob) == 0)
        {
          g_string_append_printf (ignored_fonts, "- %s (not supported by pango)\n", file);
          n_ignored++;
          continue;
        }

      hb_blob_destroy (blob);

      /* Some variable fonts have only a family name and a font version. */
      if (FcPatternGetString (fontset->fonts[i], FC_FULLNAME, 0, (FcChar8 **) &fullname) != FcResultMatch)
        {
          g_string_append_printf (ignored_fonts, "- %s (no full name)\n", file);
          n_ignored++;
          continue;
        }

      FcPatternGetString  (fontset->fonts[i], FC_FAMILY,          0, (FcChar8 **) &family);
      FcPatternGetString  (fontset->fonts[i], FC_POSTSCRIPT_NAME, 0, (FcChar8 **) &psname);
      FcPatternGetString  (fontset->fonts[i], FC_STYLE,           0, (FcChar8 **) &style);
      FcPatternGetInteger (fontset->fonts[i], FC_WEIGHT,          0,              &weight);
      FcPatternGetInteger (fontset->fonts[i], FC_WIDTH,           0,              &width);
      FcPatternGetInteger (fontset->fonts[i], FC_INDEX,           0,              &index);
      FcPatternGetInteger (fontset->fonts[i], FC_SLANT,           0,              &slant);
      FcPatternGetInteger (fontset->fonts[i], FC_FONTVERSION,     0,              &fontversion);

      /* this is for backward compatibility*/
      pattern_pfd      = pango_fc_font_description_from_pattern (fontset->fonts[i], FALSE);
      pattern_pfd_desc = pango_font_description_to_string (pattern_pfd);

      font_info[PROP_DESC]        = (gpointer)  pattern_pfd_desc;
      font_info[PROP_FULLNAME]    = (gpointer)  fullname;
      font_info[PROP_FAMILY]      = (gpointer)  family;
      font_info[PROP_STYLE]       = (gpointer)  style;
      font_info[PROP_PSNAME]      = (gpointer)  psname;
      font_info[PROP_WEIGHT]      = (gpointer) &weight;
      font_info[PROP_WIDTH]       = (gpointer) &width;
      font_info[PROP_INDEX]       = (gpointer) &index;
      font_info[PROP_SLANT]       = (gpointer) &slant;
      font_info[PROP_FONTVERSION] = (gpointer) &fontversion;
      font_info[PROP_FILE]        = (gpointer)  file;

      /* Sometimes a font has more than one fullname,
       * sometimes the second is more appropriate for display,
       * in such cases we use it instead of the first.
       */
      if (FcPatternGetString (fontset->fonts[i], FC_FULLNAME, 1, (FcChar8 **) &fullname2) != FcResultMatch)
        fullname2 = NULL;

      newname = g_strdup_printf ("gimpfont%i", i);

      xml = g_string_new ("<match>");

      /*We can't use faux bold (sometimes real bold) unless it is specified in fontconfig*/
      xml_bold_variant = g_string_new ("<?xml version=\"1.0\"?>\n<match>");

      g_string_append_printf (xml,
                              "<test name=\"family\"><string>%s</string></test>",
                              newname);
      g_string_append_printf (xml_bold_variant,
                              "<test name=\"family\"><string>%s</string></test>",
                              newname);
      g_string_append (xml_bold_variant,
                       "<test name=\"weight\" compare=\"eq\"><const>bold</const></test>");

      escaped_fullname = g_markup_escape_text (fullname, -1);
      g_string_append_printf (xml,
                              "<edit name=\"fullname\" mode=\"assign\" binding=\"strong\"><string>%s</string></edit>",
                              escaped_fullname);
      g_string_append_printf (xml_bold_variant,
                              "<edit name=\"fullname\" mode=\"prepend\" binding=\"strong\"><string>%s</string></edit>",
                              escaped_fullname);
      g_free (escaped_fullname);

      family = g_markup_escape_text (family, -1);
      g_string_append_printf (xml,
                              "<edit name=\"family\" mode=\"assign\" binding=\"strong\"><string>%s</string></edit>",
                              family);
      g_string_append_printf (xml_bold_variant,
                              "<edit name=\"family\" mode=\"assign\" binding=\"strong\"><string>%s</string></edit>",
                              family);
      g_free (family);

      escaped_file = g_markup_escape_text (file, -1);
      g_string_append_printf (xml,
                              "<edit name=\"file\" mode=\"assign\" binding=\"strong\"><string>%s</string></edit>",
                              escaped_file);
      g_string_append_printf (xml_bold_variant,
                              "<edit name=\"file\" mode=\"assign\" binding=\"strong\"><string>%s</string></edit>",
                              escaped_file);
      g_free (escaped_file);

      if (psname != NULL)
        {
          psname = g_markup_escape_text (psname, -1);
          g_string_append_printf (xml,
                                  "<edit name=\"postscriptname\" mode=\"assign\" binding=\"strong\"><string>%s</string></edit>",
                                  psname);
          g_string_append_printf (xml_bold_variant,
                                  "<edit name=\"postscriptname\" mode=\"assign\" binding=\"strong\"><string>%s</string></edit>",
                                  psname);
          g_free (psname);
        }

      if (style != NULL)
        {
          style = g_markup_escape_text (style, -1);
          g_string_append_printf (xml,
                                  "<edit name=\"style\" mode=\"assign\" binding=\"strong\"><string>%s</string></edit>",
                                  style);
          g_string_append_printf (xml_bold_variant,
                                  "<edit name=\"style\" mode=\"assign\" binding=\"strong\"><string>%s</string></edit>",
                                  style);
          g_free (style);
        }

      g_string_append (xml_bold_variant, "<edit name=\"weight\" mode=\"assign\" binding=\"strong\"><const>bold</const></edit>");

      if (weight != -1)
        g_string_append_printf (xml,
                                "<edit name=\"weight\" mode=\"prepend\" binding=\"strong\"><int>%i</int></edit>",
                                weight);

      if (width != -1)
        {
          g_string_append_printf (xml,
                                  "<edit name=\"width\" mode=\"assign\" binding=\"strong\"><int>%i</int></edit>",
                                  width);
          g_string_append_printf (xml_bold_variant,
                                  "<edit name=\"width\" mode=\"assign\" binding=\"strong\"><int>%i</int></edit>",
                                  width);
        }

      if (slant != -1)
        {
          g_string_append_printf (xml,
                                  "<edit name=\"slant\" mode=\"prepend\" binding=\"strong\"><int>%i</int></edit>",
                                  slant);
          g_string_append_printf (xml_bold_variant,
                                  "<edit name=\"slant\" mode=\"prepend\" binding=\"strong\"><int>%i</int></edit>",
                                  slant);
        }

      if (fontversion != -1)
        {
          g_string_append_printf (xml,
                                  "<edit name=\"fontversion\" mode=\"assign\" binding=\"strong\"><int>%i</int></edit>",
                                  fontversion);
          g_string_append_printf (xml_bold_variant,
                                  "<edit name=\"fontversion\" mode=\"assign\" binding=\"strong\"><int>%i</int></edit>",
                                  fontversion);
        }

      if (index != -1)
        {
          g_string_append_printf (xml,
                                  "<edit name=\"index\" mode=\"assign\" binding=\"strong\"><int>%i</int></edit>",
                                  index);
          g_string_append_printf (xml_bold_variant,
                                  "<edit name=\"index\" mode=\"assign\" binding=\"strong\"><int>%i</int></edit>",
                                  index);
        }


      g_string_append (xml, "</match>\n");
      g_string_append (xml_bold_variant, "</match>\n");

      FcConfigParseAndLoadFromMemory (FcConfigGetCurrent (), (const FcChar8 *) xml_bold_variant->str, FcTrue);
      FcConfigParseAndLoadFromMemory (FcConfigGetCurrent (), (const FcChar8 *) xml->str, FcTrue);

      pfd = pango_font_description_from_string (newname);

      if (fullname2 != NULL && g_str_is_ascii (fullname2))
        fullname = fullname2;

      gimp_font_factory_add_font (container, context, pfd, fullname, (const gchar *) file, font_info);

      g_string_append (global_xml, xml->str);

      pango_font_description_free (pattern_pfd);
      g_free (pattern_pfd_desc);
      pango_font_description_free (pfd);
      g_free (newname);
      g_string_free (xml, TRUE);
      g_string_free (xml_bold_variant, TRUE);
    }

  g_string_append (global_xml, "</fontconfig>");

  g_free (factory->fonts_renaming_config);
  factory->fonts_renaming_config = g_strdup (global_xml->str);

  if (n_ignored > 0)
    {
      if (g_getenv ("GIMP_DEBUG_FONTS") == NULL)
        g_printerr ("%s: %d unsupported fonts were ignored. Set the GIMP_DEBUG_FONTS environment variable for a listing.\n", G_STRFUNC, n_ignored);
      else
        g_printerr ("%s: %d unsupported fonts were ignored: %s", G_STRFUNC, n_ignored, ignored_fonts->str);
    }

  g_string_free (ignored_fonts, TRUE);
  g_string_free (global_xml, TRUE);

  /*  only create aliases if there is at least one font available  */
  if (fontset->nfont > 0)
    gimp_font_factory_load_aliases (container, context);

  FcFontSetDestroy (fontset);
}
