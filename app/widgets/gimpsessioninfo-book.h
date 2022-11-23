/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasessioninfo-book.h
 * Copyright (C) 2001-2007 Michael Natterer <mitch@ligma.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_SESSION_INFO_BOOK_H__
#define __LIGMA_SESSION_INFO_BOOK_H__


/**
 * LigmaSessionInfoBook:
 *
 * Contains information about a book (a GtkNotebook of dockables) in
 * the interface.
 */
struct _LigmaSessionInfoBook
{
  gint   position;
  gint   current_page;

  /*  list of LigmaSessionInfoDockable  */
  GList *dockables;
};


LigmaSessionInfoBook *
             ligma_session_info_book_new         (void);
void         ligma_session_info_book_free        (LigmaSessionInfoBook  *info);

void         ligma_session_info_book_serialize   (LigmaConfigWriter     *writer,
                                                 LigmaSessionInfoBook  *book);
GTokenType   ligma_session_info_book_deserialize (GScanner             *scanner,
                                                 gint                  scope,
                                                 LigmaSessionInfoBook **book);

LigmaSessionInfoBook *
             ligma_session_info_book_from_widget (LigmaDockbook         *dockbook);

LigmaDockbook * ligma_session_info_book_restore   (LigmaSessionInfoBook  *info,
                                                 LigmaDock             *dock);


#endif  /* __LIGMA_SESSION_INFO_BOOK_H__ */
