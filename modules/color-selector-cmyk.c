/* GIMP CMYK ColorSelector using littleCMS
 * Copyright (C) 2006  Sven Neumann <sven@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


/* definitions and variables */

#define COLORSEL_TYPE_CMYK            (colorsel_cmyk_get_type ())
#define COLORSEL_CMYK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLORSEL_TYPE_CMYK, ColorselCmyk))
#define COLORSEL_CMYK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), COLORSEL_TYPE_CMYK, ColorselCmykClass))
#define COLORSEL_IS_CMYK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLORSEL_TYPE_CMYK))
#define COLORSEL_IS_CMYK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), COLORSEL_TYPE_CMYK))


typedef struct _ColorselCmyk      ColorselCmyk;
typedef struct _ColorselCmykClass ColorselCmykClass;

struct _ColorselCmyk
{
  GimpColorSelector   parent_instance;

  GimpColorConfig          *config;
  GimpColorProfile         *simulation_profile;
  GimpColorRenderingIntent  simulation_intent;
  gboolean                  simulation_bpc;

  GtkWidget                *scales[4];
  GtkWidget                *name_label;

  gboolean                  in_destruction;
};

struct _ColorselCmykClass
{
  GimpColorSelectorClass  parent_class;
};


GType         colorsel_cmyk_get_type       (void);

static void   colorsel_cmyk_dispose        (GObject           *object);

static void   colorsel_cmyk_set_color      (GimpColorSelector *selector,
                                            GeglColor         *color);
static void   colorsel_cmyk_set_config     (GimpColorSelector *selector,
                                            GimpColorConfig   *config);
static void   colorsel_cmyk_set_simulation (GimpColorSelector *selector,
                                            GimpColorProfile  *profile,
                                            GimpColorRenderingIntent intent,
                                            gboolean           bpc);

static void   colorsel_cmyk_scale_update   (GimpLabelSpin     *scale,
                                            ColorselCmyk      *module);
static void   colorsel_cmyk_config_changed (ColorselCmyk      *module);


static const GimpModuleInfo colorsel_cmyk_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("CMYK color selector (using color profile)"),
  "Sven Neumann <sven@gimp.org>",
  "v0.1",
  "(c) 2006, released under the GPL",
  "September 2006"
};


G_DEFINE_DYNAMIC_TYPE (ColorselCmyk, colorsel_cmyk,
                       GIMP_TYPE_COLOR_SELECTOR)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &colorsel_cmyk_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  colorsel_cmyk_register_type (module);

  return TRUE;
}

static void
colorsel_cmyk_class_init (ColorselCmykClass *klass)
{
  GObjectClass           *object_class   = G_OBJECT_CLASS (klass);
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  object_class->dispose                  = colorsel_cmyk_dispose;

  selector_class->name                   = _("CMYK");
  selector_class->help_id                = "gimp-colorselector-cmyk";
  selector_class->icon_name              = GIMP_ICON_COLOR_SELECTOR_CMYK;
  selector_class->set_color              = colorsel_cmyk_set_color;
  selector_class->set_config             = colorsel_cmyk_set_config;
  selector_class->set_simulation         = colorsel_cmyk_set_simulation;

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "ColorselCmyk");
}

static void
colorsel_cmyk_class_finalize (ColorselCmykClass *klass)
{
}

static void
colorsel_cmyk_init (ColorselCmyk *module)
{
  GtkWidget *grid;
  gint       i;

  static const gchar * const cmyk_labels[] =
  {
    /* Cyan        */
    N_("_C"),
    /* Magenta     */
    N_("_M"),
    /* Yellow      */
    N_("_Y"),
    /* Key (Black) */
    N_("_K")
  };
  static const gchar * const cmyk_tips[] =
  {
    N_("Cyan"),
    N_("Magenta"),
    N_("Yellow"),
    N_("Black")
  };

  module->config = NULL;

  gtk_box_set_spacing (GTK_BOX (module), 6);

  grid = gtk_grid_new ();

  gtk_grid_set_row_spacing (GTK_GRID (grid), 1);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);

  gtk_box_pack_start (GTK_BOX (module), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  for (i = 0; i < 4; i++)
    {
      module->scales[i] = gimp_scale_entry_new (gettext (cmyk_labels[i]),
                                                0.0, 0.0, 100.0, 0);
      gimp_help_set_help_data (module->scales[i], gettext (cmyk_tips[i]), NULL);

      g_signal_connect (module->scales[i], "value-changed",
                        G_CALLBACK (colorsel_cmyk_scale_update),
                        module);

      gtk_grid_attach (GTK_GRID (grid), module->scales[i], 1, i, 3, 1);
      gtk_widget_show (module->scales[i]);
    }

  module->name_label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (module->name_label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (module->name_label), PANGO_ELLIPSIZE_END);
  gimp_label_set_attributes (GTK_LABEL (module->name_label),
                             PANGO_ATTR_SCALE, PANGO_SCALE_SMALL,
                             -1);
  gtk_box_pack_start (GTK_BOX (module), module->name_label, FALSE, FALSE, 0);
  gtk_widget_show (module->name_label);
}

static void
colorsel_cmyk_dispose (GObject *object)
{
  ColorselCmyk *module = COLORSEL_CMYK (object);

  module->in_destruction = TRUE;

  colorsel_cmyk_set_config (GIMP_COLOR_SELECTOR (object), NULL);
  g_clear_object (&module->simulation_profile);

  G_OBJECT_CLASS (colorsel_cmyk_parent_class)->dispose (object);
}

static void
colorsel_cmyk_set_color (GimpColorSelector *selector,
                         GeglColor         *color)
{
  GimpColorProfile         *cmyk_profile = NULL;
  GimpColorRenderingIntent  intent       = GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  const Babl               *space        = NULL;
  ColorselCmyk             *module       = COLORSEL_CMYK (selector);
  gfloat                    cmyk[4];

  /* Try Image Soft-proofing profile first, then default CMYK profile */
  if (module->simulation_profile)
    cmyk_profile = module->simulation_profile;

  if (! cmyk_profile && GIMP_IS_COLOR_CONFIG (module->config))
    cmyk_profile = gimp_color_config_get_cmyk_color_profile (GIMP_COLOR_CONFIG (module->config),
                                                             NULL);

  if (cmyk_profile && gimp_color_profile_is_cmyk (cmyk_profile))
    {
      intent = module->simulation_intent;

      space = gimp_color_profile_get_space (cmyk_profile, intent,
                                            NULL);
    }

  gegl_color_get_pixel (color, babl_format_with_space ("CMYK float", space), cmyk);

  for (gint i = 0; i < 4; i++)
    {
      g_signal_handlers_block_by_func (module->scales[i],
                                       colorsel_cmyk_scale_update,
                                       module);

      cmyk[i] *= 100.0;
      gimp_label_spin_set_value (GIMP_LABEL_SPIN (module->scales[i]), cmyk[i]);

      g_signal_handlers_unblock_by_func (module->scales[i],
                                         colorsel_cmyk_scale_update,
                                         module);
    }

  if (cmyk_profile && ! module->simulation_profile)
    g_object_unref (cmyk_profile);
}

static void
colorsel_cmyk_set_config (GimpColorSelector *selector,
                          GimpColorConfig   *config)
{
  ColorselCmyk *module = COLORSEL_CMYK (selector);

  if (config != module->config)
    {
      if (module->config)
        g_signal_handlers_disconnect_by_func (module->config,
                                              colorsel_cmyk_config_changed,
                                              module);

      g_set_object (&module->config, config);

      if (module->config)
        g_signal_connect_swapped (module->config, "notify",
                                  G_CALLBACK (colorsel_cmyk_config_changed),
                                  module);

      colorsel_cmyk_config_changed (module);
    }
}

static void
colorsel_cmyk_set_simulation (GimpColorSelector *selector,
                              GimpColorProfile  *profile,
                              GimpColorRenderingIntent intent,
                              gboolean           bpc)
{
  ColorselCmyk     *module = COLORSEL_CMYK (selector);
  GimpColorProfile *cmyk_profile = NULL;
  gchar            *text;

  gtk_label_set_text (GTK_LABEL (module->name_label), _("Profile: (none)"));
  gimp_help_set_help_data (module->name_label, NULL, NULL);

  g_set_object (&module->simulation_profile, profile);

  cmyk_profile = module->simulation_profile;

  if (! cmyk_profile && GIMP_IS_COLOR_CONFIG (module->config))
    cmyk_profile = gimp_color_config_get_cmyk_color_profile (GIMP_COLOR_CONFIG (module->config),
                                                             NULL);

  if (cmyk_profile && gimp_color_profile_is_cmyk (cmyk_profile))
    {
      text = g_strdup_printf (_("Profile: %s"),
                              gimp_color_profile_get_label (cmyk_profile));
      gtk_label_set_text (GTK_LABEL (module->name_label), text);
      g_free (text);

      gimp_help_set_help_data (module->name_label,
                              gimp_color_profile_get_summary (cmyk_profile),
                              NULL);
    }

  module->simulation_intent = intent;
  module->simulation_bpc    = bpc;

  if (! module->in_destruction)
    {
      GeglColor *color = gimp_color_selector_get_color (selector);

      colorsel_cmyk_set_color (selector, color);
      g_object_unref (color);
    }
}

static void
colorsel_cmyk_scale_update (GimpLabelSpin *scale,
                            ColorselCmyk  *module)
{
  GimpColorProfile        *cmyk_profile = NULL;
  GimpColorSelector       *selector     = GIMP_COLOR_SELECTOR (module);
  GimpColorRenderingIntent intent       = GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  const Babl              *space        = NULL;
  GeglColor               *color        = gegl_color_new (NULL);
  gfloat                   cmyk_values[4];

  for (gint i = 0; i < 4; i++)
    cmyk_values[i] = gimp_label_spin_get_value (GIMP_LABEL_SPIN (module->scales[i])) / 100.0;

  if (module->simulation_profile)
    cmyk_profile = module->simulation_profile;

  if (! cmyk_profile && GIMP_IS_COLOR_CONFIG (module->config))
    cmyk_profile = gimp_color_config_get_cmyk_color_profile (GIMP_COLOR_CONFIG (module->config),
                                                             NULL);
  if (cmyk_profile)
    {
      intent = module->simulation_intent;

      space = gimp_color_profile_get_space (cmyk_profile, intent, NULL);
    }

  gegl_color_set_pixel (color, babl_format_with_space ("CMYK float", space), cmyk_values);
  gimp_color_selector_set_color (selector, color);
  g_object_unref (color);

  if (cmyk_profile && ! module->simulation_profile)
    g_object_unref (cmyk_profile);
}

static void
colorsel_cmyk_config_changed (ColorselCmyk *module)
{
  GimpColorSelector       *selector     = GIMP_COLOR_SELECTOR (module);
  GimpColorConfig         *config       = module->config;
  GimpColorProfile        *rgb_profile  = NULL;
  GimpColorProfile        *cmyk_profile = NULL;
  gchar                   *text;

  gtk_label_set_text (GTK_LABEL (module->name_label), _("Profile: (none)"));
  gimp_help_set_help_data (module->name_label, NULL, NULL);

  if (! config)
    goto out;

  if (module->simulation_profile)
    cmyk_profile = module->simulation_profile;

  if (! cmyk_profile && GIMP_IS_COLOR_CONFIG (module->config))
    cmyk_profile = gimp_color_config_get_cmyk_color_profile (GIMP_COLOR_CONFIG (module->config),
                                                             NULL);

  if (! cmyk_profile)
    goto out;

  rgb_profile = gimp_color_profile_new_rgb_srgb ();

  text = g_strdup_printf (_("Profile: %s"),
                          gimp_color_profile_get_label (cmyk_profile));
  gtk_label_set_text (GTK_LABEL (module->name_label), text);
  g_free (text);

  gimp_help_set_help_data (module->name_label,
                           gimp_color_profile_get_summary (cmyk_profile),
                           NULL);

 out:

  if (rgb_profile)
    g_object_unref (rgb_profile);

  if (cmyk_profile && ! module->simulation_profile)
    g_object_unref (cmyk_profile);

  if (! module->in_destruction)
    {
      GeglColor *color = gimp_color_selector_get_color (selector);

      colorsel_cmyk_set_color (selector, color);
      g_object_unref (color);
    }
}
