
/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcoreapp.h
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

#pragma once

#include <gio/gio.h>

#include "core/core-types.h"
#include "core/gimp.h"

G_BEGIN_DECLS

#define GIMP_TYPE_CORE_APP gimp_core_app_get_type()
G_DECLARE_INTERFACE (GimpCoreApp, gimp_core_app, GIMP, CORE_APP, GObject)

struct _GimpCoreAppInterface
{
  GTypeInterface parent_iface;

  Gimp *        (*get_gimp)              (GimpCoreApp  *self);
  gboolean      (*get_quit)              (GimpCoreApp  *self);
  gboolean      (*get_as_new)            (GimpCoreApp  *self);
  const char ** (*get_filenames)         (GimpCoreApp *self);
  const char *  (*get_batch_interpreter) (GimpCoreApp *self);
  const char ** (*get_batch_commands)    (GimpCoreApp *self);
  void          (*set_exit_status)       (GimpCoreApp *self,
                                          gint         exit_status);
  gint          (*get_exit_status)       (GimpCoreApp *self);
  void          (*set_values)            (GimpCoreApp *self,
                                          Gimp        *gimp,
                                          gboolean     quit,
                                          gboolean     as_new,
                                          const char **filenames,
                                          const char  *batch_interpreter,
                                          const char **batch_commands);

  /* Padding to allow adding up to 12 new virtual functions without
   * breaking ABI. */
  gpointer padding[12];
};

void               gimp_core_app_finalize              (GObject     *object);

Gimp *             gimp_core_app_get_gimp              (GimpCoreApp *self);

gboolean           gimp_core_app_get_quit              (GimpCoreApp *self);

gboolean           gimp_core_app_get_as_new            (GimpCoreApp *self);

const char **      gimp_core_app_get_filenames         (GimpCoreApp *self);

const char *       gimp_core_app_get_batch_interpreter (GimpCoreApp *self);

const char **      gimp_core_app_get_batch_commands    (GimpCoreApp *self);

void               gimp_core_app_set_exit_status       (GimpCoreApp *self,
                                                        gint         exit_status);

gboolean           gimp_core_app_get_exit_status       (GimpCoreApp *self);

void               gimp_core_app_set_values            (GimpCoreApp *self,
                                                        Gimp        *gimp,
                                                        gboolean     quit,
                                                        gboolean     as_new,
                                                        const char **filenames,
                                                        const char  *batch_interpreter,
                                                        const char **batch_commands);

G_END_DECLS
