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
char *buffer;

typedef struct {
  char used;
  char name[25];
  unsigned short first_block;
  int size;
} dir_entry;

dir_entry dir[DIRENTRIES];

unsigned short __fs_next_free_fat() {
  for (size_t i = 33; i < bl_size(); i++) {
    if (fat[i] == 1) return i;
  }
  return -1;
}

void print_fat(){
  buffer = (char *) fat;
  for (size_t i = 0; i < sizeof(fat); i++) {
    if (i % 100 == 0) {
      printf("\n");
    }
    printf("%d ", buffer[i]);
  }
}

int fs_init() {
  //Buffer aponta para fat;
  buffer = (char *) fat;
  for (size_t i = 0; i < 32; i++) {
    bl_read(i, buffer+(i*SECTORSIZE));
  }

  // Checagens de Formtação;
  int formatado = 1;
  size_t i;
  for (i = 0; i < 32; i++) {
    if (fat[i] != 3) formatado = 0;
  }
  if (fat[i] != 4) formatado = 0;
  bl_read(32,(char*) dir);

  if (!formatado) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
    return 0;
  }

  return 1;
}

int fs_format() {
  // Em memoria azucrinamos a fat
  for (size_t i = 0; i < 32; i++) {
    fat[i] = 3;
  }
  fat[32] = 4;
  for (size_t i = 33; i < bl_size(); i++) {
    fat[i] = 1;
  }
  
  // Entao escrevemos a fat no disco :)
  for (size_t i = 0; i < 32; i++) {
    bl_write(i, ((char*) &fat) + i*CLUSTERSIZE);
  }

  // Em memória azucrinamos o dir
  for (size_t i = 0; i < DIRENTRIES; i++) {
    dir[i].used = 0;
    dir[i].name[0] = '\0';
    dir[i].first_block = -1;
    dir[i].size = 0;
  }
  // Entao escrevemos o dir no disco :)
  bl_write(32, (char*) &dir);

  return 1;
}

int fs_free() {
  //printf("Função não implementada: fs_free\n");
  int free_blocks = 0;
  for (size_t i = 0; i < bl_size(); i++) {
    if (fat[i] == 1) free_blocks++;
  }
  return free_blocks*CLUSTERSIZE;
}

int fs_list(char *buffer, int size) {
  //checar se esta sendo usado e imprimir?
  printf("Função não implementada: fs_list\n");
  return 0;
}

int fs_create(char* file_name) {
  int alvo = -1;
  for (size_t i = 0; i < DIRENTRIES; i++) {
    if (dir[i].used == 0 && alvo == -1) alvo = i;
    
    if(strcmp(dir[i].name, file_name) == 0) {
      printf("Arquivo já existe!⚠⚠⚠⚠⚠\n");
      return 0;      
    }
  }

  if (alvo == -1) {
    printf("ACABOU O ESPAÇO!⚠⚠⚠⚠⚠\n");
    return 0;
  }

  int tamanho_nome = strlen(file_name);
  if(tamanho_nome > 24) {
    printf("Nome de arquivo muito grande!⚠⚠⚠⚠⚠\n");
    return 0;
  } 
  strncpy(dir[alvo].name, file_name, tamanho_nome);
  dir[alvo].name[tamanho_nome] = '\0';
  dir[alvo].size = 0;
  dir[alvo].used = 1;
  unsigned short target_block = __fs_next_free_fat();
  
  dir[alvo].first_block = target_block;
  fat[target_block] = 2;
  
  //ESCREVER EM DISCO
  return 1;
}

int fs_remove(char *file_name) {
  for (size_t i = 0; i < DIRENTRIES; i++) {
    if( dir[i].used == 1 && strcmp(dir[i].name, file_name) == 0) {
      dir[i].used = 0;
      unsigned short target_block = dir[i].first_block;
      unsigned short new_target;
      while (target_block != 2) {
        new_target = fat[target_block];
        fat[target_block] = 1;
        target_block = new_target; 
      }
      fat[target_block] = 1;
      //ESCREVER EM DISCO
      return 1;
    }
  }
  printf("Arquivo não existe!⚠⚠⚠⚠⚠\n");
  return 0;
}

int fs_open(char *file_name, int mode) {
  printf("Função não implementada: fs_open\n");
  return -1;
}

int fs_close(int file)  {
  printf("Função não implementada: fs_close\n");
  return 0;
}

int fs_write(char *buffer, int size, int file) {
  printf("Função não implementada: fs_write\n");
  return -1;
}

int fs_read(char *buffer, int size, int file) {
  printf("Função não implementada: fs_read\n");
  return -1;
}

