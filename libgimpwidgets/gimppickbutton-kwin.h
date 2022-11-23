/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapickbutton-kwin.h
 * Copyright (C) 2017 Jehan <jehan@ligma.org>
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
#ifndef __LIGMA_PICK_BUTTON_KWIN_H__
#define __LIGMA_PICK_BUTTON_KWIN_H__

gboolean _ligma_pick_button_kwin_available (void);
void     _ligma_pick_button_kwin_pick      (LigmaPickButton *button);

#endif /* __LIGMA_PICK_BUTTON_KWIN_H__ */

