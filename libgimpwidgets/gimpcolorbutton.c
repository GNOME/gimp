/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * Gimp Color Button
 * Copyright (C) 1999 Sven Neumann
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

/* TODO:
 *
 * handle bytes != 3|4 -- would we have to provide a special color select dialog
 *                        for that case? Another possibility could be to hide or
 *                        destroy all color-related widgets in the gtk colorselector.
 */

#include "gimp.h"
#include "gimpcolorbutton.h"
#include "gimpintl.h"
#include "gimplimits.h"


/* DND -- remove as soon as gimpdnd is in libgimp */
#define DRAG_PREVIEW_SIZE   32
#define DRAG_ICON_OFFSET    -8

static void gimp_color_button_drag_begin        (GtkWidget          *widget,
						 GdkDragContext     *context,
						 gpointer            data);
static void gimp_color_button_drag_end          (GtkWidget          *widget,
						 GdkDragContext     *context,
						 gpointer            data);
static void gimp_color_button_drop_handle       (GtkWidget          *widget, 
						 GdkDragContext     *context,
						 gint                x,
						 gint                y,
						 GtkSelectionData   *selection_data,
						 guint               info,
						 guint               time,
						 gpointer            data);
static void gimp_color_button_drag_handle       (GtkWidget        *widget, 
						 GdkDragContext   *context,
						 GtkSelectionData *selection_data,
						 guint             info,
						 guint             time,
						 gpointer          data);
static const GtkTargetEntry targets[] = { { "application/x-color", 0 } };

/* end of DND */

struct _GimpColorButton
{
  GtkButton   button;
  
  gchar          *title;
  guchar         *color;
  gint            bpp;
  gint            width;
  gint            height;
  
  gdouble        *dcolor;
  GtkWidget      *preview;
  GtkWidget      *dialog;
  GtkItemFactory *item_factory;
  
  guchar         *even;
  guchar         *odd;
};


static void  gimp_color_button_destroy         (GtkObject *object);
static void  gimp_color_button_clicked         (GtkButton *button);
static void  gimp_color_button_paint           (GimpColorButton *gcb);
static void  gimp_color_button_state_changed   (GtkWidget *widget, GtkStateType previous_state);

static void  gimp_color_button_dialog_ok       (GtkWidget *widget, gpointer data);
static void  gimp_color_button_dialog_cancel   (GtkWidget *widget, gpointer data);

static void  gimp_color_button_use_fg          (gpointer callback_data, guint callback_action, 
						GtkWidget *widget);
static void  gimp_color_button_use_bg          (gpointer callback_data, guint callback_action, 
						GtkWidget *widget);

static gint   gimp_color_button_menu_popup     (GtkWidget *widget, GdkEvent *event, gpointer data);
static gchar* gimp_color_button_menu_translate (const gchar *path, gpointer func_data);


static GtkItemFactoryEntry menu_items[] = {
  { N_("/Use Foreground Color"), NULL, gimp_color_button_use_fg, 2, NULL },
  { N_("/Use Background Color"), NULL, gimp_color_button_use_bg, 2, NULL }
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


enum {
  COLOR_CHANGED,
  LAST_SIGNAL
};

static gint gimp_color_button_signals[LAST_SIGNAL] = { 0 };


static GtkWidgetClass *parent_class = NULL;

static void
gimp_color_button_destroy (GtkObject *object)
{
  GimpColorButton *gcb;
   
  g_return_if_fail (gcb = GIMP_COLOR_BUTTON (object));
  
  g_free (gcb->title);

  if (gcb->dialog)
    gtk_widget_destroy (gcb->dialog);

  g_free (gcb->dcolor);
  g_free (gcb->even);
  g_free (gcb->odd);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


static void
gimp_color_button_class_init (GimpColorButtonClass *class)
{
  GtkObjectClass *object_class;
  GtkButtonClass *button_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  button_class = (GtkButtonClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_widget_get_type ());

  gimp_color_button_signals[COLOR_CHANGED] = 
    gtk_signal_new ("color_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpColorButtonClass,
				       color_changed),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, gimp_color_button_signals, 
				LAST_SIGNAL);
  class->color_changed = NULL;

  object_class->destroy       = gimp_color_button_destroy;
  button_class->clicked       = gimp_color_button_clicked;
  widget_class->state_changed = gimp_color_button_state_changed;
}


static void
gimp_color_button_init (GimpColorButton *gcb)
{
  gcb->title    = NULL;
  gcb->bpp      = 0;
  gcb->color    = NULL;

  gcb->dcolor   = NULL;
  gcb->preview  = NULL;
  gcb->dialog   = NULL;

  gcb->even     = NULL;
  gcb->odd      = NULL; 
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


GtkWidget* 
gimp_color_button_new (gchar   *title,
		       gint     width,
		       gint     height,
		       guchar  *color,
		       gint     bpp)
{
  GimpColorButton *gcb;
  gint i;
  
  g_return_val_if_fail (width > 0 && height > 0, NULL);  
  g_return_val_if_fail (bpp == 3 || bpp == 4, NULL); 

  gcb = gtk_type_new (gimp_color_button_get_type ());

  gcb->title  = g_strdup (title);
  gcb->width  = width;
  gcb->height = height;
  gcb->color  = color;
  gcb->bpp    = bpp;
  
  gcb->dcolor = g_new (gdouble, 4);
  gcb->even   = g_new (guchar, 3 * width);
  gcb->odd    = g_new (guchar, 3 * width);

  for (i = 0; i < bpp; i++)
    gcb->dcolor[i] = (gdouble)color[i] / 255.0;

  if (bpp == 3)
    gcb->dcolor[3] = 1.0;

  gcb->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_signal_connect (GTK_OBJECT (gcb->preview), "destroy",
		      gtk_widget_destroyed, &gcb->preview);

  gtk_preview_size (GTK_PREVIEW (gcb->preview), width, height);
  gtk_container_add (GTK_CONTAINER (gcb), gcb->preview);
  gtk_widget_show (gcb->preview);
  gimp_color_button_paint (gcb);
 
  /* right-click opens a popup */
  gcb->item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<popup>", NULL);  
  gtk_item_factory_set_translate_func (gcb->item_factory, gimp_color_button_menu_translate,
	  			       NULL, NULL);
  gtk_item_factory_create_items (gcb->item_factory, nmenu_items, menu_items, gcb);
  gtk_signal_connect (GTK_OBJECT (gcb),
		      "button_press_event",
		      GTK_SIGNAL_FUNC (gimp_color_button_menu_popup),
		      gcb);

  /* DND -- to be changed as soon as gimpdnd is in libgimp */
  gtk_drag_dest_set (gcb->preview,
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_DROP,
		     targets, 1,
		     GDK_ACTION_COPY);
  gtk_drag_source_set (gcb->preview,
		       GDK_BUTTON2_MASK,
		       targets, 1,
		       GDK_ACTION_COPY | GDK_ACTION_MOVE);

  gtk_signal_connect (GTK_OBJECT (gcb->preview),
		      "drag_begin",
		      GTK_SIGNAL_FUNC (gimp_color_button_drag_begin),
		      gcb);
  gtk_signal_connect (GTK_OBJECT (gcb->preview),
		      "drag_end",
		      GTK_SIGNAL_FUNC (gimp_color_button_drag_end),
		      gcb);
  gtk_signal_connect (GTK_OBJECT (gcb->preview),
		      "drag_data_get",
		      GTK_SIGNAL_FUNC (gimp_color_button_drag_handle),
		      gcb);
  gtk_signal_connect (GTK_OBJECT (gcb->preview),
		      "drag_data_received",
		      GTK_SIGNAL_FUNC (gimp_color_button_drop_handle),
		      gcb);
  /* end of DND */

  return (GTK_WIDGET (gcb));
}

void       
gimp_color_button_update (GimpColorButton *gcb)
{
  gint i;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (gcb));

  for (i = 0; i < gcb->bpp; i++)
    gcb->dcolor[i] = (gdouble)gcb->color[i] / 255.0;

  gimp_color_button_paint (gcb);

  if (gcb->dialog)
    gtk_color_selection_set_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->colorsel),
				   gcb->dcolor);
}

static void
gimp_color_button_state_changed (GtkWidget    *widget,
				 GtkStateType  previous_state)
{  
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (widget));

  if (!GTK_WIDGET_IS_SENSITIVE (widget) && GIMP_COLOR_BUTTON (widget)->dialog)
    gtk_widget_hide (GIMP_COLOR_BUTTON (widget)->dialog);

  if (GTK_WIDGET_CLASS (parent_class)->state_changed)
    (* GTK_WIDGET_CLASS (parent_class)->state_changed) (widget, previous_state);
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
  GtkColorSelection *colorsel;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (button));
  gcb = GIMP_COLOR_BUTTON (button);

  if (!gcb->dialog)
    {
      gcb->dialog = gtk_color_selection_dialog_new (gcb->title);
      colorsel = GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->colorsel);
      gtk_color_selection_set_opacity (colorsel, (gcb->bpp == 4));
      gtk_color_selection_set_color (colorsel, gcb->dcolor);
      gtk_widget_destroy (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->help_button);
      gtk_signal_connect (GTK_OBJECT (gcb->dialog), "destroy",
			  (GtkSignalFunc) gtk_widget_destroyed, &gcb->dialog);
      gtk_signal_connect (GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->ok_button), 
			  "clicked",
			  (GtkSignalFunc) gimp_color_button_dialog_ok, gcb);
      gtk_signal_connect (GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->cancel_button), 
			  "clicked",
			  (GtkSignalFunc) gimp_color_button_dialog_cancel, gcb);
      gtk_window_position (GTK_WINDOW (gcb->dialog), GTK_WIN_POS_MOUSE);  
    }
  gtk_color_selection_set_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->colorsel), 
				 gcb->dcolor);
  gtk_widget_show (gcb->dialog);
}

static void  
gimp_color_button_paint (GimpColorButton *gcb)
{
  gint x, y, i;
  gdouble c0, c1;
  guchar *p0, *p1;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (gcb));

  p0 = gcb->even;
  p1 = gcb->odd;

  if (gcb->bpp == 3)
    {
      for (x = 0; x < gcb->width; x++)
	{
	  for (i = 0; i < 3; i++)
	    *p0++ = gcb->dcolor[i] * 255.999;
	}
      for (y = 0; y < gcb->height; y++)
	gtk_preview_draw_row (GTK_PREVIEW (gcb->preview), gcb->even, 0, y, gcb->width);
    }
  else  /* gcb->bpp == 4 */
    {
      for (x = 0; x < gcb->width; x++)
	{
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
	  for (i = 0; i < 3; i++)
	    {
	      *p0++ = (c0 + (gcb->dcolor[i] - c0) * gcb->dcolor[3]) * 255.999;
	      *p1++ = (c1 + (gcb->dcolor[i] - c1) * gcb->dcolor[3]) * 255.999;
	    }
	}
      for (y = 0; y < gcb->height; y++)
	{
	  if ((y / GIMP_CHECK_SIZE_SM) & 1)
	    gtk_preview_draw_row (GTK_PREVIEW (gcb->preview), gcb->odd, 0, y, gcb->width);
	  else
	    gtk_preview_draw_row (GTK_PREVIEW (gcb->preview), gcb->even, 0, y, gcb->width);
	} 
    }

  gtk_widget_queue_draw (gcb->preview);
}

static void  
gimp_color_button_dialog_ok (GtkWidget *widget, 
			     gpointer   data)
{
  GimpColorButton *gcb;
  guchar new_color[4];
  gboolean color_changed = FALSE;
  gint i;
  
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (data));
  gcb = GIMP_COLOR_BUTTON (data);

  gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (gcb->dialog)->colorsel), gcb->dcolor);

  for (i = 0; i < gcb->bpp; i++)
    {
      new_color[i] = gcb->dcolor[i] * 255.999;
      if (new_color[i] != gcb->color[i])
	color_changed = TRUE;
      gcb->color[i] = new_color[i];
    }
  
  gtk_widget_hide (gcb->dialog);  

  if (color_changed)
    {
      gimp_color_button_paint (gcb);
      gtk_signal_emit (GTK_OBJECT (gcb),
		       gimp_color_button_signals[COLOR_CHANGED]);
    }
}

static void  
gimp_color_button_dialog_cancel (GtkWidget *widget, 
				 gpointer   data)
{
  GimpColorButton *gcb;
  
  g_return_if_fail (GIMP_IS_COLOR_BUTTON (data));
  gcb = GIMP_COLOR_BUTTON (data);

  gtk_widget_hide (gcb->dialog);
}


static void  
gimp_color_button_use_fg (gpointer   callback_data, 
			  guint      callback_action, 
			  GtkWidget *widget)
{
  GimpColorButton *gcb;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (callback_data));
  gcb = GIMP_COLOR_BUTTON (callback_data);

  gimp_palette_get_foreground (gcb->color, gcb->color+1, gcb->color+2);

  gimp_color_button_update (gcb);
}


static void  
gimp_color_button_use_bg (gpointer   callback_data, 
			  guint      callback_action, 
			  GtkWidget *widget)
{
  GimpColorButton *gcb;

  g_return_if_fail (GIMP_IS_COLOR_BUTTON (callback_data));
  gcb = GIMP_COLOR_BUTTON (callback_data);

  gimp_palette_get_background (gcb->color, gcb->color+1, gcb->color+2);

  gimp_color_button_update (gcb);
}

static gchar* 
gimp_color_button_menu_translate (const gchar *path, 
				  gpointer     func_data)
{
  return (gettext (path));
}

/* DND -- remove as soon as gimpdnd is in libgimp */

static void
gimp_color_button_drag_begin (GtkWidget      *widget,
			      GdkDragContext *context,
			      gpointer        data)
{
  GimpColorButton *gcb = data;
  GtkWidget *window;
  GdkColor bg;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
  gtk_widget_set_usize (window, DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);
  gtk_widget_realize (window);
  gtk_object_set_data_full (GTK_OBJECT (widget),
			    "gimp-color-button-drag-window",
			    window,
			    (GtkDestroyNotify) gtk_widget_destroy);

  bg.red   = 0xffff * gcb->dcolor[0];
  bg.green = 0xffff * gcb->dcolor[1];
  bg.blue  = 0xffff * gcb->dcolor[2];

  gdk_color_alloc (gtk_widget_get_colormap (window), &bg);
  gdk_window_set_background (window->window, &bg);

  gtk_drag_set_icon_widget (context, window, DRAG_ICON_OFFSET, DRAG_ICON_OFFSET);
}

static void
gimp_color_button_drag_end (GtkWidget      *widget,
			    GdkDragContext *context,
			    gpointer        data)
{
  gtk_object_set_data (GTK_OBJECT (widget), "gimp-color-button-drag-window", NULL);
}

static void
gimp_color_button_drop_handle (GtkWidget        *widget, 
			       GdkDragContext   *context,
			       gint              x,
			       gint              y,
			       GtkSelectionData *selection_data,
			       guint             info,
			       guint             time,
			       gpointer          data)
{
  GimpColorButton *gcb = data;
  guint16 *vals;
  guchar new_color[4];
  gboolean color_changed = FALSE;
  gint i;

  if (selection_data->length < 0)
    return;

  if ((selection_data->format != 16) || 
      (selection_data->length != 8))
    {
      g_warning ("Received invalid color data\n");
      return;
    }
  
  vals = (guint16 *)selection_data->data;

  for (i = 0; i < gcb->bpp; i++)
    {
      gcb->dcolor[i] = (gdouble)vals[i] / 0xffff;
      new_color[i] = gcb->dcolor[i] * 255.999;
      if (new_color[i] != gcb->color[i])
	color_changed = TRUE;
      gcb->color[i] = new_color[i];
    }
  
  if (color_changed)
    {
      gimp_color_button_paint (gcb);
      gtk_signal_emit (GTK_OBJECT (gcb),
		       gimp_color_button_signals[COLOR_CHANGED]);
    }
}

static void
gimp_color_button_drag_handle (GtkWidget        *widget, 
			       GdkDragContext   *context,
			       GtkSelectionData *selection_data,
			       guint             info,
			       guint             time,
			       gpointer          data)
{
  GimpColorButton *gcb = data;
  guint16 vals[4];
  gint i;

  for (i = 0; i < gcb->bpp; i++)
    vals[i] = gcb->dcolor[i] * 0xffff;
    
  if (i == 3) 
    vals[3] = 0xffff;

  gtk_selection_data_set (selection_data,
			  gdk_atom_intern ("application/x-color", FALSE),
			  16, (guchar *)vals, 8);
}

/* end of DND */
