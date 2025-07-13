/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpapp.h
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

#pragma once


#define GIMP_TYPE_APP (gimp_app_get_type ())
G_DECLARE_FINAL_TYPE (GimpApp,
                      gimp_app,
                      GIMP, APP,
                      GtkApplication)


GApplication * gimp_app_new           (Gimp        *gimp,
                                       gboolean     no_splash,
                                       gboolean     quit,
                                       gboolean     as_new,
                                       const char **filenames,
                                       const char  *batch_interpreter,
                                       const char **batch_commands);

gboolean       gimp_app_get_no_splash (GimpApp     *self);
