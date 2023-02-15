#ifndef MASTER_PASS_APP_H
#define MASTER_PASS_APP_H

#include "gui.h"
#include "crypto.h"
#include "storage.h"

typedef struct MasterPassApp {
  GUI gui;
  Crypto crypto;
  Storage storage;
} MasterPassApp;

void master_pass_app_init(MasterPassApp *app);

void master_pass_app_destroy(MasterPassApp *app);

int master_pass_app_run(MasterPassApp *app, int argc, char *argv[]);

#endif
