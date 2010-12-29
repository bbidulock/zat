/* See LICENSE file for license and copyright information */

#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <gtk/gtk.h>
#include <girara.h>

gboolean cb_destroy(GtkWidget* widget, gpointer data);
void buffer_changed(girara_session_t* session);
void cb_view_vadjustment_value_changed(GtkAdjustment *adjustment, gpointer data);

#endif // CALLBACKS_H
