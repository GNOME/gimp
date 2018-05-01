/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-utils.c
 * Copyright (C) 2002-2017  Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "propgui-types.h"

#include "gimppropgui-utils.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static gboolean  gimp_prop_kelvin_presets_button_press (GtkWidget      *widget,
                                                        GdkEventButton *bevent,
                                                        GtkMenu        *menu);
static void      gimp_prop_kelvin_presets_activate     (GtkWidget      *widget,
                                                        GObject        *config);
static void      gimp_prop_random_seed_new_clicked     (GtkButton      *button,
                                                        GtkAdjustment  *adj);


/*  public functions  */

GtkWidget *
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
    { 2700, N_("2,700 K - Soft (or warm) LED lamps") },
    { 3000, N_("3,000 K – Soft (or warm) white compact fluorescent lamps") },
    { 3200, N_("3,200 K – Studio lamps, photofloods, etc.") },
    { 3300, N_("3,300 K – Incandescent lamps") },
    { 3350, N_("3,350 K – Studio \"CP\" light") },
    { 4000, N_("4,000 K - Cold (daylight) LED lamps") },
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

  gimp_help_set_help_data (button,
                           _("Choose from a list of common "
                             "color temperatures"), NULL);

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
                              g_strdup (property_name),
                              (GDestroyNotify) g_free);

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

GtkWidget *
gimp_prop_random_seed_new (GObject     *config,
                           const gchar *property_name)
{
  GtkAdjustment *adj;
  GtkWidget     *hbox;
  GtkWidget     *spin;
  GtkWidget     *button;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  spin = gimp_prop_spin_button_new (config, property_name,
                                    1.0, 10.0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spin, TRUE, TRUE, 0);
  gtk_widget_show (spin);

  button = gtk_button_new_with_label (_("New Seed"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin));

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_prop_random_seed_new_clicked),
                    adj);

  return hbox;
}


/*  private functions  */

static gboolean
gimp_prop_kelvin_presets_button_press (GtkWidget      *widget,
                                       GdkEventButton *bevent,
                                       GtkMenu        *menu)
{
  if (bevent->type == GDK_BUTTON_PRESS)
    {
      gtk_menu_popup_at_widget (menu, widget,
                                GDK_GRAVITY_WEST,
                                GDK_GRAVITY_NORTH_EAST,
                                (GdkEvent *) bevent);
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

static void
gimp_prop_random_seed_new_clicked (GtkButton     *button,
                                   GtkAdjustment *adj)
{
  guint32 value;

  value = floor (g_random_double_range (gtk_adjustment_get_lower (adj),
                                        gtk_adjustment_get_upper (adj) + 1.0));

  gtk_adjustment_set_value (adj, value);
}
