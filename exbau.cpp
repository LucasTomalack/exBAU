#include "exbau.hpp"
#include <vector>
#include <string.h>

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
    //Verifica quantos setores da seção de dados pode ser mapeado em um setor de bitmap
    int t = 8 * boot_record->sector_size;

    //Remove um setor para o boot record
    int total_sectors = boot_record->total_sectors -1;

    //Verifica o número de setores que serão mapeados em um setor de bitmap
    int sector_to_ManagerFree= round(total_sectors/t);

    //Se o disco for muito pequeno a fórmula pode dar erro
    //Caso seja o caso, automaticamente será setado um setor para o gerenciamento de espaço livre
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

void manage_sector_BitMap(FILE *disk, BootRecord boot_record, int sector_number, bool new_value){

    //Pega o bit do setor de bitmap
    unsigned short bit_position = sector_number%8;
   
    //Recebe o byte da posição do setor de bitmap
    byte_ b = get_byte_sector_BitMap(disk, boot_record, sector_number);
    //Altera o bit do setor de bitmap
    b[bit_position] = new_value;

    //Verifica o offset do setor
    unsigned seek_position = find_offset_sector(1, boot_record.sector_size);
    unsigned sector_position = seek_position + (sector_number/8);

    //Volta ao setor de bitmap e escreve o byte modificado
    fseek(disk, sector_position, SEEK_SET);
    fputc(b.to_ulong(), disk);
    
    return;   
}

unsigned int find_free_sector(FILE *disk, BootRecord boot_record){
    unsigned int offset_bitmap = find_offset_bitmap();
    byte_ b; 
    
    //Auxiliar para verificar se não está acessando um setor que não existe
    unsigned int max_sectors_data = boot_record.total_sectors - boot_record.reserved_sectors;

    //Posiciona o ponteiro no setor de bitmap
    fseek(disk, offset_bitmap, SEEK_SET);
    for(int i=0; i<max_sectors_data; i++){
        unsigned c = fgetc(disk);
        //Caso um byte seja igual a F, significa que todos os bits estão "ativados" (ocupados)
        if(c == 0xFF){
            continue;
        }
        b = c;
        //Vai buscar o bit que está livre
        for(int j=0; j<8; j++){
            if(b[j] == false){
                return i*8+j;
            }
        }
    }
    return -1;
}

byte_ get_byte_sector_BitMap(FILE *disk,BootRecord boot_record, int sector_number){
    //Verifica o offset do setor do bitmap
    unsigned seek_position = find_offset_sector(1, boot_record.sector_size);
    //Verifica o offset do byte que possui o dado do setor
    unsigned sector_position = seek_position + (sector_number/8);
    //Coloca o ponteiro no byte que possui o dado do setor
    fseek(disk, sector_position, SEEK_SET);
    byte_ b(fgetc(disk));
    return b;
}

bool check_sector_BitMap(FILE *disk, BootRecord boot_record, int sector_number){
    byte_ b = get_byte_sector_BitMap(disk, boot_record, sector_number);
    //Verifica o bit do setor de bitmap
    unsigned short bit_position = sector_number%8;
    return b[bit_position];
}

bool create_Block_DataSection(FILE *disk,  BootRecord *boot_record){
    //Verifica quantos setores o disco pode usar para a área de dados
    unsigned int sector_DataSection = boot_record->total_sectors - boot_record->reserved_sectors;

    //Calcula a posição do setor de dados
    int seek_position = find_offset_sector(boot_record->reserved_sectors, boot_record->sector_size);  

    //Criando o bloco de dados
    fseek(disk, seek_position, SEEK_SET);

    for(int i=0; i<sector_DataSection; i++){
        for(int j=1; j<=boot_record->sector_size; j++){
            //Insere o byte zerado
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
    //Calcula o total de setores
    if(file_size%boot_record.sector_size)
        boot_record.total_sectors = (file_size/boot_record.sector_size)+1;
    else
        boot_record.total_sectors = file_size/boot_record.sector_size;
    boot_record.volume_size = file_size;

    if(boot_record.total_sectors<0){
        printf("O sistema necessita ao menos 3 setores\n");
        return;
    }
    //Cria o bloco de BitMap
    create_Block_BitMap(disk, &boot_record);
    //Cria o bloco de dados
    create_Block_DataSection(disk, &boot_record);
    //Escreve o boot record no disco
    write_boot_record(disk, boot_record);
    //cria diretório raiz
    alocate_dir(disk, boot_record, 0);
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

unsigned alocate_new_sector_directory(FILE *disk, BootRecord boot_record,unsigned last_sector){
    unsigned new_sector = find_free_sector(disk, boot_record);
    if(new_sector == -1){
        printf("Não há espaço suficiente para alocar o diretório\n");
        return -1;
    }
    manage_sector_BitMap(disk, boot_record, new_sector, true);

    //Vai apontar o setor antigo para o novo setor
    unsigned int offset_last_sector_pointer = find_offset_sector_data(last_sector, boot_record.sector_size, boot_record.reserved_sectors) + (boot_record.sector_size-sizeof(unsigned int));
    fseek(disk, offset_last_sector_pointer, SEEK_SET);
    fwrite(&new_sector, sizeof(unsigned int), 1, disk);

    //Vai marcar que aquele setor é o final da "lista"
    unsigned int offset_new_sector_pointer = find_offset_sector_data(new_sector, boot_record.sector_size, boot_record.reserved_sectors) + (boot_record.sector_size-sizeof(unsigned int));
    fseek(disk, offset_new_sector_pointer, SEEK_SET);
    unsigned int next_sector = LAST_QUEUE_SECTOR;
    fwrite(&next_sector, sizeof(unsigned int), 1, disk);

    return new_sector;
}

unsigned find_offset_free_directory(FILE *disk, BootRecord boot_record, unsigned sector_number){
    unsigned offset = find_offset_sector_data(sector_number, boot_record.sector_size, boot_record.reserved_sectors);
    fseek(disk, offset, SEEK_SET);
    unsigned int max_entries = (boot_record.sector_size-sizeof(unsigned int))/sizeof(FileFormat);

    for(int i=0; i<max_entries; i++){
        FileFormat file_format;
        fread(&file_format, sizeof(FileFormat), 1, disk);
        if(file_format.attribute == LAST_FILE_ATTRIBUTE || file_format.attribute==DELETED_ATTRIBUTE){
            return offset+i*sizeof(FileFormat);
        }
    }
    return -1;
}

bool alocate_attribute_to_directory(FILE *disk, BootRecord boot_record, unsigned int sector_number, FileFormat *file_format){
    if(check_sector_BitMap(disk, boot_record, sector_number)==false){
        cout << "Setor não está ocupado" << endl;
        return false;
    }

    //Verifica algum lugar vazio no diretório
    unsigned int offset_free_directory = find_offset_free_directory(disk, boot_record, sector_number);
    if(offset_free_directory == -1){
        unsigned next_sector_offset = next_sector(disk, boot_record, sector_number);
        //Necessita alocar um novo setor para o diretório
        if(next_sector_offset==LAST_QUEUE_SECTOR){
            next_sector_offset = alocate_new_sector_directory(disk, boot_record, sector_number);
            //Não possui armazenamento para mais arquivos
            if(next_sector_offset==-1){
                return false;
            }

            return alocate_attribute_to_directory(disk, boot_record, next_sector_offset, file_format);
        }
        return alocate_attribute_to_directory(disk, boot_record, next_sector_offset, file_format);
    
    }
    //Escreve os atributos do arquivo no diretório
    fseek(disk, offset_free_directory, SEEK_SET);
    fwrite(file_format, sizeof(FileFormat), 1, disk);


    return true;
}

FileFormat new_file_format(const char *filename, const char *ext, char attribute, unsigned int first_sector, unsigned long long int size)
{
    FileFormat format;
    for (int i = 0; i < 15; i++)
        format.filename[i] = 0x00;
    for (int i = 0; i < 4; i++)
        format.ext[i] = 0x00;
    strcpy(format.filename, filename);
    strcpy(format.ext, ext);
    format.attribute = attribute;
    format.first_sector = first_sector;
    format.size = size;
    return format;
}

unsigned int alocate_dir (FILE *disk, BootRecord boot_record, unsigned int prev_dir_sector)
{
    unsigned int data_section_sectors = boot_record.total_sectors - boot_record.reserved_sectors;
    //acessa o bitmap
    unsigned int available_sector = 0;
    byte_ B;
    //Verifica algum setor livre
    unsigned int sector_position = find_free_sector(disk, boot_record);


    if(sector_position==-1){
        printf("Erro: Disco cheio\n");
        return 0;
    }
    manage_sector_BitMap(disk, boot_record, sector_position, true);
    
    //marca o ponteiro no final do setor ocupado pelo novo diretório
    fseek(disk, find_offset_sector_data(available_sector, boot_record.sector_size, boot_record.reserved_sectors) + 508, SEEK_SET);
    unsigned int ending_pointer = LAST_QUEUE_SECTOR;
    fwrite(&ending_pointer, sizeof(unsigned int), 1, disk);

    //cria os diretórios para navegação ("." e "..")
    FileFormat dot = new_file_format(".","",0x10, available_sector, 0);
    FileFormat dot2 = new_file_format("..","",0x10, prev_dir_sector, 0);
    fseek(disk, find_offset_sector_data(available_sector, boot_record.sector_size, boot_record.reserved_sectors), SEEK_SET);
    fwrite(&dot, sizeof(FileFormat), 1, disk);

    //Só vai criar o diretório ".." se o diretório não for o root
    if(prev_dir_sector>0)
        fwrite(&dot2, sizeof(FileFormat), 1, disk);



    return available_sector;
}

//aloca um arquivo no bitmap e retorna um vetor com seus setores
vector<unsigned int> alocate_file(FILE *disk, BootRecord boot_record, unsigned long long int file_size)
{
    unsigned int data_section_sectors = boot_record.total_sectors - boot_record.reserved_sectors;
    unsigned int needed_sectors=file_size/508;
    if(file_size%508)
        needed_sectors++;
    //acessa o bitmap
    vector<unsigned int> available_sectors;
    byte_ B;

    //Verifica os setores livres para o arquivo
    for(int i =0;i<needed_sectors;i++){
        //Verifica algum setor livre
        unsigned int sector_position = find_free_sector(disk, boot_record);
        if(sector_position==-1){
            return available_sectors;
        }
        available_sectors.push_back(sector_position);
        manage_sector_BitMap(disk, boot_record, sector_position, true);

    }

    return available_sectors;
}

bool copy_file(FILE *disk, BootRecord boot_record,  string filename){
    //Vai separar o nome com a extensão do arquivo
    string file_name = filename.substr(0, filename.find_last_of("."));
    string file_extension = filename.substr(filename.find_last_of(".")+1);

    if(file_name.size()>15){
        cerr << "Nome do arquivo muito longo!" << endl;
        return false;
    }

    //abre o arquivo a ser copiado
    FILE *file_to_copy = fopen(filename.c_str(), "r");
    if(file_to_copy == NULL){
        cerr << "Erro ao abrir arquivo!" << endl;
    }

    //verifica se há espaço disponível para o arquivo no sistema de arquivos
    fseek(file_to_copy, 0, SEEK_END);
    unsigned long long int file_size = ftell(file_to_copy);
    
    vector<unsigned int> alocated_sectors = alocate_file(disk, boot_record, file_size);
    unsigned int needed_sectors=file_size/508;

    if(file_size%508)
        needed_sectors++;
    if(alocated_sectors.size() < needed_sectors){
        cerr << "Não há espaço suficiente para alocar o arquivo" << endl;
        //Vai liberar os setores alocados
        for(int i=0;i<alocated_sectors.size();i++){
            manage_sector_BitMap(disk, boot_record, alocated_sectors[i], false);
        }
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
            unsigned int ending_pointer = LAST_QUEUE_SECTOR;
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

    
    
    FileFormat data = new_file_format(file_name.c_str(), file_extension.c_str(), FILE_ATTRIBUTE, alocated_sectors[0], file_size);
    bool a = alocate_attribute_to_directory(disk,boot_record,0,&data);
    cout << a << endl;
    return true;
}

unsigned int next_sector(FILE *disk, BootRecord boot_record, unsigned int sector_number)
{
    unsigned int next_sector;
    unsigned int offset = find_offset_sector_data(sector_number, boot_record.sector_size, boot_record.reserved_sectors);
    unsigned short offset_pointer = boot_record.sector_size - sizeof(unsigned int);
    fseek(disk, offset + offset_pointer, SEEK_SET);
    fread(&next_sector, sizeof(unsigned int), 1, disk);
    return next_sector;
}

void read_sector(FILE *disk, BootRecord boot_record, unsigned int sector_number, bool directory){
    //Verifica o offset do setor e acessa o arquivo/diretório
    unsigned int offset = find_offset_sector_data(sector_number, boot_record.sector_size, boot_record.reserved_sectors);
    fseek(disk, offset, SEEK_SET);

    //Responsável por apontar o ponteiro para o final setor para ver o número do próximo setor
    unsigned short offset_pointer = boot_record.sector_size - sizeof(unsigned int);
    if(directory){
        int directories_count_max = boot_record.sector_size/sizeof(FileFormat);
        for(int i =0;i<directories_count_max;i++){
            FileFormat file_format;
            fread(&file_format, sizeof(FileFormat), 1, disk);
            
            //Se for um arquivo deletado, ele ignora o arquivo
            if(file_format.attribute==DELETED_ATTRIBUTE){
                continue;
            }
            //Condição temporária (deve ser mudada posteriormente)
            else if(file_format.attribute==0){
                continue;
            }
            cout << "Nome: " << file_format.filename << "." << file_format.ext << endl;
            if(file_format.attribute==FILE_ATTRIBUTE)
                cout << "Tamanho: " << file_format.size << " bytes" << endl;
            cout << "Tipo: " << (file_format.attribute == FILE_ATTRIBUTE ? "Arquivo" : "Diretório") << endl;
            cout << "Setor inicial: " << file_format.first_sector << endl;
            cout << endl;
        }

    }
    else{
        char content;
        //Lê byte a byte do arquivo e imprime na tela
        for(int i =0;i<offset_pointer;i++){
            fread(&content, sizeof(char), 1, disk);
            cout << content;
        }
        cout << endl;
    }

    
    //Posiciona o ponteiro no final do setor para ver o número do próximo setor
    fseek(disk, offset + offset_pointer, SEEK_SET);
    
    //Lê o número do próximo setor
    unsigned int next_sector;
    fread(&next_sector, sizeof(unsigned int), 1, disk);

    //Se tiver outro setor, chama a função novamente para ler o próximo setor
    if(next_sector!=LAST_QUEUE_SECTOR){
        read_sector(disk, boot_record, next_sector, directory);
    }
}