#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"

#include "libgimp/stdplugins-intl.h"


#define COLORBUTTONWIDTH  30
#define COLORBUTTONHEIGHT 20

GtkObject *generaldarkedgeadjust = NULL;
GtkWidget *generalpaintedges = NULL;
GtkWidget *generaltileable = NULL;
GtkWidget *generaldropshadow = NULL;
GtkWidget *generalcolbutton;
GtkObject *generalshadowadjust = NULL;
GtkObject *generalshadowdepth = NULL;
GtkObject *generalshadowblur = NULL;
GtkObject *devthreshadjust = NULL;

#define NUMGENERALBGRADIO 4

static GtkWidget *generalbgradio[NUMGENERALBGRADIO];

void generalbgchange(GtkWidget *wg, void *d, int num)
{
  int n;

  if(wg) {
    n = GPOINTER_TO_INT (d);
    if(!img_has_alpha && (n == 3))
      n = 1;
    pcvals.generalbgtype = n;
  } else {
    n = num;
    if(!img_has_alpha && (n == 3))
      n = 1;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(generalbgradio[n]), TRUE);
  }
}

static void selectcolor(GtkWidget *widget, gpointer data)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(generalbgradio[0]), TRUE);
}

void create_generalpage(GtkNotebook *notebook)
{
  GtkWidget *box1, *box2, *box3, *box4, *thispage;
  GtkWidget *label, *tmpw, *frame, *table;

  label = gtk_label_new_with_mnemonic (_("_General"));

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  frame = gtk_frame_new (_("Background:"));
  gtk_box_pack_start(GTK_BOX(thispage), frame, FALSE, FALSE, 10);
  gtk_widget_show (frame);

  box2 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER(frame), box2);
  gtk_widget_show(box2);

  box3 = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box2), box3, FALSE, FALSE, 10);
  gtk_widget_show(box3);

  generalbgradio[1] = tmpw = gtk_radio_button_new_with_label(NULL, _("Keep original"));
  gtk_widget_show(tmpw);
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  g_signal_connect(tmpw, "clicked",
		   G_CALLBACK (generalbgchange), (gpointer) 1);
  gimp_help_set_help_data
    (tmpw, _("Preserve the original image as a background"), NULL);

  generalbgradio[2] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("From paper"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked",
		   G_CALLBACK (generalbgchange), (gpointer) 2);
  gimp_help_set_help_data
    (tmpw, _("Copy the texture of the selected paper as a background"), NULL);

  box3 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box2), box3,FALSE,FALSE, 10);
  gtk_widget_show(box3);

  box4 = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box3), box4, FALSE, FALSE, 0);
  gtk_widget_show(box4);

  generalbgradio[0] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tmpw)), _("Solid"));
  gtk_box_pack_start(GTK_BOX(box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked",
		   G_CALLBACK (generalbgchange), (gpointer) 0);
  gimp_help_set_help_data
    (tmpw, _("Solid colored background"), NULL);

  generalcolbutton = gimp_color_button_new (_("Color"),
					    COLORBUTTONWIDTH,
					    COLORBUTTONHEIGHT,
					    &pcvals.color,
					    GIMP_COLOR_AREA_FLAT);
  g_signal_connect (generalcolbutton, "clicked",
		    G_CALLBACK (selectcolor), NULL);
  g_signal_connect (generalcolbutton, "color_changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &pcvals.color);
  gtk_box_pack_start(GTK_BOX(box4), generalcolbutton, FALSE, FALSE, 10);
  gtk_widget_show (generalcolbutton);

  generalbgradio[3] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(generalbgradio[0])), _("Transparent"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked",
		   G_CALLBACK (generalbgchange), (gpointer) 3);
  gimp_help_set_help_data
    (tmpw, _("Use a transparent background; Only the strokes painted will be visible"), NULL);
  if(!img_has_alpha)
    gtk_widget_set_sensitive (tmpw, FALSE);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (generalbgradio[pcvals.generalbgtype]), TRUE);

  box1 = gtk_hbox_new (FALSE, 16);
  gtk_box_pack_start(GTK_BOX(thispage), box1, FALSE, FALSE, 0);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  tmpw = gtk_check_button_new_with_label( _("Paint edges"));
  generalpaintedges = tmpw;
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gimp_help_set_help_data
    (tmpw, _("Selects if to place strokes all the way out to the edges of the image"), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw),
			       pcvals.generalpaintedges);

  generaltileable = tmpw = gtk_check_button_new_with_label( _("Tileable"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gimp_help_set_help_data
    (tmpw, _("Selects if the resulting image should be seamlessly tileable"), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw),
			       pcvals.generaltileable);

  tmpw = gtk_check_button_new_with_label( _("Drop Shadow"));
  generaldropshadow = tmpw;
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gimp_help_set_help_data
    (tmpw, _("Adds a shadow effect to each brush stroke"), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw),
			       pcvals.generaldropshadow);

  table = gtk_table_new(3, 5, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE(table), 4);
  gtk_box_pack_start(GTK_BOX(box1), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  generaldarkedgeadjust =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 0,
			  _("Edge darken:"),
			  150, 6, pcvals.generaldarkedge,
			  0.0, 1.0, 0.01, 0.1, 2,
			  TRUE, 0, 0,
			  _("How much to \"darken\" the edges of each brush stroke"),
			  NULL);

  generalshadowadjust =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 1,
			  _("Shadow darken:"),
			  150, 6, pcvals.generalshadowdarkness,
			  0.0, 99.0, 0.1, 1, 2,
			  TRUE, 0, 0,
			  _("How much to \"darken\" the drop shadow"),
			  NULL);

  generalshadowdepth =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 2,
			  _("Shadow depth:"),
			  150, 6, pcvals.generalshadowdepth,
			  0, 99, 1, 5, 0,
			  TRUE, 0, 0,
			  _("The depth of the drop shadow, i.e. how far apart from the object it should be"),
			  NULL);

  generalshadowblur =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 3,
			  _("Shadow blur:"),
			  150, 6, pcvals.generalshadowblur,
			  0, 99, 1, 5, 0,
			  TRUE, 0, 0,
			  _("How much to blur the drop shadow"),
			  NULL);

  devthreshadjust =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 4,
			  _("Deviation threshold:"),
			  150, 6, pcvals.devthresh,
			  0.0, 1.0, 0.01, 0.01, 2,
			  TRUE, 0, 0,
			  _("A bailout-value for adaptive selections"),
			  NULL);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
