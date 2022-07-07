#include "exbau.hpp"

BootRecord read_boot_record(FILE *disk){
    BootRecord boot_record;
    fseek(disk, 0, SEEK_SET);
    fread(&boot_record, sizeof(BootRecord), 1, disk);
    return boot_record;
}

void write_boot_record(FILE *disk, BootRecord boot_record){
    fseek(disk, 0, SEEK_SET);
    fwrite(&boot_record, sizeof(BootRecord), 1, disk);
}

void create_Block_BitMap(FILE *disk, unsigned int group_number, BootRecord *boot_record){
    int t = boot_record->reserved_sectors * boot_record->sector_size;
    //Remove um setor para o boot record
    int total_sectors = boot_record->total_sectors -1;

    int sector_to_ManagerFree= round(total_sectors/t);
    int seek_position = 1*boot_record->sector_size;

    //Byte zerado para setar inicialmente todos os setores como livres
    bitset <8> bit = 0;

    //Criando o setor de ManagerFree
    fseek(disk, seek_position, SEEK_SET);
    for(int i=0; i<sector_to_ManagerFree; i++){
        fwrite(&bit, sizeof(bitset <8>), 1, disk);
    }
}

void format_disk (FILE *disk){
    fseek(disk,0,SEEK_END);
    int size_file = ftell(disk);
    // cout << "Tamanho do arquivo: " << size_file << endl;
}

int find_offset_sector(int sector,int sector_size){
    return sector*sector_size;
}