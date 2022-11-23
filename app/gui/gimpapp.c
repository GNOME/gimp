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

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "core/core-types.h"

#include "core/ligma.h"

#include "ligmacoreapp.h"

#include "ligmaapp.h"


enum
{
  PROP_0,
  PROP_NO_SPLASH = LIGMA_CORE_APP_PROP_LAST + 1,
};

struct _LigmaApp
{
  GtkApplication parent_instance;

  gboolean       no_splash;
};


static void ligma_app_get_property (GObject      *object,
                                   guint         property_id,
                                   GValue       *value,
                                   GParamSpec   *pspec);
static void ligma_app_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (LigmaApp, ligma_app, GTK_TYPE_APPLICATION,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CORE_APP,
                                                NULL))


static void
ligma_app_class_init (LigmaAppClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->finalize     = ligma_core_app_finalize;
  gobj_class->get_property = ligma_app_get_property;
  gobj_class->set_property = ligma_app_set_property;

  ligma_core_app_install_properties (gobj_class);

  g_object_class_install_property (gobj_class, PROP_NO_SPLASH,
                                   g_param_spec_boolean ("no-splash", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_app_init (LigmaApp *self)
{
}

static void
ligma_app_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_NO_SPLASH:
      g_value_set_boolean (value, LIGMA_APP (object)->no_splash);
      break;

    default:
      ligma_core_app_get_property (object, property_id, value, pspec);
      break;
    }
}

static void
ligma_app_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_NO_SPLASH:
      LIGMA_APP (object)->no_splash = g_value_get_boolean (value);
      break;

    default:
      ligma_core_app_set_property (object, property_id, value, pspec);
      break;
    }
}

/*  public functions  */

GApplication *
ligma_app_new (Ligma        *ligma,
              gboolean     no_splash,
              gboolean     quit,
              gboolean     as_new,
              const char **filenames,
              const char  *batch_interpreter,
              const char **batch_commands)
{
  LigmaApp *app;

  app = g_object_new (LIGMA_TYPE_APP,
                      "ligma",              ligma,
                      "filenames",         filenames,
                      "as-new",            as_new,

                      "quit",              quit,
                      "batch-interpreter", batch_interpreter,
                      "batch-commands",    batch_commands,

                      "no-splash",         no_splash,
                      NULL);

  return G_APPLICATION (app);
}

gboolean
ligma_app_get_no_splash (LigmaApp *self)
{
  g_return_val_if_fail (LIGMA_IS_APP (self), FALSE);
  return LIGMA_APP (self)->no_splash;
}
