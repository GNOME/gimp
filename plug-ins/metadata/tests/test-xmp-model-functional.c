/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2011 Róman Joost <romanofski@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "xmp-parse.h"
#include "xmp-encode.h"
#include "xmp-model.h"


#define ADD_TEST(function) \
  g_test_add ("/metadata-xmp-model/" #function, \
              GimpTestFixture, \
              NULL, \
              gimp_test_xmp_model_setup, \
              function, \
              gimp_test_xmp_model_teardown);


typedef struct
{
  XMPModel *xmp_model;
} GimpTestFixture;


typedef struct
{
  const gchar *schema_name;
  const gchar *name;
  int          pos;
} TestDataEntry;

static TestDataEntry propertiestotest[] =
{
   { "dc",  "title",       1 },
   { "dc",  "creator",     0 },
   { "dc",  "description", 1 },
   { NULL,  NULL,          0 }
};
TestDataEntry * const import_exportdata = propertiestotest;


static void gimp_test_xmp_model_setup       (GimpTestFixture *fixture,
                                             gconstpointer    data);
static void gimp_test_xmp_model_teardown    (GimpTestFixture *fixture,
                                             gconstpointer    data);


/**
 * gimp_test_xmp_model_setup:
 * @fixture: GimpTestFixture fixture
 * @data:
 *
 * Test fixture to setup an XMPModel.
 **/
static void
gimp_test_xmp_model_setup (GimpTestFixture *fixture,
                           gconstpointer    data)
{
  gchar  *uri = NULL;
  GError *error = NULL;

  fixture->xmp_model = xmp_model_new ();

  uri = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                          "plug-ins/metadata/tests/files/test.xmp",
                          NULL);

  xmp_model_parse_file (fixture->xmp_model, uri, &error);

  g_free (uri);
}


static void
gimp_test_xmp_model_teardown (GimpTestFixture *fixture,
                              gconstpointer    data)
{
  g_object_unref (fixture->xmp_model);
}

/**
 * test_xmp_model_import_export_structures:
 * @fixture:
 * @data:
 *
 * Test to assure the round trip of data import, editing, export is
 * working.
 **/
static void
test_xmp_model_import_export_structures (GimpTestFixture *fixture,
                                         gconstpointer    data)
{
  int             i, j;
  gboolean        result;
  const gchar   **before_value;
  const gchar   **after_value;
  GString        *buffer;
  TestDataEntry  *testdata;
  const gchar    *scalarvalue = "test";
  GError        **error       = NULL;

  for (i = 0; import_exportdata[i].name != NULL; ++i)
   {
    testdata = &(import_exportdata[i]);

    /* backup the original raw value */
    before_value = xmp_model_get_raw_property_value (fixture->xmp_model,
                                                     testdata->schema_name,
                                                     testdata->name);
    g_assert (before_value != NULL);

    /* set a new scalar value */
    result = xmp_model_set_scalar_property (fixture->xmp_model,
                                            testdata->schema_name,
                                            testdata->name,
                                            scalarvalue);
    g_assert (result == TRUE);

    /* export */
    buffer = g_string_new ("GIMP_TEST");
    xmp_generate_packet (fixture->xmp_model, buffer);

    /* import */
    xmp_model_parse_buffer (fixture->xmp_model,
                            buffer->str,
                            buffer->len,
                            TRUE,
                            error);
    after_value = xmp_model_get_raw_property_value (fixture->xmp_model,
                                                    testdata->schema_name,
                                                    testdata->name);
    /* check that the scalar value is correctly exported */
    g_assert (after_value != NULL);
    g_assert_cmpstr (after_value[testdata->pos], ==, scalarvalue);

    /* check that the original data is not changed */
    for (j = 0; after_value[j] != NULL; ++j)
     {
       if (j == testdata->pos)
         continue;

       g_assert (before_value[j] != NULL);
       g_assert (after_value[j]  != NULL);
       g_assert_cmpstr (before_value[j], ==, after_value[j]);
     }
   }
}

/**
 * test_xmp_model_import_export:
 * @fixture:
 * @data:
 *
 * Functional test, which assures that changes in the string
 * representation is correctly merged on export.
 **/
static void
test_xmp_model_import_export (GimpTestFixture *fixture,
                              gconstpointer    data)
{
  gboolean     result;
  const gchar **oldvalue;
  const gchar **value;
  const gchar  *scalar;
  GString      *buffer;
  GError      **error = NULL;

  /* The XMPModel is already loaded with imported XMP metadata */
  g_assert (! xmp_model_is_empty (fixture->xmp_model));
  oldvalue = xmp_model_get_raw_property_value (fixture->xmp_model,
                                               "dc",
                                               "description");
  g_assert_cmpstr (oldvalue[0], ==, "x-default");
  g_assert_cmpstr (oldvalue[2], ==, "de_DE");

  /* We can now change the string representation of the raw values,
   * which will not change the raw value itself. The current GtkEntry
   * widgets of XMP_TYPE_LANG_ALT only change the x-default value, which
   * may change in the future */
  result = xmp_model_set_scalar_property (fixture->xmp_model,
                                          "dc",
                                          "description",
                                          "My good description");
  g_assert (result == TRUE);

  value = xmp_model_get_raw_property_value (fixture->xmp_model,
                                            "dc",
                                            "description");
  g_assert (value != NULL);
  g_assert (oldvalue == value);

  scalar = xmp_model_get_scalar_property (fixture->xmp_model,
                                          "dc",
                                          "description");
  g_assert_cmpstr ("My good description", ==, scalar);

  /* Once we write the XMPModel to a file, we take care of the changes
   * in the string representation and merge it with the raw value
   */
  buffer = g_string_new ("GIMP_TEST");
  xmp_generate_packet (fixture->xmp_model, buffer);

  /* We reread the buffer into the XMPModel and expect it to have
   * updated raw values */
  xmp_model_parse_buffer (fixture->xmp_model,
                          buffer->str,
                          buffer->len,
                          TRUE,
                          error);

  value = xmp_model_get_raw_property_value (fixture->xmp_model,
                                            "dc",
                                            "description");
  g_assert (value != NULL);
  g_assert_cmpstr (value[0], ==, "x-default");
  g_assert_cmpstr (value[1], ==, "My good description");
  g_assert_cmpstr (value[2], ==, "de_DE");
  g_assert_cmpstr (value[3], ==, "Deutsche Beschreibung");
}

int main(int argc, char **argv)
{
  gint result = -1;

  g_type_init();
  g_test_init (&argc, &argv, NULL);

  ADD_TEST (test_xmp_model_import_export);
  ADD_TEST (test_xmp_model_import_export_structures);

  result = g_test_run ();

  return result;
}
