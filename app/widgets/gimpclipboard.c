/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmabuffer.h"
#include "core/ligmacurve.h"
#include "core/ligmaimage.h"
#include "core/ligmapickable.h"

#include "ligmaclipboard.h"
#include "ligmapixbuf.h"
#include "ligmaselectiondata.h"

#include "ligma-intl.h"


#define LIGMA_CLIPBOARD_KEY "ligma-clipboard"


typedef struct _LigmaClipboard LigmaClipboard;

struct _LigmaClipboard
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

  LigmaImage      *image;
  LigmaBuffer     *buffer;
  gchar          *svg;
  LigmaCurve      *curve;
};


static LigmaClipboard * ligma_clipboard_get        (Ligma             *ligma);

static LigmaClipboard * ligma_clipboard_new        (gboolean          verbose);
static void            ligma_clipboard_free       (LigmaClipboard    *ligma_clip);

static void      ligma_clipboard_clear            (LigmaClipboard    *ligma_clip);

static GdkAtom * ligma_clipboard_wait_for_targets (Ligma             *ligma,
                                                  gint             *n_targets);
static GdkAtom   ligma_clipboard_wait_for_image   (Ligma             *ligma);
static GdkAtom   ligma_clipboard_wait_for_buffer  (Ligma             *ligma);
static GdkAtom   ligma_clipboard_wait_for_svg     (Ligma             *ligma);
static GdkAtom   ligma_clipboard_wait_for_curve   (Ligma             *ligma);

static void      ligma_clipboard_send_image       (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Ligma             *ligma);
static void      ligma_clipboard_send_buffer      (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Ligma             *ligma);
static void      ligma_clipboard_send_svg         (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Ligma             *ligma);
static void      ligma_clipboard_send_curve       (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Ligma             *ligma);


/*  public functions  */

void
ligma_clipboard_init (Ligma *ligma)
{
  LigmaClipboard *ligma_clip;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma_clip = ligma_clipboard_get (ligma);

  g_return_if_fail (ligma_clip == NULL);

  ligma_clip = ligma_clipboard_new (ligma->be_verbose);

  g_object_set_data_full (G_OBJECT (ligma), LIGMA_CLIPBOARD_KEY,
                          ligma_clip, (GDestroyNotify) ligma_clipboard_free);
}

void
ligma_clipboard_exit (Ligma *ligma)
{
  GtkClipboard *clipboard;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) == G_OBJECT (ligma))
    {
      gtk_clipboard_store (clipboard);
    }

  if (clipboard)
    /* If we don't clear the clipboard, it keeps a reference on the object
     * owner (i.e. Ligma object probably) which fails to finalize.
     */
    gtk_clipboard_clear (clipboard);

  g_object_set_data (G_OBJECT (ligma), LIGMA_CLIPBOARD_KEY, NULL);
}

/**
 * ligma_clipboard_has_image:
 * @ligma: pointer to #Ligma
 *
 * Tests if there's an image in the clipboard. If the global image cut
 * buffer of @ligma is empty, this function returns %NULL.
 *
 * Returns: %TRUE if there's an image in the clipboard, %FALSE otherwise
 **/
gboolean
ligma_clipboard_has_image (Ligma *ligma)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (ligma))
    {
      if (ligma_clipboard_wait_for_image (ligma) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  ligma_clip = ligma_clipboard_get (ligma);

  return (ligma_clip->image != NULL);
}

/**
 * ligma_clipboard_has_buffer:
 * @ligma: pointer to #Ligma
 *
 * Tests if there's image data in the clipboard. If the global cut
 * buffer of @ligma is empty, this function checks if there's image
 * data in %GDK_SELECTION_CLIPBOARD. This is done in a main-loop
 * similar to gtk_clipboard_wait_is_text_available(). The same caveats
 * apply here.
 *
 * Returns: %TRUE if there's image data in the clipboard, %FALSE otherwise
 **/
gboolean
ligma_clipboard_has_buffer (Ligma *ligma)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (ligma))
    {
      if (ligma_clipboard_wait_for_buffer (ligma) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  ligma_clip = ligma_clipboard_get (ligma);

  return (ligma_clip->buffer != NULL);
}

/**
 * ligma_clipboard_has_svg:
 * @ligma: pointer to #Ligma
 *
 * Tests if there's SVG data in %GDK_SELECTION_CLIPBOARD.
 * This is done in a main-loop similar to
 * gtk_clipboard_wait_is_text_available(). The same caveats apply here.
 *
 * Returns: %TRUE if there's SVG data in the clipboard, %FALSE otherwise
 **/
gboolean
ligma_clipboard_has_svg (Ligma *ligma)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (ligma))
    {
      if (ligma_clipboard_wait_for_svg (ligma) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  ligma_clip = ligma_clipboard_get (ligma);

  return (ligma_clip->svg != NULL);
}

/**
 * ligma_clipboard_has_curve:
 * @ligma: pointer to #Ligma
 *
 * Tests if there's curve data in %GDK_SELECTION_CLIPBOARD.
 * This is done in a main-loop similar to
 * gtk_clipboard_wait_is_text_available(). The same caveats apply here.
 *
 * Returns: %TRUE if there's curve data in the clipboard, %FALSE otherwise
 **/
gboolean
ligma_clipboard_has_curve (Ligma *ligma)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (ligma))
    {
      if (ligma_clipboard_wait_for_curve (ligma) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  ligma_clip = ligma_clipboard_get (ligma);

  return (ligma_clip->curve != NULL);
}

/**
 * ligma_clipboard_get_object:
 * @ligma: pointer to #Ligma
 *
 * Retrieves either an image or a buffer from the global image cut
 * buffer of @ligma.
 *
 * The returned #LigmaObject needs to be unref'ed when it's no longer
 * needed.
 *
 * Returns: (nullable): a reference to a #LigmaObject or %NULL if there's no
 *               image or buffer in the clipboard
 **/
LigmaObject *
ligma_clipboard_get_object (Ligma *ligma)
{
  LigmaObject *object;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  object = LIGMA_OBJECT (ligma_clipboard_get_image (ligma));

  if (! object)
    object = LIGMA_OBJECT (ligma_clipboard_get_buffer (ligma));

  return object;
}

/**
 * ligma_clipboard_get_image:
 * @ligma: pointer to #Ligma
 *
 * Retrieves an image from the global image cut buffer of @ligma.
 *
 * The returned #LigmaImage needs to be unref'ed when it's no longer
 * needed.
 *
 * Returns: (nullable): a reference to a #LigmaImage or %NULL if there's no
 *               image in the clipboard
 **/
LigmaImage *
ligma_clipboard_get_image (Ligma *ligma)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;
  LigmaImage     *image = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (ligma))
    {
      GdkAtom atom = ligma_clipboard_wait_for_image (ligma);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          ligma_set_busy (ligma);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              image = ligma_selection_data_get_xcf (data, ligma);

              gtk_selection_data_free (data);
            }

          ligma_unset_busy (ligma);
        }

      return image;
    }

  ligma_clip = ligma_clipboard_get (ligma);

  if (! image && ligma_clip->image)
    image = g_object_ref (ligma_clip->image);

  return image;
}

/**
 * ligma_clipboard_get_buffer:
 * @ligma: pointer to #Ligma
 *
 * Retrieves either image data from %GDK_SELECTION_CLIPBOARD or from
 * the global cut buffer of @ligma.
 *
 * The returned #LigmaBuffer needs to be unref'ed when it's no longer
 * needed.
 *
 * Returns: (nullable): a reference to a #LigmaBuffer or %NULL if there's no
 *               image data
 **/
LigmaBuffer *
ligma_clipboard_get_buffer (Ligma *ligma)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;
  LigmaBuffer    *buffer = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (ligma))
    {
      GdkAtom atom = ligma_clipboard_wait_for_buffer (ligma);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          ligma_set_busy (ligma);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              GdkPixbuf *pixbuf = gtk_selection_data_get_pixbuf (data);

              gtk_selection_data_free (data);

              if (pixbuf)
                {
                  buffer = ligma_buffer_new_from_pixbuf (pixbuf, _("Clipboard"),
                                                        0, 0);
                  g_object_unref (pixbuf);
                }
            }

          ligma_unset_busy (ligma);
        }

      return buffer;
    }

  ligma_clip = ligma_clipboard_get (ligma);

  if (! buffer && ligma_clip->buffer)
    buffer = g_object_ref (ligma_clip->buffer);

  return buffer;
}

/**
 * ligma_clipboard_get_svg:
 * @ligma: pointer to #Ligma
 * @svg_length: returns the size of the SVG stream in bytes
 *
 * Retrieves SVG data from %GDK_SELECTION_CLIPBOARD or from the global
 * SVG buffer of @ligma.
 *
 * The returned data needs to be freed when it's no longer needed.
 *
 * Returns: (nullable): a reference to a #LigmaBuffer or %NULL if there's no
 *               image data
 **/
gchar *
ligma_clipboard_get_svg (Ligma  *ligma,
                        gsize *svg_length)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;
  gchar         *svg = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (svg_length != NULL, NULL);

  *svg_length = 0;

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (ligma))
    {
      GdkAtom atom = ligma_clipboard_wait_for_svg (ligma);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          ligma_set_busy (ligma);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              const guchar *stream;

              stream = ligma_selection_data_get_stream (data, svg_length);

              if (stream)
                svg = g_memdup2 (stream, *svg_length);

              gtk_selection_data_free (data);
            }

          ligma_unset_busy (ligma);
        }

      return svg;
    }

  ligma_clip = ligma_clipboard_get (ligma);

  if (! svg && ligma_clip->svg)
    {
      svg = g_strdup (ligma_clip->svg);
      *svg_length = strlen (svg);
    }

  return svg;
}

/**
 * ligma_clipboard_get_curve:
 * @ligma: pointer to #Ligma
 *
 * Retrieves curve data from %GDK_SELECTION_CLIPBOARD or from the global
 * curve buffer of @ligma.
 *
 * The returned curve needs to be unref'ed when it's no longer needed.
 *
 * Returns: (nullable): a reference to a #LigmaCurve or %NULL if there's no
 *               curve data
 **/
LigmaCurve *
ligma_clipboard_get_curve (Ligma *ligma)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;
  LigmaCurve     *curve = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (ligma))
    {
      GdkAtom atom = ligma_clipboard_wait_for_curve (ligma);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          ligma_set_busy (ligma);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              curve = ligma_selection_data_get_curve (data);

              gtk_selection_data_free (data);
            }

          ligma_unset_busy (ligma);
        }

      return curve;
    }

  ligma_clip = ligma_clipboard_get (ligma);

  if (! curve && ligma_clip->curve)
    curve = g_object_ref (ligma_clip->curve);

  return curve;
}

/**
 * ligma_clipboard_set_image:
 * @ligma:  pointer to #Ligma
 * @image: (nullable): a #LigmaImage, or %NULL.
 *
 * Offers the image in %GDK_SELECTION_CLIPBOARD.
 **/
void
ligma_clipboard_set_image (Ligma      *ligma,
                          LigmaImage *image)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (image == NULL || LIGMA_IS_IMAGE (image));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  ligma_clip = ligma_clipboard_get (ligma);

  ligma_clipboard_clear (ligma_clip);

  if (image)
    {
      ligma_clip->image = g_object_ref (image);

      gtk_clipboard_set_with_owner (clipboard,
                                    ligma_clip->image_target_entries,
                                    ligma_clip->n_image_target_entries,
                                    (GtkClipboardGetFunc) ligma_clipboard_send_image,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (ligma));

      /*  mark the first two entries (image/x-xcf and image/png) as
       *  suitable for storing
       */
      gtk_clipboard_set_can_store (clipboard,
                                   ligma_clip->image_target_entries,
                                   MIN (2, ligma_clip->n_image_target_entries));
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (ligma))
    {
      gtk_clipboard_clear (clipboard);
    }
}

/**
 * ligma_clipboard_set_buffer:
 * @ligma:   pointer to #Ligma
 * @buffer: (nullable): a #LigmaBuffer, or %NULL.
 *
 * Offers the buffer in %GDK_SELECTION_CLIPBOARD.
 **/
void
ligma_clipboard_set_buffer (Ligma       *ligma,
                           LigmaBuffer *buffer)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (buffer == NULL || LIGMA_IS_BUFFER (buffer));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  ligma_clip = ligma_clipboard_get (ligma);

  ligma_clipboard_clear (ligma_clip);

  if (buffer)
    {
      ligma_clip->buffer = g_object_ref (buffer);

      gtk_clipboard_set_with_owner (clipboard,
                                    ligma_clip->buffer_target_entries,
                                    ligma_clip->n_buffer_target_entries,
                                    (GtkClipboardGetFunc) ligma_clipboard_send_buffer,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (ligma));

      /*  mark the first entry (image/png) as suitable for storing  */
      if (ligma_clip->n_buffer_target_entries > 0)
        gtk_clipboard_set_can_store (clipboard,
                                     ligma_clip->buffer_target_entries, 1);
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (ligma))
    {
      gtk_clipboard_clear (clipboard);
    }
}

/**
 * ligma_clipboard_set_svg:
 * @ligma: pointer to #Ligma
 * @svg: (nullable): a string containing the SVG data, or %NULL
 *
 * Offers SVG data in %GDK_SELECTION_CLIPBOARD.
 **/
void
ligma_clipboard_set_svg (Ligma        *ligma,
                        const gchar *svg)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  ligma_clip = ligma_clipboard_get (ligma);

  ligma_clipboard_clear (ligma_clip);

  if (svg)
    {
      ligma_clip->svg = g_strdup (svg);

      gtk_clipboard_set_with_owner (clipboard,
                                    ligma_clip->svg_target_entries,
                                    ligma_clip->n_svg_target_entries,
                                    (GtkClipboardGetFunc) ligma_clipboard_send_svg,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (ligma));

      /*  mark the first entry (image/svg) as suitable for storing  */
      gtk_clipboard_set_can_store (clipboard,
                                   ligma_clip->svg_target_entries, 1);
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (ligma))
    {
      gtk_clipboard_clear (clipboard);
    }
}

/**
 * ligma_clipboard_set_text:
 * @ligma: pointer to #Ligma
 * @text: a %NULL-terminated string in UTF-8 encoding
 *
 * Offers @text in %GDK_SELECTION_CLIPBOARD and %GDK_SELECTION_PRIMARY.
 **/
void
ligma_clipboard_set_text (Ligma        *ligma,
                         const gchar *text)
{
  GtkClipboard *clipboard;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (text != NULL);

  ligma_clipboard_clear (ligma_clipboard_get (ligma));

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
 * ligma_clipboard_set_curve:
 * @ligma: pointer to #Ligma
 * @curve: (nullable): a #LigmaCurve, or %NULL
 *
 * Offers curve data in %GDK_SELECTION_CLIPBOARD.
 **/
void
ligma_clipboard_set_curve (Ligma      *ligma,
                          LigmaCurve *curve)
{
  LigmaClipboard *ligma_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (curve == NULL || LIGMA_IS_CURVE (curve));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  ligma_clip = ligma_clipboard_get (ligma);

  ligma_clipboard_clear (ligma_clip);

  if (curve)
    {
      ligma_clip->curve = g_object_ref (curve);

      gtk_clipboard_set_with_owner (clipboard,
                                    ligma_clip->curve_target_entries,
                                    ligma_clip->n_curve_target_entries,
                                    (GtkClipboardGetFunc) ligma_clipboard_send_curve,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (ligma));

      gtk_clipboard_set_can_store (clipboard,
                                   ligma_clip->curve_target_entries, 1);
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (ligma))
    {
      gtk_clipboard_clear (clipboard);
    }
}


/*  private functions  */

static LigmaClipboard *
ligma_clipboard_get (Ligma *ligma)
{
  return g_object_get_data (G_OBJECT (ligma), LIGMA_CLIPBOARD_KEY);
}

static LigmaClipboard *
ligma_clipboard_new (gboolean verbose)
{
  LigmaClipboard *ligma_clip = g_slice_new0 (LigmaClipboard);
  GSList        *list;

  ligma_clip->pixbuf_formats = ligma_pixbuf_get_formats ();

  for (list = ligma_clip->pixbuf_formats; list; list = g_slist_next (list))
    {
      GdkPixbufFormat *format = list->data;

      if (gdk_pixbuf_format_is_writable (format))
        {
          gchar **mime_types;
          gchar **type;

          mime_types = gdk_pixbuf_format_get_mime_types (format);

          for (type = mime_types; *type; type++)
            ligma_clip->n_buffer_target_entries++;

          g_strfreev (mime_types);
        }
    }

  /*  the image_target_entries have the XCF target, and all pixbuf
   *  targets that are also in buffer_target_entries
   */
  ligma_clip->n_image_target_entries = ligma_clip->n_buffer_target_entries + 1;
  ligma_clip->image_target_entries   = g_new0 (GtkTargetEntry,
                                              ligma_clip->n_image_target_entries);

  ligma_clip->image_target_entries[0].target = g_strdup ("image/x-xcf");
  ligma_clip->image_target_entries[0].flags  = 0;
  ligma_clip->image_target_entries[0].info   = 0;

  if (ligma_clip->n_buffer_target_entries > 0)
    {
      gint i = 0;

      ligma_clip->buffer_target_entries = g_new0 (GtkTargetEntry,
                                                 ligma_clip->n_buffer_target_entries);

      for (list = ligma_clip->pixbuf_formats; list; list = g_slist_next (list))
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

                  ligma_clip->image_target_entries[i + 1].target = g_strdup (mime_type);
                  ligma_clip->image_target_entries[i + 1].flags  = 0;
                  ligma_clip->image_target_entries[i + 1].info   = i + 1;

                  ligma_clip->buffer_target_entries[i].target = g_strdup (mime_type);
                  ligma_clip->buffer_target_entries[i].flags  = 0;
                  ligma_clip->buffer_target_entries[i].info   = i;

                  i++;
                }

              g_strfreev (mime_types);
              g_free (format_name);
            }
        }
    }

  ligma_clip->n_svg_target_entries = 2;
  ligma_clip->svg_target_entries   = g_new0 (GtkTargetEntry, 2);

  ligma_clip->svg_target_entries[0].target = g_strdup ("image/svg");
  ligma_clip->svg_target_entries[0].flags  = 0;
  ligma_clip->svg_target_entries[0].info   = 0;

  ligma_clip->svg_target_entries[1].target = g_strdup ("image/svg+xml");
  ligma_clip->svg_target_entries[1].flags  = 0;
  ligma_clip->svg_target_entries[1].info   = 1;

  ligma_clip->n_curve_target_entries = 1;
  ligma_clip->curve_target_entries   = g_new0 (GtkTargetEntry, 1);

  ligma_clip->curve_target_entries[0].target = g_strdup ("application/x-ligma-curve");
  ligma_clip->curve_target_entries[0].flags  = 0;
  ligma_clip->curve_target_entries[0].info   = 0;

  return ligma_clip;
}

static void
ligma_clipboard_free (LigmaClipboard *ligma_clip)
{
  gint i;

  ligma_clipboard_clear (ligma_clip);

  g_slist_free (ligma_clip->pixbuf_formats);

  for (i = 0; i < ligma_clip->n_image_target_entries; i++)
    g_free ((gchar *) ligma_clip->image_target_entries[i].target);

  g_free (ligma_clip->image_target_entries);

  for (i = 0; i < ligma_clip->n_buffer_target_entries; i++)
    g_free ((gchar *) ligma_clip->buffer_target_entries[i].target);

  g_free (ligma_clip->buffer_target_entries);

  for (i = 0; i < ligma_clip->n_svg_target_entries; i++)
    g_free ((gchar *) ligma_clip->svg_target_entries[i].target);

  g_free (ligma_clip->svg_target_entries);

  for (i = 0; i < ligma_clip->n_curve_target_entries; i++)
    g_free ((gchar *) ligma_clip->curve_target_entries[i].target);

  g_free (ligma_clip->curve_target_entries);

  g_slice_free (LigmaClipboard, ligma_clip);
}

static void
ligma_clipboard_clear (LigmaClipboard *ligma_clip)
{
  g_clear_object (&ligma_clip->image);
  g_clear_object (&ligma_clip->buffer);
  g_clear_pointer (&ligma_clip->svg, g_free);
  g_clear_object (&ligma_clip->curve);
}

static GdkAtom *
ligma_clipboard_wait_for_targets (Ligma *ligma,
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
              if (ligma->be_verbose)
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
ligma_clipboard_wait_for_image (Ligma *ligma)
{
  GdkAtom *targets;
  gint     n_targets;
  GdkAtom  result = GDK_NONE;

  targets = ligma_clipboard_wait_for_targets (ligma, &n_targets);

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
ligma_clipboard_wait_for_buffer (Ligma *ligma)
{
  LigmaClipboard *ligma_clip = ligma_clipboard_get (ligma);
  GdkAtom       *targets;
  gint           n_targets;
  GdkAtom        result    = GDK_NONE;

  targets = ligma_clipboard_wait_for_targets (ligma, &n_targets);

  if (targets)
    {
      GSList *list;

      for (list = ligma_clip->pixbuf_formats; list; list = g_slist_next (list))
        {
          GdkPixbufFormat  *format = list->data;
          gchar           **mime_types;
          gchar           **type;

          if (ligma->be_verbose)
            g_printerr ("clipboard: checking pixbuf format '%s'\n",
                        gdk_pixbuf_format_get_name (format));

          mime_types = gdk_pixbuf_format_get_mime_types (format);

          for (type = mime_types; *type; type++)
            {
              gchar   *mime_type = *type;
              GdkAtom  atom      = gdk_atom_intern (mime_type, FALSE);
              gint     i;

              if (ligma->be_verbose)
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
ligma_clipboard_wait_for_svg (Ligma *ligma)
{
  GdkAtom *targets;
  gint     n_targets;
  GdkAtom  result = GDK_NONE;

  targets = ligma_clipboard_wait_for_targets (ligma, &n_targets);

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
ligma_clipboard_wait_for_curve (Ligma *ligma)
{
  GdkAtom *targets;
  gint     n_targets;
  GdkAtom  result = GDK_NONE;

  targets = ligma_clipboard_wait_for_targets (ligma, &n_targets);

  if (targets)
    {
      GdkAtom curve_atom = gdk_atom_intern_static_string ("application/x-ligma-curve");
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
ligma_clipboard_send_image (GtkClipboard     *clipboard,
                           GtkSelectionData *data,
                           guint             info,
                           Ligma             *ligma)
{
  LigmaClipboard *ligma_clip = ligma_clipboard_get (ligma);

  ligma_set_busy (ligma);

  if (info == 0)
    {
      if (ligma->be_verbose)
        g_printerr ("clipboard: sending image data as '%s'\n",
                    ligma_clip->image_target_entries[info].target);

      ligma_selection_data_set_xcf (data, ligma_clip->image);
    }
  else
    {
      GdkPixbuf *pixbuf;

      ligma_pickable_flush (LIGMA_PICKABLE (ligma_clip->image));

      pixbuf = ligma_viewable_get_pixbuf (LIGMA_VIEWABLE (ligma_clip->image),
                                         ligma_get_user_context (ligma),
                                         ligma_image_get_width (ligma_clip->image),
                                         ligma_image_get_height (ligma_clip->image));

      if (pixbuf)
        {
          gdouble res_x;
          gdouble res_y;
          gchar   str[16];

          ligma_image_get_resolution (ligma_clip->image, &res_x, &res_y);

          g_snprintf (str, sizeof (str), "%d", ROUND (res_x));
          gdk_pixbuf_set_option (pixbuf, "x-dpi", str);

          g_snprintf (str, sizeof (str), "%d", ROUND (res_y));
          gdk_pixbuf_set_option (pixbuf, "y-dpi", str);

          if (ligma->be_verbose)
            g_printerr ("clipboard: sending image data as '%s'\n",
                        ligma_clip->image_target_entries[info].target);

          gtk_selection_data_set_pixbuf (data, pixbuf);
        }
      else
        {
          g_warning ("%s: ligma_viewable_get_pixbuf() failed", G_STRFUNC);
        }
    }

  ligma_unset_busy (ligma);
}

static void
ligma_clipboard_send_buffer (GtkClipboard     *clipboard,
                            GtkSelectionData *data,
                            guint             info,
                            Ligma             *ligma)
{
  LigmaClipboard *ligma_clip = ligma_clipboard_get (ligma);
  GdkPixbuf     *pixbuf;

  ligma_set_busy (ligma);

  pixbuf = ligma_viewable_get_pixbuf (LIGMA_VIEWABLE (ligma_clip->buffer),
                                     ligma_get_user_context (ligma),
                                     ligma_buffer_get_width (ligma_clip->buffer),
                                     ligma_buffer_get_height (ligma_clip->buffer));

  if (pixbuf)
    {
      gdouble res_x;
      gdouble res_y;
      gchar   str[16];

      ligma_buffer_get_resolution (ligma_clip->buffer, &res_x, &res_y);

      g_snprintf (str, sizeof (str), "%d", ROUND (res_x));
      gdk_pixbuf_set_option (pixbuf, "x-dpi", str);

      g_snprintf (str, sizeof (str), "%d", ROUND (res_y));
      gdk_pixbuf_set_option (pixbuf, "y-dpi", str);

      if (ligma->be_verbose)
        g_printerr ("clipboard: sending pixbuf data as '%s'\n",
                    ligma_clip->buffer_target_entries[info].target);

      gtk_selection_data_set_pixbuf (data, pixbuf);
    }
  else
    {
      g_warning ("%s: ligma_viewable_get_pixbuf() failed", G_STRFUNC);
    }

  ligma_unset_busy (ligma);
}

static void
ligma_clipboard_send_svg (GtkClipboard     *clipboard,
                         GtkSelectionData *data,
                         guint             info,
                         Ligma             *ligma)
{
  LigmaClipboard *ligma_clip = ligma_clipboard_get (ligma);

  ligma_set_busy (ligma);

  if (ligma_clip->svg)
    {
      if (ligma->be_verbose)
        g_printerr ("clipboard: sending SVG data as '%s'\n",
                    ligma_clip->svg_target_entries[info].target);

      ligma_selection_data_set_stream (data,
                                      (const guchar *) ligma_clip->svg,
                                      strlen (ligma_clip->svg));
    }

  ligma_unset_busy (ligma);
}

static void
ligma_clipboard_send_curve (GtkClipboard     *clipboard,
                           GtkSelectionData *data,
                           guint             info,
                           Ligma             *ligma)
{
  LigmaClipboard *ligma_clip = ligma_clipboard_get (ligma);

  ligma_set_busy (ligma);

  if (ligma_clip->curve)
    {
      if (ligma->be_verbose)
        g_printerr ("clipboard: sending curve data as '%s'\n",
                    ligma_clip->curve_target_entries[info].target);

      ligma_selection_data_set_curve (data, ligma_clip->curve);
    }

  ligma_unset_busy (ligma);
}
