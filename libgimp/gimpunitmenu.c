/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpunitmenu.c
 * Copyright (C) 1999 Michael Natterer <mitschel@cs.tu-berlin.de>
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

#include "gimpunitmenu.h"

#include "libgimp/gimpintl.h"


static const gchar* gimp_unit_menu_build_string (gchar *format, GUnit unit);
static void gimp_unit_menu_callback (GtkWidget *widget, gpointer data);

enum {
  GUM_UNIT_CHANGED_SIGNAL,
  LAST_SIGNAL
};

static gint gimp_unit_menu_signals[LAST_SIGNAL] = { 0 };


static void
gimp_unit_menu_class_init (GimpUnitMenuClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

  gimp_unit_menu_signals[GUM_UNIT_CHANGED_SIGNAL] = 
    gtk_signal_new ("unit_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpUnitMenuClass, gimp_unit_menu),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, gimp_unit_menu_signals, 
				LAST_SIGNAL);
  class->gimp_unit_menu = NULL;
}


static void
gimp_unit_menu_init (GimpUnitMenu *gum)
{
  gum->unit = UNIT_PIXEL;
  gum->start = 0;
}


guint
gimp_unit_menu_get_type ()
{
  static guint gum_type = 0;

  if (!gum_type)
    {
      GtkTypeInfo gum_info =
      {
	"GimpUnitMenu",
	sizeof (GimpUnitMenu),
	sizeof (GimpUnitMenuClass),
	(GtkClassInitFunc) gimp_unit_menu_class_init,
	(GtkObjectInitFunc) gimp_unit_menu_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gum_type = gtk_type_unique (gtk_option_menu_get_type (), &gum_info);
    }
  
  return gum_type;
}


GtkWidget*
gimp_unit_menu_new (gchar    *format,
		    GUnit     unit,
		    gboolean  with_pixels,
		    gboolean  with_custom)
{
  GimpUnitMenu  *gum;
  GtkWidget     *menu;
  GtkWidget     *menuitem;
  GUnit          u;

  if (with_custom); /* avoid 'unused variable' compiler warning */

  gum = gtk_type_new (gimp_unit_menu_get_type ());

  /* if we don't want pixels, start with inches */
  gum->start = with_pixels ? UNIT_PIXEL : UNIT_INCH;

  if ( (unit < gum->start) || (unit >= UNIT_END) )
    unit = gum->start;

  menu = gtk_menu_new();
  for (u = gum->start; u < gimp_unit_get_number_of_built_in_units(); u++)
    {
      menuitem =
	gtk_menu_item_new_with_label (gimp_unit_menu_build_string(format, u) );
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  (GtkSignalFunc) gimp_unit_menu_callback, gum);
      gtk_widget_show(menuitem);
      gtk_object_set_data (GTK_OBJECT (menuitem), "gimp_unit_menu", (gpointer)u);

      /* add a separator after pixels */
      if (u == UNIT_PIXEL)
	{
	  menuitem = gtk_menu_item_new ();
	  gtk_widget_show (menuitem);
	  gtk_menu_append (GTK_MENU (menu), menuitem);
	}
    } 

  gtk_option_menu_set_menu (GTK_OPTION_MENU (gum), menu);

  gum->unit = unit;
  gtk_option_menu_set_history (GTK_OPTION_MENU (gum),
			       unit - gum->start + (with_pixels ? 1 : 0));
  
  return GTK_WIDGET (gum);
}


void
gimp_unit_menu_set_unit (GimpUnitMenu *gum,
			 GUnit         unit)
{
  g_return_if_fail (gum != NULL);
  g_return_if_fail (GIMP_IS_UNIT_MENU (gum));

  /* replace UNIT_END when unit database is there */

  g_return_if_fail ((unit >= gum->start) && (unit < UNIT_END));

  gum->unit = unit;
  gtk_option_menu_set_history (GTK_OPTION_MENU (gum),
			       unit - gum->start +
			       ((gum->start == UNIT_PIXEL) ? 1 : 0));
}

GUnit
gimp_unit_menu_get_unit (GimpUnitMenu *gum)
{
  g_return_val_if_fail (gum != NULL, UNIT_INCH);
  g_return_val_if_fail (GIMP_IS_UNIT_MENU (gum), UNIT_INCH);

  return gum->unit;
}


/* most of the next two functions is stolen from app/gdisplay.h ;-) */

static int
print (char *buf, int len, int start, const char *fmt, ...)
{
  va_list args;
  int printed;

  va_start (args, fmt);

  printed = g_vsnprintf (buf + start, len - start, fmt, args);
  if (printed < 0)
    printed = len - start;

  va_end (args);

  return printed;
}

static const gchar*
gimp_unit_menu_build_string (gchar *format, GUnit unit)
{

#define BUFFER_LEN 64

  static gchar buffer[BUFFER_LEN];
  gchar format_buffer[16];
  int i = 0;

  while (i < (BUFFER_LEN - 1) && *format)
    {
      switch (*format)
	{
	case '%':
	  format++;
	  switch (*format)
	    {
	    case 0:
	      g_warning (_("unit-menu-format string ended within %%-sequence"));
	      break;
	      
	    case '%':
	      buffer[i++] = '%';
	      break;
	      
	    case 'f': /* factor (how many units make up an inch) */
	      g_snprintf(format_buffer, 16, "%%.%df",
			  gimp_unit_get_digits (unit));
	      i += print (buffer, BUFFER_LEN, i, format_buffer,
			  gimp_unit_get_factor (unit));
	      break;
	      
	    case 'y': /* symbol ("''" for inch) */
	      i += print (buffer, BUFFER_LEN, i, "%s",
			  gimp_unit_get_symbol (unit));
	      break;
	      
	    case 'a': /* abbreviation */
	      i += print (buffer, BUFFER_LEN, i, "%s",
			  gimp_unit_get_abbreviation (unit));
	      break;
	      
	    case 's': /* singular */
	      i += print (buffer, BUFFER_LEN, i, "%s",
			  gimp_unit_get_singular (unit));
	      break;
	    case 'p': /* plural */
	      i += print (buffer, BUFFER_LEN, i, "%s",
			  gimp_unit_get_plural (unit));
	      break;
	      
	    default:
	      g_warning (_("unit-menu-format contains unknown"
			   "format sequence '%%%c'"), *format);
	      break;
	    }
	  break;
	  
	default:
	  buffer[i++] = *format;
	  break;
	}
      
      format++;
    }
  
  buffer[MIN(i, BUFFER_LEN - 1)] = 0;
  
  return buffer;
}


static void
gimp_unit_menu_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpUnitMenu *gum;
  GUnit         new_unit;

  gum = data;
  new_unit = (guint) gtk_object_get_data (GTK_OBJECT (widget),
					  "gimp_unit_menu"); 
  
  if (gum->unit == new_unit)
    return;

  gum->unit = new_unit;
  gtk_signal_emit (GTK_OBJECT (gum),
		   gimp_unit_menu_signals[GUM_UNIT_CHANGED_SIGNAL]);
}
