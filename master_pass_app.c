#include "master_pass_app.h"

void master_pass_app_init(MasterPassApp* app)
{
  gui_init(&app->gui);
  crypto_init(&app->crypto);
  storage_init(&app->storage);
}

void master_pass_app_destroy(MasterPassApp* app)
{
  gui_destroy(&app->gui);
  crypto_destroy(&app->crypto);

  storage_save(&app->storage);
  storage_destroy(&app->storage);
}

int master_pass_app_run(MasterPassApp* app, int argc, char* argv[])
{
  return g_application_run(G_APPLICATION(gui_get_gtk_app(&app->gui)), argc,
                           argv);
}
