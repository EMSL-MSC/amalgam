#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include "tar.h"

#define BLOCKSIZE 1024

int main(int argc, char *argv[])
{
	char **args = NULL;
	int count = 0;
	ssize_t size;
	char *lineptr = NULL;
	size_t buffsize = 0;
	int res;
	gboolean zero = FALSE;
	gboolean source = FALSE;
	char *outfile = NULL;
	char *prefix = NULL;
	GError *error = NULL;
	GOptionContext *context;
	GOptionEntry option_entries[] =
	{
		{"amalgam", 'a', 0, G_OPTION_ARG_FILENAME, &outfile, "Filename of the amalgam file.", ""},
		{"zero", '0', 0, G_OPTION_ARG_NONE, &zero, "Read file list from stdin with \\0 separator.", ""},
		{"prefix", 'p', 0, G_OPTION_ARG_STRING, &prefix, "Prefix to use on all filenames.", ""},
		{"source", 's', 0, G_OPTION_ARG_NONE, &source, "Switch modes so that every EVEN file listed is the source to read from disk while ODD files are the stored name.", ""},
		{NULL}
	};
	context = g_option_context_new("- Amalgam file builder");
	g_option_context_add_main_entries(context, option_entries, NULL);
	res = g_option_context_parse(context, &argc, &argv, &error);
	if(!res)
	{
		fprintf(stderr, "Failed to parse arguments.\n");
		exit(-1);
	}
	if(!outfile)
	{
		fprintf(stderr, "You must specify a filename to write out to using -a.\n");
		exit(-1);
	}
	if(zero)
	{
		while((size = getdelim(&lineptr, &buffsize, '\0', stdin)) != -1)
		{
			count++;
			args = (char**)realloc(args, sizeof(char*) * count);
			if(args == NULL)
			{
				fprintf(stderr, "Failed to realloc.\n");
				exit(-1);
			}
			args[count - 1] = strdup(lineptr);
			printf("Got line %s\n", lineptr);
		}
		if(ferror(stdin))
		{
			return -errno;
		}
		printf("Size %d\n", count);
		res = amalgam_archive_build_from_list(outfile, (const char**)args, count, prefix, source);
	}
	else
	{
		res = amalgam_archive_build_from_list(outfile, (const char**)(argv + 1), argc - 1, prefix, source);
	}
	return res;
}

