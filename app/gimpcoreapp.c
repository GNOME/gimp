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

#include "gimpcoreapp.h"

#include "libgimpbase/gimpbase.h"

#define GIMP_CORE_APP_GET_PRIVATE(obj) (gimp_core_app_get_private ((GimpCoreApp *) (obj)))

enum
{
  PROP_0,
  PROP_GIMP,
  N_PROPS
};

typedef struct _GimpCoreAppPrivate GimpCoreAppPrivate;

struct _GimpCoreAppPrivate
{
  Gimp          *gimp;
  gboolean       quit;
  gboolean       as_new;
  const gchar  **filenames;
  const gchar   *batch_interpreter;
  const gchar  **batch_commands;
  gint           exit_status;
};

/*  local function prototypes  */

static GimpCoreAppPrivate *
              gimp_core_app_get_private      (GimpCoreApp        *app);
static void   gimp_core_app_private_finalize (GimpCoreAppPrivate *private);

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

  iface->get_gimp              = gimp_core_app_get_gimp;
  iface->get_quit              = gimp_core_app_get_quit;
  iface->get_as_new            = gimp_core_app_get_as_new;
  iface->get_filenames         = gimp_core_app_get_filenames;
  iface->get_batch_interpreter = gimp_core_app_get_batch_interpreter;
  iface->get_batch_commands    = gimp_core_app_get_batch_commands;
  iface->set_exit_status       = gimp_core_app_set_exit_status;
  iface->get_exit_status       = gimp_core_app_get_exit_status;
  iface->set_values            = gimp_core_app_set_values;
}

void
gimp_core_app_finalize (GObject *object)
{
  GimpCoreAppPrivate *private = gimp_core_app_get_private ((GimpCoreApp *) object);

  gimp_core_app_private_finalize (private);
}

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

const char **
gimp_core_app_get_filenames (GimpCoreApp *self)
{
  GimpCoreAppPrivate *private;

  g_return_val_if_fail (GIMP_IS_CORE_APP (self), NULL);

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  return private->filenames;
}

const char *
gimp_core_app_get_batch_interpreter (GimpCoreApp *self)
{
  GimpCoreAppPrivate *private;

  g_return_val_if_fail (GIMP_IS_CORE_APP (self), NULL);

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  return private->batch_interpreter;
}

const char **
gimp_core_app_get_batch_commands (GimpCoreApp *self)
{
  GimpCoreAppPrivate *private;

  g_return_val_if_fail (GIMP_IS_CORE_APP (self), NULL);

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  return private->batch_commands;
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

void
gimp_core_app_set_values(GimpCoreApp  *self,
                         Gimp         *gimp,
                         gboolean      quit,
                         gboolean      as_new,
                         const gchar **filenames,
                         const gchar  *batch_interpreter,
                         const gchar **batch_commands)
{
  GimpCoreAppPrivate *private;

  g_return_if_fail (GIMP_IS_CORE_APP (self));

  private = GIMP_CORE_APP_GET_PRIVATE (self);

  private->gimp              = gimp;
  private->quit              = quit;
  private->as_new            = as_new;
  private->filenames         = filenames;
  private->batch_interpreter = batch_interpreter;
  private->batch_commands    = batch_commands;
}

/*  Private functions  */

static void
gimp_core_app_private_finalize (GimpCoreAppPrivate *private)
{
  g_slice_free (GimpCoreAppPrivate, private);

  g_clear_object (&private->gimp);
}

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

      /* g_signal_connect (view, "destroy",
                        G_CALLBACK (gimp_core_app_private_dispose),
                        private); */
    }

  return private;
}
