/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorselector.h"
#include "gimpcolorselect.h"

#include "libgimp/gimpintl.h"


#define XY_DEF_WIDTH     GIMP_COLOR_SELECTOR_SIZE
#define XY_DEF_HEIGHT    GIMP_COLOR_SELECTOR_SIZE
#define Z_DEF_WIDTH      GIMP_COLOR_SELECTOR_BAR_SIZE
#define Z_DEF_HEIGHT     GIMP_COLOR_SELECTOR_SIZE

#define COLOR_AREA_MASK (GDK_EXPOSURE_MASK       | \
                         GDK_BUTTON_PRESS_MASK   | \
                         GDK_BUTTON_RELEASE_MASK | \
			 GDK_BUTTON1_MOTION_MASK | \
                         GDK_ENTER_NOTIFY_MASK)


typedef enum
{
  COLOR_SELECT_HUE = 0,
  COLOR_SELECT_SATURATION,
  COLOR_SELECT_VALUE,
  COLOR_SELECT_RED,
  COLOR_SELECT_GREEN,
  COLOR_SELECT_BLUE,
  COLOR_SELECT_ALPHA,
  COLOR_SELECT_HUE_SATURATION,
  COLOR_SELECT_HUE_VALUE,
  COLOR_SELECT_SATURATION_VALUE,
  COLOR_SELECT_RED_GREEN,
  COLOR_SELECT_RED_BLUE,
  COLOR_SELECT_GREEN_BLUE
} ColorSelectFillType;

typedef enum
{
  UPDATE_VALUES     = 1 << 0,
  UPDATE_POS        = 1 << 1,
  UPDATE_XY_COLOR   = 1 << 2,
  UPDATE_Z_COLOR    = 1 << 3,
  UPDATE_CALLER     = 1 << 6
} ColorSelectUpdateType;



#define GIMP_TYPE_COLOR_SELECT            (gimp_color_select_get_type ())
#define GIMP_COLOR_SELECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_SELECT, GimpColorSelect))
#define GIMP_COLOR_SELECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_SELECT, GimpColorSelectClass))
#define GIMP_IS_COLOR_SELECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_SELECT))
#define GIMP_IS_COLOR_SELECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_SELECT))
#define GIMP_COLOR_SELECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_SELECT, GimpColorSelectClass))



struct _GimpColorSelect
{
  GimpColorSelector  parent_instance;

  GtkWidget *xy_color;
  GtkWidget *z_color;

  gint       pos[3];

  GimpRGB    rgb;
  GimpHSV    hsv;

  gint       z_color_fill;
  gint       xy_color_fill;
  GdkGC     *gc;
};

struct _GimpColorSelectClass
{
  GimpColorSelectorClass  parent_class;
};


typedef struct _ColorSelectFill ColorSelectFill;

typedef void (* ColorSelectFillUpdateProc) (ColorSelectFill *color_select_fill);

struct _ColorSelectFill
{
  guchar  *buffer;
  gint     y;
  gint     width;
  gint     height;
  GimpHSV  hsv;
  GimpRGB  rgb;

  ColorSelectFillUpdateProc update;
};


static void   gimp_color_select_class_init  (GimpColorSelectClass  *klass);
static void   gimp_color_select_init        (GimpColorSelect       *select);

static void   gimp_color_select_finalize    (GObject               *object);

static void   gimp_color_select_set_color   (GimpColorSelector     *selector,
                                             const GimpRGB         *rgb,
                                             const GimpHSV         *hsv);
static void   gimp_color_select_set_channel (GimpColorSelector     *selector,
                                             GimpColorSelectorChannel  channel);

static void   gimp_color_select_update      (GimpColorSelect       *select,
                                             ColorSelectUpdateType  type);
static void   gimp_color_select_update_caller     (GimpColorSelect    *select);
static void   gimp_color_select_update_values     (GimpColorSelect    *select);
static void   gimp_color_select_update_rgb_values (GimpColorSelect    *select);
static void   gimp_color_select_update_hsv_values (GimpColorSelect    *select);
static void   gimp_color_select_update_pos        (GimpColorSelect    *select);

#if 0
static void   gimp_color_select_drop_color        (GtkWidget          *widget,
                                                   const GimpRGB      *color,
                                                   gpointer            data);
#endif

static gboolean   gimp_color_select_xy_expose     (GtkWidget          *widget,
                                                   GdkEventExpose     *eevent,
                                                   GimpColorSelect    *select);
static gboolean   gimp_color_select_xy_events     (GtkWidget          *widget,
                                                   GdkEvent           *event,
                                                   GimpColorSelect    *select);
static gboolean   gimp_color_select_z_expose      (GtkWidget          *widget,
                                                   GdkEventExpose     *eevent,
                                                   GimpColorSelect    *select);
static gboolean   gimp_color_select_z_events      (GtkWidget          *widet,
                                                   GdkEvent           *event,
                                                   GimpColorSelect    *select);

static void   gimp_color_select_image_fill        (GtkWidget          *widget,
                                                   ColorSelectFillType,
                                                   const GimpHSV      *hsv,
                                                   const GimpRGB      *rgb);

static void   gimp_color_select_draw_z_marker     (GimpColorSelect    *select,
                                                   GdkRectangle       *clip);
static void   gimp_color_select_draw_xy_marker    (GimpColorSelect    *select,
                                                   GdkRectangle       *clip);

static void   color_select_update_red              (ColorSelectFill *csf);
static void   color_select_update_green            (ColorSelectFill *csf);
static void   color_select_update_blue             (ColorSelectFill *csf);
static void   color_select_update_hue              (ColorSelectFill *csf);
static void   color_select_update_saturation       (ColorSelectFill *csf);
static void   color_select_update_value            (ColorSelectFill *csf);
static void   color_select_update_red_green        (ColorSelectFill *csf);
static void   color_select_update_red_blue         (ColorSelectFill *csf);
static void   color_select_update_green_blue       (ColorSelectFill *csf);
static void   color_select_update_hue_saturation   (ColorSelectFill *csf);
static void   color_select_update_hue_value        (ColorSelectFill *csf);
static void   color_select_update_saturation_value (ColorSelectFill *csf);


/*  static variables  */

static GimpColorSelectorClass *parent_class = NULL;

static ColorSelectFillUpdateProc update_procs[] =
{
  color_select_update_hue,
  color_select_update_saturation,
  color_select_update_value,
  color_select_update_red,
  color_select_update_green,
  color_select_update_blue,
  NULL, /* alpha */
  color_select_update_hue_saturation,
  color_select_update_hue_value,
  color_select_update_saturation_value,
  color_select_update_red_green,
  color_select_update_red_blue,
  color_select_update_green_blue,
};


GType
gimp_color_select_get_type (void)
{
  static GType select_type = 0;

  if (! select_type)
    {
      static const GTypeInfo select_info =
      {
        sizeof (GimpColorSelectClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_select_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorSelect),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_select_init,
      };

      select_type = g_type_register_static (GIMP_TYPE_COLOR_SELECTOR,
                                            "GimpColorSelect", 
                                            &select_info, 0);
    }

  return select_type;
}

static void
gimp_color_select_class_init (GimpColorSelectClass *klass)
{
  GObjectClass           *object_class;
  GimpColorSelectorClass *selector_class;

  object_class   = G_OBJECT_CLASS (klass);
  selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize      = gimp_color_select_finalize;

  selector_class->name        = "GIMP";
  selector_class->help_page   = "built_in.html";
  selector_class->set_color   = gimp_color_select_set_color;
  selector_class->set_channel = gimp_color_select_set_channel;
}

static void
gimp_color_select_init (GimpColorSelect *select)
{
  GtkWidget *main_hbox;
  GtkWidget *hbox;
  GtkWidget *xy_frame;
  GtkWidget *z_frame;
			
  select->z_color_fill  = COLOR_SELECT_HUE;
  select->xy_color_fill = COLOR_SELECT_SATURATION_VALUE;
  select->gc            = NULL;

  main_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (select), main_hbox, TRUE, FALSE, 0);
  gtk_widget_show (main_hbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), hbox, TRUE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The x/y component preview  */
  xy_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (xy_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), xy_frame, FALSE, FALSE, 0);
  gtk_widget_show (xy_frame);

  select->xy_color = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (select->xy_color), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (select->xy_color), XY_DEF_WIDTH, XY_DEF_HEIGHT);
  gtk_widget_set_events (select->xy_color, COLOR_AREA_MASK);
  gtk_container_add (GTK_CONTAINER (xy_frame), select->xy_color);
  gtk_widget_show (select->xy_color);

  g_signal_connect_after (G_OBJECT (select->xy_color), "expose_event",
			  G_CALLBACK (gimp_color_select_xy_expose),
			  select);
  g_signal_connect (G_OBJECT (select->xy_color), "event",
		    G_CALLBACK (gimp_color_select_xy_events),
		    select);

#if 0
  gimp_dnd_color_dest_add (select->xy_color, gimp_color_select_drop_color,
                           select);
#endif

  /*  The z component preview  */
  z_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (z_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), z_frame, FALSE, FALSE, 0);
  gtk_widget_show (z_frame);

  select->z_color = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (select->z_color), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (select->z_color), Z_DEF_WIDTH, Z_DEF_HEIGHT);
  gtk_widget_set_events (select->z_color, COLOR_AREA_MASK);
  gtk_container_add (GTK_CONTAINER (z_frame), select->z_color);
  gtk_widget_show (select->z_color);

  g_signal_connect_after (G_OBJECT (select->z_color), "expose_event",
			  G_CALLBACK (gimp_color_select_z_expose),
			  select);
  g_signal_connect (G_OBJECT (select->z_color), "event",
		    G_CALLBACK (gimp_color_select_z_events),
		    select);
}

static void
gimp_color_select_finalize (GObject *object)
{
  GimpColorSelect *select;

  select = GIMP_COLOR_SELECT (object);

  if (select->gc)
    {
      g_object_unref (select->gc);
      select->gc = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_select_set_color (GimpColorSelector *selector,
                             const GimpRGB     *rgb,
                             const GimpHSV     *hsv)
{
  GimpColorSelect *select;

  select = GIMP_COLOR_SELECT (selector);

  select->rgb = *rgb;
  select->hsv = *hsv;

  gimp_color_select_update_pos (select);

  gimp_color_select_update (select, UPDATE_Z_COLOR);
  gimp_color_select_update (select, UPDATE_XY_COLOR);
}

static void
gimp_color_select_set_channel (GimpColorSelector        *selector,
                               GimpColorSelectorChannel  channel)
{
  GimpColorSelect *select;

  select = GIMP_COLOR_SELECT (selector);

  switch ((ColorSelectFillType) channel)
    {
    case COLOR_SELECT_HUE:
      select->z_color_fill  = COLOR_SELECT_HUE;
      select->xy_color_fill = COLOR_SELECT_SATURATION_VALUE;
      break;

    case COLOR_SELECT_SATURATION:
      select->z_color_fill  = COLOR_SELECT_SATURATION;
      select->xy_color_fill = COLOR_SELECT_HUE_VALUE;
      break;

    case COLOR_SELECT_VALUE:
      select->z_color_fill  = COLOR_SELECT_VALUE;
      select->xy_color_fill = COLOR_SELECT_HUE_SATURATION;
      break;

    case COLOR_SELECT_RED:
      select->z_color_fill  = COLOR_SELECT_RED;
      select->xy_color_fill = COLOR_SELECT_GREEN_BLUE;
      break;

    case COLOR_SELECT_GREEN:
      select->z_color_fill  = COLOR_SELECT_GREEN;
      select->xy_color_fill = COLOR_SELECT_RED_BLUE;
      break;

    case COLOR_SELECT_BLUE:
      select->z_color_fill  = COLOR_SELECT_BLUE;
      select->xy_color_fill = COLOR_SELECT_RED_GREEN;
      break;

    default:
      break;
    }

  gimp_color_select_update (select, UPDATE_POS);
  gimp_color_select_update (select, UPDATE_Z_COLOR | UPDATE_XY_COLOR);
}

static void
gimp_color_select_update (GimpColorSelect       *select,
                          ColorSelectUpdateType  update)
{
  if (update & UPDATE_POS)
    gimp_color_select_update_pos (select);

  if (update & UPDATE_VALUES)
    gimp_color_select_update_values (select);

  if (update & UPDATE_XY_COLOR)
    {
      gimp_color_select_image_fill (select->xy_color, select->xy_color_fill,
                                    &select->hsv, &select->rgb);
      gtk_widget_queue_draw (select->xy_color);
    }

  if (update & UPDATE_Z_COLOR)
    {
      gimp_color_select_image_fill (select->z_color, select->z_color_fill,
                                    &select->hsv, &select->rgb);
      gtk_widget_queue_draw (select->z_color);
    }

  if (update & UPDATE_CALLER)
    gimp_color_select_update_caller (select);
}

static void
gimp_color_select_update_caller (GimpColorSelect *select)
{
  gimp_color_selector_color_changed (GIMP_COLOR_SELECTOR (select),
                                     &select->rgb,
                                     &select->hsv);
}

static void
gimp_color_select_update_values (GimpColorSelect *select)
{
  switch (select->z_color_fill)
    {
    case COLOR_SELECT_RED:
      select->rgb.b = select->pos[0] / 255.0;
      select->rgb.g = select->pos[1] / 255.0;
      select->rgb.r = select->pos[2] / 255.0;
      break;
    case COLOR_SELECT_GREEN:
      select->rgb.b = select->pos[0] / 255.0;
      select->rgb.r = select->pos[1] / 255.0;
      select->rgb.g = select->pos[2] / 255.0;
      break;
    case COLOR_SELECT_BLUE:
      select->rgb.g = select->pos[0] / 255.0;
      select->rgb.r = select->pos[1] / 255.0;
      select->rgb.b = select->pos[2] / 255.0;
      break;

    case COLOR_SELECT_HUE:
      select->hsv.v = select->pos[0] / 255.0;
      select->hsv.s = select->pos[1] / 255.0;
      select->hsv.h = select->pos[2] / 255.0;
      break;
    case COLOR_SELECT_SATURATION:
      select->hsv.v = select->pos[0] / 255.0;
      select->hsv.h = select->pos[1] / 255.0;
      select->hsv.s = select->pos[2] / 255.0;
      break;
    case COLOR_SELECT_VALUE:
      select->hsv.s = select->pos[0] / 255.0;
      select->hsv.h = select->pos[1] / 255.0;
      select->hsv.v = select->pos[2] / 255.0;
      break;
    }

  switch (select->z_color_fill)
    {
    case COLOR_SELECT_RED:
    case COLOR_SELECT_GREEN:
    case COLOR_SELECT_BLUE:
      gimp_color_select_update_hsv_values (select);
      break;

    case COLOR_SELECT_HUE:
    case COLOR_SELECT_SATURATION:
    case COLOR_SELECT_VALUE:
      gimp_color_select_update_rgb_values (select);
      break;
    }
}

static void
gimp_color_select_update_rgb_values (GimpColorSelect *select)
{
  gimp_hsv_to_rgb (&select->hsv, &select->rgb);
}

static void
gimp_color_select_update_hsv_values (GimpColorSelect *select)
{
  gimp_rgb_to_hsv (&select->rgb, &select->hsv);
}

static void
gimp_color_select_update_pos (GimpColorSelect *select)
{
  switch (select->z_color_fill)
    {
    case COLOR_SELECT_RED:
      select->pos[0] = (gint) (select->rgb.b * 255.999);
      select->pos[1] = (gint) (select->rgb.g * 255.999);
      select->pos[2] = (gint) (select->rgb.r * 255.999);
      break;
    case COLOR_SELECT_GREEN:
      select->pos[0] = (gint) (select->rgb.b * 255.999);
      select->pos[1] = (gint) (select->rgb.r * 255.999);
      select->pos[2] = (gint) (select->rgb.g * 255.999);
      break;
    case COLOR_SELECT_BLUE:
      select->pos[0] = (gint) (select->rgb.g * 255.999);
      select->pos[1] = (gint) (select->rgb.r * 255.999);
      select->pos[2] = (gint) (select->rgb.b * 255.999);
      break;

    case COLOR_SELECT_HUE:
      select->pos[0] = (gint) (select->hsv.v * 255.999);
      select->pos[1] = (gint) (select->hsv.s * 255.999);
      select->pos[2] = (gint) (select->hsv.h * 255.999);
      break;
    case COLOR_SELECT_SATURATION:
      select->pos[0] = (gint) (select->hsv.v * 255.999);
      select->pos[1] = (gint) (select->hsv.h * 255.999);
      select->pos[2] = (gint) (select->hsv.s * 255.999);
      break;
    case COLOR_SELECT_VALUE:
      select->pos[0] = (gint) (select->hsv.s * 255.999);
      select->pos[1] = (gint) (select->hsv.h * 255.999);
      select->pos[2] = (gint) (select->hsv.v * 255.999);
      break;
    }
}

#if 0
static void
gimp_color_select_drop_color (GtkWidget     *widget,
                              const GimpRGB *color,
                              gpointer       data)
{
  GimpColorSelect *select;

  select = GIMP_COLOR_SELECT (data);

  select->rgb = *color;

  gimp_color_select_update_hsv_values (select);
  gimp_color_select_update_pos (select);

  gimp_color_select_update (select, UPDATE_Z_COLOR);
  gimp_color_select_update (select, UPDATE_XY_COLOR);

  gimp_color_select_update (select, UPDATE_CALLER);
}
#endif

static gboolean
gimp_color_select_xy_expose (GtkWidget       *widget,
                             GdkEventExpose  *event,
                             GimpColorSelect *select)
{
  if (! select->gc)
    select->gc = gdk_gc_new (widget->window);

  gimp_color_select_draw_xy_marker (select, &event->area);

  return TRUE;
}

static gboolean
gimp_color_select_xy_events (GtkWidget       *widget,
                             GdkEvent        *event,
                             GimpColorSelect *select)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint            tx, ty;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      gimp_color_select_draw_xy_marker (select, NULL);

      select->pos[0] = (bevent->x * 255) / (XY_DEF_WIDTH - 1);
      select->pos[1] = 255 - (bevent->y * 255) / (XY_DEF_HEIGHT - 1);

      if (select->pos[0] < 0)
	select->pos[0] = 0;
      if (select->pos[0] > 255)
	select->pos[0] = 255;
      if (select->pos[1] < 0)
	select->pos[1] = 0;
      if (select->pos[1] > 255)
	select->pos[1] = 255;

      gdk_pointer_grab (select->xy_color->window, FALSE,
			GDK_POINTER_MOTION_HINT_MASK |
			GDK_BUTTON1_MOTION_MASK      |
			GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, bevent->time);
      gimp_color_select_draw_xy_marker (select, NULL);

      gimp_color_select_update (select, UPDATE_VALUES | UPDATE_CALLER);
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      gimp_color_select_draw_xy_marker (select, NULL);

      select->pos[0] = (bevent->x * 255) / (XY_DEF_WIDTH - 1);
      select->pos[1] = 255 - (bevent->y * 255) / (XY_DEF_HEIGHT - 1);

      if (select->pos[0] < 0)
	select->pos[0] = 0;
      if (select->pos[0] > 255)
	select->pos[0] = 255;
      if (select->pos[1] < 0)
	select->pos[1] = 0;
      if (select->pos[1] > 255)
	select->pos[1] = 255;

      gdk_pointer_ungrab (bevent->time);
      gimp_color_select_draw_xy_marker (select, NULL);
      gimp_color_select_update (select, UPDATE_VALUES | UPDATE_CALLER);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if (mevent->is_hint)
	{
	  gdk_window_get_pointer (widget->window, &tx, &ty, NULL);
	  mevent->x = tx;
	  mevent->y = ty;
	}

      gimp_color_select_draw_xy_marker (select, NULL);

      select->pos[0] = (mevent->x * 255) / (XY_DEF_WIDTH - 1);
      select->pos[1] = 255 - (mevent->y * 255) / (XY_DEF_HEIGHT - 1);

      if (select->pos[0] < 0)
	select->pos[0] = 0;
      if (select->pos[0] > 255)
	select->pos[0] = 255;
      if (select->pos[1] < 0)
	select->pos[1] = 0;
      if (select->pos[1] > 255)
	select->pos[1] = 255;

      gimp_color_select_draw_xy_marker (select, NULL);
      gimp_color_select_update (select, UPDATE_VALUES | UPDATE_CALLER);
      break;

    default:
      break;
    }

  return FALSE;
}

static gboolean
gimp_color_select_z_expose (GtkWidget       *widget,
                            GdkEventExpose  *event,
                            GimpColorSelect *select)
{
  if (! select->gc)
    select->gc = gdk_gc_new (widget->window);

  gimp_color_select_draw_z_marker (select, &event->area);

  return TRUE;
}

static gboolean
gimp_color_select_z_events (GtkWidget       *widget,
                            GdkEvent        *event,
                            GimpColorSelect *select)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint            tx, ty;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      gimp_color_select_draw_z_marker (select, NULL);

      select->pos[2] = 255 - (bevent->y * 255) / (Z_DEF_HEIGHT - 1);

      select->pos[2] = CLAMP (select->pos[2], 0, 255);

      gdk_pointer_grab (select->z_color->window, FALSE,
			GDK_POINTER_MOTION_HINT_MASK |
			GDK_BUTTON1_MOTION_MASK |
			GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, bevent->time);
      gimp_color_select_draw_z_marker (select, NULL);
      gimp_color_select_update (select, UPDATE_VALUES | UPDATE_CALLER);
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      gimp_color_select_draw_z_marker (select, NULL);

      select->pos[2] = 255 - (bevent->y * 255) / (Z_DEF_HEIGHT - 1);

      select->pos[2] = CLAMP (select->pos[2], 0, 255);

      gdk_pointer_ungrab (bevent->time);
      gimp_color_select_draw_z_marker (select, NULL);
      gimp_color_select_update (select,
                                UPDATE_VALUES | UPDATE_XY_COLOR | UPDATE_CALLER);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if (mevent->is_hint)
	{
	  gdk_window_get_pointer (widget->window, &tx, &ty, NULL);
	  mevent->x = tx;
	  mevent->y = ty;
	}

      gimp_color_select_draw_z_marker (select, NULL);

      select->pos[2] = 255 - (mevent->y * 255) / (Z_DEF_HEIGHT - 1);

      select->pos[2] = CLAMP (select->pos[2], 0, 255);

      gimp_color_select_draw_z_marker (select, NULL);
      gimp_color_select_update (select, UPDATE_VALUES | UPDATE_CALLER);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_color_select_image_fill (GtkWidget           *preview,
                              ColorSelectFillType  type,
                              const GimpHSV       *hsv,
                              const GimpRGB       *rgb)
{
  ColorSelectFill csf;
  gint            height;

  csf.buffer = g_malloc (preview->requisition.width * 3);

  csf.update = update_procs[type];

  csf.y      = -1;
  csf.width  = preview->requisition.width;
  csf.height = preview->requisition.height;
  csf.hsv    = *hsv;
  csf.rgb    = *rgb;

  height = csf.height;
  if (height > 0)
    while (height--)
      {
	if (csf.update)
	  (* csf.update) (&csf);

	gtk_preview_draw_row (GTK_PREVIEW (preview),
			      csf.buffer, 0, csf.y, csf.width);
      }

  g_free (csf.buffer);
}

static void
gimp_color_select_draw_z_marker (GimpColorSelect *select,
                                 GdkRectangle    *clip)
{
  gint width;
  gint height;
  gint y;
  gint minx;
  gint miny;

  if (! select->gc)
    return;

  y = (Z_DEF_HEIGHT - 1) - ((Z_DEF_HEIGHT - 1) * select->pos[2]) / 255;
  width  = select->z_color->requisition.width;
  height = select->z_color->requisition.height;
  minx = 0;
  miny = 0;
  if (width <= 0)
    return;

  if (clip)
    {
      width  = MIN (width,  clip->x + clip->width);
      height = MIN (height, clip->y + clip->height);
      minx   = MAX (0, clip->x);
      miny   = MAX (0, clip->y);
    }

  if (y >= miny && y < height)
    {
      gdk_gc_set_function (select->gc, GDK_INVERT);
      gdk_draw_line (select->z_color->window, select->gc, minx, y, width - 1, y);
      gdk_gc_set_function (select->gc, GDK_COPY);
    }
}

static void
gimp_color_select_draw_xy_marker (GimpColorSelect *select,
                                  GdkRectangle    *clip)
{
  gint width;
  gint height;
  gint x, y;
  gint minx, miny;

  if (! select->gc)
    return;

  x = ((XY_DEF_WIDTH - 1) * select->pos[0]) / 255;
  y = (XY_DEF_HEIGHT - 1) - ((XY_DEF_HEIGHT - 1) * select->pos[1]) / 255;
  width = select->xy_color->requisition.width;
  height = select->xy_color->requisition.height;
  minx = 0;
  miny = 0;
  if ((width <= 0) || (height <= 0))
    return;

  gdk_gc_set_function (select->gc, GDK_INVERT);

  if (clip)
    {
      width  = MIN (width,  clip->x + clip->width);
      height = MIN (height, clip->y + clip->height);
      minx   = MAX (0, clip->x);
      miny   = MAX (0, clip->y);
    }

  if (y >= miny && y < height)
    gdk_draw_line (select->xy_color->window, select->gc, minx, y, width - 1, y);

  if (x >= minx && x < width)
    gdk_draw_line (select->xy_color->window, select->gc, x, miny, x, height - 1);

  gdk_gc_set_function (select->gc, GDK_COPY);
}

static void
color_select_update_red (ColorSelectFill *csf)
{
  guchar *p;
  gint    i, r;

  p = csf->buffer;

  csf->y += 1;
  r = (csf->height - csf->y + 1) * 255 / csf->height;

  if (r < 0)
    r = 0;
  if (r > 255)
    r = 255;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = 0;
      *p++ = 0;
    }
}

static void
color_select_update_green (ColorSelectFill *csf)
{
  guchar *p;
  gint    i, g;

  p = csf->buffer;

  csf->y += 1;
  g = (csf->height - csf->y + 1) * 255 / csf->height;

  if (g < 0)
    g = 0;
  if (g > 255)
    g = 255;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = 0;
      *p++ = g;
      *p++ = 0;
    }
}

static void
color_select_update_blue (ColorSelectFill *csf)
{
  guchar *p;
  gint    i, b;

  p = csf->buffer;

  csf->y += 1;
  b = (csf->height - csf->y + 1) * 255 / csf->height;

  if (b < 0)
    b = 0;
  if (b > 255)
    b = 255;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = 0;
      *p++ = 0;
      *p++ = b;
    }
}

static void
color_select_update_hue (ColorSelectFill *csf)
{
  guchar *p;
  gfloat  h, f;
  gint    r, g, b;
  gint    i;

  p = csf->buffer;

  csf->y += 1;
  h = csf->y * 360.0 / csf->height;

  h = CLAMP (360 - h, 0, 360);

  h /= 60;
  f = (h - (int) h) * 255;

  r = g = b = 0;

  switch ((int) h)
    {
    case 0:
      r = 255;
      g = f;
      b = 0;
      break;
    case 1:
      r = 255 - f;
      g = 255;
      b = 0;
      break;
    case 2:
      r = 0;
      g = 255;
      b = f;
      break;
    case 3:
      r = 0;
      g = 255 - f;
      b = 255;
      break;
    case 4:
      r = f;
      g = 0;
      b = 255;
      break;
    case 5:
      r = 255;
      g = 0;
      b = 255 - f;
      break;
    }

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;
    }
}

static void
color_select_update_saturation (ColorSelectFill *csf)
{
  guchar *p;
  gint    s;
  gint    i;

  p = csf->buffer;

  csf->y += 1;
  s = csf->y * 255 / csf->height;

  if (s < 0)
    s = 0;
  if (s > 255)
    s = 255;

  s = 255 - s;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = s;
      *p++ = s;
      *p++ = s;
    }
}

static void
color_select_update_value (ColorSelectFill *csf)
{
  guchar *p;
  gint    v;
  gint    i;

  p = csf->buffer;

  csf->y += 1;
  v = csf->y * 255 / csf->height;

  if (v < 0)
    v = 0;
  if (v > 255)
    v = 255;

  v = 255 - v;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = v;
      *p++ = v;
      *p++ = v;
    }
}

static void
color_select_update_red_green (ColorSelectFill *csf)
{
  guchar *p;
  gint    i, r, b;
  gfloat  g, dg;

  p = csf->buffer;

  csf->y += 1;
  b = (gint) (csf->rgb.b * 255.999);
  r = (csf->height - csf->y + 1) * 255 / csf->height;

  if (r < 0)
    r = 0;
  if (r > 255)
    r = 255;

  g = 0;
  dg = 255.0 / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;

      g += dg;
    }
}

static void
color_select_update_red_blue (ColorSelectFill *csf)
{
  guchar *p;
  gint    i, r, g;
  gfloat  b, db;

  p = csf->buffer;

  csf->y += 1;
  g = (gint) (csf->rgb.g * 255.999);
  r = (csf->height - csf->y + 1) * 255 / csf->height;

  if (r < 0)
    r = 0;
  if (r > 255)
    r = 255;

  b = 0;
  db = 255.0 / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;

      b += db;
    }
}

static void
color_select_update_green_blue (ColorSelectFill *csf)
{
  guchar *p;
  gint    i, g, r;
  gfloat  b, db;

  p = csf->buffer;

  csf->y += 1;
  r = (gint) (csf->rgb.r * 255.999);
  g = (csf->height - csf->y + 1) * 255 / csf->height;

  if (g < 0)
    g = 0;
  if (g > 255)
    g = 255;

  b = 0;
  db = 255.0 / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;

      b += db;
    }
}

static void
color_select_update_hue_saturation (ColorSelectFill *csf)
{
  guchar *p;
  gfloat  h, v, s, ds;
  gint    f;
  gint    i;

  p = csf->buffer;

  csf->y += 1;
  h = 360 - (csf->y * 360 / csf->height);

  if (h < 0)
    h = 0;
  if (h > 359)
    h = 359;

  h /= 60;
  f = (h - (int) h) * 255;

  s = 0;
  ds = 1.0 / csf->width;

  v = csf->hsv.v;

  switch ((int) h)
    {
    case 0:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * 255 * (1 - s);

	  s += ds;
	}
      break;
    case 1:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);

	  s += ds;
	}
      break;
    case 2:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v *255;
	  *p++ = v * (255 - (s * (255 - f)));

	  s += ds;
	}
      break;
    case 3:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;

	  s += ds;
	}
      break;
    case 4:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * (255 * (1 - s));
	  *p++ = v * 255;

	  s += ds;
	}
      break;
    case 5:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);

	  s += ds;
	}
      break;
    }
}

static void
color_select_update_hue_value (ColorSelectFill *csf)
{
  guchar *p;
  gfloat  h, v, dv, s;
  gint    f;
  gint    i;

  p = csf->buffer;

  csf->y += 1;
  h = 360 - (csf->y * 360 / csf->height);

  if (h < 0)
    h = 0;
  if (h > 359)
    h = 359;

  h /= 60;
  f = (h - (int) h) * 255;

  v = 0;
  dv = 1.0 / csf->width;

  s = csf->hsv.s;

  switch ((int) h)
    {
    case 0:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * 255 * (1 - s);

	  v += dv;
	}
      break;
    case 1:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);

	  v += dv;
	}
      break;
    case 2:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v *255;
	  *p++ = v * (255 - (s * (255 - f)));

	  v += dv;
	}
      break;
    case 3:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;

	  v += dv;
	}
      break;
    case 4:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * (255 * (1 - s));
	  *p++ = v * 255;

	  v += dv;
	}
      break;
    case 5:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);

	  v += dv;
	}
      break;
    }
}

static void
color_select_update_saturation_value (ColorSelectFill *csf)
{
  guchar *p;
  gfloat  h, v, dv, s;
  gint    f;
  gint    i;

  p = csf->buffer;

  csf->y += 1;
  s = (gfloat) csf->y / csf->height;

  if (s < 0)
    s = 0;
  if (s > 1)
    s = 1;

  s = 1 - s;

  h = (gfloat) csf->hsv.h * 360.0;
  if (h >= 360)
    h -= 360;
  h /= 60;
  f = (h - (gint) h) * 255;

  v = 0;
  dv = 1.0 / csf->width;

  switch ((gint) h)
    {
    case 0:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * 255 * (1 - s);

	  v += dv;
	}
      break;
    case 1:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);

	  v += dv;
	}
      break;
    case 2:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v *255;
	  *p++ = v * (255 - (s * (255 - f)));

	  v += dv;
	}
      break;
    case 3:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;

	  v += dv;
	}
      break;
    case 4:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * (255 * (1 - s));
	  *p++ = v * 255;

	  v += dv;
	}
      break;
    case 5:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);

	  v += dv;
	}
      break;
    }
}
