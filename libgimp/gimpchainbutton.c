/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
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

#include "gimpchainbutton.h"
#include "pixmaps/chain.xpm"

static gchar **gimp_chain_xpm[] =
{
  chain_hor_xpm,
  chain_ver_xpm
};

static gchar **gimp_chain_broken_xpm[] =
{
  chain_broken_hor_xpm,
  chain_broken_ver_xpm 
};

static guint gimp_chain_width[] =
{
  24,
  9
};

static guint gimp_chain_height[] =
{
  9,
  24
};

static void gimp_chain_button_destroy          (GtkObject       *object);
static void gimp_chain_button_realize          (GtkWidget       *widget);

static void gimp_chain_button_clicked_callback (GtkWidget       *widget,
						GimpChainButton *gcb);
static void gimp_chain_button_draw_lines       (GtkWidget       *widget,
						GdkEvent        *event,
						GimpChainButton *gcb);

static GtkTableClass *parent_class = NULL;

static void
gimp_chain_button_destroy (GtkObject *object)
{
  GimpChainButton *gcb;

  g_return_if_fail (gcb = GIMP_CHAIN_BUTTON (object));

  if (gcb->broken)
    gdk_pixmap_unref (gcb->broken);
  if (gcb->broken_mask)
    gdk_bitmap_unref (gcb->broken_mask);

  if (gcb->chain)
    gdk_pixmap_unref (gcb->chain);
  if (gcb->chain_mask)
    gdk_bitmap_unref (gcb->chain_mask);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_chain_button_class_init (GimpChainButtonClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (gtk_table_get_type ());

  object_class->destroy = gimp_chain_button_destroy;

  widget_class->realize = gimp_chain_button_realize;
}

static void
gimp_chain_button_init (GimpChainButton *gcb)
{
  gcb->position    = GIMP_CHAIN_TOP;
  gcb->line1       = gtk_drawing_area_new ();
  gcb->line2       = gtk_drawing_area_new ();
  gcb->pixmap      = NULL;
  gcb->broken      = NULL;
  gcb->broken_mask = NULL;
  gcb->chain       = NULL;
  gcb->chain_mask  = NULL;
  gcb->active      = FALSE;

  gcb->button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (gcb->button), GTK_RELIEF_NONE);

  gcb->pixmap = gtk_type_new (gtk_pixmap_get_type ());
  gtk_pixmap_set_build_insensitive (GTK_PIXMAP (gcb->pixmap), TRUE);

  gtk_signal_connect (GTK_OBJECT(gcb->button), "clicked",
		      GTK_SIGNAL_FUNC (gimp_chain_button_clicked_callback), gcb);
  gtk_signal_connect (GTK_OBJECT (gcb->line1), "expose_event",
		      GTK_SIGNAL_FUNC (gimp_chain_button_draw_lines),
		      gcb);
  gtk_signal_connect (GTK_OBJECT (gcb->line2), "expose_event",
		      GTK_SIGNAL_FUNC (gimp_chain_button_draw_lines),
		      gcb);
}

GtkType
gimp_chain_button_get_type (void)
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

      gcb_type = gtk_type_unique (gtk_table_get_type (), &gcb_info);
    }

  return gcb_type;
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

  gcb = gtk_type_new (gimp_chain_button_get_type ());

  gcb->position = position;

  gcb->pixmap->requisition.width = 
    gimp_chain_width[position % 2] + GTK_MISC (gcb->pixmap)->xpad * 2;
  gcb->pixmap->requisition.height = 
    gimp_chain_height[position % 2] + GTK_MISC (gcb->pixmap)->ypad * 2;

  gtk_container_add (GTK_CONTAINER (gcb->button), gcb->pixmap);
  gtk_widget_show (gcb->pixmap);
  gtk_widget_show (gcb->button);

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

  gtk_widget_show (gcb->line1);
  gtk_widget_show (gcb->line2);

  return GTK_WIDGET (gcb);
}

/** 
 * gimp_chain_button_set_active:
 * @gcb: Pointer to a #GimpChainButton.
 * @is_active: The new state.
 * 
 * Sets the state of the #GimpChainButton to be either locked (TRUE) or 
 * unlocked (FALSE) and changes the showed pixmap to reflect the new state.
 */
void       
gimp_chain_button_set_active (GimpChainButton  *gcb,
			      gboolean          is_active)
{
  g_return_if_fail (gcb != NULL);
  g_return_if_fail (GIMP_IS_CHAIN_BUTTON (gcb));

  if (gcb->active != is_active)
    {
      gcb->active = is_active;
 
     if (!GTK_WIDGET_REALIZED (GTK_WIDGET (gcb)))
	return;

     if (gcb->active)
       gtk_pixmap_set (GTK_PIXMAP(gcb->pixmap), gcb->chain, gcb->chain_mask);
     else
       gtk_pixmap_set (GTK_PIXMAP(gcb->pixmap), gcb->broken, gcb->broken_mask);
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
  g_return_val_if_fail (gcb != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_CHAIN_BUTTON (gcb), FALSE);

  return gcb->active;
}


static void
gimp_chain_button_realize (GtkWidget *widget)
{
  GimpChainButton *gcb;
  GtkStyle *style;

  gcb = GIMP_CHAIN_BUTTON (widget);

  if (GTK_WIDGET_CLASS (parent_class)->realize)
    (* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

  style = gtk_widget_get_style (widget);
  gcb->chain = gdk_pixmap_create_from_xpm_d (widget->window,
					     &gcb->chain_mask,
					     &style->bg[GTK_STATE_NORMAL],
					     gimp_chain_xpm[gcb->position % 2]);
  gcb->broken = gdk_pixmap_create_from_xpm_d (widget->window,
					      &gcb->broken_mask,
					      &style->bg[GTK_STATE_NORMAL],
					      gimp_chain_broken_xpm[gcb->position % 2]);

  if (gcb->active) 
    gtk_pixmap_set (GTK_PIXMAP (gcb->pixmap), gcb->chain, gcb->chain_mask);
  else
    gtk_pixmap_set (GTK_PIXMAP (gcb->pixmap), gcb->broken, gcb->broken_mask);
}

static void
gimp_chain_button_clicked_callback (GtkWidget       *widget,
				    GimpChainButton *gcb)
{
  g_return_if_fail (gcb != NULL);
  g_return_if_fail (GIMP_IS_CHAIN_BUTTON (gcb));

  gcb->active = !(gcb->active);

  if (!GTK_WIDGET_REALIZED (GTK_WIDGET (gcb)))
    return;

  if (gcb->active)
    gtk_pixmap_set (GTK_PIXMAP(gcb->pixmap), gcb->chain, gcb->chain_mask);
  else
    gtk_pixmap_set (GTK_PIXMAP(gcb->pixmap), gcb->broken, gcb->broken_mask);
}

static void
gimp_chain_button_draw_lines (GtkWidget       *widget,
			      GdkEvent        *event,
			      GimpChainButton *gcb)
{
  GdkPoint      points[3];
  GdkPoint      buf;
  GtkShadowType	shadow;
  gint          which_line;

#define SHORT_LINE 4
  /* don't set this too high, there's no check against drawing outside 
     the widgets bounds yet (and probably never will be) */

  g_return_if_fail (gcb != NULL);
  g_return_if_fail (GIMP_IS_CHAIN_BUTTON (gcb));

  if (event); /* avoid unused variable compiler warning */

  gdk_window_clear_area (widget->window,
			 0, 0,
			 widget->allocation.width,
			 widget->allocation.height);
  
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
      return;
    }

  if ( ((shadow == GTK_SHADOW_ETCHED_OUT) && (which_line == -1)) ||
       ((shadow == GTK_SHADOW_ETCHED_IN) && (which_line == 1)) )
    {
      buf = points[0];
      points[0] = points[2];
      points[2] = buf;
    }

  gtk_draw_polygon (widget->style,
		    widget->window,
		    GTK_STATE_NORMAL,
		    shadow,
		    points,
		    3,
		    FALSE);
}
