/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapickbutton-private.h
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

#ifndef __LIGMA_PICK_BUTTON_PRIVATE_H__
#define __LIGMA_PICK_BUTTON_PRIVATE_H__


struct _LigmaPickButtonPrivate
{
  GdkCursor *cursor;
  GtkWidget *grab_widget;
};


#endif /* ! __LIGMA_PICK_BUTTON_PRIVATE_H__ */
