#include <iostream>
#include <string>
#include <cmath>
#include <bitset>
#include <vector>

typedef unsigned int Sector;

typedef struct BootRecord
{
    unsigned long int volume_size;
    unsigned short sector_size;
    unsigned short reserved_sectors;
    unsigned int total_sectors;
}__attribute__((packed)) BootRecord;

typedef struct FileFormat
{
    char filename[15];
    char ext[4];
    char attribute;
    Sector first_sector;
    unsigned long long int size;
}__attribute__((packed)) FileFormat;
using namespace std;

//Auxiliar para os atributos dos arquivos
#define FILE_ATTRIBUTE 0x20
#define DIRECTORY_ATTRIBUTE 0x10
#define DELETED_ATTRIBUTE 0xFF
#define LAST_FILE_ATTRIBUTE 0x00

#define LAST_QUEUE_SECTOR 0xFFFFFFFF

//Define um "tipo"
typedef bitset<8> byte_;


bool write_boot_record(FILE *disk, BootRecord *boot_record);

BootRecord read_boot_record(FILE *disk);

//Cria o bloco de BitMap
bool create_Block_BitMap(FILE *disk,  BootRecord *boot_record);

//Altera o status do setor da seção de dados no bloco de BitMap
void manage_sector_BitMap(FILE *disk, BootRecord boot_record, int sector_number, bool new_value);

//Retorna o byte do setor da seção de dados dentro do BitMap
byte_ get_byte_sector_BitMap(FILE *disk,BootRecord boot_record, int sector_number);

//Verifica o bit do setor da seção de dados está livre ou não
bool check_sector_BitMap(FILE *disk, BootRecord boot_record, int sector_number);

//Retorna o número do primeiro setor livre
unsigned int find_free_sector(FILE *disk, BootRecord boot_record);

//Cria o bloco de dados
bool create_Block_DataSection(FILE *disk,  BootRecord *boot_record);

//Responsável por formatar o disco
void format_disk(FILE *disk);

//Verifica o próximo ponteiro da lista
unsigned int next_sector(FILE *disk, BootRecord boot_record, unsigned int sector_number);

//Verifica o offset do setor
unsigned find_offset_sector(unsigned int sector, unsigned short sector_size);

//Verifica o offset do setor na seção de dados
unsigned find_offset_sector_data(unsigned int sector, unsigned short sector_size,unsigned reserved_sectors);

//Retorna o offset do bitmap
unsigned int find_offset_bitmap();

//Vai verificar um offset no diretório onde pode armazenar um novo atributo do arquivo/diretório
unsigned int find_offset_data(unsigned int sector, unsigned short sector_size, unsigned sector_number);

//Vai até o setor do diretório e escreve o atributo do arquivo/diretório
bool alocate_attribute_to_directory(FILE *disk, BootRecord boot_record, unsigned int sector_number, FileFormat *file_format);

//Cria um formato de arquivo a partir dos parâmetros passados
FileFormat new_file_format(const char *filename, const char *ext, char attribute, unsigned int first_sector, unsigned long long int size);

//aloca um diretório no bitmap e retorna o setor correspondente da seção de dados
unsigned int alocate_dir (FILE *disk, BootRecord boot_record, unsigned int prev_dir_sector);

//Aloca um arquivo no bitmap e retorna um vetor com os setores alocados
vector<unsigned int> alocate_file(FILE *disk, BootRecord boot_record, unsigned long long int file_size);

//copia um arquivo para o sistema de arquivos
bool copy_file(FILE *disk, BootRecord boot_record,  string filename);

//Faz a leitura de um arquivo/diretório do sistema de arquivos
void read_sector(FILE *disk, BootRecord boot_record,unsigned int sector_number, bool directory);
