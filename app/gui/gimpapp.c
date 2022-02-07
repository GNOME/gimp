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

#include "gimpapp.h"

#include "libgimpbase/gimpbase.h"

struct _GimpApp
{
  GtkApplication parent_instance;

  gboolean       no_splash;
};

G_DEFINE_TYPE_WITH_CODE (GimpApp, gimp_app, GTK_TYPE_APPLICATION,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CORE_APP,
                                                gimp_app_class_init))


static void
gimp_app_class_init (GimpAppClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->finalize     = gimp_core_app_finalize;
}

static void
gimp_app_init (GimpApp *self)
{
}

/*  public functions  */

GApplication *
gimp_app_new (Gimp        *gimp,
              gboolean     no_splash,
              gboolean     quit,
              gboolean     as_new,
              const char **filenames,
              const char  *batch_interpreter,
              const char **batch_commands)
{
  GimpApp *app;

  app = g_object_new (GIMP_TYPE_APP, NULL);

  /* We shouldn't have to pass these externally, so I didn't bother making
   * GObject properties for them. In the end, they should just be parsed by
   * the GApplication code */
  app->no_splash         = no_splash;

  gimp_core_app_set_values(GIMP_CORE_APP(app), gimp, quit, as_new, filenames,
                           batch_interpreter, batch_commands);

  return G_APPLICATION (app);
}

gboolean
gimp_app_get_no_splash (GimpApp *self)
{
  g_return_val_if_fail (GIMP_IS_APP (self), FALSE);
  return GIMP_APP (self)->no_splash;
}
