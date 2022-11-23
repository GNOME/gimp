/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmaseamlesscloneoptions.h
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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

#ifndef __LIGMA_SEAMLESS_CLONE_OPTIONS_H__
#define __LIGMA_SEAMLESS_CLONE_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_SEAMLESS_CLONE_OPTIONS            (ligma_seamless_clone_options_get_type ())
#define LIGMA_SEAMLESS_CLONE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SEAMLESS_CLONE_OPTIONS, LigmaSeamlessCloneOptions))
#define LIGMA_SEAMLESS_CLONE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SEAMLESS_CLONE_OPTIONS, LigmaSeamlessCloneOptionsClass))
#define LIGMA_IS_SEAMLESS_CLONE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SEAMLESS_CLONE_OPTIONS))
#define LIGMA_IS_SEAMLESS_CLONE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SEAMLESS_CLONE_OPTIONS))
#define LIGMA_SEAMLESS_CLONE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SEAMLESS_CLONE_OPTIONS, LigmaSeamlessCloneOptionsClass))


typedef struct _LigmaSeamlessCloneOptions      LigmaSeamlessCloneOptions;
typedef struct _LigmaSeamlessCloneOptionsClass LigmaSeamlessCloneOptionsClass;

struct _LigmaSeamlessCloneOptions
{
  LigmaToolOptions parent_instance;

  gint            max_refine_scale;
  gboolean        temp;
};

struct _LigmaSeamlessCloneOptionsClass
{
  LigmaToolOptionsClass  parent_class;
};


GType       ligma_seamless_clone_options_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_seamless_clone_options_gui      (LigmaToolOptions *tool_options);


#endif  /*  __LIGMA_SEAMLESS_CLONE_OPTIONS_H__  */
