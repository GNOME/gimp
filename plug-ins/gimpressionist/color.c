#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimpui.h>

#include "gimpressionist.h"

#include "libgimp/stdplugins-intl.h"


#define NUMCOLORRADIO 2

GtkWidget *colorradio[NUMCOLORRADIO];
GtkObject *colornoiseadjust = NULL;


void colorchange(GtkWidget *wg, void *d, int num)
{
  if (wg) {
    pcvals.colortype = (int) d;
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(colorradio[num]), TRUE);
  }
}

void create_colorpage(GtkNotebook *notebook)
{
  GtkWidget *box0, *box1, *box2, *thispage;
  GtkWidget *label, *tmpw, *table;

  label = gtk_label_new_with_mnemonic (_("Co_lor"));

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box0 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box0,FALSE,FALSE,0);
  gtk_widget_show (box0);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box0), box1,FALSE,FALSE,0);
  gtk_widget_show (box1);

  tmpw = gtk_label_new( _("Color:"));
  gtk_box_pack_start(GTK_BOX(box1), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box2 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE, 10);
  gtk_widget_show(box2);

  colorradio[0] = tmpw = gtk_radio_button_new_with_label(NULL, _("Average under brush"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(G_OBJECT(tmpw), "clicked",
		   G_CALLBACK(colorchange), (gpointer) 0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Color is computed from the average of all pixels under the brush"), NULL);

  colorradio[1] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("Center of brush"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(G_OBJECT(tmpw), "clicked",
		   G_CALLBACK(colorchange), (gpointer) 1);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Samples the color from the pixel in the center of the brush"), NULL);

  gtk_toggle_button_set_active 
    (GTK_TOGGLE_BUTTON (colorradio[pcvals.colortype]), TRUE);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE(table), 4);
  gtk_box_pack_start(GTK_BOX(box0), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  colornoiseadjust = 
    gimp_scale_entry_new (GTK_TABLE(table), 0, 0, 
			  _("Color noise:"),
			  100, -1, pcvals.colornoise, 
			  0.0, 100.0, 1.0, 5.0, 0, 
			  TRUE, 0, 0,
			  _("Adds random noise to the color"),
			  NULL);
  g_signal_connect (brushdensityadjust, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.colornoise);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
