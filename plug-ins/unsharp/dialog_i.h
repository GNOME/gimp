/* $Id$
 *
 */

#ifndef __DIALOG_I_H__
#define __DIALOG_I_H__

#define MAX_ENTRY_LENGTH 5
#define ENTRY_WIDTH 40
#define SCALE_WIDTH 100
#define ENTRY_PRECISION 1


void dialog_create_value_i(char *title, GtkTable *table, int row,
                           gint *value,
                           gint increment,
                           int left,   int right);

void dialog_iscale_update(GtkAdjustment *adjustment, gint *value);
void dialog_ientry_update(GtkWidget *widget, gint *value);

#endif /* __DIALOG_I_H__ */
