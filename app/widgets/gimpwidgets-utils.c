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


static void  gimp_message_box_response (GtkWidget  *widget,
                                        gint        response_id,
                                        MessageBox *msg_box);


extern gchar *prog_name;

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
      g_printerr ("%s: %s\n\n", domain, message);
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
      g_printerr ("%s: %s\n\n", domain, message);
      message = _("WARNING:\n"
		  "Too many open message dialogs.\n"
		  "Messages are redirected to stderr.");
    }

  msg_box = g_new0 (MessageBox, 1);

  mbox = gimp_dialog_new (_("GIMP Message"), "gimp-message",
                          NULL, 0,
			  NULL, NULL,

			  GTK_STOCK_OK, GTK_RESPONSE_CLOSE,

			  NULL);

  gtk_window_set_resizable (GTK_WINDOW (mbox), FALSE);

  g_signal_connect (mbox, "response",
                    G_CALLBACK (gimp_message_box_response),
                    msg_box);

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
gimp_message_box_response (GtkWidget  *widget,
                           gint        resonse_id,
                           MessageBox *msg_box)
{
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
gimp_menu_position (GtkMenu  *menu,
		    gint     *x,
		    gint     *y,
                    gpointer  data)
{
  GtkRequisition  requisition;
  GdkScreen      *screen;
  gint            pointer_x;
  gint            pointer_y;
  gint            screen_width;
  gint            screen_height;

  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);
  g_return_if_fail (GTK_IS_WIDGET (data));

  gdk_display_get_pointer (gtk_widget_get_display (GTK_WIDGET (data)),
                           &screen, &pointer_x, &pointer_y, NULL);

  gtk_menu_set_screen (menu, screen);

  gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

  screen_width  = gdk_screen_get_width (screen);
  screen_height = gdk_screen_get_height (screen);

  if (gtk_widget_get_direction (GTK_WIDGET (menu)) == GTK_TEXT_DIR_RTL)
    {
      *x = pointer_x - 2 - requisition.width;

      if (*x < 0)
        *x = pointer_x + 2;
    }
  else
    {
      *x = pointer_x + 2;

      if (*x + requisition.width > screen_width)
        *x = pointer_x - 2 - requisition.width;
    }

  *y = pointer_y + 2;

  if (*y + requisition.height > screen_height)
    *y = pointer_y - 2 - requisition.height;

  if (*x < 0) *x = 0;
  if (*y < 0) *y = 0;
}

/**
 * gimp_button_menu_position:
 * @button: a button widget to popup the menu from
 * @menu: the menu to position
 * @position: the preferred popup direction for the menu (left or right)
 * @x: return location for x coordinate
 * @y: return location for y coordinate
 *
 * Utility function to position a menu that pops up from a button.
 **/
void
gimp_button_menu_position (GtkWidget       *button,
                           GtkMenu         *menu,
                           GtkPositionType  position,
                           gint            *x,
                           gint            *y)
{
  GdkScreen      *screen;
  GtkRequisition  menu_requisition;

  g_return_if_fail (GTK_WIDGET_REALIZED (button));
  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  if (gtk_widget_get_direction (button) == GTK_TEXT_DIR_RTL)
    {
      switch (position)
        {
        case GTK_POS_LEFT:   position = GTK_POS_RIGHT;  break;
        case GTK_POS_RIGHT:  position = GTK_POS_LEFT;   break;
        default:
          break;
        }
    }

  screen = gtk_widget_get_screen (GTK_WIDGET (button));

  gtk_menu_set_screen (menu, screen);

  gdk_window_get_origin (button->window, x, y);

  gtk_widget_size_request (GTK_WIDGET (menu), &menu_requisition);

  *x += button->allocation.x;

  switch (position)
    {
    case GTK_POS_LEFT:
      *x -= menu_requisition.width;
      if (*x < 0)
        *x += menu_requisition.width + button->allocation.width;
      break;

    case GTK_POS_RIGHT:
      *x += button->allocation.width;
      if (*x + menu_requisition.width > gdk_screen_get_width (screen))
        *x -= button->allocation.width + menu_requisition.width;
      break;

    default:
      g_warning ("gimp_button_menu_position: "
                 "unhandled position (%d)", position);
      break;
    }

  *y += button->allocation.y + button->allocation.height / 2;

  if (*y + menu_requisition.height > gdk_screen_get_height (screen))
    *y -= menu_requisition.height;
  if (*y < 0)
    *y = 0;
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
  GdkScreen    *screen;
  GtkSettings  *settings;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), icon_size);
  g_return_val_if_fail (stock_id != NULL, icon_size);
  g_return_val_if_fail (width > 0, icon_size);
  g_return_val_if_fail (height > 0, icon_size);

  icon_set = gtk_style_lookup_icon_set (widget->style, stock_id);

  if (! icon_set)
    return GTK_ICON_SIZE_INVALID;

  screen = gtk_widget_get_screen (widget);
  settings = gtk_settings_get_for_screen (screen);

  if (! gtk_icon_size_lookup_for_settings (settings, max_size,
                                           &max_width, &max_height))
    {
      max_width  = 1024;
      max_height = 1024;
    }

  gtk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);

  for (i = 0; i < n_sizes; i++)
    {
      gint icon_width;
      gint icon_height;

      if (gtk_icon_size_lookup_for_settings (settings, sizes[i],
                                             &icon_width, &icon_height))
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
 * @rgb: the source color as #GimpRGB
 * @gdk_color: pointer to a #GdkColor
 *
 * Initializes @gdk_color from a #GimpRGB. This function does not
 * allocate the color for you. Depending on how you want to use it,
 * you may have to call gdk_colormap_alloc_color().
 **/
void
gimp_rgb_get_gdk_color (const GimpRGB *rgb,
                        GdkColor      *gdk_color)
{
  guchar r, g, b;

  g_return_if_fail (rgb != NULL);
  g_return_if_fail (gdk_color != NULL);

  gimp_rgb_get_uchar (rgb, &r, &g, &b);

  gdk_color->red   = (r << 8) | r;
  gdk_color->green = (g << 8) | g;
  gdk_color->blue  = (b << 8) | b;
}

/**
 * gimp_rgb_set_gdk_color:
 * @rgb: a #GimpRGB that is to be set
 * @gdk_color: pointer to the source #GdkColor
 *
 * Initializes @rgb from a #GdkColor. This function does not touch
 * the alpha value of @rgb.
 **/
void
gimp_rgb_set_gdk_color (GimpRGB        *rgb,
                        const GdkColor *gdk_color)
{
  guchar r, g, b;

  g_return_if_fail (rgb != NULL);
  g_return_if_fail (gdk_color != NULL);

  r = gdk_color->red   >> 8;
  g = gdk_color->green >> 8;
  b = gdk_color->blue  >> 8;

  gimp_rgb_set_uchar (rgb, r, g, b);
}
