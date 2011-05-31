/* small test app for the new unit entry widget developed during Google Summer of Code 2011 */

#include "config.h"

#include <string.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include "devel-docs/tools/units.h"

#include "gimpunitentry.h"

/* global objects */
GtkWidget *window;
GtkWidget *vbox;
GtkWidget *valign;
GtkWidget *inLabel;
GtkWidget *pxLabel;

GtkWidget *entry1;
GtkWidget *entry2;

void on_value_changed (GtkAdjustment *adj, gpointer userData)
{
  gchar text[40];
  gchar *val1 = gimp_unit_adjustment_to_string_in_unit (GIMP_UNIT_ENTRY (entry1)->unitAdjustment, GIMP_UNIT_INCH);
  gchar *val2 = gimp_unit_adjustment_to_string_in_unit (GIMP_UNIT_ENTRY (entry2)->unitAdjustment, GIMP_UNIT_INCH);
  g_sprintf (text, "%s x %s", val1, val2);
  gtk_label_set_text (GTK_LABEL(inLabel), text); 

  val1 = gimp_unit_adjustment_to_string_in_unit (GIMP_UNIT_ENTRY (entry1)->unitAdjustment, GIMP_UNIT_PIXEL);
  val2 = gimp_unit_adjustment_to_string_in_unit (GIMP_UNIT_ENTRY (entry2)->unitAdjustment, GIMP_UNIT_PIXEL);
  g_sprintf (text, "%s x %s", val1, val2);
  gtk_label_set_text (GTK_LABEL(pxLabel), text); 

  g_free (val1);
  g_free (val2);
}

/* set up interface */
void
create_interface(void)
{
  /* main window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW(window), 200, 100);

  /* vbox (used for the entries) */
  vbox = gtk_vbox_new (TRUE, 1);

  /* valign */
  valign = gtk_alignment_new (0, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (valign), vbox);
  gtk_container_add (GTK_CONTAINER (window), valign);

  /* the entries */
  entry1 = gimp_unit_entry_new ();
  entry2 = gimp_unit_entry_new ();

  gtk_box_pack_start (GTK_BOX (vbox), entry1, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), entry2, TRUE, TRUE, 0);

  gimp_unit_entry_connect (GIMP_UNIT_ENTRY (entry1), GIMP_UNIT_ENTRY (entry2));
  gimp_unit_entry_connect (GIMP_UNIT_ENTRY (entry2), GIMP_UNIT_ENTRY (entry1));

  gimp_unit_adjustment_set_value (gimp_unit_entry_get_adjustment (GIMP_UNIT_ENTRY (entry1)), 20);
  gimp_unit_adjustment_set_value (gimp_unit_entry_get_adjustment (GIMP_UNIT_ENTRY (entry2)), 20);

  /* status label */
  inLabel = gtk_label_new ("inches");
  pxLabel = gtk_label_new ("pixels");
  gtk_box_pack_end (GTK_BOX (vbox), pxLabel, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), inLabel, TRUE, TRUE, 0);

  on_value_changed (NULL, NULL);

  /* signals */
  g_signal_connect_swapped (G_OBJECT(window), "destroy",
                            G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (GIMP_UNIT_ENTRY (entry1)->unitAdjustment), "value-changed",
                    G_CALLBACK (on_value_changed), NULL);
  g_signal_connect (G_OBJECT (GIMP_UNIT_ENTRY (entry2)->unitAdjustment), "value-changed",
                    G_CALLBACK (on_value_changed), NULL);

  gtk_widget_show_all (window);
}

int main (int   argc,
          char *argv[])
{
    units_init();

    gtk_init (&argc, &argv);

    create_interface ();

    gtk_main ();
    
    return 0;
}
