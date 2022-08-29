/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010,2011,2019 Gustavo Maciel Dias Vieira
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
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define MAX_STR 256
#define MAX_ARG 32
#define COPY_BUFFER_SIZE 10

void format();
void list();
void create(char *file);
void fremove(char *file);
void copy(char *file1, char *file2);
void copyf(char *file1, char *file2);
void copyt(char *file1, char *file2);

int main(int argc, char **argv) {
  char *image;
  int size;
  char linha[MAX_STR];
  char *args[MAX_ARG + 1];
  char *token;
  int i, tam;

  size = -1;
  if (argc >= 2 && argc <= 3) {
    image = argv[1];
    if (argc > 2) {
      size = (atoi(argv[2]) * 1024 * 1024) / SECTORSIZE;
    }
  } else {
    printf("Uso: %s imagem [tamanho]\n", argv[0]);
    printf("Onde: imagem é o arquivo contendo a imagem do disco.\n");
    printf("      tamanho (opcional) é o tamanho da imagem em MB.\n");
    exit(0);
  }

  if (!bl_init(image, size)) {
    exit(0);
  }
  printf("Arquivo de imagem %s aberto.\n", image);
  printf("Tamanho %d setores (%d bytes).\n", bl_size(), bl_size() * SECTORSIZE);
  
  if (!fs_init()) {
    exit(0);
  }

  while (1) {
    printf("> ");
    linha[0] = '\0';
    fgets(linha, MAX_STR, stdin);
    tam = strlen(linha);
    if (tam > 0 && linha[tam - 1] == '\n') {
      linha[tam - 1] = '\0';
    }

    i = 0;
    token = strtok(linha, " ");
    while (token != NULL && i < MAX_ARG) {
      args[i] = token;
      i++;
      token = strtok(NULL, " ");
    }
    args[i] = NULL;
    
    if (args[0] == NULL) {
      continue;
    }

    if (!strcmp(args[0], "exit")) {
      exit(EXIT_SUCCESS);
    } else if (!strcmp(args[0], "format")) {
      format();
    } else if (!strcmp(args[0], "list")) {
      list();
    } else if (!strcmp(args[0], "create")) {
      if (i == 2) {
	create(args[1]);
      } else {
	printf("Uso: create <file>\n");
      }
    } else if (!strcmp(args[0], "remove")) {
      if (i == 2) {
	fremove(args[1]);
      } else {
	printf("Uso: remove <file>\n");
      }
    } else if (!strcmp(args[0], "copy")) {
      if (i == 3) {
	copy(args[1], args[2]);
      } else {
	printf("Uso: copy <file1> <file2>\n");
      }
    } else if (!strcmp(args[0], "copyf")) {
      if (i == 3) {
	copyf(args[1], args[2]);
      } else {
	printf("Uso: copyf <real_file> <file>\n");
      }
    } else if (!strcmp(args[0], "copyt")) {
      if (i == 3) {
	copyt(args[1], args[2]);
      } else {
	printf("Uso: copyt <file> <real_file>\n");
      }
    } else {
      printf("Comando inválido\n");
    }
  }
}

void format() {
  if (fs_format()) {
    printf("Formatação concluída. %d bytes livres.\n", fs_free());
  }
}

void list() {
  char buffer[4096];
  if (fs_list(buffer, 4096)) {
    printf("%s", buffer);
    printf("%d bytes livres.\n", fs_free());
  }
}

void create(char *file) {
  fs_create(file);
}

void fremove(char *file) {
  fs_remove(file);
}

void copy(char *file1, char *file2) {
  int fd1, fd2;
  char buffer[COPY_BUFFER_SIZE];
  int read;

  if ((fd1 = fs_open(file1, FS_R)) == -1) {
    return;
  }

  if ((fd2 = fs_open(file2, FS_W)) == -1) {
    fs_close(fd1);
    return;
  }
  while ((read = fs_read(buffer, COPY_BUFFER_SIZE, fd1)) > 0) {
    if (fs_write(buffer, read, fd2) != read) {
      fs_close(fd1);
      fs_close(fd2);
      return;
    }
  }

  fs_close(fd1);
  fs_close(fd2);
}

void copyf(char *file1, char *file2) {
  int fd2;
  char buffer[COPY_BUFFER_SIZE];
  FILE *stream;
  int read;

  stream = fopen(file1, "r");
  if (stream == NULL) {
    perror("Abrindo arquivo real para cópia (leitura)");
    return;
  }

  if ((fd2 = fs_open(file2, FS_W)) == -1) {
    fclose(stream);
    return;
  }

  while ((read = fread(buffer, sizeof(char), COPY_BUFFER_SIZE, stream)) > 0) {
    if (fs_write(buffer, read, fd2) != read) {
      fclose(stream);
      fs_close(fd2);
      return;
    }
  }

  fclose(stream);
  fs_close(fd2);
}

void copyt(char *file1, char *file2) {
  int fd1;
  char buffer[COPY_BUFFER_SIZE];
  FILE *stream;
  int read;

  if ((fd1 = fs_open(file1, FS_R)) == -1) {
    return;
  }

  stream = fopen(file2, "w+");
  if (stream == NULL) {
    perror("Abrindo arquivo real para cópia (escrita)");
    fs_close(fd1);
    return;
  }

  while ((read = fs_read(buffer, COPY_BUFFER_SIZE, fd1)) > 0) {
    if (fwrite(buffer, sizeof(char), read, stream) != read) {
      perror("Escrevendo arquivo real");
      fs_close(fd1);
      fclose(stream);
      return;
    }
  }

  fs_close(fd1);
  fclose(stream);
}
