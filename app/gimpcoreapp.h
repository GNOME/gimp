
/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacoreapp.h
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

#ifndef __LIGMA_CORE_APP_H__
#define __LIGMA_CORE_APP_H__

G_BEGIN_DECLS

enum
{
  LIGMA_CORE_APP_PROP_0,
  LIGMA_CORE_APP_PROP_LIGMA,
  LIGMA_CORE_APP_PROP_FILENAMES,
  LIGMA_CORE_APP_PROP_AS_NEW,

  LIGMA_CORE_APP_PROP_QUIT,
  LIGMA_CORE_APP_PROP_BATCH_INTERPRETER,
  LIGMA_CORE_APP_PROP_BATCH_COMMANDS,

  LIGMA_CORE_APP_PROP_LAST = LIGMA_CORE_APP_PROP_BATCH_COMMANDS,
};

#define LIGMA_TYPE_CORE_APP ligma_core_app_get_type()
G_DECLARE_INTERFACE (LigmaCoreApp, ligma_core_app, LIGMA, CORE_APP, GObject)

struct _LigmaCoreAppInterface
{
  GTypeInterface parent_iface;

  /* Padding to allow adding up to 12 new virtual functions without
   * breaking ABI. */
  gpointer padding[12];
};

void               ligma_core_app_finalize              (GObject     *object);

Ligma *             ligma_core_app_get_ligma              (LigmaCoreApp *self);

gboolean           ligma_core_app_get_quit              (LigmaCoreApp *self);

gboolean           ligma_core_app_get_as_new            (LigmaCoreApp *self);

const gchar **     ligma_core_app_get_filenames         (LigmaCoreApp *self);

const gchar *      ligma_core_app_get_batch_interpreter (LigmaCoreApp *self);

const gchar **     ligma_core_app_get_batch_commands    (LigmaCoreApp *self);

void               ligma_core_app_set_exit_status       (LigmaCoreApp *self,
                                                        gint         exit_status);

gboolean           ligma_core_app_get_exit_status       (LigmaCoreApp *self);


/* Protected functions. */

void               ligma_core_app_install_properties    (GObjectClass *klass);
void               ligma_core_app_get_property          (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);
void               ligma_core_app_set_property          (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);


G_END_DECLS

#endif /* __LIGMA_CORE_APP_H__ */
