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
  XMPModel *xmpmodel;
} GimpTestFixture;


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
  fixture->xmpmodel = xmp_model_new ();
}


static void
gimp_test_xmp_model_teardown (GimpTestFixture *fixture,
                              gconstpointer    data)
{
  g_object_unref (fixture->xmpmodel);
}


/**
 * test_xmp_model_is_empty:
 * @fixture:
 * @data:
 *
 * Test to assert that newly created models are empty
 **/
static void
test_xmp_model_is_empty (GimpTestFixture *fixture,
                         gconstpointer    data)
{
  XMPModel *xmpmodel;

  xmpmodel = xmp_model_new ();

  g_assert (xmp_model_is_empty (xmpmodel));
}

/**
 * test_xmp_model_set_get_scalar_property:
 * @fixture:
 * @data:
 *
 * Test to assert that setting and getting scalar properties don't
 * change and are always of XMP_TYPE_TEXT.
 **/
static void
test_xmp_model_set_get_scalar_property (GimpTestFixture *fixture,
                                        gconstpointer    data)
{
  const gchar *property_name = NULL;

  // Schema is nonsense, so nothing is set
  g_assert (xmp_model_set_scalar_property (fixture->xmpmodel,
                                           "SCHEMA",
                                           "key",
                                           "value") == FALSE);
  g_assert (xmp_model_is_empty (fixture->xmpmodel) == TRUE);

  // Contributor is a scalar property
  property_name = "me";
  g_assert (xmp_model_set_scalar_property (fixture->xmpmodel,
                                           "dc",
                                           "contributor",
                                           property_name) == TRUE);
  g_assert (xmp_model_is_empty (fixture->xmpmodel) == FALSE);

  // we expect the same data returned
  g_assert_cmpstr (xmp_model_get_scalar_property (fixture->xmpmodel,
                                                  "dc",
                                                  "contributor"), ==, property_name);
}

/**
 * test_xmp_model_get_raw_property_value:
 * @fixture:
 * @data:
 *
 * Tests the xmp_model_get_raw_property_value, which returns raw values
 * from the XMPModel.
 **/
static void
test_xmp_model_get_raw_property_value (GimpTestFixture *fixture,
                                       gconstpointer    data)
{
  const gchar **expected;
  const gchar **result;

  // NULL is returned if no value is set by given schema and property
  // name
  g_assert (xmp_model_get_raw_property_value (fixture->xmpmodel,
                                              "dc", "title") == NULL);

  // XMP_TYPE_LANG_ALT
  // Note: XMP_TYPE_TEXT is tested with wrapper function
  // xmp_model_set_scalar_property
  expected = g_new (const gchar *, 2);
  expected[0] = g_strdup ("en_GB");
  expected[1] = g_strdup ("my title");
  expected[2] = NULL;
  xmp_model_set_property (fixture->xmpmodel,
                          XMP_TYPE_LANG_ALT,
                          "dc",
                          "title",
                          expected);

  result = xmp_model_get_raw_property_value (fixture->xmpmodel,
                                             "dc", "title");
  g_assert_cmpstr (result[0], ==, expected[0]);
  g_assert_cmpstr (result[1], ==, expected[1]);

  // XMP_TYPE_TEXT_SEQ
  expected = g_new (const gchar *, 1);
  expected[0] = g_strdup ("Wilber");
  expected[1] = NULL;
  g_assert (xmp_model_set_property (fixture->xmpmodel,
                                    XMP_TYPE_TEXT_SEQ,
                                    "dc",
                                    "creator",
                                    expected) == TRUE);
  result = xmp_model_get_raw_property_value (fixture->xmpmodel,
                                             "dc", "creator");
  g_assert (result != NULL);
  g_assert_cmpstr (result[0], ==, expected[0]);
  g_assert_cmpstr (result[1], ==, expected[1]);
}

/**
 * test_xmp_model_parse_file:
 * @fixture:
 * @data:
 *
 * A very simple test parsing a file with XMP DC metadata.
 **/
static void
test_xmp_model_parse_file (GimpTestFixture *fixture,
                           gconstpointer    data)
{
  gchar   *uri = NULL;
  const gchar   *value = NULL;
  GError  *error = NULL;

  uri = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                          "plug-ins/metadata/tests/files/test.xmp",
                          NULL);
  g_assert (uri != NULL);

  xmp_model_parse_file (fixture->xmpmodel, uri, &error);
  g_assert (! xmp_model_is_empty (fixture->xmpmodel));

  // title
  value = xmp_model_get_scalar_property (fixture->xmpmodel, "dc", "title");
  g_assert_cmpstr (value, == , "image title");

  // creator
  value = xmp_model_get_scalar_property (fixture->xmpmodel, "dc", "creator");
  g_assert_cmpstr (value, == , "roman");

  // description
  value = xmp_model_get_scalar_property (fixture->xmpmodel, "dc", "description");
  g_assert_cmpstr (value, == , "bla");

  g_free (uri);
}

int main(int argc, char **argv)
{
  gint result = -1;

  g_type_init();
  g_test_init (&argc, &argv, NULL);

  ADD_TEST (test_xmp_model_is_empty);
  ADD_TEST (test_xmp_model_set_get_scalar_property);
  ADD_TEST (test_xmp_model_get_raw_property_value);
  ADD_TEST (test_xmp_model_parse_file);

  result = g_test_run ();

  return result;
}
