/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpapp.c
 * Copyright (C) 2022 Lukas Oberhuber <lukaso@gmail.com>
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"

#include "gimpcoreapp.h"


#define GIMP_CORE_APP_GET_PRIVATE(obj) (gimp_core_app_get_private ((GimpCoreApp *) (obj)))

typedef struct _GimpCoreAppPrivate GimpCoreAppPrivate;

struct _GimpCoreAppPrivate
{
  Gimp       *gimp;
  gboolean    as_new;
  gchar     **filenames;

  gboolean    quit;
  gchar      *batch_interpreter;
  gchar     **batch_commands;
  gint        exit_status;
};

/*  local function prototypes  */

static GimpCoreAppPrivate * gimp_core_app_get_private      (GimpCoreApp        *app);
static void                 gimp_core_app_private_finalize (GimpCoreAppPrivate *private);


G_DEFINE_INTERFACE (GimpCoreApp, gimp_core_app, G_TYPE_OBJECT)

static void
gimp_core_app_default_init (GimpCoreAppInterface *iface)
{
  /* add properties and signals to the interface here */
  g_object_interface_install_property (iface,
                                       g_param_spec_object ("gimp",
                                                            "GIMP",
                                                            "GIMP root object",
                                                            GIMP_TYPE_GIMP,
                                                            GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_interface_install_property (iface,
                                       g_param_spec_boxed ("filenames",
                                                           "Files to open at start",
                                                           "Files to open at start",
                                                           G_TYPE_STRV,
                                                           GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("as-new",
                                                             "Open images as new",
                                                             "Open images as new",
                                                             FALSE,
                                                             GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("quit",
                                                             "Quit",
                                                             "Quit GIMP immediately after running batch commands",
                                                             FALSE,
                                                             GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_interface_install_property (iface,
                                       g_param_spec_string ("batch-interpreter",
                                                            "The procedure to process batch commands with",
                                                            "The procedure to process batch commands with",
                                                            NULL,
                                                            GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_interface_install_property (iface,
                                       g_param_spec_boxed ("batch-commands",
                                                           "Batch commands to run",
                                                           "Batch commands to run",
                                                           G_TYPE_STRV,
                                                           GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


/* Protected functions. */

/**
 * gimp_container_view_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #GimpCoreApp. Please call this function in the *_class_init()
 * function of the child class.
 **/
void
gimp_core_app_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass, GIMP_CORE_APP_PROP_GIMP, "gimp");
  g_object_class_override_property (klass, GIMP_CORE_APP_PROP_FILENAMES, "filenames");
  g_object_class_override_property (klass, GIMP_CORE_APP_PROP_AS_NEW, "as-new");

  g_object_class_override_property (klass, GIMP_CORE_APP_PROP_QUIT, "quit");
  g_object_class_override_property (klass, GIMP_CORE_APP_PROP_BATCH_INTERPRETER, "batch-interpreter");
  g_object_class_override_property (klass, GIMP_CORE_APP_PROP_BATCH_COMMANDS, "batch-commands");
}

void
gimp_core_app_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpCoreAppPrivate *private;

  private = GIMP_CORE_APP_GET_PRIVATE (object);

  switch (property_id)
    {
    case GIMP_CORE_APP_PROP_GIMP:
      private->gimp = g_value_get_object (value);
      break;
    case GIMP_CORE_APP_PROP_FILENAMES:
      private->filenames = g_value_dup_boxed (value);
      break;
    case GIMP_CORE_APP_PROP_AS_NEW:
      private->as_new = g_value_get_boolean (value);
      break;
    case GIMP_CORE_APP_PROP_QUIT:
      private->quit = g_value_get_boolean (value);
      break;
    case GIMP_CORE_APP_PROP_BATCH_INTERPRETER:
      private->batch_interpreter = g_value_dup_string (value);
      break;
    case GIMP_CORE_APP_PROP_BATCH_COMMANDS:
      private->batch_commands = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_core_app_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpCoreAppPrivate *private;

  private = GIMP_CORE_APP_GET_PRIVATE (object);

  switch (property_id)
    {
    case GIMP_CORE_APP_PROP_GIMP:
      g_value_set_object (value, private->gimp);
      break;
    case GIMP_CORE_APP_PROP_FILENAMES:
      g_value_set_static_boxed (value, private->filenames);
      break;
    case GIMP_CORE_APP_PROP_AS_NEW:
      g_value_set_boolean (value, private->as_new);
      break;
    case GIMP_CORE_APP_PROP_QUIT:
      g_value_set_boolean (value, private->quit);
      break;
    case GIMP_CORE_APP_PROP_BATCH_INTERPRETER:
      g_value_set_static_string (value, private->batch_interpreter);
      break;
    case GIMP_CORE_APP_PROP_BATCH_COMMANDS:
      g_value_set_static_boxed (value, private->batch_commands);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public functions. */

Gimp *
gimp_core_app_get_gimp (GimpCoreApp *self)
{
  GimpCoreAppPrivate *private;

  g_return_val_if_fail (GIMP_IS_CORE_APP (self), NULL);

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  return private->gimp;
}

gboolean
gimp_core_app_get_quit (GimpCoreApp *self)
{
  GimpCoreAppPrivate *private;

  g_return_val_if_fail (GIMP_IS_CORE_APP (self), FALSE);

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  return private->quit;
}

gboolean
gimp_core_app_get_as_new (GimpCoreApp *self)
{
  GimpCoreAppPrivate *private;

  g_return_val_if_fail (GIMP_IS_CORE_APP (self), FALSE);

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  return private->as_new;
}

const gchar **
gimp_core_app_get_filenames (GimpCoreApp *self)
{
  GimpCoreAppPrivate *private;

  g_return_val_if_fail (GIMP_IS_CORE_APP (self), NULL);

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  return (const gchar **) private->filenames;
}

const gchar *
gimp_core_app_get_batch_interpreter (GimpCoreApp *self)
{
  GimpCoreAppPrivate *private;

  g_return_val_if_fail (GIMP_IS_CORE_APP (self), NULL);

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  return (const gchar *) private->batch_interpreter;
}

const gchar **
gimp_core_app_get_batch_commands (GimpCoreApp *self)
{
  GimpCoreAppPrivate *private;

  g_return_val_if_fail (GIMP_IS_CORE_APP (self), NULL);

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  return (const gchar **) private->batch_commands;
}

void
gimp_core_app_set_exit_status (GimpCoreApp *self, gint exit_status)
{
  GimpCoreAppPrivate *private;

  g_return_if_fail (GIMP_IS_CORE_APP (self));

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  private->exit_status = exit_status;
}

gint
gimp_core_app_get_exit_status (GimpCoreApp *self)
{
  GimpCoreAppPrivate *private;

  g_return_val_if_fail (GIMP_IS_CORE_APP (self), EXIT_FAILURE);

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  return private->exit_status;
}

/*  Private functions  */

static GimpCoreAppPrivate *
gimp_core_app_get_private (GimpCoreApp *app)
{
  GimpCoreAppPrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (GIMP_IS_CORE_APP (app), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("gimp-core-app-private");

  private = g_object_get_qdata ((GObject *) app, private_key);

  if (! private)
    {
      private = g_slice_new0 (GimpCoreAppPrivate);

      g_object_set_qdata_full ((GObject *) app, private_key, private,
                               (GDestroyNotify) gimp_core_app_private_finalize);
    }

  return private;
}

static void
gimp_core_app_private_finalize (GimpCoreAppPrivate *private)
{
  g_clear_pointer (&private->filenames, g_strfreev);
  g_clear_pointer (&private->batch_interpreter, g_free);
  g_clear_pointer (&private->batch_commands, g_strfreev);

  g_slice_free (GimpCoreAppPrivate, private);
}
