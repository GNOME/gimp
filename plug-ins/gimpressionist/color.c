#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_DIRENT_H
#define HAVE_UNISTD_H
#endif

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <libgimp/stdplugins-intl.h>
#include "gimpressionist.h"
#include "ppmtool.h"


#define NUMCOLORRADIO 2

GtkWidget *colorradio[NUMCOLORRADIO];
GtkObject *colornoiseadjust = NULL;


void colorchange(GtkWidget *wg, void *d, int num)
{
  int n;
  if(wg) {
    n = (long)d;
    pcvals.colortype = n;
  } else {
    int i;
    n = num;
    for(i = 0; i < NUMCOLORRADIO; i++)
      if(i != n)
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(colorradio[i]), FALSE);
      else
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(colorradio[n]), TRUE);
  }
}

void create_colorpage(GtkNotebook *notebook)
{
  GtkWidget *box0, *box1, *box2, *box3, *thispage;
  GtkWidget *labelbox, *menubox;
  GtkWidget *tmpw;
  int i;

  labelbox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("Color"));
  gtk_box_pack_start(GTK_BOX(labelbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(labelbox);

  menubox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("Color"));
  gtk_box_pack_start(GTK_BOX(menubox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(menubox);

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box0 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box0,FALSE,FALSE,0);
  gtk_widget_show (box0);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box0), box1,FALSE,FALSE,0);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new( _("Color:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box3 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box1), box3,FALSE,FALSE, 10);
  gtk_widget_show(box3);

  i = pcvals.colortype;

  colorradio[0] = tmpw = gtk_radio_button_new_with_label(NULL, _("Average under brush"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)colorchange, (void *)0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Color is computed from the average of all pixels under the brush"), NULL);
  if(i == 0)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  colorradio[1] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Center of brush"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)colorchange, (void *)1);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Samples the color from the pixel in the center of the brush"), NULL);
  if(i == 1)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);


  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box0), box1,FALSE,FALSE,0);
  gtk_widget_show (box1);

  tmpw = gtk_label_new( _("Color noise:"));
  gtk_box_pack_start(GTK_BOX(box1), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  colornoiseadjust = gtk_adjustment_new(pcvals.colornoise, 0.0, 101.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(colornoiseadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 1);
  gtk_box_pack_start (GTK_BOX (box1), tmpw, FALSE, FALSE, 10);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Adds random noise to the color"), NULL);
    
  gtk_notebook_append_page_menu (notebook, thispage, labelbox, menubox);
}
