#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include "tar.h"

static inline size_t amalgam_tar_pad_size(size_t size)
{
	int res = size % 512;
	if(size == 0)
	{
		return size;
	}
	if(res == 0)
	{
		return res;
	}
	return 512 - res;
}

static int header_open(struct archive *a, void *data)
{
	return ARCHIVE_OK;
}

static ssize_t header_write(struct archive *a, void *data, const void *buff, size_t size)
{
	header_data *hd = (header_data*)data;
	if(hd->in_header)
	{
		memcpy(hd->header_iter, (const char *)buff, size);
		hd->header_iter += size;
	}
	return size;
}

static int header_close(struct archive *a, void *data)
{
	return 0;
}

static int size_open(struct archive *a, void *data)
{
	return ARCHIVE_OK;
}

static ssize_t size_write(struct archive *a, void *data, const void *buff, size_t size)
{
	header_data *hd = (header_data*)data;
	if(hd->in_header)
	{
		hd->size += size;
	}
	return size;
}

static int size_close(struct archive *a, void *data)
{
	return 0;
}

int amalgam_tar_header_get(const char *filename, char **tar_header, size_t *size, struct stat *st)
{
//FIXME error handling
//FIXME filename long or short?
	header_data hd;
	struct archive *a;
	struct archive_entry *entry;
	int res;
	archive_open_callback *aoc;
	archive_write_callback *awc;
        archive_close_callback *acc;
	if(!size || !st)
	{
		return -1;
	}
	if(tar_header && !*size)
	{
		res = amalgam_tar_header_get(filename, NULL, size, st);
		if(res)
		{
			return res;
		}
	}
	if(tar_header)
	{ 
		aoc = header_open;
		awc = header_write;
        	acc = header_close;
		hd.header_iter = (char *)malloc(sizeof(char) * (*size));
		if(!hd.header_iter)
		{
			return -ENOMEM;
		}
		*tar_header = hd.header_iter;
	}
	else
	{
		aoc = size_open;
		awc = size_write;
        	acc = size_close;
	}
	a = archive_write_new();
	if(!a)
	{
		fprintf(stderr, "Failed to create new archive object.\n");
		return -ENOMEM;
	}
	hd.size = 0;
	hd.in_header = 1;
	if(*size == 0)
	{
//FIXME Do archive_* need error checks? 
		archive_write_set_bytes_per_block(a, 1);
	}
	else
	{
		archive_write_set_bytes_per_block(a, *size);
	}
	archive_write_set_compression_none(a);
	archive_write_set_format_pax(a);
	archive_write_open(a, &hd, aoc, awc, acc);
	entry = archive_entry_new();
	archive_entry_copy_stat(entry, st);
	archive_entry_set_pathname(entry, filename);
	archive_write_header(a, entry);
	hd.in_header = 0;
	if(*size == 0)
	{
		*size = hd.size;
	}
	archive_entry_free(entry);
	archive_write_close(a);
	return 0;
}

int amalgam_archive_build_tar_entry(tar_entry *ent, const char *filename, const char *srcfilename)
{
//FIXME error handling
	int res;
	struct stat st;
	size_t size = 0;
	res = stat(srcfilename, &st);
	if(res)
	{
		fprintf(stderr, "Failed stat.\n");
		return -errno;
	}
	res = amalgam_tar_header_get(filename, NULL, &size, &st);
	if(!res)
	{
		ent->header = size;
		ent->hf_pad = amalgam_tar_pad_size(size);
		ent->file = st.st_size;
		ent->end_pad = amalgam_tar_pad_size(st.st_size);
	}
	return res;
}

//FIXME generic
int amalgam_archive_build_from_list(const char *outname, const char **filename, int entries, const char *prefix, int source)
{
	int res;
	char *tmpfilename;
	char *tmpfilename2;
	struct stat st;
	tar_entry te;
	index_entry ie;
	int i;
	int fd;
	int ttype;
	int type = 2;
	int inc = 1;
	guint64 offset = 0;
	guint64 size = GUINT64_TO_LE(entries * 4); /* Header, Pad, File, Pad */
	guint64 prefix_offset = sizeof(guint64) * 3 + (entries * 4 * sizeof(index_entry));
	guint64 data_offset;
	if(source)
	{
		inc = 2;
		type = 3;
		size = GUINT64_TO_LE(entries * 2); /* Header, Pad, File, Pad / 2 for sources */
		prefix_offset = sizeof(guint64) * 3 + (entries * 2 * sizeof(index_entry));
	}
  	fd = open(outname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	write(fd, &size, sizeof(guint64)); //Write dummy final size out. Will be rewritten later
	write(fd, &size, sizeof(guint64));
	data_offset = prefix_offset;
	if(prefix)
	{
		data_offset += strlen(prefix) + 1;
		size = GUINT64_TO_LE(prefix_offset);
	}
	else
	{
		size = GUINT64_TO_LE(0);
	}
	write(fd, &size, sizeof(guint64));
	for(i = 0; i < entries; i += inc)
	{
		ttype = type;
		if(source && filename[i + 1][0] == '\0')
		{
			ttype = 2;
		}
		if(ttype == 2)
		{
			tmpfilename2 = filename[i];
		}
		else
		{
			tmpfilename2 = filename[i + 1];
		}
		if(prefix)
		{
			tmpfilename = g_strdup_printf("%s/%s", prefix, tmpfilename2);
			res = amalgam_archive_build_tar_entry(&te, filename[i], tmpfilename);
			g_free(tmpfilename);
		}
		else
		{
			res = amalgam_archive_build_tar_entry(&te, filename[i], tmpfilename2);
		}
		if(res)
		{
			return res;
		}
		printf("Header size: %ld, Pad: %ld, file size: %ld, Pad: %ld\n", te.header, te.hf_pad, te.file, te.end_pad);
		/* Entry 1 is the header */
		ie.offset = GUINT64_TO_LE(offset);
		ie.type = 1;
		ie.data = GUINT64_TO_LE(data_offset);
		write(fd, &ie, sizeof(ie));
		offset += te.header;
		/* Entry 2 is the pad between header and file */
		ie.offset = GUINT64_TO_LE(offset);
		ie.type = 0;
		ie.data = GUINT64_TO_LE(te.hf_pad);
		write(fd, &ie, sizeof(ie));
		offset += te.hf_pad;
		/* Entry 3 is the file contents */
		ie.offset = GUINT64_TO_LE(offset);
		ie.type = ttype;
		ie.data = GUINT64_TO_LE(data_offset);
		write(fd, &ie, sizeof(ie));
		offset += te.file;
		if(ttype == 3)
		{
			data_offset += sizeof(struct stat) + strlen(filename[i]) + strlen(filename[i + 1]) + 2;
		}
		else
		{
			data_offset += sizeof(struct stat) + strlen(filename[i]) + 1;
		}
		/* Entry 4 is the pad to the end of block */
		ie.offset = GUINT64_TO_LE(offset);
		ie.type = 0;
		if((ttype == 2 && i + 1 == entries) || (ttype == 3 && i + 2 == entries))
		{
			te.end_pad += 2 * 512;
		}
		ie.data = GUINT64_TO_LE(te.end_pad);
		write(fd, &ie, sizeof(ie));
		offset += te.end_pad;
	}
	if(prefix)
	{
		write(fd, prefix, strlen(prefix) + 1);
	}
	for(i = 0; i < entries; i += inc)
	{
		ttype = type;
		if(source && filename[i + 1][0] == '\0')
		{
			ttype = 2;
		}
		if(ttype == 2)
		{
			tmpfilename2 = filename[i];
		}
		else
		{
			tmpfilename2 = filename[i + 1];
		}
		if(prefix)
		{
			tmpfilename = g_strdup_printf("%s/%s", prefix, tmpfilename2);
			res = stat(tmpfilename, &st);
			g_free(tmpfilename);
		}
		else
		{
			res = stat(tmpfilename2, &st);
		}
		if(res)
		{
			return res;
		}
		write(fd, &st, sizeof(st));
		write(fd, filename[i], strlen(filename[i]) + 1);
		if(ttype == 3)
		{
			write(fd, filename[i + 1], strlen(filename[i + 1]) + 1);
		}
	}
	size = GUINT64_TO_LE(offset);
	pwrite(fd, &size, sizeof(guint64), 0); //Write final size out.
	close(fd);
	return 0;
}

typedef struct
{
	index_entry *end;
	off_t offset;
} search_t;

static int search_ie(const void *key, const void *value)
{
	search_t *search = (search_t*)key;
	index_entry *ie = (index_entry*)value;
	if(ie == search->end)
	{
		if(search->offset >= ie->offset)
		{
			return 0;
		}
		return -1;
	}
	if(search->offset < ie->offset)
	{
		return -1;
	}
	if(search->offset < (ie + 1)->offset)
	{
		return 0;
	}
	return 1;
}

void amalgam_archive_info_print(const char *data, FILE *out)
{
	char *tmpptr;
	guint64 i;
	index_entry *ie = (index_entry*)(data + sizeof(guint64) * 3);
	guint64 total_size = GUINT64_FROM_LE(*(guint64*)(data));
	guint64 ie_size = GUINT64_FROM_LE(*(guint64*)(data + sizeof(guint64)));
	guint64 prefix_offset = GUINT64_FROM_LE(*(guint64*)(data + 2 * sizeof(guint64)));
	struct stat *st;
	fprintf(out, "Total_size: %ld\n", total_size);
	fprintf(out, "Number of entries: %ld\n", ie_size);
	fprintf(out, "Prefix Offset: %ld\n", prefix_offset);
	if(prefix_offset)
	{
		fprintf(out, "Prefix: %s\n", data + prefix_offset);
	}
	for(i = 0; i < ie_size; i++)
	{
		fprintf(out, "Index entry: %ld %d %ld\n", ie[i].offset, ie[i].type, ie[i].data);
		if(ie[i].type == 1 || ie[i].type == 2)
		{
			st = (struct stat*)(data + ie[i].data);
			fprintf(out, "Size: %ld, Filename: %s\n", st->st_size, (char*)(data + ie[i].data + sizeof(struct stat)));
		}
		else if(ie[i].type == 3)
		{
			st = (struct stat*)(data + ie[i].data);
			tmpptr = (char*)(data + ie[i].data + sizeof(struct stat));
			fprintf(out, "Size: %ld, Filename: %s, Source: %s\n", st->st_size, tmpptr, tmpptr + strlen(tmpptr) + 1);
		}
		else if(ie[i].type == 0)
		{
			fprintf(out, "Pad: %ld\n", ie[i].data);
		}
	}
}

ssize_t amalgam_archive_read(const char *data, char *buffer_start, size_t size, off_t offset, amalgam_archive_readtype aar)
{
	int fd;
	int tres;
	search_t search;
	index_entry *fie;
	index_entry *ie = (index_entry*)(data + sizeof(guint64) * 3);
	guint64 total_size = GUINT64_FROM_LE(*(guint64*)(data));
	guint64 ie_size = GUINT64_FROM_LE(*(guint64*)(data + sizeof(guint64)));
	guint64 prefix_offset = GUINT64_FROM_LE(*(guint64*)(data + 2 * sizeof(guint64)));
	char *filename;
	size_t res = 0;
	size_t size_left;
	size_t tsize;
	size_t header_size;
	struct stat *st;
	char *header;
	const char *prefix = NULL;
	if(aar != AMALGAM_READ_TAR)
	{
		return -EINVAL;
	}
	if(prefix_offset)
	{
		for(prefix = data + prefix_offset; *prefix == ' ' || *prefix == '/' || *prefix == '\t'; prefix++);
		if(*prefix == '\0')
		{
			prefix = NULL;
		}
	}
	if(offset >= total_size)
	{
		return 0;
	}
	if(total_size - offset < size)
	{
		size = total_size - offset;
	}
	search.end = ie + ie_size - 1;
	size_left = size;
	amalgam_archive_info_print(data, stdout);
	printf("To read: %ld %ld\n", offset, size);
/*
	printf("Total_size: %ld\n", total_size);
	printf("Number of entries: %ld\n", ie_size);
	printf("To read: %ld %ld\n", offset, size);
	for(i = 0; i < ie_size; i++)
	{
		printf("Index entry: %ld %d %ld\n", ie[i].offset, ie[i].type, ie[i].data);
		if(ie[i].type == 1 || ie[i].type == 2)
		{
			st = (struct stat*)(data + ie[i].data);
			printf("Size: %ld, Filename: %s\n", st->st_size, (char*)(data + ie[i].data + sizeof(struct stat)));
		}
		if(ie[i].type == 0)
		{
			printf("Pad: %ld\n", ie[i].data);
		}
	}
*/
	while(res < size)
	{
		search.offset = offset;
		fie = (index_entry*)bsearch(&search, ie, ie_size, sizeof(index_entry), search_ie);
		if(fie)
		{
			if(fie->type == 0)
			{
				tsize = MIN(size_left, fie->data);
				tsize = MIN(size_left, fie->data - (offset - fie->offset));
				size_left -= tsize;
				printf("Padding %ld\n", tsize);
				memset(buffer_start + res, 0, tsize);
				res += tsize;
				offset += tsize;
			}
			else if(fie->type == 1)
			{
				header_size = fie[1].offset - fie->offset;
				tsize = MIN(size_left, header_size - (offset - fie->offset));
				st = (struct stat*)(data + fie->data);
				filename = (char*)(data + fie->data + sizeof(struct stat));
				printf("Building header. size %ld %ld\n", header_size, tsize);
				amalgam_tar_header_get(filename, &header, &header_size, st);

				size_left -= tsize;
				memcpy(buffer_start + res, header + offset - fie->offset, tsize);
				free(header);
				res += tsize;
				offset += tsize;
			}
			else if(fie->type == 2 || fie->type == 3)
			{
				header_size = fie[1].offset - fie->offset;
				tsize = MIN(size_left, header_size - (offset - fie->offset));
				filename = (char*)(data + fie->data + sizeof(struct stat));
				if(fie->type == 3)
				{
					filename = filename + strlen(filename) + 1;
				}
				printf("Building file. size %ld %ld\n", header_size, tsize);
				
				if(prefix)
				{
					filename = g_strdup_printf("%s/%s", prefix, filename);
				}
				printf("Filename: %s\n", filename);
				fd = open(filename, O_RDONLY);
				if(fd == -1)
				{
					return -EIO;
				}
				if(prefix)
				{
					g_free(filename);
				}
				size_left -= tsize;
//FIXME res
				tres = pread(fd, buffer_start + res, tsize, offset - fie->offset);
				printf("tres %d\n", tres);
				close(fd);
				if(tres == -1)
				{
					return -errno;
				}
				res += tsize;
				offset += tsize;
			}
			else
			{
				//unknown fie type.
				break;
			}
		}
		else
		{
			break;
			//error.
		}
	}
	return res;
}
