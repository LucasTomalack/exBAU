#include "exbau.hpp"
#include <vector>

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
    unsigned long long int file_size = ftell(disk);
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

unsigned int find_offset_bitmap(){
    return 512;
}

//aloca um arquivo no bitmap e retorna um vetor com seus setores
vector<unsigned int> alocate_file(FILE *disk, BootRecord boot_record, unsigned long long int file_size){
    unsigned int data_section_sectors = boot_record.total_sectors - boot_record.reserved_sectors;
    unsigned int needed_sectors=file_size/508;
    if(file_size%508)
        needed_sectors++;
    //acessa o bitmap
    vector<unsigned int> available_sectors;
    byte_ B;
    fseek(disk, find_offset_bitmap(), SEEK_SET);
    for (int i = 0; i < data_section_sectors, available_sectors.size() < needed_sectors; i++)
    {
        fread(&B, sizeof(byte_), 1, disk);
        for (short j = 0; j < 8, available_sectors.size() < needed_sectors; j++)
        {
            if(B[j]==0){
                available_sectors.push_back((i*8)+j);   
                B[j]=1;
            }        
        }
    }

    if(available_sectors.size()==needed_sectors){
        for (int i = 0; i < needed_sectors; i++)
        {
            fseek(disk, find_offset_bitmap() + available_sectors[i]/8, SEEK_SET);
            fread(&B, sizeof(byte_), 1, disk);
            B[available_sectors[i]%8] = 1;
            fseek(disk, find_offset_bitmap() + available_sectors[i]/8, SEEK_SET);
            fwrite(&B, sizeof(byte_),1,disk);
        }
    }
    return available_sectors;
}

bool copy_file(FILE *disk, BootRecord boot_record, const char *filename){
    //abre o arquivo a ser copiado
    FILE *file_to_copy = fopen(filename, "r");
    if(file_to_copy == NULL){
        cerr << "Erro ao abrir arquivo!" << endl;
    }
    // verifica se há espaço disponível para o arquivo no sistema de arquivos
    fseek(file_to_copy, 0, SEEK_END);
    unsigned long long int file_size = ftell(file_to_copy);
    
    vector<unsigned int> alocated_sectors = alocate_file(disk, boot_record, file_size);
    unsigned int needed_sectors=file_size/508;
    if(file_size%508)
        needed_sectors++;
    if(alocated_sectors.size() < needed_sectors){
        cerr << "Não há espaço suficiente para alocar o arquivo" << endl;
        return false;
    }
    
    //Copia o arquivo para os setores alocados caso exista espaço 
    fseek(file_to_copy, 0, SEEK_SET);
    for (int i = 0; i < needed_sectors; i++)
    {
        if(i+1 == needed_sectors)//ultimo setor do arquivo
        {
            char copied_sector[file_size%508];
            fread(copied_sector, sizeof(char), file_size%508, file_to_copy);
            fseek(disk, find_offset_sector_data(alocated_sectors[i], boot_record.sector_size, boot_record.reserved_sectors), SEEK_SET);
            fwrite(copied_sector, sizeof(char), file_size%508, disk);
            unsigned int ending_pointer = 0xFFFFFFFF;
            fseek(disk, find_offset_sector_data(alocated_sectors[i], boot_record.sector_size, boot_record.reserved_sectors) + 508, SEEK_SET);
            fwrite(&ending_pointer, sizeof(unsigned int), 1, disk);
        }
        else
        {
            char copied_sector[508];//508 bytes para conteudo (4bytes para ponteiro do pŕoximo bloco)
            fread(copied_sector, sizeof(char), 508, file_to_copy);
            fseek(disk, find_offset_sector_data(alocated_sectors[i], boot_record.sector_size, boot_record.reserved_sectors), SEEK_SET);
            fwrite(copied_sector, sizeof(char), 508, disk);
            fseek(disk, find_offset_sector_data(alocated_sectors[i], boot_record.sector_size, boot_record.reserved_sectors) + 508, SEEK_SET);
            fwrite(&alocated_sectors[i+1], sizeof(unsigned int), 1, disk);
        }
    }
    fclose(file_to_copy);
    return true;
}