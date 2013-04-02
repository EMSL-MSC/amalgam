/*
  Amalgamfs: Amalgam posix filesystem
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2009-2011  Kevin Fox <Kevin.Fox@pnl.gov>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB.
*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <switchuser.h>
#include "amalgamfs.h"

int main(int argc, char *argv[])
{
	int ret;
	amalgamfs_config *config = amalgamfs_config_get();
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	g_thread_init(NULL);
	d_init_buhfs("amalgamfs", "/etc/amalgam/amalgamfs-debug.ini", SIGUSR1);
	fuse_opt_parse(&args, config, global_amalgamfs_opts, amalgamfs_opt_proc);
	if(config->datadir)
	{
		ret = chdir(config->datadir);
		if(ret != 0)
		{
			fprintf(stderr, "Failed to switch to directory %s\n", config->datadir);
			return -1;
		}
	}
	else
	{
		chdir("/");
	}
	if(config->switchuser)
	{
		switchuser(config->switchuser);
	}
	ret = fuse_main(args.argc, args.argv, &amalgamfs_operations, NULL);
	return ret;
}
