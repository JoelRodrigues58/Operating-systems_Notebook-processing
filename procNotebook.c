#include "procNotebook.h"
int nComandos;

struct dependencias{
     char* comando;
     int* depende;
     char* texto;
     int pos;
};

Dependencias *global;

int ** pipes;

pid_t * pids;

int nComandosExec;

int nControladoresExec;

ssize_t readln(int fildes, char *buf, size_t nbyte){
     char aux;

     int c=0, n;

     while( ( n = read (fildes, &aux, 1) ) > 0  && aux!='\n'){
          buf[c]=aux;
          c++;
     }
     buf[c]='\0';
     return c; 

}

void calculaDependencias(char * cmd, int posicao){

     char *comand = malloc(sizeof(char)*(strlen(cmd)+1));

     strcpy(comand,cmd);

     char *cmdaux = strtok (comand," $|");

     int dependencia = atoi(cmdaux);

     global[posicao-dependencia]->depende[(global[posicao-dependencia]->pos)]=posicao;

     global[posicao-dependencia]->pos +=1;     

}


void printGlobal(){

  printf("\n***********************************\n");
  printf("*********** ESTRUTURA *************\n");
  printf("***********************************\n");

     for(int i=0; i<nComandos;i++){
          printf("     Comando %d: %s\n",i,global[i]->comando);
         for(int j=0;j<global[i]->pos;j++){
               printf("        Dependencia %d: %d\n",j,global[i]->depende[j]);
         }
     }

  printf("\n----- Processos em Execução -----\n");
     for(int i=0; i<nComandos;i++){
          printf("|   PidExec %d: %d             |\n",i,pids[i]);
     }


     for(int i=nComandos; i<nComandos*2;i++){
          printf("|   PidControl %d: %d          |\n",i,pids[i]);
     }
  printf("---------------------------------\n");

  printf("\n***********************************\n");
  printf("***********************************\n");
}


void executaComando(int posicaoComando, int erros){

     int i=0;
     
     char * cmdaux;

     int flag=0;

     char *comand = malloc(sizeof(char)*(strlen(global[posicaoComando]->comando)+1));

     strcpy(comand,global[posicaoComando]->comando);

     char**args_exec=malloc(sizeof(char*)*10);

     cmdaux = strtok (comand," ");

          if(strlen(cmdaux)==1){ 
               flag = 1; 
          }

          while (cmdaux != NULL){
               cmdaux = strtok (NULL, " ");
          
               if(cmdaux!=NULL){
                    args_exec[i]= malloc(sizeof(char)*(strlen(cmdaux)+1));
                    strcpy(args_exec[i],cmdaux);
                    i++;
               }
          }

           args_exec[i]=NULL; 

          if(!flag) { 
               
               dup2(pipes[posicaoComando][0],0);
               close(pipes[posicaoComando][1]);
          }
          
          close(pipes[posicaoComando+nComandos][0]);

          dup2(pipes[posicaoComando+nComandos][1],1);

          dup2(erros,2);

          int r = execvp(args_exec[0],&args_exec[0]); //Só irá retornar caso dê erro

          exit(r);

}

void controladorFluxo(int posicaoComando, int pipe[2]){

     char *buf = malloc(sizeof(char)*1024);

     int read_c=0;

     int read_parcial=0;

     char* j;

     char * str_pidControl;

     asprintf(&j,"%d",posicaoComando);

     close(pipes[posicaoComando+nComandos][1]);

     int fk = fork();

     if(fk==0){

          close(pipe[0]);
                    
          asprintf(&str_pidControl,"%d",getpid());
          
          write(pipe[1],str_pidControl,strlen(str_pidControl)+1);

          close(pipe[1]);

          int fd = open(strcat(j,".log"), O_RDWR | O_CREAT | O_TRUNC, 0666);

          char * buf_principal = malloc(sizeof(char)*1024);

          int contador=0;

          while( (read_parcial = read(pipes[posicaoComando+nComandos][0],buf,1024) ) > 0){

                read_c+=read_parcial;

                buf_principal= realloc(buf_principal,read_c*sizeof(char));

                strcpy(buf_principal+contador,buf);

                contador+=read_c;

                for(int i=0; i<global[posicaoComando]->pos;i++){

                    write(pipes[global[posicaoComando]->depende[i]][1],buf_principal,read_c);

               }    
              
          }

               write(fd,global[posicaoComando]->texto,strlen(global[posicaoComando]->texto));
               write(fd,"\n",1);
               write(fd,global[posicaoComando]->comando,strlen(global[posicaoComando]->comando));
               write(fd,"\n",1);
               write(fd,">>>\n",4);
               printf("BUUF %s\n",buf_principal );
               write(fd,buf_principal,read_c);
               write(fd,"<<<\n",4);

          exit(0);
     }
 
     else {

          for(int i=0; i<global[posicaoComando]->pos;i++){

               close(pipes[global[posicaoComando]->depende[i]][1]);
          }
     }

}

void controlC(int j){
     for(int i=0; i<nComandosExec;i++){
        kill(pids[i],SIGKILL);
     }

     for(int i=nComandos; i<(nControladoresExec+nComandos);i++){
        kill(pids[i],SIGKILL);
     }

     kill(getpid(),SIGKILL);
}


void parseStruct(){

    int pidExec[nComandos][2];

    char * str_pidExec;

    int pidControl[nComandos][2];

    char *buf = malloc(sizeof(char)*1024);

    int erros = open("erros.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

     for(int i=0; i<nComandos; i++ ){

      pipe(pidExec[i]);

      pipe(pidControl[i]);

          int fk = fork();

               if(fk==0){

                  close(pidExec[i][0]);
                    
                  asprintf(&str_pidExec,"%d",getpid());
          
                  write(pidExec[i][1],str_pidExec,strlen(str_pidExec)+1);

                  close(pidExec[i][1]);

                  executaComando(i,erros);
                    
               }

               else {  
                    close(pidExec[i][1]);

                    read(pidExec[i][0],buf,1024);

                    close(pidExec[i][0]);

                    pids[i]=atoi(buf);

                    nComandosExec++;

                    controladorFluxo(i,pidControl[i]);

                    close(pidControl[i][1]);

                    read(pidControl[i][0],buf,1024);

                    close(pidControl[i][0]);

                    pids[i+(nComandos)]=atoi(buf);   
                   
                    nControladoresExec++;
               }
     }

     int status=0, i=0,  read_c=0;

     close(erros);

     erros = open("erros.txt", O_RDWR | O_CREAT, 0666);

     for(int j=0; j<nComandos*2;j++){
          wait(&status);

            if(WEXITSTATUS(status)!=0){
              controlC(i);
            }
     }

     if((read_c = read(erros,buf,1024)) > 0){
              controlC(i);
      }

      close(erros);
}

void readFile2(char* filename){

    int fd = open(filename, O_RDWR | O_CREAT, 0666);

    char *buf = malloc(sizeof(char)*1024);
    int read_c;
    int i=0, flag=0;

    for(int j=0; j<nComandos; j++){
          global[j]=malloc(sizeof(struct dependencias));
    }

    while( (read_c = readln(fd,buf,1024) ) > 0){

         if(!strcmp(">>>",buf)){
              flag=1;
         }

         else if(!strcmp("<<<",buf)){
              flag=0;
         }

         else if(!flag){

              if(buf[0]=='$') {

                   global[i]->comando = malloc(sizeof(char)*(read_c+1));

                   strcpy(global[i]->comando,buf);

                   global[i]->comando[read_c]= '\0';

                   global[i]->pos=0;

                   global[i]->depende = malloc(sizeof(int)*nComandos);

                        if(buf[1]!=' '){

                             calculaDependencias(global[i]->comando,i);

                        }
                   i++;                    
              }

              else {

                   global[i]->texto = malloc(sizeof(char)*(read_c+1));

                   strcpy( global[i]->texto,buf);

                   global[i]->texto[read_c]='\0';

              }
         }         
    }
}


int readFile1(char* filename){

     int fd = open(filename, O_RDWR | O_CREAT, 0666);

     int numerocmd=0;
     char *buf = malloc(sizeof(char)*1024);
     int read_c;

     while( (read_c = readln(fd,buf,1024) ) > 0 ){

          if(buf[0]=='$') {
              numerocmd++;
          
          }
     }
     return numerocmd;
}


int parseLogs(){

     int aux = open("auxiliar.nb", O_RDWR | O_CREAT | O_TRUNC, 0666);

     int r;
     char *buf = malloc(sizeof(char)*1024);
     char* cmd;
     for(int j=0; j<nComandos; j++){

          asprintf(&cmd,"%d",j);

          int files = open(strcat(cmd,".log"), O_RDWR, 0666);

               while((r = read(files,buf,1024)) > 0){
                  buf[r]='\0';

                    write(aux,buf,r);
               }      
     }


     return 0;
}

int main(int argc, char *argv[]){

     signal(SIGINT,controlC);

     signal(SIGSEGV,controlC);    

     for(int j= 1; j<argc ; j++){

            int fk2= fork();

              if(fk2==0){

                int nr= readFile1(argv[j]);

                nComandosExec=nControladoresExec=0;

                nComandos = nr;

                pipes= malloc(sizeof(int*)*(nComandos*2));

                pids= malloc(sizeof(pid_t)*(nComandos*2));

                     for(int i=0; i<nComandos*2;i++){

                          pipes[i]=malloc(sizeof(int)*2);

                          pipe(pipes[i]);

                          pids[i] = 0;
                     }

                global = malloc(sizeof(struct dependencias)*nComandos);

                readFile2(argv[j]);

                printGlobal();

                parseStruct();

                printGlobal();

                int flag=1;

                flag = parseLogs();

                if(!flag){

                    int fk = fork();
                   
                     if(fk==0) execlp("mv","mv","auxiliar.nb",argv[j],NULL);
                 }
              }   

              else {
                wait(&fk2);

                int rm = fork();

                  if (rm == 0){

                    execlp("rm","rm","*.log",NULL);

                  }
                  
                  else wait(&rm);

              }
      }




     return 0;
}



