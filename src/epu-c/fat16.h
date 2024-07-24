#ifndef fat_h
#define fat_h

#include "inttypes.h"
#include "string.h"

typedef struct fat_boot_sector_t {
    uint8_t   bootstrap_code1[3];
    char      os_code[8];
    uint16_t  sector_size;
    uint8_t   cluster_size;
    uint16_t  reserved_sectors;
    uint8_t   copies;
    uint16_t  root_entries;
    uint16_t  sector_count_small;
    uint8_t   media_descriptor;
    uint16_t  sectors_per_fat;
    uint16_t  sectors_per_track;
    uint16_t  sectors_per_head;
    uint32_t  hidden_sectors;
    uint32_t  sector_count_large;
    uint8_t   drive_number;
    uint8_t   reserved;
    uint8_t   boot_signature;
    uint32_t  serial_number;
    char      volume_label[11];
    char      fs_type[8];
    char      bootstrap_code2[448];
    uint16_t  signature;
} __attribute__((packed)) fat_boot_sector;

typedef struct fat_file_small_t {
    char     name[8];
    char     ext[3];
    char     attr;
    uint8_t  reserved;
    uint8_t  creation_ms;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    char     _padding[2];
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t start_cluster;
    uint32_t file_size;
} __attribute__((packed)) fat_file_small;

typedef struct fat_file_extended_t {
    uint8_t        ordinal;
    uint16_t       name1[5];
    char           attr;
    char           _padding1[1];
    uint8_t        checksum;
    uint16_t       name2[6];
    char           _padding2[2];
    uint16_t       name3[2];
    fat_file_small entry;
} __attribute__((packed)) fat_file_extended;

/* (non-standard) */
typedef struct dos_time_t {
    uint8_t  cs;
    uint16_t time;
    uint16_t date;
} dos_time;

/* (non-standard) */
typedef struct fat_file_t {
    char name[8];
    char ext[3];
    uint32_t size;
    char flags;
    dos_time creation;
    dos_time last_write;
    dos_time last_read;
    void* _data;
    uint16_t _start;
} fat_file;

typedef struct fat_disk_t {
    uint8_t* data;
    fat_boot_sector* boot;
} __attribute__((packed)) fat_disk;

/* Reads the boot sector from a disk and writes it into the disk struct */
void fat_read_boot_sector(fat_disk* disk) 
#ifdef fat_impl
{
    *disk->boot = (fat_boot_sector){
        .bootstrap_code1 = { 0 }, // TODO: read this
        .os_code = { disk->data[3], disk->data[4], disk->data[5], disk->data[6], disk->data[7], disk->data[8], disk->data[9], disk->data[10] }, // TODO: read this better
        .sector_size = *(uint16_t*)(disk->data+0x000B),
        .cluster_size = *(disk->data+0x000D),
        .reserved_sectors = *(uint16_t*)(disk->data+0x000E),
        .copies = *(disk->data+0x0010),
        .root_entries = *(uint16_t*)(disk->data+0x0011),
        .sector_count_small = *(uint16_t*)(disk->data+0x0013),
        .media_descriptor = *(disk->data+0x0015),
        .sectors_per_fat = *(uint16_t*)(disk->data+0x0016),
        .sectors_per_track = *(uint16_t*)(disk->data+0x0018),
        .sectors_per_head = *(uint16_t*)(disk->data+0x001A),
        .hidden_sectors = *(uint32_t*)(disk->data+0x001C),
        .sector_count_large = *(uint32_t*)(disk->data+0x0020),
        .drive_number = *(disk->data+0x0024),
        .reserved = *(disk->data+0x0025),
        .boot_signature = *(uint32_t*)(disk->data+0x0027),
        .volume_label = { disk->data[0x002B+0], disk->data[0x002B+1], disk->data[0x002B+2], disk->data[0x002B+3], disk->data[0x002B+4], disk->data[0x002B+5], disk->data[0x002B+6], disk->data[0x002B+7], disk->data[0x002B+8], disk->data[0x002B+9], disk->data[0x002B+10] },// TODO: read this better
        .fs_type = { disk->data[0x0036+0], disk->data[0x0036+1], disk->data[0x0036+2], disk->data[0x0036+3], disk->data[0x0036+4], disk->data[0x0036+5], disk->data[0x0036+6], disk->data[0x0036+7] }, // TODO: read this better
        .bootstrap_code2 = { 0 }, // TODO: read this
        .signature = *(uint16_t*)(disk->data+0x01FE)
    };
}
#endif
;

/* Reads a small directory entry */
void fat_read_file_small(uint8_t* data, fat_file_small* file)
#ifdef fat_impl
{
    *file = (fat_file_small){
        .name = { data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7] }, // TODO: read this better
        .ext = { data[8], data[9], data[10] }, // TODO: read this better
        .attr = *(data+0x0B),
        .reserved = *(data+0x0C),
        .creation_ms = *(data+0x0D),
        .creation_time = *(uint16_t*)(data+0x0E),
        .creation_date = *(uint16_t*)(data+0x10),
        .last_access_date = *(uint16_t*)(data+0x12),
        .last_write_time = *(uint16_t*)(data+0x16),
        .last_write_date = *(uint16_t*)(data+0x18),
        .start_cluster = *(uint16_t*)(data+0x1A),
        .file_size = *(uint32_t*)(data+0x1C)
    };
}
#endif
;

/* Turns a sector number into an absolute address on a disk */
size_t fat_addr(fat_disk* disk, size_t addr)
#ifdef fat_impl
{
    return addr * disk->boot->sector_size;
}
#endif
;

/* Returns the sector of the FAT region of a disk */
size_t fat_addr_fat_region(fat_disk* disk)
#ifdef fat_impl
{
    return disk->boot->reserved_sectors;
}
#endif
;

/* Returns the sector of the root directory of a disk */
size_t fat_addr_root_directory_region(fat_disk* disk) 
#ifdef fat_impl
{
    return fat_addr_fat_region(disk)+disk->boot->copies*disk->boot->sectors_per_fat;
}
#endif
;

/* Returns the first sector of the data region of a disk */
size_t fat_addr_data_region(fat_disk* disk)
#ifdef fat_impl
{
    return fat_addr_root_directory_region(disk)+disk->boot->root_entries*32/disk->boot->sector_size;
}
#endif
;

/* Returns the sector of a cluster of a disk */
size_t fat_addr_cluster(fat_disk* disk, int cluster)
#ifdef fat_impl
{
    return fat_addr_data_region(disk)+((cluster-2)*disk->boot->cluster_size);
}
#endif
;

/* Returns the FAT entry for a cluster of a disk */
uint16_t fat_cluster_entry(fat_disk* disk, int cluster) 
#ifdef fat_impl
{
    return *(uint16_t*)(disk->data+fat_addr(disk,fat_addr_fat_region(disk))+cluster*2);
}
#endif
;

/* Retrieves the 'boot' file of a disk (not standard) */
int fat_boot_file(fat_disk* disk, void* data, int* size)
#ifdef fat_impl
{
    const size_t cluster_size = disk->boot->cluster_size*disk->boot->sector_size;
    const size_t entries = disk->boot->root_entries;
    const size_t root_addr = fat_addr(disk,fat_addr_root_directory_region(disk));
    if (entries == 0) return 1;
    fat_file_small file;
    for (size_t i = 0; i < entries; i++) {
        fat_read_file_small(disk->data+root_addr+i*32,&file);
        if (!strcmpl(file.name,"BOOT    ",8) && !strcmpl(file.ext,"   ",3)) {
            if (size) {
                *size = file.file_size;
            }
            if (data) {
                uint16_t cluster = file.start_cluster;
                size_t written = 0;
                for (;;) {
                    const size_t data_addr = fat_addr(disk,fat_addr_cluster(disk,cluster));
                    for (size_t i = 0; i < cluster_size && written < file.file_size; i++) {
                        ((uint8_t*)data)[written++] = disk->data[data_addr+i];
                    }
                    if (cluster < 0x0003 || cluster > 0xFFEF || written >= file.file_size) break;
                    cluster = fat_cluster_entry(disk,cluster);
                }
            }
            return 0;
        }
    }
    return 1;
}
#endif
;

/* Reads a directory entry (not standard) */
void fat_read_file_entry(uint8_t* data, fat_file* file)
#ifdef fat_impl
{
    *file = (fat_file){
        .name = {' '},
        .ext = {' '},
        .flags = *(data+0x0B),
        .size = *(uint32_t*)(data+0x1C),
        .creation = {
            .cs  = 0,
            .time = *(uint16_t*)(data+0x0E),
            .date = *(uint16_t*)(data+0x10)
        },
        .last_read = { 
            .cs = 0,
            .time = 0,
            .date = *(uint16_t*)(data+0x12),
        },
        .last_write = {
            .cs = 0,
            .time = *(uint16_t*)(data+0x16),
            .date = *(uint16_t*)(data+0x18),
        },
        ._data = data,
        ._start =  *(uint16_t*)(data+0x1A),
    };

    // TODO: Somehow handle LFN entries
    for (int i = 0; i < 8; i++) {
        file->name[i] = data[i];
    }
    for (int i = 0; i < 3; i++) {
        file->ext[i] = data[8+i];
    }
}
#endif
;

#endif