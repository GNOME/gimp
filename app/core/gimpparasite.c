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

#include <gtk/gtk.h>

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpparasite.h"
#include "gimpparasitelist.h"

#include "gimprc.h"


void
gimp_parasites_init (Gimp *gimp)
{
  g_return_if_fail (gimp != NULL);
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp->parasites == NULL);

  gimp->parasites = gimp_parasite_list_new ();
}

void 
gimp_parasites_exit (Gimp *gimp)
{
  g_return_if_fail (gimp != NULL);
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->parasites)
    {
      g_object_unref (G_OBJECT (gimp->parasites));
      gimp->parasites = NULL;
    }
}

void
gimp_parasite_attach (Gimp         *gimp,
		      GimpParasite *parasite)
{
  gimp_parasite_list_add (gimp->parasites, parasite);
}

void
gimp_parasite_detach (Gimp        *gimp,
		      const gchar *name)
{
  gimp_parasite_list_remove (gimp->parasites, name);
}

GimpParasite *
gimp_parasite_find (Gimp        *gimp,
		    const gchar *name)
{
  return gimp_parasite_list_find (gimp->parasites, name);
}

static void 
list_func (gchar          *key,
	   GimpParasite   *p,
	   gchar        ***cur)
{
  *(*cur)++ = (char *) g_strdup (key);
}

gchar **
gimp_parasite_list (Gimp *gimp,
		    gint *count)
{
  gchar **list;
  gchar **cur;

  *count = gimp_parasite_list_length (gimp->parasites);
  cur = list = g_new (gchar *, *count);

  gimp_parasite_list_foreach (gimp->parasites, (GHFunc) list_func, &cur);
  
  return list;
}

static void 
save_func (gchar        *key,
	   GimpParasite *parasite,
	   FILE         *fp)
{
  if (gimp_parasite_is_persistent (parasite))
    {
      gchar   *s;
      guint32  l;

      fprintf (fp, "(parasite \"%s\" %lu \"",
	       gimp_parasite_name (parasite),
	       gimp_parasite_flags (parasite));

      /*
       * the current methodology is: never move the parasiterc from one
       * system to another. If you want to do this you should probably
       * write out parasites which contain any non-alphanumeric(+some)
       * characters as \xHH sequences altogether.
       */

      for (s = (gchar *) gimp_parasite_data (parasite),
	     l = gimp_parasite_data_size (parasite);
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
gimp_parasiterc_load (Gimp *gimp)
{
  gchar *filename;

  filename = gimp_personal_rc_file ("parasiterc");
  gimprc_parse_file (filename);
  g_free (filename);
}

void
gimp_parasiterc_save (Gimp *gimp)
{
  gchar *tmp_filename = NULL;
  gchar *bak_filename = NULL;
  gchar *rc_filename  = NULL;
  FILE  *fp;

  tmp_filename = gimp_personal_rc_file ("#parasiterc.tmp~");
  bak_filename = gimp_personal_rc_file ("parasiterc.bak");
  rc_filename  = gimp_personal_rc_file ("parasiterc");

  fp = fopen (tmp_filename, "w");

  if (!fp)
    goto cleanup;

  fprintf (fp,
	   "# GIMP parasiterc\n"
	   "# This file will be entirely rewritten every time you "
	   "quit the gimp.\n\n");

  gimp_parasite_list_foreach (gimp->parasites, (GHFunc) save_func, fp);

  fclose (fp);

#if defined(G_OS_WIN32) || defined(__EMX__)
  /* First rename the old parasiterc out of the way */
  unlink (bak_filename);
  rename (rc_filename, bak_filename);
#endif

  if (rename (tmp_filename, rc_filename) != 0)
    {
#if defined(G_OS_WIN32) || defined(__EMX__)
      /* Rename the old parasiterc back */
      rename (bak_filename, rc_filename);
#endif
      unlink (tmp_filename);
    }

 cleanup:
  g_free (tmp_filename);
  g_free (bak_filename);
  g_free (rc_filename);
}
