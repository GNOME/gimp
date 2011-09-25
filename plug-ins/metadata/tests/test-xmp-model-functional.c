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
  gchar  *uri = NULL;
  GError *error = NULL;

  fixture->xmpmodel = xmp_model_new ();

  uri = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                          "plug-ins/metadata/tests/files/test.xmp",
                          NULL);

  xmp_model_parse_file (fixture->xmpmodel, uri, &error);
}


static void
gimp_test_xmp_model_teardown (GimpTestFixture *fixture,
                              gconstpointer    data)
{
  g_object_unref (fixture->xmpmodel);
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
  g_assert (TRUE);
}

int main(int argc, char **argv)
{
  gint result = -1;

  g_type_init();
  g_test_init (&argc, &argv, NULL);

  ADD_TEST (test_xmp_model_import_export);

  result = g_test_run ();

  return result;
}
