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
#include "size.h"

#include "libgimp/stdplugins-intl.h"

static GtkObject *sizenumadjust = NULL;
static GtkObject *sizefirstadjust = NULL;
static GtkObject *sizelastadjust = NULL;
static GtkWidget *sizeradio[NUMSIZERADIO];

static void size_store(GtkWidget *wg, void *d)
{
  pcvals.sizetype = GPOINTER_TO_INT (d);
}

static void size_type_restore(void)
{
  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON(sizeradio[pcvals.sizetype]), 
    TRUE
    );    
}
void size_restore(void)
{
  size_type_restore();
  gtk_adjustment_set_value(GTK_ADJUSTMENT(sizenumadjust), pcvals.sizenum);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(sizefirstadjust), pcvals.sizefirst);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(sizelastadjust), pcvals.sizelast);  
}

static void create_sizemap_dialog_helper(void)
{
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sizeradio[7]), TRUE);
    create_sizemap_dialog();
}

static void create_size_radio_button (GtkWidget *box, int orienttype, 
                                      gchar *label, gchar *help_string,
                                      GSList **radio_group
                                     )
{
  create_radio_button (box, orienttype, size_store, label, 
                       help_string, radio_group, sizeradio);
}


void create_sizepage(GtkNotebook *notebook)
{
  GtkWidget *box2, *box3, *box4, *thispage;
  GtkWidget *label, *tmpw, *table;
  GSList * radio_group = NULL;

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

  create_size_radio_button (box3, SIZE_TYPE_VALUE, _("Value"), 
    _("Let the value (brightness) of the region determine the size of the stroke"),
    &radio_group
    );
  
  create_size_radio_button (box3, SIZE_TYPE_RADIUS, _("Radius"),
     _("The distance from the center of the image determines the size of the stroke"),
    &radio_group
    );
  
  create_size_radio_button (box3, SIZE_TYPE_RANDOM, _("Random"),
     _("Selects a random size for each stroke"),
    &radio_group
    );

  create_size_radio_button (box3, SIZE_TYPE_RADIAL, _("Radial"),
    _("Let the direction from the center determine the size of the stroke"),
    &radio_group
    );
  
  box3 = gtk_vbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(box2), box3,FALSE,FALSE, 0);
  gtk_widget_show(box3);

  create_size_radio_button (box3, SIZE_TYPE_FLOWING, _("Flowing"),
    _("The strokes follow a \"flowing\" pattern"),
    &radio_group
    );
  
  create_size_radio_button (box3, SIZE_TYPE_HUE, _("Hue"),
    _("The hue of the region determines the size of the stroke"),
    &radio_group
    );

  create_size_radio_button (box3, SIZE_TYPE_ADAPTIVE, _("Adaptive"),
    _("The brush-size that matches the original image the closest is selected"),
    &radio_group
    );


  box4 = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(box3), box4, FALSE, FALSE, 0);
  gtk_widget_show(box4);

  create_size_radio_button (box4, SIZE_TYPE_MANUAL, _("Manual"),
    _("Manually specify the stroke size"),
    &radio_group
    );
  
  size_type_restore();

  tmpw = gtk_button_new_from_stock (GIMP_STOCK_EDIT);
  gtk_box_pack_start(GTK_BOX(box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  g_signal_connect(tmpw, "clicked",
		   G_CALLBACK(create_sizemap_dialog_helper), NULL);
  gimp_help_set_help_data (tmpw, _("Opens up the Size Map Editor"), NULL);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
