/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpchainbutton.c
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpchainbutton.h"
#include "gimpicons.h"


/**
 * SECTION: gimpchainbutton
 * @title: GimpChainButton
 * @short_description: Widget to visually connect two entry widgets.
 * @see_also: You may want to use the convenience function
 *            gimp_coordinates_new() to set up two GimpSizeEntries
 *            (see #GimpSizeEntry) linked with a #GimpChainButton.
 *
 * This widget provides a button showing either a linked or a broken
 * chain that can be used to link two entries, spinbuttons, colors or
 * other GUI elements and show that they may be locked. Use it for
 * example to connect X and Y ratios to provide the possibility of a
 * constrained aspect ratio.
 *
 * The #GimpChainButton only gives visual feedback, it does not really
 * connect widgets. You have to take care of locking the values
 * yourself by checking the state of the #GimpChainButton whenever a
 * value changes in one of the connected widgets and adjusting the
 * other value if necessary.
 **/


enum
{
  PROP_0,
  PROP_POSITION,
  PROP_ICON_SIZE,
  PROP_ACTIVE
};

enum
{
  TOGGLED,
  LAST_SIGNAL
};


typedef struct _GimpChainButtonPrivate
{
  GimpChainPosition  position;
  gboolean           active;

  GtkWidget         *button;
  GtkWidget         *line1;
  GtkWidget         *line2;
  GtkWidget         *image;
} GimpChainButtonPrivate;


static void      gimp_chain_button_constructed      (GObject         *object);
static void      gimp_chain_button_set_property     (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void      gimp_chain_button_get_property     (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static void      gimp_chain_button_compute_expand   (GtkWidget       *widget,
                                                     gboolean        *hexpand_p,
                                                     gboolean        *vexpand_p);

static void      gimp_chain_button_clicked_callback (GtkWidget       *widget,
                                                     GimpChainButton *button);
static void      gimp_chain_button_update_image     (GimpChainButton *button);

static GtkWidget * gimp_chain_line_new            (GimpChainPosition  position,
                                                   gint               which);


G_DEFINE_TYPE_WITH_PRIVATE (GimpChainButton, gimp_chain_button, GTK_TYPE_GRID)

#define parent_class gimp_chain_button_parent_class

static guint gimp_chain_button_signals[LAST_SIGNAL] = { 0 };

static const gchar * const gimp_chain_icon_names[] =
{
  GIMP_ICON_CHAIN_HORIZONTAL,
  GIMP_ICON_CHAIN_HORIZONTAL_BROKEN,
  GIMP_ICON_CHAIN_VERTICAL,
  GIMP_ICON_CHAIN_VERTICAL_BROKEN
};


static void
gimp_chain_button_class_init (GimpChainButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed    = gimp_chain_button_constructed;
  object_class->set_property   = gimp_chain_button_set_property;
  object_class->get_property   = gimp_chain_button_get_property;

  widget_class->compute_expand = gimp_chain_button_compute_expand;

  gimp_chain_button_signals[TOGGLED] =
    g_signal_new ("toggled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpChainButtonClass, toggled),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  klass->toggled = NULL;

  /**
   * GimpChainButton:position:
   *
   * The position in which the chain button will be used.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_POSITION,
                                   g_param_spec_enum ("position",
                                                      "Position",
                                                      "The chain's position",
                                                      GIMP_TYPE_CHAIN_POSITION,
                                                      GIMP_CHAIN_TOP,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      GIMP_PARAM_READWRITE));

  /**
   * GimpChainButton:icon-size:
   *
   * The chain button icon size.
   *
   * Since: 2.10.10
   */
  g_object_class_install_property (object_class, PROP_ICON_SIZE,
                                   g_param_spec_enum ("icon-size",
                                                      "Icon Size",
                                                      "The chain's icon size",
                                                      GTK_TYPE_ICON_SIZE,
                                                      GTK_ICON_SIZE_BUTTON,
                                                      G_PARAM_CONSTRUCT |
                                                      GIMP_PARAM_READWRITE));

  /**
   * GimpChainButton:active:
   *
   * The toggled state of the chain button.
   *
   * Since: 2.10.10
   */
  g_object_class_install_property (object_class, PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         "Active",
                                                         "The chain's toggled state",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT |
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_chain_button_init (GimpChainButton *button)
{
  GimpChainButtonPrivate *private;
  private           = gimp_chain_button_get_instance_private (button);

  private->position = GIMP_CHAIN_TOP;
  private->active   = FALSE;
  private->image    = gtk_image_new ();
  private->button   = gtk_button_new ();

  gtk_button_set_relief (GTK_BUTTON (private->button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (private->button), private->image);
  gtk_widget_show (private->image);

  g_signal_connect (private->button, "clicked",
                    G_CALLBACK (gimp_chain_button_clicked_callback),
                    button);
}

static void
gimp_chain_button_constructed (GObject *object)
{
  GimpChainButton        *button  = GIMP_CHAIN_BUTTON (object);
  GimpChainButtonPrivate *private = gimp_chain_button_get_instance_private (button);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  private->line1 = gimp_chain_line_new (private->position, 1);
  private->line2 = gimp_chain_line_new (private->position, -1);

  gimp_chain_button_update_image (button);

  if (private->position & GIMP_CHAIN_LEFT) /* are we a vertical chainbutton? */
    {
      gtk_widget_set_vexpand (private->line1, TRUE);
      gtk_widget_set_vexpand (private->line2, TRUE);

      gtk_grid_attach (GTK_GRID (button), private->line1,  0, 0, 1, 1);
      gtk_grid_attach (GTK_GRID (button), private->button, 0, 1, 1, 1);
      gtk_grid_attach (GTK_GRID (button), private->line2,  0, 2, 1, 1);
    }
  else
    {
      gtk_widget_set_hexpand (private->line1, TRUE);
      gtk_widget_set_hexpand (private->line2, TRUE);

      gtk_grid_attach (GTK_GRID (button), private->line1,  0, 0, 1, 1);
      gtk_grid_attach (GTK_GRID (button), private->button, 1, 0, 1, 1);
      gtk_grid_attach (GTK_GRID (button), private->line2,  2, 0, 1, 1);
    }

  gtk_widget_show (private->button);
  gtk_widget_show (private->line1);
  gtk_widget_show (private->line2);
}

static void
gimp_chain_button_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpChainButton        *button  = GIMP_CHAIN_BUTTON (object);
  GimpChainButtonPrivate *private = gimp_chain_button_get_instance_private (button);

  switch (property_id)
    {
    case PROP_POSITION:
      private->position = g_value_get_enum (value);
      break;

    case PROP_ICON_SIZE:
      g_object_set_property (G_OBJECT (private->image), "icon-size", value);
      break;

    case PROP_ACTIVE:
      gimp_chain_button_set_active (button, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_chain_button_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpChainButton        *button  = GIMP_CHAIN_BUTTON (object);
  GimpChainButtonPrivate *private = gimp_chain_button_get_instance_private (button);

  switch (property_id)
    {
    case PROP_POSITION:
      g_value_set_enum (value, private->position);
      break;

    case PROP_ICON_SIZE:
      g_object_get_property (G_OBJECT (private->image), "icon-size", value);
      break;

    case PROP_ACTIVE:
      g_value_set_boolean (value, gimp_chain_button_get_active (button));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_chain_button_compute_expand (GtkWidget *widget,
                                  gboolean  *hexpand_p,
                                  gboolean  *vexpand_p)
{
  /* don't inherit [hv]expand from the chain lines.  see issue #3876. */
  *hexpand_p = FALSE;
  *vexpand_p = FALSE;
}

/**
 * gimp_chain_button_new:
 * @position: The position you are going to use for the button
 *            with respect to the widgets you want to chain.
 *
 * Creates a new #GimpChainButton widget.
 *
 * This returns a button showing either a broken or a linked chain and
 * small clamps attached to both sides that visually group the two
 * widgets you want to connect. This widget looks best when attached
 * to a grid taking up two columns (or rows respectively) next to the
 * widgets that it is supposed to connect. It may work for more than
 * two widgets, but the look is optimized for two.
 *
 * Returns: Pointer to the new #GimpChainButton, which is inactive
 *          by default. Use gimp_chain_button_set_active() to
 *          change its state.
 */
GtkWidget *
gimp_chain_button_new (GimpChainPosition position)
{
  return g_object_new (GIMP_TYPE_CHAIN_BUTTON,
                       "position", position,
                       NULL);
}

/**
 * gimp_chain_button_set_icon_size:
 * @button: Pointer to a #GimpChainButton.
 * @size: The new icon size.
 *
 * Sets the icon size of the #GimpChainButton.
 *
 * Since: 2.10.10
 */
void
gimp_chain_button_set_icon_size (GimpChainButton *button,
                                 GtkIconSize      size)
{
  g_return_if_fail (GIMP_IS_CHAIN_BUTTON (button));

  g_object_set (button,
                "icon-size", size,
                NULL);
}

/**
 * gimp_chain_button_get_icon_size:
 * @button: Pointer to a #GimpChainButton.
 *
 * Gets the icon size of the #GimpChainButton.
 *
 * Returns: The icon size.
 *
 * Since: 2.10.10
 */
GtkIconSize
gimp_chain_button_get_icon_size (GimpChainButton *button)
{
  GtkIconSize size;

  g_return_val_if_fail (GIMP_IS_CHAIN_BUTTON (button), GTK_ICON_SIZE_BUTTON);

  g_object_get (button,
                "icon-size", &size,
                NULL);

  return size;
}

/**
 * gimp_chain_button_set_active:
 * @button: Pointer to a #GimpChainButton.
 * @active: The new state.
 *
 * Sets the state of the #GimpChainButton to be either locked (%TRUE) or
 * unlocked (%FALSE) and changes the showed pixmap to reflect the new state.
 */
void
gimp_chain_button_set_active (GimpChainButton  *button,
                              gboolean          active)
{
  GimpChainButtonPrivate *private;

  g_return_if_fail (GIMP_IS_CHAIN_BUTTON (button));

  private = gimp_chain_button_get_instance_private (button);

  if (private->active != active)
    {
      private->active = active ? TRUE : FALSE;

      gimp_chain_button_update_image (button);

      g_signal_emit (button, gimp_chain_button_signals[TOGGLED], 0);

      g_object_notify (G_OBJECT (button), "active");
    }
}

/**
 * gimp_chain_button_get_active
 * @button: Pointer to a #GimpChainButton.
 *
 * Checks the state of the #GimpChainButton.
 *
 * Returns: %TRUE if the #GimpChainButton is active (locked).
 */
gboolean
gimp_chain_button_get_active (GimpChainButton *button)
{
  GimpChainButtonPrivate *private;

  g_return_val_if_fail (GIMP_IS_CHAIN_BUTTON (button), FALSE);

  private = gimp_chain_button_get_instance_private (button);

  return private->active;
}

/**
 * gimp_chain_button_get_button
 * @button: A #GimpChainButton.
 *
 * Returns: (transfer none) (type GtkButton): The #GimpChainButton's button.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_chain_button_get_button (GimpChainButton *button)
{
  GimpChainButtonPrivate *private;

  g_return_val_if_fail (GIMP_IS_CHAIN_BUTTON (button), FALSE);

  private = gimp_chain_button_get_instance_private (button);

  return private->button;
}


/*  private functions  */

static void
gimp_chain_button_clicked_callback (GtkWidget       *widget,
                                    GimpChainButton *button)
{
  GimpChainButtonPrivate *private = gimp_chain_button_get_instance_private (button);

  gimp_chain_button_set_active (button, ! private->active);
}

static void
gimp_chain_button_update_image (GimpChainButton *button)
{
  GimpChainButtonPrivate *private = gimp_chain_button_get_instance_private (button);
  guint                   i;

  i = ((private->position & GIMP_CHAIN_LEFT) << 1) + (private->active ? 0 : 1);

  gtk_image_set_from_icon_name (GTK_IMAGE (private->image),
                                gimp_chain_icon_names[i],
                                gimp_chain_button_get_icon_size (button));
}


/* GimpChainLine is a simple no-window widget for drawing the lines.
 *
 * Originally this used to be a GtkDrawingArea but this turned out to
 * be a bad idea. We don't need an extra window to draw on and we also
 * don't need any input events.
 */

static GType     gimp_chain_line_get_type (void) G_GNUC_CONST;
static gboolean  gimp_chain_line_draw     (GtkWidget *widget,
                                           cairo_t   *cr);

struct _GimpChainLine
{
  GtkWidget          parent_instance;
  GimpChainPosition  position;
  gint               which;
};

typedef struct _GimpChainLine  GimpChainLine;
typedef GtkWidgetClass         GimpChainLineClass;

G_DEFINE_TYPE (GimpChainLine, gimp_chain_line, GTK_TYPE_WIDGET)

static void
gimp_chain_line_class_init (GimpChainLineClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->draw = gimp_chain_line_draw;
}

static void
gimp_chain_line_init (GimpChainLine *line)
{
  gtk_widget_set_has_window (GTK_WIDGET (line), FALSE);
}

static GtkWidget *
gimp_chain_line_new (GimpChainPosition  position,
                     gint               which)
{
  GimpChainLine *line = g_object_new (gimp_chain_line_get_type (), NULL);

  line->position = position;
  line->which    = which;

  return GTK_WIDGET (line);
}

static gboolean
gimp_chain_line_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  GtkStyleContext   *context = gtk_widget_get_style_context (widget);
  GimpChainLine     *line    = ((GimpChainLine *) widget);
  GtkAllocation      allocation;
  GdkPoint           points[3];
  GimpChainPosition  position;
  GdkRGBA            color;

  gtk_widget_get_allocation (widget, &allocation);

#define SHORT_LINE 4
  points[0].x = allocation.width  / 2;
  points[0].y = allocation.height / 2;

  position = line->position;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      switch (position)
        {
        case GIMP_CHAIN_TOP:
        case GIMP_CHAIN_BOTTOM:
          break;

        case GIMP_CHAIN_LEFT:
          position = GIMP_CHAIN_RIGHT;
          break;

        case GIMP_CHAIN_RIGHT:
          position = GIMP_CHAIN_LEFT;
          break;
        }
    }

  switch (position)
    {
    case GIMP_CHAIN_LEFT:
      points[0].x += SHORT_LINE;
      points[1].x = points[0].x - SHORT_LINE;
      points[1].y = points[0].y;
      points[2].x = points[1].x;
      points[2].y = (line->which == 1 ? allocation.height - 1 : 0);
      break;

    case GIMP_CHAIN_RIGHT:
      points[0].x -= SHORT_LINE;
      points[1].x = points[0].x + SHORT_LINE;
      points[1].y = points[0].y;
      points[2].x = points[1].x;
      points[2].y = (line->which == 1 ? allocation.height - 1 : 0);
      break;

    case GIMP_CHAIN_TOP:
      points[0].y += SHORT_LINE;
      points[1].x = points[0].x;
      points[1].y = points[0].y - SHORT_LINE;
      points[2].x = (line->which == 1 ? allocation.width - 1 : 0);
      points[2].y = points[1].y;
      break;

    case GIMP_CHAIN_BOTTOM:
      points[0].y -= SHORT_LINE;
      points[1].x = points[0].x;
      points[1].y = points[0].y + SHORT_LINE;
      points[2].x = (line->which == 1 ? allocation.width - 1 : 0);
      points[2].y = points[1].y;
      break;

    default:
      return FALSE;
    }

  cairo_move_to (cr, points[0].x, points[0].y);
  cairo_line_to (cr, points[1].x, points[1].y);
  cairo_line_to (cr, points[2].x, points[2].y);

  cairo_set_line_width (cr, 2.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
  gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget), &color);
  gdk_cairo_set_source_rgba (cr, &color);

  cairo_stroke (cr);

  return TRUE;
}
