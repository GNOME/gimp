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

#include "appenv.h"
#include "gdisplay.h"

static void
user_context_display_changed (GimpContext *context,
			      GDisplay    *display,
			      gpointer     data)
{
  gdisplay_set_menu_sensitivity (display);
}

/* FIXME: finally, install callbacks for all created contexts to prevent
 *        the image from appearing without notifying us
 */
static void
image_context_image_removed (GimpSet     *set,
			     GimpImage   *gimage,
			     GimpContext *user_context)
{
  if (gimp_context_get_image (user_context) == gimage)
    gimp_context_set_image (user_context, NULL);
}

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

  gtk_signal_connect (GTK_OBJECT (context), "display_changed",
		      GTK_SIGNAL_FUNC (user_context_display_changed),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (image_context), "remove",
		      GTK_SIGNAL_FUNC (image_context_image_removed),
		      context);
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
