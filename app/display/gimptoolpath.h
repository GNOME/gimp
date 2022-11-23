/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolpath.h
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TOOL_PATH_H__
#define __LIGMA_TOOL_PATH_H__


#include "ligmatoolwidget.h"


#define LIGMA_TYPE_TOOL_PATH            (ligma_tool_path_get_type ())
#define LIGMA_TOOL_PATH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_PATH, LigmaToolPath))
#define LIGMA_TOOL_PATH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_PATH, LigmaToolPathClass))
#define LIGMA_IS_TOOL_PATH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_PATH))
#define LIGMA_IS_TOOL_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_PATH))
#define LIGMA_TOOL_PATH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_PATH, LigmaToolPathClass))


typedef struct _LigmaToolPath        LigmaToolPath;
typedef struct _LigmaToolPathPrivate LigmaToolPathPrivate;
typedef struct _LigmaToolPathClass   LigmaToolPathClass;

struct _LigmaToolPath
{
  LigmaToolWidget       parent_instance;

  LigmaToolPathPrivate *private;
};

struct _LigmaToolPathClass
{
  LigmaToolWidgetClass  parent_class;

  void (* begin_change) (LigmaToolPath    *path,
                         const gchar     *desc);
  void (* end_change)   (LigmaToolPath    *path,
                         gboolean         success);
  void (* activate)     (LigmaToolPath    *path,
                         GdkModifierType  state);
};


GType            ligma_tool_path_get_type    (void) G_GNUC_CONST;

LigmaToolWidget * ligma_tool_path_new         (LigmaDisplayShell *shell);

void             ligma_tool_path_set_vectors (LigmaToolPath     *path,
                                             LigmaVectors      *vectors);

void             ligma_tool_path_get_popup_state (LigmaToolPath *path,
                                                 gboolean     *on_handle,
                                                 gboolean     *on_curve);

void             ligma_tool_path_delete_anchor  (LigmaToolPath *path);
void             ligma_tool_path_shift_start    (LigmaToolPath *path);
void             ligma_tool_path_insert_anchor  (LigmaToolPath *path);
void             ligma_tool_path_delete_segment (LigmaToolPath *path);
void             ligma_tool_path_reverse_stroke (LigmaToolPath *path);

#endif /* __LIGMA_TOOL_PATH_H__ */
