/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpwidgets-utils.c
 * Copyright (C) 1999-2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


extern gchar *prog_name;

static void  gimp_message_box_close_callback  (GtkWidget *widget,
					       gpointer   data);

/*
 *  Message Boxes...
 */

typedef struct _MessageBox MessageBox;

struct _MessageBox
{
  GtkWidget   *mbox;
  GtkWidget   *vbox;
  GtkWidget   *repeat_label;
  gchar       *domain;
  gchar       *message;
  gint         repeat_count;
  GtkCallback  callback;
  gpointer     data;
};

/*  the maximum number of concurrent dialog boxes */
#define MESSAGE_BOX_MAXIMUM  4

static GList *message_boxes = NULL;

void
gimp_message_box (const gchar *stock_id,
                  const gchar *domain,
                  const gchar *message,
		  GtkCallback  callback,
		  gpointer     data)
{
  MessageBox     *msg_box;
  GtkWidget      *mbox;
  GtkWidget      *hbox;
  GtkWidget      *vbox;
  GtkWidget      *image;
  GtkWidget      *label;
  GList          *list;
  PangoAttrList  *attrs;
  PangoAttribute *attr;
  gchar          *str;

  g_return_if_fail (stock_id != NULL);
  g_return_if_fail (message != NULL);

  if (! domain)
    domain = _("GIMP");

  if (g_list_length (message_boxes) > MESSAGE_BOX_MAXIMUM)
    {
      g_printerr ("%s: %s\n", domain, message);
      return;
    }

  for (list = message_boxes; list; list = list->next)
    {
      msg_box = list->data;

      if (strcmp (msg_box->message, message) == 0 &&
          strcmp (msg_box->domain,  domain)  == 0)
	{
	  msg_box->repeat_count++;

	  if (msg_box->repeat_count > 1)
	    {
	      gchar *text = g_strdup_printf (_("Message repeated %d times."), 
					     msg_box->repeat_count);
	      gtk_label_set_text (GTK_LABEL (msg_box->repeat_label), text);
	      g_free (text);
	    }
	  else
	    {
              GtkWidget *label;

              attrs = pango_attr_list_new ();

              attr = pango_attr_style_new (PANGO_STYLE_OBLIQUE);
              attr->start_index = 0;
              attr->end_index   = -1;
              pango_attr_list_insert (attrs, attr);

              label = gtk_label_new (_("Message repeated once."));
              gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
              gtk_label_set_attributes (GTK_LABEL (label), attrs);
	      gtk_box_pack_end (GTK_BOX (msg_box->vbox), label,
                                FALSE, FALSE, 0);
	      gtk_widget_show (label);

              pango_attr_list_unref (attrs);

	      msg_box->repeat_label = label;
	    }

          gtk_window_present (GTK_WINDOW (msg_box->mbox));
	  return;
	}
    }

  if (g_list_length (message_boxes) == MESSAGE_BOX_MAXIMUM)
    {
      g_printerr ("%s: %s\n", domain, message);
      message = _("WARNING:\n"
		  "Too many open message dialogs.\n"
		  "Messages are redirected to stderr.");
    }

  msg_box = g_new0 (MessageBox, 1);

  mbox = gimp_dialog_new (_("GIMP Message"), "gimp-message",
			  NULL, NULL,
			  GTK_WIN_POS_MOUSE,
			  FALSE, FALSE, FALSE,

			  GTK_STOCK_OK, gimp_message_box_close_callback,
			  msg_box, NULL, NULL, TRUE, TRUE,

			  NULL);

  gtk_window_set_resizable (GTK_WINDOW (mbox), FALSE);
  gtk_dialog_set_has_separator (GTK_DIALOG (mbox), FALSE);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (mbox)->vbox), hbox);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);  

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  attrs = pango_attr_list_new ();

  attr = pango_attr_scale_new (PANGO_SCALE_LARGE);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  str = g_strdup_printf (_("%s Message"), domain);
  label = gtk_label_new (str);
  g_free (str);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_attributes (GTK_LABEL (label), attrs);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  pango_attr_list_unref (attrs);

  label = gtk_label_new (message);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  msg_box->mbox     = mbox;
  msg_box->vbox     = vbox;
  msg_box->domain   = g_strdup (domain);
  msg_box->message  = g_strdup (message);
  msg_box->callback = callback;
  msg_box->data     = data;

  message_boxes = g_list_prepend (message_boxes, msg_box);

  gtk_widget_show (mbox);
}

static void
gimp_message_box_close_callback (GtkWidget *widget,
				 gpointer   data)
{
  MessageBox *msg_box;

  msg_box = (MessageBox *) data;

  /*  If there is a valid callback, invoke it  */
  if (msg_box->callback)
    (* msg_box->callback) (widget, msg_box->data);

  /*  Destroy the box  */
  gtk_widget_destroy (msg_box->mbox);
  
  /* make this box available again */
  message_boxes = g_list_remove (message_boxes, msg_box);

  g_free (msg_box->domain);
  g_free (msg_box->message);
  g_free (msg_box);
}

void
gimp_menu_position (GtkMenu *menu,
		    gint    *x,
		    gint    *y,
		    guint   *button,
		    guint32 *activate_time)
{
  GdkEvent       *current_event;
  GtkRequisition  requisition;
  gint            pointer_x;
  gint            pointer_y;
  gint            screen_width;
  gint            screen_height;

  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);
  g_return_if_fail (button != NULL);
  g_return_if_fail (activate_time != NULL);

  gdk_window_get_pointer (NULL, &pointer_x, &pointer_y, NULL);

  gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

  screen_width  = gdk_screen_width ()  + 2;
  screen_height = gdk_screen_height () + 2;

  *x = CLAMP (pointer_x, 2, MAX (0, screen_width  - requisition.width));
  *y = CLAMP (pointer_y, 2, MAX (0, screen_height - requisition.height));

  *x += (pointer_x <= *x) ? 2 : -2;
  *y += (pointer_y <= *y) ? 2 : -2;

  *x = MAX (*x, 0);
  *y = MAX (*y, 0);

  current_event = gtk_get_current_event ();

  if (current_event && current_event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *bevent;

      bevent = (GdkEventButton *) current_event;

      *button        = bevent->button;
      *activate_time = bevent->time;
    }
  else
    {
      *button        = 0;
      *activate_time = 0;
    }
}

void
gimp_table_attach_stock (GtkTable    *table,
                         gint         row,
			 const gchar *label_text,
			 gdouble      yalign,
                         GtkWidget   *widget,
			 gint         colspan,
                         const gchar *stock_id)
{
  GtkWidget *image;
  GtkWidget *label;

  g_return_if_fail (GTK_IS_TABLE (table));
  g_return_if_fail (label_text != NULL);

  label = gtk_label_new_with_mnemonic (label_text);

  gtk_misc_set_alignment (GTK_MISC (label), 1.0, yalign);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_table_attach (table, label, 0, 1, row, row + 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
 
  if (widget)
    {
      g_return_if_fail (GTK_IS_WIDGET (widget));

      gtk_table_attach (table, widget, 1, 1 + colspan, row, row + 1,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (widget);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
    }

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
  
  if (image)
    {
      gtk_misc_set_alignment (GTK_MISC (image), 0.0, 0.5);
      gtk_table_attach (table, image, 1 + colspan, 2 + colspan, row, row + 1,
			GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (image);
    }
}

GtkIconSize
gimp_get_icon_size (GtkWidget   *widget,
                    const gchar *stock_id,
                    GtkIconSize  max_size,
                    gint         width,
                    gint         height)
{
  GtkIconSet   *icon_set;
  GtkIconSize  *sizes;
  gint          n_sizes;
  gint          i;
  gint          width_diff  = 1024;
  gint          height_diff = 1024;
  gint          max_width;
  gint          max_height;
  GtkIconSize   icon_size = GTK_ICON_SIZE_MENU;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), icon_size);
  g_return_val_if_fail (stock_id != NULL, icon_size);
  g_return_val_if_fail (width > 0, icon_size);
  g_return_val_if_fail (height > 0, icon_size);

  icon_set = gtk_style_lookup_icon_set (widget->style, stock_id);

  if (! icon_set)
    return GTK_ICON_SIZE_INVALID;

  if (! gtk_icon_size_lookup (max_size, &max_width, &max_height))
    {
      max_width  = 1024;
      max_height = 1024;
    }

  gtk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);

  for (i = 0; i < n_sizes; i++)
    {
      gint icon_width;
      gint icon_height;

      if (gtk_icon_size_lookup (sizes[i], &icon_width, &icon_height))
        {
          if (icon_width  <= width      &&
              icon_height <= height     &&
              icon_width  <= max_width  &&
              icon_height <= max_height &&
              ((width  - icon_width)  < width_diff ||
               (height - icon_height) < height_diff))
            {
              width_diff  = width  - icon_width;
              height_diff = height - icon_height;

              icon_size = sizes[i];
            }
        }
    }

  g_free (sizes);

  return icon_size;
}


/*  The format string which is used to display modifier names
 *  <Shift>, <Ctrl> and <Alt>
 */
#define GIMP_MOD_NAME_FORMAT_STRING N_("<%s>")

const gchar *
gimp_get_mod_name_shift (void)
{
  static gchar *mod_name_shift = NULL;

  if (! mod_name_shift)
    {
      GtkAccelLabelClass *accel_label_class;

      accel_label_class = g_type_class_ref (GTK_TYPE_ACCEL_LABEL);
      mod_name_shift = g_strdup_printf (gettext (GIMP_MOD_NAME_FORMAT_STRING),
                                        accel_label_class->mod_name_shift);
      g_type_class_unref (accel_label_class);
    }

  return (const gchar *) mod_name_shift;
}

const gchar *
gimp_get_mod_name_control (void)
{
  static gchar *mod_name_control = NULL;

  if (! mod_name_control)
    {
      GtkAccelLabelClass *accel_label_class;

      accel_label_class = g_type_class_ref (GTK_TYPE_ACCEL_LABEL);
      mod_name_control = g_strdup_printf (gettext (GIMP_MOD_NAME_FORMAT_STRING),
                                          accel_label_class->mod_name_control);
      g_type_class_unref (accel_label_class);
    }

  return (const gchar *) mod_name_control;
}

const gchar *
gimp_get_mod_name_alt (void)
{
  static gchar *mod_name_alt = NULL;

  if (! mod_name_alt)
    {
      GtkAccelLabelClass *accel_label_class;

      accel_label_class = g_type_class_ref (GTK_TYPE_ACCEL_LABEL);
      mod_name_alt = g_strdup_printf (gettext (GIMP_MOD_NAME_FORMAT_STRING),
                                      accel_label_class->mod_name_alt);
      g_type_class_unref (accel_label_class);
    }

  return (const gchar *) mod_name_alt;
}

const gchar *
gimp_get_mod_separator (void)
{
  static gchar *mod_separator = NULL;

  if (! mod_separator)
    {
      GtkAccelLabelClass *accel_label_class;

      accel_label_class = g_type_class_ref (GTK_TYPE_ACCEL_LABEL);
      mod_separator = g_strdup (accel_label_class->mod_separator);
      g_type_class_unref (accel_label_class);
    }

  return (const gchar *) mod_separator;
}

/**
 * gimp_get_screen_resolution:
 * @screen: a #GdkScreen or %NULL
 * @xres: returns the horizontal screen resolution (in dpi)
 * @yres: returns the vertical screen resolution (in dpi)
 * 
 * Retrieves the screen resolution from GDK. If @screen is %NULL, the
 * default screen is used.
 **/
void
gimp_get_screen_resolution (GdkScreen *screen,
                            gdouble   *xres,
                            gdouble   *yres)
{
  gint    width, height;
  gint    width_mm, height_mm;
  gdouble x = 0.0;
  gdouble y = 0.0;

  g_return_if_fail (screen == NULL || GDK_IS_SCREEN (screen));
  g_return_if_fail (xres != NULL);
  g_return_if_fail (yres != NULL);

  if (!screen)
    screen = gdk_screen_get_default ();

  width  = gdk_screen_get_width (screen);
  height = gdk_screen_get_height (screen);

  width_mm  = gdk_screen_get_width_mm (screen);
  height_mm = gdk_screen_get_height_mm (screen);

  /*
   * From xdpyinfo.c:
   *
   * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
   *
   *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
   *         = N pixels / (M inch / 25.4)
   *         = N * 25.4 pixels / M inch
   */

  if (width_mm > 0 && height_mm > 0)
    {
      x = (width  * 25.4) / (gdouble) width_mm;
      y = (height * 25.4) / (gdouble) height_mm;
    }

  if (x < GIMP_MIN_RESOLUTION || x > GIMP_MAX_RESOLUTION ||
      y < GIMP_MIN_RESOLUTION || y > GIMP_MAX_RESOLUTION)
    {
      g_warning ("GDK returned bogus values for the screen resolution, "
                 "using 75 dpi instead.");

      x = 75.0;
      y = 75.0;
    }

  /*  round the value to full integers to give more pleasant results  */
  *xres = ROUND (x);
  *yres = ROUND (y);
}


/**
 * gimp_rgb_get_gdk_color:
 * @color: the source color as #GimpRGB
 * @gdk_color: pointer to a #GdkColor
 * 
 * Initializes @gdk_color from a #GimpRGB. This function does not
 * allocate the color for you. Depending on how you want to use it,
 * you may have to call gdk_colormap_alloc_color().
 **/
void          
gimp_rgb_get_gdk_color (const GimpRGB *color,
                        GdkColor      *gdk_color)
{
  guchar r, g, b;

  g_return_if_fail (color != NULL);
  g_return_if_fail (gdk_color != NULL);
  
  gimp_rgb_get_uchar (color, &r, &g, &b);
  
  gdk_color->red   = (r << 8) | r;
  gdk_color->green = (g << 8) | g;
  gdk_color->blue  = (b << 8) | b;
}
