#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <glib.h>

typedef struct
{
	char *datadir;
	char *workdir;
	char *switchuser;
} amalgamfs_config;

extern struct fuse_operations amalgamfs_operations;
extern struct fuse_opt global_amalgamfs_opts[];
int amalgamfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs);

amalgamfs_config *amalgamfs_config_get();
