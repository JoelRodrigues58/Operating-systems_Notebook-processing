#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

typedef struct dependencias * Dependencias;

ssize_t readln(int fildes, char *buf, size_t nbyte);

void calculaDependencias(char * cmd, int posicao);

void executaComando(int posicaoComando, int erros);

void controladorFluxo(int posicaoComando, int pipe[2]);

void controlC(int j);

void parseStruct();

void readFile2(char* filename);

int readFile1(char* filename);

int parseLogs();