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

GtkObject *sizenumadjust = NULL;
GtkObject *sizefirstadjust = NULL;
GtkObject *sizelastadjust = NULL;

#define NUMSIZERADIO 8

GtkWidget *sizeradio[NUMSIZERADIO];

void sizechange(GtkWidget *wg, void *d, int num)
{
  int n;
  if(wg) {
    n = (long)d;
    pcvals.sizetype = n;
  } else {
    int i;
    n = num;
    for(i = 0; i < NUMSIZERADIO; i++)
      if(i != n)
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(sizeradio[i]), FALSE);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(sizeradio[n]), TRUE);
  }
}

void create_sizepage(GtkNotebook *notebook)
{
  GtkWidget *box1, *box2, *box3, *box4, *thispage;
  GtkWidget *labelbox, *menubox;
  GtkWidget *tmpw;
  char title[100];
  int i;

  sprintf(title, _("Size"));

  labelbox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new(title);
  gtk_box_pack_start(GTK_BOX(labelbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(labelbox);

  menubox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new(title);
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

  tmpw = gtk_label_new( _("Sizes:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  tmpw = gtk_label_new( _("Min size:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  tmpw = gtk_label_new( _("Max size:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);


  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE, 10);
  gtk_widget_show (box2);

  sizenumadjust = gtk_adjustment_new(pcvals.sizenum, 1.0, 31.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(sizenumadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 0);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The number of directions (i.e. brushes) to use"), NULL);
  gtk_widget_show (tmpw);

  sizefirstadjust = gtk_adjustment_new(pcvals.sizefirst, 0.0, 361.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(sizefirstadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The smallest brush to create"), NULL);
  gtk_widget_show (tmpw);

  sizelastadjust = gtk_adjustment_new(pcvals.sizelast, 0.0, 361.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(sizelastadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The largest brush to create"), NULL);
  gtk_widget_show (tmpw);

  box2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new( _("Size:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box3 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box2), box3,FALSE,FALSE, 10);
  gtk_widget_show(box3);

  i = pcvals.sizetype;

  sizeradio[0] = tmpw = gtk_radio_button_new_with_label(NULL, _("Value"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)sizechange, (void *)0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Let the value (brightness) of the region determine the size of the stroke"), NULL);
  if(i == 0)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  sizeradio[1] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Radius"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)sizechange, (void *)1);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The distance from the center of the image determines the size of the stroke"), NULL);
  if(i == 1)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);
    
  sizeradio[2] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Random"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)sizechange, (void *)2);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Selects a random size for each stroke"), NULL);
  if(i == 2)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  sizeradio[3] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Radial"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)sizechange, (void *)3);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Let the direction from the center determine the size of the stroke"), NULL);
  if(i == 3)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  box3 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box2), box3,FALSE,FALSE, 10);
  gtk_widget_show(box3);


  sizeradio[4] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Flowing"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)sizechange, (void *)4);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The strokes follow a \"flowing\" pattern"), NULL);
  if(i == 4)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);


  sizeradio[5] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Hue"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)sizechange, (void *)5);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The hue of the region determines the size of the stroke"), NULL);
  if(i == 5)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  sizeradio[6] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Adaptive"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)sizechange, (void *)6);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The brush-size that matches the original image the closest is selected"), NULL);
  if(i == 6)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  box4 = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box3), box4, FALSE, FALSE, 0);
  gtk_widget_show(box4);

  sizeradio[7] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Manual"));
  gtk_box_pack_start(GTK_BOX(box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)sizechange, (void *)7);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Manually specify the stroke size"), NULL);
  if(i == 7)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  tmpw = gtk_button_new_with_label( _("Edit..."));
  gtk_box_pack_start(GTK_BOX(box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)create_sizemap_dialog, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Opens up the Size Map Editor"), NULL);

  gtk_notebook_append_page_menu (notebook, thispage, labelbox, menubox);
}
