/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppickbutton-kwin.h
 * Copyright (C) 2017 Jehan <jehan@gimp.org>
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

/* Private header file which is not meant to be exported. */

#pragma once


G_GNUC_INTERNAL gboolean _gimp_pick_button_kwin_available (void);
G_GNUC_INTERNAL void     _gimp_pick_button_kwin_pick      (GimpPickButton *button);
