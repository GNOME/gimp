/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpchainbutton.c
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org> 
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

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpchainbutton.h"
#include "gimpstock.h"


enum
{
  TOGGLED,
  LAST_SIGNAL
};


static void   gimp_chain_button_class_init       (GimpChainButtonClass *klass);
static void   gimp_chain_button_init             (GimpChainButton      *gcb);

static void   gimp_chain_button_clicked_callback (GtkWidget            *widget,
						  GimpChainButton      *gcb);
static gint   gimp_chain_button_draw_lines       (GtkWidget            *widget,
						  GdkEventExpose       *eevent,
						  GimpChainButton      *gcb);


static const gchar *gimp_chain_stock_items[] =
{
  GIMP_STOCK_HCHAIN,
  GIMP_STOCK_HCHAIN_BROKEN,
  GIMP_STOCK_VCHAIN,
  GIMP_STOCK_VCHAIN_BROKEN
};


static guint gimp_chain_button_signals[LAST_SIGNAL] = { 0 };

static GtkTableClass *parent_class = NULL;


GType
gimp_chain_button_get_type (void)
{
  static GType gcb_type = 0;

  if (! gcb_type)
    {
      static const GTypeInfo gcb_info =
      {
        sizeof (GimpChainButtonClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_chain_button_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpChainButton),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_chain_button_init,
      };

      gcb_type = g_type_register_static (GTK_TYPE_TABLE,
                                         "GimpChainButton", 
                                         &gcb_info, 0);
    }

  return gcb_type;
}

static void
gimp_chain_button_class_init (GimpChainButtonClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  gimp_chain_button_signals[TOGGLED] = 
    g_signal_new ("toggled",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpChainButtonClass, toggled),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  klass->toggled = NULL;
}

static void
gimp_chain_button_init (GimpChainButton *gcb)
{
  gcb->position    = GIMP_CHAIN_TOP;
  gcb->active      = FALSE;

  gcb->line1       = gtk_drawing_area_new ();
  gcb->line2       = gtk_drawing_area_new ();
  gcb->image       = gtk_image_new ();

  gcb->button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (gcb->button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (gcb->button), gcb->image);
  gtk_widget_show (gcb->image);

  g_signal_connect (G_OBJECT (gcb->button), "clicked",
                    G_CALLBACK (gimp_chain_button_clicked_callback), 
                    gcb);
  g_signal_connect (G_OBJECT (gcb->line1), "expose_event",
                    G_CALLBACK (gimp_chain_button_draw_lines),
                    gcb);
  g_signal_connect (G_OBJECT (gcb->line2), "expose_event",
                    G_CALLBACK (gimp_chain_button_draw_lines),
                    gcb);
}


/**
 * gimp_chain_button_new:
 * @position: The position you are going to use for the button
 *            with respect to the widgets you want to chain.
 * 
 * Creates a new #GimpChainButton widget.
 *
 * This returns a button showing either a broken or a linked chain and
 * small clamps attached to both sides that visually group the two widgets 
 * you want to connect. This widget looks best when attached
 * to a table taking up two columns (or rows respectively) next
 * to the widgets that it is supposed to connect. It may work
 * for more than two widgets, but the look is optimized for two.
 *
 * Returns: Pointer to the new #GimpChainButton, which is inactive
 *          by default. Use gimp_chain_button_set_active() to 
 *          change its state.
 */
GtkWidget *
gimp_chain_button_new (GimpChainPosition position)
{
  GimpChainButton *gcb;

  gcb = g_object_new (GIMP_TYPE_CHAIN_BUTTON, NULL);

  gcb->position = position;

  gtk_image_set_from_stock
    (GTK_IMAGE (gcb->image),
     gimp_chain_stock_items[((position & GIMP_CHAIN_LEFT) << 1) + ! gcb->active],
     GTK_ICON_SIZE_BUTTON);

  if (position & GIMP_CHAIN_LEFT) /* are we a vertical chainbutton? */
    {
      gtk_table_resize (GTK_TABLE (gcb), 3, 1);
      gtk_table_attach (GTK_TABLE (gcb), gcb->button, 0, 1, 1, 2,
			GTK_SHRINK, GTK_SHRINK, 0, 0);
      gtk_table_attach_defaults (GTK_TABLE (gcb), gcb->line1, 0, 1, 0, 1);
      gtk_table_attach_defaults (GTK_TABLE (gcb), gcb->line2, 0, 1, 2, 3);
    }
  else
    {
      gtk_table_resize (GTK_TABLE (gcb), 1, 3);
      gtk_table_attach (GTK_TABLE (gcb), gcb->button, 1, 2, 0, 1,
			GTK_SHRINK, GTK_SHRINK, 0, 0);
      gtk_table_attach_defaults (GTK_TABLE (gcb), gcb->line1, 0, 1, 0, 1);
      gtk_table_attach_defaults (GTK_TABLE (gcb), gcb->line2, 2, 3, 0, 1);
    }

  gtk_widget_show (gcb->button);
  gtk_widget_show (gcb->line1);
  gtk_widget_show (gcb->line2);

  return GTK_WIDGET (gcb);
}

/** 
 * gimp_chain_button_set_active:
 * @gcb:    Pointer to a #GimpChainButton.
 * @active: The new state.
 * 
 * Sets the state of the #GimpChainButton to be either locked (#TRUE) or 
 * unlocked (#FALSE) and changes the showed pixmap to reflect the new state.
 */
void       
gimp_chain_button_set_active (GimpChainButton  *gcb,
			      gboolean          active)
{
  g_return_if_fail (GIMP_IS_CHAIN_BUTTON (gcb));

  if (gcb->active != active)
    {
      gcb->active = active ? TRUE : FALSE;
 
      gtk_image_set_from_stock
        (GTK_IMAGE (gcb->image),
         gimp_chain_stock_items[((gcb->position & GIMP_CHAIN_LEFT) << 1) + ! gcb->active],
         GTK_ICON_SIZE_BUTTON);
    }
}

/**
 * gimp_chain_button_get_active
 * @gcb: Pointer to a #GimpChainButton.
 * 
 * Checks the state of the #GimpChainButton. 
 *
 * Returns: TRUE if the #GimpChainButton is active (locked).
 */
gboolean   
gimp_chain_button_get_active (GimpChainButton *gcb)
{
  g_return_val_if_fail (GIMP_IS_CHAIN_BUTTON (gcb), FALSE);

  return gcb->active;
}

static void
gimp_chain_button_clicked_callback (GtkWidget       *widget,
				    GimpChainButton *gcb)
{
  g_return_if_fail (GIMP_IS_CHAIN_BUTTON (gcb));

  gimp_chain_button_set_active (gcb, ! gcb->active);

  g_signal_emit (G_OBJECT (gcb), gimp_chain_button_signals[TOGGLED], 0);
}

static gint
gimp_chain_button_draw_lines (GtkWidget       *widget,
			      GdkEventExpose  *eevent,
			      GimpChainButton *gcb)
{
  GdkPoint      points[3];
  GdkPoint      buf;
  GtkShadowType	shadow;
  gint          which_line;

#define SHORT_LINE 4
  /* don't set this too high, there's no check against drawing outside 
     the widgets bounds yet (and probably never will be) */

  g_return_val_if_fail (GIMP_IS_CHAIN_BUTTON (gcb), FALSE);

  points[0].x = widget->allocation.width / 2;
  points[0].y = widget->allocation.height / 2;

  which_line = (widget == gcb->line1) ? 1 : -1;

  switch (gcb->position)
    {
    case GIMP_CHAIN_LEFT:
      points[0].x += SHORT_LINE;
      points[1].x = points[0].x - SHORT_LINE;
      points[1].y = points[0].y;
      points[2].x = points[1].x;
      points[2].y = (which_line == 1) ? widget->allocation.height - 1 : 0;
      shadow = GTK_SHADOW_ETCHED_IN;
      break;
    case GIMP_CHAIN_RIGHT:
      points[0].x -= SHORT_LINE;
      points[1].x = points[0].x + SHORT_LINE;
      points[1].y = points[0].y;
      points[2].x = points[1].x;
      points[2].y = (which_line == 1) ? widget->allocation.height - 1 : 0;
      shadow = GTK_SHADOW_ETCHED_OUT;
      break;
    case GIMP_CHAIN_TOP:
      points[0].y += SHORT_LINE;
      points[1].x = points[0].x;
      points[1].y = points[0].y - SHORT_LINE;
      points[2].x = (which_line == 1) ? widget->allocation.width - 1 : 0;
      points[2].y = points[1].y;
      shadow = GTK_SHADOW_ETCHED_OUT;
      break;
    case GIMP_CHAIN_BOTTOM:
      points[0].y -= SHORT_LINE;
      points[1].x = points[0].x;
      points[1].y = points[0].y + SHORT_LINE;
      points[2].x = (which_line == 1) ? widget->allocation.width - 1 : 0;
      points[2].y = points[1].y;
      shadow = GTK_SHADOW_ETCHED_IN;
      break;
    default:
      return FALSE;
    }

  if ( ((shadow == GTK_SHADOW_ETCHED_OUT) && (which_line == -1)) ||
       ((shadow == GTK_SHADOW_ETCHED_IN) && (which_line == 1)) )
    {
      buf = points[0];
      points[0] = points[2];
      points[2] = buf;
    }

  gtk_paint_polygon (widget->style,
		     widget->window,
		     GTK_STATE_NORMAL,
		     shadow,
		     &eevent->area,
		     widget,
		     "chainbutton",
		     points,
		     3,
		     FALSE);

  return TRUE;
}
