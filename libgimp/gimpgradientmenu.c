/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradientmenu.c
 * Copyright (C) 1998 Andy Thomas                
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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include "gimp.h"
#include "gimpui.h"

#include "libgimp-intl.h"


/* Idea is to have a function to call that returns a widget that 
 * completely controls the selection of a gradient.
 * you get a widget returned that you can use in a table say.
 * In:- Initial gradient name. Null means use current selection.
 *      pointer to func to call when gradient changes
 *      (GimpRunGradientCallback).
 * Returned:- Pointer to a widget that you can use in UI.
 * 
 * Widget simply made up of a preview widget (20x40) containing the gradient
 * which can be clicked on to changed the gradient selection.
 */


#define GSEL_DATA_KEY     "__gsel_data"
#define CELL_SIZE_HEIGHT  18
#define CELL_SIZE_WIDTH   84


struct __gradients_sel 
{
  gchar                   *title;
  GimpRunGradientCallback  callback;
  GtkWidget               *gradient_preview;
  GtkWidget               *button;
  gchar                   *gradient_name;      /* Local copy */
  gint                     sample_size;
  gchar                   *gradient_popup_pnt;
  gpointer                 data;
}; 

typedef struct __gradients_sel GSelect;


static void
gradient_preview_update (GtkWidget     *gradient_preview,
			 gint           width,
			 const gdouble *grad_data)
{
  const gdouble *src;
  gdouble        r, g, b, a;
  gdouble        c0, c1;
  guchar        *p0;
  guchar        *p1;
  guchar        *even;
  guchar        *odd;
  gint           x, y;

  width /= 4;
  width = MIN (width, CELL_SIZE_WIDTH);

  /*  Draw the gradient  */
  src = grad_data;
  p0  = even = g_malloc (width * 3);
  p1  = odd  = g_malloc (width * 3);

  for (x = 0; x < width; x++) 
    {
      r = src[x * 4 + 0];
      g = src[x * 4 + 1];
      b = src[x * 4 + 2];
      a = src[x * 4 + 3];
      
      if ((x / GIMP_CHECK_SIZE_SM) & 1) 
	{
	  c0 = GIMP_CHECK_LIGHT;
	  c1 = GIMP_CHECK_DARK;
	} 
      else 
	{
	  c0 = GIMP_CHECK_DARK;
	  c1 = GIMP_CHECK_LIGHT;
	}
    
      *p0++ = (c0 + (r - c0) * a) * 255.0;
      *p0++ = (c0 + (g - c0) * a) * 255.0;
      *p0++ = (c0 + (b - c0) * a) * 255.0;
      
      *p1++ = (c1 + (r - c1) * a) * 255.0;
      *p1++ = (c1 + (g - c1) * a) * 255.0;
      *p1++ = (c1 + (b - c1) * a) * 255.0;
    }
  
  for (y = 0; y < CELL_SIZE_HEIGHT; y++)
    {
      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	gtk_preview_draw_row (GTK_PREVIEW (gradient_preview), 
			      odd, 0, y, width);
      else
	gtk_preview_draw_row (GTK_PREVIEW (gradient_preview), 
			      even, 0, y, width);
    }

  g_free (even);
  g_free (odd);

  gtk_widget_queue_draw (gradient_preview);
}

static void
gradient_select_invoker (const gchar   *name,
			 gint           width,
			 const gdouble *grad_data,
			 gint           closing,
			 gpointer       data)
{
  GSelect *gsel = (GSelect*) data;

  g_free (gsel->gradient_name);

  gsel->gradient_name = g_strdup (name);

  gradient_preview_update (gsel->gradient_preview, width, grad_data);

  if (gsel->callback)
    gsel->callback (name, width, grad_data, closing, gsel->data);

  if (closing)
    gsel->gradient_popup_pnt = NULL;
}


static void
gradient_select_callback (GtkWidget *widget,
			  gpointer   data)
{
  GSelect *gsel = (GSelect*) data;

  if (gsel->gradient_popup_pnt)
    {
      /*  calling gimp_gradients_set_popup() raises the dialog  */
      gimp_gradients_set_popup (gsel->gradient_popup_pnt, gsel->gradient_name);
    }
  else
    {
      gsel->gradient_popup_pnt = 
	gimp_interactive_selection_gradient (gsel->title ?
					     gsel->title : _("Gradient Selection"),
					     gsel->gradient_name,
					     gsel->sample_size,
					     gradient_select_invoker,
					     gsel);
    }
}

/**
 * gimp_gradient_select_widget:
 * @title: Title of the dialog to use or %NULL to use the default title.
 * @gradient_name: Initial gradient name or %NULL to use current selection. 
 * @callback: a function to call when the selected gradient changes.
 * @data: a pointer to arbitary data to be used in the call to @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of a 
 * gradient.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns:A #GtkWidget that you can use in your UI.
 */
GtkWidget * 
gimp_gradient_select_widget (const gchar             *title,
			     const gchar             *gradient_name, 
			     GimpRunGradientCallback  callback,
			     gpointer                 data)
{
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *gradient;
  gint       width;
  gdouble   *grad_data;
  gchar     *name;
  GSelect   *gsel;
  
  gsel = g_new0 (GSelect, 1);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show(hbox);

  button = gtk_button_new ();

  gradient = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (gradient),
		    CELL_SIZE_WIDTH, CELL_SIZE_HEIGHT); 
  gtk_widget_show (gradient);
  gtk_container_add (GTK_CONTAINER (button), gradient); 

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gradient_select_callback),
                    gsel);
  
  gtk_widget_show (button);

  gsel->button           = button;
  gsel->callback         = callback;
  gsel->data             = data;
  gsel->gradient_preview = gradient;
  gsel->sample_size      = CELL_SIZE_WIDTH;

  /* Do initial gradient setup */
  name = gimp_gradients_get_gradient_data ((gchar *) gradient_name, 
					   gsel->sample_size, &width,
					   &grad_data);
  
  if (name)
    {
      gsel->gradient_name = name;

      gradient_preview_update (gsel->gradient_preview, width, grad_data);

      g_free (grad_data);
    }

  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  g_object_set_data (G_OBJECT (hbox), GSEL_DATA_KEY, gsel);

  return hbox;
}


/**
 * gimp_gradient_select_widget_close_popup:
 * @widget: A gradient select widget.
 *
 * Closes the popup window associated with @widget.
 */
void
gimp_gradient_select_widget_close_popup (GtkWidget *widget)
{
  GSelect  *gsel;
  
  gsel = (GSelect*) g_object_get_data (G_OBJECT (widget), GSEL_DATA_KEY);

  if (gsel && gsel->gradient_popup_pnt)
    {
      gimp_gradients_close_popup (gsel->gradient_popup_pnt);
      gsel->gradient_popup_pnt = NULL;
    }
}

/**
 * gimp_gradient_select_widget_set_popup:
 * @widget: A gradient select widget.
 * @gradient_name: gradient name to set.
 *
 * Sets the current gradient for the gradient
 * select widget.  Calls the callback function if one was
 * supplied in the call to gimp_gradient_select_widget().
 */
void
gimp_gradient_select_widget_set_popup (GtkWidget   *widget,
				       const gchar *gradient_name)
{
  gint      width;
  gdouble  *grad_data;
  gchar    *name;
  GSelect  *gsel;
  
  gsel = (GSelect*) g_object_get_data (G_OBJECT (widget), GSEL_DATA_KEY);

  if (gsel)
    {
      name = gimp_gradients_get_gradient_data ((gchar *) gradient_name, 
					       gsel->sample_size, &width,
					       &grad_data);
  
      if (name)
	{
	  gradient_select_invoker (name, width, grad_data, FALSE, gsel);

	  if (gsel->gradient_popup_pnt)
	    gimp_gradients_set_popup (gsel->gradient_popup_pnt, name);

	  g_free (name);
	  g_free (grad_data);
	}
    }
}
