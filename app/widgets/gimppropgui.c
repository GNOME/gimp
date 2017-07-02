/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui.c
 * Copyright (C) 2002-2017  Michael Natterer <mitch@gimp.org>
 *                          Sven Neumann <sven@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <gegl.h>
#include <gegl-paramspecs.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimpcontext.h"

#include "gimpcolorpanel.h"
#include "gimpmessagebox.h"
#include "gimpspinscale.h"
#include "gimppropgui.h"
#include "gimppropgui-channel-mixer.h"
#include "gimppropgui-color-balance.h"
#include "gimppropgui-color-rotate.h"
#include "gimppropgui-convolution-matrix.h"
#include "gimppropgui-diffration-patterns.h"
#include "gimppropgui-spiral.h"
#include "gimppropgui-eval.h"
#include "gimppropgui-generic.h"
#include "gimppropgui-hue-saturation.h"
#include "gimppropwidgets.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define HAS_KEY(p,k,v) gimp_gegl_param_spec_has_key (p, k, v)


static GtkWidget   * gimp_prop_kelvin_presets_new      (GObject        *config,
                                                        const gchar    *property_name);
static void          gimp_prop_widget_new_seed_clicked (GtkButton      *button,
                                                        GtkAdjustment  *adj);
static gboolean      gimp_prop_string_to_boolean       (GBinding       *binding,
                                                        const GValue   *from_value,
                                                        GValue         *to_value,
                                                        gpointer        user_data);
static void          gimp_prop_config_notify           (GObject        *config,
                                                        GParamSpec     *pspec,
                                                        GtkWidget      *widget);
static void          gimp_prop_widget_show             (GtkWidget      *widget,
                                                        GObject        *config);
static void          gimp_prop_free_label_ref          (GWeakRef       *label_ref);

/*  public functions  */

GtkWidget *
gimp_prop_widget_new (GObject                  *config,
                      const gchar              *property_name,
                      GeglRectangle            *area,
                      GimpContext              *context,
                      GimpCreatePickerFunc      create_picker_func,
                      GimpCreateControllerFunc  create_controller_func,
                      gpointer                  creator,
                      const gchar             **label)
{
  GParamSpec *pspec;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        property_name);

  return gimp_prop_widget_new_from_pspec (config, pspec, area, context,
                                          create_picker_func,
                                          create_controller_func,
                                          creator,
                                          label);
}

GtkWidget *
gimp_prop_widget_new_from_pspec (GObject                  *config,
                                 GParamSpec               *pspec,
                                 GeglRectangle            *area,
                                 GimpContext              *context,
                                 GimpCreatePickerFunc      create_picker_func,
                                 GimpCreateControllerFunc  create_controller_func,
                                 gpointer                  creator,
                                 const gchar             **label)
{
  GtkWidget *widget = NULL;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (pspec != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (label != NULL, NULL);

  *label = NULL;

  if (GEGL_IS_PARAM_SPEC_SEED (pspec))
    {
      GtkAdjustment *adj;
      GtkWidget     *spin;
      GtkWidget     *button;

      widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

      spin = gimp_prop_spin_button_new (config, pspec->name,
                                        1.0, 10.0, 0);
      gtk_box_pack_start (GTK_BOX (widget), spin, TRUE, TRUE, 0);
      gtk_widget_show (spin);

      button = gtk_button_new_with_label (_("New Seed"));
      gtk_box_pack_start (GTK_BOX (widget), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin));

      g_signal_connect (button, "clicked",
                        G_CALLBACK (gimp_prop_widget_new_seed_clicked),
                        adj);

      *label = g_param_spec_get_nick (pspec);
    }
  else if (G_IS_PARAM_SPEC_INT (pspec)   ||
           G_IS_PARAM_SPEC_UINT (pspec)  ||
           G_IS_PARAM_SPEC_FLOAT (pspec) ||
           G_IS_PARAM_SPEC_DOUBLE (pspec))
    {
      gdouble lower;
      gdouble upper;
      gdouble step;
      gdouble page;
      gint    digits;

      if (GEGL_IS_PARAM_SPEC_DOUBLE (pspec))
        {
          GeglParamSpecDouble *gspec = GEGL_PARAM_SPEC_DOUBLE (pspec);

          lower  = gspec->ui_minimum;
          upper  = gspec->ui_maximum;
          step   = gspec->ui_step_small;
          page   = gspec->ui_step_big;
          digits = gspec->ui_digits;
        }
      else if (GEGL_IS_PARAM_SPEC_INT (pspec))
        {
          GeglParamSpecInt *gspec = GEGL_PARAM_SPEC_INT (pspec);

          lower  = gspec->ui_minimum;
          upper  = gspec->ui_maximum;
          step   = gspec->ui_step_small;
          page   = gspec->ui_step_big;
          digits = 0;
        }
      else
        {
          gdouble value;

          /* Get the min and max for the given property. */
          _gimp_prop_widgets_get_numeric_values (config, pspec,
                                                 &value, &lower, &upper,
                                                 G_STRFUNC);

          if ((upper - lower <= 1.0) &&
              (G_IS_PARAM_SPEC_FLOAT (pspec) ||
               G_IS_PARAM_SPEC_DOUBLE (pspec)))
            {
              step   = 0.01;
              page   = 0.1;
              digits = 4;
            }
          else if ((upper - lower <= 10.0) &&
                   (G_IS_PARAM_SPEC_FLOAT (pspec) ||
                    G_IS_PARAM_SPEC_DOUBLE (pspec)))
            {
              step   = 0.1;
              page   = 1.0;
              digits = 3;
            }
          else
            {
              step   = 1.0;
              page   = 10.0;
              digits = (G_IS_PARAM_SPEC_FLOAT (pspec) ||
                        G_IS_PARAM_SPEC_DOUBLE (pspec)) ? 2 : 0;
            }
        }

      widget = gimp_prop_spin_scale_new (config, pspec->name, NULL,
                                         step, page, digits);

      if (HAS_KEY (pspec, "unit", "degree") &&
          (upper - lower) == 360.0)
        {
          GtkWidget *hbox;
          GtkWidget *dial;

          gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), TRUE);

          hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

          gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
          gtk_widget_show (widget);

          dial = gimp_prop_angle_dial_new (config, pspec->name);
          gtk_box_pack_start (GTK_BOX (hbox), dial, FALSE, FALSE, 0);
          gtk_widget_show (dial);

          gimp_help_set_help_data (hbox, g_param_spec_get_blurb (pspec), NULL);
          gimp_prop_gui_bind_label (hbox, widget);

          widget = hbox;
        }
      else if (HAS_KEY (pspec, "unit", "kelvin"))
        {
          GtkWidget *hbox;
          GtkWidget *button;

          hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

          gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
          gtk_widget_show (widget);

          button = gimp_prop_kelvin_presets_new (config, pspec->name);
          gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
          gtk_widget_show (button);

          gimp_help_set_help_data (hbox, g_param_spec_get_blurb (pspec), NULL);
          gimp_prop_gui_bind_label (hbox, widget);

          widget = hbox;
        }
      else
        {
          gimp_prop_gui_bind_label (widget, widget);

          if (area)
            {
              if (HAS_KEY (pspec, "unit", "pixel-coordinate") ||
                  HAS_KEY (pspec, "unit", "pixel-distance"))
                {
                  if (HAS_KEY (pspec, "axis", "x"))
                    {
                      g_printerr ("XXX setting width %d on %s\n",
                                  area->width, pspec->name);

                      gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (widget),
                                                        area->x,
                                                        area->x + area->width);
                    }
                  else if (HAS_KEY (pspec, "axis","y"))
                    {
                      g_printerr ("XXX setting height %d on %s\n",
                                  area->height, pspec->name);

                      gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (widget),
                                                        area->y,
                                                        area->y + area->height);
                    }
                }
            }
        }
    }
  else if (G_IS_PARAM_SPEC_STRING (pspec))
    {
      *label = g_param_spec_get_nick (pspec);

      if (GIMP_IS_PARAM_SPEC_CONFIG_PATH (pspec))
        {
          widget =
            gimp_prop_file_chooser_button_new (config, pspec->name,
                                               g_param_spec_get_nick (pspec),
                                               GTK_FILE_CHOOSER_ACTION_OPEN);
        }
      else if (HAS_KEY (pspec, "multiline", "true"))
        {
          GtkTextBuffer *buffer;
          GtkWidget     *view;

          buffer = gimp_prop_text_buffer_new (config, pspec->name, -1);
          view = gtk_text_view_new_with_buffer (buffer);
          g_object_unref (buffer);

          widget = gtk_scrolled_window_new (NULL, NULL);
          gtk_widget_set_size_request (widget, -1, 150);
          gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget),
                                               GTK_SHADOW_IN);
          gtk_container_add (GTK_CONTAINER (widget), view);
          gtk_widget_show (view);
        }
      else if (HAS_KEY (pspec, "error", "true"))
        {
          GtkWidget *l;

          widget = gimp_message_box_new (GIMP_ICON_WILBER_EEK);
          gimp_message_box_set_primary_text (GIMP_MESSAGE_BOX (widget), "%s",
                                             *label);
          gimp_message_box_set_text (GIMP_MESSAGE_BOX (widget), "%s", "");

          l = GIMP_MESSAGE_BOX (widget)->label[1];

          g_object_bind_property (config, pspec->name,
                                  l,  "label",
                                  G_BINDING_SYNC_CREATE);
          g_object_bind_property_full (config, pspec->name,
                                       widget, "visible",
                                       G_BINDING_SYNC_CREATE,
                                       gimp_prop_string_to_boolean,
                                       NULL,
                                       NULL, NULL);
          *label = NULL;
        }
      else
        {
          widget = gimp_prop_entry_new (config, pspec->name, -1);
        }

    }
  else if (G_IS_PARAM_SPEC_BOOLEAN (pspec))
    {
      widget = gimp_prop_check_button_new (config, pspec->name,
                                           g_param_spec_get_nick (pspec));

      gimp_prop_gui_bind_label (widget, widget);
    }
  else if (G_IS_PARAM_SPEC_ENUM (pspec))
    {
      widget = gimp_prop_enum_combo_box_new (config, pspec->name, 0, 0);
      gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (widget),
                                    g_param_spec_get_nick (pspec));

      gimp_prop_gui_bind_label (widget, widget);
    }
  else if (GIMP_IS_PARAM_SPEC_RGB (pspec))
    {
      GtkWidget *button;

      widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

      button = gimp_prop_color_button_new (config, pspec->name,
                                           g_param_spec_get_nick (pspec),
                                           128, 24,
                                           GIMP_COLOR_AREA_SMALL_CHECKS);
      gimp_color_button_set_update (GIMP_COLOR_BUTTON (button), TRUE);
      gimp_color_panel_set_context (GIMP_COLOR_PANEL (button), context);
      gtk_box_pack_start (GTK_BOX (widget), button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      gimp_prop_gui_bind_tooltip (widget, button);

      if (create_picker_func)
        {
          button = create_picker_func (creator,
                                       pspec->name,
                                       GIMP_ICON_COLOR_PICKER_GRAY,
                                       _("Pick color from the image"),
                                       /* pick_abyss = */ FALSE);
          gtk_box_pack_start (GTK_BOX (widget), button, FALSE, FALSE, 0);
          gtk_widget_show (button);
        }

      *label = g_param_spec_get_nick (pspec);
    }
  else
    {
      g_warning ("%s: not supported: %s (%s)\n", G_STRFUNC,
                 g_type_name (G_TYPE_FROM_INSTANCE (pspec)), pspec->name);
    }

  /* if we have any keys for dynamic properties, listen to config's notify
   * signal, and update the properties accordingly.
   */
  if (gegl_param_spec_get_property_key (pspec, "sensitive") ||
      gegl_param_spec_get_property_key (pspec, "visible")   ||
      gegl_param_spec_get_property_key (pspec, "label")     ||
      gegl_param_spec_get_property_key (pspec, "description"))
    {
      g_object_set_data (G_OBJECT (widget), "gimp-prop-pspec", pspec);

      g_signal_connect_object (config, "notify",
                               G_CALLBACK (gimp_prop_config_notify),
                               widget, 0);

      if (gegl_param_spec_get_property_key (pspec, "visible"))
        {
          /* a bit of a hack: if we have a dynamic "visible" property key,
           * connect to the widget's "show" signal, so that we can intercept
           * our caller's gtk_widget_show() call, and keep the widget hidden if
           * necessary.
           */
          g_signal_connect (widget, "show",
                            G_CALLBACK (gimp_prop_widget_show),
                            config);
        }

      /* update all the properties now */
      gimp_prop_config_notify (config, NULL, widget);
    }

  return widget;
}


typedef GtkWidget * (* GimpPropGuiNewFunc) (GObject                  *config,
                                            GParamSpec              **param_specs,
                                            guint                     n_param_specs,
                                            GeglRectangle            *area,
                                            GimpContext              *context,
                                            GimpCreatePickerFunc      create_picker_func,
                                            GimpCreateControllerFunc  create_controller_func,
                                            gpointer                  creator);

static const struct
{
  const gchar        *config_type;
  GimpPropGuiNewFunc  gui_new_func;
}

gui_new_funcs[] =
{
  { "GimpColorBalanceConfig",
    _gimp_prop_gui_new_color_balance },
  { "GimpHueSaturationConfig",
    _gimp_prop_gui_new_hue_saturation },
  { "GimpGegl-gegl-color-rotate-config",
    _gimp_prop_gui_new_color_rotate },
  { "GimpGegl-gegl-convolution-matrix-config",
    _gimp_prop_gui_new_convolution_matrix },
  { "GimpGegl-gegl-channel-mixer-config",
    _gimp_prop_gui_new_channel_mixer },
  { "GimpGegl-gegl-diffraction-patterns-config",
    _gimp_prop_gui_new_diffraction_patterns },
  { "GimpGegl-gegl-spiral-config",
    _gimp_prop_gui_new_spiral },
  { NULL,
    _gimp_prop_gui_new_generic }
};


GtkWidget *
gimp_prop_gui_new (GObject                  *config,
                   GType                     owner_type,
                   GParamFlags               flags,
                   GeglRectangle            *area,
                   GimpContext              *context,
                   GimpCreatePickerFunc      create_picker_func,
                   GimpCreateControllerFunc  create_controller_func,
                   gpointer                  creator)
{
  GtkWidget    *gui = NULL;
  GParamSpec  **param_specs;
  guint         n_param_specs;
  gint          i, j;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  param_specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                                &n_param_specs);

  for (i = 0, j = 0; i < n_param_specs; i++)
    {
      GParamSpec *pspec = param_specs[i];

      /*  ignore properties of parent classes of owner_type  */
      if (! g_type_is_a (pspec->owner_type, owner_type))
        continue;

      if (flags && ((pspec->flags & flags) != flags))
        continue;

      if (HAS_KEY (pspec, "role", "output-extent"))
        continue;

      param_specs[j] = param_specs[i];
      j++;
    }

  n_param_specs = j;

  if (n_param_specs > 0)
    {
      const gchar *config_type_name = G_OBJECT_TYPE_NAME (config);

      for (i = 0; i < G_N_ELEMENTS (gui_new_funcs); i++)
        {
          if (! gui_new_funcs[i].config_type ||
              ! strcmp (gui_new_funcs[i].config_type, config_type_name))
            {
              g_printerr ("GUI new func match: %s\n",
                          gui_new_funcs[i].config_type ?
                          gui_new_funcs[i].config_type : "generic fallback");

              gui = gui_new_funcs[i].gui_new_func (config,
                                                   param_specs, n_param_specs,
                                                   area,
                                                   context,
                                                   create_picker_func,
                                                   create_controller_func,
                                                   creator);
              break;
            }
        }
    }
  else
    {
      gui = gtk_label_new (_("This operation has no editable properties"));
      gimp_label_set_attributes (GTK_LABEL (gui),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);
      gtk_misc_set_padding (GTK_MISC (gui), 0, 4);
    }

  g_free (param_specs);

  return gui;
}

void
gimp_prop_gui_bind_container (GtkWidget *source,
                              GtkWidget *target)
{
  g_object_bind_property (source, "sensitive",
                          target, "sensitive",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (source, "visible",
                          target, "visible",
                          G_BINDING_SYNC_CREATE);
}

void
gimp_prop_gui_bind_label (GtkWidget *source,
                          GtkWidget *target)
{
  GWeakRef    *label_ref;
  const gchar *label;

  /* we want to update "target"'s "label" property whenever the label
   * expression associated with "source" is reevaluated, however, "source"
   * might not itself have a "label" property we can bind to.  just keep around
   * a reference to "target", and update its label manually.
   */
  g_return_if_fail (g_object_get_data (G_OBJECT (source),
                                       "gimp-prop-label-ref") == NULL);

  label_ref = g_slice_new (GWeakRef);

  g_weak_ref_init (label_ref, target);

  g_object_set_data_full (G_OBJECT (source),
                          "gimp-prop-label-ref", label_ref,
                          (GDestroyNotify) gimp_prop_free_label_ref);

  label = g_object_get_data (G_OBJECT (source), "gimp-prop-label");

  if (label)
    g_object_set (target, "label", label, NULL);

  /* note that "source" might be its own label widget, in which case there's no
   * need to bind the rest of the properties.
   */
  if (source != target)
    gimp_prop_gui_bind_tooltip (source, target);
}

void
gimp_prop_gui_bind_tooltip (GtkWidget *source,
                            GtkWidget *target)
{
  g_object_bind_property (source, "tooltip-text",
                          target, "tooltip-text",
                          G_BINDING_SYNC_CREATE);
}


/*  private functions  */

static void
gimp_prop_kelvin_presets_menu_position (GtkMenu  *menu,
                                        gint     *x,
                                        gint     *y,
                                        gboolean *push_in,
                                        gpointer  user_data)
{
  gimp_button_menu_position (user_data, menu, GTK_POS_LEFT, x, y);
}

static gboolean
gimp_prop_kelvin_presets_button_press (GtkWidget      *widget,
                                       GdkEventButton *bevent,
                                       GtkMenu        *menu)
{
  if (bevent->type == GDK_BUTTON_PRESS)
    {
      gtk_menu_popup (menu,
                      NULL, NULL,
                      gimp_prop_kelvin_presets_menu_position, widget,
                      bevent->button, bevent->time);
    }

  return TRUE;
}

static void
gimp_prop_kelvin_presets_activate (GtkWidget *widget,
                                   GObject   *config)
{
  const gchar *property_name;
  gdouble     *kelvin;

  property_name = g_object_get_data (G_OBJECT (widget), "property-name");
  kelvin        = g_object_get_data (G_OBJECT (widget), "kelvin");

  if (property_name && kelvin)
    g_object_set (config, property_name, *kelvin, NULL);
}

static GtkWidget *
gimp_prop_kelvin_presets_new (GObject     *config,
                              const gchar *property_name)
{
  GtkWidget *button;
  GtkWidget *menu;
  gint       i;

  const struct
  {
    gdouble      kelvin;
    const gchar *label;
  }
  kelvin_presets[] =
  {
    { 1700, N_("1,700 K – Match flame") },
    { 1850, N_("1,850 K – Candle flame, sunset/sunrise") },
    { 3000, N_("3,000 K – Soft (or warm) white compact fluorescent lamps") },
    { 3000, N_("3,300 K – Incandescent lamps") },
    { 3200, N_("3,200 K – Studio lamps, photofloods, etc.") },
    { 3350, N_("3,350 K – Studio \"CP\" light") },
    { 4100, N_("4,100 K – Moonlight") },
    { 5000, N_("5,000 K – D50") },
    { 5000, N_("5,000 K – Cool white/daylight compact fluorescent lamps") },
    { 5000, N_("5,000 K – Horizon daylight") },
    { 5500, N_("5,500 K – D55") },
    { 5500, N_("5,500 K – Vertical daylight, electronic flash") },
    { 6200, N_("6,200 K – Xenon short-arc lamp") },
    { 6500, N_("6,500 K – D65") },
    { 6500, N_("6,500 K – Daylight, overcast") },
    { 7500, N_("7,500 K – D75") },
    { 9300, N_("9,300 K") }
  };

  button = gtk_button_new ();
  gtk_widget_set_can_focus (button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_icon_name (GIMP_ICON_MENU_LEFT,
                                                      GTK_ICON_SIZE_MENU));

  menu = gtk_menu_new ();
  gtk_menu_attach_to_widget (GTK_MENU (menu), button, NULL);

  g_signal_connect (button, "button-press-event",
                    G_CALLBACK (gimp_prop_kelvin_presets_button_press),
                    menu);

  for (i = 0; i < G_N_ELEMENTS (kelvin_presets); i++)
    {
      GtkWidget *item;
      gdouble   *kelvin;

      item = gtk_menu_item_new_with_label (gettext (kelvin_presets[i].label));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      g_object_set_data_full (G_OBJECT (item), "property-name",
                              g_strdup (property_name), (GDestroyNotify) g_free);

      kelvin = g_new (gdouble, 1);
      *kelvin = kelvin_presets[i].kelvin;

      g_object_set_data_full (G_OBJECT (item), "kelvin",
                              kelvin, (GDestroyNotify) g_free);

      g_signal_connect (item, "activate",
                        G_CALLBACK (gimp_prop_kelvin_presets_activate),
                        config);

    }

  return button;
}

static void
gimp_prop_widget_new_seed_clicked (GtkButton     *button,
                                   GtkAdjustment *adj)
{
  guint32 value = g_random_int_range (gtk_adjustment_get_lower (adj),
                                      gtk_adjustment_get_upper (adj));

  gtk_adjustment_set_value (adj, value);
}

static gboolean
gimp_prop_string_to_boolean (GBinding     *binding,
                             const GValue *from_value,
                             GValue       *to_value,
                             gpointer      user_data)
{
  const gchar *string = g_value_get_string (from_value);

  g_value_set_boolean (to_value, string && *string);

  return TRUE;
}

static void
gimp_prop_config_notify (GObject    *config,
                         GParamSpec *pspec,
                         GtkWidget  *widget)
{
  GParamSpec *widget_pspec;
  GWeakRef   *label_ref;
  GtkWidget  *label_widget;
  gboolean    sensitive;
  gboolean    visible;
  gchar      *label;
  gchar      *description;

  widget_pspec = g_object_get_data (G_OBJECT (widget), "gimp-prop-pspec");
  label_ref    = g_object_get_data (G_OBJECT (widget), "gimp-prop-label-ref");

  if (label_ref)
    label_widget = g_weak_ref_get (label_ref);
  else
    label_widget = NULL;

  sensitive   = gimp_prop_eval_boolean (config, widget_pspec, "sensitive", TRUE);
  visible     = gimp_prop_eval_boolean (config, widget_pspec, "visible", TRUE);
  label       = gimp_prop_eval_string (config, widget_pspec, "label",
                                       g_param_spec_get_nick (widget_pspec));
  description = gimp_prop_eval_string (config, widget_pspec, "description",
                                       g_param_spec_get_blurb (widget_pspec));

  /* we store the label in (and pass ownership over it to) the widget's
   * "gimp-prop-label" key, so that we can use it to initialize the label
   * widget's label in gimp_prop_gui_bind_label() upon binding.
   */
  g_object_set_data_full (G_OBJECT (widget), "gimp-prop-label", label, g_free);

  g_signal_handlers_block_by_func (widget,
                                   gimp_prop_widget_show, config);

  gtk_widget_set_sensitive (widget, sensitive);
  gtk_widget_set_visible (widget, visible);
  if (label_widget) g_object_set (label_widget, "label", label, NULL);
  gimp_help_set_help_data (widget, description, NULL);

  g_signal_handlers_unblock_by_func (widget,
                                     gimp_prop_widget_show, config);

  g_free (description);

  if (label_widget)
    g_object_unref (label_widget);
}

static void
gimp_prop_widget_show (GtkWidget *widget,
                       GObject   *config)
{
  GParamSpec *widget_pspec;
  gboolean    visible;

  widget_pspec = g_object_get_data (G_OBJECT (widget), "gimp-prop-pspec");

  visible = gimp_prop_eval_boolean (config, widget_pspec, "visible", TRUE);

  gtk_widget_set_visible (widget, visible);
}

static void
gimp_prop_free_label_ref (GWeakRef *label_ref)
{
  g_weak_ref_clear (label_ref);

  g_slice_free (GWeakRef, label_ref);
}
