/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolorhistory.c
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimp-palettes.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimppalettemru.h"

#include "gimpcolorhistory.h"

#include "gimp-intl.h"

enum
{
  COLOR_SELECTED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_HISTORY_SIZE,
};


#define DEFAULT_HISTORY_SIZE 12
#define COLOR_AREA_SIZE      20

static void   gimp_color_history_constructed   (GObject           *object);
static void   gimp_color_history_finalize      (GObject           *object);
static void   gimp_color_history_set_property  (GObject           *object,
                                                guint              property_id,
                                                const GValue      *value,
                                                GParamSpec        *pspec);
static void   gimp_color_history_get_property  (GObject           *object,
                                                guint              property_id,
                                                GValue            *value,
                                                GParamSpec        *pspec);

static void   gimp_color_history_color_clicked (GtkWidget         *widget,
                                                GimpColorHistory  *history);

static void   gimp_color_history_palette_dirty (GimpPalette       *palette,
                                                GimpColorHistory  *history);

static void   gimp_color_history_color_changed (GtkWidget         *widget,
                                                gpointer           data);


G_DEFINE_TYPE (GimpColorHistory, gimp_color_history, GTK_TYPE_TABLE)

#define parent_class gimp_color_history_parent_class

static guint history_signals[LAST_SIGNAL] = { 0 };

static void
gimp_color_history_class_init (GimpColorHistoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_color_history_constructed;
  object_class->set_property = gimp_color_history_set_property;
  object_class->get_property = gimp_color_history_get_property;
  object_class->finalize     = gimp_color_history_finalize;

  history_signals[COLOR_SELECTED] =
    g_signal_new ("color-selected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorHistoryClass, color_selected),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_HISTORY_SIZE,
                                   g_param_spec_int ("history-size",
                                                     NULL, NULL,
                                                     2, G_MAXINT,
                                                     DEFAULT_HISTORY_SIZE,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  klass->color_selected = NULL;
}

static void
gimp_color_history_init (GimpColorHistory *history)
{
  history->color_areas = NULL;
}

static void
gimp_color_history_constructed (GObject *object)
{
  GimpColorHistory *history = GIMP_COLOR_HISTORY (object);
  GimpPalette      *palette;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  palette = gimp_palettes_get_color_history (history->context->gimp);

  g_signal_connect_object (palette, "dirty",
                           G_CALLBACK (gimp_color_history_palette_dirty),
                           G_OBJECT (history), 0);

  gimp_color_history_palette_dirty (palette, history);
}

static void
gimp_color_history_finalize (GObject *object)
{
  GimpColorHistory *history = GIMP_COLOR_HISTORY (object);

  g_clear_pointer (&history->color_areas, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_history_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpColorHistory *history = GIMP_COLOR_HISTORY (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      history->context = g_value_get_object (value);
      break;
    case PROP_HISTORY_SIZE:
      {
        GtkWidget *button;
        gint       i;

        /* Destroy previous color buttons. */
        gtk_container_foreach (GTK_CONTAINER (history),
                               (GtkCallback) gtk_widget_destroy, NULL);
        history->history_size = g_value_get_int (value);
        gtk_table_resize (GTK_TABLE (history),
                          2, (history->history_size + 1)/ 2);
        gtk_table_set_row_spacings (GTK_TABLE (history), 2);
        gtk_table_set_col_spacings (GTK_TABLE (history), 2);
        history->color_areas = g_realloc_n (history->color_areas,
                                            history->history_size,
                                            sizeof (GtkWidget*));
        for (i = 0; i < history->history_size; i++)
          {
            GimpRGB black = { 0.0, 0.0, 0.0, 1.0 };
            gint    row, column;

            column = i % (history->history_size / 2);
            row    = i / (history->history_size / 2);

            button = gtk_button_new ();
            gtk_widget_set_size_request (button, COLOR_AREA_SIZE, COLOR_AREA_SIZE);
            gtk_table_attach_defaults (GTK_TABLE (history), button,
                                       column, column + 1, row, row + 1);
            gtk_widget_show (button);

            history->color_areas[i] = gimp_color_area_new (&black,
                                                           GIMP_COLOR_AREA_SMALL_CHECKS,
                                                           GDK_BUTTON2_MASK);
            gimp_color_area_set_color_config (GIMP_COLOR_AREA (history->color_areas[i]),
                                              history->context->gimp->config->color_management);
            gtk_container_add (GTK_CONTAINER (button), history->color_areas[i]);
            gtk_widget_show (history->color_areas[i]);

            g_signal_connect (button, "clicked",
                              G_CALLBACK (gimp_color_history_color_clicked),
                              history);

            g_signal_connect (history->color_areas[i], "color-changed",
                              G_CALLBACK (gimp_color_history_color_changed),
                              GINT_TO_POINTER (i));
          }
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_history_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpColorHistory *history = GIMP_COLOR_HISTORY (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, history->context);
      break;
    case PROP_HISTORY_SIZE:
      g_value_set_int (value, history->history_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*  Public Functions  */

GtkWidget *
gimp_color_history_new (GimpContext *context,
                        gint         history_size)
{
  GimpColorHistory *history;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  history = g_object_new (GIMP_TYPE_COLOR_HISTORY,
                          "context",      context,
                          "history-size", history_size,
                          NULL);

  return GTK_WIDGET (history);
}

/*  Color history callback.  */

static void
gimp_color_history_color_clicked (GtkWidget        *widget,
                                  GimpColorHistory *history)
{
  GimpColorArea *color_area;
  GimpRGB        color;

  color_area = GIMP_COLOR_AREA (gtk_bin_get_child (GTK_BIN (widget)));

  gimp_color_area_get_color (color_area, &color);

  g_signal_emit (history, history_signals[COLOR_SELECTED], 0,
                 &color);
}

/* Color history palette callback. */

static void
gimp_color_history_palette_dirty (GimpPalette      *palette,
                                  GimpColorHistory *history)
{
  gint i;

  for (i = 0; i < history->history_size; i++)
    {
      GimpPaletteEntry *entry = gimp_palette_get_entry (palette, i);
      GimpRGB           black = { 0.0, 0.0, 0.0, 1.0 };

      g_signal_handlers_block_by_func (history->color_areas[i],
                                       gimp_color_history_color_changed,
                                       GINT_TO_POINTER (i));

      gimp_color_area_set_color (GIMP_COLOR_AREA (history->color_areas[i]),
                                 entry ? &entry->color : &black);

      g_signal_handlers_unblock_by_func (history->color_areas[i],
                                         gimp_color_history_color_changed,
                                         GINT_TO_POINTER (i));
    }
}

/* Color area callbacks. */

static void
gimp_color_history_color_changed (GtkWidget *widget,
                                  gpointer   data)
{
  GimpColorHistory *history;
  GimpPalette      *palette;
  GimpRGB           color;

  history = GIMP_COLOR_HISTORY (gtk_widget_get_ancestor (widget,
                                                         GIMP_TYPE_COLOR_HISTORY));

  palette = gimp_palettes_get_color_history (history->context->gimp);

  gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &color);

  gimp_palette_set_entry_color (palette, GPOINTER_TO_INT (data), &color);
}
