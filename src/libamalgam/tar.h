#ifdef __linux__
#define _FILE_OFFSET_BITS 64
#endif
#include <sys/stat.h>
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>

typedef enum
{
	AMALGAM_READ_TAR
} amalgam_archive_readtype;

typedef struct {
	char *header_iter;
	int in_header;
	size_t size;
} header_data;

typedef struct {
	size_t header;
	size_t hf_pad;
	size_t file;
	size_t end_pad;
} tar_entry;

typedef struct {
	guint64 offset;
	guint8 type;
	guint64 data;
} __attribute((__packed__)) index_entry;

int amalgam_tar_header_get(const char *filename, char **tar_header, size_t *size, struct stat *st);
int amalgam_archive_build_tar_entry(tar_entry *ent, const char *filename, const char *srcfilename);
int amalgam_archive_build_from_list(const char *outname, const char **filename, int entries, const char *prefix, int source);
ssize_t amalgam_archive_read(const char *data, char *buffer_start, size_t size, off_t offset, amalgam_archive_readtype aar);
