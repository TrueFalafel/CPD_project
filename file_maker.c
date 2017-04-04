#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main(){
    int size = 10000;
    int live = 500000;

    int cells[3][live];
    memset(cells, 0, 3*live);
    int x, y, z;
    int index = 0;
    int found;
    srand(time(NULL));
    for(int i = 0; i < live; i++){
        x = random()%size;
        y = random()%size;
        z = random()%size;
        found = 0;

        for(int j = 0; j < live; j++){
            if(x == cells[0][j]){
                if(y == cells[1][j]){
                    if(z == cells[2][j]){
                        i = i -1;
                        found = 1;
                        break;
                    }
                }
            }
        }
        if(found == 0){
            cells[0][index] = x;
            cells[1][index] = y;
            cells[2][index] = z;
            index++;
        }
    }

    FILE* fd = fopen("InputFiles/s10000e500000.in", "w");
    fprintf(fd, "%d\n", size);
    for(int n = 0; n < live; n++){
        fprintf(fd, "%d %d %d\n", cells[0][n], cells[1][n], cells[2][n]);
    }
    fclose(fd);
    return 0;
}
