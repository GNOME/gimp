/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball 
 *
 * gimpbutton.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpbutton.h"


enum
{
  EXTENDED_CLICKED,
  LAST_SIGNAL
};


static void       gimp_button_class_init     (GimpButtonClass  *klass);
static void       gimp_button_init           (GimpButton       *button);

static gboolean   gimp_button_button_press   (GtkWidget        *widget,
					      GdkEventButton   *event);
static gboolean   gimp_button_button_release (GtkWidget        *widget,
					      GdkEventButton   *event);


static guint button_signals[LAST_SIGNAL] = { 0 };

static GtkButtonClass *parent_class = NULL;


GtkType
gimp_button_get_type (void)
{
  static guint button_type = 0;

  if (!button_type)
    {
      GtkTypeInfo button_info =
      {
	"GimpButton",
	sizeof (GimpButton),
	sizeof (GimpButtonClass),
	(GtkClassInitFunc) gimp_button_class_init,
	(GtkObjectInitFunc) gimp_button_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      button_type = gtk_type_unique (GTK_TYPE_BUTTON, &button_info);
    }

  return button_type;
}

static void
gimp_button_class_init (GimpButtonClass *klass)
{
  GObjectClass   *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  button_signals[EXTENDED_CLICKED] = 
    g_signal_new ("extended_clicked",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpButtonClass, extended_clicked),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__UINT,
		  G_TYPE_NONE, 1,
		  G_TYPE_UINT);

  widget_class->button_press_event   = gimp_button_button_press;
  widget_class->button_release_event = gimp_button_button_release;
}

static void
gimp_button_init (GimpButton *button)
{
  button->press_state = 0;
}

/**
 * gimp_button_new:
 *
 * Creates a new #GimpButton widget.
 *
 * Returns: A pointer to the new #GimpButton widget.
 **/
GtkWidget *
gimp_button_new (void)
{
  GimpButton *button;

  button = gtk_type_new (GIMP_TYPE_BUTTON);

  return GTK_WIDGET (button);
}

static gboolean
gimp_button_button_press (GtkWidget      *widget,
			  GdkEventButton *bevent)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_BUTTON (widget), FALSE);
  g_return_val_if_fail (bevent != NULL, FALSE);

  if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 1)
    {
      GimpButton *button;

      button = GIMP_BUTTON (widget);

      button->press_state = bevent->state;
    }

  if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
    return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

  return TRUE;
}

static gint
gimp_button_button_release (GtkWidget      *widget,
			    GdkEventButton *bevent)
{
  GtkButton *button;
  gboolean   extended_clicked = FALSE;
  gboolean   retval           = TRUE;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_BUTTON (widget), FALSE);
  g_return_val_if_fail (bevent != NULL, FALSE);

  button = GTK_BUTTON (widget);

  if (bevent->button == 1)
    {
      if (button->in_button &&
	  (GIMP_BUTTON (button)->press_state &
	   (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)))
	{
	  g_signal_emit (G_OBJECT (widget),
                         button_signals[EXTENDED_CLICKED], 0,
                         GIMP_BUTTON (button)->press_state);

	  extended_clicked  = TRUE;

	  /* HACK: don't let GtkButton emit "clicked" by telling it that
	   * the mouse pointer is outside the widget
	   */
	  button->in_button = FALSE;
	}
    }

  if (GTK_WIDGET_CLASS (parent_class)->button_release_event)
    retval = GTK_WIDGET_CLASS (parent_class)->button_release_event (widget,
								    bevent);

  if (extended_clicked)
    {
      /* revert the above HACK and let the button draw itself in the
       * correct state, because upchaining with "in_button" == FALSE
       * messed it up
       */
      button->in_button = TRUE;

      gtk_widget_set_state (widget, GTK_STATE_PRELIGHT);
      gtk_widget_draw (widget, NULL);
   }

  return retval;
}
