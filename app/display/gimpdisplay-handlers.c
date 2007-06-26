/* GIMP - The GNU Image Manipulation Program
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

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "file/file-utils.h"

#include "gimpdisplay.h"
#include "gimpdisplay-handlers.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_display_update_handler (GimpProjection *projection,
                                           gboolean        now,
                                           gint            x,
                                           gint            y,
                                           gint            w,
                                           gint            h,
                                           GimpDisplay    *display);
static void   gimp_display_flush_handler  (GimpImage      *image,
                                           gboolean        invalidate_preview,
                                           GimpDisplay    *display);
static void   gimp_display_saved_handler  (GimpImage      *image,
                                           const gchar    *uri,
                                           GimpDisplay    *display);


/*  public functions  */

void
gimp_display_connect (GimpDisplay *display,
                      GimpImage   *image)
{
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (display->image == NULL);

  display->image    = image;
  display->instance = image->instance_count;

  image->instance_count++;   /* this is obsolete */
  image->disp_count++;

#if 0
  g_print ("%s: image->ref_count before refing: %d\n",
           G_STRFUNC, G_OBJECT (display->image)->ref_count);
#endif

  g_object_ref (image);

  g_signal_connect (image->projection, "update",
                    G_CALLBACK (gimp_display_update_handler),
                    display);
  g_signal_connect (image, "flush",
                    G_CALLBACK (gimp_display_flush_handler),
                    display);
  g_signal_connect (image, "saved",
                    G_CALLBACK (gimp_display_saved_handler),
                    display);
}

void
gimp_display_disconnect (GimpDisplay *display)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (GIMP_IS_IMAGE (display->image));

  g_signal_handlers_disconnect_by_func (display->image,
                                        gimp_display_saved_handler,
                                        display);
  g_signal_handlers_disconnect_by_func (display->image,
                                        gimp_display_flush_handler,
                                        display);
  g_signal_handlers_disconnect_by_func (display->image->projection,
                                        gimp_display_update_handler,
                                        display);

  display->image->disp_count--;

#if 0
  g_print ("%s: image->ref_count before unrefing: %d\n",
           G_STRFUNC, G_OBJECT (display->image)->ref_count);
#endif

  /*  set display->image to NULL before unrefing because there may be code
   *  that listens for image removals and then iterates the display list
   *  to find a valid display.
   */
  image = display->image;
  display->image = NULL;

  g_object_unref (image);
}


/*  private functions  */

static void
gimp_display_update_handler (GimpProjection *projection,
                             gboolean        now,
                             gint            x,
                             gint            y,
                             gint            w,
                             gint            h,
                             GimpDisplay    *display)
{
  gimp_display_update_area (display, now, x, y, w, h);
}

static void
gimp_display_flush_handler (GimpImage   *image,
                            gboolean     invalidate_preview,
                            GimpDisplay *display)
{
  gimp_display_flush (display);
}

static void
gimp_display_saved_handler (GimpImage   *image,
                            const gchar *uri,
                            GimpDisplay *display)
{
  gchar *filename = file_utils_uri_display_name (uri);

  gimp_message (image->gimp, G_OBJECT (display), GIMP_MESSAGE_INFO,
                _("Image saved to '%s'"), filename);

  g_free (filename);
}
