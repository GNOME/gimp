#ifndef __DIALOG_F_H__
#define __DIALOG_F_H__

#define MAX_ENTRY_LENGTH 5
#define ENTRY_WIDTH 40
#define SCALE_WIDTH 100
#define ENTRY_PRECISION 1


void dialog_create_value_f(char *title, GtkTable *table, int row,
                           gdouble *value,
                           gdouble increment, int precision,
                           int left,   int right);

void dialog_fscale_update(GtkAdjustment *adjustment, gdouble *value);
void dialog_fentry_update(GtkWidget *widget, gdouble   *value);

#endif /* __DIALOG_F_H__ */
