
#include "exbau.hpp"


int main(){
    FILE *disk = fopen("test.img", "rb+");    
    if(disk==NULL)
    {
        cout << "Erro ao abrir o arquivo" << endl;
        return 0;
    }

    format_disk(disk);
    fclose(disk);
    return 0;
}