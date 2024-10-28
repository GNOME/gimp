/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2024 Jehan
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <libgimp/gimp.h>

static GimpPDBStatusType  status   = GIMP_PDB_SUCCESS;
static GError            *error    = NULL;
static const gchar       *testname = NULL;

#define GIMP_TEST_RETURN \
gimp_test_cleanup: \
    return gimp_procedure_new_return_values (procedure, status, error);

#define GIMP_TEST_START(name) if (error) goto gimp_test_cleanup; testname = name; printf ("Starting test '%s': ", testname);

#define GIMP_TEST_END(test) \
  if (! (test)) \
    { \
      printf ("FAILED\n"); \
      error = g_error_new_literal (GIMP_PLUG_IN_ERROR, 0, testname); \
      status = GIMP_PDB_EXECUTION_ERROR; \
    } \
  else \
    { \
      printf ("OK\n"); \
    }

typedef struct _GimpCTest      GimpCTest;
typedef struct _GimpCTestClass GimpCTestClass;

struct _GimpCTest
{
  GimpPlugIn      parent_instance;
};

struct _GimpCTestClass
{
  GimpPlugInClass parent_class;
};


/* Declare local functions.
 */

#define GIMP_C_TEST_TYPE  (gimp_c_test_get_type ())
#define GIMP_C_TEST (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_C_TEST_TYPE, GimpCTest))

GType                   gimp_c_test_get_type         (void) G_GNUC_CONST;

static GList          * gimp_c_test_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * gimp_c_test_create_procedure (GimpPlugIn           *plug_in,
                                                      const gchar          *name);

static GimpValueArray * gimp_c_test_run              (GimpProcedure        *procedure,
                                                      GimpRunMode           run_mode,
                                                      GimpImage            *image,
                                                      GimpDrawable        **drawables,
                                                      GimpProcedureConfig  *config,
                                                      gpointer              run_data);


G_DEFINE_TYPE (GimpCTest, gimp_c_test, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIMP_C_TEST_TYPE)


static void
gimp_c_test_class_init (GimpCTestClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = gimp_c_test_query_procedures;
  plug_in_class->create_procedure = gimp_c_test_create_procedure;
}

static void
gimp_c_test_init (GimpCTest *ctest)
{
}

static GList *
gimp_c_test_query_procedures (GimpPlugIn *plug_in)
{
  gchar *testname  = g_path_get_basename (__FILE__);
  gchar *dot;

  dot  = g_strrstr (testname, ".");
  *dot = '\0';

  return g_list_append (NULL, testname);
}

static GimpProcedure *
gimp_c_test_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;
  gchar         *testname  = g_path_get_basename (__FILE__);
  gchar         *dot;

  dot  = g_strrstr (testname, ".");
  *dot = '\0';

  if (! strcmp (name, testname))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            gimp_c_test_run, NULL, NULL);
      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask(procedure, GIMP_PROCEDURE_SENSITIVE_ALWAYS);
    }

  g_free (testname);

  return procedure;
}
