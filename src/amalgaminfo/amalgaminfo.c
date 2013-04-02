#include <errno.h>
#include <sys/mman.h>
#include "tar.h"

int main(int argc, char *argv[])
{
	int i;
	int fd;
	int res;
	char *data;
	struct stat st;
	GError *error = NULL;
	GOptionContext *context;
	GOptionEntry option_entries[] =
	{
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
	for(i = 1; i < argc; i++)
	{
		fd = open(argv[i], O_RDONLY);
		if(fd == -1)
		{
			fprintf(stderr, "Failed to open %s: %d\n", argv[i], errno);
		}
		else
		{
			res = fstat(fd, &st);
			if(res)
			{
				fprintf(stderr, "Failed to stat %s.\n", argv[i]);
			}
			else
			{
				data = (char*)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
				if(!data)
				{
					fprintf(stderr, "Failed to memmap %s.\n", argv[i]);
				}
				else
				{
					amalgam_archive_info_print(data, stdout);
					res = munmap(data, st.st_size);
				}
			}
		}
	}
	return 0;
}

