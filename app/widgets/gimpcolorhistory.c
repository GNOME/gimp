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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimp-palettes.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "app/core/gimpimage-colormap.h"
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
#define CHANNEL_EPSILON      1e-3

/* GObject methods */
static void   gimp_color_history_constructed                    (GObject           *object);
static void   gimp_color_history_finalize                       (GObject           *object);
static void   gimp_color_history_set_property                   (GObject           *object,
                                                                 guint              property_id,
                                                                 const GValue      *value,
                                                                 GParamSpec        *pspec);
static void   gimp_color_history_get_property                   (GObject           *object,
                                                                 guint              property_id,
                                                                 GValue            *value,
                                                                 GParamSpec        *pspec);

/* GtkWidget methods */
static GtkSizeRequestMode gimp_color_history_get_request_mode   (GtkWidget         *widget);
static void   gimp_color_history_get_preferred_width_for_height (GtkWidget         *widget,
                                                                 gint               height,
                                                                 gint              *minimum_width,
                                                                 gint              *natural_width);
static void   gimp_color_history_get_preferred_height_for_width (GtkWidget         *widget,
                                                                 gint               width,
                                                                 gint              *minimum_height,
                                                                 gint              *natural_height);
static void   gimp_color_history_size_allocate                  (GtkWidget         *widget,
                                                                 GdkRectangle      *allocation);

/* Signal handlers */
static void   gimp_color_history_color_clicked                  (GtkWidget         *widget,
                                                                 GimpColorHistory  *history);

static void   gimp_color_history_palette_dirty                  (GimpColorHistory  *history);

static void   gimp_color_history_color_changed                  (GtkWidget         *widget,
                                                                 gpointer           data);

static void   gimp_color_history_image_changed                  (GimpContext       *context,
                                                                 GimpImage         *image,
                                                                 GimpColorHistory  *history);

/* Utils */
static void   gimp_color_history_reorganize                     (GimpColorHistory  *history);


G_DEFINE_TYPE (GimpColorHistory, gimp_color_history, GTK_TYPE_GRID)

#define parent_class gimp_color_history_parent_class

static guint history_signals[LAST_SIGNAL] = { 0 };

static void
gimp_color_history_class_init (GimpColorHistoryClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = gimp_color_history_constructed;
  object_class->set_property = gimp_color_history_set_property;
  object_class->get_property = gimp_color_history_get_property;
  object_class->finalize     = gimp_color_history_finalize;

  widget_class->get_request_mode               = gimp_color_history_get_request_mode;
  widget_class->get_preferred_width_for_height = gimp_color_history_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = gimp_color_history_get_preferred_height_for_width;
  widget_class->size_allocate                  = gimp_color_history_size_allocate;

  history_signals[COLOR_SELECTED] =
    g_signal_new ("color-selected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorHistoryClass, color_selected),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_COLOR);

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

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "GimpColorHistory");

  klass->color_selected = NULL;
}

static void
gimp_color_history_init (GimpColorHistory *history)
{
  history->color_areas = NULL;
  history->buttons     = NULL;
  history->n_rows      = 1;

  gtk_grid_set_row_spacing (GTK_GRID (history), 2);
  gtk_grid_set_column_spacing (GTK_GRID (history), 2);
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
                           G_OBJECT (history), G_CONNECT_SWAPPED);
}

static void
gimp_color_history_finalize (GObject *object)
{
  GimpColorHistory *history = GIMP_COLOR_HISTORY (object);

  if (history->context)
    g_signal_handlers_disconnect_by_func (history->context,
                                          gimp_color_history_image_changed,
                                          history);
  g_clear_object (&history->context);

  if (history->active_image)
    g_signal_handlers_disconnect_by_func (history->active_image,
                                          G_CALLBACK (gimp_color_history_palette_dirty),
                                          history);

  g_clear_pointer (&history->color_areas, g_free);
  g_clear_pointer (&history->buttons,     g_free);

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
      if (history->context)
        g_signal_handlers_disconnect_by_func (history->context,
                                              gimp_color_history_image_changed,
                                              history);

      if (history->active_image)
        {
          g_signal_handlers_disconnect_by_func (history->active_image,
                                                G_CALLBACK (gimp_color_history_palette_dirty),
                                                history);
          history->active_image = NULL;
        }

      history->context = g_value_dup_object (value);

      if (history->context)
        {
          g_signal_connect (history->context, "image-changed",
                            G_CALLBACK (gimp_color_history_image_changed),
                            history);

          history->active_image = gimp_context_get_image (history->context);

          if (history->active_image)
            {
              g_signal_connect_swapped (history->active_image, "notify::base-type",
                                        G_CALLBACK (gimp_color_history_palette_dirty),
                                        history);
              g_signal_connect_swapped (history->active_image, "colormap-changed",
                                        G_CALLBACK (gimp_color_history_palette_dirty),
                                        history);
            }
        }
      break;

    case PROP_HISTORY_SIZE:
      {
        GtkWidget *button;
        GtkWidget *color_area;
        gint       i;

        history->history_size = g_value_get_int (value);

        /* Destroy previous color buttons. */
        gtk_container_foreach (GTK_CONTAINER (history),
                               (GtkCallback) gtk_widget_destroy, NULL);
        history->buttons = g_realloc_n (history->buttons,
                                        history->history_size,
                                        sizeof (GtkWidget*));
        history->color_areas = g_realloc_n (history->color_areas,
                                            history->history_size,
                                            sizeof (GtkWidget*));

        for (i = 0; i < history->history_size; i++)
          {
            GeglColor *black = gegl_color_new ("black");
            gint       row, column;

            column = i % (history->history_size / history->n_rows);
            row    = i / (history->history_size / history->n_rows);

            button = gtk_button_new ();
            gtk_widget_set_size_request (button, COLOR_AREA_SIZE, COLOR_AREA_SIZE);
            gtk_grid_attach (GTK_GRID (history), button, column, row, 1, 1);
            gtk_widget_show (button);

            color_area = gimp_color_area_new (black, GIMP_COLOR_AREA_SMALL_CHECKS,
                                              GDK_BUTTON2_MASK);
            gimp_color_area_set_color_config (GIMP_COLOR_AREA (color_area),
                                              history->context->gimp->config->color_management);
            gtk_container_add (GTK_CONTAINER (button), color_area);
            gtk_widget_show (color_area);

            g_signal_connect (button, "clicked",
                              G_CALLBACK (gimp_color_history_color_clicked),
                              history);

            g_signal_connect (color_area, "color-changed",
                              G_CALLBACK (gimp_color_history_color_changed),
                              GINT_TO_POINTER (i));

            history->buttons[i]     = button;
            history->color_areas[i] = color_area;

            g_object_unref (black);
          }

        gimp_color_history_palette_dirty (history);
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

static GtkSizeRequestMode
gimp_color_history_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
gimp_color_history_get_preferred_width_for_height (GtkWidget *widget,
                                                   gint       height,
                                                   gint      *minimum_width,
                                                   gint      *natural_width)
{
  GimpColorHistory *history = GIMP_COLOR_HISTORY (widget);
  GtkWidget        *button;
  gint              button_width  = COLOR_AREA_SIZE;

  button = gtk_grid_get_child_at (GTK_GRID (widget), 0, 0);
  if (button)
    button_width = MAX (gtk_widget_get_allocated_width (button),
                        COLOR_AREA_SIZE);

  /* This is a bit of a trick. Though the height actually depends on the
   * width, I actually just request for about half the expected width if
   * we were to align all color buttons on one line. This way, it is
   * possible to resize the widget smaller than it currently is, hence
   * allowing reorganization of icons.
   */
  *minimum_width = button_width * (1 + history->history_size / 2);
  *natural_width = *minimum_width;
}

static void
gimp_color_history_get_preferred_height_for_width (GtkWidget *widget,
                                                   gint       width,
                                                   gint      *minimum_height,
                                                   gint      *natural_height)
{
  GimpColorHistory *history = GIMP_COLOR_HISTORY (widget);
  GtkWidget        *button;
  gint              button_width  = COLOR_AREA_SIZE;
  gint              button_height = COLOR_AREA_SIZE;
  gint              height;

  button = gtk_grid_get_child_at (GTK_GRID (widget), 0, 0);
  if (button)
    {
      button_width = MAX (gtk_widget_get_allocated_width (button),
                          COLOR_AREA_SIZE);
      button_height = MAX (gtk_widget_get_allocated_height (button),
                           COLOR_AREA_SIZE);
    }

  if (width > button_width * history->history_size + 2 * (history->history_size - 1))
    height = button_height;
  else
    height = 2 * button_height + 2;

  *minimum_height = height;
  *natural_height = height;
}

static void
gimp_color_history_size_allocate (GtkWidget    *widget,
                                  GdkRectangle *allocation)
{
  GimpColorHistory *history = GIMP_COLOR_HISTORY (widget);
  GtkWidget        *button;
  gint              button_width  = COLOR_AREA_SIZE;
  gint              button_height = COLOR_AREA_SIZE;
  gint              height;
  gint              n_rows;

  button = gtk_grid_get_child_at (GTK_GRID (widget), 0, 0);
  if (button)
    {
      button_width = MAX (gtk_widget_get_allocated_width (button),
                          COLOR_AREA_SIZE);
      button_height = MAX (gtk_widget_get_allocated_height (button),
                           COLOR_AREA_SIZE);
    }

  if (allocation->width > button_width * history->history_size +
                          2 * (history->history_size - 1))
    {
      n_rows = 1;
      height = button_height;
    }
  else
    {
      n_rows = 2;
      height = 2 * button_height + 2;
    }

  if (n_rows != history->n_rows)
    {
      history->n_rows = n_rows;
      gimp_color_history_reorganize (history);
    }

  allocation->height = height;
  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
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
  GeglColor     *color;

  color_area = GIMP_COLOR_AREA (gtk_bin_get_child (GTK_BIN (widget)));

  color = gimp_color_area_get_color (color_area);
  g_signal_emit (history, history_signals[COLOR_SELECTED], 0, color);
  g_object_unref (color);
}

/* Color history palette callback. */

static void
gimp_color_history_palette_dirty (GimpColorHistory *history)
{
  GimpPalette       *palette;
  GimpPalette       *colormap_palette = NULL;
  GimpImageBaseType  base_type        = GIMP_RGB;
  gint               i;

  palette = gimp_palettes_get_color_history (history->context->gimp);
  if (history->active_image)
    {
      base_type = gimp_image_get_base_type (history->active_image);
      if (base_type == GIMP_INDEXED)
        colormap_palette = gimp_image_get_colormap_palette (history->active_image);
    }

  for (i = 0; i < history->history_size; i++)
    {
      GimpPaletteEntry *entry = gimp_palette_get_entry (palette, i);
      GeglColor        *black = gegl_color_new ("black");
      GeglColor        *color = entry ? entry->color : black;
      gboolean          oog   = FALSE;

      g_signal_handlers_block_by_func (history->color_areas[i],
                                       gimp_color_history_color_changed,
                                       GINT_TO_POINTER (i));

      gimp_color_area_set_color (GIMP_COLOR_AREA (history->color_areas[i]), color);

      /* Now that palette colors can be any model and any space, just looking at
       * whether they are out of [0; 1] range is not enough (a same color could
       * be in or out range depending on the color space it is stored as).
       * What we are really looking for is:
       * 1. Whether they are out of the palette (indexed image case);
       * 2. Whether they are out of the active image's space
       *    (independently of their own space).
       */
      if (colormap_palette)
        oog = (! gimp_palette_find_entry (colormap_palette, color, NULL));
      else if (history->active_image)
        oog = gimp_color_is_out_of_gamut (color, gimp_image_get_layer_space (history->active_image));
      else
        oog = gimp_color_is_out_of_self_gamut (color);

      gimp_color_area_set_out_of_gamut (GIMP_COLOR_AREA (history->color_areas[i]), oog);

      g_signal_handlers_unblock_by_func (history->color_areas[i],
                                         gimp_color_history_color_changed,
                                         GINT_TO_POINTER (i));

      g_object_unref (black);
    }
}

/* Color area callbacks. */

static void
gimp_color_history_color_changed (GtkWidget *widget,
                                  gpointer   data)
{
  GimpColorHistory *history;
  GimpPalette      *palette;
  GeglColor        *color;

  history = GIMP_COLOR_HISTORY (gtk_widget_get_ancestor (widget,
                                                         GIMP_TYPE_COLOR_HISTORY));

  palette = gimp_palettes_get_color_history (history->context->gimp);

  color = gimp_color_area_get_color (GIMP_COLOR_AREA (widget));

  gimp_palette_set_entry_color (palette, GPOINTER_TO_INT (data), color, FALSE);
}

static void
gimp_color_history_image_changed (GimpContext      *context,
                                  GimpImage        *image,
                                  GimpColorHistory *history)
{
  /* Update active image. */
  if (history->active_image)
    g_signal_handlers_disconnect_by_func (history->active_image,
                                          G_CALLBACK (gimp_color_history_palette_dirty),
                                          history);
  history->active_image = image;
  if (image)
    {
      g_signal_connect_swapped (image, "notify::base-type",
                                G_CALLBACK (gimp_color_history_palette_dirty),
                                history);
      g_signal_connect_swapped (image, "colormap-changed",
                                G_CALLBACK (gimp_color_history_palette_dirty),
                                history);
    }

  /* Update the palette. */
  gimp_color_history_palette_dirty (history);
}

static void
gimp_color_history_reorganize (GimpColorHistory *history)
{
  GtkWidget   *button;
  gint       i;

  g_return_if_fail (history->buttons[0] && GTK_IS_BUTTON (history->buttons[0]));

  for (i = 0; i < history->history_size; i++)
    {
      gint row, column;

      column = i % (history->history_size / history->n_rows);
      row    = i / (history->history_size / history->n_rows);

      button = history->buttons[i];

      g_object_ref (button);
      gtk_container_remove (GTK_CONTAINER (history), button);
      gtk_grid_attach (GTK_GRID (history), button, column, row, 1, 1);
    }
}
