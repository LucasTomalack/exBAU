
#include "exbau.hpp"


int main(){
    FILE *disk = fopen("test.img", "rb+");    
    if(disk==NULL)
        return 0;

    format_disk(disk);
    return 0;
}