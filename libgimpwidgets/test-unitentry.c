#include "config.h"

#include <string.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "libgimpbase/gimpbase.h"

#include "devel-docs/tools/units.h"

#include "gimpunitentry.h"

#define ADD_TEST(function) \
  g_test_add ("/gimpunitentry/" #function, \
              GimpTestFixture, \
              NULL, \
              gimp_test_unitentry_setup, \
              function, \
              gimp_test_unitentry_teardown);

typedef struct
{
  GimpUnitEntry      *entry1, *entry2;
  GimpUnitAdjustment *adj1, *adj2;
} GimpTestFixture;

static void
gimp_test_unitentry_setup (GimpTestFixture *fixture,
                           gconstpointer    data)
{

  fixture->entry1 = GIMP_UNIT_ENTRY (gimp_unit_entry_new ());
  fixture->entry2 = GIMP_UNIT_ENTRY (gimp_unit_entry_new ());

  fixture->adj1 = gimp_unit_adjustment_new ();
  gimp_unit_entry_set_adjustment (fixture->entry1, fixture->adj1);
  fixture->adj2 = gimp_unit_adjustment_new ();
  gimp_unit_entry_set_adjustment (fixture->entry2, fixture->adj2);

  gimp_unit_entry_connect (fixture->entry1, fixture->entry2);
  gimp_unit_entry_connect (fixture->entry2, fixture->entry1);
}

static void
gimp_test_unitentry_teardown (GimpTestFixture *fixture,
                              gconstpointer    data)
{
  g_object_ref_sink (fixture->entry1);
  g_object_unref    (fixture->entry1);
  fixture->entry1 = NULL;
  g_object_ref_sink (fixture->entry2);
  g_object_unref    (fixture->entry2);
  fixture->entry1 = NULL;
}

/**
 * parse:
 *
 * Test that simple parsing and unit determination works
 **/
static void
parse (GimpTestFixture *f,
       gconstpointer    data)
{
  gtk_entry_set_text (GTK_ENTRY (f->entry1), "10 px");

  g_assert (gimp_unit_entry_get_pixels      (f->entry1) == 10.0);
  g_assert (gimp_unit_adjustment_get_unit   (f->adj1)   == GIMP_UNIT_PIXEL);
}

/**
 * parse_without_unit:
 *
 * Test that default unit is correcty applied when the user just
 * enters a value without unit
 **/
static void
parse_without_unit (GimpTestFixture *f,
                    gconstpointer    data)
{
  gimp_unit_adjustment_set_unit (f->adj1, GIMP_UNIT_PIXEL);

  gtk_entry_set_text (GTK_ENTRY (f->entry1), "10");

  g_assert (gimp_unit_entry_get_pixels      (f->entry1) == 10.0);
  g_assert (gimp_unit_adjustment_get_unit   (f->adj1)   == GIMP_UNIT_PIXEL);
}

/**
 * parse_addition_without_unit:
 *
 * Test that input in the form "10 in + 20" works
 **/
static void
parse_addition_without_unit (GimpTestFixture *f,
                             gconstpointer    data)
{
  gimp_unit_adjustment_set_unit (f->adj1, GIMP_UNIT_INCH);

  gtk_entry_set_text (GTK_ENTRY (f->entry1), "10 in + 20");

  g_assert (gimp_unit_adjustment_get_value (f->adj1) == 30.0);
  g_assert (gimp_unit_adjustment_get_unit  (f->adj1) == GIMP_UNIT_INCH);
}

/**
 * parse_different_units:
 * 
 * Test that entering a term with different units gets calculated
 * correctly
 */
static void
parse_different_units (GimpTestFixture *f,
                       gconstpointer    data)
{
  gimp_unit_adjustment_set_resolution (f->adj1, 72);

  gtk_entry_set_text (GTK_ENTRY (f->entry1), "10 in + 72 px");

  g_assert (gimp_unit_adjustment_get_value (f->adj1) == 11.0);
  g_assert (gimp_unit_adjustment_get_unit  (f->adj1) == GIMP_UNIT_INCH);
}

/**
 * change_units:
 * 
 * Test that changing the unit of one entry correctly changes
 * the connected entry
 */
static void
change_units (GimpTestFixture *f,
              gconstpointer    data)
{
  gimp_unit_adjustment_set_resolution (f->adj1, 72);
  gimp_unit_adjustment_set_resolution (f->adj2, 72);
  gimp_unit_adjustment_set_unit       (f->adj1, GIMP_UNIT_PIXEL);
  gimp_unit_adjustment_set_unit       (f->adj2, GIMP_UNIT_PIXEL);
  gimp_unit_adjustment_set_value      (f->adj2, 72);

  gtk_entry_set_text (GTK_ENTRY (f->entry1), "10 in");

  g_assert (gimp_unit_adjustment_get_value (f->adj2) == 1.0);
  g_assert (gimp_unit_adjustment_get_unit  (f->adj2) == GIMP_UNIT_INCH);
}

/**
 * parse_multiplication
 *
 * Test that multiplication with constant factor works
 **/
static void
parse_multiplication (GimpTestFixture *f,
                      gconstpointer    data)
{
  gimp_unit_adjustment_set_unit (f->adj1, GIMP_UNIT_INCH);

  gtk_entry_set_text (GTK_ENTRY (f->entry1), "10 in * 10");

  g_assert (gimp_unit_adjustment_get_value (f->adj1) == 100.0);
  g_assert (gimp_unit_adjustment_get_unit  (f->adj1) == GIMP_UNIT_INCH);
}

/**
 * parse_multidim_multiplication
 *
 * Test that multidimensional multiplication is not accepted as correct
 * input (e.g. "10 in * 10 px")
 **/
static void
parse_multidim_multiplication (GimpTestFixture *f,
                               gconstpointer    data)
{
  gimp_unit_adjustment_set_unit  (f->adj1, GIMP_UNIT_INCH);
  gimp_unit_adjustment_set_value (f->adj1, 10.0);

  gtk_entry_set_text (GTK_ENTRY (f->entry1), "10 in * 10 px");

  g_assert (gimp_unit_adjustment_get_value (f->adj1) == 10.0);
  g_assert (gimp_unit_adjustment_get_unit  (f->adj1) == GIMP_UNIT_INCH);
}

int main (int   argc,
          char *argv[])
{
    g_type_init ();
    units_init  ();
    g_test_init (&argc, &argv, NULL);
    gtk_init (&argc, &argv);

    ADD_TEST (parse);
    ADD_TEST (parse_without_unit);
    ADD_TEST (parse_addition_without_unit)
    ADD_TEST (parse_different_units);
    ADD_TEST (change_units);
    ADD_TEST (parse_multiplication);
    ADD_TEST (parse_multidim_multiplication)
    
    return g_test_run ();
}
