/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_PROCEDURE_H__
#define __LIGMA_PROCEDURE_H__


#include "core/ligmaviewable.h"


typedef LigmaValueArray * (* LigmaMarshalFunc) (LigmaProcedure         *procedure,
                                              Ligma                  *ligma,
                                              LigmaContext           *context,
                                              LigmaProgress          *progress,
                                              const LigmaValueArray  *args,
                                              GError               **error);


#define LIGMA_TYPE_PROCEDURE            (ligma_procedure_get_type ())
#define LIGMA_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PROCEDURE, LigmaProcedure))
#define LIGMA_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PROCEDURE, LigmaProcedureClass))
#define LIGMA_IS_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PROCEDURE))
#define LIGMA_IS_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PROCEDURE))
#define LIGMA_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PROCEDURE, LigmaProcedureClass))


typedef struct _LigmaProcedureClass LigmaProcedureClass;

struct _LigmaProcedure
{
  LigmaViewable      parent_instance;

  LigmaPDBProcType   proc_type;      /* Type of procedure              */

  gboolean          static_help;    /* Are the strings allocated?     */
  gchar            *blurb;          /* Short procedure description    */
  gchar            *help;           /* Detailed help instructions     */
  gchar            *help_id;        /* Help ID                        */

  gboolean          static_attribution;
  gchar            *authors;        /* Authors field                  */
  gchar            *copyright;      /* Copyright field                */
  gchar            *date;           /* Date field                     */

  gchar            *deprecated;     /* Replacement if deprecated      */

  gchar            *label;          /* Cached label string            */

  gint32            num_args;       /* Number of procedure arguments  */
  GParamSpec      **args;           /* Array of procedure arguments   */

  gint32            num_values;     /* Number of return values        */
  GParamSpec      **values;         /* Array of return values         */

  LigmaMarshalFunc   marshal_func;   /* Marshaller for internal procs  */
};

struct _LigmaProcedureClass
{
  LigmaViewableClass parent_class;

  const gchar    * (* get_label)      (LigmaProcedure   *procedure);
  const gchar    * (* get_menu_label) (LigmaProcedure   *procedure);
  const gchar    * (* get_blurb)      (LigmaProcedure   *procedure);
  const gchar    * (* get_help_id)    (LigmaProcedure   *procedure);
  gboolean         (* get_sensitive)  (LigmaProcedure   *procedure,
                                       LigmaObject      *object,
                                       const gchar    **reason);

  LigmaValueArray * (* execute)        (LigmaProcedure   *procedure,
                                       Ligma            *ligma,
                                       LigmaContext     *context,
                                       LigmaProgress    *progress,
                                       LigmaValueArray  *args,
                                       GError         **error);
  void             (* execute_async)  (LigmaProcedure   *procedure,
                                       Ligma            *ligma,
                                       LigmaContext     *context,
                                       LigmaProgress    *progress,
                                       LigmaValueArray  *args,
                                       LigmaDisplay     *display);
};


GType            ligma_procedure_get_type           (void) G_GNUC_CONST;

LigmaProcedure  * ligma_procedure_new                (LigmaMarshalFunc   marshal_func);

void             ligma_procedure_set_help           (LigmaProcedure    *procedure,
                                                    const gchar      *blurb,
                                                    const gchar      *help,
                                                    const gchar      *help_id);
void             ligma_procedure_set_static_help    (LigmaProcedure    *procedure,
                                                    const gchar      *blurb,
                                                    const gchar      *help,
                                                    const gchar      *help_id);
void             ligma_procedure_take_help          (LigmaProcedure    *procedure,
                                                    gchar            *blurb,
                                                    gchar            *help,
                                                    gchar            *help_id);

void             ligma_procedure_set_attribution    (LigmaProcedure    *procedure,
                                                    const gchar      *authors,
                                                    const gchar      *copyright,
                                                    const gchar      *date);
void             ligma_procedure_set_static_attribution
                                                   (LigmaProcedure    *procedure,
                                                    const gchar      *authors,
                                                    const gchar      *copyright,
                                                    const gchar      *date);
void             ligma_procedure_take_attribution   (LigmaProcedure    *procedure,
                                                    gchar            *authors,
                                                    gchar            *copyright,
                                                    gchar            *date);

void             ligma_procedure_set_deprecated     (LigmaProcedure    *procedure,
                                                    const gchar      *deprecated);

const gchar    * ligma_procedure_get_label          (LigmaProcedure    *procedure);
const gchar    * ligma_procedure_get_menu_label     (LigmaProcedure    *procedure);
const gchar    * ligma_procedure_get_blurb          (LigmaProcedure    *procedure);
const gchar    * ligma_procedure_get_help           (LigmaProcedure    *procedure);
const gchar    * ligma_procedure_get_help_id        (LigmaProcedure    *procedure);
gboolean         ligma_procedure_get_sensitive      (LigmaProcedure    *procedure,
                                                    LigmaObject       *object,
                                                    const gchar     **reason);

void             ligma_procedure_add_argument       (LigmaProcedure    *procedure,
                                                    GParamSpec       *pspec);
void             ligma_procedure_add_return_value   (LigmaProcedure    *procedure,
                                                    GParamSpec       *pspec);

LigmaValueArray * ligma_procedure_get_arguments      (LigmaProcedure    *procedure);
LigmaValueArray * ligma_procedure_get_return_values  (LigmaProcedure    *procedure,
                                                    gboolean          success,
                                                    const GError     *error);

LigmaProcedure  * ligma_procedure_create_override    (LigmaProcedure    *procedure,
                                                    LigmaMarshalFunc   new_marshal_func);

LigmaValueArray * ligma_procedure_execute            (LigmaProcedure    *procedure,
                                                    Ligma             *ligma,
                                                    LigmaContext      *context,
                                                    LigmaProgress     *progress,
                                                    LigmaValueArray   *args,
                                                    GError          **error);
void             ligma_procedure_execute_async      (LigmaProcedure    *procedure,
                                                    Ligma             *ligma,
                                                    LigmaContext      *context,
                                                    LigmaProgress     *progress,
                                                    LigmaValueArray   *args,
                                                    LigmaDisplay      *display,
                                                    GError          **error);

gint             ligma_procedure_name_compare       (LigmaProcedure    *proc1,
                                                    LigmaProcedure    *proc2);



#endif  /*  __LIGMA_PROCEDURE_H__  */
