/* GIMP CMYK ColorSelector
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


/* definitions and variables */

#define COLORSEL_TYPE_CMYK            (colorsel_cmyk_type)
#define COLORSEL_CMYK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLORSEL_TYPE_CMYK, ColorselCmyk))
#define COLORSEL_CMYK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), COLORSEL_TYPE_CMYK, ColorselCmykClass))
#define COLORSEL_IS_CMYK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLORSEL_TYPE_CMYK))
#define COLORSEL_IS_CMYK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), COLORSEL_TYPE_CMYK))


typedef struct _ColorselCmyk      ColorselCmyk;
typedef struct _ColorselCmykClass ColorselCmykClass;

struct _ColorselCmyk
{
  GimpColorSelector  parent_instance;

  GimpCMYK           cmyk;
  gdouble            pullout;
  GtkAdjustment     *adj[5];
};

struct _ColorselCmykClass
{
  GimpColorSelectorClass  parent_class;
};


G_MODULE_EXPORT const GimpModuleInfo * gimp_module_query    (GTypeModule *module);
G_MODULE_EXPORT gboolean               gimp_module_register (GTypeModule *module);

static GType  colorsel_cmyk_get_type   (GTypeModule       *module);
static void   colorsel_cmyk_class_init (ColorselCmykClass *klass);
static void   colorsel_cmyk_init       (ColorselCmyk      *cmyk);

static void   colorsel_cmyk_set_color      (GimpColorSelector *selector,
                                            const GimpRGB     *rgb,
                                            const GimpHSV     *hsv);
static void   colorsel_cmyk_adj_update     (GtkAdjustment     *adj,
                                            ColorselCmyk      *module);
static void   colorsel_cmyk_pullout_update (GtkAdjustment     *adj,
                                            ColorselCmyk      *module);


static const GimpModuleInfo colorsel_cmyk_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("CMYK color selector"),
  "Sven Neumann <sven@gimp.org>",
  "v0.2",
  "(c) 2003, released under the GPL",
  "July 2003"
};

static GType colorsel_cmyk_type = 0;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &colorsel_cmyk_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  colorsel_cmyk_get_type (module);

  return TRUE;
}

static GType
colorsel_cmyk_get_type (GTypeModule *module)
{
  if (! colorsel_cmyk_type)
    {
      const GTypeInfo select_info =
      {
        sizeof (ColorselCmykClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) colorsel_cmyk_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (ColorselCmyk),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) colorsel_cmyk_init,
      };

      colorsel_cmyk_type =
        g_type_module_register_type (module,
                                     GIMP_TYPE_COLOR_SELECTOR,
                                     "ColorselCmyk",
                                     &select_info, 0);
    }

  return colorsel_cmyk_type;
}

static void
colorsel_cmyk_class_init (ColorselCmykClass *klass)
{
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  selector_class->name      = _("CMYK");
  selector_class->help_id   = "gimp-colorselector-cmyk";
  selector_class->stock_id  = GTK_STOCK_PRINT;  /* FIXME */
  selector_class->set_color = colorsel_cmyk_set_color;
}

static void
colorsel_cmyk_init (ColorselCmyk *module)
{
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkObject *adj;
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

  module->pullout = 1.0;

  table = gtk_table_new (5, 4, FALSE);

  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 3, 4);

  gtk_box_pack_start (GTK_BOX (module), table, TRUE, TRUE, 0);

  for (i = 0; i < 4; i++)
    {
      adj = gimp_scale_entry_new (GTK_TABLE (table), 1, i,
                                  gettext (cmyk_labels[i]),
                                  -1, -1,
                                  0.0,
                                  0.0, 100.0,
                                  1.0, 10.0,
                                  0,
                                  TRUE, 0.0, 0.0,
                                  gettext (cmyk_tips[i]),
                                  NULL);

      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (colorsel_cmyk_adj_update),
                        module);

      module->adj[i] = GTK_ADJUSTMENT (adj);
    }

  label = gtk_label_new_with_mnemonic (_("Black _pullout:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label,
                    1, 3, i, i + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&adj, module->pullout * 100.0,
                                     0.0, 100.0, 1.0, 10.0, 0.0,
                                     1.0, 0);

  gtk_table_attach (GTK_TABLE (table), spinbutton,
                    3, 4, i, i + 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (spinbutton);

  gimp_help_set_help_data (spinbutton,
                           _("The percentage of black to pull out "
                             "of the colored inks."), NULL);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (colorsel_cmyk_pullout_update),
                    module);

  module->adj[i] = GTK_ADJUSTMENT (adj);

  gtk_widget_show (table);
}

static void
colorsel_cmyk_set_color (GimpColorSelector *selector,
                         const GimpRGB     *rgb,
                         const GimpHSV     *hsv)
{
  ColorselCmyk *module = COLORSEL_CMYK (selector);

  gimp_rgb_to_cmyk (rgb, module->pullout, &module->cmyk);

  gtk_adjustment_set_value (module->adj[0], module->cmyk.c * 100.0);
  gtk_adjustment_set_value (module->adj[1], module->cmyk.m * 100.0);
  gtk_adjustment_set_value (module->adj[2], module->cmyk.y * 100.0);
  gtk_adjustment_set_value (module->adj[3], module->cmyk.k * 100.0);
}

static void
colorsel_cmyk_adj_update (GtkAdjustment *adj,
                          ColorselCmyk  *module)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (module);
  gint               i;

  for (i = 0; i < 4; i++)
    if (module->adj[i] == adj)
      break;

  switch (i)
    {
    case 0:
      module->cmyk.c = adj->value / 100.0;
      break;
    case 1:
      module->cmyk.m = adj->value / 100.0;
      break;
    case 2:
      module->cmyk.y = adj->value / 100.0;
      break;
    case 3:
      module->cmyk.k = adj->value / 100.0;
      break;
    default:
      return;
    }

  gimp_cmyk_to_rgb (&module->cmyk, &selector->rgb);
  gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);

  gimp_color_selector_color_changed (selector);
}

static void
colorsel_cmyk_pullout_update (GtkAdjustment *adj,
                              ColorselCmyk  *module)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (module);

  module->pullout = adj->value / 100.0;

  gimp_color_selector_set_color (selector, &selector->rgb, &selector->hsv);
}
