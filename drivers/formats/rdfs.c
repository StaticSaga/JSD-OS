#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "rdfs.h"

static int rdfs_mount_disk(filesystem_drive* d);
static fs_index rdfs_get_relative_location(fs_index location, size_t byte_offset, const filesystem_drive* fd);
static fs_index rdfs_read_chunks(uint8_t* dest, fs_index location, size_t num_bytes, const filesystem_drive* fd);
static void rdfs_read_dir(directory_handle* dest, const file_handle* f, const filesystem_drive* fd);

static filesystem_driver rdfs_driver = {
	rdfs_mount_disk,
	rdfs_get_relative_location,
	rdfs_read_chunks,
	rdfs_read_dir
};

void rdfs_init()
{
	filesystem_add_driver(&rdfs_driver);
}

static int rdfs_mount_disk(filesystem_drive* fd)
{
	if(fd->minimum_block_size > 1 || fd->dsk_driver->allocate_buffer != NULL)
	{
		return DRIVE_NOT_SUPPORTED;
	}

	uint8_t magic[4];
	filesystem_read_blocks_from_disk(fd, 0, &magic[0], sizeof(magic));

	if(memcmp(magic, "RDSK", 4) == 0)
	{
		file_handle dir = {
			NULL, NULL, NULL,
			IS_DIR, fd->index, sizeof(uint8_t) * 4,
			0, 0, 0
		};

		rdfs_read_dir(&fd->root, &dir, fd);
		return MOUNT_SUCCESS;
	}
	return UNKNOWN_FILESYSTEM;
}

static fs_index rdfs_get_relative_location(fs_index location, size_t byte_offset, const filesystem_drive* fd)
{
	return location + byte_offset;
}

static fs_index rdfs_read_chunks(uint8_t* dest, fs_index location, size_t num_bytes, const filesystem_drive* fd)
{
	filesystem_read_blocks_from_disk(fd, location, dest, num_bytes);
	return location + num_bytes;
}

typedef struct
{
	char 		name[8];
	char 		extension[3];
	uint8_t		attributes;
	uint32_t	offset;
	uint32_t	size;
	time_t		modified;
	time_t		created;
} __attribute__((packed)) rdfs_dir_entry;

static void rdfs_read_dir(directory_handle* dest, const file_handle* f, const filesystem_drive* fd)
{
	fs_index location = f->location_on_disk;

	uint32_t num_files;
	filesystem_read_blocks_from_disk(fd, location, (uint8_t*)&num_files, sizeof(uint32_t));

	dest->name = "";
	dest->file_list = (file_handle*)malloc(num_files * sizeof(file_handle));
	dest->drive = fd->index;

	fs_index disk_location = location + sizeof(uint32_t);
	for(size_t i = 0; i < num_files; i++)
	{
		rdfs_dir_entry entry;
		filesystem_read_blocks_from_disk(fd, disk_location, (uint8_t*)&entry, sizeof(rdfs_dir_entry));
		disk_location += sizeof(rdfs_dir_entry);

		char* full_name = (char*)malloc(13 * sizeof(char));

		dest->file_list[i].name = (char*)malloc(9 * sizeof(char));
		memcpy(dest->file_list[i].name, entry.name, 8);
		dest->file_list[i].name[8] = '\0';

		dest->file_list[i].type = (char*)malloc(4 * sizeof(char));
		memcpy(dest->file_list[i].type, entry.extension, 3);

		//count chars in extension
		size_t ext_length = 0;
		for(; ext_length < 3 && entry.extension[ext_length] != ' '; ext_length++);
		dest->file_list[i].type[ext_length] = '\0';

		strcpy(full_name, strtok(dest->file_list[i].name, " "));
		if(ext_length > 0)
		{
			strcat(full_name, ".");
			strcat(full_name, strtok(dest->file_list[i].type, " "));
		}

		dest->file_list[i].full_name = full_name;

		dest->file_list[i].size = entry.size;

		dest->file_list[i].time_created = entry.created;
		dest->file_list[i].time_modified = entry.modified;

		dest->file_list[i].disk = fd->index;
		dest->file_list[i].location_on_disk = entry.offset;

		uint32_t flags = 0;

		if(entry.attributes & IS_DIR)
		{
			flags |= IS_DIR;
		}

		dest->file_list[i].flags = flags;
	}

	dest->num_files = num_files;
}
