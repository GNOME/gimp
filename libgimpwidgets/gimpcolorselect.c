/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselect.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorselector.h"
#include "gimpcolorselect.h"
#include "gimphelpui.h"
#include "gimpicons.h"
#include "gimpwidgetsutils.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpcolorselect
 * @title: GimpColorSelect
 * @short_description: A #GimpColorSelector implementation.
 *
 * The #GimpColorSelect widget is an implementation of a
 * #GimpColorSelector. It shows a square area that supports
 * interactively changing two color channels and a smaller area to
 * change the third channel. You can select which channel should be
 * the third by calling gimp_color_selector_set_channel(). The widget
 * will then change the other two channels accordingly.
 **/


#define COLOR_AREA_EVENT_MASK (GDK_EXPOSURE_MASK       | \
                               GDK_BUTTON_PRESS_MASK   | \
                               GDK_BUTTON_RELEASE_MASK | \
                               GDK_BUTTON_MOTION_MASK  | \
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

  COLOR_SELECT_LCH_LIGHTNESS,
  COLOR_SELECT_LCH_CHROMA,
  COLOR_SELECT_LCH_HUE,

  COLOR_SELECT_HUE_SATURATION,
  COLOR_SELECT_HUE_VALUE,
  COLOR_SELECT_SATURATION_VALUE,

  COLOR_SELECT_RED_GREEN,
  COLOR_SELECT_RED_BLUE,
  COLOR_SELECT_GREEN_BLUE,

  COLOR_SELECT_LCH_HUE_CHROMA,
  COLOR_SELECT_LCH_HUE_LIGHTNESS,
  COLOR_SELECT_LCH_CHROMA_LIGHTNESS
} ColorSelectFillType;

typedef enum
{
  UPDATE_VALUES     = 1 << 0,
  UPDATE_POS        = 1 << 1,
  UPDATE_XY_COLOR   = 1 << 2,
  UPDATE_Z_COLOR    = 1 << 3,
  UPDATE_CALLER     = 1 << 6
} ColorSelectUpdateType;

typedef enum
{
  DRAG_NONE,
  DRAG_XY,
  DRAG_Z
} ColorSelectDragMode;


typedef struct _GimpLCH  GimpLCH;

struct _GimpLCH
{
  gdouble l, c, h, a;
};


#define GIMP_COLOR_SELECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_SELECT, GimpColorSelectClass))
#define GIMP_IS_COLOR_SELECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_SELECT))
#define GIMP_COLOR_SELECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_SELECT, GimpColorSelectClass))


typedef struct _GimpColorSelectClass GimpColorSelectClass;

struct _GimpColorSelect
{
  GimpColorSelector    parent_instance;

  GtkWidget           *toggle_box[3];

  GtkWidget           *xy_color;
  ColorSelectFillType  xy_color_fill;
  guchar              *xy_buf;
  gint                 xy_width;
  gint                 xy_height;
  gint                 xy_rowstride;
  gboolean             xy_needs_render;

  GtkWidget           *z_color;
  ColorSelectFillType  z_color_fill;
  guchar              *z_buf;
  gint                 z_width;
  gint                 z_height;
  gint                 z_rowstride;
  gboolean             z_needs_render;

  gdouble              pos[3];

  ColorSelectDragMode  drag_mode;

  GimpColorConfig     *config;
  GimpColorTransform  *transform;
  guchar               oog_color[3];
};

struct _GimpColorSelectClass
{
  GimpColorSelectorClass  parent_class;
};


typedef struct _ColorSelectFill ColorSelectFill;

typedef void (* ColorSelectRenderFunc) (ColorSelectFill *color_select_fill);

struct _ColorSelectFill
{
  guchar  *buffer;
  gint     y;
  gint     width;
  gint     height;
  GimpRGB  rgb;
  GimpHSV  hsv;
  GimpLCH  lch;
  guchar   oog_color[3];

  ColorSelectRenderFunc render_line;
};


static void   gimp_color_select_finalize        (GObject            *object);

static void   gimp_color_select_togg_visible    (GimpColorSelector  *selector,
                                                 gboolean            visible);
static void   gimp_color_select_togg_sensitive  (GimpColorSelector  *selector,
                                                 gboolean            sensitive);
static void   gimp_color_select_set_color       (GimpColorSelector  *selector,
                                                 const GimpRGB      *rgb,
                                                 const GimpHSV      *hsv);
static void   gimp_color_select_set_channel     (GimpColorSelector  *selector,
                                                 GimpColorSelectorChannel  channel);
static void   gimp_color_select_set_model_visible
                                                (GimpColorSelector  *selector,
                                                 GimpColorSelectorModel  model,
                                                 gboolean            visible);
static void   gimp_color_select_set_config      (GimpColorSelector  *selector,
                                                 GimpColorConfig    *config);

static void   gimp_color_select_channel_toggled (GtkWidget          *widget,
                                                 GimpColorSelect    *select);

static void   gimp_color_select_update          (GimpColorSelect    *select,
                                                 ColorSelectUpdateType  type);
static void   gimp_color_select_update_values   (GimpColorSelect    *select);
static void   gimp_color_select_update_pos      (GimpColorSelect    *select);

#if 0
static void   gimp_color_select_drop_color      (GtkWidget          *widget,
                                                 gint                x,
                                                 gint                y,
                                                 const GimpRGB      *color,
                                                 gpointer            data);
#endif

static void  gimp_color_select_xy_size_allocate (GtkWidget          *widget,
                                                 GtkAllocation      *allocation,
                                                 GimpColorSelect    *select);
static gboolean   gimp_color_select_xy_draw     (GtkWidget          *widget,
                                                 cairo_t            *cr,
                                                 GimpColorSelect    *select);
static gboolean   gimp_color_select_xy_events   (GtkWidget          *widget,
                                                 GdkEvent           *event,
                                                 GimpColorSelect    *select);
static void   gimp_color_select_z_size_allocate (GtkWidget          *widget,
                                                 GtkAllocation      *allocation,
                                                 GimpColorSelect    *select);
static gboolean   gimp_color_select_z_draw      (GtkWidget          *widget,
                                                 cairo_t            *cr,
                                                 GimpColorSelect    *select);
static gboolean   gimp_color_select_z_events    (GtkWidget          *widget,
                                                 GdkEvent           *event,
                                                 GimpColorSelect    *select);

static void   gimp_color_select_render          (GtkWidget          *widget,
                                                 guchar             *buf,
                                                 gint                width,
                                                 gint                height,
                                                 gint                rowstride,
                                                 ColorSelectFillType fill_type,
                                                 const GimpHSV      *hsv,
                                                 const GimpRGB      *rgb,
                                                 const guchar       *oog_color);

static void   color_select_render_red              (ColorSelectFill *csf);
static void   color_select_render_green            (ColorSelectFill *csf);
static void   color_select_render_blue             (ColorSelectFill *csf);

static void   color_select_render_hue              (ColorSelectFill *csf);
static void   color_select_render_saturation       (ColorSelectFill *csf);
static void   color_select_render_value            (ColorSelectFill *csf);

static void   color_select_render_lch_lightness    (ColorSelectFill *csf);
static void   color_select_render_lch_chroma       (ColorSelectFill *csf);
static void   color_select_render_lch_hue          (ColorSelectFill *csf);

static void   color_select_render_red_green        (ColorSelectFill *csf);
static void   color_select_render_red_blue         (ColorSelectFill *csf);
static void   color_select_render_green_blue       (ColorSelectFill *csf);

static void   color_select_render_hue_saturation   (ColorSelectFill *csf);
static void   color_select_render_hue_value        (ColorSelectFill *csf);
static void   color_select_render_saturation_value (ColorSelectFill *csf);

static void   color_select_render_lch_chroma_lightness (ColorSelectFill *csf);
static void   color_select_render_lch_hue_lightness    (ColorSelectFill *csf);
static void   color_select_render_lch_hue_chroma       (ColorSelectFill *csf);

static void   gimp_color_select_create_transform   (GimpColorSelect  *select);
static void   gimp_color_select_destroy_transform  (GimpColorSelect  *select);
static void   gimp_color_select_notify_config      (GimpColorConfig  *config,
                                                    const GParamSpec *pspec,
                                                    GimpColorSelect  *select);


G_DEFINE_TYPE (GimpColorSelect, gimp_color_select, GIMP_TYPE_COLOR_SELECTOR)

#define parent_class gimp_color_select_parent_class

static const ColorSelectRenderFunc render_funcs[] =
{
  color_select_render_hue,
  color_select_render_saturation,
  color_select_render_value,

  color_select_render_red,
  color_select_render_green,
  color_select_render_blue,
  NULL, /* alpha */

  color_select_render_lch_lightness,
  color_select_render_lch_chroma,
  color_select_render_lch_hue,

  color_select_render_hue_saturation,
  color_select_render_hue_value,
  color_select_render_saturation_value,

  color_select_render_red_green,
  color_select_render_red_blue,
  color_select_render_green_blue,

  color_select_render_lch_hue_chroma,
  color_select_render_lch_hue_lightness,
  color_select_render_lch_chroma_lightness
};

static const Babl *fish_rgb_to_lch    = NULL;
static const Babl *fish_lch_to_rgb    = NULL;
static const Babl *fish_lch_to_rgb_u8 = NULL;


static void
gimp_color_select_class_init (GimpColorSelectClass *klass)
{
  GObjectClass           *object_class   = G_OBJECT_CLASS (klass);
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  object_class->finalize                = gimp_color_select_finalize;

  selector_class->name                  = "GIMP";
  selector_class->help_id               = "gimp-colorselector-gimp";
  selector_class->icon_name             = GIMP_ICON_WILBER;
  selector_class->set_toggles_visible   = gimp_color_select_togg_visible;
  selector_class->set_toggles_sensitive = gimp_color_select_togg_sensitive;
  selector_class->set_color             = gimp_color_select_set_color;
  selector_class->set_channel           = gimp_color_select_set_channel;
  selector_class->set_model_visible     = gimp_color_select_set_model_visible;
  selector_class->set_config            = gimp_color_select_set_config;

  fish_rgb_to_lch    = babl_fish (babl_format ("R'G'B'A double"),
                                  babl_format ("CIE LCH(ab) double"));
  fish_lch_to_rgb    = babl_fish (babl_format ("CIE LCH(ab) double"),
                                  babl_format ("R'G'B' double"));
  fish_lch_to_rgb_u8 = babl_fish (babl_format ("CIE LCH(ab) double"),
                                  babl_format ("R'G'B' u8"));
}

static void
gimp_color_select_init (GimpColorSelect *select)
{
  GimpColorSelector      *selector = GIMP_COLOR_SELECTOR (select);
  GtkWidget              *hbox;
  GtkWidget              *frame;
  GtkWidget              *vbox;
  GEnumClass             *model_class;
  GEnumClass             *channel_class;
  GimpEnumDesc           *enum_desc;
  GimpColorSelectorModel  model;
  GSList                 *group = NULL;

  /* Default values. */
  select->z_color_fill  = COLOR_SELECT_HUE;
  select->xy_color_fill = COLOR_SELECT_SATURATION_VALUE;
  select->drag_mode     = DRAG_NONE;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (select), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /*  The x/y component preview  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  select->xy_color = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (select->xy_color), FALSE);
  g_object_add_weak_pointer (G_OBJECT (select->xy_color),
                             (gpointer) &select->xy_color);
  gtk_widget_set_size_request (select->xy_color,
                               GIMP_COLOR_SELECTOR_SIZE,
                               GIMP_COLOR_SELECTOR_SIZE);
  gtk_widget_set_events (select->xy_color, COLOR_AREA_EVENT_MASK);
  gtk_container_add (GTK_CONTAINER (frame), select->xy_color);
  gtk_widget_show (select->xy_color);

  g_signal_connect (select->xy_color, "size-allocate",
                    G_CALLBACK (gimp_color_select_xy_size_allocate),
                    select);
  g_signal_connect_after (select->xy_color, "draw",
                          G_CALLBACK (gimp_color_select_xy_draw),
                          select);
  g_signal_connect (select->xy_color, "event",
                    G_CALLBACK (gimp_color_select_xy_events),
                    select);

#if 0
  gimp_dnd_color_dest_add (select->xy_color, gimp_color_select_drop_color,
                           select);
#endif

  /*  The z component preview  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  select->z_color = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (select->z_color), FALSE);
  g_object_add_weak_pointer (G_OBJECT (select->z_color),
                             (gpointer) &select->z_color);
  gtk_widget_set_size_request (select->z_color,
                               GIMP_COLOR_SELECTOR_BAR_SIZE, -1);
  gtk_widget_set_events (select->z_color, COLOR_AREA_EVENT_MASK);
  gtk_container_add (GTK_CONTAINER (frame), select->z_color);
  gtk_widget_show (select->z_color);

  g_signal_connect (select->z_color, "size-allocate",
                    G_CALLBACK (gimp_color_select_z_size_allocate),
                    select);
  g_signal_connect_after (select->z_color, "draw",
                          G_CALLBACK (gimp_color_select_z_draw),
                          select);
  g_signal_connect (select->z_color, "event",
                    G_CALLBACK (gimp_color_select_z_events),
                    select);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  model_class   = g_type_class_ref (GIMP_TYPE_COLOR_SELECTOR_MODEL);
  channel_class = g_type_class_ref (GIMP_TYPE_COLOR_SELECTOR_CHANNEL);

  for (model = GIMP_COLOR_SELECTOR_MODEL_RGB;
       model <= GIMP_COLOR_SELECTOR_MODEL_HSV;
       model++)
    {
      enum_desc = gimp_enum_get_desc (model_class, model);

      select->toggle_box[model] = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
      gtk_box_pack_start (GTK_BOX (vbox), select->toggle_box[model],
                          FALSE, FALSE, 0);

      if (gimp_color_selector_get_model_visible (selector, model))
        gtk_widget_show (select->toggle_box[model]);

      /*  channel toggles  */
      {
        GimpColorSelectorChannel  channel = GIMP_COLOR_SELECTOR_RED;
        GimpColorSelectorChannel  end_channel;

        switch (model)
          {
          case GIMP_COLOR_SELECTOR_MODEL_RGB:
            channel = GIMP_COLOR_SELECTOR_RED;
            break;
          case GIMP_COLOR_SELECTOR_MODEL_LCH:
            channel = GIMP_COLOR_SELECTOR_LCH_LIGHTNESS;
            break;
          case GIMP_COLOR_SELECTOR_MODEL_HSV:
            channel = GIMP_COLOR_SELECTOR_HUE;
            break;
          default:
            /* Should not happen. */
            g_return_if_reached ();
            break;
          }

        end_channel = channel + 3;

        for (; channel < end_channel; channel++)
          {
            GtkWidget *button;

            enum_desc = gimp_enum_get_desc (channel_class, channel);

            button = gtk_radio_button_new_with_mnemonic (group,
                                                         gettext (enum_desc->value_desc));
            group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
            gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
            gtk_box_pack_start (GTK_BOX (select->toggle_box[model]), button,
                                TRUE, TRUE, 0);
            gtk_widget_show (button);

            g_object_set_data (G_OBJECT (button), "channel",
                               GINT_TO_POINTER (channel));

            if (channel == gimp_color_selector_get_channel (selector))
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

            gimp_help_set_help_data (button, gettext (enum_desc->value_help),
                                     NULL);

            g_signal_connect (button, "toggled",
                              G_CALLBACK (gimp_color_select_channel_toggled),
                              select);
          }
      }
    }

  g_type_class_unref (model_class);
  g_type_class_unref (channel_class);
}

static void
gimp_color_select_finalize (GObject *object)
{
  GimpColorSelect *select = GIMP_COLOR_SELECT (object);

  g_clear_pointer (&select->xy_buf, g_free);
  select->xy_width     = 0;
  select->xy_height    = 0;
  select->xy_rowstride = 0;

  g_clear_pointer (&select->z_buf, g_free);
  select->z_width     = 0;
  select->z_height    = 0;
  select->z_rowstride = 0;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_select_togg_visible (GimpColorSelector *selector,
                                gboolean           visible)
{
  GimpColorSelect *select = GIMP_COLOR_SELECT (selector);
  gint             i;

  for (i = 0; i < 3; i++)
    {
      gtk_widget_set_visible (select->toggle_box[i], visible);
    }
}

static void
gimp_color_select_togg_sensitive (GimpColorSelector *selector,
                                  gboolean           sensitive)
{
  GimpColorSelect *select = GIMP_COLOR_SELECT (selector);
  gint             i;

  for (i = 0; i < 3; i++)
    {
      gtk_widget_set_sensitive (select->toggle_box[i], sensitive);
    }
}

static void
gimp_color_select_set_color (GimpColorSelector *selector,
                             const GimpRGB     *rgb,
                             const GimpHSV     *hsv)
{
  GimpColorSelect *select = GIMP_COLOR_SELECT (selector);

  gimp_color_select_update (select,
                            UPDATE_POS | UPDATE_XY_COLOR | UPDATE_Z_COLOR);
}

static void
gimp_color_select_set_channel (GimpColorSelector        *selector,
                               GimpColorSelectorChannel  channel)
{
  GimpColorSelect *select = GIMP_COLOR_SELECT (selector);

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

    case COLOR_SELECT_LCH_LIGHTNESS:
      select->z_color_fill  = COLOR_SELECT_LCH_LIGHTNESS;
      select->xy_color_fill = COLOR_SELECT_LCH_HUE_CHROMA;
      break;

    case COLOR_SELECT_LCH_CHROMA:
      select->z_color_fill  = COLOR_SELECT_LCH_CHROMA;
      select->xy_color_fill = COLOR_SELECT_LCH_HUE_LIGHTNESS;
      break;

     case COLOR_SELECT_LCH_HUE:
      select->z_color_fill  = COLOR_SELECT_LCH_HUE;
      select->xy_color_fill = COLOR_SELECT_LCH_CHROMA_LIGHTNESS;
      break;

    default:
      break;
    }

  gimp_color_select_update (select,
                            UPDATE_POS | UPDATE_Z_COLOR | UPDATE_XY_COLOR);
}

static void
gimp_color_select_set_model_visible (GimpColorSelector      *selector,
                                     GimpColorSelectorModel  model,
                                     gboolean                visible)
{
  GimpColorSelect *select = GIMP_COLOR_SELECT (selector);

  gtk_widget_set_visible (select->toggle_box[model], visible);
}

static void
gimp_color_select_set_config (GimpColorSelector *selector,
                              GimpColorConfig   *config)
{
  GimpColorSelect *select = GIMP_COLOR_SELECT (selector);

  if (config != select->config)
    {
      if (select->config)
        {
          g_signal_handlers_disconnect_by_func (select->config,
                                                gimp_color_select_notify_config,
                                                select);
          g_object_unref (select->config);

          gimp_color_select_destroy_transform (select);
        }

      select->config = config;

      if (select->config)
        {
          g_object_ref (select->config);

          g_signal_connect (select->config, "notify",
                            G_CALLBACK (gimp_color_select_notify_config),
                            select);

          gimp_color_select_notify_config (select->config, NULL, select);
        }
    }
}

static void
gimp_color_select_channel_toggled (GtkWidget       *widget,
                                   GimpColorSelect *select)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GimpColorSelector        *selector = GIMP_COLOR_SELECTOR (select);
      GimpColorSelectorChannel  channel;

      channel = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                    "channel"));

      gimp_color_selector_set_channel (selector, channel);
    }
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
      select->xy_needs_render = TRUE;
      gtk_widget_queue_draw (select->xy_color);
    }

  if (update & UPDATE_Z_COLOR)
    {
      select->z_needs_render = TRUE;
      gtk_widget_queue_draw (select->z_color);
    }

  if (update & UPDATE_CALLER)
    gimp_color_selector_color_changed (GIMP_COLOR_SELECTOR (select));
}

static void
gimp_color_select_update_values (GimpColorSelect *select)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (select);
  GimpLCH            lch;

  switch (select->z_color_fill)
    {
    case COLOR_SELECT_RED:
      selector->rgb.g = select->pos[0];
      selector->rgb.b = select->pos[1];
      selector->rgb.r = select->pos[2];
      break;
    case COLOR_SELECT_GREEN:
      selector->rgb.r = select->pos[0];
      selector->rgb.b = select->pos[1];
      selector->rgb.g = select->pos[2];
      break;
    case COLOR_SELECT_BLUE:
      selector->rgb.r = select->pos[0];
      selector->rgb.g = select->pos[1];
      selector->rgb.b = select->pos[2];
      break;

    case COLOR_SELECT_HUE:
      selector->hsv.s = select->pos[0];
      selector->hsv.v = select->pos[1];
      selector->hsv.h = select->pos[2];
      break;
    case COLOR_SELECT_SATURATION:
      selector->hsv.h = select->pos[0];
      selector->hsv.v = select->pos[1];
      selector->hsv.s = select->pos[2];
      break;
    case COLOR_SELECT_VALUE:
      selector->hsv.h = select->pos[0];
      selector->hsv.s = select->pos[1];
      selector->hsv.v = select->pos[2];
      break;

    case COLOR_SELECT_LCH_LIGHTNESS:
      lch.h = select->pos[0] * 360;
      lch.c = select->pos[1] * 200;
      lch.l = select->pos[2] * 100;
      break;
    case COLOR_SELECT_LCH_CHROMA:
      lch.h = select->pos[0] * 360;
      lch.l = select->pos[1] * 100;
      lch.c = select->pos[2] * 200;
      break;
    case COLOR_SELECT_LCH_HUE:
      lch.c = select->pos[0] * 200;
      lch.l = select->pos[1] * 100;
      lch.h = select->pos[2] * 360;
      break;

    default:
      break;
    }

  switch (select->z_color_fill)
    {
    case COLOR_SELECT_RED:
    case COLOR_SELECT_GREEN:
    case COLOR_SELECT_BLUE:
      gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);
      break;

    case COLOR_SELECT_HUE:
    case COLOR_SELECT_SATURATION:
    case COLOR_SELECT_VALUE:
      gimp_hsv_to_rgb (&selector->hsv, &selector->rgb);
      break;

    case COLOR_SELECT_LCH_LIGHTNESS:
    case COLOR_SELECT_LCH_CHROMA:
    case COLOR_SELECT_LCH_HUE:
      babl_process (fish_lch_to_rgb, &lch, &selector->rgb, 1);
      gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);
      break;

    default:
      break;
    }
}

static void
gimp_color_select_update_pos (GimpColorSelect *select)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (select);
  GimpLCH            lch;

  babl_process (fish_rgb_to_lch, &selector->rgb, &lch, 1);

  switch (select->z_color_fill)
    {
    case COLOR_SELECT_RED:
      select->pos[0] = CLAMP (selector->rgb.g, 0.0, 1.0);
      select->pos[1] = CLAMP (selector->rgb.b, 0.0, 1.0);
      select->pos[2] = CLAMP (selector->rgb.r, 0.0, 1.0);
      break;
    case COLOR_SELECT_GREEN:
      select->pos[0] = CLAMP (selector->rgb.r, 0.0, 1.0);
      select->pos[1] = CLAMP (selector->rgb.b, 0.0, 1.0);
      select->pos[2] = CLAMP (selector->rgb.g, 0.0, 1.0);
      break;
    case COLOR_SELECT_BLUE:
      select->pos[0] = CLAMP (selector->rgb.r, 0.0, 1.0);
      select->pos[1] = CLAMP (selector->rgb.g, 0.0, 1.0);
      select->pos[2] = CLAMP (selector->rgb.b, 0.0, 1.0);
      break;

    case COLOR_SELECT_HUE:
      select->pos[0] = CLAMP (selector->hsv.s, 0.0, 1.0);
      select->pos[1] = CLAMP (selector->hsv.v, 0.0, 1.0);
      select->pos[2] = CLAMP (selector->hsv.h, 0.0, 1.0);
      break;
    case COLOR_SELECT_SATURATION:
      select->pos[0] = CLAMP (selector->hsv.h, 0.0, 1.0);
      select->pos[1] = CLAMP (selector->hsv.v, 0.0, 1.0);
      select->pos[2] = CLAMP (selector->hsv.s, 0.0, 1.0);
      break;
    case COLOR_SELECT_VALUE:
      select->pos[0] = CLAMP (selector->hsv.h, 0.0, 1.0);
      select->pos[1] = CLAMP (selector->hsv.s, 0.0, 1.0);
      select->pos[2] = CLAMP (selector->hsv.v, 0.0, 1.0);
      break;

    case COLOR_SELECT_LCH_LIGHTNESS:
      select->pos[0] = CLAMP (lch.h / 360, 0.0, 1.0);
      select->pos[1] = CLAMP (lch.c / 200, 0.0, 1.0);
      select->pos[2] = CLAMP (lch.l / 100, 0.0, 1.0);
      break;
    case COLOR_SELECT_LCH_CHROMA:
      select->pos[0] = CLAMP (lch.h / 360, 0.0, 1.0);
      select->pos[1] = CLAMP (lch.l / 100, 0.0, 1.0);
      select->pos[2] = CLAMP (lch.c / 200, 0.0, 1.0);
      break;
    case COLOR_SELECT_LCH_HUE:
      select->pos[0] = CLAMP (lch.c / 200, 0.0, 1.0);
      select->pos[1] = CLAMP (lch.l / 100, 0.0, 1.0);
      select->pos[2] = CLAMP (lch.h / 360, 0.0, 1.0);
      break;

    default:
      break;
    }
}

#if 0
static void
gimp_color_select_drop_color (GtkWidget     *widget,
                              gint           x,
                              gint           y,
                              const GimpRGB *color,
                              gpointer       data)
{
  GimpColorSelect *select = GIMP_COLOR_SELECT (data);

  select->rgb = *color;

  gimp_color_select_update_hsv_values (select);
  gimp_color_select_update_lch_values (select);

  gimp_color_select_update (select,
                            UPDATE_POS | UPDATE_XY_COLOR | UPDATE_Z_COLOR |
                            UPDATE_CALLER);
}
#endif

static void
gimp_color_select_xy_size_allocate (GtkWidget       *widget,
                                    GtkAllocation   *allocation,
                                    GimpColorSelect *select)
{
  if (allocation->width  != select->xy_width ||
      allocation->height != select->xy_height)
    {
      select->xy_width  = allocation->width;
      select->xy_height = allocation->height;

      select->xy_rowstride = (select->xy_width * 3 + 3) & ~3;

      g_free (select->xy_buf);
      select->xy_buf = g_new (guchar, select->xy_rowstride * select->xy_height);

      select->xy_needs_render = TRUE;
    }

  gimp_color_select_update (select, UPDATE_XY_COLOR);
}

static gboolean
gimp_color_select_xy_draw (GtkWidget       *widget,
                           cairo_t         *cr,
                           GimpColorSelect *select)
{
  GtkAllocation  allocation;
  GdkPixbuf     *pixbuf;
  gint           x, y;

  if (! select->xy_buf)
    return FALSE;

  if (select->xy_needs_render)
    {
      GimpColorSelector *selector = GIMP_COLOR_SELECTOR (select);

      gimp_color_select_render (select->xy_color,
                                select->xy_buf,
                                select->xy_width,
                                select->xy_height,
                                select->xy_rowstride,
                                select->xy_color_fill,
                                &selector->hsv,
                                &selector->rgb,
                                select->oog_color);
      select->xy_needs_render = FALSE;
    }

  if (! select->transform)
    gimp_color_select_create_transform (select);

  if (select->transform)
    {
      const Babl *format = babl_format ("R'G'B' u8");
      guchar     *buf    = g_new (guchar,
                                  select->xy_rowstride * select->xy_height);
      guchar     *src    = select->xy_buf;
      guchar     *dest   = buf;
      gint        i;

      for (i = 0; i < select->xy_height; i++)
        {
          gimp_color_transform_process_pixels (select->transform,
                                               format, src,
                                               format, dest,
                                               select->xy_width);

          src  += select->xy_rowstride;
          dest += select->xy_rowstride;
        }

      pixbuf = gdk_pixbuf_new_from_data (buf,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         select->xy_width,
                                         select->xy_height,
                                         select->xy_rowstride,
                                         (GdkPixbufDestroyNotify) g_free, NULL);
    }
  else
    {
      pixbuf = gdk_pixbuf_new_from_data (select->xy_buf,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         select->xy_width,
                                         select->xy_height,
                                         select->xy_rowstride,
                                         NULL, NULL);
    }

  gtk_widget_get_allocation (select->xy_color, &allocation);

  gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
  g_object_unref (pixbuf);
  cairo_paint (cr);

  x = (allocation.width  - 1) * select->pos[0];
  y = (allocation.height - 1) - (allocation.height - 1) * select->pos[1];

  cairo_move_to (cr, 0,                y + 0.5);
  cairo_line_to (cr, allocation.width, y + 0.5);

  cairo_move_to (cr, x + 0.5, 0);
  cairo_line_to (cr, x + 0.5, allocation.height);

  cairo_set_line_width (cr, 3.0);
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
  cairo_stroke_preserve (cr);

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_stroke (cr);

  return TRUE;
}

static gboolean
gimp_color_select_xy_events (GtkWidget       *widget,
                             GdkEvent        *event,
                             GimpColorSelect *select)
{
  GtkAllocation allocation;
  gdouble       x, y;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;

        if (select->drag_mode != DRAG_NONE || bevent->button != 1)
          return FALSE;

        x = bevent->x;
        y = bevent->y;

        gtk_grab_add (widget);
        select->drag_mode = DRAG_XY;
      }
      break;

    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;

        if (select->drag_mode != DRAG_XY || bevent->button != 1)
          return FALSE;

        x = bevent->x;
        y = bevent->y;

        gtk_grab_remove (widget);
        select->drag_mode = DRAG_NONE;
      }
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *mevent = (GdkEventMotion *) event;

        if (select->drag_mode != DRAG_XY)
          return FALSE;

        x = mevent->x;
        y = mevent->y;
      }
      break;

    default:
      return FALSE;
    }

  gtk_widget_get_allocation (select->xy_color, &allocation);

  if (allocation.width > 1 && allocation.height > 1)
    {
      select->pos[0] = x  / (allocation.width - 1);
      select->pos[1] = 1.0 - y / (allocation.height - 1);
    }

  select->pos[0] = CLAMP (select->pos[0], 0.0, 1.0);
  select->pos[1] = CLAMP (select->pos[1], 0.0, 1.0);

  gtk_widget_queue_draw (select->xy_color);

  gimp_color_select_update (select, UPDATE_VALUES | UPDATE_CALLER);

  /* Ask for more motion events in case the event was a hint */
  gdk_event_request_motions ((GdkEventMotion *) event);

  return TRUE;
}

static void
gimp_color_select_z_size_allocate (GtkWidget       *widget,
                                   GtkAllocation   *allocation,
                                   GimpColorSelect *select)
{
  if (allocation->width  != select->z_width ||
      allocation->height != select->z_height)
    {
      select->z_width  = allocation->width;
      select->z_height = allocation->height;

      select->z_rowstride = (select->z_width * 3 + 3) & ~3;

      g_free (select->z_buf);
      select->z_buf = g_new (guchar, select->z_rowstride * select->z_height);

      select->z_needs_render = TRUE;
    }

  gimp_color_select_update (select, UPDATE_Z_COLOR);
}

static gboolean
gimp_color_select_z_draw (GtkWidget       *widget,
                          cairo_t         *cr,
                          GimpColorSelect *select)
{
  GtkAllocation  allocation;
  GdkPixbuf     *pixbuf;
  gint           y;

  if (! select->z_buf)
    return FALSE;

  if (select->z_needs_render)
    {
      GimpColorSelector *selector = GIMP_COLOR_SELECTOR (select);

      gimp_color_select_render (select->z_color,
                                select->z_buf,
                                select->z_width,
                                select->z_height,
                                select->z_rowstride,
                                select->z_color_fill,
                                &selector->hsv,
                                &selector->rgb,
                                select->oog_color);
      select->z_needs_render = FALSE;
    }

  gtk_widget_get_allocation (widget, &allocation);

  if (! select->transform)
    gimp_color_select_create_transform (select);

  if (select->transform)
    {
      const Babl *format = babl_format ("R'G'B' u8");
      guchar     *buf    = g_new (guchar,
                                  select->z_rowstride * select->z_height);
      guchar     *src    = select->z_buf;
      guchar     *dest   = buf;
      gint        i;

      for (i = 0; i < select->z_height; i++)
        {
          gimp_color_transform_process_pixels (select->transform,
                                               format, src,
                                               format, dest,
                                               select->z_width);

          src  += select->z_rowstride;
          dest += select->z_rowstride;
        }

      pixbuf = gdk_pixbuf_new_from_data (buf,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         select->z_width,
                                         select->z_height,
                                         select->z_rowstride,
                                         (GdkPixbufDestroyNotify) g_free, NULL);
    }
  else
    {
      pixbuf = gdk_pixbuf_new_from_data (select->z_buf,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         select->z_width,
                                         select->z_height,
                                         select->z_rowstride,
                                         NULL, NULL);
    }

  gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
  g_object_unref (pixbuf);
  cairo_paint (cr);

  y = (allocation.height - 1) - (allocation.height - 1) * select->pos[2];

  cairo_move_to (cr, 0,                y + 0.5);
  cairo_line_to (cr, allocation.width, y + 0.5);

  cairo_set_line_width (cr, 3.0);
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
  cairo_stroke_preserve (cr);

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_stroke (cr);

  return TRUE;
}

static gboolean
gimp_color_select_z_events (GtkWidget       *widget,
                            GdkEvent        *event,
                            GimpColorSelect *select)
{
  GtkAllocation allocation;
  gdouble       z;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;

        if (select->drag_mode != DRAG_NONE || bevent->button != 1)
          return FALSE;

        z = bevent->y;

        gtk_grab_add (widget);
        select->drag_mode = DRAG_Z;
      }
      break;

    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;

        if (select->drag_mode != DRAG_Z || bevent->button != 1)
          return FALSE;

        z = bevent->y;

        gtk_grab_remove (widget);
        select->drag_mode = DRAG_NONE;
      }
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *mevent = (GdkEventMotion *) event;

        if (select->drag_mode != DRAG_Z)
          return FALSE;

        z = mevent->y;
      }
      break;

    default:
      return FALSE;
    }

  gtk_widget_get_allocation (select->z_color, &allocation);

  if (allocation.height > 1)
    select->pos[2] = 1.0 - z / (allocation.height - 1);

  select->pos[2] = CLAMP (select->pos[2], 0.0, 1.0);

  gtk_widget_queue_draw (select->z_color);

  gimp_color_select_update (select,
                            UPDATE_VALUES | UPDATE_XY_COLOR | UPDATE_CALLER);

  /* Ask for more motion events in case the event was a hint */
  gdk_event_request_motions ((GdkEventMotion *) event);

  return TRUE;
}

static void
gimp_color_select_render (GtkWidget           *preview,
                          guchar              *buf,
                          gint                 width,
                          gint                 height,
                          gint                 rowstride,
                          ColorSelectFillType  fill_type,
                          const GimpHSV       *hsv,
                          const GimpRGB       *rgb,
                          const guchar        *oog_color)
{
  ColorSelectFill  csf;

  csf.width       = width;
  csf.height      = height;
  csf.hsv         = *hsv;
  csf.rgb         = *rgb;
  csf.render_line = render_funcs[fill_type];

  csf.oog_color[0] = oog_color[0];
  csf.oog_color[1] = oog_color[1];
  csf.oog_color[2] = oog_color[2];

  babl_process (fish_rgb_to_lch, rgb, &csf.lch, 1);

  for (csf.y = 0; csf.y < csf.height; csf.y++)
    {
      csf.buffer = buf;

      csf.render_line (&csf);

      buf += rowstride;
    }
}

static void
color_select_render_red (ColorSelectFill *csf)
{
  guchar *p = csf->buffer;
  gint    i, r;

  r = (csf->height - csf->y + 1) * 255 / csf->height;
  r = CLAMP (r, 0, 255);

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = 0;
      *p++ = 0;
    }
}

static void
color_select_render_green (ColorSelectFill *csf)
{
  guchar *p = csf->buffer;
  gint    i, g;

  g = (csf->height - csf->y + 1) * 255 / csf->height;
  g = CLAMP (g, 0, 255);

  for (i = 0; i < csf->width; i++)
    {
      *p++ = 0;
      *p++ = g;
      *p++ = 0;
    }
}

static void
color_select_render_blue (ColorSelectFill *csf)
{
  guchar *p = csf->buffer;
  gint    i, b;

  b = (csf->height - csf->y + 1) * 255 / csf->height;
  b = CLAMP (b, 0, 255);

  for (i = 0; i < csf->width; i++)
    {
      *p++ = 0;
      *p++ = 0;
      *p++ = b;
    }
}

static void
color_select_render_hue (ColorSelectFill *csf)
{
  guchar *p = csf->buffer;
  gfloat  h, f;
  gint    r, g, b;
  gint    i;

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
color_select_render_saturation (ColorSelectFill *csf)
{
  guchar *p = csf->buffer;
  gint    s;
  gint    i;

  s = csf->y * 255 / csf->height;
  s = CLAMP (s, 0, 255);

  s = 255 - s;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = s;
      *p++ = s;
      *p++ = s;
    }
}

static void
color_select_render_value (ColorSelectFill *csf)
{
  guchar *p = csf->buffer;
  gint    v;
  gint    i;

  v = csf->y * 255 / csf->height;
  v = CLAMP (v, 0, 255);

  v = 255 - v;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = v;
      *p++ = v;
      *p++ = v;
    }
}

static void
color_select_render_lch_lightness (ColorSelectFill *csf)
{
  guchar  *p   = csf->buffer;
  GimpLCH  lch = { 0.0, 0.0, 0.0, 1.0 };
  guchar   rgb[3];
  gint     i;

  lch.l = (csf->height - 1 - csf->y) * 100.0 / csf->height;
  babl_process (fish_lch_to_rgb_u8, &lch, &rgb, 1);

  for (i = 0; i < csf->width; i++)
    {
      *p++ = rgb[0];
      *p++ = rgb[1];
      *p++ = rgb[2];
    }
}

static void
color_select_render_lch_chroma (ColorSelectFill *csf)
{
  guchar  *p   = csf->buffer;
  GimpLCH  lch = { 80.0, 0.0, 0.0, 1.0 };
  guchar   rgb[3];
  gint     i;

  lch.c = (csf->height - 1 - csf->y) * 200.0 / csf->height ;
  babl_process (fish_lch_to_rgb_u8, &lch, &rgb, 1);

  for (i = 0; i < csf->width; i++)
    {
      *p++ = rgb[0];
      *p++ = rgb[1];
      *p++ = rgb[2];
    }
}

static void
color_select_render_lch_hue (ColorSelectFill *csf)
{
  guchar  *p   = csf->buffer;
  GimpLCH  lch = { 80.0, 200.0, 0.0, 1.0 };
  guchar   rgb[3];
  gint     i;

  lch.h = (csf->height - 1 - csf->y) * 360.0 / csf->height;
  babl_process (fish_lch_to_rgb_u8, &lch, &rgb, 1);

  for (i = 0; i < csf->width; i++)
    {
      *p++ = rgb[0];
      *p++ = rgb[1];
      *p++ = rgb[2];
    }
}

static void
color_select_render_red_green (ColorSelectFill *csf)
{
  guchar *p  = csf->buffer;
  gfloat  r  = 0;
  gfloat  g  = 0;
  gfloat  b  = 0;
  gfloat  dr = 0;
  gint    i;

  b = csf->rgb.b * 255.0;

  if (b < 0.0 || b > 255.0)
    {
      r = csf->oog_color[0];
      g = csf->oog_color[1];
      b = csf->oog_color[2];
    }
  else
    {
      g = (csf->height - csf->y + 1) * 255.0 / csf->height;

      dr = 255.0 / csf->width;
    }

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;

      r += dr;
    }
}

static void
color_select_render_red_blue (ColorSelectFill *csf)
{
  guchar *p  = csf->buffer;
  gfloat  r  = 0;
  gfloat  g  = 0;
  gfloat  b  = 0;
  gfloat  dr = 0;
  gint    i;

  g = csf->rgb.g * 255.0;

  if (g < 0.0 || g > 255.0)
    {
      r = csf->oog_color[0];
      g = csf->oog_color[1];
      b = csf->oog_color[2];
    }
  else
    {
      b = (csf->height - csf->y + 1) * 255.0 / csf->height;

      dr = 255.0 / csf->width;
    }

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;

      r += dr;
    }
}

static void
color_select_render_green_blue (ColorSelectFill *csf)
{
  guchar *p  = csf->buffer;
  gfloat  r  = 0;
  gfloat  g  = 0;
  gfloat  b  = 0;
  gfloat  dg = 0;
  gint    i;

  r = csf->rgb.r * 255.0;

  if (r < 0.0 || r > 255.0)
    {
      r = csf->oog_color[0];
      g = csf->oog_color[1];
      b = csf->oog_color[2];
    }
  else
    {
      b = (csf->height - csf->y + 1) * 255.0 / csf->height;

      dg = 255.0 / csf->width;
    }

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;

      g += dg;
    }
}

static void
color_select_render_hue_saturation (ColorSelectFill *csf)
{
  guchar *p = csf->buffer;
  gfloat  h, dh, s, v;
  gint    f;
  gint    i;

  v = csf->hsv.v;

  s = (gfloat) csf->y / csf->height;
  s = CLAMP (s, 0.0, 1.0);
  s = 1.0 - s;

  h = 0;
  dh = 360.0 / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      gfloat r, g, b;

      f = ((h / 60) - (int) (h / 60)) * 255;

      switch ((int) (h / 60))
        {
        default:
        case 0:
          r = v * 255;
          g = v * (255 - (s * (255 - f)));
          b = v * 255 * (1 - s);
          break;
        case 1:
          r = v * (255 - s * f);
          g = v * 255;
          b = v * 255 * (1 - s);
          break;
        case 2:
          r = v * 255 * (1 - s);
          g = v *255;
          b = v * (255 - (s * (255 - f)));
          break;
        case 3:
          r = v * 255 * (1 - s);
          g = v * (255 - s * f);
          b = v * 255;
          break;
        case 4:
          r = v * (255 - (s * (255 - f)));
          g = v * (255 * (1 - s));
          b = v * 255;
          break;
        case 5:
          r = v * 255;
          g = v * 255 * (1 - s);
          b = v * (255 - s * f);
          break;
        }

      if (r < 0.0 || r > 255.0 ||
          g < 0.0 || g > 255.0 ||
          b < 0.0 || b > 255.0)
        {
          *p++ = csf->oog_color[0];
          *p++ = csf->oog_color[1];
          *p++ = csf->oog_color[2];
        }
      else
        {
          *p++ = r;
          *p++ = g;
          *p++ = b;
        }

      h += dh;
    }
}

static void
color_select_render_hue_value (ColorSelectFill *csf)
{
  guchar *p = csf->buffer;
  gfloat  h, dh, s, v;
  gint    f;
  gint    i;

  s = csf->hsv.s;

  v = (gfloat) csf->y / csf->height;
  v = CLAMP (v, 0.0, 1.0);
  v = 1.0 - v;

  h = 0;
  dh = 360.0 / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      gfloat r, g, b;

      f = ((h / 60) - (int) (h / 60)) * 255;

      switch ((int) (h / 60))
        {
        default:
        case 0:
          r = v * 255;
          g = v * (255 - (s * (255 - f)));
          b = v * 255 * (1 - s);
          break;
        case 1:
          r = v * (255 - s * f);
          g = v * 255;
          b = v * 255 * (1 - s);
          break;
        case 2:
          r = v * 255 * (1 - s);
          g = v *255;
          b = v * (255 - (s * (255 - f)));
          break;
        case 3:
          r = v * 255 * (1 - s);
          g = v * (255 - s * f);
          b = v * 255;
          break;
        case 4:
          r = v * (255 - (s * (255 - f)));
          g = v * (255 * (1 - s));
          b = v * 255;
          break;
        case 5:
          r = v * 255;
          g = v * 255 * (1 - s);
          b = v * (255 - s * f);
          break;
        }

      if (r < 0.0 || r > 255.0 ||
          g < 0.0 || g > 255.0 ||
          b < 0.0 || b > 255.0)
        {
          *p++ = csf->oog_color[0];
          *p++ = csf->oog_color[1];
          *p++ = csf->oog_color[2];
        }
      else
        {
          *p++ = r;
          *p++ = g;
          *p++ = b;
        }

      h += dh;
    }
}

static void
color_select_render_saturation_value (ColorSelectFill *csf)
{
  guchar *p = csf->buffer;
  gfloat  h, s, ds, v;
  gint    f;
  gint    i;

  h = (gfloat) csf->hsv.h * 360.0;
  if (h >= 360)
    h -= 360;
  h /= 60;
  f = (h - (gint) h) * 255;

  v = (gfloat) csf->y / csf->height;
  v = CLAMP (v, 0.0, 1.0);
  v = 1.0 - v;

  s = 0;
  ds = 1.0 / csf->width;

  switch ((gint) h)
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
color_select_render_lch_chroma_lightness (ColorSelectFill *csf)
{
  guchar  *p = csf->buffer;
  GimpLCH  lch;
  gint     i;

  lch.l = (csf->height - 1 - csf->y) * 100.0 / csf->height;
  lch.h = csf->lch.h;

  for (i = 0; i < csf->width; i++)
    {
      GimpRGB rgb;

      lch.c = i * 200.0 / csf->width;

      babl_process (fish_lch_to_rgb, &lch, &rgb, 1);

      if (rgb.r < 0.0 || rgb.r > 1.0 ||
          rgb.g < 0.0 || rgb.g > 1.0 ||
          rgb.b < 0.0 || rgb.b > 1.0)
        {
          p[0] = csf->oog_color[0];
          p[1] = csf->oog_color[1];
          p[2] = csf->oog_color[2];
        }
      else
        {
          gimp_rgb_get_uchar (&rgb, p, p + 1, p + 2);
        }

      p += 3;
    }
}

static void
color_select_render_lch_hue_lightness (ColorSelectFill *csf)
{
  guchar  *p = csf->buffer;
  GimpLCH  lch;
  gint     i;

  lch.l = (csf->height - 1 - csf->y) * 100.0 / csf->height;
  lch.c = csf->lch.c;

  for (i = 0; i < csf->width; i++)
    {
      GimpRGB rgb;

      lch.h = i * 360.0 / csf->width;

      babl_process (fish_lch_to_rgb, &lch, &rgb, 1);

      if (rgb.r < 0.0 || rgb.r > 1.0 ||
          rgb.g < 0.0 || rgb.g > 1.0 ||
          rgb.b < 0.0 || rgb.b > 1.0)
        {
          p[0] = csf->oog_color[0];
          p[1] = csf->oog_color[1];
          p[2] = csf->oog_color[2];
        }
      else
        {
          gimp_rgb_get_uchar (&rgb, p, p + 1, p + 2);
        }

      p += 3;
    }
}

static void
color_select_render_lch_hue_chroma (ColorSelectFill *csf)
{
  guchar  *p = csf->buffer;
  GimpLCH  lch;
  gint     i;

  lch.l = csf->lch.l;
  lch.c = (csf->height - 1 - csf->y) * 200.0 / csf->height;

  for (i = 0; i < csf->width; i++)
    {
      GimpRGB rgb;

      lch.h = i * 360.0 / csf->width;

      babl_process (fish_lch_to_rgb, &lch, &rgb, 1);

      if (rgb.r < 0.0 || rgb.r > 1.0 ||
          rgb.g < 0.0 || rgb.g > 1.0 ||
          rgb.b < 0.0 || rgb.b > 1.0)
        {
          p[0] = csf->oog_color[0];
          p[1] = csf->oog_color[1];
          p[2] = csf->oog_color[2];
        }
      else
        {
          gimp_rgb_get_uchar (&rgb, p, p + 1, p + 2);
        }

      p += 3;
    }
}

static void
gimp_color_select_create_transform (GimpColorSelect *select)
{
  if (select->config)
    {
      static GimpColorProfile *profile = NULL;

      const Babl *format = babl_format ("cairo-RGB24");

      if (G_UNLIKELY (! profile))
        profile = gimp_color_profile_new_rgb_srgb ();

      select->transform = gimp_widget_get_color_transform (GTK_WIDGET (select),
                                                           select->config,
                                                           profile,
                                                           format,
                                                           format);
    }
}

static void
gimp_color_select_destroy_transform (GimpColorSelect *select)
{
  if (select->transform)
    {
      g_object_unref (select->transform);
      select->transform = NULL;
    }

  gtk_widget_queue_draw (select->xy_color);
  gtk_widget_queue_draw (select->z_color);
}

static void
gimp_color_select_notify_config (GimpColorConfig  *config,
                                 const GParamSpec *pspec,
                                 GimpColorSelect  *select)
{
  gimp_color_select_destroy_transform (select);

  gimp_rgb_get_uchar (&config->out_of_gamut_color,
                      select->oog_color,
                      select->oog_color + 1,
                      select->oog_color + 2);
  select->xy_needs_render = TRUE;
  select->z_needs_render  = TRUE;
}
