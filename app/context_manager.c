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

#include "context_manager.h"

void
context_manager_init (void)
{
  GimpContext *context;

  /*  Implicitly create the standard context
   */
  context = gimp_context_get_standard ();

  /*  To be loaded from disk later
   */
  context = gimp_context_new ("Default", NULL, NULL);
  gimp_context_set_default (context);

  /*  Finally the user context will be initialized with the default context's
   *  values.
   */
  context = gimp_context_new ("User", NULL, NULL);
  gimp_context_set_user (context);
  gimp_context_set_current (context);
}

void
context_manager_free (void)
{
  gtk_object_unref (GTK_OBJECT (gimp_context_get_user ()));
  gimp_context_set_user (NULL);
  gimp_context_set_current (NULL);

  /*  Save to disk before destroying later
   */
  gtk_object_unref (GTK_OBJECT (gimp_context_get_default ()));
  gimp_context_set_default (NULL);
}
