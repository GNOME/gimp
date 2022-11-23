/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmachainbutton.c
 * Copyright (C) 1999-2000 Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"

#include "ligmawidgetstypes.h"

#include "ligmachainbutton.h"
#include "ligmaicons.h"


/**
 * SECTION: ligmachainbutton
 * @title: LigmaChainButton
 * @short_description: Widget to visually connect two entry widgets.
 * @see_also: You may want to use the convenience function
 *            ligma_coordinates_new() to set up two LigmaSizeEntries
 *            (see #LigmaSizeEntry) linked with a #LigmaChainButton.
 *
 * This widget provides a button showing either a linked or a broken
 * chain that can be used to link two entries, spinbuttons, colors or
 * other GUI elements and show that they may be locked. Use it for
 * example to connect X and Y ratios to provide the possibility of a
 * constrained aspect ratio.
 *
 * The #LigmaChainButton only gives visual feedback, it does not really
 * connect widgets. You have to take care of locking the values
 * yourself by checking the state of the #LigmaChainButton whenever a
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


struct _LigmaChainButtonPrivate
{
  LigmaChainPosition  position;
  gboolean           active;

  GtkWidget         *button;
  GtkWidget         *line1;
  GtkWidget         *line2;
  GtkWidget         *image;
};

#define GET_PRIVATE(obj) (((LigmaChainButton *) (obj))->priv)


static void      ligma_chain_button_constructed      (GObject         *object);
static void      ligma_chain_button_set_property     (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void      ligma_chain_button_get_property     (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static void      ligma_chain_button_compute_expand   (GtkWidget       *widget,
                                                     gboolean        *hexpand_p,
                                                     gboolean        *vexpand_p);

static void      ligma_chain_button_clicked_callback (GtkWidget       *widget,
                                                     LigmaChainButton *button);
static void      ligma_chain_button_update_image     (LigmaChainButton *button);

static GtkWidget * ligma_chain_line_new            (LigmaChainPosition  position,
                                                   gint               which);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaChainButton, ligma_chain_button, GTK_TYPE_GRID)

#define parent_class ligma_chain_button_parent_class

static guint ligma_chain_button_signals[LAST_SIGNAL] = { 0 };

static const gchar * const ligma_chain_icon_names[] =
{
  LIGMA_ICON_CHAIN_HORIZONTAL,
  LIGMA_ICON_CHAIN_HORIZONTAL_BROKEN,
  LIGMA_ICON_CHAIN_VERTICAL,
  LIGMA_ICON_CHAIN_VERTICAL_BROKEN
};


static void
ligma_chain_button_class_init (LigmaChainButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed    = ligma_chain_button_constructed;
  object_class->set_property   = ligma_chain_button_set_property;
  object_class->get_property   = ligma_chain_button_get_property;

  widget_class->compute_expand = ligma_chain_button_compute_expand;

  ligma_chain_button_signals[TOGGLED] =
    g_signal_new ("toggled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaChainButtonClass, toggled),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  klass->toggled = NULL;

  /**
   * LigmaChainButton:position:
   *
   * The position in which the chain button will be used.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_POSITION,
                                   g_param_spec_enum ("position",
                                                      "Position",
                                                      "The chain's position",
                                                      LIGMA_TYPE_CHAIN_POSITION,
                                                      LIGMA_CHAIN_TOP,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      LIGMA_PARAM_READWRITE));

  /**
   * LigmaChainButton:icon-size:
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
                                                      LIGMA_PARAM_READWRITE));

  /**
   * LigmaChainButton:active:
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
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_chain_button_init (LigmaChainButton *button)
{
  LigmaChainButtonPrivate *private;

  button->priv = ligma_chain_button_get_instance_private (button);

  private           = GET_PRIVATE (button);
  private->position = LIGMA_CHAIN_TOP;
  private->active   = FALSE;
  private->image    = gtk_image_new ();
  private->button   = gtk_button_new ();

  gtk_button_set_relief (GTK_BUTTON (private->button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (private->button), private->image);
  gtk_widget_show (private->image);

  g_signal_connect (private->button, "clicked",
                    G_CALLBACK (ligma_chain_button_clicked_callback),
                    button);
}

static void
ligma_chain_button_constructed (GObject *object)
{
  LigmaChainButton        *button  = LIGMA_CHAIN_BUTTON (object);
  LigmaChainButtonPrivate *private = GET_PRIVATE (button);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  private->line1 = ligma_chain_line_new (private->position, 1);
  private->line2 = ligma_chain_line_new (private->position, -1);

  ligma_chain_button_update_image (button);

  if (private->position & LIGMA_CHAIN_LEFT) /* are we a vertical chainbutton? */
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
ligma_chain_button_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaChainButton        *button  = LIGMA_CHAIN_BUTTON (object);
  LigmaChainButtonPrivate *private = GET_PRIVATE (button);

  switch (property_id)
    {
    case PROP_POSITION:
      private->position = g_value_get_enum (value);
      break;

    case PROP_ICON_SIZE:
      g_object_set_property (G_OBJECT (private->image), "icon-size", value);
      break;

    case PROP_ACTIVE:
      ligma_chain_button_set_active (button, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_chain_button_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaChainButton        *button  = LIGMA_CHAIN_BUTTON (object);
  LigmaChainButtonPrivate *private = GET_PRIVATE (button);

  switch (property_id)
    {
    case PROP_POSITION:
      g_value_set_enum (value, private->position);
      break;

    case PROP_ICON_SIZE:
      g_object_get_property (G_OBJECT (private->image), "icon-size", value);
      break;

    case PROP_ACTIVE:
      g_value_set_boolean (value, ligma_chain_button_get_active (button));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_chain_button_compute_expand (GtkWidget *widget,
                                  gboolean  *hexpand_p,
                                  gboolean  *vexpand_p)
{
  /* don't inherit [hv]expand from the chain lines.  see issue #3876. */
  *hexpand_p = FALSE;
  *vexpand_p = FALSE;
}

/**
 * ligma_chain_button_new:
 * @position: The position you are going to use for the button
 *            with respect to the widgets you want to chain.
 *
 * Creates a new #LigmaChainButton widget.
 *
 * This returns a button showing either a broken or a linked chain and
 * small clamps attached to both sides that visually group the two
 * widgets you want to connect. This widget looks best when attached
 * to a grid taking up two columns (or rows respectively) next to the
 * widgets that it is supposed to connect. It may work for more than
 * two widgets, but the look is optimized for two.
 *
 * Returns: Pointer to the new #LigmaChainButton, which is inactive
 *          by default. Use ligma_chain_button_set_active() to
 *          change its state.
 */
GtkWidget *
ligma_chain_button_new (LigmaChainPosition position)
{
  return g_object_new (LIGMA_TYPE_CHAIN_BUTTON,
                       "position", position,
                       NULL);
}

/**
 * ligma_chain_button_set_icon_size:
 * @button: Pointer to a #LigmaChainButton.
 * @size: The new icon size.
 *
 * Sets the icon size of the #LigmaChainButton.
 *
 * Since: 2.10.10
 */
void
ligma_chain_button_set_icon_size (LigmaChainButton *button,
                                 GtkIconSize      size)
{
  g_return_if_fail (LIGMA_IS_CHAIN_BUTTON (button));

  g_object_set (button,
                "icon-size", size,
                NULL);
}

/**
 * ligma_chain_button_get_icon_size:
 * @button: Pointer to a #LigmaChainButton.
 *
 * Gets the icon size of the #LigmaChainButton.
 *
 * Returns: The icon size.
 *
 * Since: 2.10.10
 */
GtkIconSize
ligma_chain_button_get_icon_size (LigmaChainButton *button)
{
  GtkIconSize size;

  g_return_val_if_fail (LIGMA_IS_CHAIN_BUTTON (button), GTK_ICON_SIZE_BUTTON);

  g_object_get (button,
                "icon-size", &size,
                NULL);

  return size;
}

/**
 * ligma_chain_button_set_active:
 * @button: Pointer to a #LigmaChainButton.
 * @active: The new state.
 *
 * Sets the state of the #LigmaChainButton to be either locked (%TRUE) or
 * unlocked (%FALSE) and changes the showed pixmap to reflect the new state.
 */
void
ligma_chain_button_set_active (LigmaChainButton  *button,
                              gboolean          active)
{
  LigmaChainButtonPrivate *private;

  g_return_if_fail (LIGMA_IS_CHAIN_BUTTON (button));

  private = GET_PRIVATE (button);

  if (private->active != active)
    {
      private->active = active ? TRUE : FALSE;

      ligma_chain_button_update_image (button);

      g_signal_emit (button, ligma_chain_button_signals[TOGGLED], 0);

      g_object_notify (G_OBJECT (button), "active");
    }
}

/**
 * ligma_chain_button_get_active
 * @button: Pointer to a #LigmaChainButton.
 *
 * Checks the state of the #LigmaChainButton.
 *
 * Returns: %TRUE if the #LigmaChainButton is active (locked).
 */
gboolean
ligma_chain_button_get_active (LigmaChainButton *button)
{
  LigmaChainButtonPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CHAIN_BUTTON (button), FALSE);

  private = GET_PRIVATE (button);

  return private->active;
}

/**
 * ligma_chain_button_get_button
 * @button: A #LigmaChainButton.
 *
 * Returns: (transfer none) (type GtkButton): The #LigmaChainButton's button.
 *
 * Since: 3.0
 */
GtkWidget *
ligma_chain_button_get_button (LigmaChainButton *button)
{
  LigmaChainButtonPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CHAIN_BUTTON (button), FALSE);

  private = GET_PRIVATE (button);

  return private->button;
}


/*  private functions  */

static void
ligma_chain_button_clicked_callback (GtkWidget       *widget,
                                    LigmaChainButton *button)
{
  LigmaChainButtonPrivate *private = GET_PRIVATE (button);

  ligma_chain_button_set_active (button, ! private->active);
}

static void
ligma_chain_button_update_image (LigmaChainButton *button)
{
  LigmaChainButtonPrivate *private = GET_PRIVATE (button);
  guint                   i;

  i = ((private->position & LIGMA_CHAIN_LEFT) << 1) + (private->active ? 0 : 1);

  gtk_image_set_from_icon_name (GTK_IMAGE (private->image),
                                ligma_chain_icon_names[i],
                                ligma_chain_button_get_icon_size (button));
}


/* LigmaChainLine is a simple no-window widget for drawing the lines.
 *
 * Originally this used to be a GtkDrawingArea but this turned out to
 * be a bad idea. We don't need an extra window to draw on and we also
 * don't need any input events.
 */

static GType     ligma_chain_line_get_type (void) G_GNUC_CONST;
static gboolean  ligma_chain_line_draw     (GtkWidget *widget,
                                           cairo_t   *cr);

struct _LigmaChainLine
{
  GtkWidget          parent_instance;
  LigmaChainPosition  position;
  gint               which;
};

typedef struct _LigmaChainLine  LigmaChainLine;
typedef GtkWidgetClass         LigmaChainLineClass;

G_DEFINE_TYPE (LigmaChainLine, ligma_chain_line, GTK_TYPE_WIDGET)

static void
ligma_chain_line_class_init (LigmaChainLineClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->draw = ligma_chain_line_draw;
}

static void
ligma_chain_line_init (LigmaChainLine *line)
{
  gtk_widget_set_has_window (GTK_WIDGET (line), FALSE);
}

static GtkWidget *
ligma_chain_line_new (LigmaChainPosition  position,
                     gint               which)
{
  LigmaChainLine *line = g_object_new (ligma_chain_line_get_type (), NULL);

  line->position = position;
  line->which    = which;

  return GTK_WIDGET (line);
}

static gboolean
ligma_chain_line_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  GtkStyleContext   *context = gtk_widget_get_style_context (widget);
  LigmaChainLine     *line    = ((LigmaChainLine *) widget);
  GtkAllocation      allocation;
  GdkPoint           points[3];
  LigmaChainPosition  position;
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
        case LIGMA_CHAIN_TOP:
        case LIGMA_CHAIN_BOTTOM:
          break;

        case LIGMA_CHAIN_LEFT:
          position = LIGMA_CHAIN_RIGHT;
          break;

        case LIGMA_CHAIN_RIGHT:
          position = LIGMA_CHAIN_LEFT;
          break;
        }
    }

  switch (position)
    {
    case LIGMA_CHAIN_LEFT:
      points[0].x += SHORT_LINE;
      points[1].x = points[0].x - SHORT_LINE;
      points[1].y = points[0].y;
      points[2].x = points[1].x;
      points[2].y = (line->which == 1 ? allocation.height - 1 : 0);
      break;

    case LIGMA_CHAIN_RIGHT:
      points[0].x -= SHORT_LINE;
      points[1].x = points[0].x + SHORT_LINE;
      points[1].y = points[0].y;
      points[2].x = points[1].x;
      points[2].y = (line->which == 1 ? allocation.height - 1 : 0);
      break;

    case LIGMA_CHAIN_TOP:
      points[0].y += SHORT_LINE;
      points[1].x = points[0].x;
      points[1].y = points[0].y - SHORT_LINE;
      points[2].x = (line->which == 1 ? allocation.width - 1 : 0);
      points[2].y = points[1].y;
      break;

    case LIGMA_CHAIN_BOTTOM:
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
