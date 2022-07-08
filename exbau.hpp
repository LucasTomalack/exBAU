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

bool write_boot_record(FILE *disk, BootRecord *boot_record);

BootRecord read_boot_record(FILE *disk);

// Cria o bloco de BitMap
bool create_Block_BitMap(FILE *disk,  BootRecord *boot_record);

// Cria o bloco de dados
bool create_Block_DataSection(FILE *disk,  BootRecord *boot_record);



// Responsável por formatar o disco
void format_disk(FILE *disk);

// Verifica o offset do setor
unsigned find_offset_sector(unsigned int sector, unsigned short sector_size);

// Verifica o offset do setor na seção de dados
unsigned find_offset_sector_data(unsigned int sector, unsigned short sector_size,unsigned reserved_sectors);
