/* Save Bald Elite best scores to the homebrew SD directory.
 * Never block gameplay if the card is write protected or directory is absent.
 */
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include "save.h"
#define FILEPATH "sdmc:/3ds/bald_elite_3ds/scores.txt"
void be_save_load(BeGame *game){
    FILE *f=fopen(FILEPATH,"r");
    if(!f)return;
    char signature[32]={0};
    if(fscanf(f,"%31s",signature)!=1||strcmp(signature,"BALDELITE3DS1")!=0){
        fclose(f);return;
    }
    int data[BE_CANDIDATES][2],complete=1;
    for(int i=0;i<BE_CANDIDATES;i++)for(int m=0;m<2;m++){
        if(fscanf(f,"%d",&data[i][m])!=1 || data[i][m]<0 ||
           data[i][m]>1000000000){complete=0;break;}
    }
    fclose(f);
    if(complete)memcpy(game->best,data,sizeof(data));
}
void be_save_write(const BeGame *game){
    (void)mkdir("sdmc:/3ds/bald_elite_3ds",0777);
    FILE *f=fopen(FILEPATH,"w");
    if(!f)return;
    fprintf(f,"BALDELITE3DS1\n");
    for(int i=0;i<BE_CANDIDATES;i++){
        fprintf(f,"%d %d\n",game->best[i][0],game->best[i][1]);
    }
    fclose(f);
}
