/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaprogressbar.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_UI_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligmaui.h> can be included directly."
#endif

#ifndef __LIGMA_PROGRESS_BAR_H__
#define __LIGMA_PROGRESS_BAR_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_PROGRESS_BAR            (ligma_progress_bar_get_type ())
#define LIGMA_PROGRESS_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PROGRESS_BAR, LigmaProgressBar))
#define LIGMA_PROGRESS_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PROGRESS_BAR, LigmaProgressBarClass))
#define LIGMA_IS_PROGRESS_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PROGRESS_BAR))
#define LIGMA_IS_PROGRESS_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PROGRESS_BAR))
#define LIGMA_PROGRESS_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PROGRESS_BAR, LigmaProgressBarClass))


typedef struct _LigmaProgressBarClass  LigmaProgressBarClass;

struct _LigmaProgressBar
{
  GtkProgressBar  parent_instance;

  const gchar    *progress_callback;
  gboolean        cancelable;
};

struct _LigmaProgressBarClass
{
  GtkProgressBarClass  parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
};


GType       ligma_progress_bar_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_progress_bar_new      (void);


G_END_DECLS

#endif /* __LIGMA_PROGRESS_BAR_H__ */
