
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

    // format_disk(disk);
    BootRecord boot_record = read_boot_record(disk);;
    
    copy_file(disk, boot_record, "copy.txt");
    copy_file(disk, boot_record, "another.txt");
    // copy_file(disk, boot_record, "bigfile.txt");
    cout << find_offset_sector_data(0,512,boot_record.reserved_sectors) << endl;
    fclose(disk);
    return 0;
}