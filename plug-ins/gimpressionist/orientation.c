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
#include "gimpressionist.h"
#include "ppmtool.h"
#include <libgimp/stdplugins-intl.h>


GtkObject *orientnumadjust = NULL;
GtkObject *orientfirstadjust = NULL;
GtkObject *orientlastadjust = NULL;

#define NUMORIENTRADIO 8

GtkWidget *orientradio[NUMORIENTRADIO];

void orientchange(GtkWidget *wg, void *d, int num)
{
  int n;
  if(wg) {
    n = (long)d;
    pcvals.orienttype = n;
  } else {
    int i;
    n = num;
    for(i = 0; i < NUMORIENTRADIO; i++)
      if(i != n)
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(orientradio[i]), FALSE);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(orientradio[n]), TRUE);
  }
}

void create_orientationpage(GtkNotebook *notebook)
{
  GtkWidget *box1, *box2, *box3, *box4, *thispage;
  GtkWidget *labelbox, *menubox;
  GtkWidget *tmpw;
  int i;

  labelbox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("Orientation"));
  gtk_box_pack_start(GTK_BOX(labelbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(labelbox);

  menubox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("Orientation"));
  gtk_box_pack_start(GTK_BOX(menubox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(menubox);

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1,FALSE,FALSE,0);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new( _("Directions:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  tmpw = gtk_label_new( _("Start angle:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  tmpw = gtk_label_new( _("Angle span:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);


  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE, 10);
  gtk_widget_show (box2);

  orientnumadjust = gtk_adjustment_new(pcvals.orientnum, 1.0, 31.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(orientnumadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 0);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The number of directions (i.e. brushes) to use"), NULL);
  gtk_widget_show (tmpw);

  orientfirstadjust = gtk_adjustment_new(pcvals.orientfirst, 0.0, 361.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(orientfirstadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The angle of the first brush to create"), NULL);
  gtk_widget_show (tmpw);

  orientlastadjust = gtk_adjustment_new(pcvals.orientlast, 0.0, 361.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(orientlastadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("How large an angle-span to use (360 = full circle)"), NULL);
  gtk_widget_show (tmpw);

  box2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new( _("Orientation:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box3 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box2), box3,FALSE,FALSE, 10);
  gtk_widget_show(box3);

  i = pcvals.orienttype;

  orientradio[0] = tmpw = gtk_radio_button_new_with_label(NULL, _("Value"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)orientchange, (void *)0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Let the value (brightness) of the region determine the direction of the stroke"), NULL);
  if(i == 0)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  orientradio[1] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Radius"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)orientchange, (void *)1);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The distance from the center of the image determines the direction of the stroke"), NULL);
  if(i == 1)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);
    
  orientradio[2] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Random"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)orientchange, (void *)2);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Selects a random direction of each stroke"), NULL);
  if(i == 2)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  orientradio[3] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Radial"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)orientchange, (void *)3);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Let the direction from the center determine the direction of the stroke"), NULL);
  if(i == 3)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  box3 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box2), box3,FALSE,FALSE, 10);
  gtk_widget_show(box3);


  orientradio[4] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Flowing"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)orientchange, (void *)4);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The strokes follow a \"flowing\" pattern"), NULL);
  if(i == 4)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);



  orientradio[5] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Hue"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)orientchange, (void *)5);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The hue of the region determines the direction of the stroke"), NULL);
  if(i == 5)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  orientradio[6] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Adaptive"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)orientchange, (void *)6);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The direction that matches the original image the closest is selected"), NULL);
  if(i == 6)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);


  box4 = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box3), box4, FALSE, FALSE, 0);
  gtk_widget_show(box4);

  orientradio[7] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Manual"));
  gtk_box_pack_start(GTK_BOX(box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)orientchange, (void *)7);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Manually specify the stroke orientation"), NULL);
  if(i == 7)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  tmpw = gtk_button_new_with_label( _("Edit..."));
  gtk_box_pack_start(GTK_BOX(box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)create_orientmap_dialog, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Opens up the Orientation Map Editor"), NULL);

  gtk_notebook_append_page_menu (notebook, thispage, labelbox, menubox);
}
