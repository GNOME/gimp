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


static inline gchar * jump_to_next_char (const gchar  *str,
                                         glong        *length);
static inline gchar * extract_token     (const gchar **str);


gchar * 
gimp_config_substitute_path (GObject     *object,
                             const gchar *path,
                             gboolean     use_env)
{
  const gchar *subst;
  const gchar *p;
  gchar       *new_path = NULL;
  gchar       *n;
  gchar       *token;
  glong        length = 0;
  GSList      *list;
  GSList      *substitutions = NULL;

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
	  subst = gimp_config_lookup_unknown_token (object, token);

	  if (!subst && use_env) 
	    {
	      if (strcmp (token, "gimp_dir") == 0)
                subst = gimp_directory ();

              if (!subst)
                subst = g_getenv (token);

#ifdef G_OS_WIN32
	      /* The default user gimprc on Windows references
	       * ${TEMP}, but not all Windows installations have that
	       * environment variable, even if it should be kinda
	       * standard. So special-case it.
	       */
	      if (!subst && strcmp (token, "TEMP") == 0)
                subst = g_get_tmp_dir ();
#endif
            }
          
          if (!subst)
            {
              g_message ("token referenced but not defined: ${%s}", token);
  
              g_free (token);
              goto cleanup;
            }

          substitutions = g_slist_prepend (substitutions, g_strdup (subst));
          substitutions = g_slist_prepend (substitutions, token);

          length += strlen (subst);
        }
      else
	{
          p = jump_to_next_char (p, &length);
	}
    }

  if (!substitutions)
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
          for (list = substitutions; list; list = g_slist_next (list))
            {
              if (strcmp ((gchar *) list->data, token) == 0)
                {
                  list = g_slist_next (list);

                  *n = '\0';
                  strcat (n, (const gchar *) list->data);
                  n += strlen (list->data);

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
  for (list = substitutions; list; list = g_slist_next (list))
    g_free (list->data);

  g_slist_free (substitutions);

  return new_path;  
}

static inline gchar *
jump_to_next_char (const gchar *str,
                   glong       *length)
{
  gchar *next;
  
  next = g_utf8_next_char (str);
  
  if (next)
    *length += g_utf8_pointer_to_offset (str, next);
  else
    *length += strlen (str);
  
  return next;
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
