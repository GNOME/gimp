/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gui-types.h"

#include "config/gimpconfig-path.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "themes.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gui_themes_dir_foreach_func (const GimpDatafileData *file_data,
                                           gpointer                user_data);


/*  private variables  */

static GHashTable *themes_hash = NULL;


/*  public functions  */

void
themes_init (Gimp *gimp)
{
  GimpGuiConfig *config;
  const gchar   *theme_dir;
  gchar         *gtkrc;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_GUI_CONFIG (gimp->config);

  themes_hash = g_hash_table_new_full (g_str_hash,
				       g_str_equal,
				       g_free,
				       g_free);

  if (config->theme_path)
    {
      gchar *path;

      path = gimp_config_path_expand (config->theme_path, TRUE, NULL);

      gimp_datafiles_read_directories (path,
				       G_FILE_TEST_IS_DIR,
				       gui_themes_dir_foreach_func,
				       gimp);

      g_free (path);
    }

  theme_dir = themes_get_theme_dir (gimp);

  if (theme_dir)
    {
      gtkrc = g_build_filename (theme_dir, "gtkrc", NULL);
    }
  else
    {
      /*  get the hardcoded default theme gtkrc  */
      gtkrc = g_strdup (gimp_gtkrc ());
    }

  if (gimp->be_verbose)
    g_print (_("Parsing '%s'\n"), gtkrc);

  gtk_rc_parse (gtkrc);
  g_free (gtkrc);

  /*  parse the user gtkrc  */

  gtkrc = gimp_personal_rc_file ("gtkrc");

  if (gimp->be_verbose)
    g_print (_("Parsing '%s'\n"), gtkrc);

  gtk_rc_parse (gtkrc);
  g_free (gtkrc);
}

void
themes_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (themes_hash)
    {
      g_hash_table_destroy (themes_hash);
      themes_hash = NULL;
    }
}

const gchar *
themes_get_theme_dir (Gimp *gimp)
{
  GimpGuiConfig *config;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  config = GIMP_GUI_CONFIG (gimp->config);

  if (config->theme)
    return g_hash_table_lookup (themes_hash, config->theme);

  return g_hash_table_lookup (themes_hash, "Default");
}


/*  private functions  */

static void
gui_themes_dir_foreach_func (const GimpDatafileData *file_data,
                             gpointer                user_data)
{
  Gimp *gimp;

  gimp = GIMP (user_data);

  if (gimp->be_verbose)
    g_print (_("Adding theme '%s' (%s)\n"),
             file_data->basename, file_data->filename);

  g_hash_table_insert (themes_hash,
		       g_strdup (file_data->basename),
		       g_strdup (file_data->filename));
}
