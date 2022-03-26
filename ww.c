#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define BUFFER_SIZE 5
void wrap(int width,int fd_input,int fd_output);

int isFileOrDir(char *name) {
    struct stat data;
    int error = stat(name, &data);
    if(error) {
        perror(name);
        return 0;
    }

    if(S_ISREG(data.st_mode)) {
        return 1; // it's a file
    }
    if(S_ISDIR(data.st_mode)) {
        return 2; // it's a directory
    }
    return 0;
}

int main(int argc, char*argv[])
{
   if (argc == 1 || argc > 3) {
        return EXIT_FAILURE;
    }
  int width = atoi(argv[1]);
  assert(width > 0);
  
int f = isFileOrDir(argv[2]);

  if (f == 1) {
      int fd = open(argv[2], O_RDONLY);
     if(fd < 0){
    // there was some error in opening the file
        perror(argv[2]);
     exit(EXIT_FAILURE);
     }
    wrap(width,fd,1);
  }
  return EXIT_SUCCESS;
}
void wrap(int fd_input, int fd_output){
  char buffer[BUFFER_SIZE];
   int bytes_read= read(fd_input,buffer, BUFFER_SIZE);
  if(bytes_read < 0){
    // an error occurred
    exit(EXIT_FAILURE);
  }
  if(bytes_read == 0){
    // we have reached the end of the file
    close(fd);
    exit(EXIT_SUCCESS);
  }
