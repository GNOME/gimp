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

#include "gui-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpviewable.h"

#include "widgets/gimpdnd.h"
#include "widgets/gimpselectiondata.h"

#include "clipboard.h"

#include "gimp-intl.h"


static  const GtkTargetEntry  target_entry = GIMP_TARGET_PNG;


static void      clipboard_buffer_changed    (Gimp             *gimp);
static void      clipboard_set               (Gimp             *gimp,
                                              GimpBuffer       *buffer);

static void      clipboard_get               (GtkClipboard     *clipboard,
                                              GtkSelectionData *selection_data,
                                              guint             info,
                                              Gimp             *gimp);
static gboolean  clipboard_wait_is_available (void);


void
clipboard_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  clipboard_set (gimp, gimp->global_buffer);

  g_signal_connect_object (gimp, "buffer_changed",
                           G_CALLBACK (clipboard_buffer_changed),
                           NULL, 0);

}

void
clipboard_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_handlers_disconnect_by_func (gimp,
                                        G_CALLBACK (clipboard_buffer_changed),
                                        NULL);
  clipboard_set (gimp, NULL);
}

/**
 * clipboard_is_available:
 * @gimp: pointer to #Gimp
 *
 * Tests if there's image data in the clipboard. If the global cut
 * buffer of @gimp is empty, this function checks if there's image
 * data in %GDK_SELECTION_CLIPBOARD. This is done in a main-loop
 * similar to gtk_clipboard_wait_is_text_available(). The same caveats
 * apply here.
 *
 * Return value: %TRUE if there's image data in the clipboard, %FALSE otherwise
 **/
gboolean
clipboard_is_available (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  if (gimp->global_buffer)
    return TRUE;

  return clipboard_wait_is_available ();
}


static TileManager *
tile_manager_new_from_pixbuf (GdkPixbuf *pixbuf)
{
  TileManager *tiles;
  guchar      *pixels;
  PixelRegion  destPR;
  gint         width;
  gint         height;
  gint         rowstride;
  gint         channels;
  gint         y;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  width     = gdk_pixbuf_get_width (pixbuf);
  height    = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  channels  = gdk_pixbuf_get_n_channels (pixbuf);

  tiles = tile_manager_new (width, height, channels);

  pixel_region_init (&destPR, tiles, 0, 0, width, height, TRUE);

  for (y = 0, pixels = gdk_pixbuf_get_pixels (pixbuf);
       y < height;
       y++, pixels += rowstride)
    {
      pixel_region_set_row (&destPR, 0, y, width, pixels);
   }

  return tiles;
}

/**
 * clipboard_get_buffer:
 * @gimp: pointer to #Gimp
 *
 * Retrieves either image data from %GDK_SELECTION_CLIPBOARD or from
 * the global cut buffer of @gimp.
 *
 * The returned #GimpBuffer needs to be unref'ed when it's no longer
 * needed.
 *
 * Return value: a reference to a #GimpBuffer or %NULL if there's no
 *               image data
 **/
GimpBuffer *
clipboard_get_buffer (Gimp *gimp)
{
  GimpBuffer   *buffer = NULL;
  GtkClipboard *clipboard;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard                                              &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (gimp) &&
      clipboard_wait_is_available ())
    {
      GtkSelectionData *data;
      GdkAtom           atom = gdk_atom_intern (target_entry.target, FALSE);

      gimp_set_busy (gimp);

      data = gtk_clipboard_wait_for_contents (clipboard, atom);

      if (data)
        {
          GdkPixbuf *pixbuf = gimp_selection_data_get_pixbuf (data);

          gtk_selection_data_free (data);

          if (pixbuf)
            {
              TileManager *tiles = tile_manager_new_from_pixbuf (pixbuf);

              g_object_unref (pixbuf);

              buffer = gimp_buffer_new (tiles, _("Clipboard"), FALSE);
            }
        }

      gimp_unset_busy (gimp);
    }

  if (! buffer && gimp->global_buffer)
    buffer = g_object_ref (gimp->global_buffer);

  return buffer;
}

static void
clipboard_buffer_changed (Gimp *gimp)
{
  clipboard_set (gimp, gimp->global_buffer);
}

static void
clipboard_set (Gimp       *gimp,
               GimpBuffer *buffer)
{
  GtkClipboard *clipboard;

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  if (buffer)
    {
      gtk_clipboard_set_with_owner (clipboard,
                                    &target_entry, 1,
                                    (GtkClipboardGetFunc)   clipboard_get,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (gimp));
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (gimp))
    {
      gtk_clipboard_clear (clipboard);
    }
}

static gboolean
clipboard_wait_is_available (void)
{
  GtkClipboard *clipboard;
  gboolean      result = FALSE;

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard)
    {
      GtkSelectionData *data;

      data = gtk_clipboard_wait_for_contents (clipboard,
                                              gdk_atom_intern ("TARGETS",
                                                               FALSE));
      if (data)
        {
          GdkAtom *targets;
          gint     n_targets;

          if (gtk_selection_data_get_targets (data, &targets, &n_targets))
            {
              GdkAtom  atom = gdk_atom_intern (target_entry.target, FALSE);
              gint     i;

              for (i = 0; i < n_targets; i++)
                if (targets[i] == atom)
                  result = TRUE;

              g_free (targets);
            }

          gtk_selection_data_free (data);
        }
    }

  return result;
}

static void
clipboard_get (GtkClipboard     *clipboard,
               GtkSelectionData *selection_data,
               guint             info,
               Gimp             *gimp)
{
  GimpBuffer *buffer = gimp->global_buffer;
  GdkPixbuf  *pixbuf;

  gimp_set_busy (gimp);

  pixbuf = gimp_viewable_get_pixbuf (GIMP_VIEWABLE (buffer),
                                     gimp_buffer_get_width (buffer),
                                     gimp_buffer_get_height (buffer));

  if (pixbuf)
    {
      GdkAtom  atom = gdk_atom_intern (target_entry.target, FALSE);

      gimp_selection_data_set_pixbuf (selection_data, atom, pixbuf);
    }
  else
    {
      g_warning ("%s: gimp_viewable_get_pixbuf() failed", G_STRFUNC);
    }

  gimp_unset_busy (gimp);
}
