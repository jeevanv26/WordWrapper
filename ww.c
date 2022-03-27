#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFFER_SIZE 5
void wrap(int width,int fd_input,int fd_output);
int isFileOrDir(char *name);


int main(int argc, char*argv[])
{
   if (argc == 1 || argc > 3) {
        return EXIT_FAILURE;
    }
  int width = atoi(argv[1]);
  assert(width > 0);
  
  // direc
  if(argc == 2) {
     char *dirName = argv[2];
     struct dirent *file;
     DIR *dir = opendir(dirName);
     
     chdir(dirName);
     int fileLength;
     
     if (dir) {
        while ( (file = readdir(dir)) != NULL) {
           fileLength = strlen(file->d_name);
           char *fileName = malloc(sizeof(char) * fileLength + 1);
           strcpy(fileName, file->d_name);
           
           if ( !((strncmp(".", fileName, 1) == 0) || (strncmp("wrap.", fileName, 5) == 0)) ) {
              int fd = open(fileName, O_RDONLY);
              char *wrapped = malloc(sizeof(char) * 6 + fileLength);
              strcpy(wrapped, "wrap.");
              strcat(wrapped, fileName);
              
              int newfd = open(wrapped, O_WRONLY | O_TRUNC | O_CREAT, 0666);
              wrap(width, fd, newfd);
              free(wrapped);
              close(fd);
              close(newfd);
           }
           free(fileName);
        }
        closedir(dir);
     }
    // wrap(width,0,1);
  }
  else{
    int f = isFileOrDir(argv[2]);
      // file
      if (f == 1) {
        int fd = open(argv[2], O_RDONLY);
        if(fd < 0){
          // there was some error in opening the file
        perror(argv[2]);
        exit(EXIT_FAILURE);
        }
     wrap(width,fd,1);
    }
  }
  return EXIT_SUCCESS;
}
void wrap(int width, int fd_input, int fd_output){
  char buffer[BUFFER_SIZE];
   int bytes_read= read(fd_input,buffer, BUFFER_SIZE);
   int spaces = 0;
   int newlines =0;
   int index = 0;
   int wordLength =0;
   int currentPosition = 0;
   char * word = (char*)malloc(10*(char));
   char space = ' ';
   char nextline ='\n';
   int bytes_written = 0;
   boolean failure = false;
  while(bytes_read != 0){
    if(bytes_read < 0){
      // an error occurred
      exit(EXIT_FAILURE);
    }
    else{
      for(int x= 0; x< bytes_read; x++){
          if(buffer[x] == '\n'){
            if(wordLength!=0){
              if(wordLength > width){
                bytes_written = write(fd_output,&nextline, 1);
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                failure = true;
                free(word);
                word = (char*)malloc(10*(char));
              }
              else if(newlines >=2){
                bytes_written = write(fd_output,&nextline, 1);
                bytes_written = write(fd_output,&nextline, 1);
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*)malloc(10*(char));
              }
              else if(newlines < 2 && spaces == 0 && currentPosition + wordLength <= width){
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition + =  wordLength;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*)malloc(10*(char));
              }
              else if(newlines < 2 && spaces ==0 && currentPosition + wordLength > width){
                bytes_written = write(fd_output,&nextline, 1);
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*)malloc(10*(char));
              }
              else if( currentPosition + wordLength + 1 <= width){
                bytes_written = write(fd_output,&space, 1);
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*)malloc(10*(char));
              }
              else if( currentPosition + wordLength + 1 > width){
                bytes_written = write(fd_output,&nextline, 1);
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*)malloc(10*(char));
              }
            newlines++;
          }
          else if(isspace(buffer[x]) > 0 ){
            if(wordLength!=0){
              if(wordLength > width){
                bytes_written = write(fd_output,&nextline, 1);
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                failure = true;
                free(word);
                word = (char*)malloc(10*(char));
              }
              else if(newlines >=2){
                bytes_written = write(fd_output,&nextline, 1);
                bytes_written = write(fd_output,&nextline, 1);
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*)malloc(10*(char));
              }
              else if(newlines < 2 && spaces ==0 && currentPosition + wordLength <= width){
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition + =  wordLength;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*)malloc(10*(char));
              }
              else if(newlines < 2 && spaces ==0 && currentPosition + wordLength > width){
                bytes_written = write(fd_output,&nextline, 1);
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*)malloc(10*(char));
              }
              else if( currentPosition + wordLength + 1 <= width){
                bytes_written = write(fd_output,&space, 1);
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*)malloc(10*(char));
              }
              else if( currentPosition + wordLength + 1 > width){
                bytes_written = write(fd_output,&nextline, 1);
                bytes_written = write(fd_output,&word, wordLength);
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*)malloc(10*(char));
              }
            spaces++;
          }
          else{
            if(index > arrayLength-1){
               word = (char*)realloc(10*(char));
              word[index] = buffer[x];
              index ++;
              wordLength++;
            }
            else{
              word[index] = buffer[x];
              index++;
              wordLength++;
            }

          }
      }
    }
    bytes_read = read(fd_input,buffer, BUFFER_SIZE);
  }
   // we have reached the end of the file
  free(word);
  close(fd);
  if(failure = true)
    exit(EXIT_FAILURE);
  exit(EXIT_SUCCESS);
}

  int isFileOrDir(char *name) {
      struct stat data;
      int error = stat(name, &data);
      if(error) {
          perror(name);
          exit(EXIT_FAILURE);
      }

      if(S_ISREG(data.st_mode)) {
          return 1; // it's a file
      }
      if(S_ISDIR(data.st_mode)) {
          return 2; // it's a directory
      }
      return 0;
  }
