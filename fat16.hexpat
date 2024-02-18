struct boot_sector {
    u8   code[3];
    char os_code[8];
    u16  sector_size;
    u8   cluster_size;
    u16  reserved_sectors;
    u8   copies;
    u16  root_entries;
    u16  sector_count_small;
    u8   media_descriptor;
    u16  sectors_per_fat;
    u16  sectors_per_track;
    u16  sectors_per_head;
    u32  hidden_sectors;
    u32  sector_count_large;
    u8   drive_number;
    u8   reserved;
    u8   boot_signature;
    u32  serial_number;
    char volume_label[11];
    char fs_type[8];
    char bootstrap_code[448];
    u16  signature;
};

struct file_allocation_table {
    u16 type;
};

bitfield fat_file_attr {
    read_only : 1;
    hidden    : 1;
    system    : 1;
    volume    : 1;
    directory : 1;
    archive   : 1;
    padding   : 2;
};

struct fat_file_small {
    char name[8];
    char ext[3];
    fat_file_attr attr;
    u8 reserved;
    u8 creation_ms;
    u16 creation_time;
    u16 creation_date;
    u16 last_access_date;
    padding[2];
    u16 last_write_time;
    u16 last_write_date;
    u16 start_cluster;
    u32 file_size;
};

struct fat_file {
    u8 ordinal;
    char16 name1[5];
    fat_file_attr attr;
    padding[1];
    u8 checksum;
    char16 name2[6];
    padding[2];
    char16 name3[2];
    fat_file_small entry;
};

struct sector {
    u8 data[512];
};

boot_sector boot @0x0000;
file_allocation_table fat @(boot.reserved_sectors*512);
// fat_file root @((boot.reserved_sectors+boot.copies*boot.sectors_per_fat)*512);
fat_file f @((boot.reserved_sectors+boot.copies*boot.sectors_per_fat)*512+0xa0);
sector sectors[] @0x200;



