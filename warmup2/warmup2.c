#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include "my402list.h"
#include "cs402.h"

#define handle_error_en(en, msg) do{ errno=en;perror(msg);exit(EXIT_FAILURE);}while(0)

FILE *fp;
int t,sigcome,srvwait,arvover,num_token,totaltn,droptn,totalpk,droppk;//t:tfile
double startm;
char buf [1026];
struct sigaction act;
My402List Q1, Q2, E;
pthread_t arvthread, tknthread, srvthread;
pthread_mutex_t m;
pthread_cond_t cv=PTHREAD_COND_INITIALIZER;
sigset_t new;

typedef struct Parameters{
    double lambda;
    double mu;
    double r;
    int B;
    int P;
    int num_arr;
    } parame;

typedef struct Packet{
    int num;
    double r_servicetm;//requested
    int num_token;
    double arrtm;
    double m_intarrtm;//measured
    double entQ1tm;
    double leaveQ1tm;
    double entQ2tm;
    double leaveQ2tm;
    double ejectStm;
    double servicetm;
    double systemtm;
    } packet;

void handler(int sig){
    int s;
    sigcome=1;
    if(arvover!=1){
       s=pthread_cancel(tknthread);
       if(s!=0) handle_error_en(s,"pthread_cancel");
       s=pthread_cancel(arvthread);
       if(s!=0) handle_error_en(s,"pthread_cancel");
       if(srvwait==1) s=pthread_cancel(srvthread);
       if(s!=0) handle_error_en(s,"pthread_cancel");
       }
}

void *arv(void *ptr){
    parame *parame1;
    parame1=(parame*)ptr;
    double tm, prvtm, integ, fract;
    char *start_ptr, *tab_ptr;  
    struct timespec slptm;
    struct timeval tv;   
    My402ListElem *p=NULL;
    packet *k;
    arvover=0;
    droppk=0;

    if(t==1){
       fgets(buf,sizeof(buf),fp);
       start_ptr=buf;
       while(1){
          tab_ptr=strchr(start_ptr,'\t');
	  if(tab_ptr != NULL) *tab_ptr++=' ';
          else break;
          start_ptr=tab_ptr;
          }
       sscanf(buf,"%lf%d%lf",&parame1->lambda,&parame1->P,&parame1->mu); 
       parame1->lambda=1/(parame1->lambda/1000);
       parame1->mu=1/(parame1->mu/1000);
       }

    //sleep    
    fract=modf(1/parame1->lambda,&integ);
    slptm.tv_sec=integ;
    slptm.tv_nsec=fract*1000000000;
    nanosleep(&slptm,NULL);

    for(totalpk=1;totalpk<=parame1->num_arr;totalpk++){
        //drop packet
        if(parame1->P > parame1->B){
           gettimeofday(&tv,NULL);
           tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;
           printf("%012.3lfms: packet p%d arrives, needs %d tokens, dropped\n",tm*1000,totalpk,parame1->P);
           droppk++;
           if(t==1){
              fgets(buf,sizeof(buf),fp);
              start_ptr=buf;
              while(1){
                 tab_ptr=strchr(start_ptr,'\t');
	         if(tab_ptr!=NULL) *tab_ptr++=' ';
                 else break;
                 start_ptr=tab_ptr;
                 }
              sscanf(buf,"%lf%d%lf",&parame1->lambda,&parame1->P,&parame1->mu); 
              parame1->lambda=1/(parame1->lambda/1000);
              parame1->mu=1/(parame1->mu/1000);
              }  
           continue;
           }

        //packet arrives
        packet *new=(packet*)malloc(sizeof(packet));  
        new->r_servicetm=1/parame1->mu;
        new->num_token=parame1->P;
        new->num=totalpk; 
        gettimeofday(&tv,NULL);
        new->arrtm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;
        if(totalpk==1) prvtm=0.0;  
        new->m_intarrtm=new->arrtm-prvtm; 
        pthread_testcancel();//xxxxxx
        pthread_mutex_lock(&m);//======
        printf("%012.3lfms: p%d arrives, needs %d tokens, inter-arrival time = %.3lfms\n",new->arrtm*1000,new->num,new->num_token,new->m_intarrtm*1000);
        prvtm=new->arrtm;   

        //enter Q1
        My402ListAppend(&Q1,new);
        gettimeofday(&tv,NULL);
        new->entQ1tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm; 
        pthread_mutex_unlock(&m);//======  
        pthread_testcancel();//xxxxxx    
        pthread_mutex_lock(&m);//======          
        printf("%012.3lfms: p%d enter Q1\n",new->entQ1tm*1000,new->num);
        
        p=My402ListFirst(&Q1);
        k=p->obj;  
        if(num_token>=k->num_token){
           //leave Q1
           num_token=num_token-parame1->P;
           My402ListUnlink(&Q1,p);
           gettimeofday(&tv, NULL);
           k->leaveQ1tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;
           tm=k->leaveQ1tm-k->entQ1tm;     

           //enter Q2
           if(My402ListLength(&Q2)==0){
              My402ListAppend(&Q2,k);
              pthread_cond_signal(&cv);
              }
           else My402ListAppend(&Q2,k);
           gettimeofday(&tv,NULL);
           k->entQ2tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;
           pthread_mutex_unlock(&m);//======  
           pthread_testcancel();//xxxxxx    
           pthread_mutex_lock(&m);//======  
           printf("%012.3lfms: p%d leaves Q1, time in Q1 = %.3lfms, token bucket now has %d token...by arv\n",k->leaveQ1tm*1000,k->num,tm*1000,num_token);
           printf("%012.3lfms: p%d enter Q2\n",k->entQ2tm*1000,k->num);
           }     
        pthread_mutex_unlock(&m);//======
        //get service time
        gettimeofday(&tv,NULL);
        tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm-new->arrtm;

        if(t==1){
           fgets(buf,sizeof(buf),fp);
           start_ptr=buf;
           while(1){
              tab_ptr=strchr(start_ptr,'\t');
	      if(tab_ptr!=NULL) *tab_ptr++=' ';
              else break;
              start_ptr=tab_ptr;
              }
           sscanf(buf,"%lf%d%lf",&parame1->lambda,&parame1->P,&parame1->mu); 
           parame1->lambda=1/(parame1->lambda/1000);
           parame1->mu=1/(parame1->mu/1000);
           }          

        //sleep   
        tm=(1/parame1->lambda)-tm;
        fract=modf(tm,&integ);
        slptm.tv_sec=integ;
        slptm.tv_nsec=fract*1000000000;
        nanosleep(&slptm,NULL);       
        }

    while(My402ListLength(&Q1)!=0){
        pthread_mutex_lock(&m);//======
        p=My402ListFirst(&Q1);
        k=p->obj;  
        if(num_token>=k->num_token){
           //leave Q1
           num_token=num_token-k->num_token;
           My402ListUnlink(&Q1, p);
           gettimeofday(&tv,NULL);
           k->leaveQ1tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;
           tm=k->leaveQ1tm-k->entQ1tm;                    

           //enter Q2
           if(My402ListLength(&Q2)==0){
              My402ListAppend(&Q2, k);
              pthread_cond_signal(&cv);
              }
           else My402ListAppend(&Q2, k);
           gettimeofday(&tv,NULL);
           k->entQ2tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;
           pthread_mutex_unlock(&m);//======  
           pthread_testcancel();//xxxxxx    
           pthread_mutex_lock(&m);//======  
           printf("%012.3lfms: p%d leaves Q1, time in Q1 = %.3lfms, token bucket now has %d token\n",k->leaveQ1tm*1000,k->num,tm*1000,num_token);
           printf("%012.3lfms: p%d enter Q2\n",k->entQ2tm*1000,k->num);
           }
        pthread_mutex_unlock(&m);//======
        nanosleep(&slptm,NULL);  
        }         

    arvover=1;
    pthread_exit(0);
}

void *tkn(void *ptr){
    parame *parame1;
    parame1=(parame*)ptr;
    double tm, fract, integ;
    struct timespec slptm;
    struct timeval tv;
    My402ListElem *p=NULL;
    packet *k;
    num_token=0;
    totaltn=0;
    droptn=0;

    while(!arvover){
        //sleep
        fract=modf(1/parame1->r,&integ);
        slptm.tv_sec=integ;
        slptm.tv_nsec=fract*1000000000;       
        nanosleep(&slptm,NULL); 

        gettimeofday(&tv,NULL);
        tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;

        //drop
        if(num_token==parame1->B){
           printf("%012.3lfms: token t%d arrives, dropped\n",tm*1000,++totaltn);
           droptn++;
           continue;
           }    

        //token arrives
        pthread_testcancel();//xxxxxx
        pthread_mutex_lock(&m);//======
        printf("%012.3lfms: token t%d arrives, token bucket now has %d token",tm*1000,++totaltn,++num_token);
        if(totaltn!=1) printf("s\n"); 
        else printf("\n");

        if(My402ListLength(&Q1)!=0 && arvover==0){
           p=My402ListFirst(&Q1);
           k=p->obj;  
           if(num_token>=k->num_token){
              //leave Q1
              num_token=num_token-k->num_token;
              My402ListUnlink(&Q1,p);
              gettimeofday(&tv, NULL);
              k->leaveQ1tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;
              tm=k->leaveQ1tm-k->entQ1tm;   

              //enter Q2
              if(My402ListLength(&Q2)==0){
                 My402ListAppend(&Q2,k);
                 pthread_cond_signal(&cv);  
                 }
              else My402ListAppend(&Q2,k);
              gettimeofday(&tv,NULL);
              k->entQ2tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;
              pthread_mutex_unlock(&m);//======
              pthread_testcancel();//xxxxxx
              pthread_mutex_lock(&m);//======
              printf("%012.3lfms: p%d leaves Q1, time in Q1 = %.3lfms, token bucket now has %d token\n",k->leaveQ1tm*1000,k->num,tm*1000,num_token);
              printf("%012.3lfms: p%d enter Q2\n",k->entQ2tm*1000,k->num);   
              }
           }
        pthread_mutex_unlock(&m);//======
        }  

    pthread_exit(0);
}

void *srv(void *ptr){
    parame *parame1;
    parame1=(parame*)ptr;
    int i=0;
    double tm,integ,fract;
    struct timespec slptm;
    struct timeval tv;
    My402ListElem *p=NULL;
    packet *k;
    act.sa_handler=handler;
    sigaction(SIGINT,&act,NULL);
    pthread_sigmask(SIG_UNBLOCK,&new,NULL);

    while(i<parame1->num_arr){
        pthread_mutex_lock(&m);
        while(My402ListLength(&Q2)==0 && arvover==0){
              srvwait=1;
              pthread_cond_wait(&cv, &m);
              }
        if(arvover==1 && My402ListLength(&Q2)==0) pthread_exit(0);
        srvwait=0;

        //leave Q2
        p=My402ListFirst(&Q2);
        k=p->obj;
        My402ListUnlink(&Q2,p);
        gettimeofday(&tv,NULL);
        k->leaveQ2tm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;
        tm=k->leaveQ2tm-k->entQ2tm;
        if(sigcome==1) pthread_exit(0); 
        printf("%012.3lfms: p%d begin service at S, time in Q2 = %.3lfms\n",k->leaveQ2tm*1000,k->num,tm*1000);
        pthread_mutex_unlock(&m); 

        //sleep
        fract=modf(k->r_servicetm,&integ);
        slptm.tv_sec=integ;
        slptm.tv_nsec=fract*1000000000;       
        nanosleep(&slptm,NULL);        

        //eject 
        i++;
        gettimeofday(&tv,NULL);
        k->ejectStm=tv.tv_sec+(tv.tv_usec/1000000.0)-startm;
        k->servicetm=k->ejectStm-k->leaveQ2tm; 
        k->systemtm=k->ejectStm-k->arrtm; 
        printf("%012.3lfms: p%d departs from S, service time = %.3lfms, time in system = %.3lfms\n",k->ejectStm*1000,k->num,k->servicetm*1000,k->systemtm*1000);
        My402ListAppend(&E,k);

        if(sigcome==1) pthread_exit(0);
        }

    pthread_exit(0);
}

int main(int argc, char *argv[]){
    int i=argc,j,l;
    double intarrtm=0,servicetm=0,systemtm=0,timeinQ1=0,timeinQ2=0,tminsrv=0,emutm=0,sqsystm=0;
    struct timeval startv;
    parame parame1={0.5, 0.35, 1.5, 10, 3, 20};//default
    My402ListElem *p=NULL;
    packet *k;   

    memset(&Q1,0,sizeof(My402List));
    memset(&Q2,0,sizeof(My402List));
    memset(&E,0,sizeof(My402List));
    My402ListInit(&Q1);
    My402ListInit(&Q2);
    My402ListInit(&E);

    if(i!=1){
       for(i=(argc-1)/2,j=1,t=0;i>0;i--,j=j+2){
           if(strcmp(argv[j],"-lambda")==0) parame1.lambda=atof(argv[j+1]);
           else if(strcmp(argv[j],"-mu")==0) parame1.mu=atof(argv[j+1]);
           else if(strcmp(argv[j],"-r")==0) parame1.r=atof(argv[j+1]);
           else if(strcmp(argv[j],"-B")==0) parame1.B=atoi(argv[j+1]);
           else if(strcmp(argv[j],"-P")==0) parame1.P=atoi(argv[j+1]);
           else if(strcmp(argv[j],"-n")==0) parame1.num_arr=atoi(argv[j+1]);
           else if(strcmp(argv[j],"-t")==0){
                   t=1;
                   fp=fopen(argv[j+1],"r"); 
                   if(!fp) fp=fopen("tsfile.tfile","r"); 
                   if(!fp){
                      perror("Error opening file");
                      exit(1);
                      }
                   printf("Emulation Parameters:\n");
                   printf("    tsfile = %s\n",argv[j+1]);
                   printf("    r = %f\n",parame1.r);
                   printf("    B = %d\n",parame1.B);
                   fgets(buf,sizeof(buf),fp);
                   sscanf(buf,"%d",&parame1.num_arr);
                   printf("    number to arrive = %d\n\n",parame1.num_arr);                   
                   }
           }
         }

    if(parame1.lambda<0.1) parame1.lambda=0.1;
    if(parame1.mu<0.1) parame1.mu=0.1;
    if(parame1.r<0.1) parame1.r=0.1;

    if(t==0){
       printf("Emulation Parameters:\n");
       printf("    lambda = %f\n",parame1.lambda);
       printf("    mu = %f\n",parame1.mu);
       printf("    r = %f\n",parame1.r);
       printf("    B = %d\n",parame1.B);
       printf("    P = %d\n",parame1.P);
       printf("    number to arrive = %d\n\n",parame1.num_arr);
       }

    gettimeofday(&startv,NULL);
    startm=startv.tv_sec+(startv.tv_usec/1000000.0);
    printf("%012.3lfms: emulation begins\n",startm-startm);

    sigcome=0;
    sigemptyset(&new);
    sigaddset(&new,SIGINT);
    pthread_sigmask(SIG_BLOCK,&new,NULL);    
    pthread_mutex_init(&m,NULL);
    pthread_create(&arvthread,0,arv,(void*)&parame1);
    pthread_create(&tknthread,0,tkn,(void*)&parame1);
    pthread_create(&srvthread,0,srv,(void*)&parame1);
    pthread_join(tknthread,0);
    pthread_join(arvthread,0);
    pthread_cond_signal(&cv);
    pthread_join(srvthread,0);
    pthread_mutex_destroy(&m);

    l=My402ListLength(&Q1);
    if(l!=0){
       for(i=0,p=My402ListFirst(&Q1);l>0;i++,l--,p=My402ListNext(&Q1,p)){
           k=p->obj;
           intarrtm=(intarrtm*i+k->m_intarrtm)/(i+1);
           }
       My402ListUnlinkAll(&Q1);
       } 

    l=My402ListLength(&Q2);
    if(l!=0){
       for(p=My402ListFirst(&Q2);l>0;i++,l--,p=My402ListNext(&Q2,p)){
           k=p->obj;
           intarrtm=(intarrtm*i+k->m_intarrtm)/(i+1);
           }
       My402ListUnlinkAll(&Q2);
       }  

    l=My402ListLength(&E);
    if(l!=0){
       for(j=0,p=My402ListFirst(&E);l>0;i++,j++,l--,p=My402ListNext(&E,p)){
           k=p->obj;
           intarrtm=(intarrtm*i+k->m_intarrtm)/(i+1);
           servicetm=(servicetm*j+k->servicetm)/(j+1);
           systemtm=(systemtm*j+k->systemtm)/(j+1);
           sqsystm=(sqsystm*j+pow(k->systemtm,2))/(j+1);
           timeinQ1=timeinQ1+(k->leaveQ1tm-k->entQ1tm);
           timeinQ2=timeinQ2+(k->leaveQ2tm-k->entQ2tm);
           tminsrv=tminsrv+k->servicetm;
           } 
        p=My402ListLast(&E);
        k=p->obj;
        emutm=k->ejectStm;
        My402ListUnlinkAll(&E);
        } 

    printf("\nStatistics:\n\n");
    printf("    average packet inter-arrival time = %.06gs\n",intarrtm);
    printf("    average packet service time = %.6gs\n\n",servicetm);
    if(emutm!=0){
       printf("    average number of packets in Q1 = %.6g\n",timeinQ1/emutm);
       printf("    average number of packets in Q2 = %.6g\n",timeinQ2/emutm);
       printf("    average number of packets at S = %.6g\n\n",tminsrv/emutm);
       }
    else{
       printf("    average number of packets in Q1 = N/A (no completed packet)\n");
       printf("    average number of packets in Q2 = N/A (no completed packet)\n");
       printf("    average number of packets at S = N/A (no completed packet)\n\n");         
         }
    printf("    average time a packet spent in system = %.6gs\n",systemtm);
    printf("    standard deviation for time spent in system = %.6g\n\n",sqrt(sqsystm-pow(systemtm,2)));
    if(totaltn!=0) printf("    token drop probability = %.6g\n",(double)droptn/(double)totaltn);
    else printf("    token drop probability = N/A (no packet arrived at this facility)\n");
    if(totalpk!=0) printf("    packets drop probability = %.6g\n\n",(double)droppk/(double)totalpk);
    else printf("    packets drop probability = N/A (no packet arrived at this facility)\n\n");

    return(0);
}
