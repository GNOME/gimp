/* gimpparasite.c: Copyright 1998 Jay Cox <jaycox@earthlink.net> 
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

#include <glib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "app_procs.h"
#include "parasitelist.h"
#include "gimpparasite.h"
#include "gimprc.h"

#include "libgimp/parasite.h"
#include "libgimp/gimpenv.h"

static ParasiteList *parasites = NULL;

void 
gimp_init_parasites (void)
{
  g_return_if_fail (parasites == NULL);
  parasites = parasite_list_new ();
  gimp_parasiterc_load ();
}

void
gimp_parasite_attach (GimpParasite *p)
{
  parasite_list_add (parasites, p);
}

void
gimp_parasite_detach (const gchar *name)
{
  parasite_list_remove (parasites, name);
}

GimpParasite *
gimp_parasite_find (const gchar *name)
{
  return parasite_list_find (parasites, name);
}

static void 
list_func (gchar          *key,
	   GimpParasite   *p,
	   gchar        ***cur)
{
  *(*cur)++ = (char *) g_strdup (key);
}

gchar **
gimp_parasite_list (gint *count)
{
  gchar **list;
  gchar **cur;

  *count = parasite_list_length (parasites);
  cur = list = g_new (gchar *, *count);

  parasite_list_foreach (parasites, (GHFunc) list_func, &cur);
  
  return list;
}

static void 
save_func (gchar        *key,
	   GimpParasite *p,
	   FILE         *fp)
{
  if (gimp_parasite_is_persistent (p))
    {
      gchar   *s;
      guint32  l;

      fprintf (fp, "(parasite \"%s\" %lu \"",
	       gimp_parasite_name (p), gimp_parasite_flags (p));

      /*
       * the current methodology is: never move the parasiterc from one
       * system to another. If you want to do this you should probably
       * write out parasites which contain any non-alphanumeric(+some)
       * characters as \xHH sequences altogether.
       */

      for (s = (gchar *) gimp_parasite_data (p), l = gimp_parasite_data_size (p);
           l;
           l--, s++)
        {
          switch (*s)
            {
              case '\\': fputs ("\\\\", fp); break;
              case '\0': fputs ("\\0", fp); break;
              case '"' : fputs ("\\\"", fp); break;
              /* disabled, not portable!  */
/*              case '\n': fputs ("\\n", fp); break;*/
/*              case '\r': fputs ("\\r", fp); break;*/
              case 26  : fputs ("\\z", fp); break;
              default  : fputc (*s, fp); break;
            }
        }
      
      fputs ("\")\n\n", fp);
    }
}

void
gimp_parasiterc_save (void)
{
  gchar *filename;
  FILE  *fp;

  filename = gimp_personal_rc_file ("#parasiterc.tmp~");

  fp = fopen (filename, "w");
  g_free (filename);
  if (!fp)
    return;

  fprintf (fp,
	   "# GIMP parasiterc\n"
	   "# This file will be entirely rewritten every time you "
	   "quit the gimp.\n\n");

  parasite_list_foreach (parasites, (GHFunc)save_func, fp);

  fclose (fp);

#if defined(G_OS_WIN32) || defined(__EMX__)
  /* First rename the old parasiterc out of the way */
  unlink (gimp_personal_rc_file ("parasiterc.bak"));
  rename (gimp_personal_rc_file ("parasiterc"),
	  gimp_personal_rc_file ("parasiterc.bak"));
#endif

  if (rename (gimp_personal_rc_file ("#parasiterc.tmp~"),
	      gimp_personal_rc_file ("parasiterc")) != 0)
    {
#if defined(G_OS_WIN32) || defined(__EMX__)
      /* Rename the old parasiterc back */
      rename (gimp_personal_rc_file ("parasiterc.bak"),
	      gimp_personal_rc_file ("parasiterc"));
#endif
      unlink (gimp_personal_rc_file ("#parasiterc.tmp~"));
    }
}

void
gimp_parasiterc_load (void)
{
  gchar *filename;

  filename = gimp_personal_rc_file ("parasiterc");
  app_init_update_status (NULL, filename, -1);
  parse_gimprc_file (filename);
  g_free (filename);
}
