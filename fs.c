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
} file_iterator;

file_iterator fit[DIRENTRIES];

typedef struct {
  char used;
  char name[25];
  unsigned short first_block;
  int size;
} dir_entry;

dir_entry dir[DIRENTRIES];

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

// Funcao Auxiliar interna do fs que printa a fat guardada em memoria.
// void print_fat(){
//   buffer = (char *) fat;
//   for (size_t i = 0; i < sizeof(fat); i++) {
//     if (i % 100 == 0) {
//       printf("\n");
//     }
//     printf("%d ", buffer[i]);
//   }
// }

int fs_init() {
  //Buffer aponta para fat.
  char* buffer = (char *) fat;
  for (size_t i = 0; i < 32; i++) {
    // Cada chamada de bl_read le um setor inteiro, portanto precisamos somar esse valor ao multiplicador de indice.
    bl_read(i, buffer+(i*SECTORSIZE));
  }

  // Checagens de Formtação.
  int formatado = 1;
  size_t i;
  for (i = 0; i < 32; i++) {
    // Itera pelos primeiros 32 setores da fat checando se estao com o valor corredo.
    if (fat[i] != 3) formatado = 0;
  }
  // Faz o mesmo para o setor 33, que tem que possuir o valor 4 para o unico diretorio do arquivo.
  if (fat[i] != 4) formatado = 0;

  // Finaliza lendo o Setor do diretorio e carregando na memoria.
  bl_read(32,(char*) dir);

  if (!formatado) {
    printf("Sistema de arquivo não formatado!⚠⚠⚠⚠⚠\n");
    return 0;
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
  int free_blocks = 0;
  for (size_t i = 0; i < bl_size(); i++) {
    if (fat[i] == 1) free_blocks++;
  }
  return free_blocks*CLUSTERSIZE;
}

int fs_list(char *buffer, int size) {
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
  printf("Função não implementada: fs_open\n");
  //Dependendo do modo: pode ser read ou write
  //caso for read, precisa checar a existencia do arquivo 
  //caso for write, checar se ja existe, se nao existe vc cria ele 
  //abre
  //preenche o fit com o número certin

  return -1;
}

int fs_close(int file)  {
  printf("Função não implementada: fs_close\n");
  //precisa checar se o arquivo esta aberto
  //flush no buffer da fit do arquivo em questao
  //limpar as variaveis da fit para o novo uso caso aconteca
  //fecha
  return 0;
}

int fs_write(char *buffer, int size, int file) {
  printf("Função não implementada: fs_write\n");
  //checar se esta open e modo WRITE
  //brincar com o buffer da fit
  //retorna quantidade de bytes escritos
  return -1;
}

int fs_read(char *buffer, int size, int file) {
  printf("Função não implementada: fs_read\n");
  //checar se esta open e modo READ
  //brincar com o buffer da fit
  //tomar cuidado para nao devolver lixo
  //ou seja, o arquivo pode acabar antes da quantidade que o usuario pediu
  //leia apenas ate EOF e mande o usuario burro tomar no cu
  //ate mesmo se vc ja estiver no fim, nao leia nada retorne zero como um chad
  //retorna a quantidade de bytes lidos
  return -1;
}

