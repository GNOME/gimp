/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpcolorbutton.c
 * Copyright (C) 1999-2001 Sven Neumann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorarea.h"
#include "gimpcolorbutton.h"

#include "libgimp/gimppalette.h"

#include "libgimp/libgimp-intl.h"


static void    gimp_color_button_class_init     (GimpColorButtonClass *klass);
static void    gimp_color_button_init           (GimpColorButton      *gcb);
static void    gimp_color_button_destroy        (GtkObject            *object);

static void    gimp_color_button_clicked        (GtkButton    *button);
static void    gimp_color_button_state_changed  (GtkWidget    *widget,
						 GtkStateType  previous_state);

static void    gimp_color_button_dialog_ok      (GtkWidget    *widget,
						 gpointer      data);
static void    gimp_color_button_dialog_cancel  (GtkWidget    *widget,
						 gpointer      data);

static void    gimp_color_button_use_fg         (gpointer      callback_data,
						 guint         callback_action, 
						 GtkWidget    *widget);
static void    gimp_color_button_use_bg         (gpointer      callback_data,
						 guint         callback_action, 
						 GtkWidget    *widget);

static gint    gimp_color_button_menu_popup     (GtkWidget    *widget,
						 GdkEvent     *event,
						 gpointer      data);
static gchar * gimp_color_button_menu_translate (const gchar  *path,
						 gpointer      func_data);

static void    gimp_color_button_color_changed  (GtkObject    *object,
						 gpointer      data);


static GtkItemFactoryEntry menu_items[] =
{
  { N_("/Use Foreground Color"), NULL, gimp_color_button_use_fg, 2, NULL },
  { N_("/Use Background Color"), NULL, gimp_color_button_use_bg, 2, NULL }
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

static guint gimp_color_button_signals[LAST_SIGNAL] = { 0 };

static GtkPreviewClass *parent_class = NULL;


static void
gimp_color_button_destroy (GtkObject *object)
{
  GimpColorButton *gcb;
   
  g_return_if_fail (gcb = GIMP_COLOR_BUTTON (object));
  
  g_free (gcb->title);

  if (gcb->dialog)
    gtk_widget_destroy (gcb->dialog);

  if (gcb->color_area)
    gtk_widget_destroy (gcb->color_area);
  
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


static void
gimp_color_button_class_init (GimpColorButtonClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkButtonClass *button_class;

  object_class = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;
  button_class = (GtkButtonClass *) klass;
 
  parent_class = gtk_type_class (gtk_widget_get_type ());

  gimp_color_button_signals[COLOR_CHANGED] = 
    gtk_signal_new ("color_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpColorButtonClass,
				       color_changed),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE,
		    0);

  gtk_object_class_add_signals (object_class, gimp_color_button_signals, 
				LAST_SIGNAL);

  klass->color_changed        = NULL;

  object_class->destroy       = gimp_color_button_destroy;
  widget_class->state_changed = gimp_color_button_state_changed;
  button_class->clicked       = gimp_color_button_clicked;
}

static void
gimp_color_button_init (GimpColorButton *gcb)
{
  GimpRGB color;

  gcb->title        = NULL;
  gcb->dialog       = NULL;

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);
  gcb->color_area = gimp_color_area_new (&color, FALSE, GDK_BUTTON2_MASK);
  gtk_signal_connect (GTK_OBJECT (gcb->color_area), "color_changed",
		      gimp_color_button_color_changed,
		      gcb);

  gtk_container_add (GTK_CONTAINER (gcb), gcb->color_area);
  gtk_widget_show (gcb->color_area);
  
  /* right-click opens a popup */
  gcb->item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<popup>", NULL);  
  gtk_item_factory_set_translate_func (gcb->item_factory,
				       gimp_color_button_menu_translate,
	  			       NULL, NULL);
  gtk_item_factory_create_items (gcb->item_factory, 
				 nmenu_items, menu_items, gcb);
  gtk_signal_connect (GTK_OBJECT (gcb), "button_press_event",
		      GTK_SIGNAL_FUNC (gimp_color_button_menu_popup),
		      gcb); 
}

GtkType
gimp_color_button_get_type (void)
{
  static guint gcb_type = 0;

  if (!gcb_type)
    {
      GtkTypeInfo gcb_info =
      {
	"GimpColorButton",
	sizeof (GimpColorButton),
	sizeof (GimpColorButtonClass),
	(GtkClassInitFunc) gimp_color_button_class_init,
	(GtkObjectInitFunc) gimp_color_button_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gcb_type = gtk_type_unique (gtk_button_get_type (), &gcb_info);
    }
  
  return gcb_type;
}

/**
 * gimp_color_button_new:
 * @title: String that will be used as title for the color_selector.
 * @width: Width of the colorpreview in pixels.
 * @height: Height of the colorpreview in pixels.
 * @color: A pointer to a #GimpRGB color.
 * @type: 
 * 
 * Creates a new #GimpColorButton widget.
 *
 * This returns a button with a preview showing the color.
 * When the button is clicked a GtkColorSelectionDialog is opened.
 * If the user changes the color the new color is written into the
 * array that was used to pass the initial color and the "color_changed"
 * signal is emitted.
 * 
 * Returns: Pointer to the new #GimpColorButton widget.
 **/
GtkWidget *
gimp_color_button_new (const gchar       *title,
		       gint               width,
		       gint               height,
		       const GimpRGB     *color,
		       GimpColorAreaType  type)
{
  GimpColorButton *gcb;
  
  g_return_val_if_fail (color != NULL, NULL);  

  gcb = gtk_type_new (gimp_color_button_get_type ());

  gcb->title = g_strdup (title);
  
  gtk_widget_set_usize (GTK_WIDGET (gcb->color_area), width, height);

  gimp_color_area_set_color (GIMP_COLOR_AREA (gcb->color_area), color);
  gimp_color_area_set_type (GIMP_COLOR_AREA (gcb->color_area), type);

  return GTK_WIDGET (gcb);
}

/**
 * gimp_color_button_set_color:
 * @gcb: Pointer to a #GimpColorButton.
 * @color: Pointer to the new #GimpRGB color.
 * 
 **/
void       
gimp_color_button_set_color (GimpColorButton *gcb,
			     const GimpRGB   *color)
{
  g_return_if_fail (gcb != NULL);
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (gcb));

  g_return_if_fail (color != NULL);

  gimp_color_area_set_color (GIMP_COLOR_AREA (gcb->color_area), color);
}

/**
 * gimp_color_button_get_color:
 * @gcb: Pointer to a #GimpColorButton.
 * 
 **/
void
gimp_color_button_get_color (GimpColorButton *gcb,
			     GimpRGB         *color)
{
  g_return_if_fail (gcb != NULL);
  g_return_if_fail (color != NULL);  

  g_return_if_fail (color != NULL);

  gimp_color_area_get_color (GIMP_COLOR_AREA (gcb->color_area), color);
}

/**
 * gimp_color_button_has_alpha:
 * @gcb: Pointer to a #GimpColorButton.
 * 
 **/
gboolean
gimp_color_button_has_alpha (GimpColorButton *gcb)
{
  g_return_val_if_fail (gcb != NULL, FALSE);

  return gimp_color_area_has_alpha (GIMP_COLOR_AREA (gcb->color_area));
}

void
gimp_color_button_set_type (GimpColorButton   *gcb,
			    GimpColorAreaType  type)
{
  g_return_if_fail (gcb != NULL);

  gimp_color_area_set_type (GIMP_COLOR_AREA (gcb->color_area), type);
}

static void
gimp_color_button_state_changed (GtkWidget    *widget,
				 GtkStateType  previous_state)
{  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (widget));

  if (!GTK_WIDGET_IS_SENSITIVE (widget) && GIMP_COLOR_BUTTON (widget)->dialog)
    gtk_widget_hide (GIMP_COLOR_BUTTON (widget)->dialog);

  if (GTK_WIDGET_CLASS (parent_class)->state_changed)
    GTK_WIDGET_CLASS (parent_class)->state_changed (widget, previous_state);
}

static gint
gimp_color_button_menu_popup (GtkWidget *widget,
			      GdkEvent  *event,
			      gpointer   data)
{
  GimpColorButton *gcb;
  GdkEventButton *bevent;
  gint x;
  gint y;

  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_COLOR_BUTTON (data), FALSE);

  gcb = GIMP_COLOR_BUTTON (data);
 
  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  bevent = (GdkEventButton *) event;
  
  if (bevent->button != 3)
    return FALSE;
 
  gdk_window_get_origin (GTK_WIDGET (widget)->window, &x, &y);
  gtk_item_factory_popup (gcb->item_factory, 
			  x + bevent->x, y + bevent->y, 
			  bevent->button, bevent->time);

  return (TRUE);  
}

static void
gimp_color_button_clicked (GtkButton *button)
{
  GimpColorButton *gcb;
  GimpRGB          color;
  gdouble          dcolor[4];

  g_return_if_fail (button != NULL);
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));

  gcb = GIMP_COLOR_BUTTON (button);
  
  gimp_color_button_get_color (gcb, &color);

  dcolor[0] = color.r;
  dcolor[1] = color.g;
  dcolor[2] = color.b;
  dcolor[3] = color.a;

  if (!gcb->dialog)
    {
      gcb->dialog = gtk_color_selection_dialog_new (gcb->title);

      gtk_color_selection_set_opacity (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->colorsel), gimp_color_button_has_alpha (gcb));
      gtk_color_selection_set_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->colorsel), dcolor);

      gtk_widget_destroy (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->help_button);
      gtk_container_set_border_width (GTK_CONTAINER (gcb->dialog), 2);

      gtk_signal_connect (GTK_OBJECT (gcb->dialog), "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroyed), 
			  &gcb->dialog);
      gtk_signal_connect (GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->ok_button), 
			  "clicked",
			  GTK_SIGNAL_FUNC (gimp_color_button_dialog_ok), 
			  gcb);
      gtk_signal_connect (GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->cancel_button), 
			  "clicked",
			  GTK_SIGNAL_FUNC (gimp_color_button_dialog_cancel), 
			  gcb);
      gtk_window_set_position (GTK_WINDOW (gcb->dialog), GTK_WIN_POS_MOUSE);  
    }

  gtk_color_selection_set_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->colorsel), dcolor);

  gtk_widget_show (gcb->dialog);
}

static void  
gimp_color_button_dialog_ok (GtkWidget *widget, 
			     gpointer   data)
{
  GimpColorButton *gcb;
  GimpRGB          color;
  gdouble          dcolor[4];

  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (data));

  gcb = GIMP_COLOR_BUTTON (data);

  gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->colorsel), dcolor);

  gimp_rgba_set (&color, dcolor[0], dcolor[1], dcolor[2], dcolor[3]);
  gimp_color_button_set_color (gcb, &color);

  gtk_widget_hide (gcb->dialog);  
}

static void  
gimp_color_button_dialog_cancel (GtkWidget *widget, 
				 gpointer   data)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (data));

  gtk_widget_hide (GIMP_COLOR_BUTTON (data)->dialog);
}


static void  
gimp_color_button_use_fg (gpointer   callback_data, 
			  guint      callback_action, 
			  GtkWidget *widget)
{
  GimpRGB  color;

  g_return_if_fail (callback_data != NULL);
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (callback_data));

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (callback_data), &color);
  gimp_palette_get_foreground_rgb (&color);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (callback_data), &color);
}

static void  
gimp_color_button_use_bg (gpointer   callback_data, 
			  guint      callback_action, 
			  GtkWidget *widget)
{
  GimpRGB  color;

  g_return_if_fail (callback_data != NULL);
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (callback_data));

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (callback_data), &color);
  gimp_palette_get_background_rgb (&color);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (callback_data), &color);
}

static void
gimp_color_button_color_changed (GtkObject *object,
				 gpointer   data)
{
  GimpColorButton *gcb = GIMP_COLOR_BUTTON (data);

  if (gcb->dialog)
    {
      GimpRGB  color;
      gdouble  dcolor[4];

      gimp_color_button_get_color (GIMP_COLOR_BUTTON (data), &color);
      
      dcolor[0] = color.r;
      dcolor[1] = color.g;
      dcolor[2] = color.b;
      dcolor[3] = color.a;
      
      gtk_color_selection_set_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->colorsel), dcolor);
    }

  gtk_signal_emit (GTK_OBJECT (gcb), 
		   gimp_color_button_signals[COLOR_CHANGED]);
}

static gchar *
gimp_color_button_menu_translate (const gchar *path, 
				  gpointer     func_data)
{
  return (gettext (path));
}
