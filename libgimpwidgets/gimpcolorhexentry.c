/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorhexentry.c
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmacolor/ligmacolor.h"

#include "ligmawidgetstypes.h"

#include "ligmacellrenderercolor.h"
#include "ligmacolorhexentry.h"
#include "ligmahelpui.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmacolorhexentry
 * @title: LigmaColorHexEntry
 * @short_description: Widget for entering a color's hex triplet.
 *
 * Widget for entering a color's hex triplet.
 **/


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum
{
  COLUMN_NAME,
  COLUMN_COLOR,
  NUM_COLUMNS
};


struct _LigmaColorHexEntryPrivate
{
  LigmaRGB  color;
};

#define GET_PRIVATE(obj) (((LigmaColorHexEntry *) (obj))->priv)


static void      ligma_color_hex_entry_constructed (GObject            *object);

static gboolean  ligma_color_hex_entry_events      (GtkWidget          *widget,
                                                   GdkEvent           *event);

static gboolean  ligma_color_hex_entry_events      (GtkWidget          *widget,
                                                   GdkEvent           *event);

static gboolean  ligma_color_hex_entry_matched     (GtkEntryCompletion *completion,
                                                   GtkTreeModel       *model,
                                                   GtkTreeIter        *iter,
                                                   LigmaColorHexEntry  *entry);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaColorHexEntry, ligma_color_hex_entry,
                            GTK_TYPE_ENTRY)

#define parent_class ligma_color_hex_entry_parent_class

static guint entry_signals[LAST_SIGNAL] = { 0 };


static void
ligma_color_hex_entry_class_init (LigmaColorHexEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  entry_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorHexEntryClass, color_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed = ligma_color_hex_entry_constructed;

  klass->color_changed      = NULL;
}

static void
ligma_color_hex_entry_init (LigmaColorHexEntry *entry)
{
  LigmaColorHexEntryPrivate *private;
  GtkEntryCompletion       *completion;
  GtkCellRenderer          *cell;
  GtkListStore             *store;
  LigmaRGB                  *colors;
  const gchar             **names;
  gint                      num_colors;
  gint                      i;

  entry->priv = ligma_color_hex_entry_get_instance_private (entry);

  private = GET_PRIVATE (entry);

  /* GtkEntry's minimum size is way too large, set a reasonable one
   * for our use case
   */
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 8);

  ligma_help_set_help_data (GTK_WIDGET (entry),
                           _("Hexadecimal color notation as used in HTML and "
                             "CSS.  This entry also accepts CSS color names."),
                           NULL);

  ligma_rgba_set (&private->color, 0.0, 0.0, 0.0, 1.0);

  store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, LIGMA_TYPE_RGB);

  ligma_rgb_list_names (&names, &colors, &num_colors);

  for (i = 0; i < num_colors; i++)
    {
      GtkTreeIter  iter;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLUMN_NAME,  names[i],
                          COLUMN_COLOR, colors + i,
                          -1);
    }

  g_free (colors);
  g_free (names);

  completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                             "model", store,
                             NULL);
  g_object_unref (store);

  cell = ligma_cell_renderer_color_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), cell,
                                  "color", COLUMN_COLOR,
                                  NULL);

  gtk_entry_completion_set_text_column (completion, COLUMN_NAME);

  gtk_entry_set_completion (GTK_ENTRY (entry), completion);
  g_object_unref (completion);

  g_signal_connect (entry, "focus-out-event",
                    G_CALLBACK (ligma_color_hex_entry_events),
                    NULL);
  g_signal_connect (entry, "key-press-event",
                    G_CALLBACK (ligma_color_hex_entry_events),
                    NULL);

  g_signal_connect (completion, "match-selected",
                    G_CALLBACK (ligma_color_hex_entry_matched),
                    entry);
}

static void
ligma_color_hex_entry_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_entry_set_text (GTK_ENTRY (object), "000000");
}

/**
 * ligma_color_hex_entry_new:
 *
 * Returns: a new #LigmaColorHexEntry widget
 *
 * Since: 2.2
 **/
GtkWidget *
ligma_color_hex_entry_new (void)
{
  return g_object_new (LIGMA_TYPE_COLOR_HEX_ENTRY, NULL);
}

/**
 * ligma_color_hex_entry_set_color:
 * @entry: a #LigmaColorHexEntry widget
 * @color: pointer to a #LigmaRGB
 *
 * Sets the color displayed by a #LigmaColorHexEntry. If the new color
 * is different to the previously set color, the "color-changed"
 * signal is emitted.
 *
 * Since: 2.2
 **/
void
ligma_color_hex_entry_set_color (LigmaColorHexEntry *entry,
                                const LigmaRGB     *color)
{
  LigmaColorHexEntryPrivate *private;

  g_return_if_fail (LIGMA_IS_COLOR_HEX_ENTRY (entry));
  g_return_if_fail (color != NULL);

  private = GET_PRIVATE (entry);

  if (ligma_rgb_distance (&private->color, color) > 0.0)
    {
      gchar   buffer[8];
      guchar  r, g, b;

      ligma_rgb_set (&private->color, color->r, color->g, color->b);
      ligma_rgb_clamp (&private->color);

      ligma_rgb_get_uchar (&private->color, &r, &g, &b);
      g_snprintf (buffer, sizeof (buffer), "%.2x%.2x%.2x", r, g, b);

      gtk_entry_set_text (GTK_ENTRY (entry), buffer);

      /* move cursor to the end */
      gtk_editable_set_position (GTK_EDITABLE (entry), -1);

     g_signal_emit (entry, entry_signals[COLOR_CHANGED], 0);
    }
}

/**
 * ligma_color_hex_entry_get_color:
 * @entry: a #LigmaColorHexEntry widget
 * @color: (out caller-allocates): pointer to a #LigmaRGB
 *
 * Retrieves the color value displayed by a #LigmaColorHexEntry.
 *
 * Since: 2.2
 **/
void
ligma_color_hex_entry_get_color (LigmaColorHexEntry *entry,
                                LigmaRGB           *color)
{
  LigmaColorHexEntryPrivate *private;

  g_return_if_fail (LIGMA_IS_COLOR_HEX_ENTRY (entry));
  g_return_if_fail (color != NULL);

  private = GET_PRIVATE (entry);

  *color = private->color;
}

static gboolean
ligma_color_hex_entry_events (GtkWidget *widget,
                             GdkEvent  *event)
{
  LigmaColorHexEntry        *entry   = LIGMA_COLOR_HEX_ENTRY (widget);
  LigmaColorHexEntryPrivate *private = GET_PRIVATE (entry);

  switch (event->type)
    {
    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;

        if (kevent->keyval != GDK_KEY_Return   &&
            kevent->keyval != GDK_KEY_KP_Enter &&
            kevent->keyval != GDK_KEY_ISO_Enter)
          break;
        /*  else fall through  */
      }

    case GDK_FOCUS_CHANGE:
      {
        const gchar *text;
        gchar        buffer[8];
        guchar       r, g, b;

        text = gtk_entry_get_text (GTK_ENTRY (widget));

        ligma_rgb_get_uchar (&private->color, &r, &g, &b);
        g_snprintf (buffer, sizeof (buffer), "%.2x%.2x%.2x", r, g, b);

        if (g_ascii_strcasecmp (buffer, text) != 0)
          {
            LigmaRGB  color;
            gsize    len = strlen (text);

            if (len > 0 &&
                (ligma_rgb_parse_hex (&color, text, len) ||
                 ligma_rgb_parse_name (&color, text, -1)))
              {
                ligma_color_hex_entry_set_color (entry, &color);
              }
            else
              {
                gtk_entry_set_text (GTK_ENTRY (entry), buffer);
              }
          }
      }
      break;

    default:
      /*  do nothing  */
      break;
    }

  return FALSE;
}

static gboolean
ligma_color_hex_entry_matched (GtkEntryCompletion *completion,
                              GtkTreeModel       *model,
                              GtkTreeIter        *iter,
                              LigmaColorHexEntry  *entry)
{
  gchar   *name;
  LigmaRGB  color;

  gtk_tree_model_get (model, iter,
                      COLUMN_NAME, &name,
                      -1);

  if (ligma_rgb_parse_name (&color, name, -1))
    ligma_color_hex_entry_set_color (entry, &color);

  g_free (name);

  return TRUE;
}
