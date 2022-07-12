#include "exbau.hpp"
#include <stdlib.h>
#include <vector>
#include <string.h>

using namespace std;

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
    unsigned int t = 8 * boot_record->sector_size;

    //Remove um setor para o boot record
    unsigned int total_sectors = boot_record->total_sectors -1;

    //Verifica o número de setores que serão mapeados em um setor de bitmap
    unsigned short sector_to_ManagerFree;

    if(total_sectors % t == 0)
        sector_to_ManagerFree = total_sectors / t;
    else
        sector_to_ManagerFree =( total_sectors / t) + 1;

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
    unsigned int offset_bitmap = find_offset_sector(1,boot_record.sector_size);
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

void format_disk (FILE *disk,unsigned number_sectors){
    fseek(disk,0,SEEK_END);
    unsigned long long int file_size;

    BootRecord boot_record;
    boot_record.sector_size = 512;
    boot_record.code = BOOT_CODE;

    if(number_sectors==0){
       file_size = ftell(disk); 
    }
    else file_size = (number_sectors*boot_record.sector_size);
    
    if(file_size==0){
        cerr << "Erro: Arquivo vazio" << endl;
        return;
    }else if(file_size>ftell(disk)){
        cerr << "Maior número de setores que o próprio tamanho do arquivo" << endl;
        return;
    }

    cout << "Formatando... por favor, aguarde!" << endl;
    
    //Calcula o total de setores
    if(file_size%boot_record.sector_size)
        boot_record.total_sectors = (file_size/boot_record.sector_size)+1;
    else
        boot_record.total_sectors = file_size/boot_record.sector_size;
    boot_record.volume_size = file_size;

    if(boot_record.total_sectors<3){
        cerr << "O sistema necessita ao menos 3 setores" << endl;
        return;
    }
    //Cria o bloco de BitMap
    if(!create_Block_BitMap(disk, &boot_record))
    {
        cerr << "Erro ao criar o bloco de BitMap" << endl;
        return;
    }
    //Cria o bloco de dados
    if(!create_Block_DataSection(disk, &boot_record)){
        cerr << "Erro ao criar o bloco de dados" << endl;
        return;
    }
    //Escreve o boot record no disco
    if(!write_boot_record(disk, boot_record)){
        cerr << "Erro ao escrever o boot record" << endl;
        return;
    }
    //cria diretório raiz
    if(alocate_dir(disk, boot_record, 0,"root")!=0){
        cerr << "Erro ao criar o diretório raiz" << endl;
        return;
    }

    cout << "Formatado com sucesso" << endl;

}

unsigned find_offset_sector(unsigned sector,unsigned short sector_size){
    return sector*sector_size;
}

unsigned find_offset_sector_data(unsigned sector,BootRecord boot_record){
    return (boot_record.reserved_sectors+ sector)*boot_record.sector_size;
}

unsigned alocate_new_sector_directory(FILE *disk, BootRecord boot_record,unsigned last_sector){
    unsigned new_sector = find_free_sector(disk, boot_record);
    if(new_sector == -1){
        cerr << "Não há espaço suficiente para alocar o diretório" << endl;
        return -1;
    }
    manage_sector_BitMap(disk, boot_record, new_sector, true);

    //Vai apontar o setor antigo para o novo setor
    unsigned int offset_last_sector_pointer = find_offset_sector_data(last_sector, boot_record) + (boot_record.sector_size-sizeof(unsigned int));
    fseek(disk, offset_last_sector_pointer, SEEK_SET);
    fwrite(&new_sector, sizeof(unsigned int), 1, disk);

    //Vai marcar que aquele setor é o final da "lista"
    unsigned int offset_new_sector_pointer = find_offset_sector_data(new_sector, boot_record) + (boot_record.sector_size-sizeof(unsigned int));
    fseek(disk, offset_new_sector_pointer, SEEK_SET);
    unsigned int next_sector = LAST_QUEUE_SECTOR;
    fwrite(&next_sector, sizeof(unsigned int), 1, disk);

    return new_sector;
}

unsigned find_offset_free_directory(FILE *disk, BootRecord boot_record, unsigned sector_number){
    unsigned offset = find_offset_sector_data(sector_number, boot_record);
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

unsigned find_offset_attribute_file(FILE *disk, BootRecord boot_record,unsigned sector_number_dir, unsigned short pos_file){
    unsigned offset_dir = find_offset_sector_data(sector_number_dir,boot_record);
    return offset_dir + (sizeof(FileFormat) * (pos_file-1));
    
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

unsigned int alocate_dir (FILE *disk, BootRecord boot_record, unsigned int prev_dir_sector, string directoryname)
{
    unsigned int data_section_sectors = boot_record.total_sectors - boot_record.reserved_sectors;
    //acessa o bitmap
    byte_ B;
    //Verifica algum setor livre
    unsigned int available_sector = find_free_sector(disk, boot_record);

    if(directoryname.size()>15){
        cerr << "Nome do diretório muito longo" << endl;
        return -1;
    }

    if(available_sector==-1){
        cerr << "Erro: Disco cheio" << endl;
        return 0;
    }

    //Marca como ocupado
    manage_sector_BitMap(disk, boot_record, available_sector, true);

    unsigned short count_max_directories = (boot_record.sector_size-sizeof(unsigned int))/sizeof(FileFormat);
    
    //marca o ponteiro no final do setor ocupado pelo novo diretório
    fseek(disk, find_offset_sector_data(available_sector, boot_record) + 508, SEEK_SET);
    unsigned int ending_pointer = LAST_QUEUE_SECTOR;
    fwrite(&ending_pointer, sizeof(unsigned int), 1, disk);

    //cria os diretórios para navegação ("." e "..")
    FileFormat dot = new_file_format(".","",0x10, available_sector, 0);
    fseek(disk, find_offset_sector_data(available_sector, boot_record), SEEK_SET);
    fwrite(&dot, sizeof(FileFormat), 1, disk);


    //Só vai criar o diretório ".." se o diretório não for o root
    if(available_sector!=0){
        FileFormat dot2 = new_file_format("..","",0x10, prev_dir_sector, 0);
        fwrite(&dot2, sizeof(FileFormat), 1, disk);
        
        FileFormat new_dir = new_file_format(directoryname.c_str(),"",DIRECTORY_ATTRIBUTE, available_sector, 0);
        alocate_attribute_to_directory(disk, boot_record, prev_dir_sector, &new_dir);
        
        //Vai remover da contagem máximo de arquivos por setor os dois diretórios criados (. e ..)
        count_max_directories -=2;
    }
    //Se não, só remove um
    else count_max_directories--;


    //Vai setar os atributos de todos os diretórios como "vazios", para que possam ser criados novos arquivos
    FileFormat empty_dir = new_file_format("","",LAST_FILE_ATTRIBUTE, 0, 0);
    // unsigned char attribute = LAST_FILE_ATTRIBUTE;
    for(int i=1;i<=count_max_directories;i++){
        fwrite(&empty_dir, sizeof(FileFormat), 1, disk);   
    }
    
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

bool copy_file_to_exBAU(FILE *disk, BootRecord boot_record,  string filename, unsigned int dir_sector){
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
        return false;
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
            fseek(disk, find_offset_sector_data(alocated_sectors[i], boot_record), SEEK_SET);
            fwrite(copied_sector, sizeof(char), file_size%508, disk);
            unsigned int ending_pointer = LAST_QUEUE_SECTOR;
            fseek(disk, find_offset_sector_data(alocated_sectors[i], boot_record) + 508, SEEK_SET);
            fwrite(&ending_pointer, sizeof(unsigned int), 1, disk);
        }
        else
        {
            char copied_sector[508];//508 bytes para conteudo (4bytes para ponteiro do pŕoximo bloco)
            fread(copied_sector, sizeof(char), 508, file_to_copy);
            fseek(disk, find_offset_sector_data(alocated_sectors[i], boot_record), SEEK_SET);
            fwrite(copied_sector, sizeof(char), 508, disk);
            fseek(disk, find_offset_sector_data(alocated_sectors[i], boot_record) + 508, SEEK_SET);
            fwrite(&alocated_sectors[i+1], sizeof(unsigned int), 1, disk);
        }
    }
    fclose(file_to_copy);

    
    
    FileFormat data = new_file_format(file_name.c_str(), file_extension.c_str(), FILE_ATTRIBUTE, alocated_sectors[0], file_size);
    bool a = alocate_attribute_to_directory(disk, boot_record, dir_sector, &data);
    return true;
}

bool copy_file_to_system(FILE *disk, BootRecord boot_record, FileFormat data){
    string name;
    name.append(data.filename);
    name.append(".");
    name.append(data.ext);
    FILE *disk2 = fopen(name.c_str(),"w+");

    if(disk2==NULL){
        cerr << "Erro ao copiar o arquivo" << endl;
        return false;
    }

    unsigned long long size_restant = data.size;

    char c;
    //Variável para economizar ficar realizando os cálculos
    size_t size_char = sizeof(char);

    //Verifica até onde pode fazer a leitura
    unsigned pointer = boot_record.sector_size - sizeof(unsigned int);

    //Responsável por indicar o setor para fazer a leitura
    unsigned next_sector_offset = data.first_sector;

    for(;size_restant>0;){
        //Posiciona o ponteiro no local
        fseek(disk,find_offset_sector_data(next_sector_offset,boot_record),SEEK_SET);
        for(int i=0;i<pointer && size_restant>0;i++,size_restant--){
            fread(&c,size_char,1,disk);
            fwrite(&c,size_char,1,disk2);
        }

        //Recebe o próximo ponteiro para ler        
        next_sector_offset = next_sector(disk,boot_record,next_sector_offset);

        //Se chegou no último setor e não conseguiu ler todo o arquivo, então ocorreu algum erro
        if(next_sector_offset==LAST_QUEUE_SECTOR && size_restant>0){
            cerr << "Algum erro ocorreu ao copiar o arquivo" << endl;
            fclose(disk2);
            return false;
        }
        
    }

    fclose(disk2);
    return true;
}

bool delete_file(FILE *disk, BootRecord boot_record, unsigned offset){
    FileFormat data;
    fseek(disk, offset, SEEK_SET);
    fread(&data, sizeof(FileFormat), 1, disk);

    //Está tentando apagar o diretório raiz
    if(data.first_sector==0){
        return false;
    }

    vector<unsigned int> sectors_to_delete;
    sectors_to_delete.push_back(data.first_sector);


    //Pega os setores do arquivo/diretório
    for(int i=1;;i++){
        unsigned int next_sector_offset = next_sector(disk, boot_record, sectors_to_delete[i-1]);

        //Se não possuir mais setores, sai do loop
        if(next_sector_offset == LAST_QUEUE_SECTOR)
            break;

        //Se ele achar algum setor que está marcado como vazio, possivelmente acessou algum lixo
        if(!check_sector_BitMap(disk, boot_record,next_sector_offset)){
            return false;
        }
        sectors_to_delete.push_back(next_sector_offset);
    }
    
    //Vai liberar os setores alocados
    for(int i=0;i<sectors_to_delete.size();i++){
        manage_sector_BitMap(disk, boot_record, sectors_to_delete[i], false);
    }

    //Deleta o arquivo do diretório
    fseek(disk, offset, SEEK_SET);
    data.attribute = DELETED_ATTRIBUTE;
    data.first_sector=0;
    fwrite(&data, sizeof(FileFormat), 1, disk);

    return true;
}

bool delete_sector(FILE *disk, BootRecord boot_record,unsigned prev_sector, unsigned sector){
    unsigned offset = find_offset_sector_data(sector,boot_record);
    unsigned offset_prev = find_offset_sector_data(prev_sector,boot_record);
    unsigned next_sector_number = next_sector(disk,boot_record,sector);

    unsigned pointer_offset = boot_record.sector_size-sizeof(unsigned int);

    if(prev_sector!=-1){
        fseek(disk,offset_prev+pointer_offset,SEEK_SET);
        fwrite(&next_sector_number,sizeof(unsigned),1,disk);
    }
    manage_sector_BitMap(disk,boot_record,sector,false);
    
    return true;
}

unsigned int next_sector(FILE *disk, BootRecord boot_record, unsigned int sector_number)
{
    unsigned int next_sector;
    unsigned int offset = find_offset_sector_data(sector_number, boot_record);
    unsigned short offset_pointer = boot_record.sector_size - sizeof(unsigned int);
    fseek(disk, offset + offset_pointer, SEEK_SET);
    fread(&next_sector, sizeof(unsigned int), 1, disk);
    return next_sector;
}

void read_sector(FILE *disk, BootRecord boot_record, unsigned sector_dir,unsigned short pos_file){
    //Responsável por apontar o ponteiro para o final setor para ver o número do próximo setor
    unsigned short offset_pointer = boot_record.sector_size - sizeof(unsigned int);
    if(pos_file==0){
        //Posiciona o ponteiro
        fseek(disk,find_offset_sector_data(sector_dir,boot_record),SEEK_SET);

        //Verifica o número máximo de arquivos/diretórios em um setor
        unsigned short directories_count_max = (boot_record.sector_size-sizeof(unsigned int))/sizeof(FileFormat);
        for(int i =0;i<directories_count_max;i++){
            FileFormat file_format;
            fread(&file_format, sizeof(FileFormat), 1, disk);
            
            //Se for um arquivo deletado, ele ignora o arquivo
            if(file_format.attribute==DELETED_ATTRIBUTE){
                continue;
            }
            else if(file_format.attribute==LAST_FILE_ATTRIBUTE){
                break;
            }

            if(file_format.attribute==FILE_ATTRIBUTE)
            {
                cout << "Nome: " << file_format.filename << "." << file_format.ext << endl;
                cout << "Tamanho: " << file_format.size << " bytes" << endl;
            }
            else cout << "Nome: " << file_format.filename << " | ";
            cout << "Tipo: " << (file_format.attribute == FILE_ATTRIBUTE ? "Arquivo" : "Diretório") << endl;
            // cout << "Setor inicial: " << file_format.first_sector << endl;
            cout << endl;
        }
        unsigned next_sector_number = next_sector(disk,boot_record,sector_dir);
        if(next_sector_number!=LAST_QUEUE_SECTOR)
            return read_sector(disk,boot_record,next_sector_number,pos_file);
        
        else return;
    }
    else{
        unsigned offset_file = find_offset_attribute_file(disk, boot_record,sector_dir, pos_file);
        FileFormat data;
        //Lê o arquivo
        fseek(disk, offset_file, SEEK_SET);
        fread(&data, sizeof(FileFormat), 1, disk);

        if(data.attribute==DELETED_ATTRIBUTE){
            return;
        }

        //Verifica até onde pode fazer a leitura do seto
        unsigned max_read_per_sector = boot_record.sector_size-sizeof(unsigned int);

        //Verifica o tamanho do arquivo que falta ler
        unsigned long long restant_size = data.size;

        unsigned next_sector_offset = data.first_sector;

        char c;
        size_t size_char = sizeof(char);
        while(true){
            //Posiciona no setor
            fseek(disk,find_offset_sector_data(next_sector_offset,boot_record),SEEK_SET);
            
            //Faz a leitura de cada caractere e imprime
            for(int i =0 ;i<max_read_per_sector && restant_size>0;i++,restant_size--){
                fread(&c,size_char,1,disk);
                cout << c;
            }

            //Se chegar ao fina ele encerra a leitura
            if(restant_size==0){
                break;
            }

            next_sector_offset = next_sector(disk,boot_record,next_sector_offset);

            //Se não possuir mais setores para ler, também encerra
            if(next_sector_offset==LAST_QUEUE_SECTOR){
                break;
            }

        }   
        cout << endl;
        return;
    }
}

FileFormat find_format_dir(FILE *disk, BootRecord boot_record, const char *name, unsigned sector_dir){
    fseek(disk, find_offset_sector_data(sector_dir, boot_record), SEEK_SET);
    FileFormat file_format;
    do
    {
        fread(&file_format, sizeof(FileFormat), 1, disk); 
    } while ((strcmp(file_format.filename, name)!=0));
    
    return file_format;
}

unsigned short find_pos_file(FILE *disk, BootRecord boot_record, const char *name, unsigned sector_dir){
    fseek(disk, find_offset_sector_data(sector_dir, boot_record), SEEK_SET);
    FileFormat file_format;
    unsigned short pos=0;
    do
    {
        fread(&file_format, sizeof(FileFormat), 1, disk);
        pos++;
    } while ((strcmp(file_format.filename, name)!=0));
    
    return pos;
}

void main_menu(FILE *disk){
    // FORMATADOR (PARAMETRO: TAMANHO EM SETORES DO ARQUIVO)
    BootRecord boot_record;
    bool is_formated;
    system("clear");
    while (true)
    {
        boot_record = read_boot_record(disk); 
        is_formated = (boot_record.code == BOOT_CODE);

        cout << "============== Sistema de Arquivos exBAU ==============" << endl;
        cout << "Menu Principal:" << endl;
        cout << "0 - Sair" << endl;
        cout << "1 - Formatar volume" << endl;
        if (is_formated)
            cout << "2 - Navegar no sistema" << endl;
        short option;
        cin >> option;
        switch (option)
        {
        case 0:
            exit(EXIT_SUCCESS);
            break;
        
        case 1:
            system("clear");
            int setores;
            cout << "Para formatar o volume inteiro digite 0.\nDigite quantos setores(512B) o volume possui: ";
            cin >> setores;
            format_disk(disk, setores); 
            break;
        case 2:
            if(!is_formated){
                cerr << "Não é possível navegar em um volume não formatado!";
                break;
            }
            navigation_menu(disk, boot_record);
            // navegar nos diretórios
        }
    }
    
}

void navigation_menu(FILE *disk, BootRecord boot_record){
    unsigned int current_dir = 0; // root dir
    while (true)
    {
        cout << "\n\n";
        cout << "============== Sistema de Arquivos exBAU ==============" << endl;
        cout << "Menu de navegação:" << endl;
        cout << "0 - Sair" << endl;
        cout << "1 - Listar diretório atual" << endl;
        cout << "2 - Exibir arquivo" << endl;
        cout << "3 - Acessar subdiretório" << endl;
        cout << "4 - Copiar arquivo para o diretório atual" << endl;
        cout << "5 - Copiar arquivo para o disco" << endl;
        cout << "6 - Criar subdiretório no diretório atual" << endl;
         
        short option;
        cin >> option;
        switch (option)
        {
            case 0:
                return;
                break;
        
            case 1:
                read_sector(disk, boot_record, current_dir, 0);
                break;

            case 2: {
                string arq;
                cout << "Digite o nome do arquivo da listagem que deseja exibir (sem a extensão): ";
                cin >> arq;
                unsigned short pos = find_pos_file(disk, boot_record, arq.c_str(), current_dir);
                cout << "\nConteúdo do arquivo:\n" << endl;
                read_sector(disk, boot_record, current_dir, pos);
                cout << endl;
                break;
            }
        
            case 3:{ 
                string dir;
                cout << "Digite o nome do diretório da listagem que deseja acessar: ";
                cin >> dir;
                FileFormat next_dir = find_format_dir(disk, boot_record, dir.c_str(), current_dir);
                current_dir = next_dir.first_sector;
                system("clear");
                break;
            }
            case 4:{
                string filename;
                cout << "Digite o nome do arquivo que deseja copiar: " ;
                cin >> filename;
                if(!copy_file_to_exBAU(disk, boot_record, filename, current_dir))
                    cerr << "Falha ao copiar arquivo!" << endl;
                break;
            }
            case 5:{
                string to_copy;
                cout << "Digite o nome do arquivo que deseja copiar (sem a extensão): ";
                cin >> to_copy;
                FileFormat file = find_format_dir(disk, boot_record, to_copy.c_str(), current_dir);
                if(!copy_file_to_system(disk, boot_record, file))
                    cerr << "Erro ao copiar arquivo!" << endl;
                break;
            }
            case 6:{
                string new_dir_name;
                cout << "Digite o nome do novo diretório: ";
                cin >> new_dir_name;
                if(alocate_dir(disk, boot_record, current_dir, new_dir_name) == 0)
                    cerr << "Erro ao criar novo diretório!" << endl;
                break;
            }
        }
    }
}