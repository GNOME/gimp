#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "placement.h"

#include "libgimp/stdplugins-intl.h"


#define NUMPLACERADIO 2

static GtkWidget *placeradio[NUMPLACERADIO];
static GtkWidget *placecenter = NULL;
static GtkObject *brushdensityadjust = NULL;

void place_restore()
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(placeradio[pcvals.placetype]), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(placecenter), pcvals.placecenter);
  gtk_adjustment_set_value (GTK_ADJUSTMENT(brushdensityadjust), pcvals.brushdensity);
}

void place_store()
{
  pcvals.placecenter = GTK_TOGGLE_BUTTON(placecenter)->active;
}

void create_placementpage(GtkNotebook *notebook)
{
  GtkWidget *vbox;
  GtkWidget *label, *tmpw, *table, *frame;

  label = gtk_label_new_with_mnemonic (_("Pl_acement"));

  vbox = gtk_vbox_new(FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_widget_show(vbox);

  frame = gimp_int_radio_group_new (TRUE, _("Placement"),
				    G_CALLBACK (gimp_radio_button_update),
				    &pcvals.placetype, 0,

				    _("Randomly"), PLACEMENT_TYPE_RANDOM, 
                        &placeradio[PLACEMENT_TYPE_RANDOM],
				    _("Evenly distributed"), PLACEMENT_TYPE_EVEN_DIST, 
                        &placeradio[PLACEMENT_TYPE_EVEN_DIST],

				    NULL);

  gimp_help_set_help_data
    (placeradio[PLACEMENT_TYPE_RANDOM], 
     _("Place strokes randomly around the image"), NULL);
  gimp_help_set_help_data
    (placeradio[PLACEMENT_TYPE_EVEN_DIST], 
     _("The strokes are evenly distributed across the image"),
     NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (placeradio[pcvals.placetype]), TRUE);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE(table), 6);
  gtk_box_pack_start(GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  brushdensityadjust =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 0,
			  _("Stroke _density:"),
			  100, -1, pcvals.brushdensity,
			  1.0, 50.0, 1.0, 5.0, 0,
			  TRUE, 0, 0,
			  _("The relative density of the brush strokes"),
			  NULL);
  g_signal_connect (brushdensityadjust, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.brushdensity);

  placecenter = tmpw = gtk_check_button_new_with_mnemonic( _("Centerize"));
  gtk_box_pack_start(GTK_BOX (vbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gimp_help_set_help_data
    (tmpw, _("Focus the brush strokes around the center of the image"), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), pcvals.placecenter);

  gtk_notebook_append_page_menu (notebook, vbox, label, NULL);
}
