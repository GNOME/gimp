/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gimpchainbutton.h"
#include "chain.xpm"

static char **gimp_chain_xpm[] = {
  chain_hor_xpm,
  chain_ver_xpm
};

static char **gimp_chain_broken_xpm[] = { 
  chain_broken_hor_xpm,
  chain_broken_ver_xpm 
};

void gimp_chain_button_realize (GtkWidget *widget, GimpChainButton *gcb);
void gimp_chain_button_callback (GtkWidget *widget, GimpChainButton *gcb);

static void
gimp_chain_button_class_init (GimpChainButtonClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

  class->gimp_chain_button = NULL;
}

static void
gimp_chain_button_init (GimpChainButton *gcb)
{
  gcb->position    = GIMP_CHAIN_TOP;
  gcb->tooltips    = NULL;
  gcb->tip         = NULL;
  gcb->pixmap      = NULL;
  gcb->broken      = NULL;
  gcb->broken_mask = NULL;
  gcb->chain       = NULL;
  gcb->chain_mask  = NULL;
  gcb->active      = FALSE;

  gtk_signal_connect (GTK_OBJECT(gcb), "clicked",
		      GTK_SIGNAL_FUNC(gimp_chain_button_callback), gcb);
  /* That's all we do here, since setting the pixmaps won't work before 
     the parent window is realized.
     We connect to the realized-signal instead and do the rest there. */
  gtk_signal_connect (GTK_OBJECT(gcb), "realize",
		      GTK_SIGNAL_FUNC(gimp_chain_button_realize), gcb);
  
}

guint
gimp_chain_button_get_type ()
{
  static guint gcb_type = 0;

  if (!gcb_type)
    {
      GtkTypeInfo gcb_info =
      {
	"GimpChainButton",
	sizeof (GimpChainButton),
	sizeof (GimpChainButtonClass),
	(GtkClassInitFunc) gimp_chain_button_class_init,
	(GtkObjectInitFunc) gimp_chain_button_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gcb_type = gtk_type_unique (gtk_button_get_type (), &gcb_info);
    }
  
  return gcb_type;
}


GtkWidget* 
gimp_chain_button_new (GimpChainPosition  position)
{
  GimpChainButton *gcb;
  
  gcb = gtk_type_new (gimp_chain_button_get_type ());
  gcb->position = position;
  gtk_button_set_relief (GTK_BUTTON(gcb), GTK_RELIEF_NONE);

  return (GTK_WIDGET(gcb));
}

void
gimp_chain_button_realize (GtkWidget *widget, GimpChainButton *gcb)
{
  GtkStyle  *style;
  GtkWidget *parent;
  GtkWidget *box;

  parent = GTK_WIDGET(gcb)->parent;
  if (!GTK_WIDGET_REALIZED (parent))
    return;

  style = gtk_widget_get_style (parent);
  gcb->chain = gdk_pixmap_create_from_xpm_d (parent->window,
					     &gcb->chain_mask,
					     &style->bg[GTK_STATE_NORMAL],
					     gimp_chain_xpm[gcb->position % 2]);
  gcb->broken = gdk_pixmap_create_from_xpm_d (parent->window,
					      &gcb->broken_mask,
					      &style->bg[GTK_STATE_NORMAL],
					      gimp_chain_broken_xpm[gcb->position % 2]);

  if (gcb->active) 
    gcb->pixmap = gtk_pixmap_new (gcb->chain, gcb->chain_mask);
  else
    gcb->pixmap = gtk_pixmap_new (gcb->broken, gcb->broken_mask);

  box = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (box), 0);
  gtk_container_add (GTK_CONTAINER (gcb), box);
  gtk_box_pack_start (GTK_BOX (box), gcb->pixmap, TRUE, TRUE, 2);
  gtk_widget_show (gcb->pixmap);
  gtk_widget_show (box);
}

void
gimp_chain_button_callback (GtkWidget *widget, GimpChainButton *gcb)
{
  g_return_if_fail (gcb != NULL);
  g_return_if_fail (GIMP_IS_CHAIN_BUTTON (gcb));

  gcb->active = !(gcb->active);

  if (!GTK_WIDGET_REALIZED (GTK_WIDGET(gcb)))
    return;

  if (gcb->active)
    gtk_pixmap_set (GTK_PIXMAP(gcb->pixmap), gcb->chain, gcb->chain_mask);
  else
    gtk_pixmap_set (GTK_PIXMAP(gcb->pixmap), gcb->broken, gcb->broken_mask);
}

void       
gimp_chain_button_set_active (GimpChainButton  *gcb,
			      gboolean          is_active)
{
  g_return_if_fail (gcb != NULL);
  g_return_if_fail (GIMP_IS_CHAIN_BUTTON (gcb));

  is_active = is_active != 0;

  if (gcb->active != is_active)
    {
      gcb->active = is_active;
 
     if (!GTK_WIDGET_REALIZED (GTK_WIDGET(gcb)))
	return;

     if (gcb->active)
       gtk_pixmap_set (GTK_PIXMAP(gcb->pixmap), gcb->chain, gcb->chain_mask);
     else
       gtk_pixmap_set (GTK_PIXMAP(gcb->pixmap), gcb->broken, gcb->broken_mask);
    }
}

gboolean   
gimp_chain_button_get_active (GimpChainButton *gcb)
{
  g_return_val_if_fail (gcb != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_CHAIN_BUTTON (gcb), FALSE);

  return (gcb->active) ? TRUE : FALSE;
}




