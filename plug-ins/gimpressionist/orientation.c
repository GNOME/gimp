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

#include "libgimp/stdplugins-intl.h"


GtkObject *orientnumadjust = NULL;
GtkObject *orientfirstadjust = NULL;
GtkObject *orientlastadjust = NULL;

#define NUMORIENTRADIO 8

GtkWidget *orientradio[NUMORIENTRADIO];

void
orientchange (GtkWidget *wg, void *d, int num)
{
  if (wg) {
    pcvals.orienttype = GPOINTER_TO_INT (d);
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(orientradio[num]), TRUE);
  }
}

void
create_orientationpage (GtkNotebook *notebook)
{
  GtkWidget *box1, *box2, *box3, *box4, *thispage;
  GtkWidget *label, *tmpw, *table;

  label = gtk_label_new_with_mnemonic (_("Or_ientation"));

  thispage = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show (thispage);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (thispage), box1,FALSE,FALSE,0);
  gtk_widget_show (box1);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (box1), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  orientnumadjust = 
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			  _("Directions:"),
			  150, -1, pcvals.orientnum, 
			  1.0, 30.0, 1.0, 1.0, 0, 
			  TRUE, 0, 0,
			  _("The number of directions (i.e. brushes) to use"),
			  NULL);
  g_signal_connect (orientnumadjust, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pcvals.orientnum);

  orientfirstadjust = 
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			  _("Start angle:"),
			  150, -1, pcvals.orientfirst, 
			  0.0, 360.0, 1.0, 10.0, 0, 
			  TRUE, 0, 0,
			  _("The starting angle of the first brush to create"),
			  NULL);
  g_signal_connect (orientfirstadjust, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.orientfirst);

  orientlastadjust = 
    gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			  _("Angle span:"),
			  150, -1, pcvals.orientlast, 
			  0.0, 360.0, 1.0, 10.0, 0, 
			  TRUE, 0, 0,
			  _("The angle span of the first brush to create"),
			  NULL);
  g_signal_connect (orientlastadjust, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.orientlast);

  box2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (thispage), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new (_("Orientation:"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);

  box3 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 10);
  gtk_widget_show (box3);

  orientradio[0] = tmpw = gtk_radio_button_new_with_label (NULL, _("Value"));
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
		    G_CALLBACK (orientchange), (gpointer) 0);
  gimp_help_set_help_data 
    (tmpw, _("Let the value (brightness) of the region determine the direction of the stroke"), NULL);

  orientradio[1] = tmpw =
    gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (tmpw)),
				     _("Radius"));
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
		    G_CALLBACK (orientchange), (gpointer) 1);
  gimp_help_set_help_data 
    (tmpw, _("The distance from the center of the image determines the direction of the stroke"), NULL);
    
  orientradio[2] = tmpw = 
    gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (tmpw)),
				     _("Random"));
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
		    G_CALLBACK (orientchange), (gpointer) 2);
  gimp_help_set_help_data 
    (tmpw, _("Selects a random direction of each stroke"), NULL);

  orientradio[3] = tmpw = 
    gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (tmpw)),
				     _("Radial"));
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
		    G_CALLBACK (orientchange), (gpointer) 3);
  gimp_help_set_help_data 
    (tmpw, _("Let the direction from the center determine the direction of the stroke"), NULL);

  box3 = gtk_vbox_new (FALSE,0);
  gtk_box_pack_start (GTK_BOX (box2), box3,FALSE,FALSE, 10);
  gtk_widget_show (box3);

  orientradio[4] = tmpw =
    gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (tmpw)),
				     _("Flowing"));
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
		    G_CALLBACK (orientchange), (gpointer) 4);
  gimp_help_set_help_data 
    (tmpw, _("The strokes follow a \"flowing\" pattern"), NULL);

  orientradio[5] = tmpw =
    gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (tmpw)),
				     _("Hue"));
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
		    G_CALLBACK (orientchange), (gpointer) 5);
  gimp_help_set_help_data 
    (tmpw, _("The hue of the region determines the direction of the stroke"),
     NULL);

  orientradio[6] = tmpw =
    gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (tmpw)),
				     _("Adaptive"));
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
		    G_CALLBACK (orientchange), (gpointer) 6);
  gimp_help_set_help_data 
    (tmpw, _("The direction that matches the original image the closest is selected"), NULL);

  box4 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box3), box4, FALSE, FALSE, 0);
  gtk_widget_show (box4);

  orientradio[7] = tmpw =
    gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (tmpw)),
				     _("Manual"));
  gtk_box_pack_start (GTK_BOX (box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
		    G_CALLBACK (orientchange), (gpointer) 7);
  gimp_help_set_help_data 
    (tmpw, _("Manually specify the stroke orientation"), NULL);

  gtk_toggle_button_set_active 
    (GTK_TOGGLE_BUTTON (orientradio[pcvals.orienttype]), TRUE);

  tmpw = gtk_button_new_from_stock (GIMP_STOCK_EDIT);
  gtk_box_pack_start (GTK_BOX (box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
		    G_CALLBACK (create_orientmap_dialog), NULL);
  gimp_help_set_help_data 
    (tmpw, _("Opens up the Orientation Map Editor"), NULL);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
