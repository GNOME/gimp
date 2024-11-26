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
 * <https://www.gnu.org/licenses/>.
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


struct _GimpColorSelect
{
  GimpColorSelector    parent_instance;

  GtkWidget           *toggle_box[3];
  GtkWidget           *label;
  GtkWidget           *simulation_label;

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
  gfloat               z_value;

  ColorSelectDragMode  drag_mode;

  GimpColorConfig     *config;
  const Babl          *format;
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
  gfloat   rgb[3];
  gfloat   hsv[3];
  gfloat   lch[4];
  guchar   oog_color[3];

  ColorSelectRenderFunc render_line;
};


static void       gimp_color_select_finalize               (GObject                  *object);

static void       gimp_color_select_togg_visible           (GimpColorSelector        *selector,
                                                            gboolean                  visible);
static void       gimp_color_select_togg_sensitive         (GimpColorSelector        *selector,
                                                            gboolean                  sensitive);
static void       gimp_color_select_set_color              (GimpColorSelector        *selector,
                                                            GeglColor                *color);
static void       gimp_color_select_set_channel            (GimpColorSelector        *selector,
                                                            GimpColorSelectorChannel  channel);
static void       gimp_color_select_set_model_visible      (GimpColorSelector        *selector,
                                                            GimpColorSelectorModel    model,
                                                            gboolean                  visible);
static void       gimp_color_select_set_format             (GimpColorSelector        *selector,
                                                            const Babl               *format);
static void       gimp_color_select_set_simulation         (GimpColorSelector        *selector,
                                                            GimpColorProfile         *profile,
                                                            GimpColorRenderingIntent  intent,
                                                            gboolean                  bpc);
static void       gimp_color_select_set_config             (GimpColorSelector        *selector,
                                                            GimpColorConfig          *config);
static void       gimp_color_select_simulation             (GimpColorSelector        *selector,
                                                            gboolean                  enabled);

static void       gimp_color_select_channel_toggled        (GtkWidget                *widget,
                                                            GimpColorSelect          *select);

static void       gimp_color_select_update                 (GimpColorSelect          *select,
                                                            ColorSelectUpdateType     type);
static void       gimp_color_select_update_values          (GimpColorSelect          *select);
static void       gimp_color_select_update_pos             (GimpColorSelect          *select);

#if 0
static void       gimp_color_select_drop_color             (GtkWidget          *widget,
                                                            gint                x,
                                                            gint                y,
                                                            GeglColor          *color,
                                                            gpointer            data);
#endif

static void       gimp_color_select_xy_size_allocate       (GtkWidget          *widget,
                                                            GtkAllocation      *allocation,
                                                            GimpColorSelect    *select);
static gboolean   gimp_color_select_xy_draw                (GtkWidget          *widget,
                                                            cairo_t            *cr,
                                                            GimpColorSelect    *select);
static gboolean   gimp_color_select_xy_events              (GtkWidget          *widget,
                                                            GdkEvent           *event,
                                                            GimpColorSelect    *select);
static void       gimp_color_select_z_size_allocate        (GtkWidget          *widget,
                                                            GtkAllocation      *allocation,
                                                            GimpColorSelect    *select);
static gboolean   gimp_color_select_z_draw                 (GtkWidget          *widget,
                                                            cairo_t            *cr,
                                                            GimpColorSelect    *select);
static gboolean   gimp_color_select_z_events               (GtkWidget          *widget,
                                                            GdkEvent           *event,
                                                            GimpColorSelect    *select);

static void       gimp_color_select_render                 (GtkWidget          *widget,
                                                            guchar             *buf,
                                                            gint                width,
                                                            gint                height,
                                                            gint                rowstride,
                                                            ColorSelectFillType fill_type,
                                                            GeglColor          *color,
                                                            const guchar       *oog_color);

static void       color_select_render_red                  (ColorSelectFill *csf);
static void       color_select_render_green                (ColorSelectFill *csf);
static void       color_select_render_blue                 (ColorSelectFill *csf);

static void       color_select_render_hue                  (ColorSelectFill *csf);
static void       color_select_render_saturation           (ColorSelectFill *csf);
static void       color_select_render_value                (ColorSelectFill *csf);

static void       color_select_render_lch_lightness        (ColorSelectFill *csf);
static void       color_select_render_lch_chroma           (ColorSelectFill *csf);
static void       color_select_render_lch_hue              (ColorSelectFill *csf);

static void       color_select_render_red_green            (ColorSelectFill *csf);
static void       color_select_render_red_blue             (ColorSelectFill *csf);
static void       color_select_render_green_blue           (ColorSelectFill *csf);

static void       color_select_render_hue_saturation       (ColorSelectFill *csf);
static void       color_select_render_hue_value            (ColorSelectFill *csf);
static void       color_select_render_saturation_value     (ColorSelectFill *csf);

static void       color_select_render_lch_chroma_lightness (ColorSelectFill *csf);
static void       color_select_render_lch_hue_lightness    (ColorSelectFill *csf);
static void       color_select_render_lch_hue_chroma       (ColorSelectFill *csf);

static void       gimp_color_select_notify_config          (GimpColorConfig  *config,
                                                            const GParamSpec *pspec,
                                                            GimpColorSelect  *select);

static gfloat     gimp_color_select_get_z_value            (GimpColorSelect *select,
                                                            GeglColor       *color);

static void       gimp_color_select_set_label              (GimpColorSelect *select);


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

static const Babl *fish_lch_to_rgb       = NULL;
static const Babl *fish_lch_to_rgb_u8    = NULL;
static const Babl *fish_lch_to_softproof = NULL;
static const Babl *rgbf_format           = NULL;
static const Babl *rgbu_format           = NULL;
static const Babl *hsvf_format           = NULL;
static const Babl *softproof_format      = NULL;


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
  selector_class->set_simulation        = gimp_color_select_set_simulation;
  selector_class->set_format            = gimp_color_select_set_format;
  selector_class->set_config            = gimp_color_select_set_config;
  selector_class->simulation            = gimp_color_select_simulation;

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "GimpColorSelect");

  fish_lch_to_rgb    = babl_fish (babl_format ("CIE LCH(ab) float"),
                                  babl_format ("R'G'B' double"));
  fish_lch_to_rgb_u8 = babl_fish (babl_format ("CIE LCH(ab) float"),
                                  babl_format ("R'G'B' u8"));
  rgbf_format        = babl_format ("R'G'B' float");
  rgbu_format        = babl_format ("R'G'B' u8");
  hsvf_format        = babl_format ("HSV float");
}

static void
gimp_color_select_init (GimpColorSelect *select)
{
  GimpColorSelector      *selector = GIMP_COLOR_SELECTOR (select);
  GtkWidget              *hbox;
  GtkWidget              *frame;
  GtkWidget              *vbox;
  GtkWidget              *grid;
  GEnumClass             *model_class;
  GEnumClass             *channel_class;
  const GimpEnumDesc     *enum_desc;
  GimpColorSelectorModel  model;
  GSList                 *group = NULL;

  /* Default values. */
  select->z_value       = G_MINFLOAT;
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

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 1);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 1);
  gtk_box_pack_start (GTK_BOX (select), grid, FALSE, FALSE, 0);
  gtk_widget_set_visible (grid, TRUE);

  select->label = gtk_label_new (NULL);
  gtk_widget_set_halign (select->label, GTK_ALIGN_START);
  gtk_widget_set_vexpand (select->label, FALSE);
  gtk_label_set_ellipsize (GTK_LABEL (select->label), PANGO_ELLIPSIZE_END);
  gtk_label_set_justify (GTK_LABEL (select->label), GTK_JUSTIFY_LEFT);
  gtk_grid_attach (GTK_GRID (grid), select->label, 0, 0, 1, 1);
  gtk_widget_show (select->label);
  gimp_color_select_set_label (select);

  select->simulation_label = gtk_label_new (NULL);
  gtk_widget_set_halign (select->simulation_label, GTK_ALIGN_START);
  gtk_widget_set_vexpand (select->simulation_label, FALSE);
  gtk_label_set_ellipsize (GTK_LABEL (select->simulation_label), PANGO_ELLIPSIZE_END);
  gtk_label_set_justify (GTK_LABEL (select->simulation_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_markup (GTK_LABEL (select->simulation_label), _("Soft-Proof Profile: <i>unknown</i>"));
  gtk_grid_attach (GTK_GRID (grid), select->simulation_label, 0, 1, 1, 1);

  g_type_class_unref (model_class);
  g_type_class_unref (channel_class);

  /* See z_color_fill and xy_color_fill values at top of the init()
   * function. Make sure the GimpColorSelector code is not out-of-sync.
   */
  gimp_color_selector_set_channel (selector, GIMP_COLOR_SELECTOR_HUE);
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
                             GeglColor         *color)
{
  GimpColorSelect       *select       = GIMP_COLOR_SELECT (selector);
  ColorSelectUpdateType  update_flags = UPDATE_POS;
  gfloat                 z_value;

  z_value = gimp_color_select_get_z_value (select, color);

/* Note: we are not comparing full colors, hence not using
 * gimp_color_is_perceptually_identical(). It's more of a quick comparison on
 * whether the Z channel is different enough that we'd want to redraw the XY
 * preview too.
 */
#define GIMP_RGBA_EPSILON 1e-4
  if (fabs (z_value - select->z_value) > GIMP_RGBA_EPSILON)
    update_flags |= UPDATE_XY_COLOR;
#undef GIMP_RGBA_EPSILON

  gimp_color_select_update (select, update_flags);
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

  gimp_color_select_set_label (select);
}

static void
gimp_color_select_set_model_visible (GimpColorSelector      *selector,
                                     GimpColorSelectorModel  model,
                                     gboolean                visible)
{
  GimpColorSelect *select = GIMP_COLOR_SELECT (selector);

  if (! visible || gimp_color_selector_get_toggles_visible (selector))
    gtk_widget_set_visible (select->toggle_box[model], visible);
}

static void
gimp_color_select_set_format (GimpColorSelector *selector,
                              const Babl        *format)
{
  GimpColorSelect *select = GIMP_COLOR_SELECT (selector);

  if (select->format != format)
    {
      select->format = format;

      rgbf_format        = babl_format_with_space ("R'G'B' float", format);
      rgbu_format        = babl_format_with_space ("R'G'B' u8", format);
      hsvf_format        = babl_format_with_space ("HSV float", format);
      fish_lch_to_rgb    = babl_fish (babl_format ("CIE LCH(ab) float"),
                                      babl_format_with_space ("R'G'B' double", format));
      fish_lch_to_rgb_u8 = babl_fish (babl_format ("CIE LCH(ab) float"),
                                      babl_format_with_space ("R'G'B' u8", format));

      gimp_color_select_set_label (select);

      select->xy_needs_render = TRUE;
      select->z_needs_render  = TRUE;

      gtk_widget_queue_draw (select->xy_color);
      gtk_widget_queue_draw (select->z_color);
      gimp_color_select_update (select,
                                UPDATE_POS | UPDATE_XY_COLOR | UPDATE_Z_COLOR |
                                UPDATE_CALLER);
    }
}

static void
gimp_color_select_set_simulation (GimpColorSelector        *selector,
                                  GimpColorProfile         *profile,
                                  GimpColorRenderingIntent  intent,
                                  gboolean                  bpc)
{
  gimp_color_select_simulation (selector,
                                gimp_color_selector_get_simulation (selector,
                                                                    NULL, NULL, NULL));
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
        }

      g_set_object (&select->config, config);

      if (select->config)
        {
          g_signal_connect (select->config, "notify",
                            G_CALLBACK (gimp_color_select_notify_config),
                            select);

          gimp_color_select_notify_config (select->config, NULL, select);
        }
    }
}

static void
gimp_color_select_simulation (GimpColorSelector *selector,
                              gboolean           enabled)
{
  GimpColorSelect  *select  = GIMP_COLOR_SELECT (selector);
  const Babl       *format  = NULL;
  GimpColorProfile *profile = NULL;

  if (enabled && gimp_color_selector_get_simulation (selector, &profile, NULL, NULL) && profile)
    {
      GError *error = NULL;

      if (gimp_color_profile_is_rgb (profile))
        format = gimp_color_profile_get_format (profile, babl_format ("R'G'B' float"),
                                                GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                &error);
      else if (gimp_color_profile_is_cmyk (profile))
        format = gimp_color_profile_get_format (profile, babl_format ("CMYK float"),
                                                GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                &error);
      else if (gimp_color_profile_is_gray (profile))
        format = gimp_color_profile_get_format (profile, babl_format ("Y' float"),
                                                GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                &error);

      if (error)
        {
          g_printerr ("Color Selector: invalid color profile: %s\n", error->message);
          format = NULL;
          g_clear_error (&error);
        }
    }

  if (format)
    {
      softproof_format      = format;
      fish_lch_to_softproof = babl_fish (babl_format ("CIE LCH(ab) float"), format);

      if (babl_format_get_space (format) == babl_space ("sRGB"))
        {
          gtk_label_set_text (GTK_LABEL (select->simulation_label), _("Soft-Proof Profile: sRGB"));
          gimp_help_set_help_data (select->simulation_label, NULL, NULL);
        }
      else if (profile != NULL)
        {
          gchar *text;

          text = g_strdup_printf (_("Soft-Proof Profile: %s"), gimp_color_profile_get_label (profile));
          gtk_label_set_text (GTK_LABEL (select->simulation_label), text);
          gimp_help_set_help_data (select->simulation_label,
                                   gimp_color_profile_get_summary (profile),
                                   NULL);
          g_free (text);
        }
      else
        {
          gtk_label_set_markup (GTK_LABEL (select->simulation_label), _("Soft-Proof Profile: <i>unknown</i>"));
          gimp_help_set_help_data (select->simulation_label, NULL, NULL);
        }

      select->xy_needs_render = TRUE;
      select->z_needs_render  = TRUE;

      gtk_widget_queue_draw (select->xy_color);
      gtk_widget_queue_draw (select->z_color);

      gtk_widget_show (select->simulation_label);
    }
  else
    {
      softproof_format      = NULL;
      fish_lch_to_softproof = NULL;

      gtk_label_set_markup (GTK_LABEL (select->simulation_label), _("Soft-Proof Profile: <i>unknown</i>"));
      gimp_help_set_help_data (select->simulation_label, NULL, NULL);
      gtk_widget_hide (select->simulation_label);
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
}

static void
gimp_color_select_update_values (GimpColorSelect *select)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (select);
  GeglColor         *color    = gimp_color_selector_get_color (selector);
  gdouble            values[3];
  gfloat             values_float[3];
  const Babl        *rgb_format;
  const Babl        *hsv_format;

  rgb_format = babl_format_with_space ("R'G'B' double", select->format);
  hsv_format = babl_format_with_space ("HSV float", select->format);

  switch (select->z_color_fill)
    {
    case COLOR_SELECT_RED:
      values[0] = select->pos[2];
      values[1] = select->pos[0];
      values[2] = select->pos[1];
      gegl_color_set_pixel (color, rgb_format, values);
      break;
    case COLOR_SELECT_GREEN:
      values[0] = select->pos[0];
      values[1] = select->pos[2];
      values[2] = select->pos[1];
      gegl_color_set_pixel (color, rgb_format, values);
      break;
    case COLOR_SELECT_BLUE:
      gegl_color_set_pixel (color, rgb_format, select->pos);
      break;

    case COLOR_SELECT_HUE:
      values_float[0] = select->pos[2];
      values_float[1] = select->pos[0];
      values_float[2] = select->pos[1];
      gegl_color_set_pixel (color, hsv_format, values_float);
      break;
    case COLOR_SELECT_SATURATION:
      values_float[0] = select->pos[0];
      values_float[1] = select->pos[2];
      values_float[2] = select->pos[1];
      gegl_color_set_pixel (color, hsv_format, values_float);
      break;
    case COLOR_SELECT_VALUE:
      values_float[0] = select->pos[0];
      values_float[1] = select->pos[1];
      values_float[2] = select->pos[2];
      gegl_color_set_pixel (color, hsv_format, values_float);
      break;

    case COLOR_SELECT_LCH_LIGHTNESS:
      values_float[0] = select->pos[2] * 100.0;
      values_float[1] = select->pos[1] * 200.0;
      values_float[2] = select->pos[0] * 360.0;
      gegl_color_set_pixel (color, babl_format ("CIE LCH(ab) float"), values_float);
      break;
    case COLOR_SELECT_LCH_CHROMA:
      values_float[0] = select->pos[1] * 100.0;
      values_float[1] = select->pos[2] * 200.0;
      values_float[2] = select->pos[0] * 360.0;
      gegl_color_set_pixel (color, babl_format ("CIE LCH(ab) float"), values_float);
      break;
    case COLOR_SELECT_LCH_HUE:
      values_float[0] = select->pos[1] * 100.0;
      values_float[1] = select->pos[0] * 200.0;
      values_float[2] = select->pos[2] * 360.0;
      gegl_color_set_pixel (color, babl_format ("CIE LCH(ab) float"), values_float);
      break;

    default:
      break;
    }

  gimp_color_selector_set_color (selector, color);

  g_object_unref (color);
}

static void
gimp_color_select_update_pos (GimpColorSelect *select)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (select);
  GeglColor         *color    = gimp_color_selector_get_color (selector);
  gdouble            rgb[3];
  gfloat             lch[3];
  gfloat             hsv[3];

  gegl_color_get_pixel (color, babl_format_with_space ("R'G'B' double", select->format), rgb);
  gegl_color_get_pixel (color, babl_format_with_space ("HSV float", select->format), hsv);
  gegl_color_get_pixel (color, babl_format ("CIE LCH(ab) float"), lch);
  g_object_unref (color);

  switch (select->z_color_fill)
    {
    case COLOR_SELECT_RED:
      select->pos[0] = CLAMP (rgb[1], 0.0, 1.0);
      select->pos[1] = CLAMP (rgb[2], 0.0, 1.0);
      select->pos[2] = CLAMP (rgb[0], 0.0, 1.0);
      break;
    case COLOR_SELECT_GREEN:
      select->pos[0] = CLAMP (rgb[0], 0.0, 1.0);
      select->pos[1] = CLAMP (rgb[2], 0.0, 1.0);
      select->pos[2] = CLAMP (rgb[1], 0.0, 1.0);
      break;
    case COLOR_SELECT_BLUE:
      select->pos[0] = CLAMP (rgb[0], 0.0, 1.0);
      select->pos[1] = CLAMP (rgb[1], 0.0, 1.0);
      select->pos[2] = CLAMP (rgb[2], 0.0, 1.0);
      break;

    case COLOR_SELECT_HUE:
      select->pos[0] = CLAMP (hsv[1], 0.0, 1.0);
      select->pos[1] = CLAMP (hsv[2], 0.0, 1.0);
      select->pos[2] = CLAMP (hsv[0], 0.0, 1.0);
      break;
    case COLOR_SELECT_SATURATION:
      select->pos[0] = CLAMP (hsv[0], 0.0, 1.0);
      select->pos[1] = CLAMP (hsv[2], 0.0, 1.0);
      select->pos[2] = CLAMP (hsv[1], 0.0, 1.0);
      break;
    case COLOR_SELECT_VALUE:
      select->pos[0] = CLAMP (hsv[0], 0.0, 1.0);
      select->pos[1] = CLAMP (hsv[1], 0.0, 1.0);
      select->pos[2] = CLAMP (hsv[2], 0.0, 1.0);
      break;

    case COLOR_SELECT_LCH_LIGHTNESS:
      select->pos[0] = CLAMP (lch[2] / 360, 0.0, 1.0);
      select->pos[1] = CLAMP (lch[1] / 200, 0.0, 1.0);
      select->pos[2] = CLAMP (lch[0] / 100, 0.0, 1.0);
      break;
    case COLOR_SELECT_LCH_CHROMA:
      select->pos[0] = CLAMP (lch[2] / 360, 0.0, 1.0);
      select->pos[1] = CLAMP (lch[0] / 100, 0.0, 1.0);
      select->pos[2] = CLAMP (lch[1] / 200, 0.0, 1.0);
      break;
    case COLOR_SELECT_LCH_HUE:
      select->pos[0] = CLAMP (lch[1] / 200, 0.0, 1.0);
      select->pos[1] = CLAMP (lch[0] / 100, 0.0, 1.0);
      select->pos[2] = CLAMP (lch[2] / 360, 0.0, 1.0);
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
                              GeglColor     *color,
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
  const Babl    *render_space;
  const Babl    *render_fish;
  guchar        *buf;
  guchar        *src;
  guchar        *dest;
  gint           x, y;

  if (! select->xy_buf)
    return FALSE;

  if (select->xy_needs_render)
    {
      GimpColorSelector *selector = GIMP_COLOR_SELECTOR (select);
      GeglColor         *color    = gimp_color_selector_get_color (selector);

      gimp_color_select_render (select->xy_color,
                                select->xy_buf,
                                select->xy_width,
                                select->xy_height,
                                select->xy_rowstride,
                                select->xy_color_fill,
                                color,
                                select->oog_color);
      select->z_value = gimp_color_select_get_z_value (select, color);
      select->xy_needs_render = FALSE;

      g_object_unref (color);
    }

  render_space = gimp_widget_get_render_space (widget, select->config);
  render_fish  = babl_fish (babl_format_with_space ("R'G'B' u8", select->format),
                            babl_format_with_space ("R'G'B' u8", render_space));

  buf  = g_new (guchar, select->xy_rowstride * select->xy_height);
  src  = select->xy_buf;
  dest = buf;
  for (gint i = 0; i < select->xy_height; i++)
    {
      babl_process (render_fish, src, dest, select->xy_width);

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
  const Babl    *render_space;
  const Babl    *render_fish;
  guchar        *buf;
  guchar        *src;
  guchar        *dest;
  gint           y;

  if (! select->z_buf)
    return FALSE;

  if (select->z_needs_render)
    {
      GimpColorSelector *selector = GIMP_COLOR_SELECTOR (select);
      GeglColor         *color    = gimp_color_selector_get_color (selector);

      gimp_color_select_render (select->z_color,
                                select->z_buf,
                                select->z_width,
                                select->z_height,
                                select->z_rowstride,
                                select->z_color_fill,
                                color,
                                select->oog_color);
      select->z_needs_render = FALSE;

      g_object_unref (color);
    }

  gtk_widget_get_allocation (widget, &allocation);

  render_space = gimp_widget_get_render_space (widget, select->config);
  render_fish  = babl_fish (babl_format_with_space ("R'G'B' u8", select->format),
                            babl_format_with_space ("R'G'B' u8", render_space));

  buf  = g_new (guchar, select->z_rowstride * select->z_height);
  src  = select->z_buf;
  dest = buf;
  for (gint i = 0; i < select->z_height; i++)
    {
      babl_process (render_fish, src, dest, select->z_width);

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
                          GeglColor           *color,
                          const guchar        *oog_color)
{
  ColorSelectFill  csf;

  csf.width       = width;
  csf.height      = height;
  csf.render_line = render_funcs[fill_type];

  gegl_color_get_pixel (color, rgbf_format, csf.rgb);
  gegl_color_get_pixel (color, hsvf_format, csf.hsv);
  gegl_color_get_pixel (color, babl_format ("CIE LCH(ab) alpha float"), csf.lch);

  csf.oog_color[0] = oog_color[0];
  csf.oog_color[1] = oog_color[1];
  csf.oog_color[2] = oog_color[2];

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
  gfloat   lch[4] = { 0.0, 0.0, 0.0, 1.0 };
  guchar   rgb[3];
  gint     i;

  lch[0] = (csf->height - 1 - csf->y) * 100.0 / csf->height;
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
  gfloat   lch[4] = { 80.0, 0.0, 0.0, 1.0 };
  guchar   rgb[3];
  gint     i;

  lch[1] = (csf->height - 1 - csf->y) * 200.0 / csf->height ;
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
  guchar  *p      = csf->buffer;
  gfloat   lch[4] = { 80.0, 200.0, 0.0, 1.0 };
  guchar   rgb[3];
  gint     i;

  lch[2] = (csf->height - 1 - csf->y) * 360.0 / csf->height;
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
  GeglColor *c = NULL;
  guchar    *p = csf->buffer;
  gfloat     dr;
  gfloat     rgb[3];
  gint       i;

  c = gegl_color_new (NULL);

  rgb[0] = 0.f;
  rgb[1] = ((gfloat) (csf->height - csf->y + 1)) / csf->height;
  rgb[2] = csf->rgb[2];

  dr = 1.f / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      gegl_color_set_pixel (c, rgbf_format, rgb);
      if (softproof_format &&
          gimp_color_is_out_of_gamut (c, babl_format_get_space (softproof_format)))
        {
          *p++ = csf->oog_color[0];
          *p++ = csf->oog_color[1];
          *p++ = csf->oog_color[2];
        }
      else
        {
          gegl_color_get_pixel (c, rgbu_format, p);
          p += 3;
        }

      rgb[0] += dr;
    }

  g_object_unref (c);
}

static void
color_select_render_red_blue (ColorSelectFill *csf)
{
  GeglColor *c = NULL;
  guchar    *p = csf->buffer;
  gfloat     rgb[3];
  gfloat     dr;
  gint       i;

  c = gegl_color_new (NULL);

  rgb[0] = 0.f;
  rgb[1] = csf->rgb[1];
  rgb[2] = ((gfloat) (csf->height - csf->y + 1)) / csf->height;

  dr = 1.f / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      gegl_color_set_pixel (c, rgbf_format, rgb);
      if (softproof_format &&
          gimp_color_is_out_of_gamut (c, babl_format_get_space (softproof_format)))
        {
          *p++ = csf->oog_color[0];
          *p++ = csf->oog_color[1];
          *p++ = csf->oog_color[2];
        }
      else
        {
          gegl_color_get_pixel (c, rgbu_format, p);
          p += 3;
        }

      rgb[0] += dr;
    }

  g_object_unref (c);
}

static void
color_select_render_green_blue (ColorSelectFill *csf)
{
  GeglColor *c = NULL;
  guchar    *p = csf->buffer;
  gfloat     rgb[3];
  gfloat     dg;
  gint       i;

  c = gegl_color_new (NULL);

  rgb[0] = csf->rgb[0];
  rgb[1] = 0.f;
  rgb[2] = ((gfloat) (csf->height - csf->y + 1)) / csf->height;

  dg = 1.f / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      gegl_color_set_pixel (c, rgbf_format, rgb);
      if (softproof_format &&
          gimp_color_is_out_of_gamut (c, babl_format_get_space (softproof_format)))
        {
          *p++ = csf->oog_color[0];
          *p++ = csf->oog_color[1];
          *p++ = csf->oog_color[2];
        }
      else
        {
          gegl_color_get_pixel (c, rgbu_format, p);
          p += 3;
        }

      rgb[1] += dg;
    }

  g_object_unref (c);
}

static void
color_select_render_hue_saturation (ColorSelectFill *csf)
{
  GeglColor *c = NULL;
  guchar    *p = csf->buffer;
  gfloat     hsv[3];
  gfloat     dh;

  c = gegl_color_new (NULL);

  hsv[0] = 0.f;
  hsv[1] = (gfloat) csf->y / csf->height;
  hsv[1] = CLAMP (hsv[1], 0.f, 1.f);
  hsv[1] = 1.f - hsv[1];
  hsv[2] = csf->hsv[2];

  dh = 1.f / (gfloat) csf->width;

  for (gint i = 0; i < csf->width; i++)
    {
      gegl_color_set_pixel (c, hsvf_format, hsv);
      if (softproof_format &&
          gimp_color_is_out_of_gamut (c, babl_format_get_space (softproof_format)))
        {
          *p++ = csf->oog_color[0];
          *p++ = csf->oog_color[1];
          *p++ = csf->oog_color[2];
        }
      else
        {
          gegl_color_get_pixel (c, rgbu_format, p);
          p += 3;
        }

      hsv[0] += dh;
    }

  g_object_unref (c);
}

static void
color_select_render_hue_value (ColorSelectFill *csf)
{
  GeglColor *c = NULL;
  guchar    *p = csf->buffer;
  gfloat     hsv[3];
  gfloat     dh;

  c = gegl_color_new (NULL);

  hsv[0] = 0.f;
  hsv[1] = csf->hsv[1];
  hsv[2] = (gfloat) csf->y / csf->height;
  hsv[2] = CLAMP (hsv[2], 0.f, 1.f);
  hsv[2] = 1.f - hsv[2];

  dh = 1.f / (gfloat) csf->width;

  for (gint i = 0; i < csf->width; i++)
    {
      gegl_color_set_pixel (c, hsvf_format, hsv);
      if (softproof_format &&
          gimp_color_is_out_of_gamut (c, babl_format_get_space (softproof_format)))
        {
          *p++ = csf->oog_color[0];
          *p++ = csf->oog_color[1];
          *p++ = csf->oog_color[2];
        }
      else
        {
          gegl_color_get_pixel (c, rgbu_format, p);
          p += 3;
        }

      hsv[0] += dh;
    }

  g_object_unref (c);
}

static void
color_select_render_saturation_value (ColorSelectFill *csf)
{
  GeglColor *c = NULL;
  guchar    *p = csf->buffer;
  gfloat     hsv[3];
  gfloat     ds;

  c = gegl_color_new (NULL);

  hsv[0] = csf->hsv[0];
  hsv[1] = 0.f;
  hsv[2] = 1.f - CLAMP ((gfloat) csf->y / csf->height, 0.f, 1.f);

  ds = 1.f / (gfloat) csf->width;

  for (gint i = 0; i < csf->width; i++)
    {
      gegl_color_set_pixel (c, hsvf_format, hsv);
      if (softproof_format &&
          gimp_color_is_out_of_gamut (c, babl_format_get_space (softproof_format)))
        {
          *p++ = csf->oog_color[0];
          *p++ = csf->oog_color[1];
          *p++ = csf->oog_color[2];
        }
      else
        {
          gegl_color_get_pixel (c, rgbu_format, p);
          p += 3;
        }

      hsv[1] += ds;
    }

  g_object_unref (c);
}

static void
color_select_render_lch_chroma_lightness (ColorSelectFill *csf)
{
  GeglColor *c = NULL;
  guchar    *p = csf->buffer;
  gfloat     lch[3];

  c = gegl_color_new (NULL);

  lch[0] = (csf->height - 1 - csf->y) * 100.0 / csf->height;
  lch[2] = csf->lch[2];

  for (gint i = 0; i < csf->width; i++)
    {
      lch[1] = i * 200.0 / csf->width;

      gegl_color_set_pixel (c, babl_format ("CIE LCH(ab) float"), lch);

      if (gimp_color_is_out_of_gamut (c, babl_format_get_space (rgbf_format)) ||
          (softproof_format &&
           gimp_color_is_out_of_gamut (c, babl_format_get_space (softproof_format))))
        {
          p[0] = csf->oog_color[0];
          p[1] = csf->oog_color[1];
          p[2] = csf->oog_color[2];
        }
      else
        {
          gegl_color_get_pixel (c, rgbu_format, p);
        }

      p += 3;
    }

  g_object_unref (c);
}

static void
color_select_render_lch_hue_lightness (ColorSelectFill *csf)
{
  GeglColor *c = NULL;
  guchar    *p = csf->buffer;
  gfloat     lch[3];

  c = gegl_color_new (NULL);

  lch[0] = (csf->height - 1 - csf->y) * 100.0 / csf->height;
  lch[1] = csf->lch[1];

  for (gint i = 0; i < csf->width; i++)
    {
      lch[2] = i * 360.0 / csf->width;

      gegl_color_set_pixel (c, babl_format ("CIE LCH(ab) float"), lch);

      if (gimp_color_is_out_of_gamut (c, babl_format_get_space (rgbf_format)) ||
          (softproof_format &&
           gimp_color_is_out_of_gamut (c, babl_format_get_space (softproof_format))))
        {
          p[0] = csf->oog_color[0];
          p[1] = csf->oog_color[1];
          p[2] = csf->oog_color[2];
        }
      else
        {
          gegl_color_get_pixel (c, rgbu_format, p);
        }

      p += 3;
    }

  g_object_unref (c);
}

static void
color_select_render_lch_hue_chroma (ColorSelectFill *csf)
{
  GeglColor *c = NULL;
  guchar    *p = csf->buffer;
  gfloat     lch[3];
  gint       i;

  c = gegl_color_new (NULL);

  lch[0] = csf->lch[0];
  lch[1] = (csf->height - 1 - csf->y) * 200.0 / csf->height;

  for (i = 0; i < csf->width; i++)
    {
      lch[2] = i * 360.0 / csf->width;

      gegl_color_set_pixel (c, babl_format ("CIE LCH(ab) float"), lch);

      if (gimp_color_is_out_of_gamut (c, babl_format_get_space (rgbf_format)) ||
          (softproof_format &&
           gimp_color_is_out_of_gamut (c, babl_format_get_space (softproof_format))))
        {
          p[0] = csf->oog_color[0];
          p[1] = csf->oog_color[1];
          p[2] = csf->oog_color[2];
        }
      else
        {
          gegl_color_get_pixel (c, rgbu_format, p);
        }

      p += 3;
    }

  g_object_unref (c);
}

static void
gimp_color_select_notify_config (GimpColorConfig  *config,
                                 const GParamSpec *pspec,
                                 GimpColorSelect  *select)
{
  GeglColor *color;

  color = gimp_color_config_get_out_of_gamut_color (config);
  /* TODO: shouldn't this be color-managed too, using the target space into
   * consideration?
   */
  gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), select->oog_color);
  select->xy_needs_render = TRUE;
  select->z_needs_render  = TRUE;

  g_object_unref (color);
}

static gfloat
gimp_color_select_get_z_value (GimpColorSelect *select,
                               GeglColor       *color)
{
  gint   z_channel = -1;
  gfloat channels[3];

  switch (select->z_color_fill)
    {
    case COLOR_SELECT_RED:
      z_channel = 0;
    case COLOR_SELECT_GREEN:
      z_channel = (z_channel > -1) ? z_channel : 1;
    case COLOR_SELECT_BLUE:
      z_channel = (z_channel > -1) ? z_channel : 2;
      gegl_color_get_pixel (color, rgbf_format, channels);
      break;
    case COLOR_SELECT_HUE:
      z_channel = 0;
    case COLOR_SELECT_SATURATION:
      z_channel = (z_channel > -1) ? z_channel : 1;
    case COLOR_SELECT_VALUE:
      z_channel = (z_channel > -1) ? z_channel : 2;
      gegl_color_get_pixel (color, hsvf_format, channels);
      break;
    case COLOR_SELECT_LCH_LIGHTNESS:
      z_channel = 0;
    case COLOR_SELECT_LCH_CHROMA:
      z_channel = (z_channel > -1) ? z_channel : 1;
    case COLOR_SELECT_LCH_HUE:
      z_channel = (z_channel > -1) ? z_channel : 2;
      gegl_color_get_pixel (color, babl_format ("CIE LCH(ab) float"), channels);
      break;
    default:
      g_return_val_if_reached (G_MINFLOAT);
    }

  return channels[z_channel];
}

static void
gimp_color_select_set_label (GimpColorSelect *select)
{
  GEnumClass               *enum_class;
  const GimpEnumDesc       *enum_desc;
  const gchar              *model_help;
  gchar                    *model_label;
  GimpColorSelectorChannel  channel;
  GimpColorSelectorModel    model      = GIMP_COLOR_SELECTOR_MODEL_RGB;
  gboolean                  with_space = TRUE;

  channel = gimp_color_selector_get_channel (GIMP_COLOR_SELECTOR (select));
  switch (channel)
    {
    case GIMP_COLOR_SELECTOR_HUE:
    case GIMP_COLOR_SELECTOR_SATURATION:
    case GIMP_COLOR_SELECTOR_VALUE:
      model = GIMP_COLOR_SELECTOR_MODEL_HSV;
      break;
    case GIMP_COLOR_SELECTOR_RED:
    case GIMP_COLOR_SELECTOR_GREEN:
    case GIMP_COLOR_SELECTOR_BLUE:
      model = GIMP_COLOR_SELECTOR_MODEL_RGB;
      break;
    case GIMP_COLOR_SELECTOR_LCH_LIGHTNESS:
    case GIMP_COLOR_SELECTOR_LCH_CHROMA:
    case GIMP_COLOR_SELECTOR_LCH_HUE:
      model = GIMP_COLOR_SELECTOR_MODEL_LCH;
      with_space = FALSE;
      break;
    case GIMP_COLOR_SELECTOR_ALPHA:
      /* Let's ignore label change when alpha is selected (it can be
       * used with any model).
       */
      return;
    }

  enum_class  = g_type_class_ref (GIMP_TYPE_COLOR_SELECTOR_MODEL);
  enum_desc   = gimp_enum_get_desc (enum_class, model);
  model_help  = gettext (enum_desc->value_help);
  model_label = g_strdup_printf (_("Model: %s"), gettext (enum_desc->value_desc));

  if (with_space)
    {
      if (select->format == NULL || babl_format_get_space (select->format) == babl_space ("sRGB"))
        {
          gchar *label;

          /* TODO: in future, this could be a better thought localized
           * string, but for now, I had to preserve string freeze.
           */
          label = g_strdup_printf ("%s - %s", model_label, _("Profile: sRGB"));
          gtk_label_set_text (GTK_LABEL (select->label), label);
          gimp_help_set_help_data (select->label, model_help, NULL);
          g_free (label);
        }
      else
        {
          GimpColorProfile *profile = NULL;
          const gchar      *icc;
          gint              icc_len;

          icc = babl_space_get_icc (babl_format_get_space (select->format), &icc_len);
          profile = gimp_color_profile_new_from_icc_profile ((const guint8 *) icc, icc_len, NULL);

          if (profile != NULL)
            {
              gchar *label;
              gchar *text;

              text  = g_strdup_printf (_("Profile: %s"), gimp_color_profile_get_label (profile));
              label = g_strdup_printf ("%s - %s", model_label, text);
              gtk_label_set_text (GTK_LABEL (select->label), label);
              gimp_help_set_help_data (select->label,
                                       gimp_color_profile_get_summary (profile),
                                       NULL);
              g_free (text);
              g_free (label);
            }
          else
            {
              gchar *label;

              label = g_strdup_printf ("%s - %s", model_label, _("Profile: <i>unknown</i>"));
              gtk_label_set_markup (GTK_LABEL (select->label), label);
              gimp_help_set_help_data (select->label, model_help, NULL);
              g_free (label);
            }

          g_clear_object (&profile);
        }
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (select->label), model_label);
      gimp_help_set_help_data (select->label, model_help, NULL);
    }

  g_free (model_label);
}
