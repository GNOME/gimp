/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppickbutton-private.h
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

#ifndef __GIMP_PICK_BUTTON_PRIVATE_H__
#define __GIMP_PICK_BUTTON_PRIVATE_H__


typedef struct _GimpPickButtonPrivate
{
  GdkCursor *cursor;
  GtkWidget *grab_widget;
} GimpPickButtonPrivate;


#endif /* ! __GIMP_PICK_BUTTON_PRIVATE_H__ */
