#include <iostream>
#include <string>
#include <cmath>
#include <bitset>
#include <vector>

typedef struct BootRecord
{
    unsigned long int volume_size;
    unsigned short sector_size;
    unsigned short reserved_sectors;
    unsigned int total_sectors;
}__attribute__((packed)) BootRecord;

typedef struct FileFormat
{
    char filename[21];
    char ext[4];
    char attribute;
    unsigned short first_sector;
    unsigned int size;
}__attribute__((packed)) FileFormat;
using namespace std;

// Define um "tipo"
typedef bitset<8> byte_;


bool write_boot_record(FILE *disk, BootRecord *boot_record);

BootRecord read_boot_record(FILE *disk);

// Cria o bloco de BitMap
bool create_Block_BitMap(FILE *disk,  BootRecord *boot_record);

void manage_sector_BitMap(FILE *disk, BootRecord *boot_record, int sector_number, bool new_value);

// Retorna o byte do setor da seção de dados dentro do BitMap
byte_ get_byte_sector_BitMap(FILE *disk,BootRecord *boot_record, int sector_number);

// Verifica o bit do setor da seção de dados está livre ou não
bool check_sector_BitMap(FILE *disk, BootRecord *boot_record, int sector_number);

// Cria o bloco de dados
bool create_Block_DataSection(FILE *disk,  BootRecord *boot_record);

// Responsável por formatar o disco
void format_disk(FILE *disk);

// Verifica o offset do setor
unsigned find_offset_sector(unsigned int sector, unsigned short sector_size);

// Verifica o offset do setor na seção de dados
unsigned find_offset_sector_data(unsigned int sector, unsigned short sector_size,unsigned reserved_sectors);

//Retorna o offset do bitmap
unsigned int find_offset_bitmap();

//Aloca um arquivo no bitmap e retorna um vetor com os setores alocados
vector<unsigned int> alocate_file(FILE *disk, BootRecord boot_record, unsigned long long int file_size);

//copia um arquivo para o sistema de arquivos
bool copy_file(FILE *disk, BootRecord boot_record, const char *filename);