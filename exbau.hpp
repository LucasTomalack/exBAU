#include <iostream>
#include <string>
#include <cmath>
#include <bitset>

typedef struct BootRecord
{
    unsigned long int volume_size;
    unsigned short sector_size;
    unsigned short reserved_sectors;
    unsigned int total_sectors;
}__attribute__((packed)) BootRecord;

typedef struct FormatFile
{
    char filename[21];
    char ext[4];
    char attribute;
    unsigned short first_sector;
    unsigned int size;
}__attribute__((packed)) FileFormat;

using namespace std;

BootRecord read_boot_record(FILE *disk);

void format_disk(FILE *disk);