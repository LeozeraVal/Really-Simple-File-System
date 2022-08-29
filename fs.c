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
  //printf("Função não implementada: fs_init\n");
  // ANOTACOES
  // MUITO ESTRANHO LER!
  // tirar douglas do grupo 💩
  int formatar = 0;
  fs_format();
  buffer = (char *) fat;
  for (size_t i = 0; i < 16; i++) {
    bl_read(i, buffer+(i*SECTORSIZE));
  }

  size_t i;
  for (i = 0; i < 32; i++) {
    if (fat[i] != 3) formatar = 1;
  }

  if (fat[i] != 4) formatar = 1;
  
  for (size_t i = 33; i < FATCLUSTERS; i++) {
    if (fat[i] < 33) formatar = 1;
  }
  return 1;
  // precisa checar extensivamente cada arquivo do dir?
  //bl_read(16,)




  //return formatar ? fs_format() : 1;
}

int fs_format() {
  //printf("Função não implementada: fs_format\n");
  
  // ANOTACOES
  // COMO PERCORRE FAT?
  // COMO ESCREVE?
  // FORMAT CERTO?

  // Em memoria azucrinamos a fat
  for (size_t i = 0; i < 32; i++) {
    fat[i] = 3;
  }
  fat[32] = 4;
  for (size_t i = 33; i < FATCLUSTERS; i++) {
    fat[i] = 1;
  }
  
  // Entao escrevemos a fat no disco :)
  for (size_t i = 0; i < 32; i++) {
    bl_write(i, (char*) &fat+i*CLUSTERSIZE);
  }

  // Em memória azucrinamos o dir
  for (size_t i = 0; i < DIRENTRIES; i++) {
    dir[i].used = 69;
    dir[i].name[0] = 'C';
    dir[i].first_block = -1;
    dir[i].size = 420;
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
  printf("Função não implementada: fs_list\n");
  return 0;
}

int fs_create(char* file_name) {
  printf("Função não implementada: fs_create\n");
  return 0;
}

int fs_remove(char *file_name) {
  printf("Função não implementada: fs_remove\n");
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

