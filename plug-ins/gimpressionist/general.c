#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "infile.h"

#include "libgimp/stdplugins-intl.h"


#define COLORBUTTONWIDTH  30
#define COLORBUTTONHEIGHT 20


#define NUMGENERALBGRADIO 4

static GtkWidget *generalbgradio[NUMGENERALBGRADIO];
static GtkWidget *generalpaintedges = NULL;
static GtkObject *generaldarkedgeadjust = NULL;
static GtkWidget *generaltileable;
static GtkWidget *generaldropshadow = NULL;
static GtkWidget *generalcolbutton;
static GtkObject *generalshadowadjust = NULL;
static GtkObject *generalshadowdepth = NULL;
static GtkObject *generalshadowblur = NULL;
static GtkObject *devthreshadjust = NULL;

static int normalize_bg(int n)
{
  return (!img_has_alpha && (n == 3)) ? 1 : n;
}

static void general_bg_store(GtkWidget *wg, void *d)
{
  pcvals.generalbgtype = normalize_bg (GPOINTER_TO_INT (d));
}

void general_store(void)
{
  pcvals.generalpaintedges = GTK_TOGGLE_BUTTON(generalpaintedges)->active;
  pcvals.generaldarkedge = GTK_ADJUSTMENT(generaldarkedgeadjust)->value;
  pcvals.generaltileable = GTK_TOGGLE_BUTTON(generaltileable)->active;
  pcvals.generaldropshadow = GTK_TOGGLE_BUTTON(generaldropshadow)->active;
  pcvals.generalshadowdarkness = GTK_ADJUSTMENT(generalshadowadjust)->value;
  pcvals.generalshadowdepth = GTK_ADJUSTMENT(generalshadowdepth)->value;
  pcvals.generalshadowblur = GTK_ADJUSTMENT(generalshadowblur)->value;
  pcvals.devthresh = GTK_ADJUSTMENT(devthreshadjust)->value;
}

void general_restore(void)
{
  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON (generalbgradio[normalize_bg (pcvals.generalbgtype)]
                      ),
    TRUE
    );

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (generalpaintedges),
                                pcvals.generalpaintedges);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (generaldarkedgeadjust),
                            pcvals.generaldarkedge);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (generalshadowadjust),
                            pcvals.generalshadowdarkness);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (generaldropshadow),
                                pcvals.generaldropshadow);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (generalshadowdepth),
                            pcvals.generalshadowdepth);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (generalshadowblur),
                            pcvals.generalshadowblur);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (generaltileable),
                                pcvals.generaltileable);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (generalcolbutton),
                               &pcvals.color);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (devthreshadjust),
                            pcvals.devthresh);
}

static void selectcolor(GtkWidget *widget, gpointer data)
{
  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON(generalbgradio[BG_TYPE_SOLID]), 
    TRUE
    );
}

static GtkWidget *create_general_button (GtkWidget *box, int idx, 
                                         gchar *label, gchar *help_string,
                                         GSList **radio_group
                                        )
{
  return create_radio_button (box, idx, general_bg_store, label, 
                              help_string, radio_group, generalbgradio);
}

void create_generalpage(GtkNotebook *notebook)
{
  GtkWidget *box1, *box2, *box3, *box4, *thispage;
  GtkWidget *label, *tmpw, *frame, *table;
  GSList * radio_group = NULL;

  label = gtk_label_new_with_mnemonic (_("_General"));

  thispage = gtk_vbox_new(FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_widget_show(thispage);

  frame = gimp_frame_new (_("Background"));
  gtk_box_pack_start(GTK_BOX(thispage), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box3 = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER(frame), box3);
  gtk_widget_show (box3);

  create_general_button(box3, BG_TYPE_KEEP_ORIGINAL, _("Keep original"),
          _("Preserve the original image as a background"),
          &radio_group
          );

  create_general_button(box3, BG_TYPE_FROM_PAPER, _("From paper"),
          _("Copy the texture of the selected paper as a background"),
          &radio_group
          );

  box4 = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start(GTK_BOX(box3), box4, FALSE, FALSE, 0);
  gtk_widget_show(box4);

  create_general_button(box4, BG_TYPE_SOLID, _("Solid"),
          _("Solid colored background"),
          &radio_group
          );
  

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
  gtk_box_pack_start(GTK_BOX(box4), generalcolbutton, FALSE, FALSE, 0);
  gtk_widget_show (generalcolbutton);

  tmpw = 
       create_general_button(box3, BG_TYPE_TRANSPARENT, _("Transparent"),
           _("Use a transparent background; Only the strokes painted will be visible"),
           &radio_group
          );

  if(!img_has_alpha)
    gtk_widget_set_sensitive (tmpw, FALSE);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (generalbgradio[pcvals.generalbgtype]), TRUE);

  box1 = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start(GTK_BOX(thispage), box1, FALSE, FALSE, 0);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (FALSE, 6);
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

  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE(table), 6);
  gtk_table_set_row_spacings (GTK_TABLE(table), 6);
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
