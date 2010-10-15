/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimp3migration.c
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <math.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimp3migration.h"


#if 0

GtkWidget *
gtk_box_new (GtkOrientation  orientation,
             gint            spacing)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    return gtk_hbox_new (FALSE, spacing);
  else
    return gtk_vbox_new (FALSE, spacing);
}

GtkWidget *
gtk_button_box_new (GtkOrientation  orientation)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    return gtk_hbutton_box_new ();
  else
    return gtk_vbutton_box_new ();
}

GtkWidget *
gtk_paned_new (GtkOrientation  orientation)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    return gtk_hpaned_new ();
  else
    return gtk_vpaned_new ();
}

GtkWidget *
gtk_scale_new (GtkOrientation  orientation,
               GtkAdjustment  *adjustment)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    return gtk_hscale_new (adjustment);
  else
    return gtk_vscale_new (adjustment);
}

GtkWidget *
gtk_scrollbar_new (GtkOrientation  orientation,
                   GtkAdjustment  *adjustment)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    return gtk_hscrollbar_new (adjustment);
  else
    return gtk_vscrollbar_new (adjustment);
}

GtkWidget *
gtk_separator_new (GtkOrientation  orientation)
{
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    return gtk_hseparator_new ();
  else
    return gtk_vseparator_new ();
}

#endif /* if 0 */


#if ! GTK_CHECK_VERSION (3, 3, 0)

gboolean
gdk_event_triggers_context_menu (const GdkEvent *event)
{
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *bevent = (GdkEventButton *) event;

      if (bevent->button == 3 &&
          ! (bevent->state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK)))
        return TRUE;

#ifdef GDK_WINDOWING_QUARTZ
      if (bevent->button == 1 &&
          ! (bevent->state & (GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)) &&
          (bevent->state & GDK_CONTROL_MASK))
        return TRUE;
#endif
    }

  return FALSE;
}

GdkModifierType
gdk_keymap_get_modifier_mask (GdkKeymap         *keymap,
                              GdkModifierIntent  intent)
{
  g_return_val_if_fail (GDK_IS_KEYMAP (keymap), 0);

#ifdef GDK_WINDOWING_QUARTZ
  switch (intent)
    {
    case GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR:
      return GDK_MOD2_MASK;

    case GDK_MODIFIER_INTENT_CONTEXT_MENU:
      return GDK_CONTROL_MASK;

    case GDK_MODIFIER_INTENT_EXTEND_SELECTION:
      return GDK_SHIFT_MASK;

    case GDK_MODIFIER_INTENT_MODIFY_SELECTION:
      return GDK_MOD2_MASK;

    case GDK_MODIFIER_INTENT_NO_TEXT_INPUT:
      return GDK_MOD2_MASK | GDK_CONTROL_MASK;

    default:
      g_return_val_if_reached (0);
    }
#else
  switch (intent)
    {
    case GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR:
      return GDK_CONTROL_MASK;

    case GDK_MODIFIER_INTENT_CONTEXT_MENU:
      return 0;

    case GDK_MODIFIER_INTENT_EXTEND_SELECTION:
      return GDK_SHIFT_MASK;

    case GDK_MODIFIER_INTENT_MODIFY_SELECTION:
      return GDK_CONTROL_MASK;

    case GDK_MODIFIER_INTENT_NO_TEXT_INPUT:
      return GDK_MOD1_MASK | GDK_CONTROL_MASK;

    default:
      g_return_val_if_reached (0);
    }
#endif
}

GdkModifierType
gtk_widget_get_modifier_mask (GtkWidget         *widget,
                              GdkModifierIntent  intent)
{
  GdkDisplay *display;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), 0);

  display = gtk_widget_get_display (widget);

  return gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                       intent);
}

#endif /* GTK+ 3.3 */

gboolean
gdk_cairo_get_clip_rectangle (cairo_t      *cr,
                              GdkRectangle *rect)
{
  double x1, y1, x2, y2;
  gboolean clip_exists;

  cairo_clip_extents (cr, &x1, &y1, &x2, &y2);

  clip_exists = x1 < x2 && y1 < y2;

  if (rect)
    {
      x1 = floor (x1);
      y1 = floor (y1);
      x2 = ceil (x2);
      y2 = ceil (y2);

      rect->x      = CLAMP (x1,      G_MININT, G_MAXINT);
      rect->y      = CLAMP (y1,      G_MININT, G_MAXINT);
      rect->width  = CLAMP (x2 - x1, G_MININT, G_MAXINT);
      rect->height = CLAMP (y2 - y1, G_MININT, G_MAXINT);
    }

  return clip_exists;
}

void
gdk_screen_get_monitor_workarea (GdkScreen    *screen,
                                 gint          monitor_num,
                                 GdkRectangle *dest)
{
  gdk_screen_get_monitor_geometry (screen, monitor_num, dest);
}

void
gtk_label_set_xalign (GtkLabel *label,
                      gfloat    xalign)
{
  g_return_if_fail (GTK_IS_LABEL (label));

  xalign = CLAMP (xalign, 0.0, 1.0);

  g_object_set (label, "xalign", xalign, NULL);
}

gfloat
gtk_label_get_xalign (GtkLabel *label)
{
  gfloat xalign;

  g_return_val_if_fail (GTK_IS_LABEL (label), 0.5);

  g_object_get (label, "xalign", &xalign, NULL);

  return xalign;
}

void
gtk_label_set_yalign (GtkLabel *label,
                      gfloat    yalign)
{
  g_return_if_fail (GTK_IS_LABEL (label));

  yalign = CLAMP (yalign, 0.0, 1.0);

  g_object_set (label, "yalign", yalign, NULL);
}

gfloat
gtk_label_get_yalign (GtkLabel *label)
{
  gfloat yalign;

  g_return_val_if_fail (GTK_IS_LABEL (label), 0.5);

  g_object_get (label, "yalign", &yalign, NULL);

  return yalign;
}
