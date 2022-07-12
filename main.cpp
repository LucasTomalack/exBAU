#include "exbau.hpp"
#include <stdlib.h>
#include <string.h>
#include <vector>

using namespace std;

int main(int argc, char const *argv[])
{
    if (argc < 2){
        cerr << "./exBAU <volume path>" << endl;
        exit(EXIT_FAILURE);
    }

    FILE *disk = fopen(argv[1], "rb+");
    if(!disk){
        cerr << "Falha ao abrir volume!" << endl;
        exit(EXIT_FAILURE);
    }

    main_menu(disk);
    fclose(disk);
    return 0;
}
