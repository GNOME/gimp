/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * String substitution utilities for config files
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#include <stdio.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpenv.h"

#include "gimpconfig.h"
#include "gimpconfig-substitute.h"


#define SUBSTS_ALLOC 4

static inline gchar * extract_token (const gchar **str);


gchar * 
gimp_config_substitute_path (GObject     *object,
                             const gchar *path,
                             gboolean     use_env)
{
  const gchar *p;
  const gchar *s;
  gchar       *n;
  gchar       *token;
  gchar       *new_path = NULL;
  gchar      **substs   = NULL;
  guint        n_substs = 0;
  gint         length   = 0;
  gint         i;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  p = path;

  while (*p)
    {
#ifndef G_OS_WIN32
      if (*p == '~')
	{
	  length += strlen (g_get_home_dir ());
	  p += 1;
	}
      else
#endif
      if ((token = extract_token (&p)) != NULL)
        {
          for (i = 0; i < n_substs; i++)
            if (strcmp (substs[2*i], token) == 0)
              break;
          
          if (i < n_substs)
            {
              s = substs[2*i+1];
            }
          else
            {
              s = gimp_config_lookup_unknown_token (object, token);

              if (!s && use_env) 
                {
                  if (!s && strcmp (token, "gimp_dir") == 0)
                    s = gimp_directory ();
                  
                  if (!s && strcmp (token, "gimp_datadir") == 0)
                    s = gimp_data_directory ();
                  
                  if (!s && 
                      ((strcmp (token, "gimp_plug_in_dir")) == 0 || 
                       (strcmp (token, "gimp_plugin_dir")) == 0))
                    s = gimp_plug_in_directory ();
                  
                  if (!s && strcmp (token, "gimp_sysconfdir") == 0)
                    s = gimp_sysconf_directory ();
                  
                  if (!s)
                    s = g_getenv (token);
                  
#ifdef G_OS_WIN32
                  /* The default user gimprc on Windows references
                   * ${TEMP}, but not all Windows installations have that
                   * environment variable, even if it should be kinda
                   * standard. So special-case it.
                   */
                  if (!s && strcmp (token, "TEMP") == 0)
                    s = g_get_tmp_dir ();
#endif
                }
          
              if (!s)
                {
                  g_message ("token referenced but not defined: ${%s}", token);
                  
                  g_free (token);
                  goto cleanup;
                }

              if (n_substs % SUBSTS_ALLOC == 0)
                substs = g_renew (gchar *, substs, 2*(n_substs+SUBSTS_ALLOC));

              substs[2*n_substs]     = token;
              substs[2*n_substs + 1] = (gchar *) s;
              n_substs++;
            }

          length += strlen (s);
        }
      else
	{
          length += g_utf8_skip[(guchar) *p];
          p = g_utf8_next_char (p);
	}
    }

  if (!n_substs)
    return g_strdup (path);

  new_path = g_new (gchar, length + 1);

  p = path;
  n = new_path;

  while (*p)
    {
#ifndef G_OS_WIN32
      if (*p == '~')
	{
	  *n = '\0';
	  strcat (n, g_get_home_dir ());
	  n += strlen (g_get_home_dir ());
	  p += 1;
	}
      else
#endif
      if ((token = extract_token (&p)) != NULL)
        {
          for (i = 0; i < n_substs; i++)
            {
              if (strcmp (substs[2*i], token) == 0)
                {
                  s = substs[2*i+1];

                  *n = '\0';
                  strcat (n, s);
                  n += strlen (s);

                  break;
                }
            }

          g_free (token);
	}
      else
	{
	  *n++ = *p++;
	}
    }

  *n = '\0';

 cleanup:
  for (i = 0; i < n_substs; i++)
    g_free (substs[2*i]);

  g_free (substs);

  return new_path;  
}

static inline gchar *
extract_token (const gchar **str)
{
  const gchar *p;
  gchar       *token;

  if (strncmp (*str, "${", 2))
    return NULL;

  p = *str + 2;

  while (*p && (*p != '}'))
    p = g_utf8_next_char (p);

  if (!p)
    return NULL;

  token = g_strndup (*str + 2, g_utf8_pointer_to_offset (*str + 2, p));

  *str = p + 1; /* after the closing bracket */  

  return token;
}
