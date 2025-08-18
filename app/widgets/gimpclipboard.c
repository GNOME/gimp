/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcurve.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"

#include "gimpclipboard.h"
#include "gimppixbuf.h"
#include "gimpselectiondata.h"

#include "gimp-intl.h"


#define GIMP_CLIPBOARD_KEY "gimp-clipboard"


typedef struct _GimpClipboard GimpClipboard;

struct _GimpClipboard
{
  GSList         *pixbuf_formats;

  GtkTargetEntry *image_target_entries;
  gint            n_image_target_entries;

  GtkTargetEntry *buffer_target_entries;
  gint            n_buffer_target_entries;

  GtkTargetEntry *svg_target_entries;
  gint            n_svg_target_entries;

  GtkTargetEntry *curve_target_entries;
  gint            n_curve_target_entries;

  GimpImage      *image;
  GimpBuffer     *buffer;
  gchar          *svg;
  GimpCurve      *curve;
};


static GimpClipboard * gimp_clipboard_get        (Gimp             *gimp);

static GimpClipboard * gimp_clipboard_new        (gboolean          verbose);
static void            gimp_clipboard_free       (GimpClipboard    *gimp_clip);

static void      gimp_clipboard_clear            (GimpClipboard    *gimp_clip);

static GdkAtom * gimp_clipboard_wait_for_targets (Gimp             *gimp,
                                                  gint             *n_targets);
static GdkAtom   gimp_clipboard_wait_for_image   (Gimp             *gimp);
static GdkAtom   gimp_clipboard_wait_for_buffer  (Gimp             *gimp);
static GdkAtom   gimp_clipboard_wait_for_svg     (Gimp             *gimp);
static GdkAtom   gimp_clipboard_wait_for_curve   (Gimp             *gimp);

static void      gimp_clipboard_send_image       (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Gimp             *gimp);
static void      gimp_clipboard_send_buffer      (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Gimp             *gimp);
static void      gimp_clipboard_send_svg         (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Gimp             *gimp);
static void      gimp_clipboard_send_curve       (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Gimp             *gimp);


/*  public functions  */

void
gimp_clipboard_init (Gimp *gimp)
{
  GimpClipboard *gimp_clip;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_clip = gimp_clipboard_get (gimp);

  g_return_if_fail (gimp_clip == NULL);

  gimp_clip = gimp_clipboard_new (gimp->be_verbose);

  g_object_set_data_full (G_OBJECT (gimp), GIMP_CLIPBOARD_KEY,
                          gimp_clip, (GDestroyNotify) gimp_clipboard_free);
}

void
gimp_clipboard_exit (Gimp *gimp)
{
  GtkClipboard *clipboard;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) == G_OBJECT (gimp))
    {
      gtk_clipboard_store (clipboard);
    }

  if (clipboard)
    /* If we don't clear the clipboard, it keeps a reference on the object
     * owner (i.e. Gimp object probably) which fails to finalize.
     */
    gtk_clipboard_clear (clipboard);

  g_object_set_data (G_OBJECT (gimp), GIMP_CLIPBOARD_KEY, NULL);
}

/**
 * gimp_clipboard_has_image:
 * @gimp: pointer to #Gimp
 *
 * Tests if there's an image in the clipboard. If the global image cut
 * buffer of @gimp is empty, this function returns %NULL.
 *
 * Returns: %TRUE if there's an image in the clipboard, %FALSE otherwise
 **/
gboolean
gimp_clipboard_has_image (Gimp *gimp)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (gimp))
    {
      if (gimp_clipboard_wait_for_image (gimp) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  gimp_clip = gimp_clipboard_get (gimp);

  return (gimp_clip->image != NULL);
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
 * Returns: %TRUE if there's image data in the clipboard, %FALSE otherwise
 **/
gboolean
gimp_clipboard_has_buffer (Gimp *gimp)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (gimp))
    {
      if (gimp_clipboard_wait_for_buffer (gimp) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  gimp_clip = gimp_clipboard_get (gimp);

  return (gimp_clip->buffer != NULL);
}

/**
 * gimp_clipboard_has_svg:
 * @gimp: pointer to #Gimp
 *
 * Tests if there's SVG data in %GDK_SELECTION_CLIPBOARD.
 * This is done in a main-loop similar to
 * gtk_clipboard_wait_is_text_available(). The same caveats apply here.
 *
 * Returns: %TRUE if there's SVG data in the clipboard, %FALSE otherwise
 **/
gboolean
gimp_clipboard_has_svg (Gimp *gimp)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (gimp))
    {
      if (gimp_clipboard_wait_for_svg (gimp) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  gimp_clip = gimp_clipboard_get (gimp);

  return (gimp_clip->svg != NULL);
}

/**
 * gimp_clipboard_has_curve:
 * @gimp: pointer to #Gimp
 *
 * Tests if there's curve data in %GDK_SELECTION_CLIPBOARD.
 * This is done in a main-loop similar to
 * gtk_clipboard_wait_is_text_available(). The same caveats apply here.
 *
 * Returns: %TRUE if there's curve data in the clipboard, %FALSE otherwise
 **/
gboolean
gimp_clipboard_has_curve (Gimp *gimp)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (gimp))
    {
      if (gimp_clipboard_wait_for_curve (gimp) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  gimp_clip = gimp_clipboard_get (gimp);

  return (gimp_clip->curve != NULL);
}

/**
 * gimp_clipboard_get_object:
 * @gimp: pointer to #Gimp
 *
 * Retrieves either an image or a buffer from the global image cut
 * buffer of @gimp.
 *
 * The returned #GimpObject needs to be unref'ed when it's no longer
 * needed.
 *
 * Returns: (nullable): a reference to a #GimpObject or %NULL if there's no
 *               image or buffer in the clipboard
 **/
GimpObject *
gimp_clipboard_get_object (Gimp *gimp)
{
  GimpObject *object;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  object = GIMP_OBJECT (gimp_clipboard_get_image (gimp));

  if (! object)
    object = GIMP_OBJECT (gimp_clipboard_get_buffer (gimp));

  return object;
}

/**
 * gimp_clipboard_get_image:
 * @gimp: pointer to #Gimp
 *
 * Retrieves an image from the global image cut buffer of @gimp.
 *
 * The returned #GimpImage needs to be unref'ed when it's no longer
 * needed.
 *
 * Returns: (nullable): a reference to a #GimpImage or %NULL if there's no
 *               image in the clipboard
 **/
GimpImage *
gimp_clipboard_get_image (Gimp *gimp)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;
  GimpImage     *image = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (gimp))
    {
      GdkAtom atom = gimp_clipboard_wait_for_image (gimp);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          gimp_set_busy (gimp);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              image = gimp_selection_data_get_xcf (data, gimp);

              gtk_selection_data_free (data);
            }

          gimp_unset_busy (gimp);
        }

      return image;
    }

  gimp_clip = gimp_clipboard_get (gimp);

  if (! image && gimp_clip->image)
    image = g_object_ref (gimp_clip->image);

  return image;
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
 * Returns: (nullable): a reference to a #GimpBuffer or %NULL if there's no
 *               image data
 **/
GimpBuffer *
gimp_clipboard_get_buffer (Gimp *gimp)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;
  GimpBuffer    *buffer = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (gimp))
    {
      GdkAtom atom = gimp_clipboard_wait_for_buffer (gimp);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          gimp_set_busy (gimp);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              GdkPixbuf *pixbuf = gtk_selection_data_get_pixbuf (data);

              gtk_selection_data_free (data);

              if (pixbuf)
                {
                  buffer = gimp_buffer_new_from_pixbuf (pixbuf, _("Clipboard"),
                                                        0, 0);
                  g_object_unref (pixbuf);
                }
            }

          gimp_unset_busy (gimp);
        }

      return buffer;
    }

  gimp_clip = gimp_clipboard_get (gimp);

  if (! buffer && gimp_clip->buffer)
    buffer = g_object_ref (gimp_clip->buffer);

  return buffer;
}

/**
 * gimp_clipboard_get_svg:
 * @gimp: pointer to #Gimp
 * @svg_length: returns the size of the SVG stream in bytes
 *
 * Retrieves SVG data from %GDK_SELECTION_CLIPBOARD or from the global
 * SVG buffer of @gimp.
 *
 * The returned data needs to be freed when it's no longer needed.
 *
 * Returns: (nullable): a reference to a #GimpBuffer or %NULL if there's no
 *               image data
 **/
gchar *
gimp_clipboard_get_svg (Gimp  *gimp,
                        gsize *svg_length)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;
  gchar         *svg = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (svg_length != NULL, NULL);

  *svg_length = 0;

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (gimp))
    {
      GdkAtom atom = gimp_clipboard_wait_for_svg (gimp);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          gimp_set_busy (gimp);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              const guchar *stream;

              stream = gimp_selection_data_get_stream (data, svg_length);

              if (stream)
                svg = g_memdup2 (stream, *svg_length);

              gtk_selection_data_free (data);
            }

          gimp_unset_busy (gimp);
        }

      return svg;
    }

  gimp_clip = gimp_clipboard_get (gimp);

  if (! svg && gimp_clip->svg)
    {
      svg = g_strdup (gimp_clip->svg);
      *svg_length = strlen (svg);
    }

  return svg;
}

/**
 * gimp_clipboard_get_curve:
 * @gimp: pointer to #Gimp
 *
 * Retrieves curve data from %GDK_SELECTION_CLIPBOARD or from the global
 * curve buffer of @gimp.
 *
 * The returned curve needs to be unref'ed when it's no longer needed.
 *
 * Returns: (nullable): a reference to a #GimpCurve or %NULL if there's no
 *               curve data
 **/
GimpCurve *
gimp_clipboard_get_curve (Gimp *gimp)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;
  GimpCurve     *curve = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (gimp))
    {
      GdkAtom atom = gimp_clipboard_wait_for_curve (gimp);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          gimp_set_busy (gimp);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              curve = gimp_selection_data_get_curve (data);

              gtk_selection_data_free (data);
            }

          gimp_unset_busy (gimp);
        }

      return curve;
    }

  gimp_clip = gimp_clipboard_get (gimp);

  if (! curve && gimp_clip->curve)
    curve = g_object_ref (gimp_clip->curve);

  return curve;
}

/**
 * gimp_clipboard_set_image:
 * @gimp:  pointer to #Gimp
 * @image: (nullable): a #GimpImage, or %NULL.
 *
 * Offers the image in %GDK_SELECTION_CLIPBOARD.
 **/
void
gimp_clipboard_set_image (Gimp      *gimp,
                          GimpImage *image)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  gimp_clip = gimp_clipboard_get (gimp);

  gimp_clipboard_clear (gimp_clip);

  if (image)
    {
      gimp_clip->image = g_object_ref (image);

      gtk_clipboard_set_with_owner (clipboard,
                                    gimp_clip->image_target_entries,
                                    gimp_clip->n_image_target_entries,
                                    (GtkClipboardGetFunc) gimp_clipboard_send_image,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (gimp));

      /*  mark the first two entries (image/x-xcf and image/png) as
       *  suitable for storing
       */
      gtk_clipboard_set_can_store (clipboard,
                                   gimp_clip->image_target_entries,
                                   MIN (2, gimp_clip->n_image_target_entries));
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (gimp))
    {
      gtk_clipboard_clear (clipboard);
    }
}

/**
 * gimp_clipboard_set_buffer:
 * @gimp:   pointer to #Gimp
 * @buffer: (nullable): a #GimpBuffer, or %NULL.
 *
 * Offers the buffer in %GDK_SELECTION_CLIPBOARD.
 **/
void
gimp_clipboard_set_buffer (Gimp       *gimp,
                           GimpBuffer *buffer)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (buffer == NULL || GIMP_IS_BUFFER (buffer));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  gimp_clip = gimp_clipboard_get (gimp);

  gimp_clipboard_clear (gimp_clip);

  if (buffer)
    {
      gimp_clip->buffer = g_object_ref (buffer);

      gtk_clipboard_set_with_owner (clipboard,
                                    gimp_clip->buffer_target_entries,
                                    gimp_clip->n_buffer_target_entries,
                                    (GtkClipboardGetFunc) gimp_clipboard_send_buffer,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (gimp));

      /*  mark the first entry (image/png) as suitable for storing  */
      if (gimp_clip->n_buffer_target_entries > 0)
        gtk_clipboard_set_can_store (clipboard,
                                     gimp_clip->buffer_target_entries, 1);
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (gimp))
    {
      gtk_clipboard_clear (clipboard);
    }
}

/**
 * gimp_clipboard_set_svg:
 * @gimp: pointer to #Gimp
 * @svg: (nullable): a string containing the SVG data, or %NULL
 *
 * Offers SVG data in %GDK_SELECTION_CLIPBOARD.
 **/
void
gimp_clipboard_set_svg (Gimp        *gimp,
                        const gchar *svg)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  gimp_clip = gimp_clipboard_get (gimp);

  gimp_clipboard_clear (gimp_clip);

  if (svg)
    {
      gimp_clip->svg = g_strdup (svg);

      gtk_clipboard_set_with_owner (clipboard,
                                    gimp_clip->svg_target_entries,
                                    gimp_clip->n_svg_target_entries,
                                    (GtkClipboardGetFunc) gimp_clipboard_send_svg,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (gimp));

      /*  mark the first entry (image/svg) as suitable for storing  */
      gtk_clipboard_set_can_store (clipboard,
                                   gimp_clip->svg_target_entries, 1);
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (gimp))
    {
      gtk_clipboard_clear (clipboard);
    }
}

/**
 * gimp_clipboard_set_text:
 * @gimp: pointer to #Gimp
 * @text: a %NULL-terminated string in UTF-8 encoding
 *
 * Offers @text in %GDK_SELECTION_CLIPBOARD and %GDK_SELECTION_PRIMARY.
 **/
void
gimp_clipboard_set_text (Gimp        *gimp,
                         const gchar *text)
{
  GtkClipboard *clipboard;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (text != NULL);

  gimp_clipboard_clear (gimp_clipboard_get (gimp));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (clipboard)
    gtk_clipboard_set_text (clipboard, text, -1);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_PRIMARY);
  if (clipboard)
    gtk_clipboard_set_text (clipboard, text, -1);
}

/**
 * gimp_clipboard_set_curve:
 * @gimp: pointer to #Gimp
 * @curve: (nullable): a #GimpCurve, or %NULL
 *
 * Offers curve data in %GDK_SELECTION_CLIPBOARD.
 **/
void
gimp_clipboard_set_curve (Gimp      *gimp,
                          GimpCurve *curve)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (curve == NULL || GIMP_IS_CURVE (curve));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  gimp_clip = gimp_clipboard_get (gimp);

  gimp_clipboard_clear (gimp_clip);

  if (curve)
    {
      gimp_clip->curve = g_object_ref (curve);

      gtk_clipboard_set_with_owner (clipboard,
                                    gimp_clip->curve_target_entries,
                                    gimp_clip->n_curve_target_entries,
                                    (GtkClipboardGetFunc) gimp_clipboard_send_curve,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (gimp));

      gtk_clipboard_set_can_store (clipboard,
                                   gimp_clip->curve_target_entries, 1);
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (gimp))
    {
      gtk_clipboard_clear (clipboard);
    }
}


/*  private functions  */

static GimpClipboard *
gimp_clipboard_get (Gimp *gimp)
{
  return g_object_get_data (G_OBJECT (gimp), GIMP_CLIPBOARD_KEY);
}

static GimpClipboard *
gimp_clipboard_new (gboolean verbose)
{
  GimpClipboard *gimp_clip = g_slice_new0 (GimpClipboard);
  GSList        *list;

  gimp_clip->pixbuf_formats = gimp_pixbuf_get_formats ();

  for (list = gimp_clip->pixbuf_formats; list; list = g_slist_next (list))
    {
      GdkPixbufFormat *format = list->data;

      if (gdk_pixbuf_format_is_writable (format))
        {
          gchar **mime_types;
          gchar **type;

          mime_types = gdk_pixbuf_format_get_mime_types (format);

          for (type = mime_types; *type; type++)
            gimp_clip->n_buffer_target_entries++;

          g_strfreev (mime_types);
        }
    }

  /*  the image_target_entries have the XCF target, and all pixbuf
   *  targets that are also in buffer_target_entries
   */
  gimp_clip->n_image_target_entries = gimp_clip->n_buffer_target_entries + 1;
  gimp_clip->image_target_entries   = g_new0 (GtkTargetEntry,
                                              gimp_clip->n_image_target_entries);

  gimp_clip->image_target_entries[0].target = g_strdup ("image/x-xcf");
  gimp_clip->image_target_entries[0].flags  = 0;
  gimp_clip->image_target_entries[0].info   = 0;

  if (gimp_clip->n_buffer_target_entries > 0)
    {
      gint i = 0;

      gimp_clip->buffer_target_entries = g_new0 (GtkTargetEntry,
                                                 gimp_clip->n_buffer_target_entries);

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
                  const gchar *mime_type = *type;

                  if (verbose)
                    g_printerr ("clipboard: writable pixbuf format: %s\n",
                                mime_type);

                  gimp_clip->image_target_entries[i + 1].target = g_strdup (mime_type);
                  gimp_clip->image_target_entries[i + 1].flags  = 0;
                  gimp_clip->image_target_entries[i + 1].info   = i + 1;

                  gimp_clip->buffer_target_entries[i].target = g_strdup (mime_type);
                  gimp_clip->buffer_target_entries[i].flags  = 0;
                  gimp_clip->buffer_target_entries[i].info   = i;

                  i++;
                }

              g_strfreev (mime_types);
              g_free (format_name);
            }
        }
    }

  gimp_clip->n_svg_target_entries = 2;
  gimp_clip->svg_target_entries   = g_new0 (GtkTargetEntry, 2);

  gimp_clip->svg_target_entries[0].target = g_strdup ("image/svg");
  gimp_clip->svg_target_entries[0].flags  = 0;
  gimp_clip->svg_target_entries[0].info   = 0;

  gimp_clip->svg_target_entries[1].target = g_strdup ("image/svg+xml");
  gimp_clip->svg_target_entries[1].flags  = 0;
  gimp_clip->svg_target_entries[1].info   = 1;

  gimp_clip->n_curve_target_entries = 1;
  gimp_clip->curve_target_entries   = g_new0 (GtkTargetEntry, 1);

  gimp_clip->curve_target_entries[0].target = g_strdup ("application/x-gimp-curve");
  gimp_clip->curve_target_entries[0].flags  = 0;
  gimp_clip->curve_target_entries[0].info   = 0;

  return gimp_clip;
}

static void
gimp_clipboard_free (GimpClipboard *gimp_clip)
{
  gint i;

  gimp_clipboard_clear (gimp_clip);

  g_slist_free (gimp_clip->pixbuf_formats);

  for (i = 0; i < gimp_clip->n_image_target_entries; i++)
    g_free ((gchar *) gimp_clip->image_target_entries[i].target);

  g_free (gimp_clip->image_target_entries);

  for (i = 0; i < gimp_clip->n_buffer_target_entries; i++)
    g_free ((gchar *) gimp_clip->buffer_target_entries[i].target);

  g_free (gimp_clip->buffer_target_entries);

  for (i = 0; i < gimp_clip->n_svg_target_entries; i++)
    g_free ((gchar *) gimp_clip->svg_target_entries[i].target);

  g_free (gimp_clip->svg_target_entries);

  for (i = 0; i < gimp_clip->n_curve_target_entries; i++)
    g_free ((gchar *) gimp_clip->curve_target_entries[i].target);

  g_free (gimp_clip->curve_target_entries);

  g_slice_free (GimpClipboard, gimp_clip);
}

static void
gimp_clipboard_clear (GimpClipboard *gimp_clip)
{
  g_clear_object (&gimp_clip->image);
  g_clear_object (&gimp_clip->buffer);
  g_clear_pointer (&gimp_clip->svg, g_free);
  g_clear_object (&gimp_clip->curve);
}

static GdkAtom *
gimp_clipboard_wait_for_targets (Gimp *gimp,
                                 gint *n_targets)
{
  GtkClipboard *clipboard;

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard)
    {
      GtkSelectionData *data;
      GdkAtom           atom = gdk_atom_intern_static_string ("TARGETS");

      data = gtk_clipboard_wait_for_contents (clipboard, atom);

      if (data)
        {
          GdkAtom  *targets;
          gboolean  success;

          success = gtk_selection_data_get_targets (data, &targets, n_targets);

          gtk_selection_data_free (data);

          if (success)
            {
              if (gimp->be_verbose)
                {
                  gint i;

                  for (i = 0; i < *n_targets; i++)
                    g_printerr ("clipboard: offered type: %s\n",
                                gdk_atom_name (targets[i]));

                  g_printerr ("\n");
                }

              return targets;
            }
        }
    }

  return NULL;
}

static GdkAtom
gimp_clipboard_wait_for_image (Gimp *gimp)
{
  GdkAtom *targets;
  gint     n_targets;
  GdkAtom  result = GDK_NONE;

  targets = gimp_clipboard_wait_for_targets (gimp, &n_targets);

  if (targets)
    {
      GdkAtom image_atom = gdk_atom_intern_static_string ("image/x-xcf");
      gint    i;

      for (i = 0; i < n_targets; i++)
        {
          if (targets[i] == image_atom)
            {
              result = image_atom;
              break;
            }
        }

      g_free (targets);
    }

  return result;
}

static GdkAtom
gimp_clipboard_wait_for_buffer (Gimp *gimp)
{
  GimpClipboard *gimp_clip = gimp_clipboard_get (gimp);
  GdkAtom       *targets;
  gint           n_targets;
  GdkAtom        result    = GDK_NONE;

  targets = gimp_clipboard_wait_for_targets (gimp, &n_targets);

  if (targets)
    {
      GSList *list;

      for (list = gimp_clip->pixbuf_formats; list; list = g_slist_next (list))
        {
          GdkPixbufFormat  *format = list->data;
          gchar           **mime_types;
          gchar           **type;

          if (gimp->be_verbose)
            g_printerr ("clipboard: checking pixbuf format '%s'\n",
                        gdk_pixbuf_format_get_name (format));

          mime_types = gdk_pixbuf_format_get_mime_types (format);

          for (type = mime_types; *type; type++)
            {
              gchar   *mime_type = *type;
              GdkAtom  atom      = gdk_atom_intern (mime_type, FALSE);
              gint     i;

              if (gimp->be_verbose)
                g_printerr ("  - checking mime type '%s'\n", mime_type);

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

static GdkAtom
gimp_clipboard_wait_for_svg (Gimp *gimp)
{
  GdkAtom *targets;
  gint     n_targets;
  GdkAtom  result = GDK_NONE;

  targets = gimp_clipboard_wait_for_targets (gimp, &n_targets);

  if (targets)
    {
      GdkAtom svg_atom     = gdk_atom_intern_static_string ("image/svg");
      GdkAtom svg_xml_atom = gdk_atom_intern_static_string ("image/svg+xml");
      gint    i;

      for (i = 0; i < n_targets; i++)
        {
          if (targets[i] == svg_atom)
            {
              result = svg_atom;
              break;
            }
          else if (targets[i] == svg_xml_atom)
            {
              result = svg_xml_atom;
              break;
            }
        }

      g_free (targets);
    }

  return result;
}

static GdkAtom
gimp_clipboard_wait_for_curve (Gimp *gimp)
{
  GdkAtom *targets;
  gint     n_targets;
  GdkAtom  result = GDK_NONE;

  targets = gimp_clipboard_wait_for_targets (gimp, &n_targets);

  if (targets)
    {
      GdkAtom curve_atom = gdk_atom_intern_static_string ("application/x-gimp-curve");
      gint    i;

      for (i = 0; i < n_targets; i++)
        {
          if (targets[i] == curve_atom)
            {
              result = curve_atom;
              break;
            }
        }

      g_free (targets);
    }

  return result;
}

static void
gimp_clipboard_send_image (GtkClipboard     *clipboard,
                           GtkSelectionData *data,
                           guint             info,
                           Gimp             *gimp)
{
  GimpClipboard *gimp_clip = gimp_clipboard_get (gimp);

  gimp_set_busy (gimp);

  if (info == 0)
    {
      if (gimp->be_verbose)
        g_printerr ("clipboard: sending image data as '%s'\n",
                    gimp_clip->image_target_entries[info].target);

      gimp_selection_data_set_xcf (data, gimp_clip->image);
    }
  else
    {
      GdkPixbuf *pixbuf;

      gimp_pickable_flush (GIMP_PICKABLE (gimp_clip->image));

      pixbuf = gimp_viewable_get_pixbuf (GIMP_VIEWABLE (gimp_clip->image),
                                         gimp_get_user_context (gimp),
                                         gimp_image_get_width (gimp_clip->image),
                                         gimp_image_get_height (gimp_clip->image),
                                         NULL);

      if (pixbuf)
        {
          gdouble res_x;
          gdouble res_y;
          gchar   str[16];

          gimp_image_get_resolution (gimp_clip->image, &res_x, &res_y);

          g_snprintf (str, sizeof (str), "%d", ROUND (res_x));
          gdk_pixbuf_set_option (pixbuf, "x-dpi", str);

          g_snprintf (str, sizeof (str), "%d", ROUND (res_y));
          gdk_pixbuf_set_option (pixbuf, "y-dpi", str);

          if (gimp->be_verbose)
            g_printerr ("clipboard: sending image data as '%s'\n",
                        gimp_clip->image_target_entries[info].target);

          gtk_selection_data_set_pixbuf (data, pixbuf);
        }
      else
        {
          g_warning ("%s: gimp_viewable_get_pixbuf() failed", G_STRFUNC);
        }
    }

  gimp_unset_busy (gimp);
}

static void
gimp_clipboard_send_buffer (GtkClipboard     *clipboard,
                            GtkSelectionData *data,
                            guint             info,
                            Gimp             *gimp)
{
  GimpClipboard *gimp_clip = gimp_clipboard_get (gimp);
  GdkPixbuf     *pixbuf;

  gimp_set_busy (gimp);

  pixbuf = gimp_viewable_get_pixbuf (GIMP_VIEWABLE (gimp_clip->buffer),
                                     gimp_get_user_context (gimp),
                                     gimp_buffer_get_width (gimp_clip->buffer),
                                     gimp_buffer_get_height (gimp_clip->buffer),
                                     NULL);

  if (pixbuf)
    {
      gdouble res_x;
      gdouble res_y;
      gchar   str[16];

      gimp_buffer_get_resolution (gimp_clip->buffer, &res_x, &res_y);

      g_snprintf (str, sizeof (str), "%d", ROUND (res_x));
      gdk_pixbuf_set_option (pixbuf, "x-dpi", str);

      g_snprintf (str, sizeof (str), "%d", ROUND (res_y));
      gdk_pixbuf_set_option (pixbuf, "y-dpi", str);

      if (gimp->be_verbose)
        g_printerr ("clipboard: sending pixbuf data as '%s'\n",
                    gimp_clip->buffer_target_entries[info].target);

      gtk_selection_data_set_pixbuf (data, pixbuf);
    }
  else
    {
      g_warning ("%s: gimp_viewable_get_pixbuf() failed", G_STRFUNC);
    }

  gimp_unset_busy (gimp);
}

static void
gimp_clipboard_send_svg (GtkClipboard     *clipboard,
                         GtkSelectionData *data,
                         guint             info,
                         Gimp             *gimp)
{
  GimpClipboard *gimp_clip = gimp_clipboard_get (gimp);

  gimp_set_busy (gimp);

  if (gimp_clip->svg)
    {
      if (gimp->be_verbose)
        g_printerr ("clipboard: sending SVG data as '%s'\n",
                    gimp_clip->svg_target_entries[info].target);

      gimp_selection_data_set_stream (data,
                                      (const guchar *) gimp_clip->svg,
                                      strlen (gimp_clip->svg));
    }

  gimp_unset_busy (gimp);
}

static void
gimp_clipboard_send_curve (GtkClipboard     *clipboard,
                           GtkSelectionData *data,
                           guint             info,
                           Gimp             *gimp)
{
  GimpClipboard *gimp_clip = gimp_clipboard_get (gimp);

  gimp_set_busy (gimp);

  if (gimp_clip->curve)
    {
      if (gimp->be_verbose)
        g_printerr ("clipboard: sending curve data as '%s'\n",
                    gimp_clip->curve_target_entries[info].target);

      gimp_selection_data_set_curve (data, gimp_clip->curve);
    }

  gimp_unset_busy (gimp);
}
