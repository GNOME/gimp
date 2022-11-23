/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaapp.h
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

#ifndef __LIGMA_APP_H__
#define __LIGMA_APP_H__


#define LIGMA_TYPE_APP (ligma_app_get_type ())
G_DECLARE_FINAL_TYPE (LigmaApp, ligma_app, LIGMA, APP, GtkApplication)

GApplication * ligma_app_new                   (Ligma        *ligma,
                                               gboolean     no_splash,
                                               gboolean     quit,
                                               gboolean     as_new,
                                               const char **filenames,
                                               const char  *batch_interpreter,
                                               const char **batch_commands);

gboolean       ligma_app_get_no_splash         (LigmaApp     *self);

#endif /* __LIGMA_APP_H__ */
