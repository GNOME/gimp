#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimpui.h>

#include "gimpressionist.h"

#include "libgimp/stdplugins-intl.h"


#define NUMPLACERADIO 2

static GtkWidget *placeradio[NUMPLACERADIO];
GtkWidget *placecenter = NULL;
GtkObject *brushdensityadjust = NULL;

void placechange(GtkWidget *wg, void *d, int num)
{
  if (wg) {
    pcvals.placetype = (gint) d;
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(placeradio[num]), TRUE);
  }
}

void create_placementpage(GtkNotebook *notebook)
{
  GtkWidget *box0, *box1, *box2, *thispage;
  GtkWidget *label, *tmpw, *table;

  label = gtk_label_new_with_mnemonic (_("Pl_acement"));

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box0 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box0,FALSE,FALSE,0);
  gtk_widget_show (box0);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box0), box1,FALSE,FALSE,0);
  gtk_widget_show (box1);

  tmpw = gtk_label_new( _("Placement:"));
  gtk_box_pack_start(GTK_BOX(box1), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box2 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE, 10);
  gtk_widget_show(box2);

  placeradio[0] = tmpw = gtk_radio_button_new_with_label(NULL, _("Randomly"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(G_OBJECT(tmpw), "clicked",
		   G_CALLBACK(placechange), (gpointer) 0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Place strokes randomly around the image"), NULL);

  placeradio[1] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("Evenly distributed"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(G_OBJECT(tmpw), "clicked",
		   G_CALLBACK(placechange), (gpointer) 1);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The strokes are evenly distributed across the image"), NULL);

  gtk_toggle_button_set_active 
    (GTK_TOGGLE_BUTTON (placeradio[pcvals.placetype]), TRUE);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE(table), 4);
  gtk_box_pack_start(GTK_BOX(box0), table, FALSE, FALSE, 0);
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
  gtk_box_pack_start(GTK_BOX(box0), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Focus the brush strokes around the center of the image"), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), pcvals.placecenter);
    
  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
