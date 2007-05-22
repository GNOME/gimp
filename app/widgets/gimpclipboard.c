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

#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"

#include "gimpclipboard.h"
#include "gimppixbuf.h"
#include "gimpselectiondata.h"

#include "gimp-intl.h"


#define GIMP_CLIPBOARD_KEY "gimp-clipboard"


typedef struct _GimpClipboard GimpClipboard;

struct _GimpClipboard
{
  GSList         *pixbuf_formats;

  GtkTargetEntry *target_entries;
  gint            n_target_entries;

  GtkTargetEntry *svg_target_entries;
  gint            n_svg_target_entries;

  GimpBuffer     *buffer;
  gchar          *svg;
};


static GimpClipboard * gimp_clipboard_get        (Gimp             *gimp);
static void            gimp_clipboard_clear      (GimpClipboard    *gimp_clip);
static void            gimp_clipboard_free       (GimpClipboard    *gimp_clip);

static GdkAtom * gimp_clipboard_wait_for_targets (Gimp             *gimp,
                                                  gint             *n_targets);
static GdkAtom   gimp_clipboard_wait_for_buffer  (Gimp             *gimp);
static GdkAtom   gimp_clipboard_wait_for_svg     (Gimp             *gimp);

static void      gimp_clipboard_send_buffer      (GtkClipboard     *clipboard,
                                                  GtkSelectionData *selection_data,
                                                  guint             info,
                                                  Gimp             *gimp);
static void      gimp_clipboard_send_svg         (GtkClipboard     *clipboard,
                                                  GtkSelectionData *selection_data,
                                                  guint             info,
                                                  Gimp             *gimp);


/*  public functions  */

void
gimp_clipboard_init (Gimp *gimp)
{
  GimpClipboard *gimp_clip;
  GSList        *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_clip = gimp_clipboard_get (gimp);

  g_return_if_fail (gimp_clip == NULL);

  gimp_clip = g_slice_new0 (GimpClipboard);

  g_object_set_data_full (G_OBJECT (gimp), GIMP_CLIPBOARD_KEY,
                          gimp_clip, (GDestroyNotify) gimp_clipboard_free);

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
            gimp_clip->n_target_entries++;

          g_strfreev (mime_types);
        }
    }

  if (gimp_clip->n_target_entries > 0)
    {
      gint i = 0;

      gimp_clip->target_entries = g_new0 (GtkTargetEntry,
                                          gimp_clip->n_target_entries);

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

                  if (gimp->be_verbose)
                    g_printerr ("clipboard: writable pixbuf format: %s\n",
                                mime_type);

                  gimp_clip->target_entries[i].target = g_strdup (mime_type);
                  gimp_clip->target_entries[i].flags  = 0;
                  gimp_clip->target_entries[i].info   = i;

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
}

void
gimp_clipboard_exit (Gimp *gimp)
{
  GtkClipboard *clipboard;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard && gtk_clipboard_get_owner (clipboard) == G_OBJECT (gimp))
    gtk_clipboard_store (clipboard);

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
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard                                                &&
      gtk_clipboard_get_owner (clipboard)   != G_OBJECT (gimp) &&
      gimp_clipboard_wait_for_buffer (gimp) != GDK_NONE)
    {
      return TRUE;
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
 * Return value: %TRUE if there's SVG data in the clipboard, %FALSE otherwise
 **/
gboolean
gimp_clipboard_has_svg (Gimp *gimp)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard                                              &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (gimp) &&
      gimp_clipboard_wait_for_svg (gimp)  != GDK_NONE)
    {
      return TRUE;
    }

  gimp_clip = gimp_clipboard_get (gimp);

  return (gimp_clip->svg != NULL);
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
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;
  GdkAtom        atom;
  GimpBuffer    *buffer = NULL;

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
          GdkPixbuf *pixbuf = gtk_selection_data_get_pixbuf (data);

          gtk_selection_data_free (data);

          if (pixbuf)
            {
              buffer = gimp_buffer_new_from_pixbuf (pixbuf, _("Clipboard"));
              g_object_unref (pixbuf);
            }
        }

      gimp_unset_busy (gimp);
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
 * Return value: a reference to a #GimpBuffer or %NULL if there's no
 *               image data
 **/
gchar *
gimp_clipboard_get_svg (Gimp  *gimp,
                        gsize *svg_length)
{
  GimpClipboard *gimp_clip;
  GtkClipboard  *clipboard;
  GdkAtom        atom;
  gchar         *svg = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (svg_length != NULL, NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard                                                      &&
      gtk_clipboard_get_owner (clipboard)         != G_OBJECT (gimp) &&
      (atom = gimp_clipboard_wait_for_svg (gimp)) != GDK_NONE)
    {
      GtkSelectionData *data;

      gimp_set_busy (gimp);

      data = gtk_clipboard_wait_for_contents (clipboard, atom);

      if (data)
        {
          const guchar *stream;

          stream = gimp_selection_data_get_stream (data, svg_length);

          if (stream)
            svg = g_memdup (stream, *svg_length);

          gtk_selection_data_free (data);
        }

      gimp_unset_busy (gimp);
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
 * gimp_clipboard_set_buffer:
 * @gimp:   pointer to #Gimp
 * @buffer: a #GimpBuffer, or %NULL.
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
                                    gimp_clip->target_entries,
                                    gimp_clip->n_target_entries,
                                    (GtkClipboardGetFunc) gimp_clipboard_send_buffer,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (gimp));

      /*  mark the first entry (image/png) as suitable for storing  */
      gtk_clipboard_set_can_store (clipboard, gimp_clip->target_entries, 1);
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (gimp))
    {
      gtk_clipboard_clear (clipboard);
    }
}

/**
 * gimp_clipboard_set_svg:
 * @gimp: pointer to #Gimp
 * @svg: a string containing the SVG data, or %NULL
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
      gtk_clipboard_set_can_store (clipboard, gimp_clip->svg_target_entries, 1);
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


/*  private functions  */

static GimpClipboard *
gimp_clipboard_get (Gimp *gimp)
{
  return g_object_get_data (G_OBJECT (gimp), GIMP_CLIPBOARD_KEY);
}

static void
gimp_clipboard_clear (GimpClipboard *gimp_clip)
{
  if (gimp_clip->buffer)
    {
      g_object_unref (gimp_clip->buffer);
      gimp_clip->buffer = NULL;
    }

  if (gimp_clip->svg)
    {
      g_free (gimp_clip->svg);
      gimp_clip->svg = NULL;
    }
}

static void
gimp_clipboard_free (GimpClipboard *gimp_clip)
{
  gint i;

  gimp_clipboard_clear (gimp_clip);

  g_slist_free (gimp_clip->pixbuf_formats);

  for (i = 0; i < gimp_clip->n_target_entries; i++)
    g_free (gimp_clip->target_entries[i].target);

  g_free (gimp_clip->target_entries);

  for (i = 0; i < gimp_clip->n_svg_target_entries; i++)
    g_free (gimp_clip->svg_target_entries[i].target);

  g_free (gimp_clip->svg_target_entries);

  g_slice_free (GimpClipboard, gimp_clip);
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

static void
gimp_clipboard_send_buffer (GtkClipboard     *clipboard,
                            GtkSelectionData *selection_data,
                            guint             info,
                            Gimp             *gimp)
{
  GimpClipboard *gimp_clip = gimp_clipboard_get (gimp);
  GdkPixbuf     *pixbuf;

  gimp_set_busy (gimp);

  pixbuf = gimp_viewable_get_pixbuf (GIMP_VIEWABLE (gimp_clip->buffer),
                                     gimp_get_user_context (gimp),
                                     gimp_buffer_get_width (gimp_clip->buffer),
                                     gimp_buffer_get_height (gimp_clip->buffer));

  if (pixbuf)
    {
      if (gimp->be_verbose)
        g_printerr ("clipboard: sending pixbuf data as '%s'\n",
                    gimp_clip->target_entries[info].target);

      gtk_selection_data_set_pixbuf (selection_data, pixbuf);
    }
  else
    {
      g_warning ("%s: gimp_viewable_get_pixbuf() failed", G_STRFUNC);
    }

  gimp_unset_busy (gimp);
}

static void
gimp_clipboard_send_svg (GtkClipboard     *clipboard,
                         GtkSelectionData *selection_data,
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

      gimp_selection_data_set_stream (selection_data,
                                      (const guchar *) gimp_clip->svg,
                                      strlen (gimp_clip->svg));
    }

  gimp_unset_busy (gimp);
}
