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
#include "ppmtool.h"

#include "libgimp/stdplugins-intl.h"


GtkObject *sizenumadjust = NULL;
GtkObject *sizefirstadjust = NULL;
GtkObject *sizelastadjust = NULL;

#define NUMSIZERADIO 8

GtkWidget *sizeradio[NUMSIZERADIO];

void sizechange(GtkWidget *wg, void *d, int num)
{
  if(wg) {
    pcvals.sizetype = GPOINTER_TO_INT (d);
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(sizeradio[num]), TRUE);
  }
}

void create_sizepage(GtkNotebook *notebook)
{
  GtkWidget *box2, *box3, *box4, *thispage;
  GtkWidget *label, *tmpw, *table;

  label = gtk_label_new_with_mnemonic (_("_Size"));

  thispage = gtk_vbox_new(FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_widget_show(thispage);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start(GTK_BOX(thispage), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  sizenumadjust =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 0,
			  _("Sizes:"),
			  150, -1, pcvals.sizenum,
			  1.0, 30.0, 1.0, 1.0, 0,
			  TRUE, 0, 0,
			  _("The number of sizes of brushes to use"),
			  NULL);
  g_signal_connect (sizenumadjust, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pcvals.sizenum);

  sizefirstadjust =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 1,
			  _("Minimum size:"),
			  150, -1, pcvals.sizefirst,
			  0.0, 360.0, 1.0, 10.0, 0,
			  TRUE, 0, 0,
			  _("The smallest brush to create"),
			  NULL);
  g_signal_connect (sizefirstadjust, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.sizefirst);

  sizelastadjust =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 2,
			  _("Maximum size:"),
			  150, -1, pcvals.sizelast,
			  0.0, 360.0, 1.0, 10.0, 0,
			  TRUE, 0, 0,
			  _("The largest brush to create"),
			  NULL);
  g_signal_connect (sizelastadjust, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.sizelast);

  box2 = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start(GTK_BOX(thispage), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  box3 = gtk_vbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(box2), box3, FALSE, FALSE, 0);
  gtk_widget_show(box3);

  tmpw = gtk_label_new( _("Size:"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box3 = gtk_vbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(box2), box3, FALSE, FALSE, 0);
  gtk_widget_show(box3);

  sizeradio[0] = tmpw = gtk_radio_button_new_with_label(NULL, _("Value"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked", G_CALLBACK(sizechange),
		   GINT_TO_POINTER(0));
  gimp_help_set_help_data (tmpw, _("Let the value (brightness) of the region determine the size of the stroke"), NULL);

  sizeradio[1] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("Radius"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked", G_CALLBACK(sizechange),
		   GINT_TO_POINTER(1));
  gimp_help_set_help_data (tmpw, _("The distance from the center of the image determines the size of the stroke"), NULL);

  sizeradio[2] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("Random"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked", G_CALLBACK(sizechange),
		   GINT_TO_POINTER(2));
  gimp_help_set_help_data (tmpw, _("Selects a random size for each stroke"), NULL);

  sizeradio[3] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("Radial"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked", G_CALLBACK(sizechange),
		   GINT_TO_POINTER(3));
  gimp_help_set_help_data (tmpw, _("Let the direction from the center determine the size of the stroke"), NULL);

  box3 = gtk_vbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(box2), box3,FALSE,FALSE, 0);
  gtk_widget_show(box3);

  sizeradio[4] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("Flowing"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked", G_CALLBACK(sizechange),
		   GINT_TO_POINTER(4));
  gimp_help_set_help_data
    (tmpw, _("The strokes follow a \"flowing\" pattern"), NULL);

  sizeradio[5] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("Hue"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked", G_CALLBACK(sizechange),
		   GINT_TO_POINTER(5));
  gimp_help_set_help_data
    (tmpw, _("The hue of the region determines the size of the stroke"),
     NULL);

  sizeradio[6] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("Adaptive"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked", G_CALLBACK(sizechange),
		   GINT_TO_POINTER(6));
  gimp_help_set_help_data (tmpw, _("The brush-size that matches the original image the closest is selected"), NULL);

  box4 = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(box3), box4, FALSE, FALSE, 0);
  gtk_widget_show(box4);

  sizeradio[7] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("Manual"));
  gtk_box_pack_start(GTK_BOX(box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked", G_CALLBACK(sizechange),
		   GINT_TO_POINTER(7));
  gimp_help_set_help_data (tmpw, _("Manually specify the stroke size"), NULL);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (sizeradio[pcvals.sizetype]), TRUE);

  tmpw = gtk_button_new_from_stock (GIMP_STOCK_EDIT);
  gtk_box_pack_start(GTK_BOX(box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked",
		   G_CALLBACK(create_sizemap_dialog), NULL);
  gimp_help_set_help_data (tmpw, _("Opens up the Size Map Editor"), NULL);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
