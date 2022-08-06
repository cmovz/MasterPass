#include "asm.h"
#include "master_pass_app.h"
#include "cleanse.h"
#include <sys/mman.h>
#include <stdlib.h>

#define MLOCK_SIZE (0x1000 * 16)

int main(int argc, char* argv[])
{
  MasterPassApp app;
  char* mlock_addr = (char*)&app + sizeof app - MLOCK_SIZE;
  int status;

  /* lock the stack here */
  if(-1 == mlock(mlock_addr, MLOCK_SIZE)){
    perror("mlock()");
    return EXIT_FAILURE;
  }

  master_pass_app_init(&app);
  status = master_pass_app_run(&app, argc, argv);
  master_pass_app_destroy(&app);

  /* clean up stack */
  CleanseStack(16);

  munlock(mlock_addr, MLOCK_SIZE);
  return status;
}
