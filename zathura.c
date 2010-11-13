/* See LICENSE file for license and copyright information */

#include "callbacks.h"
#include "config.h"
#include "shortcuts.h"
#include "zathura.h"

/* function implementation */
bool
init_zathura()
{
  if(!(Zathura.UI.session = girara_session_create()))
    return false;

  if(!girara_session_init(Zathura.UI.session))
    return false;

  /* UI */
  Zathura.UI.buffer = girara_statusbar_item_add(Zathura.UI.session, FALSE, FALSE, FALSE, NULL);
  if(!Zathura.UI.buffer) {
    girara_session_destroy(Zathura.UI.session);
    return false;
  }

  /* signals */
  g_signal_connect(G_OBJECT(Zathura.UI.session->gtk.window), "destroy", G_CALLBACK(cb_destroy), NULL);

  /* girara events */
  Zathura.UI.session->events.buffer_changed = buffer_changed;

  /* configuration */
  config_load_default();

  return true;
}

/* main function */
int main(int argc, char* argv[])
{
  g_thread_init(NULL);
  gdk_threads_init();
  gtk_init(&argc, &argv);

  if(!init_zathura()) {
    printf("error: coult not initialize zathura\n");
    return -1;
  }

  gdk_threads_enter();
  gtk_main();
  gdk_threads_leave();

  return 0;
}
