/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpcolorarea.c
 * Copyright (C) 2001 Sven Neumann
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
#include "libgimp/gimplimits.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorarea.h"


#define DRAG_PREVIEW_SIZE   32
#define DRAG_ICON_OFFSET    -8


static const GtkTargetEntry targets[] = { { "application/x-color", 0 } };


struct _GimpColorArea
{
  GtkPreview         preview;

  GimpColorAreaType  type;
  GimpRGB            color;
};

enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

static guint           gimp_color_area_signals[LAST_SIGNAL] = { 0 };
static GtkWidgetClass *parent_class = NULL;


static void  gimp_color_area_class_init (GimpColorAreaClass *klass);
static void  gimp_color_area_init       (GimpColorArea      *gca);
static void  gimp_color_area_destroy    (GtkObject          *object);
static void  gimp_color_area_update     (GimpColorArea      *gca);

static void  gimp_color_area_drag_begin         (GtkWidget        *widget,
						 GdkDragContext   *context);
static void  gimp_color_area_drag_end           (GtkWidget        *widget,
						 GdkDragContext   *context);
static void  gimp_color_area_drag_data_received (GtkWidget        *widget, 
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 GtkSelectionData *selection_data,
						 guint             info,
						 guint             time);
static void  gimp_color_area_drag_data_get      (GtkWidget        *widget, 
						 GdkDragContext   *context,
						 GtkSelectionData *selection_data,
						 guint             info,
						 guint             time);

GtkType
gimp_color_area_get_type (void)
{
  static guint gca_type = 0;

  if (!gca_type)
    {
      GtkTypeInfo gca_info =
      {
	"GimpColorArea",
	sizeof (GimpColorArea),
	sizeof (GimpColorAreaClass),
	(GtkClassInitFunc) gimp_color_area_class_init,
	(GtkObjectInitFunc) gimp_color_area_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gca_type = gtk_type_unique (gtk_preview_get_type (), &gca_info);
    }
  
  return gca_type;
}

static void
gimp_color_area_class_init (GimpColorAreaClass *klass)
{
  GtkObjectClass  *object_class;
  GtkWidgetClass  *widget_class;

  object_class  = (GtkObjectClass*) klass;
  widget_class  = (GtkWidgetClass*) klass;

  parent_class = gtk_type_class (gtk_preview_get_type ());

  gimp_color_area_signals[COLOR_CHANGED] = 
    gtk_signal_new ("color_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpColorAreaClass,
				       color_changed),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE,
		    0);

  gtk_object_class_add_signals (object_class, gimp_color_area_signals, 
				LAST_SIGNAL);

  klass->color_changed = NULL;

  object_class->destroy = gimp_color_area_destroy;

  widget_class->drag_begin         = gimp_color_area_drag_begin;
  widget_class->drag_end           = gimp_color_area_drag_end;
  widget_class->drag_data_received = gimp_color_area_drag_data_received;
  widget_class->drag_data_get      = gimp_color_area_drag_data_get;
}

static void
gimp_color_area_init (GimpColorArea *gca)
{
  gimp_rgba_set (&gca->color, 0.0, 0.0, 0.0, 1.0);
  gca->type = GIMP_COLOR_AREA_FLAT;

  GTK_PREVIEW (gca)->type   = GTK_PREVIEW_COLOR;
  GTK_PREVIEW (gca)->bpp    = 3;
  GTK_PREVIEW (gca)->dither = GDK_RGB_DITHER_NORMAL;
  GTK_PREVIEW (gca)->expand = TRUE;

  gtk_signal_connect_after (GTK_OBJECT (gca), "realize", 
			    GTK_SIGNAL_FUNC (gimp_color_area_update),
			    NULL);
  gtk_signal_connect (GTK_OBJECT (gca), "size_allocate", 
		      GTK_SIGNAL_FUNC (gimp_color_area_update),
		      NULL);
}

static void
gimp_color_area_destroy (GtkObject *object)
{
  GimpColorArea *gca;
   
  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_COLOR_AREA (object));
  
  gca = GIMP_COLOR_AREA (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

/**
 * gimp_color_area_new:
 * @color: A pointer to a #GimpRGB struct.
 * @alpha: If the color_area should show alpha.
 * @drag_mask: The event_mask that should trigger drags.
 * 
 * Creates a new #GimpColorArea widget.
 *
 * This returns a preview area showing the color. It handles color
 * DND. If the color changes, the "color_changed" signal is emitted.
 * 
 * Returns: Pointer to the new #GimpColorArea widget.
 **/
GtkWidget *
gimp_color_area_new (const GimpRGB     *color,
		     GimpColorAreaType  type,
		     GdkModifierType    drag_mask)
{
  GimpColorArea *gca;

  g_return_val_if_fail (color != NULL, NULL); 
 
  gca = gtk_type_new (gimp_color_area_get_type ());

  gca->color = *color;
  gca->type  = type;
 
  gtk_drag_dest_set (GTK_WIDGET (gca),
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_DROP,
		     targets, 1,
		     GDK_ACTION_COPY);
  
  /*  do we need this ??  */
  drag_mask &= (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK);

  if (drag_mask)
    gtk_drag_source_set (GTK_WIDGET (gca),
			 drag_mask,
			 targets, 1,
			 GDK_ACTION_COPY | GDK_ACTION_MOVE);

  return GTK_WIDGET (gca);
}

/**
 * gimp_color_area_set_color:
 * @gca: Pointer to a #GimpColorArea.
 * @color:
 * 
 **/
void       
gimp_color_area_set_color (GimpColorArea *gca,
			   const GimpRGB *color)
{
  g_return_if_fail (gca != NULL);
  g_return_if_fail (GIMP_IS_COLOR_AREA (gca));

  g_return_if_fail (color != NULL);

  if (gimp_rgba_distance (&gca->color, color) > 0.000001)
    {
      gca->color = *color;

      gimp_color_area_update (gca);

      gtk_signal_emit (GTK_OBJECT (gca),
		       gimp_color_area_signals[COLOR_CHANGED]);
    }
}

void
gimp_color_area_get_color (GimpColorArea *gca,
			   GimpRGB       *color)
{
  g_return_if_fail (gca != NULL);
  g_return_if_fail (GIMP_IS_COLOR_AREA (gca));

  *color = gca->color;
}

gboolean    
gimp_color_area_has_alpha (GimpColorArea *gca)
{
  g_return_val_if_fail (gca != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_COLOR_AREA (gca), FALSE);

  return gca->type != GIMP_COLOR_AREA_FLAT;
}

void        
gimp_color_area_set_type (GimpColorArea     *gca,
			  GimpColorAreaType  type)
{
  g_return_if_fail (gca != NULL);
  g_return_if_fail (GIMP_IS_COLOR_AREA (gca));

  gca->type = type;
  gimp_color_area_update (gca);
}

static void
gimp_color_area_update (GimpColorArea *gca)
{
  gint     window_width, window_height;
  guint    width, height;
  guint    x, y;
  guint    check_size = 0;
  guchar   light[3];
  guchar   dark[3];
  guchar   opaque[3];
  guchar  *p;
  guchar  *buf;
  gdouble  frac;

  g_return_if_fail (gca != NULL);
  g_return_if_fail (GIMP_IS_COLOR_AREA (gca));

  if (! GTK_WIDGET_REALIZED (GTK_WIDGET (gca)))
    return;

  gdk_window_get_size (GTK_WIDGET (gca)->window, 
		       &window_width, &window_height);

  if (window_width < 1 || window_height < 1)
    return;

  width  = window_width;
  height = window_height;

  switch (gca->type)
    {
    case GIMP_COLOR_AREA_FLAT:
      check_size = 0;
      break;

    case GIMP_COLOR_AREA_SMALL_CHECKS:
      check_size = GIMP_CHECK_SIZE_SM;
      break;

    case GIMP_COLOR_AREA_LARGE_CHECKS:
      check_size = GIMP_CHECK_SIZE;
      break;
    }
      
  p = buf = g_new (guchar, width * 3);

  opaque[0] = gca->color.r * 255.999;
  opaque[1] = gca->color.g * 255.999;
  opaque[2] = gca->color.b * 255.999;

  if (check_size && gca->color.a < 1.0)
    {
      light[0] = (GIMP_CHECK_LIGHT + 
		  (gca->color.r - GIMP_CHECK_LIGHT) * gca->color.a) * 255.999;
      dark[0]  = (GIMP_CHECK_DARK + 
		  (gca->color.r - GIMP_CHECK_DARK)  * gca->color.a) * 255.999;
      light[1] = (GIMP_CHECK_LIGHT + 
		  (gca->color.g - GIMP_CHECK_LIGHT) * gca->color.a) * 255.999;
      dark[1]  = (GIMP_CHECK_DARK + 
		  (gca->color.g - GIMP_CHECK_DARK)  * gca->color.a) * 255.999;
      light[2] = (GIMP_CHECK_LIGHT + 
		  (gca->color.b - GIMP_CHECK_LIGHT) * gca->color.a) * 255.999;
      dark[2]  = (GIMP_CHECK_DARK + 
		  (gca->color.b - GIMP_CHECK_DARK)  * gca->color.a) * 255.999;

      for (y = 0; y < height; y++)
	{
	  p = buf;

	  for (x = 0; x < width; x++)
	    {
	      if (x * height > y * width)
		{
		  *p++ = opaque[0];
		  *p++ = opaque[1];
		  *p++ = opaque[2];

		  continue;
		}

	      frac = y - (gdouble) (x * height) / (gdouble) width;
	      
	      if (((x / check_size) ^ (y / check_size)) & 1) 
		{
		  if ((gint) frac)
		    {
		      *p++ = light[0];
		      *p++ = light[1];
		      *p++ = light[2];
		    }
		  else
		    {
		      *p++ = (gdouble) light[0]  * frac + 
			     (gdouble) opaque[0] * (1.0 - frac);
		      *p++ = (gdouble) light[1]  * frac + 
			     (gdouble) opaque[1] * (1.0 - frac);
		      *p++ = (gdouble) light[2]  * frac + 
			     (gdouble) opaque[2] * (1.0 - frac);
		    }
		}
	      else
		{
		  if ((gint) frac)
		    {
		      *p++ = dark[0];
		      *p++ = dark[1];
		      *p++ = dark[2];
		    }
		  else
		    {
		      *p++ = (gdouble) dark[0] * frac + 
			     (gdouble) opaque[0] * (1.0 - frac);
		      *p++ = (gdouble) dark[1] * frac + 
			     (gdouble) opaque[1] * (1.0 - frac);
		      *p++ = (gdouble) dark[2] * frac + 
			     (gdouble) opaque[2] * (1.0 - frac);
		    }
		}
	    }

	  gtk_preview_draw_row (GTK_PREVIEW (gca), buf, 
				0, height - y - 1, width);
	} 
    }
  else
    {
      for (x = 0; x < width; x++)
	{
	  *p++ = opaque[0];
	  *p++ = opaque[1];
	  *p++ = opaque[2];
	}

      for (y = 0; y < height; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gca), buf, 0, y, width);
    }

  g_free (buf);

  gtk_widget_queue_draw (GTK_WIDGET (gca));
}

static void
gimp_color_area_drag_begin (GtkWidget      *widget,
			    GdkDragContext *context)
{
  GimpRGB    color;
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *color_area;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_realize (window);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (window), frame);

  gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &color);

  color_area = 
    gimp_color_area_new (&color,
			 GIMP_COLOR_AREA (widget)->type,
			 0);

  gtk_widget_set_usize (color_area, DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), color_area);
  gtk_widget_show (color_area);
  gtk_widget_show (frame);

  gtk_object_set_data_full (GTK_OBJECT (widget),
			    "gimp-color-area-drag-window",
			    window,
			    (GtkDestroyNotify) gtk_widget_destroy);

  gtk_drag_set_icon_widget (context, window, DRAG_ICON_OFFSET, DRAG_ICON_OFFSET);
}

static void
gimp_color_area_drag_end (GtkWidget      *widget,
			  GdkDragContext *context)
{
  gtk_object_set_data (GTK_OBJECT (widget),
		       "gimp-color-area-drag-window", NULL);
}

static void
gimp_color_area_drag_data_received (GtkWidget        *widget, 
				    GdkDragContext   *context,
				    gint              x,
				    gint              y,
				    GtkSelectionData *selection_data,
				    guint             info,
				    guint             time)
{
  GimpColorArea *gca;
  GimpRGB        color;
  guint16       *vals;

  gca = GIMP_COLOR_AREA (widget);

  if (selection_data->length < 0)
    return;

  if ((selection_data->format != 16) || 
      (selection_data->length != 8))
    {
      g_warning ("Received invalid color data");
      return;
    }

  vals = (guint16 *)selection_data->data;

  gimp_rgba_set (&color, 
		 (gdouble) vals[0] / 0xffff,
		 (gdouble) vals[1] / 0xffff,
		 (gdouble) vals[2] / 0xffff,
		 (gdouble) vals[3] / 0xffff);

  gimp_color_area_set_color (gca, &color);
}

static void
gimp_color_area_drag_data_get (GtkWidget        *widget, 
			       GdkDragContext   *context,
			       GtkSelectionData *selection_data,
			       guint             info,
			       guint             time)
{
  GimpColorArea *gca;
  guint16        vals[4];

  gca = GIMP_COLOR_AREA (widget);

  vals[0] = gca->color.r * 0xffff;
  vals[1] = gca->color.g * 0xffff;
  vals[2] = gca->color.b * 0xffff;

  if (gca->type == GIMP_COLOR_AREA_FLAT)
    vals[3] = 0xffff;
  else
    vals[3] = gca->color.a * 0xffff;

  gtk_selection_data_set (selection_data,
			  gdk_atom_intern ("application/x-color", FALSE),
			  16, (guchar *)vals, 8);
}
