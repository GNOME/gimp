/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaapp.c
 * Copyright (C) 2021 Niels De Graef <nielsdegraef@gmail.com>
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

#include "ligmaconsoleapp.h"
#include "ligmacoreapp.h"

struct _LigmaConsoleApp
{
  GApplication parent_instance;
};

G_DEFINE_TYPE_WITH_CODE (LigmaConsoleApp, ligma_console_app, G_TYPE_APPLICATION,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CORE_APP,
                                                ligma_console_app_class_init))


static void
ligma_console_app_class_init (LigmaConsoleAppClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->finalize     = ligma_core_app_finalize;
  gobj_class->get_property = ligma_core_app_get_property;
  gobj_class->set_property = ligma_core_app_set_property;

  ligma_core_app_install_properties (gobj_class);
}

static void
ligma_console_app_init (LigmaConsoleApp *self)
{
}

/*  public functions  */

GApplication *
ligma_console_app_new (Ligma        *ligma,
                      gboolean     quit,
                      gboolean     as_new,
                      const char **filenames,
                      const char  *batch_interpreter,
                      const char **batch_commands)
{
  LigmaConsoleApp *app;

  app = g_object_new (LIGMA_TYPE_CONSOLE_APP,
                      "ligma",              ligma,
                      "filenames",         filenames,
                      "as-new",            as_new,

                      "quit",              quit,
                      "batch-interpreter", batch_interpreter,
                      "batch-commands",    batch_commands,
                      NULL);

  return G_APPLICATION (app);
}
