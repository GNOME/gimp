/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpdnd-xds.c
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
 *
 * Saving Files via Drag-and-Drop:
 * The Direct Save Protocol for the X Window System
 *
 *   http://www.newplanetsoftware.com/xds/
 *   http://rox.sourceforge.net/xds.html
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

#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "file/file-save.h"
#include "file/file-utils.h"

#include "gimpdnd-xds.h"

#include "gimp-intl.h"


#ifdef DEBUG_DND
#define D(stmnt) stmnt
#else
#define D(stmnt)
#endif


#define MAX_URI_LEN 4096


void
gimp_dnd_xds_source_set (GdkDragContext *context,
                         GimpImage      *image)
{
  GdkAtom  property;
  gchar   *filename;

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  D (g_printerr ("\ngimp_dnd_xds_source_set\n"));

  property = gdk_atom_intern ("XdndDirectSave0", FALSE);

  if (image && (filename = gimp_image_get_filename (image)))
    {
      GdkAtom  type     = gdk_atom_intern ("text/plain", FALSE);
      gchar   *basename = g_path_get_basename (filename);

      gdk_property_change (context->source_window,
                           property, type, 8, GDK_PROP_MODE_REPLACE,
                           basename, strlen (basename));

      g_free (basename);
      g_free (filename);
    }
  else
    {
      gdk_property_delete (context->source_window, property);
    }
}

void
gimp_dnd_xds_save_image (GdkDragContext   *context,
                         GimpImage        *image,
                         GtkSelectionData *selection,
                         GdkAtom           atom)
{
  PlugInProcDef *proc;
  GdkAtom        property = gdk_atom_intern ("XdndDirectSave0", FALSE);
  GdkAtom        type     = gdk_atom_intern ("text/plain", FALSE);
  gint           length;
  guchar        *data;
  gchar         *uri;
  GError        *error = NULL;

  g_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  D (g_printerr ("\ngimp_dnd_xds_save_image\n"));

  if (! gdk_property_get (context->source_window, property, type,
                          0, MAX_URI_LEN, FALSE,
                          NULL, NULL, &length, &data))
    return;


  uri = g_strndup ((const gchar *) data, length);
  g_free (data);

  proc = file_utils_find_proc (image->gimp->save_procs, uri);

  if (! proc)
    proc = file_utils_find_proc (image->gimp->save_procs,
                                 gimp_object_get_name (GIMP_OBJECT (image)));

  if (proc)
    {
      if (file_save (image, gimp_get_user_context (image->gimp), NULL,
                     uri, proc, GIMP_RUN_INTERACTIVE, FALSE,
                     &error) == GIMP_PDB_SUCCESS)
        {
          gtk_selection_data_set (selection, atom, 8, "S", 1);
        }
      else
        {
          gtk_selection_data_set (selection, atom, 8, "F", 1);

          if (error)
            {
              gchar *filename = file_utils_uri_to_utf8_filename (uri);

              g_message (_("Saving '%s' failed:\n\n%s"),
                         filename, error->message);

              g_free (filename);
              g_error_free (error);
            }
        }
    }
  else
    {
      gtk_selection_data_set (selection, atom, 8, "F", 1);

      g_message (_("The given filename does not have any known "
                   "file extension."));
    }

  g_free (uri);

  gimp_dnd_xds_source_set (context, NULL);
}
