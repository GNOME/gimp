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
  GtkWidget   *dialog;
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


static void  gimp_message_box_set_icons (GtkWidget   *dialog,
                                         const gchar *stock_id);
static void  gimp_message_box_response  (GtkWidget   *widget,
                                         gint         response_id,
                                         MessageBox  *msg_box);


extern gchar *prog_name;

static GList *message_boxes = NULL;


void
gimp_message_box (const gchar *stock_id,
                  const gchar *domain,
                  const gchar *message,
		  GtkCallback  callback,
		  gpointer     data)
{
  MessageBox *msg_box;
  GtkWidget  *dialog;
  GtkWidget  *hbox;
  GtkWidget  *vbox;
  GtkWidget  *image;
  GtkWidget  *label;
  GList      *list;
  gchar      *str;

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

              label = gtk_label_new (_("Message repeated once."));
              gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
              gimp_label_set_attributes (GTK_LABEL (label),
                                         PANGO_ATTR_STYLE, PANGO_STYLE_OBLIQUE,
                                         -1);
	      gtk_box_pack_end (GTK_BOX (msg_box->vbox), label,
                                FALSE, FALSE, 0);
	      gtk_widget_show (label);

	      msg_box->repeat_label = label;
	    }

          gtk_window_present (GTK_WINDOW (msg_box->dialog));
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

  dialog = gimp_dialog_new (_("GIMP Message"), "gimp-message",
                            NULL, 0,
                            NULL, NULL,

                            GTK_STOCK_OK, GTK_RESPONSE_CLOSE,

                            NULL);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  gimp_message_box_set_icons (dialog, stock_id);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_message_box_response),
                    msg_box);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  str = g_strdup_printf (_("%s Message"), domain);
  label = gtk_label_new (str);
  g_free (str);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = gtk_label_new (message);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  msg_box->dialog   = dialog;
  msg_box->vbox     = vbox;
  msg_box->domain   = g_strdup (domain);
  msg_box->message  = g_strdup (message);
  msg_box->callback = callback;
  msg_box->data     = data;

  message_boxes = g_list_prepend (message_boxes, msg_box);

  gtk_widget_show (dialog);
}

static void
gimp_message_box_set_icons (GtkWidget   *dialog,
                            const gchar *stock_id)
{
  GtkIconSet  *icon_set;
  GtkIconSize *sizes;
  GList       *icons = NULL;
  gint         i, n_sizes;

  gtk_widget_ensure_style (dialog);

  icon_set = gtk_style_lookup_icon_set (dialog->style, stock_id);

  gtk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);

  for (i = 0; i < n_sizes; i++)
    {
      if (sizes[i] < GTK_ICON_SIZE_DIALOG)  /* skip the large version */
        icons = g_list_prepend (icons,
                                gtk_widget_render_icon (dialog,
                                                        stock_id, sizes[i],
                                                        NULL));
    }

  g_free (sizes);

  if (icons)
    {
      gtk_window_set_icon_list (GTK_WINDOW (dialog), icons);

      g_list_foreach (icons, (GFunc) g_object_unref, NULL);
      g_list_free (icons);
    }
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
  gtk_widget_destroy (msg_box->dialog);

  /* make this box available again */
  message_boxes = g_list_remove (message_boxes, msg_box);

  g_free (msg_box->domain);
  g_free (msg_box->message);
  g_free (msg_box);
}

/**
 * gimp_menu_position:
 * @menu: a #GtkMenu widget
 * @x: pointer to horizontal position
 * @y: pointer to vertical position
 *
 * Positions a #GtkMenu so that it pops up on screen.  This function
 * takes care of the preferred popup direction (taken from the widget
 * render direction) and it handles multiple monitors representing a
 * single #GdkScreen (Xinerama).
 *
 * You should call this function with @x and @y initialized to the
 * origin of the menu. This is typically the center of the widget the
 * menu is popped up from. gimp_menu_position() will then decide if
 * and how these initial values need to be changed.
 **/
void
gimp_menu_position (GtkMenu *menu,
		    gint    *x,
		    gint    *y)
{
  GtkWidget      *widget;
  GdkScreen      *screen;
  GtkRequisition  requisition;
  GdkRectangle    rect;
  gint            monitor;

  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  widget = GTK_WIDGET (menu);

  screen = gtk_widget_get_screen (widget);

  monitor = gdk_screen_get_monitor_at_point (screen, *x, *y);
  gdk_screen_get_monitor_geometry (screen, monitor, &rect);

  gtk_menu_set_screen (menu, screen);

#ifdef __GNUC__
#warning FIXME: remove this hack as soon as we depend on GTK+ 2.4.4
#endif
  if (gtk_check_version (2, 4, 4))
    gtk_menu_set_monitor (menu, monitor);

  gtk_widget_size_request (widget, &requisition);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      *x -= requisition.width;
      if (*x < rect.x)
        *x += requisition.width;
    }
  else
    {
      if (*x + requisition.width > rect.x + rect.width)
        *x -= requisition.width;
    }

  if (*x < rect.x)
    *x = rect.x;

  if (*y + requisition.height > rect.y + rect.height)
    *y -= requisition.height;

  if (*y < rect.y)
    *y = rect.y;
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
  GdkRectangle    rect;
  gint            monitor;

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

  gdk_window_get_origin (button->window, x, y);

  gtk_widget_size_request (GTK_WIDGET (menu), &menu_requisition);

  screen = gtk_widget_get_screen (button);

  monitor = gdk_screen_get_monitor_at_point (screen, *x, *y);
  gdk_screen_get_monitor_geometry (screen, monitor, &rect);

  gtk_menu_set_screen (menu, screen);

#ifdef __GNUC__
#warning FIXME: remove this hack as soon as we depend on GTK+ 2.4.4
#endif
  if (gtk_check_version (2, 4, 4))
    gtk_menu_set_monitor (menu, monitor);

  *x += button->allocation.x;

  switch (position)
    {
    case GTK_POS_LEFT:
      *x -= menu_requisition.width;
      if (*x < rect.x)
        *x += menu_requisition.width + button->allocation.width;
      break;

    case GTK_POS_RIGHT:
      *x += button->allocation.width;
      if (*x + menu_requisition.width > rect.x + rect.width)
        *x -= button->allocation.width + menu_requisition.width;
      break;

    default:
      g_warning ("%s: unhandled position (%d)", G_STRFUNC, position);
      break;
    }

  *y += button->allocation.y + button->allocation.height / 2;

  if (*y + menu_requisition.height > rect.y + rect.height)
    *y -= menu_requisition.height;
  if (*y < rect.y)
    *y = rect.y;
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

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, yalign);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
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

const gchar *
gimp_get_mod_string (GdkModifierType modifiers)
{
  static struct
  {
    GdkModifierType  modifiers;
    gchar           *name;
  }
  modifier_strings[] =
  {
    { GDK_SHIFT_MASK,                                    NULL },
    { GDK_CONTROL_MASK,                                  NULL },
    { GDK_MOD1_MASK,                                     NULL },
    { GDK_SHIFT_MASK | GDK_CONTROL_MASK,                 NULL },
    { GDK_SHIFT_MASK | GDK_MOD1_MASK,                    NULL },
    { GDK_CONTROL_MASK | GDK_MOD1_MASK,                  NULL },
    { GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK, NULL }
  };

  gint i;

  for (i = 0; i < G_N_ELEMENTS (modifier_strings); i++)
    {
      if (modifiers == modifier_strings[i].modifiers)
        {
          if (! modifier_strings[i].name)
            {
              GString *str = g_string_new ("");

              if (modifiers & GDK_SHIFT_MASK)
                {
                  g_string_append (str, gimp_get_mod_name_shift ());
                }

              if (modifiers & GDK_CONTROL_MASK)
                {
                  if (str->len)
                    g_string_append (str, gimp_get_mod_separator ());

                  g_string_append (str, gimp_get_mod_name_control ());
                }

              if (modifiers & GDK_MOD1_MASK)
                {
                  if (str->len)
                    g_string_append (str, gimp_get_mod_separator ());

                  g_string_append (str, gimp_get_mod_name_alt ());
                }

              modifier_strings[i].name = g_string_free (str, FALSE);
            }

          return modifier_strings[i].name;
        }
    }

  return NULL;
}

static void
gimp_substitute_underscores (gchar *str)
{
  gchar *p;

  for (p = str; *p; p++)
    if (*p == '_')
      *p = ' ';
}

gchar *
gimp_get_accel_string (guint           key,
                       GdkModifierType modifiers)
{
  GtkAccelLabelClass *accel_label_class;
  GString            *gstring;
  gunichar            ch;

  accel_label_class = g_type_class_peek (GTK_TYPE_ACCEL_LABEL);

  gstring = g_string_new (gimp_get_mod_string (modifiers));

  if (gstring->len > 0)
    g_string_append (gstring, gimp_get_mod_separator ());

  ch = gdk_keyval_to_unicode (key);

  if (ch && (g_unichar_isgraph (ch) || ch == ' ') &&
      (ch < 0x80 || accel_label_class->latin1_to_char))
    {
      switch (ch)
        {
        case ' ':
          g_string_append (gstring, "Space");
          break;
        case '\\':
          g_string_append (gstring, "Backslash");
          break;
        default:
          g_string_append_unichar (gstring, g_unichar_toupper (ch));
          break;
        }
    }
  else
    {
      gchar *tmp;

      tmp = gtk_accelerator_name (key, 0);

      if (tmp[0] != 0 && tmp[1] == 0)
        tmp[0] = g_ascii_toupper (tmp[0]);

      gimp_substitute_underscores (tmp);
      g_string_append (gstring, tmp);
      g_free (tmp);
    }

  return g_string_free (gstring, FALSE);
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

void
gimp_window_set_hint (GtkWindow      *window,
                      GimpWindowHint  hint)
{
  g_return_if_fail (GTK_IS_WINDOW (window));

  switch (hint)
    {
    case GIMP_WINDOW_HINT_NORMAL:
      gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_NORMAL);
      break;

    case GIMP_WINDOW_HINT_UTILITY:
      gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_UTILITY);
      break;

    case GIMP_WINDOW_HINT_KEEP_ABOVE:
      gtk_window_set_keep_above (window, TRUE);
      break;
    }
}
