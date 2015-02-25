#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "cs402.h"
#include "my402list.h"

void printamt(double bal);
double checkamt(double num);

int main(int argc, char *argv[]){
    FILE *fp = fopen("test.tfile", "r"); 
    if(!fp && argc == 3 && strcmp(argv[1], "sort") == 0) fp = fopen(argv[2], "r"); 
    if(!fp && argc == 2 && strcmp(argv[1], "sort") == 0) fp = stdin;
    if(!fp){
       perror("Error opening file");
       exit(1);
       }

    My402ListElem *p=NULL;
    My402List list;
    memset(&list, 0, sizeof(My402List));
    if(!My402ListInit(&list)){
       perror("Error init list");
       exit(1);
       }

    struct data{
                char type[1];
    		int time;
    		double amount;
    		char discri[30];
		};
    struct data *p_obj;
    int length, c, i;
    char buf [1026], num[13];
    char *start_ptr, *tab_ptr;     
       
    while(fgets(buf, sizeof(buf), fp) != NULL){

          /* * * input * * */

          /*===CODE FROM SLIDE===*/   
          start_ptr = buf;
          c = 0;
          while(1){
                   tab_ptr = strchr(start_ptr, '\t');
	           if(tab_ptr != NULL) *tab_ptr++ = ' ';
	           else break;
                   start_ptr = tab_ptr;
                   c++;
	           }
          /*=====================*/ 
          if(c != 3){
             printf("Wrong input\n");
             exit(1);
             }
          struct data *new = (struct data*)malloc(sizeof(struct data));
          sscanf(buf, "%s%d%s %[^\n]", new->type, &new->time, num, new->discri);
          new->amount = atof(num);

          /* * * malformed cmd * * */

          for(i = 0, c = 0; i <= 13; i++){
              if(c != 0){
                 if(c == 3)
                    if(num[i] != '\0'){
                       printf("Wrong amount\n");
                       exit(1);
                    }
                 c++;
                 }
              if(num[i] == '.') c = 1;
              }

          if(new->type[0] != '-' && new->type[0] != '+'){
             printf("Wrong type\n");
             exit(1);
             } 
          if(new->amount >= 10000000 || new->amount < 0){
             printf("Wrong amount\n");
             exit(1);
             } 
          if(new->time > (int)time(NULL)){
             printf("Wrong time\n");
             exit(1);
             }
          if(new->discri[0] == '\0'){
             printf("Wrong description\n");
             exit(1);
             }
          memset(buf, '\0', sizeof(buf));

          /* * * sort data * * */

          length = My402ListLength(&list);
          if(length == 0) My402ListAppend(&list, new);
          else{
               for(p = My402ListFirst(&list); length > 0; length--, p = My402ListNext(&list, p)){
                   p_obj = p->obj;
                   if(p_obj->time > new->time){
                      My402ListInsertBefore(&list, new, p);
                      break;
                      }
                   if(p_obj->time == new->time){
                      printf("Wrong times\n");
                      exit(1);
                      }
                   if((p_obj->time < new->time) && (length == 1)){
                      My402ListInsertAfter(&list, new, p);
                      break;
                      } 
                   }
               }
          }

     /* * * print * * */

     char t1[4],t2[4],t4[9],cp[26];
     int t3, t5, slen;
     double bal = 0;

     length = My402ListLength(&list); 

     printf("+-----------------+--------------------------+----------------+----------------+\n");
     printf("|       Date      | Description              |         Amount |        Balance |\n");
     printf("+-----------------+--------------------------+----------------+----------------+\n");  

     for(p = My402ListFirst(&list); length > 0; length--, p = My402ListNext(&list, p)){

         p_obj = p->obj;

         /* * * Date * * */

         time_t unixtime = p_obj->time;
         memset(t1, '\0', sizeof(t1));
         memset(t2, '\0', sizeof(t2));
         sscanf(ctime(&unixtime), "%3s %3s %d %s %d", t1, t2, &t3, t4, &t5);
         if(t3 >= 10) printf("| %s %s %d %d ", t1, t2, t3, t5);
         else printf("| %s %s  %d %d ", t1, t2, t3, t5);

         /* * * Description * * */

         slen = strlen(p_obj->discri);
         if(slen <= 24){
            printf("| %s", p_obj->discri);
            for(; slen <= 24; slen++) printf(" ");
            }
         else{
              memset(cp, '\0', sizeof(cp));
              strncpy(cp, p_obj->discri, 24);
              printf("| %s ", cp);
              }

         /* * * Amount * * */

         if(p_obj->type[0] == '-'){ 
            printf("| (");
            printamt(p_obj->amount);
            printf(") ");
            bal = bal - checkamt(p_obj->amount);
            }
         else{ 
              printf("|  ");
              printamt(p_obj->amount);
              printf("  "); 
              bal = bal + checkamt(p_obj->amount);
              } 
 
         /* * * Balance * * */ 

         if(bal < 0) printf("| (");
         else printf("|  ");

         printamt(bal);

         if(bal < 0) printf(") |\n");
         else printf("  |\n"); 
         }

     printf("+-----------------+--------------------------+----------------+----------------+\n");

     My402ListUnlinkAll(&list);
     free(p);

     fclose(fp);

     return(0);
}

double checkamt(double num){
     int i, c = 0;
     char amt[14];

     memset(amt, '\0', sizeof(amt));
     gcvt(num, 13, amt);
     
     for(i = 0; i <= 13; i++){
         if(c != 0){
            if(c > 2) amt[i] = '0';
            c++;
            }
         if(amt[i] == '.') c = 1;
         }

     return atof(amt);
}

void printamt(double m){
     char amt[10];
     int i;

     memset(amt, '0', sizeof(amt));
     gcvt(fabs(m), 9, amt);

     for(i = 0; i <= 9; i++) 
         if(!amt[i]) amt[i] = '0';

     if(fabs(m) >= 1000000) printf("%c,%c%c%c,%c%c%c.%c%c", amt[0],amt[1],amt[2],amt[3],amt[4],amt[5],amt[6],amt[8],amt[9]); 
     else if(fabs(m) >= 100000) printf("  %c%c%c,%c%c%c.%c%c", amt[0],amt[1],amt[2],amt[3],amt[4],amt[5],amt[7],amt[8]);   
     else if(fabs(m) >= 10000) printf("   %c%c,%c%c%c.%c%c", amt[0],amt[1],amt[2],amt[3],amt[4],amt[6],amt[7]); 
     else if(fabs(m) >= 1000) printf("    %c,%c%c%c.%c%c", amt[0],amt[1],amt[2],amt[3],amt[5],amt[6]);

     else if(fabs(m) >= 100) printf("      %c%c%c.%c%c", amt[0],amt[1],amt[2],amt[4],amt[5]); 
     else if(fabs(m) >= 10) printf("       %c%c.%c%c", amt[0],amt[1],amt[3],amt[4]); 
     else printf("        %c.%c%c", amt[0],amt[2],amt[3]); 
}
