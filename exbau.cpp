#include "exbau.hpp"

BootRecord read_boot_record(FILE *disk){
    BootRecord boot_record;
    fseek(disk, 0, SEEK_SET);
    fread(&boot_record, sizeof(BootRecord), 1, disk);
    return boot_record;
}

bool write_boot_record(FILE *disk, BootRecord boot_record){
    fseek(disk, 0, SEEK_SET);
    fwrite(&boot_record, sizeof(BootRecord), 1, disk);
    return true;
}

bool create_Block_BitMap(FILE *disk,  BootRecord *boot_record){
    // Verifica quantos setores da seção de dados pode ser mapeado em um setor de bitmap
    int t = 8 * boot_record->sector_size;

    //Remove um setor para o boot record
    int total_sectors = boot_record->total_sectors -1;

    // Verifica o número de setores que serão mapeados em um setor de bitmap
    int sector_to_ManagerFree= round(total_sectors/t);

    // Se o disco for muito pequeno a fórmula pode dar erro
    // Caso seja o caso, automaticamente será setado um setor para o gerenciamento de espaço livre
    if(sector_to_ManagerFree == 0){
        sector_to_ManagerFree = 1;
    }

    int seek_position = find_offset_sector(1, boot_record->sector_size);

    //Criando o bloco de ManagerFree
    fseek(disk, seek_position, SEEK_SET);
    for(int i=0; i<sector_to_ManagerFree; i++){
        for(int j=1; j<=boot_record->sector_size; j++){
            fputc(0, disk);
        }
    }
    boot_record->reserved_sectors = sector_to_ManagerFree+1;

    return true;
}

void manage_sector_BitMap(FILE *disk, BootRecord *boot_record, int sector_number, bool new_value){

    // Pega o bit do setor de bitmap
    unsigned short bit_position = sector_number%8;
   
    // Recebe o byte da posição do setor de bitmap
    byte_ b = get_byte_sector_BitMap(disk, boot_record, sector_number);
    // Altera o bit do setor de bitmap
    b[bit_position] = new_value;
    cout << "Sector: " << sector_number << " - " << b << endl;

    // Verifica o offset do setor
    unsigned seek_position = find_offset_sector(1, boot_record->sector_size);
    unsigned sector_position = seek_position + (sector_number/8);

    //Volta ao setor de bitmap e escreve o byte modificado
    fseek(disk, sector_position, SEEK_SET);
    fputc(b.to_ulong(), disk);
    
    return;   
}

byte_ get_byte_sector_BitMap(FILE *disk,BootRecord *boot_record, int sector_number){
    // Verifica o offset do setor do bitmap
    unsigned seek_position = find_offset_sector(1, boot_record->sector_size);
    // Verifica o offset do byte que possui o dado do setor
    unsigned sector_position = seek_position + (sector_number/8);
    // Coloca o ponteiro no byte que possui o dado do setor
    fseek(disk, sector_position, SEEK_SET);
    byte_ b(fgetc(disk));
    return b;
}

bool check_sector_BitMap(FILE *disk, BootRecord *boot_record, int sector_number){
    byte_ b = get_byte_sector_BitMap(disk, boot_record, sector_number);
    // Verifica o bit do setor de bitmap
    unsigned short bit_position = sector_number%8;
    return b[bit_position];
}

bool create_Block_DataSection(FILE *disk,  BootRecord *boot_record){
    // Verifica quantos setores o disco pode usar para a área de dados
    unsigned int sector_DataSection = boot_record->total_sectors - boot_record->reserved_sectors;

    // Calcula a posição do setor de dados
    int seek_position = find_offset_sector(boot_record->reserved_sectors, boot_record->sector_size);  

    //Criando o bloco de dados
    fseek(disk, seek_position, SEEK_SET);

    for(int i=0; i<sector_DataSection; i++){
        for(int j=1; j<=boot_record->sector_size; j++){
            // Insere o byte zerado
            fputc(0, disk);
            
        }
    }
    return true;
}

void format_disk (FILE *disk){
    fseek(disk,0,SEEK_END);
    unsigned long file_size = ftell(disk);
    if(file_size==0){
        printf("Erro: Arquivo vazio\n");
        return;
    }

    cout << "Formatando... por favor, aguarde!" << endl;
    BootRecord boot_record;
    boot_record.sector_size = 512;
    // Calcula o total de setores
    if(file_size%boot_record.sector_size)
        boot_record.total_sectors = (file_size/boot_record.sector_size)+1;
    else
        boot_record.total_sectors = file_size/boot_record.sector_size;
    boot_record.volume_size = file_size;

    if(boot_record.total_sectors<0){
        printf("O sistema necessita ao menos 3 setores\n");
        return;
    }
    // Cria o bloco de BitMap
    create_Block_BitMap(disk, &boot_record);
    // Cria o bloco de dados
    create_Block_DataSection(disk, &boot_record);
    // Escreve o boot record no disco
    write_boot_record(disk, boot_record);
    cout << "Formatado com sucesso" << endl;


}

unsigned find_offset_sector(unsigned sector,unsigned short sector_size){
    return sector*sector_size;
}

unsigned find_offset_sector_data(unsigned sector,unsigned short sector_size,unsigned reserved_sectors){
    return (reserved_sectors+ sector)*sector_size+sector_size;
}