/* small gui test app for the new unit entry widget developed during Google Summer of Code 2011 */

#include "config.h"

#include <string.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "libgimpbase/gimpbase.h"

#include "devel-docs/tools/units.h"

#include "gimpunitentrytable.h"

/* global objects */
GtkWidget *window;
GtkWidget *vbox;
GtkWidget *valign;

GimpUnitEntryTable *entryTable;

/* set up interface */
void
create_interface(void)
{
  GimpUnitEntry *a, *b;

  /* main window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW(window), 200, 100);

  /* vbox (used for the entries) */
  vbox = gtk_vbox_new (FALSE, 1);

  /* valign */
  valign = gtk_alignment_new (0, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (valign), vbox);
  gtk_container_add (GTK_CONTAINER (window), valign);

  /* entry table */
  entryTable = GIMP_UNIT_ENTRY_TABLE (gimp_unit_entry_table_new ());
  gimp_unit_entry_table_add_entry (entryTable, "width", "Width");
  gimp_unit_entry_table_add_entry (entryTable, "height", "Height");
  gimp_unit_entry_table_add_label (entryTable, GIMP_UNIT_PIXEL, "width", "height");

  gimp_unit_entry_table_add_chainbutton (entryTable, "width", "height");

  /* set some default values */
  a = gimp_unit_entry_table_get_entry (entryTable, "width");
  b = gimp_unit_entry_table_get_entry (entryTable, "height");
  gimp_unit_adjustment_set_value (gimp_unit_entry_get_adjustment (a), 20);
  gimp_unit_adjustment_set_value (gimp_unit_entry_get_adjustment (b), 20);

  gtk_box_pack_start (GTK_BOX (vbox), entryTable->table, FALSE, TRUE, 0);

  /* resolution entry */
  entryTable = GIMP_UNIT_ENTRY_TABLE (gimp_unit_entry_table_new ());
  gimp_unit_entry_table_add_entry (entryTable, "xres", "X Resolution");
  gimp_unit_entry_table_add_entry (entryTable, "yres", "Y Resolution");

  /* set some default values */
  a = gimp_unit_entry_table_get_entry (entryTable, "xres");
  b = gimp_unit_entry_table_get_entry (entryTable, "yres");
  gimp_unit_entry_set_value (a, 72);
  gimp_unit_entry_set_value (b, 72);
  gimp_unit_entry_set_res_mode (a, TRUE);
  gimp_unit_entry_set_res_mode (b, TRUE);

  gtk_box_pack_end (GTK_BOX (vbox), entryTable->table, FALSE, TRUE, 5);

  /* signals */
  g_signal_connect_swapped (G_OBJECT(window), "destroy",
                            G_CALLBACK(gtk_main_quit), NULL);
 
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
