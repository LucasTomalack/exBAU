
#include "exbau.hpp"
#include <vector>

using namespace std;

int main(){
    FILE *disk = fopen("test2.img", "rb+");    
    if(disk==NULL)
    {
        cout << "Erro ao abrir o arquivo" << endl;
        return 0;
    }

    // format_disk(disk);
    BootRecord boot_record = read_boot_record(disk);
    // copy_file(disk,boot_record, "exbau.hpp");
    read_sector(disk, boot_record, 0,true);
    fclose(disk);
    return 0;
}