/* GIMP Wheel ColorSelector
 * Copyright (C) 2008  Michael Natterer <mitch@gimp.org>
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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


#ifdef __GNUC__
#warning FIXME: remove hacks here as soon as we depend on GTK 2.14
#endif

#ifndef __GTK_HSV_H__

#define GTK_TYPE_HSV (gtk_hsv_get_type ())
#define GTK_HSV(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_HSV, GtkHSV))

typedef struct _GtkHSV      GtkHSV;

GType      gtk_hsv_get_type     (void) G_GNUC_CONST;
GtkWidget* gtk_hsv_new          (void);
void       gtk_hsv_set_color    (GtkHSV    *hsv,
                                 double     h,
                                 double     s,
                                 double     v);
void       gtk_hsv_get_color    (GtkHSV    *hsv,
                                 gdouble   *h,
                                 gdouble   *s,
                                 gdouble   *v);
void       gtk_hsv_set_metrics  (GtkHSV    *hsv,
                                 gint       size,
                                 gint       ring_width);

#endif /* __GTK_HSV_H__ */


#define COLORSEL_TYPE_WHEEL            (colorsel_wheel_get_type ())
#define COLORSEL_WHEEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLORSEL_TYPE_WHEEL, ColorselWheel))
#define COLORSEL_WHEEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), COLORSEL_TYPE_WHEEL, ColorselWheelClass))
#define COLORSEL_IS_WHEEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLORSEL_TYPE_WHEEL))
#define COLORSEL_IS_WHEEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), COLORSEL_TYPE_WHEEL))


typedef struct _ColorselWheel      ColorselWheel;
typedef struct _ColorselWheelClass ColorselWheelClass;

struct _ColorselWheel
{
  GimpColorSelector  parent_instance;

  GtkWidget         *hsv;
};

struct _ColorselWheelClass
{
  GimpColorSelectorClass  parent_class;
};


GType         colorsel_wheel_get_type      (void);

static void   colorsel_wheel_set_color     (GimpColorSelector *selector,
                                            const GimpRGB     *rgb,
                                            const GimpHSV     *hsv);

static void   colorsel_wheel_size_allocate (GtkWidget         *frame,
                                            GtkAllocation     *allocation,
                                            ColorselWheel     *wheel);
static void   colorsel_wheel_size_request  (GtkWidget         *dont_use,
                                            GtkRequisition    *requisition,
                                            ColorselWheel     *wheel);
static void   colorsel_wheel_changed       (GtkHSV            *hsv,
                                            GimpColorSelector *selector);

static const GimpModuleInfo colorsel_wheel_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("HSV color wheel"),
  "Michael Natterer <mitch@gimp.org>",
  "v1.0",
  "(c) 2008, released under the GPL",
  "08 Aug 2008"
};


G_DEFINE_DYNAMIC_TYPE (ColorselWheel, colorsel_wheel,
                       GIMP_TYPE_COLOR_SELECTOR)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &colorsel_wheel_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  colorsel_wheel_register_type (module);

  return TRUE;
}

static void
colorsel_wheel_class_init (ColorselWheelClass *klass)
{
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  selector_class->name      = _("Wheel");
  selector_class->help_id   = "gimp-colorselector-triangle";
  selector_class->stock_id  = GIMP_STOCK_COLOR_TRIANGLE;
  selector_class->set_color = colorsel_wheel_set_color;
}

static void
colorsel_wheel_class_finalize (ColorselWheelClass *klass)
{
}

static void
colorsel_wheel_init (ColorselWheel *wheel)
{
  GtkWidget *frame;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (wheel), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  g_signal_connect (frame, "size-allocate",
                    G_CALLBACK (colorsel_wheel_size_allocate),
                    wheel);

  if (gtk_check_version (2, 13, 7))
    {
      /*  for old versions of GtkHSV, we pack the thing into an alignment
       *  and force the alignment to have a small requisition, because
       *  it will be smart enough to deal with a larger allocation
       */
      GtkWidget *alignment;

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_container_add (GTK_CONTAINER (frame), alignment);
      gtk_widget_show (alignment);

      g_signal_connect (alignment, "size-request",
                        G_CALLBACK (colorsel_wheel_size_request),
                        wheel);

      frame = alignment;
    }

  wheel->hsv = gtk_hsv_new ();
  gtk_container_add (GTK_CONTAINER (frame), wheel->hsv);
  gtk_widget_show (wheel->hsv);

  if (! gtk_check_version (2, 13, 7))
    {
      /*  for new versions of GtkHSV we don't need above alignment hack,
       *  because it is smart enough by itself to cope with a larger
       *  allocation than it requested
       */
      g_signal_connect (wheel->hsv, "size-request",
                        G_CALLBACK (colorsel_wheel_size_request),
                        wheel);
    }

  g_signal_connect (wheel->hsv, "changed",
                    G_CALLBACK (colorsel_wheel_changed),
                    wheel);
}

static void
colorsel_wheel_set_color (GimpColorSelector *selector,
                          const GimpRGB     *rgb,
                          const GimpHSV     *hsv)
{
  ColorselWheel *wheel = COLORSEL_WHEEL (selector);

  gtk_hsv_set_color (GTK_HSV (wheel->hsv), hsv->h, hsv->s, hsv->v);
}

static void
colorsel_wheel_size_allocate (GtkWidget     *frame,
                              GtkAllocation *allocation,
                              ColorselWheel *wheel)
{
  GtkStyle *style = gtk_widget_get_style (frame);
  gint      focus_width;
  gint      focus_padding;
  gint      size;

  gtk_widget_style_get (frame,
                        "focus-line-width", &focus_width,
                        "focus-padding",    &focus_padding,
                        NULL);

  size = (MIN (allocation->width, allocation->height) -
          2 * MAX (style->xthickness, style->ythickness) -
          2 * (focus_width + focus_padding));

  gtk_hsv_set_metrics (GTK_HSV (wheel->hsv), size, size / 10);
}

static void
colorsel_wheel_size_request (GtkWidget      *dont_use,
                             GtkRequisition *requisition,
                             ColorselWheel  *wheel)
{
  gint focus_width;
  gint focus_padding;

  gtk_widget_style_get (wheel->hsv,
                        "focus-line-width", &focus_width,
                        "focus-padding",    &focus_padding,
                        NULL);

  requisition->width  = 2 * (focus_width + focus_padding);
  requisition->height = 2 * (focus_width + focus_padding);
}

static void
colorsel_wheel_changed (GtkHSV            *hsv,
                        GimpColorSelector *selector)
{
  gtk_hsv_get_color (hsv,
                     &selector->hsv.h,
                     &selector->hsv.s,
                     &selector->hsv.v);
  gimp_hsv_to_rgb (&selector->hsv, &selector->rgb);

  gimp_color_selector_color_changed (selector);
}
