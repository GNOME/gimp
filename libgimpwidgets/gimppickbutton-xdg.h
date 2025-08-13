/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppickbutton-xdg.h
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

/* Private header file which is not meant to be exported. */
#ifndef __GIMP_PICK_BUTTON_XDG_H__
#define __GIMP_PICK_BUTTON_XDG_H__

gboolean _gimp_pick_button_xdg_available (void);
void     _gimp_pick_button_xdg_pick      (GimpPickButton *button);

#endif /* __GIMP_PICK_BUTTON_XDG_H__ */

