/*
 * "$Id$"
 *
 *   Main window code for Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu), Steve Miller (smiller@rni.net)
 *      and Michael Natterer (mitch@gimp.org)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "print_gimp.h"

#ifndef GIMP_1_0

#include "libgimp/stdplugins-intl.h"

extern vars_t   vars;
extern gint     plist_count;       /* Number of system printers */
extern gint     plist_current;     /* Current system printer */
extern plist_t  *plist;		/* System printers */

GtkWidget *gimp_color_adjust_dialog;

static GtkObject *brightness_adjustment;
static GtkObject *saturation_adjustment;
static GtkObject *density_adjustment;
static GtkObject *contrast_adjustment;
static GtkObject *red_adjustment;
static GtkObject *green_adjustment;
static GtkObject *blue_adjustment;
static GtkObject *gamma_adjustment;

static GtkWidget *dither_algo_button = NULL;
static GtkWidget *dither_algo_menu   = NULL;

static void gimp_brightness_update (GtkAdjustment *adjustment);
static void gimp_saturation_update (GtkAdjustment *adjustment);
static void gimp_density_update    (GtkAdjustment *adjustment);
static void gimp_contrast_update   (GtkAdjustment *adjustment);
static void gimp_red_update        (GtkAdjustment *adjustment);
static void gimp_green_update      (GtkAdjustment *adjustment);
static void gimp_blue_update       (GtkAdjustment *adjustment);
static void gimp_gamma_update      (GtkAdjustment *adjustment);

static void gimp_dither_algo_callback (GtkWidget *widget,
				       gpointer   data);
void gimp_build_dither_menu    (void);


/*
 * gimp_create_color_adjust_window (void)
 *
 * NOTES:
 *   creates the color adjuster popup, allowing the user to adjust brightness,
 *   contrast, saturation, etc.
 */
void
gimp_create_color_adjust_window (void)
{
  GtkWidget *dialog;
  GtkWidget *table;

  gimp_color_adjust_dialog = dialog =
    gimp_dialog_new (_("Print Color Adjust"), "print",
		     gimp_plugin_help_func, "filters/print.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("Close"), gtk_widget_hide,
		     NULL, 1, NULL, TRUE, TRUE,

		     NULL);

  table = gtk_table_new (9, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 4, 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 7, 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table,
		      FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Brightness slider...
   */

  brightness_adjustment =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                          _("Brightness:"), 200, 0,
                          vars.brightness, 0.0, 400.0, 1.0, 10.0, 0,
                          TRUE, 0, 0,
                          NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (brightness_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_brightness_update),
                      NULL);

  /*
   * Contrast slider...
   */

  contrast_adjustment =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                          _("Contrast:"), 200, 0,
                          vars.contrast, 25.0, 400.0, 1.0, 10.0, 0,
                          TRUE, 0, 0,
                          NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (contrast_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_contrast_update),
                      NULL);

  /*
   * Red slider...
   */

  red_adjustment =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                          _("Red:"), 200, 0,
                          vars.red, 0.0, 200.0, 1.0, 10.0, 0,
                          TRUE, 0, 0,
                          NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (red_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_red_update),
                      NULL);

  /*
   * Green slider...
   */

  green_adjustment =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                          _("Green:"), 200, 0,
                          vars.green, 0.0, 200.0, 1.0, 10.0, 0,
                          TRUE, 0, 0,
                          NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (green_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_green_update),
                      NULL);

  /*
   * Blue slider...
   */

  blue_adjustment =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
                          _("Blue:"), 200, 0,
                          vars.blue, 0.0, 200.0, 1.0, 10.0, 0,
                          TRUE, 0, 0,
                          NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (blue_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_blue_update),
                      NULL);

  /*
   * Saturation slider...
   */

  saturation_adjustment =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 5,
                          _("Saturation:"), 200, 0,
                          vars.saturation, 0, 10.0, 0.001, 0.01, 3,
                          TRUE, 0, 0,
                          NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (saturation_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_saturation_update),
                      NULL);

  /*
   * Density slider...
   */

  density_adjustment =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 6,
                          _("Density:"), 200, 0,
                          vars.density, 0.1, 3.0, 0.001, 0.01, 3,
                          TRUE, 0, 0,
                          NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (density_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_density_update),
                      NULL);

  /*
   * Gamma slider...
   */

  gamma_adjustment =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 7,
                          _("Gamma:"), 200, 0,
                          vars.gamma, 0.1, 4.0, 0.001, 0.01, 3,
                          TRUE, 0, 0,
                          NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (gamma_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_gamma_update),
                      NULL);

  /*
   * Dither algorithm option menu...
   */

  dither_algo_button = gtk_option_menu_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 8,
			     _("Dither Algorithm:"), 1.0, 0.5,
			     dither_algo_button, 1, TRUE);
  gimp_build_dither_menu ();
}

static void
gimp_brightness_update (GtkAdjustment *adjustment)
{
  if (vars.brightness != adjustment->value)
    {
      vars.brightness = adjustment->value;
      plist[plist_current].v.brightness = adjustment->value;
    }
}

static void
gimp_contrast_update (GtkAdjustment *adjustment)
{
  if (vars.contrast != adjustment->value)
    {
      vars.contrast = adjustment->value;
      plist[plist_current].v.contrast = adjustment->value;
    }
}

static void
gimp_red_update (GtkAdjustment *adjustment)
{
  if (vars.red != adjustment->value)
    {
      vars.red = adjustment->value;
      plist[plist_current].v.red = adjustment->value;
    }
}

static void
gimp_green_update (GtkAdjustment *adjustment)
{
  if (vars.green != adjustment->value)
    {
      vars.green = adjustment->value;
      plist[plist_current].v.green = adjustment->value;
    }
}

static void
gimp_blue_update (GtkAdjustment *adjustment)
{
  if (vars.blue != adjustment->value)
    {
      vars.blue = adjustment->value;
      plist[plist_current].v.blue = adjustment->value;
    }
}

static void
gimp_saturation_update (GtkAdjustment *adjustment)
{
  if (vars.saturation != adjustment->value)
    {
      vars.saturation = adjustment->value;
      plist[plist_current].v.saturation = adjustment->value;
    }
}

static void
gimp_density_update (GtkAdjustment *adjustment)
{
  if (vars.density != adjustment->value)
    {
      vars.density = adjustment->value;
      plist[plist_current].v.density = adjustment->value;
    }
}

static void
gimp_gamma_update (GtkAdjustment *adjustment)
{
  if (vars.gamma != adjustment->value)
    {
      vars.gamma = adjustment->value;
      plist[plist_current].v.gamma = adjustment->value;
    }
}

void
gimp_do_color_updates(void)
{
  gtk_adjustment_set_value (GTK_ADJUSTMENT (brightness_adjustment),
			    plist[plist_current].v.brightness);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (gamma_adjustment),
			    plist[plist_current].v.gamma);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (contrast_adjustment),
			    plist[plist_current].v.contrast);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (red_adjustment),
			    plist[plist_current].v.red);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (green_adjustment),
			    plist[plist_current].v.green);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (blue_adjustment),
			    plist[plist_current].v.blue);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (saturation_adjustment),
			    plist[plist_current].v.saturation);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (density_adjustment),
			    plist[plist_current].v.density);
}

void
gimp_build_dither_menu (void)
{
  GtkWidget *item;
  GtkWidget *item0 = NULL;
  gint i;

  if (dither_algo_menu != NULL)
    {
      gtk_widget_destroy (dither_algo_menu);
      dither_algo_menu = NULL;
    }

  dither_algo_menu = gtk_menu_new ();

  if (num_dither_algos == 0)
    {
      item = gtk_menu_item_new_with_label (_("Standard"));
      gtk_menu_append (GTK_MENU (dither_algo_menu), item);
      gtk_widget_show (item);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (dither_algo_button),
				dither_algo_menu);
      gtk_widget_set_sensitive (dither_algo_button, FALSE);
      return;
    }
  else
    {
      gtk_widget_set_sensitive (dither_algo_button, TRUE);
    }

  for (i = 0; i < num_dither_algos; i++)
    {
      item = gtk_menu_item_new_with_label (gettext (dither_algo_names[i]));
      if (i == 0)
	item0 = item;
      gtk_menu_append (GTK_MENU (dither_algo_menu), item);
      gtk_signal_connect (GTK_OBJECT (item), "activate",
			  GTK_SIGNAL_FUNC (gimp_dither_algo_callback),
			  (gpointer) i);
      gtk_widget_show (item);
    }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (dither_algo_button),
			    dither_algo_menu);

  for (i = 0; i < num_dither_algos; i++)
    {
#ifdef DEBUG
      g_print ("item[%d] = \'%s\'\n", i, dither_algo_names[i]);
#endif /* DEBUG */

      if (strcmp (dither_algo_names[i], plist[plist_current].v.dither_algorithm) == 0)
	{
	  gtk_option_menu_set_history (GTK_OPTION_MENU (dither_algo_button), i);
	  break;
	}
    }

  if (i == num_dither_algos)
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (dither_algo_button), 0);
      gtk_signal_emit_by_name (GTK_OBJECT (item0), "activate");
    }
}

static void
gimp_dither_algo_callback (GtkWidget *widget,
			   gpointer   data)
{
  strcpy(vars.dither_algorithm, dither_algo_names[(gint) data]);
  strcpy(plist[plist_current].v.dither_algorithm,
	 dither_algo_names[(gint) data]);
}

#endif  /* ! GIMP_1_0 */
