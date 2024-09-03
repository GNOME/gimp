/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpwidgets-utils.c
 * Copyright (C) 1999-2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_WIN32
#include <dwmapi.h>
#include <gdk/gdkwin32.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#ifdef PLATFORM_OSX
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gegl/gimp-babl.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpprogress.h"
#include "core/gimptoolinfo.h"

#include "gimpaccellabel.h"
#include "gimpaction.h"
#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockcontainer.h"
#include "gimpdockwindow.h"
#include "gimperrordialog.h"
#include "gimpsessioninfo.h"
#include "gimptoolbutton.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"
#include "gimpwindowstrategy.h"

#include "gimp-intl.h"


#define GIMP_TOOL_OPTIONS_GUI_KEY      "gimp-tool-options-gui"
#define GIMP_TOOL_OPTIONS_GUI_FUNC_KEY "gimp-tool-options-gui-func"


typedef struct
{
  GList       **blink_script;
  const gchar  *widget_identifier;
  const gchar  *settings_value;
} BlinkSearch;

typedef struct
{
  GtkWidget *widget;
  gchar     *settings_value;
} BlinkStep;

static void         gimp_widget_blink_after             (GtkWidget    *widget,
                                                         gint          ms_timeout);
static void         gimp_search_widget_rec              (GtkWidget    *widget,
                                                         BlinkSearch  *data);
static void         gimp_blink_free_script              (GList        *blink_scenario);

static gboolean     gimp_window_transient_on_mapped     (GtkWidget    *widget,
                                                         GdkEventAny  *event,
                                                         GimpProgress *progress);
static void         gimp_window_set_transient_cb        (GtkWidget    *window,
                                                         GdkEventAny  *event,
                                                         GBytes       *handle);


GtkWidget *
gimp_menu_item_get_image (GtkMenuItem *item)
{
  g_return_val_if_fail (GTK_IS_MENU_ITEM (item), NULL);

  return g_object_get_data (G_OBJECT (item), "gimp-menu-item-image");
}

void
gimp_menu_item_set_image (GtkMenuItem *item,
                          GtkWidget   *image,
                          GimpAction  *action)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *accel_label;
  GtkWidget *old_image;

  g_return_if_fail (GTK_IS_MENU_ITEM (item));
  g_return_if_fail (image == NULL || GTK_IS_WIDGET (image));

  hbox = g_object_get_data (G_OBJECT (item), "gimp-menu-item-hbox");

  if (! hbox)
    {
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      g_object_set_data (G_OBJECT (item), "gimp-menu-item-hbox", hbox);

      label = gtk_bin_get_child (GTK_BIN (item));
      g_object_set_data (G_OBJECT (item), "gimp-menu-item-label", label);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);

      g_object_ref (label);
      gtk_container_remove (GTK_CONTAINER (item), label);
      gtk_container_add (GTK_CONTAINER (hbox), label);
      g_object_unref (label);

      if (action)
        {
          accel_label = gimp_accel_label_new (action);
          g_object_set_data (G_OBJECT (item), "gimp-menu-item-accel", accel_label);
          gtk_container_add (GTK_CONTAINER (hbox), accel_label);
          gtk_widget_set_hexpand (GTK_WIDGET (accel_label), TRUE);
          gtk_label_set_xalign (GTK_LABEL (accel_label), 1.0);
          gtk_widget_show (accel_label);
        }

      gtk_container_add (GTK_CONTAINER (item), hbox);
      gtk_widget_show (hbox);
    }

  old_image = g_object_get_data (G_OBJECT (item), "gimp-menu-item-image");

  if (old_image != image)
    {
      if (old_image)
        {
          gtk_widget_destroy (old_image);
          g_object_set_data (G_OBJECT (item), "gimp-menu-item-image", NULL);
        }

      if (image)
        {
          gtk_container_add (GTK_CONTAINER (hbox), image);
          gtk_box_reorder_child (GTK_BOX (hbox), image, 0);
          g_object_set_data (G_OBJECT (item), "gimp-menu-item-image", image);
          gtk_widget_show (image);
        }
    }
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
  GtkRequisition  requisition;
  GdkRectangle    workarea;

  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  widget = GTK_WIDGET (menu);

  gdk_monitor_get_workarea (gimp_widget_get_monitor (widget), &workarea);

  gtk_menu_set_screen (menu, gtk_widget_get_screen (widget));

  gtk_widget_get_preferred_size (widget, &requisition, NULL);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      *x -= requisition.width;
      if (*x < workarea.x)
        *x += requisition.width;
    }
  else
    {
      if (*x + requisition.width > workarea.x + workarea.width)
        *x -= requisition.width;
    }

  if (*x < workarea.x)
    *x = workarea.x;

  if (*y + requisition.height > workarea.y + workarea.height)
    *y -= requisition.height;

  if (*y < workarea.y)
    *y = workarea.y;
}

void
gimp_grid_attach_icon (GtkGrid     *grid,
                       gint         row,
                       const gchar *icon_name,
                       GtkWidget   *widget,
                       gint         columns)
{
  GtkWidget *image;

  g_return_if_fail (GTK_IS_GRID (grid));
  g_return_if_fail (icon_name != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_halign (image, GTK_ALIGN_END);
  gtk_grid_attach (grid, image, 0, row, 1, 1);
  gtk_widget_show (image);

  gtk_grid_attach (grid, widget, 1, row, columns, 1);
  gtk_widget_show (widget);
}

void
gimp_enum_radio_box_add (GtkBox    *box,
                         GtkWidget *widget,
                         gint       enum_value,
                         gboolean   below)
{
  GList *children;
  GList *list;
  gint   pos;

  g_return_if_fail (GTK_IS_BOX (box));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  children = gtk_container_get_children (GTK_CONTAINER (box));

  for (list = children, pos = 1;
       list;
       list = g_list_next (list), pos++)
    {
      if (GTK_IS_RADIO_BUTTON (list->data) &&
          GPOINTER_TO_INT (g_object_get_data (list->data, "gimp-item-data")) ==
          enum_value)
        {
          GtkWidget *radio = list->data;
          GtkWidget *hbox;

          hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
          gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
          gtk_box_reorder_child (GTK_BOX (box), hbox, pos);

          if (below)
            {
              GtkWidget *spacer;
              gint       indicator_size;
              gint       indicator_spacing;
              gint       focus_width;
              gint       focus_padding;
              gint       border_width;

              gtk_widget_style_get (radio,
                                    "indicator-size",    &indicator_size,
                                    "indicator-spacing", &indicator_spacing,
                                    "focus-line-width",  &focus_width,
                                    "focus-padding",     &focus_padding,
                                    NULL);

              border_width = gtk_container_get_border_width (GTK_CONTAINER (radio));

              spacer = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
              gtk_widget_set_size_request (spacer,
                                           indicator_size +
                                           3 * indicator_spacing +
                                           focus_width +
                                           focus_padding +
                                           border_width,
                                           -1);
              gtk_box_pack_start (GTK_BOX (hbox), spacer, FALSE, FALSE, 0);
              gtk_widget_show (spacer);
            }
          else
            {
              GtkSizeGroup *size_group;

              size_group = g_object_get_data (G_OBJECT (box), "size-group");

              if (! size_group)
                {
                  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
                  g_object_set_data (G_OBJECT (box), "size-group", size_group);

                  gtk_size_group_add_widget (size_group, radio);
                  g_object_unref (size_group);
                }
              else
                {
                  gtk_size_group_add_widget (size_group, radio);
                }

              gtk_box_set_spacing (GTK_BOX (hbox), 4);

              g_object_ref (radio);
              gtk_container_remove (GTK_CONTAINER (box), radio);
              gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);
              g_object_unref (radio);
            }

          gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
          gtk_widget_show (widget);

          g_object_bind_property (radio,  "active",
                                  widget, "sensitive",
                                  G_BINDING_SYNC_CREATE);

          gtk_widget_show (hbox);

          break;
        }
    }

  g_list_free (children);
}

void
gimp_enum_radio_frame_add (GtkFrame  *frame,
                           GtkWidget *widget,
                           gint       enum_value,
                           gboolean   below)
{
  GtkWidget *box;

  g_return_if_fail (GTK_IS_FRAME (frame));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  box = gtk_bin_get_child (GTK_BIN (frame));

  g_return_if_fail (GTK_IS_BOX (box));

  gimp_enum_radio_box_add (GTK_BOX (box), widget, enum_value, below);
}

/**
 * gimp_widget_load_icon:
 * @widget:                  parent widget (to determine icon theme and
 *                           style)
 * @icon_name:               icon name
 * @size:                    requested pixel size
 *
 * Loads an icon into a pixbuf with size as close as possible to @size.
 * If icon does not exist or fail to load, the function will fallback to
 * "gimp-wilber-eek" instead to prevent NULL pixbuf. As a last resort,
 * if even the fallback failed to load, a magenta @size square will be
 * returned, so this function is guaranteed to always return a
 * #GdkPixbuf.
 *
 * Returns: a newly allocated #GdkPixbuf containing @icon_name at
 * size @size or a fallback icon/size.
 **/
GdkPixbuf *
gimp_widget_load_icon (GtkWidget   *widget,
                       const gchar *icon_name,
                       gint         size)
{
  GdkPixbuf    *pixbuf = NULL;
  GtkIconTheme *icon_theme;
  GtkIconInfo  *icon_info;
  gchar        *name;
  gint          scale_factor;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));
  scale_factor = gtk_widget_get_scale_factor (widget);
  name = g_strdup_printf ("%s-symbolic", icon_name);
  /* This will find the symbolic icon and fallback to non-symbolic
   * depending on icon theme.
   */
  icon_info = gtk_icon_theme_lookup_icon_for_scale (icon_theme, name,
                                                    size, scale_factor,
                                                    GTK_ICON_LOOKUP_GENERIC_FALLBACK);
  g_free (name);

  if (icon_info)
    {
      pixbuf = gtk_icon_info_load_symbolic_for_context (icon_info,
                                                        gtk_widget_get_style_context (widget),
                                                        NULL, NULL);
      g_object_unref (icon_info);
      if (! pixbuf)
        /* The icon was seemingly present in the current icon theme, yet
         * it failed to load. Maybe the file is broken?
         * As last resort, try to load "gimp-wilber-eek" as fallback.
         * Note that we are not making more checks, so if the fallback
         * icon fails to load as well, the function may still return NULL.
         */
        g_printerr ("WARNING: icon '%s' failed to load. Check the files "
                    "in your icon theme.\n", icon_name);
    }
  else
    g_printerr ("WARNING: icon theme has no icon '%s'.\n", icon_name);

  /* First fallback: gimp-wilber-eek */
  if (! pixbuf)
    {
      icon_info = gtk_icon_theme_lookup_icon_for_scale (icon_theme,
                                                        GIMP_ICON_WILBER_EEK "-symbolic",
                                                        size, scale_factor,
                                                        GTK_ICON_LOOKUP_GENERIC_FALLBACK);
      if (icon_info)
        {
          pixbuf = gtk_icon_info_load_symbolic_for_context (icon_info,
                                                            gtk_widget_get_style_context (widget),
                                                            NULL, NULL);
          g_object_unref (icon_info);
          if (! pixbuf)
            g_printerr ("WARNING: icon '%s' failed to load. Check the files "
                        "in your icon theme.\n", GIMP_ICON_WILBER_EEK);
        }
      else
        {
          g_printerr ("WARNING: icon theme has no icon '%s'.\n",
                      GIMP_ICON_WILBER_EEK);
        }
    }

  /* Last fallback: just a magenta square. */
  if (! pixbuf)
    {
      /* As last resort, just draw an ugly magenta square. */
      guchar *data;
      gint    rowstride = 3 * size * scale_factor;
      gint    i, j;

      data = g_new (guchar, rowstride * size);
      for (i = 0; i < size; i++)
        {
          for (j = 0; j < size * scale_factor; j++)
            {
              data[i * rowstride + j * 3] = 255;
              data[i * rowstride + j * 3 + 1] = 0;
              data[i * rowstride + j * 3 + 2] = 255;
            }
        }
      pixbuf = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB, FALSE,
                                         8,
                                         size * scale_factor,
                                         size * scale_factor, rowstride,
                                         (GdkPixbufDestroyNotify) g_free,
                                         NULL);
    }

  /* Small assertion test to get a warning if we ever get NULL return
   * value, as this is never supposed to happen.
   */
  g_return_val_if_fail (pixbuf != NULL, NULL);

  return pixbuf;
}

GimpTabStyle
gimp_preview_tab_style_to_icon (GimpTabStyle tab_style)
{
  switch (tab_style)
    {
    case GIMP_TAB_STYLE_PREVIEW:
      tab_style = GIMP_TAB_STYLE_ICON;
      break;

    case GIMP_TAB_STYLE_PREVIEW_NAME:
      tab_style = GIMP_TAB_STYLE_ICON_NAME;
      break;

    case GIMP_TAB_STYLE_PREVIEW_BLURB:
      tab_style = GIMP_TAB_STYLE_ICON_BLURB;
      break;

    default:
      break;
    }

  return tab_style;
}

const gchar *
gimp_get_mod_string (GdkModifierType modifiers)
{
  static GHashTable *mod_labels;
  gchar             *label;

  if (! modifiers)
    return NULL;

  if (G_UNLIKELY (! mod_labels))
    mod_labels = g_hash_table_new (g_int_hash, g_int_equal);

  modifiers = gimp_replace_virtual_modifiers (modifiers);

  label = g_hash_table_lookup (mod_labels, &modifiers);

  if (! label)
    {
      GtkAccelLabelClass *accel_label_class;

      label = gtk_accelerator_get_label (0, modifiers);

      accel_label_class = g_type_class_ref (GTK_TYPE_ACCEL_LABEL);

      if (accel_label_class->mod_separator &&
          *accel_label_class->mod_separator)
        {
          gchar *sep = g_strrstr (label, accel_label_class->mod_separator);

          if (sep - label ==
              strlen (label) - strlen (accel_label_class->mod_separator))
            *sep = '\0';
        }

      g_type_class_unref (accel_label_class);

      g_hash_table_insert (mod_labels,
                           g_memdup2 (&modifiers, sizeof (GdkModifierType)),
                           label);
    }

  return label;
}

#define BUF_SIZE 100
/**
 * gimp_suggest_modifiers:
 * @message:                 initial text for the message
 * @modifiers:               bit mask of modifiers that should be suggested
 * @extend_selection_format: optional format string for the
 *                           "Extend selection" modifier
 * @toggle_behavior_format:  optional format string for the
 *                           "Toggle behavior" modifier
 * @alt_format:              optional format string for the Alt modifier
 *
 * Utility function to build a message suggesting to use some
 * modifiers for performing different actions (only Shift, Ctrl and
 * Alt are currently supported).  If some of these modifiers are
 * already active, they will not be suggested.  The optional format
 * strings #extend_selection_format, #toggle_behavior_format and
 * #alt_format may be used to describe what the modifier will do.
 * They must contain a single '%%s' which will be replaced by the name
 * of the modifier.  They can also be %NULL if the modifier name
 * should be left alone.
 *
 * Returns: a newly allocated string containing the message.
 **/
gchar *
gimp_suggest_modifiers (const gchar     *message,
                        GdkModifierType  modifiers,
                        const gchar     *extend_selection_format,
                        const gchar     *toggle_behavior_format,
                        const gchar     *alt_format)
{
  GdkModifierType  extend_mask = gimp_get_extend_selection_mask ();
  GdkModifierType  toggle_mask = gimp_get_toggle_behavior_mask ();
  gchar            msg_buf[3][BUF_SIZE];
  gint             num_msgs = 0;
  gboolean         try      = FALSE;

  if (modifiers & extend_mask)
    {
      if (extend_selection_format && *extend_selection_format)
        {
          g_snprintf (msg_buf[num_msgs], BUF_SIZE, extend_selection_format,
                      gimp_get_mod_string (extend_mask));
        }
      else
        {
          g_strlcpy (msg_buf[num_msgs],
                     gimp_get_mod_string (extend_mask), BUF_SIZE);
          try = TRUE;
        }

      num_msgs++;
    }

  if (modifiers & toggle_mask)
    {
      if (toggle_behavior_format && *toggle_behavior_format)
        {
          g_snprintf (msg_buf[num_msgs], BUF_SIZE, toggle_behavior_format,
                      gimp_get_mod_string (toggle_mask));
        }
      else
        {
          g_strlcpy (msg_buf[num_msgs],
                     gimp_get_mod_string (toggle_mask), BUF_SIZE);
          try = TRUE;
        }

      num_msgs++;
    }

  if (modifiers & GDK_MOD1_MASK)
    {
      if (alt_format && *alt_format)
        {
          g_snprintf (msg_buf[num_msgs], BUF_SIZE, alt_format,
                      gimp_get_mod_string (GDK_MOD1_MASK));
        }
      else
        {
          g_strlcpy (msg_buf[num_msgs],
                     gimp_get_mod_string (GDK_MOD1_MASK), BUF_SIZE);
          try = TRUE;
        }

      num_msgs++;
    }

  /* This convoluted way to build the message using multiple format strings
   * tries to make the messages easier to translate to other languages.
   */

  switch (num_msgs)
    {
    case 1:
      return g_strdup_printf (try ? _("%s (try %s)") : _("%s (%s)"),
                              message, msg_buf[0]);

    case 2:
      return g_strdup_printf (_("%s (try %s, %s)"),
                              message, msg_buf[0], msg_buf[1]);

    case 3:
      return g_strdup_printf (_("%s (try %s, %s, %s)"),
                              message, msg_buf[0], msg_buf[1], msg_buf[2]);
    }

  return g_strdup (message);
}
#undef BUF_SIZE

GimpChannelOps
gimp_modifiers_to_channel_op (GdkModifierType  modifiers)
{
  GdkModifierType extend_mask = gimp_get_extend_selection_mask ();
  GdkModifierType modify_mask = gimp_get_modify_selection_mask ();

  if (modifiers & extend_mask)
    {
      if (modifiers & modify_mask)
        {
          return GIMP_CHANNEL_OP_INTERSECT;
        }
      else
        {
          return GIMP_CHANNEL_OP_ADD;
        }
    }
  else if (modifiers & modify_mask)
    {
      return GIMP_CHANNEL_OP_SUBTRACT;
    }

  return GIMP_CHANNEL_OP_REPLACE;
}

GdkModifierType
gimp_replace_virtual_modifiers (GdkModifierType modifiers)
{
  GdkDisplay      *display = gdk_display_get_default ();
  GdkModifierType  result  = 0;
  gint             i;

  for (i = 0; i < 8; i++)
    {
      GdkModifierType real = 1 << i;

      if (modifiers & real)
        {
          GdkModifierType virtual = real;

          gdk_keymap_add_virtual_modifiers (gdk_keymap_get_for_display (display),
                                            &virtual);

          if (virtual == real)
            result |= virtual;
          else
            result |= virtual & ~real;
        }
    }

  return result;
}

GdkModifierType
gimp_get_primary_accelerator_mask (void)
{
  GdkDisplay *display = gdk_display_get_default ();

  return gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                       GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR);
}

GdkModifierType
gimp_get_extend_selection_mask (void)
{
  GdkDisplay *display = gdk_display_get_default ();

  return gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                       GDK_MODIFIER_INTENT_EXTEND_SELECTION);
}

GdkModifierType
gimp_get_modify_selection_mask (void)
{
  GdkDisplay *display = gdk_display_get_default ();

  return gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                       GDK_MODIFIER_INTENT_MODIFY_SELECTION);
}

GdkModifierType
gimp_get_toggle_behavior_mask (void)
{
  GdkDisplay *display = gdk_display_get_default ();

  /* use the modify selection modifier */
  return gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                       GDK_MODIFIER_INTENT_MODIFY_SELECTION);
}

GdkModifierType
gimp_get_constrain_behavior_mask (void)
{
  GdkDisplay *display = gdk_display_get_default ();

  /* use the modify selection modifier */
  return gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                       GDK_MODIFIER_INTENT_MODIFY_SELECTION);
}

GdkModifierType
gimp_get_all_modifiers_mask (void)
{
  GdkDisplay *display = gdk_display_get_default ();

  return (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK |
          gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                        GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR) |
          gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                        GDK_MODIFIER_INTENT_EXTEND_SELECTION) |
          gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                        GDK_MODIFIER_INTENT_MODIFY_SELECTION));
}

/**
 * gimp_get_monitor_resolution:
 * @screen: a #GdkScreen
 * @monitor: a monitor number
 * @xres: returns the horizontal monitor resolution (in dpi)
 * @yres: returns the vertical monitor resolution (in dpi)
 *
 * Retrieves the monitor's resolution from GDK.
 **/
void
gimp_get_monitor_resolution (GdkMonitor *monitor,
                             gdouble    *xres,
                             gdouble    *yres)
{
  GdkRectangle size_pixels;
  gint         width_mm, height_mm;
  gdouble      x = 0.0;
  gdouble      y = 0.0;
#ifdef PLATFORM_OSX
  CGSize       size;
#endif

  g_return_if_fail (GDK_IS_MONITOR (monitor));
  g_return_if_fail (xres != NULL);
  g_return_if_fail (yres != NULL);

#ifndef PLATFORM_OSX
  gdk_monitor_get_geometry (monitor, &size_pixels);

  width_mm  = gdk_monitor_get_width_mm  (monitor);
  height_mm = gdk_monitor_get_height_mm (monitor);
#else
  width_mm  = 0;
  height_mm = 0;
  size = CGDisplayScreenSize (kCGDirectMainDisplay);
  if (!CGSizeEqualToSize (size, CGSizeZero))
    {
      width_mm  = size.width;
      height_mm = size.height;
    }
  size_pixels.width  = CGDisplayPixelsWide (kCGDirectMainDisplay);
  size_pixels.height = CGDisplayPixelsHigh (kCGDirectMainDisplay);
#endif
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
      x = (size_pixels.width  * 25.4) / (gdouble) width_mm;
      y = (size_pixels.height * 25.4) / (gdouble) height_mm;
    }

  if (x < GIMP_MIN_RESOLUTION || x > GIMP_MAX_RESOLUTION ||
      y < GIMP_MIN_RESOLUTION || y > GIMP_MAX_RESOLUTION)
    {
      g_printerr ("gimp_get_monitor_resolution(): GDK returned bogus "
                  "values for the monitor resolution, using 96 dpi instead.\n");

      x = 96.0;
      y = 96.0;
    }

  /*  round the value to full integers to give more pleasant results  */
  *xres = ROUND (x);
  *yres = ROUND (y);
}

gboolean
gimp_get_style_color (GtkWidget   *widget,
                      const gchar *property_name,
                      GdkRGBA     *color)
{
  GdkRGBA *c = NULL;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (property_name != NULL, FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  gtk_widget_style_get (widget,
                        property_name, &c,
                        NULL);

  if (c)
    {
      *color = *c;
      gdk_rgba_free (c);

      return TRUE;
    }

  /* return ugly magenta to indicate that something is wrong */
  color->red   = 1.0;
  color->green = 1.0;
  color->blue  = 0.0;
  color->alpha = 1.0;

  return FALSE;
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

/* similar to what we have in libgimp/gimpui.c */
static GdkWindow *
gimp_get_foreign_window (gpointer window)
{
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    return gdk_x11_window_foreign_new_for_display (gdk_display_get_default (),
                                                   (Window) window);
#endif

#ifdef GDK_WINDOWING_WIN32
  return gdk_win32_window_foreign_new_for_display (gdk_display_get_default (),
                                                   (HWND) window);
#endif

  return NULL;
}

void
gimp_window_set_transient_for (GtkWindow    *window,
                               GimpProgress *parent)
{
  g_signal_connect_after (window, "map-event",
                          G_CALLBACK (gimp_window_transient_on_mapped),
                          parent);

  if (gtk_widget_get_mapped (GTK_WIDGET (window)))
    gimp_window_transient_on_mapped (GTK_WIDGET (window), NULL, parent);
}

void
gimp_window_set_transient_for_handle (GtkWindow *window,
                                      GBytes    *handle)
{
  g_return_if_fail (GTK_IS_WINDOW (window));
  g_return_if_fail (handle != NULL);

  g_signal_connect_data (window, "map-event",
                         G_CALLBACK (gimp_window_set_transient_cb),
                         g_bytes_ref (handle),
                         (GClosureNotify) g_bytes_unref,
                         G_CONNECT_AFTER);

  if (gtk_widget_get_mapped (GTK_WIDGET (window)))
    gimp_window_set_transient_cb (GTK_WIDGET (window), NULL, handle);
}

static void
gimp_widget_accels_changed (GimpAction   *action,
                            const gchar **accels,
                            GtkWidget    *widget)
{
  const gchar *tooltip;
  const gchar *help_id;

  tooltip = gimp_action_get_tooltip (action);
  help_id = gimp_action_get_help_id (action);

  if (accels && accels[0])
    {
      gchar *escaped = g_markup_escape_text (tooltip, -1);
      gchar *tmp     = g_strdup_printf ("%s  <b>%s</b>", escaped, accels[0]);

      g_free (escaped);

      gimp_help_set_help_data_with_markup (widget, tmp, help_id);
      g_free (tmp);
    }
  else
    {
      gimp_help_set_help_data (widget, tooltip, help_id);
    }
}

static void   gimp_accel_help_widget_weak_notify (gpointer  accel_group,
                                                  GObject  *where_widget_was);

static void
gimp_accel_help_accel_group_weak_notify (gpointer  widget,
                                         GObject  *where_action_was)
{
  g_object_weak_unref (widget,
                       gimp_accel_help_widget_weak_notify,
                       where_action_was);

  g_object_set_data (widget, "gimp-accel-help-action", NULL);
}

static void
gimp_accel_help_widget_weak_notify (gpointer  action,
                                    GObject  *where_widget_was)
{
  g_object_weak_unref (action,
                       gimp_accel_help_accel_group_weak_notify,
                       where_widget_was);
}

void
gimp_widget_set_accel_help (GtkWidget  *widget,
                            GimpAction *action)
{
  GimpAction *prev_action;

  prev_action = g_object_get_data (G_OBJECT (widget), "gimp-accel-help-action");

  if (prev_action)
    {
      g_signal_handlers_disconnect_by_func (prev_action,
                                            gimp_widget_accels_changed,
                                            widget);
      g_object_weak_unref (G_OBJECT (prev_action),
                           gimp_accel_help_accel_group_weak_notify,
                           widget);
      g_object_weak_unref (G_OBJECT (widget),
                           gimp_accel_help_widget_weak_notify,
                           prev_action);
      g_object_set_data (G_OBJECT (widget), "gimp-accel-help-action", NULL);
    }

  if (action)
    {
      gchar **accels;

      g_object_set_data (G_OBJECT (widget), "gimp-accel-help-action",
                         action);
      g_object_weak_ref (G_OBJECT (action),
                         gimp_accel_help_accel_group_weak_notify,
                         widget);
      g_object_weak_ref (G_OBJECT (widget),
                         gimp_accel_help_widget_weak_notify,
                         action);

      g_object_set_data (G_OBJECT (widget), "gimp-accel-action",
                         action);

      g_signal_connect_object (action, "accels-changed",
                               G_CALLBACK (gimp_widget_accels_changed),
                               widget, 0);

      accels = gimp_action_get_display_accels (action);
      gimp_widget_accels_changed (action, (const gchar **) accels, widget);
      g_strfreev (accels);
    }
  else
    {
      gimp_help_set_help_data (widget,
                               gimp_action_get_tooltip (action),
                               gimp_action_get_help_id (action));

    }
}

const gchar *
gimp_get_message_icon_name (GimpMessageSeverity severity)
{
  switch (severity)
    {
    case GIMP_MESSAGE_INFO:
      return GIMP_ICON_DIALOG_INFORMATION;

    case GIMP_MESSAGE_WARNING:
      return GIMP_ICON_DIALOG_WARNING;

    case GIMP_MESSAGE_ERROR:
      return GIMP_ICON_DIALOG_ERROR;

    case GIMP_MESSAGE_BUG_WARNING:
    case GIMP_MESSAGE_BUG_CRITICAL:
      return GIMP_ICON_WILBER_EEK;
    }

  g_return_val_if_reached (GIMP_ICON_DIALOG_WARNING);
}

gboolean
gimp_get_color_tag_color (GimpColorTag  color_tag,
                          GeglColor    *color,
                          gboolean      inherited)
{
  static const struct
  {
    guchar r;
    guchar g;
    guchar b;
  }
  colors[] =
  {
    {    0,   0,   0  }, /* none   */
    {   84, 102, 159  }, /* blue   */
    {  111, 143,  48  }, /* green  */
    {  210, 182,  45  }, /* yellow */
    {  217, 122,  38  }, /* orange */
    {   87,  53,  25  }, /* brown  */
    {  170,  42,  47  }, /* red    */
    {   99,  66, 174  }, /* violet */
    {   87,  87,  87  }  /* gray   */
  };

  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (color_tag < G_N_ELEMENTS (colors), FALSE);

  if (color_tag > GIMP_COLOR_TAG_NONE)
    {
      if (inherited)
        {
          /* Composite color on top of white (20% opacity) */
          gdouble opacity = 0.2f;
          gdouble factor  = 1.0f - opacity;

          gegl_color_set_rgba (color,
                               (colors[color_tag].r / 255.0f) * factor + opacity,
                               (colors[color_tag].g / 255.0f) * factor + opacity,
                               (colors[color_tag].b / 255.0f) * factor + opacity,
                               1.0f);
        }
      else
        {
          gegl_color_set_rgba (color,
                               colors[color_tag].r / 255.0f,
                               colors[color_tag].g / 255.0f,
                               colors[color_tag].b / 255.0f,
                               1.0f);
        }

      return TRUE;
    }

  return FALSE;
}

void
gimp_pango_layout_set_scale (PangoLayout *layout,
                             gdouble      scale)
{
  PangoAttrList  *attrs;
  PangoAttribute *attr;

  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  attrs = pango_attr_list_new ();

  attr = pango_attr_scale_new (scale);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);
}

void
gimp_pango_layout_set_weight (PangoLayout *layout,
                              PangoWeight  weight)
{
  PangoAttrList  *attrs;
  PangoAttribute *attr;

  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  attrs = pango_attr_list_new ();

  attr = pango_attr_weight_new (weight);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);
}

static gboolean
gimp_highlight_widget_draw (GtkWidget *widget,
                            cairo_t   *cr,
                            gpointer   data)
{
  GdkRectangle *rect = (GdkRectangle *) data;

  /* this code is a straight copy of draw_flash() from gtk-inspector's
   * inspect-button.c
   */

  GtkAllocation alloc;

  if (rect)
    {
      alloc.x = rect->x;
      alloc.y = rect->y;
      alloc.width = rect->width;
      alloc.height = rect->height;
    }
  else if (GTK_IS_WINDOW (widget))
    {
      GtkWidget *child = gtk_bin_get_child (GTK_BIN (widget));
      /* We don't want to draw the drag highlight around the
       * CSD window decorations
       */
      if (child == NULL)
        return FALSE;

      gtk_widget_get_allocation (child, &alloc);
    }
  else
    {
      alloc.x = 0;
      alloc.y = 0;
      alloc.width = gtk_widget_get_allocated_width (widget);
      alloc.height = gtk_widget_get_allocated_height (widget);
    }

  cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.2);
  cairo_rectangle (cr,
                   alloc.x + 0.5, alloc.y + 0.5,
                   alloc.width - 1, alloc.height - 1);
  cairo_fill (cr);

  return FALSE;
}

/**
 * gimp_highlight_widget:
 * @widget:
 * @highlight:
 * @rect:
 *
 * Turns highlighting for @widget on or off according to
 * @highlight, in a similar fashion to gtk_drag_highlight()
 * and gtk_drag_unhighlight().
 *
 * If @rect is %NULL, highlight the full widget, otherwise highlight the
 * specific rectangle in widget coordinates.
 * When unhighlighting (i.e. @highlight is %FALSE), the value of @rect
 * doesn't matter, as the previously used rectangle will be reused.
 **/
void
gimp_highlight_widget (GtkWidget    *widget,
                       gboolean      highlight,
                       GdkRectangle *rect)
{
  GdkRectangle *old_rect;
  gboolean      old_highlight;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  highlight = highlight ? TRUE : FALSE;

  old_highlight = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                      "gimp-widget-highlight"));
  old_rect = g_object_get_data (G_OBJECT (widget), "gimp-widget-highlight-rect");

  if (highlight && old_highlight &&
      rect && old_rect && ! gdk_rectangle_equal (rect, old_rect))
    {
      /* Highlight area changed. */
      gimp_highlight_widget (widget, FALSE, NULL);
      old_highlight = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                          "gimp-widget-highlight"));
      old_rect = g_object_get_data (G_OBJECT (widget), "gimp-widget-highlight-rect");
    }

  if (highlight != old_highlight)
    {
      if (highlight)
        {
          GdkRectangle *new_rect = NULL;

          if (rect)
            {
              new_rect = g_new0 (GdkRectangle, 1);
              *new_rect = *rect;
              g_object_set_data_full (G_OBJECT (widget),
                                      "gimp-widget-highlight-rect",
                                      new_rect,
                                      (GDestroyNotify) g_free);
            }
          g_signal_connect_after (widget, "draw",
                                  G_CALLBACK (gimp_highlight_widget_draw),
                                  new_rect);
        }
      else
        {
          if (old_rect)
            {
              g_signal_handlers_disconnect_by_func (widget,
                                                    gimp_highlight_widget_draw,
                                                    old_rect);
              g_object_set_data (G_OBJECT (widget),
                                 "gimp-widget-highlight-rect",
                                 NULL);
            }

          g_signal_handlers_disconnect_by_func (widget,
                                                gimp_highlight_widget_draw,
                                                NULL);
        }

      g_object_set_data (G_OBJECT (widget),
                         "gimp-widget-highlight",
                         GINT_TO_POINTER (highlight));

      gtk_widget_queue_draw (widget);
    }
}

typedef struct
{
  gint          timeout_id;
  gint          counter;
  GdkRectangle *rect;
} WidgetBlink;

static WidgetBlink *
widget_blink_new (void)
{
  WidgetBlink *blink;

  blink = g_slice_new (WidgetBlink);

  blink->timeout_id = 0;
  blink->counter    = 0;
  blink->rect       = NULL;

  return blink;
}

static void
widget_blink_free (WidgetBlink *blink)
{
  if (blink->timeout_id)
    {
      g_source_remove (blink->timeout_id);
      blink->timeout_id = 0;
    }

  if (blink->rect)
    g_slice_free (GdkRectangle, blink->rect);

  g_slice_free (WidgetBlink, blink);
}

static gboolean
gimp_widget_blink_start_timeout (GtkWidget *widget)
{
  WidgetBlink *blink;

  blink = g_object_get_data (G_OBJECT (widget), "gimp-widget-blink");
  if (blink)
    {
      blink->timeout_id = 0;
      gimp_widget_blink (widget);
    }
  else
    {
      /* If the data is not here anymore, our blink has been canceled
       * already. Also delete the script, if any.
       */
      g_object_set_data (G_OBJECT (widget), "gimp-widget-blink-script", NULL);
    }

  return G_SOURCE_REMOVE;
}

static gboolean
gimp_widget_blink_popover_remove (GtkWidget *widget)
{
  gtk_widget_destroy (widget);

  return G_SOURCE_REMOVE;
}

static gboolean
gimp_widget_blink_timeout (GtkWidget *widget)
{
  WidgetBlink *blink;
  GList       *script;

  blink  = g_object_get_data (G_OBJECT (widget), "gimp-widget-blink");
  script = g_object_get_data (G_OBJECT (widget), "gimp-widget-blink-script");

  gimp_highlight_widget (widget, blink->counter % 2 == 1, blink->rect);
  blink->counter++;

  if (blink->counter == 1)
    {
      if (script)
        {
          BlinkStep *step         = script->data;
          gchar     *popover_text = NULL;

          if (step->settings_value)
            {
              const gchar *prop_name;
              GObject     *config;

              prop_name = g_object_get_data (G_OBJECT (widget),
                                             "gimp-widget-property-name");
              config    = g_object_get_data (G_OBJECT (widget),
                                             "gimp-widget-property-config");

              if (config && G_IS_OBJECT (config) && prop_name)
                {
                  GParamSpec  *param_spec;
                  const gchar *nick;

                  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                             prop_name);
                  if (! param_spec)
                    {
                      g_printerr ("%s: %s has no property named '%s'.\n",
                                  G_STRFUNC,
                                  g_type_name (G_TYPE_FROM_INSTANCE (config)),
                                  prop_name);
                      return G_SOURCE_CONTINUE;
                    }
                  if (! (param_spec->flags & G_PARAM_WRITABLE))
                    {
                      g_printerr ("%s: property '%s' of %s is not writable.\n",
                                  G_STRFUNC,
                                  param_spec->name,
                                  g_type_name (param_spec->owner_type));
                      return G_SOURCE_CONTINUE;
                    }

                  nick = g_param_spec_get_nick (param_spec);

                  if (g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), G_TYPE_PARAM_ENUM) ||
                      g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), G_TYPE_PARAM_INT)  ||
                      g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), G_TYPE_PARAM_BOOLEAN))
                    {
                      gchar  *endptr;
                      gint64  enum_value;

                      enum_value = g_ascii_strtoll (step->settings_value, &endptr, 10);
                      if (enum_value == 0 && endptr == step->settings_value)
                        {
                          g_printerr ("%s: settings value '%s' cannot properly be converted to int.\n",
                                      G_STRFUNC, step->settings_value);
                          return G_SOURCE_CONTINUE;
                        }

                      g_object_set (config,
                                    prop_name, enum_value,
                                    NULL);

                      if (nick)
                        {
                          if (g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), G_TYPE_PARAM_BOOLEAN))
                            {
                              if ((gboolean) enum_value)
                                /* TRANSLATORS: the %s will be replaced
                                 * by the localized label of a boolean settings
                                 * (e.g. a checkbox settings) displayed
                                 * in some dockable GUI.
                                 */
                                popover_text = g_strdup_printf (_("Switch \"%s\" ON"), nick);
                              else
                                popover_text = g_strdup_printf (_("Switch \"%s\" OFF"), nick);
                            }
                          else if (g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), G_TYPE_PARAM_ENUM))
                            {
                              GParamSpecEnum   *pspec_enum = (GParamSpecEnum *) param_spec;
                              const GEnumValue *genum_value;

                              genum_value = g_enum_get_value (pspec_enum->enum_class, enum_value);
                              if (genum_value)
                                {
                                  const gchar *enum_desc;

                                  enum_desc = gimp_enum_value_get_desc (pspec_enum->enum_class, genum_value);
                                  if (enum_desc)
                                    /* TRANSLATORS: the %s will be replaced
                                     * by the localized label of a
                                     * multi-choice settings displayed
                                     * in some dockable GUI.
                                     */
                                    popover_text = g_strdup_printf (_("Select \"%s\""), enum_desc);
                                }
                            }
                        }
                    }
                  else
                    {
                      g_printerr ("%s: currently unsupported type '%s' for property %s of %s.\n",
                                  G_STRFUNC,
                                  g_type_name (G_TYPE_FROM_INSTANCE (param_spec)),
                                  param_spec->name,
                                  g_type_name (param_spec->owner_type));
                    }
                }
            }
          else if (GIMP_IS_TOOL_BUTTON (widget))
            {
              GimpToolInfo *info;

              info = gimp_tool_button_get_tool_info (GIMP_TOOL_BUTTON (widget));
              /* TRANSLATORS: %s will be a tool name, so we'll get a
               * final string looking like 'Activate the "Bucket fill" tool'.
               */
              popover_text = g_strdup_printf (_("Activate the \"%s\" tool"),
                                              info->label);
            }

          if (popover_text != NULL)
            {
              GtkWidget *popover = gtk_popover_new (widget);
              GtkWidget *label   = gtk_label_new (popover_text);

              gtk_container_add (GTK_CONTAINER (popover), label);
              gtk_widget_show (label);
              gtk_widget_show (popover);

              g_timeout_add (1200,
                             (GSourceFunc) gimp_widget_blink_popover_remove,
                             popover);
              g_free (popover_text);
            }
        }
    }
  else if (blink->counter == 3)
    {
      blink->timeout_id = 0;

      g_object_set_data (G_OBJECT (widget), "gimp-widget-blink", NULL);

      if (script)
        {
          if (script->next)
            {
              BlinkStep *next_step   = script->next->data;
              GtkWidget *next_widget = next_step->widget;

              g_object_set_data_full (G_OBJECT (next_widget), "gimp-widget-blink-script",
                                      script->next,
                                      (GDestroyNotify) gimp_blink_free_script);
              script->next->prev = NULL;
              script->next       = NULL;

              gimp_widget_blink_after (next_widget, 800);
            }

          g_object_set_data (G_OBJECT (widget), "gimp-widget-blink-script", NULL);
        }

      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}

void
gimp_widget_blink (GtkWidget *widget)
{
  WidgetBlink *blink;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_widget_blink_cancel (widget);

  blink = widget_blink_new ();

  g_object_set_data_full (G_OBJECT (widget), "gimp-widget-blink", blink,
                          (GDestroyNotify) widget_blink_free);

  blink->timeout_id = g_timeout_add (150,
                                     (GSourceFunc) gimp_widget_blink_timeout,
                                     widget);

  gimp_highlight_widget (widget, TRUE, NULL);

  while ((widget = gtk_widget_get_parent (widget)))
    gimp_widget_blink_cancel (widget);
}

void
gimp_widget_script_blink (GtkWidget    *widget,
                          const gchar  *settings_value,
                          GList       **blink_scenario)
{
  BlinkStep *step;

  step = g_slice_new (BlinkStep);
  step->widget         = widget;
  step->settings_value = g_strdup (settings_value);

  *blink_scenario = g_list_append (*blink_scenario, step);

  while ((widget = gtk_widget_get_parent (widget)))
    gimp_widget_blink_cancel (widget);
}

/* gimp_blink_play_script:
 * @blink_scenario:
 *
 * This function will play the @blink_scenario and free the associated
 * data once done.
 */
void
gimp_blink_play_script (GList *blink_scenario)
{
  BlinkStep *step;

  g_return_if_fail (g_list_length (blink_scenario) > 0);

  step = blink_scenario->data;

  g_object_set_data_full (G_OBJECT (step->widget),
                          "gimp-widget-blink-script",
                          blink_scenario,
                          (GDestroyNotify) gimp_blink_free_script);
  gimp_widget_blink (step->widget);
}

void
gimp_widget_blink_rect (GtkWidget    *widget,
                        GdkRectangle *rect)
{
  WidgetBlink *blink;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_widget_blink_cancel (widget);

  blink          = widget_blink_new ();
  blink->rect    = g_slice_new (GdkRectangle);
  *(blink->rect) = *rect;

  g_object_set_data_full (G_OBJECT (widget), "gimp-widget-blink", blink,
                          (GDestroyNotify) widget_blink_free);

  blink->timeout_id = g_timeout_add (150,
                                     (GSourceFunc) gimp_widget_blink_timeout,
                                     widget);

  gimp_highlight_widget (widget, TRUE, blink->rect);

  while ((widget = gtk_widget_get_parent (widget)))
    gimp_widget_blink_cancel (widget);
}

void
gimp_widget_blink_cancel (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (g_object_get_data (G_OBJECT (widget), "gimp-widget-blink"))
    {
      gimp_highlight_widget (widget, FALSE, NULL);

      g_object_set_data (G_OBJECT (widget), "gimp-widget-blink", NULL);
    }
}

/**
 * gimp_blink_toolbox:
 * @gimp:
 * @action_name:
 * @blink_scenario:
 *
 * This is similar to gimp_blink_dockable() for the toolbox
 * specifically. What it will do, additionally to blink the tool button,
 * is first to activate it.
 *
 * Also the identifier is easy as it is simply the action name.
 */
void
gimp_blink_toolbox (Gimp         *gimp,
                    const gchar  *action_name,
                    GList       **blink_scenario)
{
  GimpUIManager *ui_manager;
  GtkWidget     *toolbox;

  /* As a special case, for the toolbox, we don't just raise it,
   * we also select the tool if one was requested.
   */
  toolbox = gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (gimp)),
                                                       gimp,
                                                       gimp_dialog_factory_get_singleton (),
                                                       gimp_get_monitor_at_pointer (),
                                                       "gimp-toolbox");
  /* Find and activate the tool. */
  if (toolbox && action_name != NULL &&
      (ui_manager = gimp_dock_get_ui_manager (GIMP_DOCK (toolbox))))
    {
      GimpAction *action;

      action = gimp_ui_manager_find_action (ui_manager, "tools", action_name);
      gimp_action_activate (GIMP_ACTION (action));
    }
  gimp_blink_dockable (gimp, "gimp-toolbox", action_name, NULL, blink_scenario);
}

/**
 * gimp_blink_dockable:
 * @gimp:
 * @dockable_identifier:
 * @widget_identifier:
 * @settings_value:
 * @blink_scenario:
 *
 * This function will raise the dockable identified by
 * @dockable_identifier. The list of dockable identifiers can be found
 * in `app/dialogs/dialogs.c`.
 *
 * Then it will find the widget identified by @widget_identifier. Note
 * that for propwidgets, this is usually the associated property name.
 *
 * If @settings_value is set, then it will try to change the widget
 * value, depending the type of widgets. Note that for now, only
 * property widgets of enum, int or boolean types can be set (so the
 * @settings_value string must represent an int value).
 *
 * Finally it will either blink this widget immediately to raise
 * attention, or add it to the @blink_scenario if not %NULL. The blink
 * scenario must be explicitly started with gimp_blink_play_script()
 * when ready. @blink_scenario should be considered as opaque data, so
 * you should not free it directly and let gimp_blink_play_script() do
 * so for you.
 */
void
gimp_blink_dockable (Gimp         *gimp,
                     const gchar  *dockable_identifier,
                     const gchar  *widget_identifier,
                     const gchar  *settings_value,
                     GList       **blink_scenario)
{
  GtkWidget   *dockable;
  GdkMonitor  *monitor;
  BlinkSearch *data;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  monitor = gimp_get_monitor_at_pointer ();

  dockable = gimp_window_strategy_show_dockable_dialog (
    GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (gimp)),
    gimp,
    gimp_dialog_factory_get_singleton (),
    monitor,
    dockable_identifier);

  if (! dockable)
    return;

  if (widget_identifier)
    {
      data = g_slice_new (BlinkSearch);
      data->blink_script      = blink_scenario;
      data->widget_identifier = widget_identifier;
      data->settings_value    = settings_value;
      gtk_container_foreach (GTK_CONTAINER (dockable),
                             (GtkCallback) gimp_search_widget_rec,
                             (gpointer) data);
      g_slice_free (BlinkSearch, data);
    }
}

/**
 * gimp_dock_with_window_new:
 * @factory: a #GimpDialogFacotry
 * @monitor: the #GdkMonitor the dock window should appear on
 * @toolbox: if %TRUE; gives a "gimp-toolbox-window" with a
 *           "gimp-toolbox", "gimp-dock-window"+"gimp-dock"
 *           otherwise
 *
 * Returns: the newly created #GimpDock with the #GimpDockWindow
 **/
GtkWidget *
gimp_dock_with_window_new (GimpDialogFactory *factory,
                           GdkMonitor        *monitor,
                           gboolean           toolbox)
{
  GtkWidget         *dock_window;
  GimpDockContainer *dock_container;
  GtkWidget         *dock;
  GimpUIManager     *ui_manager;

  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  /* Create a dock window to put the dock in. We need to create the
   * dock window before the dock because the dock has a dependency to
   * the ui manager in the dock window
   */
  dock_window = gimp_dialog_factory_dialog_new (factory, monitor,
                                                NULL /*ui_manager*/,
                                                NULL,
                                                (toolbox ?
                                                 "gimp-toolbox-window" :
                                                 "gimp-dock-window"),
                                                -1 /*view_size*/,
                                                FALSE /*present*/);

  dock_container = GIMP_DOCK_CONTAINER (dock_window);
  ui_manager     = gimp_dock_container_get_ui_manager (dock_container);
  dock           = gimp_dialog_factory_dialog_new (factory, monitor,
                                                   ui_manager,
                                                   dock_window,
                                                   (toolbox ?
                                                    "gimp-toolbox" :
                                                    "gimp-dock"),
                                                   -1 /*view_size*/,
                                                   FALSE /*present*/);

  if (dock)
    gimp_dock_window_add_dock (GIMP_DOCK_WINDOW (dock_window),
                               GIMP_DOCK (dock),
                               -1);

  return dock;
}

GtkWidget *
gimp_tools_get_tool_options_gui (GimpToolOptions *tool_options)
{
  GtkWidget *widget;

  widget = g_object_get_data (G_OBJECT (tool_options),
                              GIMP_TOOL_OPTIONS_GUI_KEY);

  if (! widget)
    {
      GimpToolOptionsGUIFunc func;

      func = g_object_get_data (G_OBJECT (tool_options),
                                GIMP_TOOL_OPTIONS_GUI_FUNC_KEY);

      if (func)
        {
          widget = func (tool_options);

          gimp_tools_set_tool_options_gui (tool_options, widget);
        }
    }

  return widget;
}

void
gimp_tools_set_tool_options_gui (GimpToolOptions *tool_options,
                                 GtkWidget       *widget)
{
  GtkWidget *prev_widget;

  prev_widget = g_object_get_data (G_OBJECT (tool_options),
                                   GIMP_TOOL_OPTIONS_GUI_KEY);

  if (widget == prev_widget)
    return;

  if (prev_widget)
    gtk_widget_destroy (prev_widget);

  g_object_set_data_full (G_OBJECT (tool_options),
                          GIMP_TOOL_OPTIONS_GUI_KEY,
                          widget ? g_object_ref_sink (widget)      : NULL,
                          widget ? (GDestroyNotify) g_object_unref : NULL);
}

void
gimp_tools_set_tool_options_gui_func (GimpToolOptions        *tool_options,
                                      GimpToolOptionsGUIFunc  func)
{
  g_object_set_data (G_OBJECT (tool_options),
                     GIMP_TOOL_OPTIONS_GUI_FUNC_KEY,
                     func);
}

gboolean
gimp_widget_get_fully_opaque (GtkWidget *widget)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  return g_object_get_data (G_OBJECT (widget),
                            "gimp-widget-fully-opaque") != NULL;
}

void
gimp_widget_set_fully_opaque (GtkWidget *widget,
                              gboolean   fully_opaque)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  return g_object_set_data (G_OBJECT (widget),
                            "gimp-widget-fully-opaque",
                            GINT_TO_POINTER (fully_opaque));
}

static void
gimp_gtk_container_clear_callback (GtkWidget    *widget,
                                   GtkContainer *container)
{
  gtk_container_remove (container, widget);
}

void
gimp_gtk_container_clear (GtkContainer *container)
{
  gtk_container_foreach (container,
                         (GtkCallback) gimp_gtk_container_clear_callback,
                         container);
}

void
gimp_button_set_suggested (GtkWidget      *button,
                           gboolean        suggested,
                           GtkReliefStyle  default_relief)
{
  GtkStyleContext *style;

  g_return_if_fail (GTK_IS_BUTTON (button));

  style = gtk_widget_get_style_context (button);

  if (suggested)
    {
      gtk_style_context_add_class (style, "suggested-action");
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);
    }
  else
    {
      gtk_style_context_remove_class (style, "suggested-action");
      gtk_button_set_relief (GTK_BUTTON (button), default_relief);
    }
}

void
gimp_button_set_destructive (GtkWidget      *button,
                             gboolean        destructive,
                             GtkReliefStyle  default_relief)
{
  GtkStyleContext *style;

  g_return_if_fail (GTK_IS_BUTTON (button));

  style = gtk_widget_get_style_context (button);

  if (destructive)
    {
      gtk_style_context_add_class (style, "destructive-action");
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);
    }
  else
    {
      gtk_style_context_remove_class (style, "destructive-action");
      gtk_button_set_relief (GTK_BUTTON (button), default_relief);
    }
}

void
gimp_gtk_adjustment_chain (GtkAdjustment *adjustment1,
                           GtkAdjustment *adjustment2)
{
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment1));
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment2));

  g_object_bind_property (adjustment1, "value",
                          adjustment2, "lower",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (adjustment2, "value",
                          adjustment1, "upper",
                          G_BINDING_SYNC_CREATE);
}

static gboolean
gimp_print_event_free (gpointer data)
{
  g_free (data);

  return FALSE;
}

const gchar *
gimp_print_event (const GdkEvent *event)
{
  gchar *str;
  gchar *tmp;

  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      str = g_strdup_printf ("ENTER_NOTIFY (mode %d)",
                             event->crossing.mode);
      break;

    case GDK_LEAVE_NOTIFY:
      str = g_strdup_printf ("LEAVE_NOTIFY (mode %d)",
                             event->crossing.mode);
      break;

    case GDK_PROXIMITY_IN:
      str = g_strdup ("PROXIMITY_IN");
      break;

    case GDK_PROXIMITY_OUT:
      str = g_strdup ("PROXIMITY_OUT");
      break;

    case GDK_FOCUS_CHANGE:
      if (event->focus_change.in)
        str = g_strdup ("FOCUS_IN");
      else
        str = g_strdup ("FOCUS_OUT");
      break;

    case GDK_BUTTON_PRESS:
      str = g_strdup_printf ("BUTTON_PRESS (%d @ %0.0f:%0.0f)",
                             event->button.button,
                             event->button.x,
                             event->button.y);
      break;

    case GDK_2BUTTON_PRESS:
      str = g_strdup_printf ("2BUTTON_PRESS (%d @ %0.0f:%0.0f)",
                             event->button.button,
                             event->button.x,
                             event->button.y);
      break;

    case GDK_3BUTTON_PRESS:
      str = g_strdup_printf ("3BUTTON_PRESS (%d @ %0.0f:%0.0f)",
                             event->button.button,
                             event->button.x,
                             event->button.y);
      break;

    case GDK_BUTTON_RELEASE:
      str = g_strdup_printf ("BUTTON_RELEASE (%d @ %0.0f:%0.0f)",
                             event->button.button,
                             event->button.x,
                             event->button.y);
      break;

    case GDK_SCROLL:
      str = g_strdup_printf ("SCROLL (%d)",
                             event->scroll.direction);
      break;

    case GDK_MOTION_NOTIFY:
      str = g_strdup_printf ("MOTION_NOTIFY (%0.0f:%0.0f %d)",
                             event->motion.x,
                             event->motion.y,
                             event->motion.time);
      break;

    case GDK_KEY_PRESS:
      str = g_strdup_printf ("KEY_PRESS (%d, %s)",
                             event->key.keyval,
                             gdk_keyval_name (event->key.keyval) ?
                             gdk_keyval_name (event->key.keyval) : "<none>");
      break;

    case GDK_KEY_RELEASE:
      str = g_strdup_printf ("KEY_RELEASE (%d, %s)",
                             event->key.keyval,
                             gdk_keyval_name (event->key.keyval) ?
                             gdk_keyval_name (event->key.keyval) : "<none>");
      break;

    default:
      str = g_strdup_printf ("UNHANDLED (type %d)",
                             event->type);
      break;
    }

  tmp = g_strdup_printf ("%s (device '%s', source device '%s')",
                         str,
                         gdk_device_get_name (gdk_event_get_device (event)),
                         gdk_device_get_name (gdk_event_get_source_device (event)));
  g_free (str);
  str = tmp;

  g_idle_add (gimp_print_event_free, str);

  return str;
}

gboolean
gimp_color_profile_store_add_defaults (GimpColorProfileStore  *store,
                                       GimpColorConfig        *config,
                                       GimpImageBaseType       base_type,
                                       GimpPrecision           precision,
                                       GError                **error)
{
  GimpColorProfile *profile;
  gchar            *label;
  GError           *my_error = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE_STORE (store), FALSE);
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  profile = gimp_babl_get_builtin_color_profile (base_type,
                                                 gimp_babl_trc (precision));

  if (base_type == GIMP_GRAY)
    {
      label = g_strdup_printf (_("Built-in grayscale (%s)"),
                               gimp_color_profile_get_label (profile));

      profile = gimp_color_config_get_gray_color_profile (config, &my_error);
    }
  else
    {
      label = g_strdup_printf (_("Built-in RGB (%s)"),
                               gimp_color_profile_get_label (profile));

      profile = gimp_color_config_get_rgb_color_profile (config, &my_error);
    }

  gimp_color_profile_store_add_file (store, NULL, label);
  g_free (label);

  if (profile)
    {
      gchar *path;
      GFile *file;

      if (base_type == GIMP_GRAY)
        {
          g_object_get (config, "gray-profile", &path, NULL);
          file = gimp_file_new_for_config_path (path, NULL);
          g_free (path);

          label = g_strdup_printf (_("Preferred grayscale (%s)"),
                                   gimp_color_profile_get_label (profile));
        }
      else
        {
          g_object_get (config, "rgb-profile", &path, NULL);
          file = gimp_file_new_for_config_path (path, NULL);
          g_free (path);

          label = g_strdup_printf (_("Preferred RGB (%s)"),
                                   gimp_color_profile_get_label (profile));
        }

      g_object_unref (profile);

      gimp_color_profile_store_add_file (store, file, label);

      g_object_unref (file);
      g_free (label);

      return TRUE;
    }
  else if (my_error)
    {
      g_propagate_error (error, my_error);

      return FALSE;
    }

  return TRUE;
}

static void
connect_path_show (GimpColorProfileChooserDialog *dialog)
{
  GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
  GFile          *file    = gtk_file_chooser_get_file (chooser);

  if (file)
    {
      /*  if something is already selected in this dialog,
       *  leave it alone
       */
      g_object_unref (file);
    }
  else
    {
      GObject     *config;
      const gchar *property;
      gchar       *path = NULL;

      config   = g_object_get_data (G_OBJECT (dialog), "profile-path-config");
      property = g_object_get_data (G_OBJECT (dialog), "profile-path-property");

      g_object_get (config, property, &path, NULL);

      if (path)
        {
          GFile *folder = gimp_file_new_for_config_path (path, NULL);

          if (folder)
            {
              gtk_file_chooser_set_current_folder_file (chooser, folder, NULL);
              g_object_unref (folder);
            }

          g_free (path);
        }
    }
}

static void
connect_path_response (GimpColorProfileChooserDialog *dialog,
                       gint                           response)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
      GFile          *file    = gtk_file_chooser_get_file (chooser);

      if (file)
        {
          GFile *folder = gtk_file_chooser_get_current_folder_file (chooser);

          if (folder)
            {
              GObject     *config;
              const gchar *property;
              gchar       *path = NULL;

              config   = g_object_get_data (G_OBJECT (dialog),
                                            "profile-path-config");
              property = g_object_get_data (G_OBJECT (dialog),
                                            "profile-path-property");

              path = gimp_file_get_config_path (folder, NULL);

              g_object_set (config, property, path, NULL);

              if (path)
                g_free (path);

              g_object_unref (folder);
            }

          g_object_unref (file);
        }
    }
}

void
gimp_color_profile_chooser_dialog_connect_path (GtkWidget   *dialog,
                                                GObject     *config,
                                                const gchar *property_name)
{
  g_return_if_fail (GIMP_IS_COLOR_PROFILE_CHOOSER_DIALOG (dialog));
  g_return_if_fail (G_IS_OBJECT (config));
  g_return_if_fail (property_name != NULL);

  g_object_set_data_full (G_OBJECT (dialog), "profile-path-config",
                          g_object_ref (config),
                          (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (dialog), "profile-path-property",
                          g_strdup (property_name),
                          (GDestroyNotify) g_free);

  g_signal_connect (dialog, "show",
                    G_CALLBACK (connect_path_show),
                    NULL);
  g_signal_connect (dialog, "response",
                    G_CALLBACK (connect_path_response),
                    NULL);
}

void
gimp_widget_flush_expose (void)
{
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
}

void
gimp_make_valid_action_name (gchar *action_name)
{
  /* g_action_name_is_valid() says: "action_name is valid if it consists only of
   * alphanumeric characters, plus ‘-‘ and ‘.’. The empty string is not a valid
   * action name."
   */
  for (gint i = 0; action_name[i] != '\0'; i++)
    {
      if (action_name[i] == '-' || action_name[i] == '.'   ||
          (action_name[i] >= 'a' && action_name[i] <= 'z') ||
          (action_name[i] >= 'A' && action_name[i] <= 'Z') ||
          (action_name[i] >= '0' && action_name[i] <= '9'))
        continue;

      action_name[i] = '-';
    }
}


/*  private functions  */

static void
gimp_widget_blink_after (GtkWidget *widget,
                         gint       ms_timeout)
{
  WidgetBlink *blink;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  blink = widget_blink_new ();

  g_object_set_data_full (G_OBJECT (widget), "gimp-widget-blink", blink,
                          (GDestroyNotify) widget_blink_free);

  blink->timeout_id = g_timeout_add (ms_timeout,
                                     (GSourceFunc) gimp_widget_blink_start_timeout,
                                     widget);
}

static void
gimp_search_widget_rec (GtkWidget   *widget,
                        BlinkSearch *data)
{
  GList       **blink_script   = data->blink_script;
  const gchar  *searched_id    = data->widget_identifier;
  const gchar  *settings_value = data->settings_value;
  const gchar  *id;

  id = g_object_get_data (G_OBJECT (widget),
                          "gimp-widget-identifier");

  if (id == NULL)
    /* Using propwidgets identifiers as fallback. */
    id = g_object_get_data (G_OBJECT (widget),
                            "gimp-widget-property-name");

  if (id && g_strcmp0 (id, searched_id) == 0)
    {
      /* Giving focus to help scrolling the dockable so that the
       * widget is visible. Note that it seems to work fine if the
       * dockable was already present, not if it was just created.
       *
       * TODO: this should be fixed so that we always make the
       * widget visible before blinking, otherwise it's a bit
       * useless when this happens.
       */
      gtk_widget_grab_focus (widget);
      if (blink_script)
        gimp_widget_script_blink (widget, settings_value, blink_script);
      else if (gtk_widget_is_visible (widget))
        gimp_widget_blink (widget);
    }
  else if (GTK_IS_CONTAINER (widget))
    {
      gtk_container_foreach (GTK_CONTAINER (widget),
                             (GtkCallback) gimp_search_widget_rec,
                             (gpointer) data);

      if (GTK_IS_TREE_VIEW (widget))
        {
          GList *columns;

          columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (widget));
          for (GList *iter = columns; iter; iter = iter->next)
            {
              GtkWidget *column_widget;

              column_widget = gtk_tree_view_column_get_widget (iter->data);

              if (column_widget)
                gimp_search_widget_rec (column_widget, data);
            }

          g_list_free (columns);
        }
    }
}

static void
gimp_blink_free_script (GList *blink_scenario)
{
  GList *iter;

  for (iter = blink_scenario; iter; iter = iter->next)
    {
      BlinkStep *step = iter->data;

      g_free (step->settings_value);
      g_slice_free (BlinkStep, step);
    }
  g_list_free (blink_scenario);
}

gchar *
gimp_utils_make_canonical_menu_label (const gchar *path)
{
  gchar **split_path;
  gchar  *canon_path;

  /* The first underscore of each path item is a mnemonic. */
  split_path = g_strsplit (path, "_", 2);
  canon_path = g_strjoinv ("", split_path);
  g_strfreev (split_path);

  return canon_path;
}

/**
 * gimp_utils_break_menu_path:
 * @path:
 * @section_name:
 *
 * Break @path which is in the form "/_some/p_ath" into a GStrv containing "some"
 * and "path", in a canonical way:
 * - Multiple slashes are folded to one (e.g. "//some///path/" and "/some/path"
 *   are equivalent).
 * - End slash or none are equivalent.
 * - Individual path components are canonicalized with
 *   gimp_utils_make_canonical_menu_label(), in particular with mnemonic
 *   underscore removed.
 *
 * Moreover if @section_name is not %NULL, then a path component of the form
 * "[section]" at the end of the path will be removed and "section" will be
 * stored inside @section_name. Furthermore, "[[label]]" or "[[label]" will both
 * be transformed into a component path "[label]", without being removed, as a
 * way to create menu paths containing square brackets.
 */
gchar **
gimp_utils_break_menu_path (const gchar  *path,
                            gchar       **mnemonic_path1,
                            gchar       **section_name)
{
  GRegex   *path_regex;
  gchar   **paths;
  GString  *mnemonic_string = NULL;
  gint      start = 0;

  g_return_val_if_fail (path != NULL, NULL);

  path_regex = g_regex_new ("/+", 0, 0, NULL);

  if (mnemonic_path1 != NULL)
    mnemonic_string = g_string_new (NULL);

  /* Get rid of leading slashes. */
  while (path[start] == '/' && path[start] != '\0')
    start++;

  /* Get rid of empty last item because of trailing slashes. */
  paths = g_regex_split (path_regex, path + start, 0);
  if (g_strv_length (paths) > 0 &&
      strlen (paths[g_strv_length (paths) - 1]) == 0)
    {
      g_free (paths[g_strv_length (paths) - 1]);
      paths[g_strv_length (paths) - 1] = NULL;
    }

  for (int i = 0; paths[i]; i++)
    {
      gchar *canon_path;

      if (section_name && paths[i + 1] == NULL)
        {
          gint path_len;

          path_len = strlen (paths[i]);
          if (path_len > 2 && paths[i][0] == '[' && paths[i][path_len - 1] == ']')
            {
              if (paths[i][1] == '[')
                {
                  if (paths[i][path_len - 2] == ']')
                    paths[i][path_len - 1] = '\0';

                  canon_path = g_strdup (paths[i] + 1);
                  g_free (paths[i]);
                  paths[i] = canon_path;
                }
              else
                {
                  paths[i][path_len - 1] = '\0';
                  *section_name = g_strdup (paths[i] + 1);

                  g_free (paths[i]);
                  paths[i] = NULL;
                  break;
                }
            }
        }

      if (mnemonic_string != NULL)
        g_string_append_printf (mnemonic_string, "/%s", paths[i]);

      canon_path = gimp_utils_make_canonical_menu_label (paths[i]);
      g_free (paths[i]);
      paths[i] = canon_path;
    }

  if (mnemonic_path1 != NULL)
    *mnemonic_path1 = g_string_free (mnemonic_string, FALSE);

  g_regex_unref (path_regex);

  return paths;
}

gboolean
gimp_utils_are_menu_path_identical (const gchar  *path1,
                                    const gchar  *path2,
                                    gchar       **canonical_path1,
                                    gchar       **mnemonic_path1,
                                    gchar       **path1_section_name)
{
  gchar    **paths1;
  gchar    **paths2;
  gboolean   identical = TRUE;

  if (path1 == NULL)
    path1 = "/";
  if (path2 == NULL)
    path2 = "/";

  paths1 = gimp_utils_break_menu_path (path1, mnemonic_path1, path1_section_name);
  paths2 = gimp_utils_break_menu_path (path2, NULL, NULL);
  if (g_strv_length (paths1) != g_strv_length (paths2))
    {
      identical = FALSE;
    }
  else
    {
      for (int i = 0; paths1[i]; i++)
        {
          if (g_strcmp0 (paths1[i], paths2[i]) != 0)
            {
              identical = FALSE;
              break;
            }
        }
    }

  if (canonical_path1)
    {
      gchar *joined = g_strjoinv ("/", paths1);

      *canonical_path1 = g_strdup_printf ("/%s", joined);

      g_free (joined);
    }

  g_strfreev (paths1);
  g_strfreev (paths2);

  return identical;
}

static gboolean
gimp_window_transient_on_mapped (GtkWidget    *window,
                                 GdkEventAny  *event G_GNUC_UNUSED,
                                 GimpProgress *progress)
{
  GBytes *handle;

  handle = gimp_progress_get_window_id (progress);

  if (handle == NULL)
    return FALSE;

  gimp_window_set_transient_cb (window, NULL, handle);

  g_bytes_unref (handle);

  return FALSE;
}

static void
gimp_window_set_transient_cb (GtkWidget   *window,
                              GdkEventAny *event G_GNUC_UNUSED,
                              GBytes      *handle)
{
  gboolean transient_set = FALSE;

  g_return_if_fail (handle != NULL);

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ()))
    {
      char *wayland_handle;

      wayland_handle = (char *) g_bytes_get_data (handle, NULL);
      gdk_wayland_window_set_transient_for_exported (gtk_widget_get_window (window),
                                                     wayland_handle);
      transient_set = TRUE;
    }
#endif

#ifdef GDK_WINDOWING_X11
  if (! transient_set && GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    {
      GdkWindow *parent;
      Window    *handle_data;
      Window     parent_ID;
      gsize      handle_size;

      handle_data = (Window *) g_bytes_get_data (handle, &handle_size);
      g_return_if_fail (handle_size == sizeof (Window));
      parent_ID = *handle_data;

      parent = gimp_get_foreign_window ((gpointer) parent_ID);

      if (parent)
        gdk_window_set_transient_for (gtk_widget_get_window (window), parent);

      transient_set = TRUE;
    }
#endif
  /* Cross-process transient-for is broken in gdk/win32. It causes hangs of the
   * main process and we still don't know why.
   * If it eventually is fixed to actually work, change this to a run-time check
   * of GTK+ version. Remember to change also gimp_window_transient_on_mapped()
   * in libgimp/gimpui.c
   *
   * Note: this hanging bug is still happening with GTK+3 as of mid-2023 with
   * steps described in comment 4 in:
   * https://bugzilla.gnome.org/show_bug.cgi?id=359538
   */
#if 0 && defined (GDK_WINDOWING_WIN32)
  if (! transient_set)
    {
      GdkWindow *parent;
      HANDLE    *handle_data;
      HANDLE     parent_ID;
      gsize      handle_size;

      handle_data = (HANDLE *) g_bytes_get_data (handle, &handle_size);
      g_return_if_fail (handle_size == sizeof (HANDLE));
      parent_ID = *handle_data;

      parent = gimp_get_foreign_window ((gpointer) parent_ID);

      if (parent)
        gdk_window_set_transient_for (gtk_widget_get_window (window), parent);

      transient_set = TRUE;
    }
#endif
}

#ifdef G_OS_WIN32
void
gimp_window_set_title_bar_theme (Gimp      *gimp,
                                 GtkWidget *dialog)
{
  HWND           hwnd;
  GdkWindow     *window        = NULL;
  gboolean       use_dark_mode = FALSE;

  window = gtk_widget_get_window (GTK_WIDGET (dialog));
  if (window)
    {
      if (gimp)
        {
          GimpGuiConfig *config;

          config = GIMP_GUI_CONFIG (gimp->config);
          use_dark_mode = (config->theme_scheme != GIMP_THEME_LIGHT);
        }
      else
        {
          GtkStyleContext *style;
          GdkRGBA         *color = NULL;

          /* Workaround if we don't have access to GimpGuiConfig.
           * If the background color is below the threshold, then we're
           * likely in dark mode.
           */
          style = gtk_widget_get_style_context (dialog);
          gtk_style_context_get (style, gtk_style_context_get_state (style),
                                 GTK_STYLE_PROPERTY_BACKGROUND_COLOR, &color,
                                 NULL);
          if (color)
            {
              if (color->red < 0.5 && color->green < 0.5 && color->blue < 0.5)
                use_dark_mode = TRUE;

              gdk_rgba_free (color);
            }
        }

        hwnd = (HWND) gdk_win32_window_get_handle (window);
        DwmSetWindowAttribute (hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                               &use_dark_mode, sizeof (use_dark_mode));
    }
}
#endif
