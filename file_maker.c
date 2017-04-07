#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main(){
    int size = 500;
    int live = 200000;

    int cells[3][live];
    memset(cells, 0, 3*live);
    int x, y, z;
    int index = 0;
    int found;
	int i,j,n;
    srand(time(NULL));
    for(i = 0; i < live; i++){
		x = random()%size;
        y = random()%size;
        z = random()%size;
        found = 0;

        for( j = 0; j < live; j++){
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

	char title[100];
	/*sprintf(title,"InputFiles/s%de%d.in", size, live);
    FILE* fd = fopen(title, "w");
    fprintf(fd, "%d\n", size);*/
	sprintf(title,"InputFiles/s1000e%d.in", live);
    FILE* fd = fopen(title, "w");
    fprintf(fd, "%d\n", size*2);
    for( n = 0; n < live; n++){
        fprintf(fd, "%d %d %d\n", cells[0][n], cells[1][n], cells[2][n]);
    }
    fclose(fd);
    return 0;
}
