/* See LICENSE file for license and copyright information */

#include <regex.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* macros */
#define LENGTH(x) sizeof(x)/sizeof((x)[0])

/* enums */
enum { NEXT, PREVIOUS, LEFT, RIGHT, UP, DOWN,
       BOTTOM, TOP, HIDE, NORMAL, HIGHLIGHT,
       INSERT, VISUAL, DELETE_LAST_WORD, DEFAULT,
       ERROR, WARNING, NEXT_GROUP, PREVIOUS_GROUP };

/* zathura */
struct
{
  struct
  {
    GtkWindow         *window;
    GtkBox            *box;
    GtkScrolledWindow *view;
    GtkWidget         *statusbar;
    GtkBox            *statusbar_entries;
    GtkEntry          *inputbar;
  } UI;

  struct
  {
    GdkColor default_fg;
    GdkColor default_bg;
    GdkColor inputbar_fg;
    GdkColor inputbar_bg;
    GdkColor statusbar_fg;
    GdkColor statusbar_bg;
    GdkColor completion_fg;
    GdkColor completion_bg;
    GdkColor completion_g_bg;
    GdkColor completion_g_fg;
    GdkColor completion_hl_fg;
    GdkColor completion_hl_bg;
    GdkColor notification_e_fg;
    GdkColor notification_e_bg;
    GdkColor notification_w_fg;
    GdkColor notification_w_bg;
    PangoFontDescription *font;
  } Style;

  struct
  {
    GString *buffer;
    GList   *history;
    int      mode;
    GtkLabel *status_text;
    GtkLabel *status_buffer;
    GtkLabel *status_state;
  } Global;

} Zathura;

/* typedefs */
struct CElement
{
  char            *value;
  char            *description;
  struct CElement *next;
};

typedef struct CElement CompletionElement;

struct CGroup
{
  char              *value;
  CompletionElement *elements;
  struct CGroup     *next;
};

typedef struct CGroup CompletionGroup;

typedef struct
{
  CompletionGroup* groups;
} Completion;

typedef struct
{
  char*      command;
  char*      description;
  int        command_id;
  gboolean   is_group;
  GtkWidget* row;
} CompletionRow;

typedef struct
{
  int   n;
  void *data;
} Argument;

typedef struct
{
  int mask;
  int key;
  void (*function)(Argument*);
  int mode;
  Argument argument;
} Shortcut;

typedef struct
{
  int mask;
  int key;
  void (*function)(Argument*);
  Argument argument;
} InputbarShortcut;

typedef struct
{
  char* command;
  char* abbr;
  void (*function)(int, char**);
  Completion* (*completion)(char*)q
  char* description;
} Command;

typedef struct
{
  char* regex;
  void (*function)(char*, Argument*);
  Argument argument;
} BufferCommand;

typedef struct
{
  char identifier;
  void (*function)(char*, Argument*);
  int always;
  Argument argument;
} SpecialCommand;

/* function declarations */
void init_zathura();
void change_mode(int);
void notify(int, char*);
void update_status(char*, char*);
void setCompletionRowColor(GtkBox*, int, int);
GtkEventBox* createCompletionRow(GtkBox*, char*, char*, gboolean);

/* shortcut declarations */
void sc_abort(Argument*);
void sc_change_mode(Argument*);
void sc_focus_inputbar(Argument*);
void sc_quit(Argument*);

/* inputbar shortcut declarations */
void isc_abort(Argument*);
void isc_command_history(Argument*);
void isc_completion(Argument*);
void isc_string_manipulation(Argument*);

/* command declarations */
void cmd_quit(int, char**);

/* completion commands */
Completion* cc_open(char*);

/* buffer command declarations */
void bcmd_goto(char*, Argument*);

/* special command delcarations */
void scmd_search(char*, Argument*);

/* callback declarations */
gboolean cb_destroy(GtkWidget*, gpointer);
gboolean cb_view_kb_pressed(GtkWidget*, GdkEventKey*, gpointer);
gboolean cb_inputbar_kb_pressed(GtkWidget*, GdkEventKey*, gpointer);
gboolean cb_inputbar_activate(GtkEntry*, gpointer);

/* configuration */
#include "config.h"

/* function implementation */
void
init_zathura()
{
  /* look */
  gdk_color_parse(default_fgcolor,        &(Zathura.Style.default_fg));
  gdk_color_parse(default_bgcolor,        &(Zathura.Style.default_bg));
  gdk_color_parse(inputbar_fgcolor,       &(Zathura.Style.inputbar_fg));
  gdk_color_parse(inputbar_bgcolor,       &(Zathura.Style.inputbar_bg));
  gdk_color_parse(statusbar_fgcolor,      &(Zathura.Style.statusbar_fg));
  gdk_color_parse(statusbar_bgcolor,      &(Zathura.Style.statusbar_bg));
  gdk_color_parse(completion_fgcolor,     &(Zathura.Style.completion_fg));
  gdk_color_parse(completion_bgcolor,     &(Zathura.Style.completion_bg));
  gdk_color_parse(completion_g_fgcolor,   &(Zathura.Style.completion_g_fg));
  gdk_color_parse(completion_g_fgcolor,   &(Zathura.Style.completion_g_fg));
  gdk_color_parse(completion_hl_fgcolor,  &(Zathura.Style.completion_hl_fg));
  gdk_color_parse(completion_hl_bgcolor,  &(Zathura.Style.completion_hl_bg));
  gdk_color_parse(notification_e_fgcolor, &(Zathura.Style.notification_e_fg));
  gdk_color_parse(notification_e_bgcolor, &(Zathura.Style.notification_e_bg));
  gdk_color_parse(notification_w_fgcolor, &(Zathura.Style.notification_w_fg));
  gdk_color_parse(notification_w_bgcolor, &(Zathura.Style.notification_w_bg));
  Zathura.Style.font = pango_font_description_from_string(font);

  /* other */
  Zathura.Global.mode = NORMAL;

  /* UI */
  Zathura.UI.window            = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  Zathura.UI.box               = GTK_BOX(gtk_vbox_new(FALSE, 0));
  Zathura.UI.view              = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
  Zathura.UI.statusbar         = gtk_event_box_new();
  Zathura.UI.statusbar_entries = GTK_BOX(gtk_hbox_new(FALSE, 0));
  Zathura.UI.inputbar          = GTK_ENTRY(gtk_entry_new());

  /* window */
  gtk_window_set_title(Zathura.UI.window, "zathura");
  GdkGeometry hints = { 1, 1 };
  gtk_window_set_geometry_hints(Zathura.UI.window, NULL, &hints, GDK_HINT_MIN_SIZE);
  gtk_window_set_default_size(Zathura.UI.window, DEFAULT_WIDTH, DEFAULT_HEIGHT);
  g_signal_connect(G_OBJECT(Zathura.UI.window), "destroy", G_CALLBACK(cb_destroy), NULL);

  /* box */
  gtk_box_set_spacing(Zathura.UI.box, 0);
  gtk_container_add(GTK_CONTAINER(Zathura.UI.window), GTK_WIDGET(Zathura.UI.box));

  /* view */
  g_signal_connect(G_OBJECT(Zathura.UI.view), "key-press-event", G_CALLBACK(cb_view_kb_pressed), NULL);

  /* statusbar */
  gtk_widget_modify_bg(GTK_WIDGET(Zathura.UI.statusbar), GTK_STATE_NORMAL, &(Zathura.Style.statusbar_bg));

  Zathura.Global.status_text   = GTK_LABEL(gtk_label_new(NULL));
  Zathura.Global.status_state  = GTK_LABEL(gtk_label_new(NULL));
  Zathura.Global.status_buffer = GTK_LABEL(gtk_label_new(NULL));

  gtk_widget_modify_fg(GTK_WIDGET(Zathura.Global.status_text),  GTK_STATE_NORMAL, &(Zathura.Style.statusbar_fg));
  gtk_widget_modify_fg(GTK_WIDGET(Zathura.Global.status_state), GTK_STATE_NORMAL, &(Zathura.Style.statusbar_fg));
  gtk_widget_modify_fg(GTK_WIDGET(Zathura.Global.status_buffer), GTK_STATE_NORMAL, &(Zathura.Style.statusbar_fg));

  gtk_widget_modify_font(GTK_WIDGET(Zathura.Global.status_text),  Zathura.Style.font);
  gtk_widget_modify_font(GTK_WIDGET(Zathura.Global.status_state), Zathura.Style.font);
  gtk_widget_modify_font(GTK_WIDGET(Zathura.Global.status_buffer), Zathura.Style.font);

  gtk_misc_set_alignment(GTK_MISC(Zathura.Global.status_text),  0.0, 0.0);
  gtk_misc_set_alignment(GTK_MISC(Zathura.Global.status_state), 1.0, 0.0);
  gtk_misc_set_alignment(GTK_MISC(Zathura.Global.status_buffer), 1.0, 0.0);

  gtk_misc_set_padding(GTK_MISC(Zathura.Global.status_text),  2.0, 4.0);
  gtk_misc_set_padding(GTK_MISC(Zathura.Global.status_state), 2.0, 4.0);
  gtk_misc_set_padding(GTK_MISC(Zathura.Global.status_buffer), 2.0, 4.0);

  gtk_label_set_use_markup(Zathura.Global.status_text,  TRUE);
  gtk_label_set_use_markup(Zathura.Global.status_state, TRUE);
  gtk_label_set_use_markup(Zathura.Global.status_buffer, TRUE);

  gtk_box_pack_start(Zathura.UI.statusbar_entries, GTK_WIDGET(Zathura.Global.status_text),  TRUE,  TRUE,  2);
  gtk_box_pack_start(Zathura.UI.statusbar_entries, GTK_WIDGET(Zathura.Global.status_buffer), FALSE, FALSE, 2);
  gtk_box_pack_start(Zathura.UI.statusbar_entries, GTK_WIDGET(Zathura.Global.status_state), FALSE, FALSE, 2);

  gtk_container_add(GTK_CONTAINER(Zathura.UI.statusbar), GTK_WIDGET(Zathura.UI.statusbar_entries));

  /* inputbar */
  gtk_entry_set_inner_border(Zathura.UI.inputbar, NULL);
  gtk_entry_set_has_frame(   Zathura.UI.inputbar, FALSE);
  gtk_editable_set_editable( GTK_EDITABLE(Zathura.UI.inputbar), TRUE);

  gtk_widget_modify_base(GTK_WIDGET(Zathura.UI.inputbar), GTK_STATE_NORMAL, &(Zathura.Style.inputbar_bg));
  gtk_widget_modify_text(GTK_WIDGET(Zathura.UI.inputbar), GTK_STATE_NORMAL, &(Zathura.Style.inputbar_fg));
  gtk_widget_modify_font(GTK_WIDGET(Zathura.UI.inputbar),                     Zathura.Style.font);

  g_signal_connect(G_OBJECT(Zathura.UI.inputbar), "key-press-event", G_CALLBACK(cb_inputbar_kb_pressed), NULL);
  g_signal_connect(G_OBJECT(Zathura.UI.inputbar), "activate",        G_CALLBACK(cb_inputbar_activate),   NULL);

  /* packing */
  gtk_box_pack_start(Zathura.UI.box, GTK_WIDGET(Zathura.UI.view),      TRUE,  TRUE,  0);
  gtk_box_pack_start(Zathura.UI.box, GTK_WIDGET(Zathura.UI.statusbar), FALSE, FALSE, 0);
  gtk_box_pack_end(  Zathura.UI.box, GTK_WIDGET(Zathura.UI.inputbar),  FALSE, FALSE, 0);
}

void
change_mode(int mode)
{
  char* mode_text;

  switch(mode)
  {
    case INSERT:
      mode_text = "-- INSERT --";
      gtk_editable_set_editable( GTK_EDITABLE(Zathura.UI.inputbar), FALSE);
      break;
    case VISUAL:
      mode_text = "-- VISUAL --";
      gtk_editable_set_editable( GTK_EDITABLE(Zathura.UI.inputbar), FALSE);
      break;
    default:
      mode_text = "";
      mode      = NORMAL;
      gtk_editable_set_editable( GTK_EDITABLE(Zathura.UI.inputbar), TRUE);
      break;
  }

  Zathura.Global.mode = mode;
  notify(DEFAULT, mode_text);
}

void notify(int level, char* message)
{
  switch(level)
  {
    case ERROR:
      gtk_widget_modify_base(GTK_WIDGET(Zathura.UI.inputbar), GTK_STATE_NORMAL, &(Zathura.Style.notification_e_bg));
      gtk_widget_modify_text(GTK_WIDGET(Zathura.UI.inputbar), GTK_STATE_NORMAL, &(Zathura.Style.notification_e_fg));
      break;
    case WARNING:
      gtk_widget_modify_base(GTK_WIDGET(Zathura.UI.inputbar), GTK_STATE_NORMAL, &(Zathura.Style.notification_w_bg));
      gtk_widget_modify_text(GTK_WIDGET(Zathura.UI.inputbar), GTK_STATE_NORMAL, &(Zathura.Style.notification_w_fg));
      break;
    default:
      gtk_widget_modify_base(GTK_WIDGET(Zathura.UI.inputbar), GTK_STATE_NORMAL, &(Zathura.Style.inputbar_bg));
      gtk_widget_modify_text(GTK_WIDGET(Zathura.UI.inputbar), GTK_STATE_NORMAL, &(Zathura.Style.inputbar_fg));
      break;
  }
 
  if(message)
    gtk_entry_set_text(Zathura.UI.inputbar, message);
}

void
update_status(char* text, char* state)
{
  /* update text */
  if(text)
    gtk_label_set_markup((GtkLabel*) Zathura.Global.status_text, text);

  /* update state */
  if(state)
    gtk_label_set_markup((GtkLabel*) Zathura.Global.status_state, state);
}

GtkEventBox*
createCompletionRow(GtkBox* results, char* command, char* description, gboolean group)
{
  GtkBox      *col = GTK_BOX(gtk_hbox_new(FALSE, 0));
  GtkEventBox *row = GTK_EVENT_BOX(gtk_event_box_new());

  GtkLabel *show_command     = GTK_LABEL(gtk_label_new(NULL));
  GtkLabel *show_description = GTK_LABEL(gtk_label_new(NULL));

  gtk_misc_set_alignment(GTK_MISC(show_command),     0.0, 0.0);
  gtk_misc_set_alignment(GTK_MISC(show_description), 0.0, 0.0);

  if(group)
  {
    gtk_misc_set_padding(GTK_MISC(show_command),     2.0, 4.0);
    gtk_misc_set_padding(GTK_MISC(show_description), 2.0, 4.0);
  }
  else
  {
    gtk_misc_set_padding(GTK_MISC(show_command),     1.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(show_description), 1.0, 1.0);
  }

  gtk_label_set_use_markup(show_command,     TRUE),
  gtk_label_set_use_markup(show_description, TRUE),

  gtk_label_set_markup(show_command,     g_markup_printf_escaped(FORMAT_COMMAND,     command ? command : ""));
  gtk_label_set_markup(show_description, g_markup_printf_escaped(FORMAT_DESCRIPTION, description ? description : ""));

  if(group)
  {
    gtk_widget_modify_fg(GTK_WIDGET(show_command),     GTK_STATE_NORMAL, &(Zathura.Style.completion_g_fg));
    gtk_widget_modify_fg(GTK_WIDGET(show_description), GTK_STATE_NORMAL, &(Zathura.Style.completion_g_fg));
    gtk_widget_modify_bg(GTK_WIDGET(row),              GTK_STATE_NORMAL, &(Zathura.Style.completion_g_bg));
  }
  else
  {
    gtk_widget_modify_fg(GTK_WIDGET(show_command),     GTK_STATE_NORMAL, &(Zathura.Style.completion_fg));
    gtk_widget_modify_fg(GTK_WIDGET(show_description), GTK_STATE_NORMAL, &(Zathura.Style.completion_fg));
    gtk_widget_modify_bg(GTK_WIDGET(row),              GTK_STATE_NORMAL, &(Zathura.Style.completion_bg));
  }

  gtk_widget_modify_font(GTK_WIDGET(show_command),     Zathura.Style.font);
  gtk_widget_modify_font(GTK_WIDGET(show_description), Zathura.Style.font);

  gtk_box_pack_start(GTK_BOX(col), GTK_WIDGET(show_command),     TRUE,  TRUE,  2);
  gtk_box_pack_start(GTK_BOX(col), GTK_WIDGET(show_description), FALSE, FALSE, 2);

  gtk_container_add(GTK_CONTAINER(row), GTK_WIDGET(col));

  gtk_box_pack_start(results, GTK_WIDGET(row), FALSE, FALSE, 0);

  return row;
}

void
setCompletionRowColor(GtkBox* results, int mode, int id)
{
  GtkEventBox *row   = (GtkEventBox*) g_list_nth_data(gtk_container_get_children(GTK_CONTAINER(results)), id);
 
  if(row)
  {
    GtkBox      *col   = (GtkBox*)      g_list_nth_data(gtk_container_get_children(GTK_CONTAINER(row)), 0);
    GtkLabel    *cmd   = (GtkLabel*)    g_list_nth_data(gtk_container_get_children(GTK_CONTAINER(col)), 0);
    GtkLabel    *cdesc = (GtkLabel*)    g_list_nth_data(gtk_container_get_children(GTK_CONTAINER(col)), 1);

    if(mode == NORMAL)
    {
      gtk_widget_modify_fg(GTK_WIDGET(cmd),   GTK_STATE_NORMAL, &(Zathura.Style.completion_fg));
      gtk_widget_modify_fg(GTK_WIDGET(cdesc), GTK_STATE_NORMAL, &(Zathura.Style.completion_fg));
      gtk_widget_modify_bg(GTK_WIDGET(row),   GTK_STATE_NORMAL, &(Zathura.Style.completion_bg));
    }
    else
    {
      gtk_widget_modify_fg(GTK_WIDGET(cmd),   GTK_STATE_NORMAL, &(Zathura.Style.completion_hl_fg));
      gtk_widget_modify_fg(GTK_WIDGET(cdesc), GTK_STATE_NORMAL, &(Zathura.Style.completion_hl_fg));
      gtk_widget_modify_bg(GTK_WIDGET(row),   GTK_STATE_NORMAL, &(Zathura.Style.completion_hl_bg));
    }
  }
}

/* shortcut implementation */
void
sc_abort(Argument* argument)
{
  /* Clear buffer */
  if(Zathura.Global.buffer)
  {
    g_string_free(Zathura.Global.buffer, TRUE);
    Zathura.Global.buffer = NULL;
    gtk_label_set_markup((GtkLabel*) Zathura.Global.status_buffer, "");
  }

  /* Set back to normal mode */
  change_mode(NORMAL);
}

void
sc_change_mode(Argument* argument)
{
  if(argument)
    change_mode(argument->n);
}

void 
sc_focus_inputbar(Argument* argument)
{
  if(argument->data)
  {
    notify(DEFAULT, argument->data);
    gtk_widget_grab_focus(GTK_WIDGET(Zathura.UI.inputbar));
    gtk_editable_set_position(GTK_EDITABLE(Zathura.UI.inputbar), -1);
  }
}

void
sc_quit(Argument* argument)
{
  cb_destroy(NULL, NULL);
}

/* inputbar shortcut declarations */
void
isc_abort(Argument* argument)
{
  Argument arg = { HIDE };
  isc_completion(&arg);

  notify(DEFAULT, "");
  gtk_widget_grab_focus(GTK_WIDGET(Zathura.UI.view));
}

void
isc_command_history(Argument* argument)
{
  static int current = 0;
  int        length  = g_list_length(Zathura.Global.history);

  if(length > 0)
  {
    if(argument->n == NEXT)
      current = (length + current + 1) % length;
    else
      current = (length + current - 1) % length;

    gchar* command = (gchar*) g_list_nth_data(Zathura.Global.history, current);
    notify(DEFAULT, command);
    gtk_widget_grab_focus(GTK_WIDGET(Zathura.UI.inputbar));
    gtk_editable_set_position(GTK_EDITABLE(Zathura.UI.inputbar), -1);
  }
}

void 
isc_completion(Argument* argument)
{
  gchar *input      = gtk_editable_get_chars(GTK_EDITABLE(Zathura.UI.inputbar), 1, -1);
  gchar  identifier = gtk_editable_get_chars(GTK_EDITABLE(Zathura.UI.inputbar), 0,  1)[0];
  int    length     = strlen(input);
 
  if(!length && !identifier)
    return;

  /* get current information*/
  char* first_space = strstr(input, " ");
  char* current_command;
  char* current_parameter;
  int   current_command_length;
  int   current_parameter_length;

  if(!first_space)
  {
    current_command          = input;
    current_command_length   = length;
    current_parameter        = NULL;
    current_parameter_length = 0;
  }
  else
  {
    int offset               = abs(input - first_space);
    current_command          = g_strndup(input, offset);
    current_command_length   = strlen(current_command);
    current_parameter        = input + offset + 1;
    current_parameter_length = strlen(current_parameter);
  }

  /* if the identifier does not match the command sign and
   * the completion should not be hidden, leave this function */
  if((identifier != ':') && (argument->n != HIDE))
    return;

  /* static elements */
  static GtkBox        *results = NULL;
  static CompletionRow *rows    = NULL;

  static int current_item = 0;
  static int n_items      = 0;

  static char *previous_command   = NULL;
  static char *previous_parameter = NULL;
  static int   previous_id        = 0;
  static int   previous_length    = 0;

  static gboolean command_mode = TRUE;

  /* delete old list iff
   *   the completion should be hidden
   *   the current command differs from the previous one
   *   the current parameter differs from the previous one
   */
  if( (argument->n == HIDE) ||
      (current_parameter && previous_parameter && strcmp(current_parameter, previous_parameter)) ||
      (current_command && previous_command && strcmp(current_command, previous_command)) ||
      (previous_length != length)
    )
  {
    if(results)
      gtk_widget_destroy(GTK_WIDGET(results));

    results = NULL;

    if(rows)
      free(rows);

    rows         = NULL;
    current_item = 0;
    n_items      = 0;
    command_mode = TRUE;

    if(argument->n == HIDE)
      return;
  }

  /* create new list iff
   *  there is no current list
   *  the current command differs from the previous one
   *  the current parameter differs from the previous one
   */
  if( (!results) )
  {
    results = GTK_BOX(gtk_vbox_new(FALSE, 0));

    /* create list based on parameters iff
     *  there is a current parameter given
     *  there is an old list with commands
     *  the current command does not differ from the previous one
     *  the current command has an completion function
     */
    if( (previous_command) && (current_parameter) && !strcmp(current_command, previous_command) )
    {
      if(previous_id < 0 || !commands[previous_id].completion)
        return;

      Completion *result = commands[previous_id].completion(current_parameter);

      if(!result && result->groups)
        return;

      command_mode = FALSE;
      CompletionGroup* group;
      CompletionElement* element;

      rows = malloc(sizeof(CompletionRow));

      for(group = result->groups; group != NULL; group = group->next)
      {
        int group_elements = 0;

        for(element = group->elements; element != NULL; element = element->next)
        {
          if( (element->value) &&
              (current_parameter_length <= strlen(element->value)) && !strncmp(current_parameter, element->value, current_parameter_length)
            )
          {
            rows = realloc(rows, (n_items + 1) * sizeof(CompletionRow));
            rows[n_items].command     = element->value;
            rows[n_items].description = element->description;
            rows[n_items].command_id  = previous_id;
            rows[n_items].is_group    = FALSE;
            rows[n_items++].row       = GTK_WIDGET(createCompletionRow(results, element->value, element->description, FALSE));
            group_elements++;
          }
        }

        if(group->value && group_elements > 0)
        {
          rows = realloc(rows, (n_items + 1) * sizeof(CompletionRow));
          rows[n_items].command     = group->value;
          rows[n_items].description = NULL;
          rows[n_items].command_id  = -1;
          rows[n_items].is_group    = TRUE;
          rows[n_items].row       = GTK_WIDGET(createCompletionRow(results, group->value, NULL, TRUE));

          /* Swap group element with first element of the list */
          CompletionRow temp = rows[n_items - group_elements];
          gtk_box_reorder_child(results, rows[n_items].row, n_items - group_elements);
          rows[n_items - group_elements] = rows[n_items];
          rows[n_items] = temp;

          n_items++;
        }
      }

      /* clean up */
      group = result->groups;

      while(group)
      {
        element = group->elements;
        while(element)
        {
          CompletionElement* ne = element->next;
          free(element);
          element = ne;
        }

        CompletionGroup *ng = group->next;
        free(group);
        group = ng;
      }
    }
    /* create list based on commands */
    else
    {
      int i = 0;
      command_mode = TRUE;

      rows = malloc(LENGTH(commands) * sizeof(CompletionRow));

      for(i = 0; i < LENGTH(commands); i++)
      {
        int abbr_length = strlen(commands[i].abbr);
        int cmd_length  = strlen(commands[i].command);

        /* add command to list iff
         *  the current command would match the command
         *  the current command would match the abbreviation
         */
        if( ((current_command_length <= cmd_length)  && !strncmp(current_command, commands[i].command, current_command_length)) ||
            ((current_command_length <= abbr_length) && !strncmp(current_command, commands[i].abbr,    current_command_length))
          )
        {
          rows[n_items].command     = commands[i].command;
          rows[n_items].description = commands[i].description;
          rows[n_items].command_id  = i;
          rows[n_items].is_group    = FALSE;
          rows[n_items++].row       = GTK_WIDGET(createCompletionRow(results, commands[i].command, commands[i].description, FALSE));
        }
      }

      rows = realloc(rows, n_items * sizeof(CompletionRow));
    }

    gtk_box_pack_start(Zathura.UI.box, GTK_WIDGET(results), FALSE, FALSE, 0);
    gtk_widget_show_all(GTK_WIDGET(Zathura.UI.window));

    current_item = (argument->n == NEXT) ? -1 : 0;
  }

  /* update coloring iff
   *  there is a list with items
   */
  if( (results) && (n_items > 0) )
  {
    setCompletionRowColor(results, NORMAL, current_item);
    char* temp;
    int i = 0, next_group = 0;

    for(i = 0; i < n_items; i++)
    {
      if(argument->n == NEXT || argument->n == NEXT_GROUP)
        current_item = (current_item + n_items + 1) % n_items;
      else if(argument->n == PREVIOUS || argument->n == PREVIOUS_GROUP)
        current_item = (current_item + n_items - 1) % n_items;

      if(rows[current_item].is_group)
      {
        if(!command_mode && (argument->n == NEXT_GROUP || argument->n == PREVIOUS_GROUP))
          next_group = 1;
        continue;
      }
      else
      {
        if(!command_mode && (next_group == 0) && (argument->n == NEXT_GROUP || argument->n == PREVIOUS_GROUP))
          continue;
        break;
      }
    }

    setCompletionRowColor(results, HIGHLIGHT, current_item);

    if(command_mode)
    {
      char* cp = (current_parameter) ? g_strconcat(" ", current_parameter, NULL) : 0;
      temp = g_strconcat(":", rows[current_item].command, cp, NULL);
    }
    else
    {
      temp = g_strconcat(":", previous_command, " ", rows[current_item].command, NULL);
    }

    gtk_entry_set_text(Zathura.UI.inputbar, temp);
    gtk_editable_set_position(GTK_EDITABLE(Zathura.UI.inputbar), -1);
    g_free(temp);

    previous_command   = (command_mode) ? rows[current_item].command : current_command;
    previous_parameter = (command_mode) ? current_parameter : rows[current_item].command;
    previous_length    = strlen(previous_command) + ((command_mode) ? (length - current_command_length) : (strlen(previous_parameter) + 1));
    previous_id        = rows[current_item].command_id;
  }
}

void
isc_string_manipulation(Argument* argument)
{
  if(argument->n == DELETE_LAST_WORD)
  {
    gchar *input  = gtk_editable_get_chars(GTK_EDITABLE(Zathura.UI.inputbar), 0, -1);
    int    length = strlen(input);
    int    i      = 0;

    for(i = length; i > 0; i--)
    {
      if( (input[i] == ' ') ||
          (input[i] == '/') )
      {
        if(i == (length - 1))
          continue;

        i = (input[i] == ' ') ? (i - 1) : i;
        break;
      }
    }

    notify(DEFAULT, g_strndup(input, i + 1));
    gtk_editable_set_position(GTK_EDITABLE(Zathura.UI.inputbar), -1);
  }
}

/* command implementation */
void
cmd_quit(int argc, char** argv)
{
  cb_destroy(NULL, NULL);
}

/* completion command implementation */
Completion* cc_open(char* input)
{
  Completion *completion = malloc(sizeof(Completion));
  CompletionGroup* group = malloc(sizeof(CompletionGroup));
  CompletionElement *el1 = malloc(sizeof(CompletionElement));
  CompletionElement *el2 = malloc(sizeof(CompletionElement));
  CompletionElement *el3 = malloc(sizeof(CompletionElement));
  CompletionElement *el4 = malloc(sizeof(CompletionElement));
  CompletionElement *el5 = malloc(sizeof(CompletionElement));
  CompletionElement *el6 = malloc(sizeof(CompletionElement));
  CompletionGroup* g2 = malloc(sizeof(CompletionGroup));

  el1->value = "Word 1";
  el1->description = NULL;
  el1->next = (CompletionElement*) el2;
  el2->value = "Word 2";
  el2->description = NULL;
  el2->next = (CompletionElement*) el3;
  el3->value = "Word 3";
  el3->description = NULL;
  el3->next = NULL;

  el4->value = "Word 4";
  el4->description = NULL;
  el4->next = (CompletionElement*) el5;
  el5->value = "Word 5";
  el5->description = NULL;
  el5->next = (CompletionElement*) el6;
  el6->value = "Word 6";
  el6->description = NULL;
  el6->next = NULL;

  group->value = "Group 1";
  group->elements = el1;
  group->next = g2;

  g2->value = "Group 2";
  g2->elements = el4;
  g2->next = NULL;

  completion->groups = group;

  return completion;
}

/* buffer command implementation */
void 
bcmd_goto(char* buffer, Argument* argument)
{

}

/* special command implementation */
void
scmd_search(char* input, Argument* argument)
{

}

/* callback implementation */
gboolean
cb_destroy(GtkWidget* widget, gpointer data)
{
  pango_font_description_free(Zathura.Style.font);
  gtk_main_quit();

  return TRUE;
}

gboolean
cb_view_kb_pressed(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  int i;
  for(i = 0; i < LENGTH(shortcuts); i++)
  {
    if (event->keyval == shortcuts[i].key && 
      (((event->state & shortcuts[i].mask) == shortcuts[i].mask) || shortcuts[i].mask == 0)
      && (Zathura.Global.mode == shortcuts[i].mode || shortcuts[i].mode == -1))
    {
      shortcuts[i].function(&(shortcuts[i].argument));
      return TRUE;
    }
  }

  /* append only numbers and characters to buffer */
  if( (event->keyval >= 0x30) && (event->keyval <= 0x7A))
  {
    if(!Zathura.Global.buffer)
      Zathura.Global.buffer = g_string_new("");

    Zathura.Global.buffer = g_string_append_c(Zathura.Global.buffer, event->keyval);
    gtk_label_set_markup((GtkLabel*) Zathura.Global.status_buffer, Zathura.Global.buffer->str);
  }

  /* search buffer commands */
  if(Zathura.Global.buffer)
  {
    for(i = 0; i < LENGTH(buffer_commands); i++)
    {
      regex_t regex;
      int     status;

      regcomp(&regex, buffer_commands[i].regex, REG_EXTENDED);
      status = regexec(&regex, Zathura.Global.buffer->str, (size_t) 0, NULL, 0);
      regfree(&regex);

      if(status == 0)
      {
        buffer_commands[i].function(Zathura.Global.buffer->str, &(buffer_commands[i].argument));
        g_string_free(Zathura.Global.buffer, TRUE);
        Zathura.Global.buffer = NULL;
        gtk_label_set_markup((GtkLabel*) Zathura.Global.status_buffer, "");

        return TRUE;
      }
    }
  }

  return FALSE;
}

gboolean
cb_inputbar_kb_pressed(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  int i;

  /* inputbar shortcuts */
  for(i = 0; i < LENGTH(inputbar_shortcuts); i++)
  {
    if(event->keyval == inputbar_shortcuts[i].key &&
      (((event->state & inputbar_shortcuts[i].mask) == inputbar_shortcuts[i].mask)
       || inputbar_shortcuts[i].mask == 0))
    {
      inputbar_shortcuts[i].function(&(inputbar_shortcuts[i].argument));
      return TRUE;
    }
  }

  /* special commands */
  char identifier = gtk_editable_get_chars(GTK_EDITABLE(Zathura.UI.inputbar), 0, 1)[0];
  for(i = 0; i < LENGTH(special_commands); i++)
  {
    if((identifier == special_commands[i].identifier) &&
       (special_commands[i].always == 1))
    {
      gchar *input  = gtk_editable_get_chars(GTK_EDITABLE(Zathura.UI.inputbar), 1, -1);
      special_commands[i].function(input, &(special_commands[i].argument));
      return FALSE;
    }
  }

  return FALSE;
}

gboolean
cb_inputbar_activate(GtkEntry* entry, gpointer data)
{
  gchar  *input   = gtk_editable_get_chars(GTK_EDITABLE(entry), 1, -1);
  gchar **tokens  = g_strsplit(input, " ", -1);
  gchar  *command = tokens[0];
  int     length  = g_strv_length(tokens);
  int          i  = 0;

  /* no input */
  if(length < 1)
  {
    isc_abort(NULL);
    return FALSE;
  }

  /* special commands */
  char identifier = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, 1)[0];
  for(i = 0; i < LENGTH(special_commands); i++)
  {
    if(identifier == special_commands[i].identifier)
    {
      special_commands[i].function(input, &(special_commands[i].argument));
      isc_abort(NULL);
      return TRUE;
    }
  }

  /* search commands */
  for(i = 0; i < LENGTH(commands); i++)
  {
    if((g_strcmp0(command, commands[i].command) == 0) ||
       (g_strcmp0(command, commands[i].abbr)    == 0))
    {
      commands[i].function(length - 1, tokens + 1);
      break;
    }
  }

  /* append input to the command history */
  Zathura.Global.history = g_list_append(Zathura.Global.history, g_strdup(gtk_entry_get_text(entry)));

  isc_abort(NULL);

  return TRUE;
}

/* main function */
int main(int argc, char* argv[])
{
  gtk_init(&argc, &argv);

  init_zathura();
  update_status("Text", "State");

  gtk_widget_show_all(GTK_WIDGET(Zathura.UI.window));
  gtk_main();

  return 0;
}
