/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010,2012,2019 Gustavo Maciel Dias Vieira
 * Copyright © 2010 Rodrigo Rocco Barbieri
 *
 * This file is part of RSFS.
 *
 * RSFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define CLUSTERSIZE 4096
#define FATCLUSTERS 65536
#define DIRENTRIES 128

unsigned short fat[FATCLUSTERS];

typedef struct {
  char buffer[CLUSTERSIZE];
  char open;
  int buffer_pointer;
  int block_pointer;
  int mode;
  int gindex;
} file_iterator;

file_iterator fit[DIRENTRIES];

typedef struct {
  char used;
  char name[25];
  unsigned short first_block;
  int size;
} dir_entry;

dir_entry dir[DIRENTRIES];

int __fs_check_format() {
  // Checagens de Formtação.
  size_t i;
  for (i = 0; i < 32; i++) {
    // Itera pelos primeiros 32 setores da fat checando se estao com o valor corredo.
    if (fat[i] != 3) return 0;
  }
  // Faz o mesmo para o setor 33, que tem que possuir o valor 4 para o unico diretorio do arquivo.
  if (fat[i] != 4) return 0;

  return 1;
}

int __fs_find_file(char *file_name) {
  int alvo = -1;
  for (size_t i = 0; i < DIRENTRIES; i++) {
    // Caso não encontremos o arquivo para leitura retornamos erro.
    if(strcmp(dir[i].name, file_name) == 0 && dir[i].used == 1) {
      alvo = i;     
      break;
    }
  }
  return alvo;
}

// Funcao Auxiliar interna do fs que retorna o proximo setor livre da fat.
unsigned short __fs_next_free_fat() {
  for (size_t i = 33; i < bl_size(); i++) {
    if (fat[i] == 1) return i;
  }
  return -1;
}

// Funcao Auxiliar interna do fs que escreve a fat e o unico dir no arquivo.
void __fs_write_fat_dir_disk() {
  for (size_t i = 0; i < 32; i++) {
    bl_write(i, ((char*) &fat) + i*CLUSTERSIZE);
  }
  bl_write(32,(char*) &dir);
}

int  __fs_flush_fit(int file, int qnt) {
  bl_write(fit[file].block_pointer, fit[file].buffer);
  int livre = __fs_next_free_fat();
  if (livre == -1) return 0;
  fat[fit[file].block_pointer] = livre;
  fit[file].block_pointer = fat[fit[file].block_pointer];
  fat[fit[file].block_pointer] = 2;
  fit[file].buffer_pointer = 0;
  dir[file].size += qnt;
  __fs_write_fat_dir_disk();
  return 1;
}

// Funcao Auxiliar interna do fs que printa a fat guardada em memoria.
void __fs_print_fat(){
  char* buffer = (char *) fat;
  for (size_t i = 0; i < sizeof(fat); i++) {
    if (i % 100 == 0) {
      printf("\n");
    }
    printf("%d ", buffer[i]);
  }
}

// Funcao Auxiliar para printar os fits ativos jkjjjjjjjjjjjjjjjjjjjjjj
void __fs_print_fit() {
  for (size_t i = 0; i < DIRENTRIES; i++) {
    if (fit[i].open == 1) {
      printf("FIT ENCONTRADO: \n");
      printf("Modo: %d, Block Pointer: %d, Buffer Pointer: %d\n", fit[i].mode, fit[i].block_pointer, fit[i].buffer_pointer);
    }
  }
}

int fs_init() {
  //Buffer aponta para fat.
  char* buffer = (char *) fat;
  for (size_t i = 0; i < 32; i++) {
    // Cada chamada de bl_read le um setor inteiro, portanto precisamos somar esse valor ao multiplicador de indice.
    bl_read(i, buffer+(i*SECTORSIZE));
  }

  // Finaliza lendo o Setor do diretorio e carregando na memoria.
  bl_read(32,(char*) dir);

  // Checa se o arquivo lido esta formatado ou não;
  if (!__fs_check_format()) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
  }

  return 1;
}

int fs_format() {
  // Primeiro populamos a fat em memoria, primeiramente os 32 primeiros setores com o valor 3.
  // Depois o setor 33 com o valor 4 de diretorio.
  for (size_t i = 0; i < 32; i++) {
    fat[i] = 3;
  }
  fat[32] = 4;

  // Para o resto da fat ate o bl_size, populamos.
  for (size_t i = 33; i < bl_size(); i++) {
    fat[i] = 1;
  }
  
  // Entao escrevemos a fat no disco.
  for (size_t i = 0; i < 32; i++) {
    bl_write(i, ((char*) &fat) + i*CLUSTERSIZE);
  }

  // Em memória populamos o dir.
  for (size_t i = 0; i < DIRENTRIES; i++) {
    dir[i].used = 0;
    dir[i].name[0] = '\0';
    dir[i].first_block = -1;
    dir[i].size = 0;
  }

  // Entao escrevemos o dir no disco.
  bl_write(32, (char*) &dir);

  return 1;
}

int fs_free() {

  // Checa se o arquivo lido esta formatado ou não;
  if (!__fs_check_format()) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
    return -1;
  }

  int free_blocks = 0;
  for (size_t i = 0; i < bl_size(); i++) {
    if (fat[i] == 1) free_blocks++;
  }
  return free_blocks*CLUSTERSIZE;
}

int fs_list(char *buffer, int size) {

  if (!__fs_check_format()) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
    return 0;
  }

  // Criamos uma string temporaria que ira ser escrita no buffer.
  char temp[38];

  // Por algum motivo precisamos limpar o buffer antes de efetuar o strcat,
  // Pelo que entendemos deveria vir limpa do shell.c, portanto utilizamos esse metodo
  // apesar de ser invasivo.
  buffer[0] = '\0';

  for (size_t i = 0; i < DIRENTRIES; i++) {
    if (dir[i].used == 1) {
      // Para cada arquivo utilizado, printamos seu formato na string temp.
      sprintf(temp, "%s\t\t%d\n", dir[i].name, dir[i].size);
      // Depois concatenamos no buffer.
      strcat(buffer,temp);
    } 
  }

  return 1;
}

int fs_create(char* file_name) {

  if (!__fs_check_format()) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
    return 0;
  }

  int alvo = -1;
  for (size_t i = 0; i < DIRENTRIES; i++) {
    // Procuramos o primeiro local vazio.
    if (dir[i].used == 0 && alvo == -1) alvo = i;
    
    // Caso encontremos um arquivo com o mesmo nome retornamos erro.
    if(strcmp(dir[i].name, file_name) == 0 && dir[i].used == 1) {
      printf("Arquivo já existe!⚠⚠⚠⚠⚠\n");
      return 0;      
    }
  }

  // Se nao achamos espaco vazio retornamos e printamos erro.
  if (alvo == -1) {
    printf("ACABOU O ESPAÇO!⚠⚠⚠⚠⚠\n");
    return 0;
  }

  // Efetuamos a checagem de tamanho de nome de arquivo.
  int tamanho_nome = strlen(file_name);
  if(tamanho_nome > 24) {
    printf("Nome de arquivo muito grande!⚠⚠⚠⚠⚠\n");
    return 0;
  } 

  // Populamos a estrutura de dir com as informacoes passadas.
  strncpy(dir[alvo].name, file_name, tamanho_nome);
  dir[alvo].name[tamanho_nome] = '\0';
  dir[alvo].size = 0;
  dir[alvo].used = 1;
  // Utilizamos a funcao auxiliar para buscar o proximo setor vazio na fat.
  unsigned short target_block = __fs_next_free_fat();
  dir[alvo].first_block = target_block;

  // Escrevemos na fat em memoria o setor ocupado.
  fat[target_block] = 2;
  
  // Finalmente escrevemos no disco a fat.
  __fs_write_fat_dir_disk();
  return 1;
}

int fs_remove(char *file_name) {

  if (!__fs_check_format()) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
    return 0;
  }

  for (size_t i = 0; i < DIRENTRIES; i++) {
    // Procuramos o arquivo fornecido na estrutura de diretorio.
    if(dir[i].used == 1 && strcmp(dir[i].name, file_name) == 0) {
      dir[i].used = 0;
      unsigned short target_block = dir[i].first_block;
      unsigned short new_target;
      do {
        // Utilizamos new_target para iterar pelos blocos do arquivo na fat
        // e modificamos para apontar setor vazio ate chegarmos no 2, que limpamos e saimos do loop.
        new_target = fat[target_block];
        fat[target_block] = 1;
        target_block = new_target; 
      } while (target_block != 2);
      // Escrevemos a fat e dir no disco.
      __fs_write_fat_dir_disk();
      return 1;
    }
  }
  printf("Arquivo não existe!⚠⚠⚠⚠⚠\n");
  return 0;
}

int fs_open(char *file_name, int mode) {

  if (!__fs_check_format()) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
    return -1;
  }

  if (mode == FS_R) {
    int alvo = __fs_find_file(file_name);

    if (alvo == -1) {
      printf("Arquivo inexiste!⚠⚠⚠⚠⚠\n");
      return -1; 
    }

    fit[alvo].block_pointer = dir[alvo].first_block;
    fit[alvo].open = 1;
    fit[alvo].buffer_pointer = 0;
    fit[alvo].mode = mode;
    return alvo;
  }

  // Se chegou aqui eh FS_W
  if (mode == FS_W) {
    int alvo = __fs_find_file(file_name);

    if (alvo != -1) {
      fs_remove(file_name);
    }

    fs_create(file_name);
    alvo = __fs_find_file(file_name);
    fit[alvo].block_pointer = dir[alvo].first_block;
    fit[alvo].mode = mode;
    fit[alvo].buffer_pointer = 0;
    fit[alvo].open = 1;
    fit[alvo].gindex = 0;

    __fs_write_fat_dir_disk();
    return alvo;
  }

  printf("Modo de abertura de arquivo não suportado!⚠⚠⚠⚠⚠");
  return -1;
}

int fs_close(int file)  {

  if (!__fs_check_format()) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
    return 0;
  }
  //precisa checar se o arquivo esta aberto
  //flush no buffer da fit do arquivo em questao
  //limpar as variaveis da fit para o novo uso caso aconteca
  //fecha
  if (fit[file].open != 1) {
    printf("Arquivo não está aberto!⚠⚠⚠⚠⚠");
    return 0;
  }

  // Flush no buffer
  if (fit[file].mode == FS_W) {
    if (fit[file].buffer_pointer != 0) {
      // Pode fragmentação interna? 🤔
      if(!__fs_flush_fit(file, fit[file].buffer_pointer)){
        printf("Não há mais espaço no disco para fechar o arquivo!⚠⚠⚠⚠⚠");
        return 0;
      }
    }
  }
  

  // Limpando variaveis da fit;
  fit[file].block_pointer = 0;
  fit[file].open = 0;
  fit[file].buffer[0] = '\0';
  fit[file].buffer_pointer = 0;
  fit[file].mode = -1;
  fit[file].gindex = 0;
  return 1;
}

int fs_write(char *buffer, int size, int file) {

  if (!__fs_check_format()) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
    return -1;
  }

  if (fit[file].open != 1 || fit[file].mode != FS_W) {
    printf("Arquivo não está aberto ou não está em modo de escrita!⚠⚠⚠⚠⚠");
    return -1;
  }
  
  size_t i;
  for (i = 0; i < size; i++) {
    fit[file].buffer[fit[file].buffer_pointer] = buffer[i];
    fit[file].buffer_pointer++;
    if (fit[file].buffer_pointer == CLUSTERSIZE) {
      if(!__fs_flush_fit(file, fit[file].buffer_pointer)){
        printf("Não há mais espaço no disco!⚠⚠⚠⚠⚠");
        return -1;
      }
    }
  }
  return i;
}

int fs_read(char *buffer, int size, int file) {

  if (!__fs_check_format()) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
    return -1;
  }

  //checar se esta open e modo READ
  //brincar com o buffer da fit
  //tomar cuidado para nao devolver lixo
  //ou seja, o arquivo pode acabar antes da quantidade que o usuario pediu
  //leia apenas ate EOF e mande o usuario burro tomar no cu
  //ate mesmo se vc ja estiver no fim, nao leia nada retorne zero como um chad
  //retorna a quantidade de bytes lidos
  if (fit[file].open != 1 || fit[file].mode != FS_R) {
    printf("Arquivo não está aberto ou não está em modo de leitura!⚠⚠⚠⚠⚠");
    return -1;
  }

  size_t qtd = 0;
  while(qtd < size && qtd < dir[file].size) {
    if (fit[file].block_pointer == 2) {
      break;
    }
    bl_read(fit[file].block_pointer, fit[file].buffer);
    while(fit[file].buffer_pointer < CLUSTERSIZE && qtd < size && fit[file].gindex < dir[file].size) {
      strncpy(buffer+qtd, &fit[file].buffer[fit[file].buffer_pointer], 1);
      qtd++;
      fit[file].buffer_pointer++;
      fit[file].gindex++;
    }

    if (qtd == size || fit[file].gindex == dir[file].size) {
      break;
    }
    
    fit[file].block_pointer = fat[fit[file].block_pointer];
    fit[file].buffer_pointer = 0;
  }
  return qtd;
}

