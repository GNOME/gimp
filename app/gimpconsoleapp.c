/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpapp.c
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

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"

#include "gimpconsoleapp.h"
#include "gimpcoreapp.h"

struct _GimpConsoleApp
{
  GApplication parent_instance;
};

G_DEFINE_TYPE_WITH_CODE (GimpConsoleApp, gimp_console_app, G_TYPE_APPLICATION,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CORE_APP,
                                                gimp_console_app_class_init))


static void
gimp_console_app_class_init (GimpConsoleAppClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->finalize     = gimp_core_app_finalize;
  gobj_class->get_property = gimp_core_app_get_property;
  gobj_class->set_property = gimp_core_app_set_property;

  gimp_core_app_install_properties (gobj_class);
}

static void
gimp_console_app_init (GimpConsoleApp *self)
{
}

/*  public functions  */

GApplication *
gimp_console_app_new (Gimp        *gimp,
                      gboolean     quit,
                      gboolean     as_new,
                      const char **filenames,
                      const char  *batch_interpreter,
                      const char **batch_commands)
{
  GimpConsoleApp *app;

  app = g_object_new (GIMP_TYPE_CONSOLE_APP,
                      "gimp",              gimp,
                      "filenames",         filenames,
                      "as-new",            as_new,

                      "quit",              quit,
                      "batch-interpreter", batch_interpreter,
                      "batch-commands",    batch_commands,
                      NULL);

  return G_APPLICATION (app);
}
