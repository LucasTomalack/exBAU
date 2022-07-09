
#include "exbau.hpp"
#include <vector>

using namespace std;

int main(){
    FILE *disk = fopen("test.img", "rb+");    
    if(disk==NULL)
    {
        cout << "Erro ao abrir o arquivo" << endl;
        return 0;
    }

    format_disk(disk);
    BootRecord boot_record = read_boot_record(disk);;
    cout << find_offset_sector_data(0,512,boot_record.reserved_sectors) << endl;
    fclose(disk);
    return 0;
}