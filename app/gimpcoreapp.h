
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

#ifndef __GIMP_CORE_APP_H__
#define __GIMP_CORE_APP_H__

G_BEGIN_DECLS

#define GIMP_APPLICATION_ID "org.gimp.GIMP"

enum
{
  GIMP_CORE_APP_PROP_0,
  GIMP_CORE_APP_PROP_GIMP,
  GIMP_CORE_APP_PROP_FILENAMES,
  GIMP_CORE_APP_PROP_AS_NEW,

  GIMP_CORE_APP_PROP_QUIT,
  GIMP_CORE_APP_PROP_BATCH_INTERPRETER,
  GIMP_CORE_APP_PROP_BATCH_COMMANDS,

  GIMP_CORE_APP_PROP_LAST = GIMP_CORE_APP_PROP_BATCH_COMMANDS,
};

#define GIMP_TYPE_CORE_APP gimp_core_app_get_type()
G_DECLARE_INTERFACE (GimpCoreApp, gimp_core_app, GIMP, CORE_APP, GObject)

struct _GimpCoreAppInterface
{
  GTypeInterface parent_iface;

  /* Padding to allow adding up to 12 new virtual functions without
   * breaking ABI. */
  gpointer padding[12];
};

Gimp *             gimp_core_app_get_gimp              (GimpCoreApp *self);

gboolean           gimp_core_app_get_quit              (GimpCoreApp *self);

gboolean           gimp_core_app_get_as_new            (GimpCoreApp *self);

const gchar **     gimp_core_app_get_filenames         (GimpCoreApp *self);

const gchar *      gimp_core_app_get_batch_interpreter (GimpCoreApp *self);

const gchar **     gimp_core_app_get_batch_commands    (GimpCoreApp *self);

void               gimp_core_app_set_exit_status       (GimpCoreApp *self,
                                                        gint         exit_status);

gboolean           gimp_core_app_get_exit_status       (GimpCoreApp *self);


/* Protected functions. */

void               gimp_core_app_install_properties    (GObjectClass *klass);
void               gimp_core_app_get_property          (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);
void               gimp_core_app_set_property          (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);


G_END_DECLS

#endif /* __GIMP_CORE_APP_H__ */
