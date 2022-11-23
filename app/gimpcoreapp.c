/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaapp.c
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

#include "libligmabase/ligmabase.h"

#include "core/core-types.h"

#include "core/ligma.h"

#include "ligmacoreapp.h"


#define LIGMA_CORE_APP_GET_PRIVATE(obj) (ligma_core_app_get_private ((LigmaCoreApp *) (obj)))

typedef struct _LigmaCoreAppPrivate LigmaCoreAppPrivate;

struct _LigmaCoreAppPrivate
{
  Ligma       *ligma;
  gboolean    as_new;
  gchar     **filenames;

  gboolean    quit;
  gchar      *batch_interpreter;
  gchar     **batch_commands;
  gint        exit_status;
};

/*  local function prototypes  */

static LigmaCoreAppPrivate * ligma_core_app_get_private      (LigmaCoreApp        *app);
static void                 ligma_core_app_private_finalize (LigmaCoreAppPrivate *private);


G_DEFINE_INTERFACE (LigmaCoreApp, ligma_core_app, G_TYPE_OBJECT)

static void
ligma_core_app_default_init (LigmaCoreAppInterface *iface)
{
  /* add properties and signals to the interface here */
  g_object_interface_install_property (iface,
                                       g_param_spec_object ("ligma",
                                                            "LIGMA",
                                                            "LIGMA root object",
                                                            LIGMA_TYPE_LIGMA,
                                                            LIGMA_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_interface_install_property (iface,
                                       g_param_spec_boxed ("filenames",
                                                           "Files to open at start",
                                                           "Files to open at start",
                                                           G_TYPE_STRV,
                                                           LIGMA_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("as-new",
                                                             "Open images as new",
                                                             "Open images as new",
                                                             FALSE,
                                                             LIGMA_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("quit",
                                                             "Quit",
                                                             "Quit LIGMA immediately after running batch commands",
                                                             FALSE,
                                                             LIGMA_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_interface_install_property (iface,
                                       g_param_spec_string ("batch-interpreter",
                                                            "The procedure to process batch commands with",
                                                            "The procedure to process batch commands with",
                                                            NULL,
                                                            LIGMA_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_interface_install_property (iface,
                                       g_param_spec_boxed ("batch-commands",
                                                           "Batch commands to run",
                                                           "Batch commands to run",
                                                           G_TYPE_STRV,
                                                           LIGMA_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


/* Protected functions. */

void
ligma_core_app_finalize (GObject *object)
{
  g_object_set_qdata (object,
                      g_quark_from_static_string ("ligma-core-app-private"),
                      NULL);
}

/**
 * ligma_container_view_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #LigmaCoreApp. Please call this function in the *_class_init()
 * function of the child class.
 **/
void
ligma_core_app_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass, LIGMA_CORE_APP_PROP_LIGMA, "ligma");
  g_object_class_override_property (klass, LIGMA_CORE_APP_PROP_FILENAMES, "filenames");
  g_object_class_override_property (klass, LIGMA_CORE_APP_PROP_AS_NEW, "as-new");

  g_object_class_override_property (klass, LIGMA_CORE_APP_PROP_QUIT, "quit");
  g_object_class_override_property (klass, LIGMA_CORE_APP_PROP_BATCH_INTERPRETER, "batch-interpreter");
  g_object_class_override_property (klass, LIGMA_CORE_APP_PROP_BATCH_COMMANDS, "batch-commands");
}

void
ligma_core_app_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  LigmaCoreAppPrivate *private;

  private = LIGMA_CORE_APP_GET_PRIVATE (object);

  switch (property_id)
    {
    case LIGMA_CORE_APP_PROP_LIGMA:
      private->ligma = g_value_get_object (value);
      break;
    case LIGMA_CORE_APP_PROP_FILENAMES:
      private->filenames = g_value_dup_boxed (value);
      break;
    case LIGMA_CORE_APP_PROP_AS_NEW:
      private->as_new = g_value_get_boolean (value);
      break;
    case LIGMA_CORE_APP_PROP_QUIT:
      private->quit = g_value_get_boolean (value);
      break;
    case LIGMA_CORE_APP_PROP_BATCH_INTERPRETER:
      private->batch_interpreter = g_value_dup_string (value);
      break;
    case LIGMA_CORE_APP_PROP_BATCH_COMMANDS:
      private->batch_commands = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
ligma_core_app_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  LigmaCoreAppPrivate *private;

  private = LIGMA_CORE_APP_GET_PRIVATE (object);

  switch (property_id)
    {
    case LIGMA_CORE_APP_PROP_LIGMA:
      g_value_set_object (value, private->ligma);
      break;
    case LIGMA_CORE_APP_PROP_FILENAMES:
      g_value_set_static_boxed (value, private->filenames);
      break;
    case LIGMA_CORE_APP_PROP_AS_NEW:
      g_value_set_boolean (value, private->as_new);
      break;
    case LIGMA_CORE_APP_PROP_QUIT:
      g_value_set_boolean (value, private->quit);
      break;
    case LIGMA_CORE_APP_PROP_BATCH_INTERPRETER:
      g_value_set_static_string (value, private->batch_interpreter);
      break;
    case LIGMA_CORE_APP_PROP_BATCH_COMMANDS:
      g_value_set_static_boxed (value, private->batch_commands);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public functions. */

Ligma *
ligma_core_app_get_ligma (LigmaCoreApp *self)
{
  LigmaCoreAppPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CORE_APP (self), NULL);

  private = LIGMA_CORE_APP_GET_PRIVATE (self);

  return private->ligma;
}

gboolean
ligma_core_app_get_quit (LigmaCoreApp *self)
{
  LigmaCoreAppPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CORE_APP (self), FALSE);

  private = LIGMA_CORE_APP_GET_PRIVATE (self);

  return private->quit;
}

gboolean
ligma_core_app_get_as_new (LigmaCoreApp *self)
{
  LigmaCoreAppPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CORE_APP (self), FALSE);

  private = LIGMA_CORE_APP_GET_PRIVATE (self);

  return private->as_new;
}

const gchar **
ligma_core_app_get_filenames (LigmaCoreApp *self)
{
  LigmaCoreAppPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CORE_APP (self), NULL);

  private = LIGMA_CORE_APP_GET_PRIVATE (self);

  return (const gchar **) private->filenames;
}

const gchar *
ligma_core_app_get_batch_interpreter (LigmaCoreApp *self)
{
  LigmaCoreAppPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CORE_APP (self), NULL);

  private = LIGMA_CORE_APP_GET_PRIVATE (self);

  return (const gchar *) private->batch_interpreter;
}

const gchar **
ligma_core_app_get_batch_commands (LigmaCoreApp *self)
{
  LigmaCoreAppPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CORE_APP (self), NULL);

  private = LIGMA_CORE_APP_GET_PRIVATE (self);

  return (const gchar **) private->batch_commands;
}

void
ligma_core_app_set_exit_status (LigmaCoreApp *self, gint exit_status)
{
  LigmaCoreAppPrivate *private;

  g_return_if_fail (LIGMA_IS_CORE_APP (self));

  private = LIGMA_CORE_APP_GET_PRIVATE (self);

  private->exit_status = exit_status;
}

gint
ligma_core_app_get_exit_status (LigmaCoreApp *self)
{
  LigmaCoreAppPrivate *private;

  g_return_val_if_fail (LIGMA_IS_CORE_APP (self), EXIT_FAILURE);

  private = LIGMA_CORE_APP_GET_PRIVATE (self);

  return private->exit_status;
}

/*  Private functions  */

static LigmaCoreAppPrivate *
ligma_core_app_get_private (LigmaCoreApp *app)
{
  LigmaCoreAppPrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (LIGMA_IS_CORE_APP (app), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("ligma-core-app-private");

  private = g_object_get_qdata ((GObject *) app, private_key);

  if (! private)
    {
      private = g_slice_new0 (LigmaCoreAppPrivate);

      g_object_set_qdata_full ((GObject *) app, private_key, private,
                               (GDestroyNotify) ligma_core_app_private_finalize);
    }

  return private;
}

static void
ligma_core_app_private_finalize (LigmaCoreAppPrivate *private)
{
  g_clear_pointer (&private->filenames, g_strfreev);
  g_clear_pointer (&private->batch_interpreter, g_free);
  g_clear_pointer (&private->batch_commands, g_strfreev);

  g_slice_free (LigmaCoreAppPrivate, private);
}
