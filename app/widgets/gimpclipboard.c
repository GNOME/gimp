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

#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpviewable.h"

#include "gimpclipboard.h"
#include "gimpdnd.h"
#include "gimpselectiondata.h"

#include "gimp-intl.h"


#define GIMP_CLIPBOARD_KEY "gimp-clipboard"


typedef struct _GimpClipboard GimpClipboard;

struct _GimpClipboard
{
  GSList          *pixbuf_formats;

  GtkTargetEntry  *target_entries;
  gint             n_target_entries;
  gchar          **savers;
};


static GimpClipboard * gimp_clipboard_get        (Gimp             *gimp);
static void            gimp_clipboard_free       (GimpClipboard    *gimp_clip);

static void      gimp_clipboard_buffer_changed   (Gimp             *gimp);
static void      gimp_clipboard_set_buffer       (Gimp             *gimp,
                                                  GimpBuffer       *buffer);

static void      gimp_clipboard_send_buffer      (GtkClipboard     *clipboard,
                                                  GtkSelectionData *selection_data,
                                                  guint             info,
                                                  Gimp             *gimp);

static GdkAtom * gimp_clipboard_wait_for_targets (gint             *n_targets);
static GdkAtom   gimp_clipboard_wait_for_buffer  (Gimp             *gimp);

static gint      gimp_clipboard_format_compare   (GdkPixbufFormat  *a,
                                                  GdkPixbufFormat  *b);


void
gimp_clipboard_init (Gimp *gimp)
{
  GimpClipboard *gimp_clip;
  GSList        *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_clip = gimp_clipboard_get (gimp);

  g_return_if_fail (gimp_clip == NULL);

  gimp_clip = g_new0 (GimpClipboard, 1);

  g_object_set_data_full (G_OBJECT (gimp), GIMP_CLIPBOARD_KEY,
                          gimp_clip, (GDestroyNotify) gimp_clipboard_free);

  gimp_clipboard_set_buffer (gimp, gimp->global_buffer);

  g_signal_connect_object (gimp, "buffer_changed",
                           G_CALLBACK (gimp_clipboard_buffer_changed),
                           NULL, 0);

  gimp_clip->pixbuf_formats =
    g_slist_sort (gdk_pixbuf_get_formats (),
                  (GCompareFunc) gimp_clipboard_format_compare);

  for (list = gimp_clip->pixbuf_formats; list; list = g_slist_next (list))
    {
      GdkPixbufFormat *format = list->data;

      if (gdk_pixbuf_format_is_writable (format))
        {
          gchar **mime_types;
          gchar **type;

          mime_types = gdk_pixbuf_format_get_mime_types (format);

          for (type = mime_types; *type; type++)
            gimp_clip->n_target_entries++;

          g_strfreev (mime_types);
        }
    }

  if (gimp_clip->n_target_entries > 0)
    {
      gint i = 0;

      gimp_clip->target_entries = g_new0 (GtkTargetEntry,
                                          gimp_clip->n_target_entries);
      gimp_clip->savers         = g_new0 (gchar *,
                                          gimp_clip->n_target_entries + 1);

      for (list = gimp_clip->pixbuf_formats; list; list = g_slist_next (list))
        {
          GdkPixbufFormat *format = list->data;

          if (gdk_pixbuf_format_is_writable (format))
            {
              gchar  *format_name;
              gchar **mime_types;
              gchar **type;

              format_name = gdk_pixbuf_format_get_name (format);
              mime_types  = gdk_pixbuf_format_get_mime_types (format);

              for (type = mime_types; *type; type++)
                {
                  gchar *mime_type = *type;

                  if (gimp->be_verbose)
                    g_print ("GimpClipboard: writable pixbuf format: %s\n",
                             mime_type);

                  gimp_clip->target_entries[i].target = g_strdup (mime_type);
                  gimp_clip->target_entries[i].flags  = 0;
                  gimp_clip->target_entries[i].info   = i;

                  gimp_clip->savers[i]                = g_strdup (format_name);

                  i++;
                }

              g_strfreev (mime_types);
              g_free (format_name);
            }
        }
    }
}

void
gimp_clipboard_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_handlers_disconnect_by_func (gimp,
                                        G_CALLBACK (gimp_clipboard_buffer_changed),
                                        NULL);
  gimp_clipboard_set_buffer (gimp, NULL);

  g_object_set_data (G_OBJECT (gimp), GIMP_CLIPBOARD_KEY, NULL);
}

/**
 * gimp_clipboard_has_buffer:
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
gimp_clipboard_has_buffer (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  if (gimp->global_buffer)
    return TRUE;

  return (gimp_clipboard_wait_for_buffer (gimp) != GDK_NONE);
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
 * gimp_clipboard_get_buffer:
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
gimp_clipboard_get_buffer (Gimp *gimp)
{
  GimpBuffer   *buffer = NULL;
  GtkClipboard *clipboard;
  GdkAtom       atom;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard                                                         &&
      gtk_clipboard_get_owner (clipboard)            != G_OBJECT (gimp) &&
      (atom = gimp_clipboard_wait_for_buffer (gimp)) != GDK_NONE)
    {
      GtkSelectionData *data;

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


/*  private functions  */

static GimpClipboard *
gimp_clipboard_get (Gimp *gimp)
{
  return g_object_get_data (G_OBJECT (gimp), GIMP_CLIPBOARD_KEY);
}

static void
gimp_clipboard_free (GimpClipboard *gimp_clip)
{
  g_slist_free (gimp_clip->pixbuf_formats);
  g_free (gimp_clip->target_entries);
  g_strfreev (gimp_clip->savers);
  g_free (gimp_clip);
}

static void
gimp_clipboard_buffer_changed (Gimp *gimp)
{
  gimp_clipboard_set_buffer (gimp, gimp->global_buffer);
}

static void
gimp_clipboard_set_buffer (Gimp       *gimp,
                           GimpBuffer *buffer)
{
  GimpClipboard *gimp_clip = gimp_clipboard_get (gimp);
  GtkClipboard  *clipboard;

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  if (buffer)
    {
      gtk_clipboard_set_with_owner (clipboard,
                                    gimp_clip->target_entries,
                                    gimp_clip->n_target_entries,
                                    (GtkClipboardGetFunc)   gimp_clipboard_send_buffer,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (gimp));
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (gimp))
    {
      gtk_clipboard_clear (clipboard);
    }
}

static GdkAtom *
gimp_clipboard_wait_for_targets (gint *n_targets)
{
  GtkClipboard *clipboard;

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
          GdkAtom  *targets;
          gboolean  success;

          success = gtk_selection_data_get_targets (data, &targets, n_targets);

          gtk_selection_data_free (data);

          if (success)
            {
              gint i;

              for (i = 0; i < *n_targets; i++)
                g_print ("offered type: %s\n", gdk_atom_name (targets[i]));

              g_print ("\n");

              return targets;
            }
        }
    }

  return NULL;
}

static GdkAtom
gimp_clipboard_wait_for_buffer (Gimp *gimp)
{
  GimpClipboard *gimp_clip = gimp_clipboard_get (gimp);
  GdkAtom       *targets;
  gint           n_targets;
  GdkAtom        result    = GDK_NONE;

  targets = gimp_clipboard_wait_for_targets (&n_targets);

  if (targets)
    {
      GSList *list;

      for (list = gimp_clip->pixbuf_formats; list; list = g_slist_next (list))
        {
          GdkPixbufFormat  *format = list->data;
          gchar           **mime_types;
          gchar           **type;

          g_print ("checking pixbuf format '%s'\n",
                   gdk_pixbuf_format_get_name (format));

          mime_types = gdk_pixbuf_format_get_mime_types (format);

          for (type = mime_types; *type; type++)
            {
              gchar   *mime_type = *type;
              GdkAtom  atom      = gdk_atom_intern (mime_type, FALSE);
              gint     i;

              g_print (" - checking mime type '%s'\n", mime_type);

              for (i = 0; i < n_targets; i++)
                {
                  if (targets[i] == atom)
                    {
                      result = atom;
                      break;
                    }
                }

              if (result != GDK_NONE)
                break;
            }

          g_strfreev (mime_types);

          if (result != GDK_NONE)
            break;
        }

      g_free (targets);
    }

  return result;
}

static void
gimp_clipboard_send_buffer (GtkClipboard     *clipboard,
                            GtkSelectionData *selection_data,
                            guint             info,
                            Gimp             *gimp)
{
  GimpClipboard *gimp_clip = gimp_clipboard_get (gimp);
  GimpBuffer    *buffer    = gimp->global_buffer;
  GdkPixbuf     *pixbuf;

  gimp_set_busy (gimp);

  pixbuf = gimp_viewable_get_pixbuf (GIMP_VIEWABLE (buffer),
                                     gimp_buffer_get_width (buffer),
                                     gimp_buffer_get_height (buffer));

  if (pixbuf)
    {
      GdkAtom atom = gdk_atom_intern (gimp_clip->target_entries[info].target,
                                      FALSE);

      g_print ("sending pixbuf data as '%s' (%s)\n",
               gimp_clip->target_entries[info].target,
               gimp_clip->savers[info]);

      gimp_selection_data_set_pixbuf (selection_data, atom, pixbuf,
                                      gimp_clip->savers[info]);
    }
  else
    {
      g_warning ("%s: gimp_viewable_get_pixbuf() failed", G_STRFUNC);
    }

  gimp_unset_busy (gimp);
}

static gint
gimp_clipboard_format_compare (GdkPixbufFormat *a,
                               GdkPixbufFormat *b)
{
  gchar *a_name = gdk_pixbuf_format_get_name (a);
  gchar *b_name = gdk_pixbuf_format_get_name (b);
  gint   retval = 0;

#ifdef GDK_WINDOWING_WIN32
  /*  move BMP to the front of the list  */
  if (strcmp (a_name, "bmp") == 0)
    retval = -1;
  else if (strcmp (b_name, "bmp") == 0)
    retval = 1;
  else
#endif

  /*  move PNG to the front of the list  */
  if (strcmp (a_name, "png") == 0)
    retval = -1;
  else if (strcmp (b_name, "png") == 0)
    retval = 1;

  /*  move JPEG to the end of the list  */
  else if (strcmp (a_name, "jpeg") == 0)
    retval = 1;
  else if (strcmp (b_name, "jpeg") == 0)
    retval = -1;

  /*  move GIF to the end of the list  */
  else if (strcmp (a_name, "gif") == 0)
    retval = 1;
  else if (strcmp (b_name, "gif") == 0)
    retval = -1;

  g_free (a_name);
  g_free (b_name);

  return retval;
}
