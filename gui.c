#include "gui.h"
#include "master_pass_app.h"
#include "encode.h"
#include "cleanse.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

/* place = NULL is used as an internal namespace */
#define RESERVED_PLACE NULL
#define RESERVED_PLACE_SIZE 0
#define DIR_BASE ".master_pass"
#define DB_PATH_SIZE (12 + 1 + CRYPTO_KEY_SIZE * 2 + 1)

/* shared across every state */
#define GTK_APP(g) g->data[0]
#define WINDOW(g) g->data[1]
#define BOX(g) g->data[2]
#define GRID(g) g->data[3]

/* unique to welcome state */
#define PASS_ENTRY(g) g->data[4]
#define PASS_CONF_ENTRY(g) g->data[5]
#define OPEN_SAFE_BUTTON(g) g->data[6]
#define CREATE_SAFE_BUTTON(g) g->data[7]

/* unique to working state */
#define PLACE_ENTRY(g) g->data[4]
#define LOGIN_ENTRY(g) g->data[5]
#define GO_BUTTON(g) g->data[6]
#define NEXT_BUTTON(g) g->data[7]
#define FIRST_BUTTON(g) g->data[8]
#define HEX_BUTTON(g) g->data[9]
#define BASE64_BUTTON(g) g->data[10]
#define CLOSE_BUTTON(g) g->data[11]

/* macros to make things less painful */
#define ATTACH(gui,widget,x,y,w,h) \
  gtk_grid_attach(GTK_GRID(GRID(gui)), widget, x, y, w, h)

#define FATAL {\
  fprintf(stderr, "CRITICAL ERROR: %s at line %d\n", __FILE__, __LINE__);\
  exit(EXIT_FAILURE);\
}

typedef enum {
  PASS_OP_GET, PASS_OP_RESET, PASS_OP_ADD
} EPassOp;

typedef struct {
  char const* place;
  size_t place_size;
  char const* login;
  size_t login_size;
} WorkingInput;

typedef struct {
  char const* pass;
  size_t pass_size;
  char const* conf;
  size_t conf_size;
} WelcomeInput;

/* declarations */
static void show_welcome(GUI* gui);

/* functions */
/* return dir, create it if doesn't exist */
static char* create_path(unsigned char const* hash)
{
  char* path;
  char* slash;
  char const* home_dir;
  size_t home_dir_len;
  struct stat st;

  home_dir = getenv("HOME");
  if(!home_dir)
    goto err;

  home_dir_len = strlen(home_dir);

  path = malloc(home_dir_len + 1 + DB_PATH_SIZE);
  if(!path)
    goto err;

  memcpy(path, home_dir, home_dir_len);
  path[home_dir_len] = '/';
  memcpy(path + home_dir_len + 1, DIR_BASE, sizeof DIR_BASE);

  slash = path + home_dir_len + 1 + sizeof DIR_BASE - 1;
  *slash = '\0';

  if(-1 == stat(path, &st) && -1 == mkdir(path, 0700))
    goto err1;

  *slash = '/';
  bin_to_hex(slash + 1, hash, CRYPTO_KEY_SIZE);
  return path;

err1:
  free(path);
err:
  return NULL;
}

static void error_popup(GUI* gui, char const* msg)
{
  GtkWidget* popup = gtk_message_dialog_new(WINDOW(gui), GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                            "Error: %s", msg);
  gtk_dialog_run(GTK_DIALOG(popup));
  gtk_widget_destroy(popup);
}

static void set_clipboard(void const* data, size_t size)
{
  gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
                         data, size);
}

static void add_box_and_grid(GUI* gui)
{
  BOX(gui) = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_top(BOX(gui), 8);
  gtk_widget_set_margin_bottom(BOX(gui), 8);
  gtk_widget_set_margin_start(BOX(gui), 8);
  gtk_widget_set_margin_end(BOX(gui), 8);
  gtk_container_add(GTK_CONTAINER(WINDOW(gui)), BOX(gui));

  GRID(gui) = gtk_grid_new();
  gtk_grid_set_column_homogeneous(GTK_GRID(GRID(gui)), TRUE);
  gtk_grid_set_row_homogeneous(GTK_GRID(GRID(gui)), TRUE);
  gtk_grid_set_column_spacing(GTK_GRID(GRID(gui)), 4);
  gtk_grid_set_row_spacing(GTK_GRID(GRID(gui)), 4);
  gtk_box_pack_start(GTK_BOX(BOX(gui)), GRID(gui), TRUE, TRUE, 0);
}

/* working screen generates passwords and allows to change them */
static void destroy_working(GUI* gui)
{
  MasterPassApp* app = (MasterPassApp*)gui;

  if(!storage_save(&app->storage))
    FATAL;

  storage_destroy(&app->storage);
  storage_init(&app->storage);

  crypto_reset_password(&app->crypto);
  gtk_widget_destroy(BOX(gui));

  /* clean up passwords */
  cleanse(gui->hex, 65);
  cleanse(gui->base64, 45);
}

static void need_place_and_login(GUI* gui)
{
  gtk_widget_set_sensitive(GO_BUTTON(gui), FALSE);
  gtk_widget_set_sensitive(HEX_BUTTON(gui), FALSE);
  gtk_widget_set_sensitive(BASE64_BUTTON(gui), FALSE);
  gtk_widget_set_sensitive(NEXT_BUTTON(gui), FALSE);
  gtk_widget_set_sensitive(FIRST_BUTTON(gui), FALSE);
}

static void has_place_and_login(GUI* gui)
{
  gtk_widget_set_sensitive(GO_BUTTON(gui), TRUE);
}

static void activate_copy(GUI* gui)
{
  gtk_widget_set_sensitive(GO_BUTTON(gui), FALSE);
  gtk_widget_set_sensitive(HEX_BUTTON(gui), TRUE);
  gtk_widget_set_sensitive(BASE64_BUTTON(gui), TRUE);
  gtk_widget_set_sensitive(NEXT_BUTTON(gui), TRUE);
  gtk_widget_set_sensitive(FIRST_BUTTON(gui), !gui->is_first);
}

static void get_working_input(WorkingInput* wi, GUI* gui)
{
  wi->place = gtk_entry_get_text(GTK_ENTRY(PLACE_ENTRY(gui)));
  wi->place_size = strlen(wi->place);
  wi->login = gtk_entry_get_text(GTK_ENTRY(LOGIN_ENTRY(gui)));
  wi->login_size = strlen(wi->login);
}

static void check_inputs(GtkEditable* editable, gpointer data)
{
  GUI* gui = data;
  WorkingInput wi;

  need_place_and_login(gui);
  get_working_input(&wi, gui);

  if(wi.login_size && wi.place_size)
    has_place_and_login(gui);
}

static void manage_password(EPassOp op, __uint128_t* value, MasterPassApp* app,
                            WorkingInput* wi)
{
  unsigned char key[32]; /* only first 16 bytes are used as key */
  __uint128_t stored_value;

  crypto_generate_public_hash(&app->crypto, key, wi->place, wi->place_size,
                                                 wi->login, wi->login_size);

  if(!storage_find(&app->storage, key, (unsigned char*)&stored_value))
    stored_value = 0;

  switch(op){
  case PASS_OP_RESET:
    stored_value = 0;
    storage_remove(&app->storage, key);
    break;

  case PASS_OP_ADD:
    stored_value += *value;
    if(!storage_insert(&app->storage, key, (unsigned char const*)&stored_value))
      FATAL;
  }

  *value = stored_value;
}

static void derive_password(MasterPassApp* app, WorkingInput* wi,
                            void const* version)
{
  crypto_generate_password(&app->crypto, app->gui.base64, app->gui.hex,
                           wi->place, wi->place_size, wi->login, wi->login_size,
                           version);
}

static void go(GtkButton* button, gpointer data)
{
  MasterPassApp* app = data;
  GUI* gui = data;
  WorkingInput wi;
  __uint128_t version;

  get_working_input(&wi, gui);
  manage_password(PASS_OP_GET, &version, app, &wi);
  derive_password(app, &wi, &version);

  gui->is_first = 0 == version;
  activate_copy(gui);
}

static void hex_copy(GtkButton* button, gpointer data)
{
  GUI* gui = data;
  set_clipboard(gui->hex, 64);
}

static void base64_copy(GtkButton* button, gpointer data)
{
  GUI* gui = data;
  set_clipboard(gui->base64, 44);
}

static void next_password(GtkButton* button, gpointer data)
{
  MasterPassApp* app = data;
  GUI* gui = data;
  WorkingInput wi;
  __uint128_t version = 1; /* increment by 1 */

  get_working_input(&wi, gui);
  manage_password(PASS_OP_ADD, &version, app, &wi);
  derive_password(app, &wi, &version);

  gui->is_first = FALSE;
  activate_copy(gui);
}

static void first_password_again(GtkButton* button, gpointer data)
{
  MasterPassApp* app = data;
  GUI* gui = data;
  WorkingInput wi;
  __uint128_t version = 0; /* reset password */

  get_working_input(&wi, gui);
  manage_password(PASS_OP_RESET, &version, app, &wi);
  derive_password(app, &wi, &version);

  gui->is_first = TRUE;
  activate_copy(gui);
}

static void build_working_layout(GUI* gui)
{
  add_box_and_grid(gui);

  PLACE_ENTRY(gui) = gtk_entry_new();
  ATTACH(gui, PLACE_ENTRY(gui), 0, 0, 6, 1);

  LOGIN_ENTRY(gui) = gtk_entry_new();
  ATTACH(gui, LOGIN_ENTRY(gui), 0, 1, 6, 1);

  GO_BUTTON(gui) = gtk_button_new_with_label("GO!");
  ATTACH(gui, GO_BUTTON(gui), 6, 0, 2, 2);

  HEX_BUTTON(gui) = gtk_button_new_with_label("hex copy");
  ATTACH(gui, HEX_BUTTON(gui), 0, 2, 6, 1);

  NEXT_BUTTON(gui) = gtk_button_new_with_label("next");
  ATTACH(gui, NEXT_BUTTON(gui), 6, 2, 2, 1);

  BASE64_BUTTON(gui) = gtk_button_new_with_label("base64 copy");
  ATTACH(gui, BASE64_BUTTON(gui), 0, 3, 6, 1);

  FIRST_BUTTON(gui) = gtk_button_new_with_label("first");
  ATTACH(gui, FIRST_BUTTON(gui), 6, 3, 2, 1);

  CLOSE_BUTTON(gui) = gtk_button_new_with_label("close safe");
  ATTACH(gui, CLOSE_BUTTON(gui), 0, 4, 8, 1);
}

static void attach_working_handlers(GUI* gui)
{
  g_signal_connect(PLACE_ENTRY(gui), "changed", G_CALLBACK(check_inputs), gui);
  g_signal_connect(LOGIN_ENTRY(gui), "changed", G_CALLBACK(check_inputs), gui);
  g_signal_connect(GO_BUTTON(gui), "clicked", G_CALLBACK(go), gui);
  g_signal_connect(HEX_BUTTON(gui), "clicked", G_CALLBACK(hex_copy), gui);
  g_signal_connect(NEXT_BUTTON(gui), "clicked", G_CALLBACK(next_password), gui);
  g_signal_connect(BASE64_BUTTON(gui), "clicked", G_CALLBACK(base64_copy), gui);
  g_signal_connect(FIRST_BUTTON(gui), "clicked",
                   G_CALLBACK(first_password_again), gui);
  g_signal_connect_swapped(CLOSE_BUTTON(gui), "clicked",
                           G_CALLBACK(show_welcome), gui);
}

static void show_working(GUI* gui)
{
  gui->destroyer(gui);

  build_working_layout(gui);
  attach_working_handlers(gui);

  /* gray out buttons */
  need_place_and_login(gui);

  gui->destroyer = destroy_working;
  gtk_widget_show_all(WINDOW(gui));
}

/* welcome screen takes passwords and creates new safe or opens existing one */
static void get_welcome_input(WelcomeInput* wi, GUI* gui)
{
  wi->pass = gtk_entry_get_text(GTK_ENTRY(PASS_ENTRY(gui)));
  wi->pass_size = strlen(wi->pass);
  wi->conf = gtk_entry_get_text(GTK_ENTRY(PASS_CONF_ENTRY(gui)));
  wi->conf_size = strlen(wi->conf);
}

static gboolean open_or_create_safe_or_die(gpointer data, gboolean create)
{
  MasterPassApp* app = data;
  GUI* gui = data;
  WelcomeInput wi;
  char* path;
  unsigned char db_name[CRYPTO_KEY_SIZE];
  struct stat st;
  int exists;

  get_welcome_input(&wi, gui);

  crypto_set_password(&app->crypto, wi.pass, wi.pass_size);
  crypto_generate_public_hash(&app->crypto, db_name, RESERVED_PLACE,
                              RESERVED_PLACE_SIZE, "file_name", 9);

  path = create_path(db_name);
  if(!path)
    FATAL;

  exists = -1 != stat(path, &st);

  /* create but exists */
  if(create){
    if(exists)
      goto err;
  }

  /* open but doesn't exist */
  else
  if(!exists)
    goto err;

  if(!storage_load(&app->storage, path))
    FATAL;

  free(path);

  return TRUE;

err:
  crypto_reset_password(&app->crypto);
  free(path);
  return FALSE;
}

static gboolean open_safe(GtkWidget* widget, gpointer data)
{
  if(!open_or_create_safe_or_die(data, FALSE))
    error_popup(data, "Safe doesn't exist!");

  else
    show_working(data);

  return TRUE;
}

static gboolean create_safe(GtkWidget* widget, gpointer data)
{
  if(!open_or_create_safe_or_die(data, TRUE))
    error_popup(data, "Safe exists!");

  else
    show_working(data);

  return TRUE;
}

static void create_or_open_by_enter(GtkEntry* widget, gpointer data)
{
  GUI* gui = data;

  if(gtk_widget_get_sensitive(OPEN_SAFE_BUTTON(gui)))
    open_safe(NULL, data);

  else
  if(gtk_widget_get_sensitive(CREATE_SAFE_BUTTON(gui)))
    create_safe(NULL, data);
}

static void pass_input(GtkEditable* editable, gpointer data)
{
  GUI* gui = data;
  WelcomeInput wi;

  get_welcome_input(&wi, gui);

  if(wi.conf_size){
    gtk_widget_set_sensitive(OPEN_SAFE_BUTTON(gui), FALSE);
    gtk_widget_set_sensitive(CREATE_SAFE_BUTTON(gui),
                             0 == strcmp(wi.pass, wi.conf));
  }
  else {
    gtk_widget_set_sensitive(CREATE_SAFE_BUTTON(gui), FALSE);
    gtk_widget_set_sensitive(OPEN_SAFE_BUTTON(gui), wi.pass_size);
  }
}

static void init_welcome_buttons(GUI* gui)
{
  pass_input(NULL, gui);
}

static void destroy_welcome(GUI* gui)
{
  gtk_widget_destroy(BOX(gui));
}

static void build_welcome_layout(GUI* gui)
{
  gtk_window_resize(GTK_WINDOW(WINDOW(gui)), 480, 1);

  add_box_and_grid(gui);

  PASS_ENTRY(gui) = gtk_entry_new();
  gtk_entry_set_input_purpose(GTK_ENTRY(PASS_ENTRY(gui)),
                              GTK_INPUT_PURPOSE_PASSWORD);
  gtk_entry_set_visibility(GTK_ENTRY(PASS_ENTRY(gui)), FALSE);
  ATTACH(gui, PASS_ENTRY(gui), 0, 0, 2, 1);

  OPEN_SAFE_BUTTON(gui) = gtk_button_new_with_label("Open Safe");
  ATTACH(gui, OPEN_SAFE_BUTTON(gui), 2, 0, 1, 1);

  PASS_CONF_ENTRY(gui) = gtk_entry_new();
  gtk_entry_set_input_purpose(GTK_ENTRY(PASS_CONF_ENTRY(gui)),
                              GTK_INPUT_PURPOSE_PASSWORD);
  gtk_entry_set_visibility(GTK_ENTRY(PASS_CONF_ENTRY(gui)), FALSE);
  ATTACH(gui, PASS_CONF_ENTRY(gui), 0, 1, 2, 1);

  CREATE_SAFE_BUTTON(gui) = gtk_button_new_with_label("New Safe");
  ATTACH(gui, CREATE_SAFE_BUTTON(gui), 2, 1, 1, 1);
}

static void attach_welcome_handlers(GUI* gui)
{
  g_signal_connect(PASS_ENTRY(gui), "changed", G_CALLBACK(pass_input), gui);
  g_signal_connect(PASS_ENTRY(gui), "activate",
                   G_CALLBACK(create_or_open_by_enter), gui);
  g_signal_connect(PASS_CONF_ENTRY(gui), "changed",
                   G_CALLBACK(pass_input), gui);
  g_signal_connect(PASS_CONF_ENTRY(gui), "activate",
                   G_CALLBACK(create_or_open_by_enter), gui);
  g_signal_connect(OPEN_SAFE_BUTTON(gui), "clicked", G_CALLBACK(open_safe),
                   gui);
  g_signal_connect(CREATE_SAFE_BUTTON(gui), "clicked", G_CALLBACK(create_safe),
                   gui);
}

static void show_welcome(GUI* gui)
{
  gui->destroyer(gui);

  build_welcome_layout(gui);
  attach_welcome_handlers(gui);
  init_welcome_buttons(gui);

  gui->destroyer = destroy_welcome;
  gtk_widget_show_all(WINDOW(gui));
}

static void do_nothing(GUI* gui)
{
}

static gboolean on_delete(GtkWidget* widget, GdkEvent* event, gpointer data)
{
  GUI* gui = data;
  gui->destroyer(gui); /* this might clean up sensitive data from memory */
  return FALSE; /* let the default handler run */
}

static void activate(GtkApplication* gtk_app, gpointer app_ptr)
{
  MasterPassApp* app = app_ptr;
  GUI* gui = &app->gui;

  WINDOW(gui) = gtk_application_window_new(gtk_app);
  g_signal_connect(WINDOW(gui), "delete-event", G_CALLBACK(on_delete), gui);

  gtk_window_set_title(GTK_WINDOW(WINDOW(gui)), "Master Pass");
  gtk_window_set_default_size(GTK_WINDOW(WINDOW(gui)), 480, 0);
  show_welcome(gui);
}

/* Public interfaces */
void gui_init(GUI* gui)
{
  GTK_APP(gui) = gtk_application_new("master-pass.com.wordpress.artfulcode",
                                     G_APPLICATION_FLAGS_NONE);
  WINDOW(gui) = NULL;
  gui->destroyer = do_nothing;

  g_signal_connect(G_APPLICATION(GTK_APP(gui)), "activate",
                   G_CALLBACK(activate), gui);
}

void gui_destroy(GUI* gui)
{
  g_object_unref(GTK_APP(gui));
}

GtkApplication* gui_get_gtk_app(GUI* gui)
{
  return GTK_APP(gui);
}
