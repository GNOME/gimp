#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"

#include "libgimp/stdplugins-intl.h"


#define NUMCOLORRADIO 2

static GtkWidget *colorradio[NUMCOLORRADIO];
GtkObject *colornoiseadjust = NULL;


void colorchange(int num)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(colorradio[num]), TRUE);
}

void create_colorpage(GtkNotebook *notebook)
{
  GtkWidget *vbox, *hbox, *thispage;
  GtkWidget *label, *table;
  GtkWidget *frame;

  label = gtk_label_new_with_mnemonic (_("Co_lor"));

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), vbox,FALSE,FALSE,0);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gimp_int_radio_group_new (TRUE, _("Color"),
				    G_CALLBACK (gimp_radio_button_update),
				    &pcvals.colortype, 0,

				    _("A_verage under brush"),
				    0, &colorradio[0],
				    _("C_enter of brush"),
				    1, &colorradio[1],

				    NULL);

  gimp_help_set_help_data 
    (colorradio[0], 
     _("Color is computed from the average of all pixels under the brush"), 
     NULL);
  gimp_help_set_help_data 
    (colorradio[1], 
     _("Samples the color from the pixel in the center of the brush"), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  gtk_toggle_button_set_active 
    (GTK_TOGGLE_BUTTON (colorradio[pcvals.colortype]), TRUE);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE(table), 4);
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  colornoiseadjust = 
    gimp_scale_entry_new (GTK_TABLE(table), 0, 0, 
			  _("Color _noise:"),
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
