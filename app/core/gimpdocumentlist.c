/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <glib-object.h>

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpscanner.h"

#include "gimp.h"
#include "gimpdocuments.h"
#include "gimpimagefile.h"
#include "gimplist.h"

#include "libgimp/gimpintl.h"


void
gimp_documents_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp->documents == NULL);

  gimp->documents = gimp_list_new (GIMP_TYPE_IMAGEFILE,
				   GIMP_CONTAINER_POLICY_STRONG);
  gimp_object_set_name (GIMP_OBJECT (gimp->documents), "documents");
}

void 
gimp_documents_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->documents)
    {
      g_object_unref (G_OBJECT (gimp->documents));
      gimp->documents = NULL;
    }
}

void
gimp_documents_load (Gimp *gimp)
{
  gchar      *filename;
  GScanner   *scanner;
  GTokenType  token;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("documents");
  scanner = gimp_scanner_new (filename);
  g_free (filename);

  if (! scanner)
    return;

  g_scanner_scope_add_symbol (scanner, 0,
                              "document", GINT_TO_POINTER (1));

  token = G_TOKEN_LEFT_PAREN;

  do
    {
      if (g_scanner_peek_next_token (scanner) != token)
        break;

      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          token = G_TOKEN_RIGHT_PAREN;
          if (scanner->value.v_symbol == GINT_TO_POINTER (1))
            {
              gchar         *uri;
              GimpImagefile *imagefile;

              if (! gimp_scanner_parse_string (scanner, &uri))
                {
                  token = G_TOKEN_STRING;
                  break;
                }

              imagefile = gimp_imagefile_new (uri);

              g_free (uri);

              GIMP_LIST (gimp->documents)->list =
                g_list_append (GIMP_LIST (gimp->documents)->list, imagefile);

              gimp->documents->num_children++;
            }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }
  while (token != G_TOKEN_EOF);

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }

  gimp_scanner_destroy (scanner);
}

static void
gimp_documents_save_func (GimpImagefile *imagefile,
			  FILE          *fp)
{
  fprintf (fp, "(document \"%s\")\n", GIMP_OBJECT (imagefile)->name);
}

void
gimp_documents_save (Gimp *gimp)
{
  gchar *tmp_filename = NULL;
  gchar *bak_filename = NULL;
  gchar *rc_filename  = NULL;
  FILE  *fp;

  tmp_filename = gimp_personal_rc_file ("#documents.tmp~");
  bak_filename = gimp_personal_rc_file ("documents.bak");
  rc_filename  = gimp_personal_rc_file ("documents");

  fp = fopen (tmp_filename, "w");

  if (! fp)
    goto cleanup;

  fprintf (fp,
	   "# GIMP documents\n"
	   "#\n"
	   "# This file will be entirely rewritten every time you\n"
	   "# quit the gimp.\n\n");

  gimp_container_foreach (gimp->documents,
			  (GFunc) gimp_documents_save_func,
			  fp);

  fclose (fp);

#if defined(G_OS_WIN32) || defined(__EMX__)
  /* First rename the old documents out of the way */
  unlink (bak_filename);
  rename (rc_filename, bak_filename);
#endif

  if (rename (tmp_filename, rc_filename) != 0)
    {
#if defined(G_OS_WIN32) || defined(__EMX__)
      /* Rename the old documentrc back */
      rename (bak_filename, rc_filename);
#endif
      unlink (tmp_filename);
    }

 cleanup:
  g_free (tmp_filename);
  g_free (bak_filename);
  g_free (rc_filename);
}

void
gimp_documents_add (Gimp        *gimp,
		    const gchar *filename)
{
  GimpImagefile *imagefile;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (filename != NULL);

  imagefile = (GimpImagefile *)
    gimp_container_get_child_by_name (gimp->documents, filename);

  if (imagefile)
    {
      gimp_container_reorder (gimp->documents, GIMP_OBJECT (imagefile), 0);
    }
  else
    {
      imagefile = gimp_imagefile_new (filename);

      gimp_container_add (gimp->documents, GIMP_OBJECT (imagefile));
      g_object_unref (G_OBJECT (imagefile));
    }
}
