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
#include <ctype.h>
#include <pthread.h>

#define BUFFER_SIZE 5
void wrap(int width,int fd_input,int fd_output);
int isFileOrDir(char *name);
bool traversalDone = false;

struct Node {
  char *fileName;
  struct Node *next;
};

struct Queue {
  struct Node *start;
  struct Node *end;
  int numActiveThreads;
  pthread_mutex_t lock;
  pthread_cond_t dequeue;
};

struct Queue2 {
  struct Node *start;
  struct Node *end;
  pthread_mutex_t lock2;
  pthread_cond_t dequeue2;
};

struct Args{
  struct Queue* dirQ;
  struct Queue2 *fileQ;
  int width;
};

void queue_init(struct Queue *q) {
  q->start = NULL;
  q->end = NULL;
  q->numActiveThreads = 0;

  pthread_mutex_init(&q->lock, NULL);
  pthread_cond_init(&q->dequeue, NULL);
}

void queue_init2(struct Queue2 *q) {
  q->start = NULL;
  q->end = NULL;
  pthread_mutex_init(&q->lock2, NULL);
  pthread_cond_init(&q->dequeue2, NULL);
}

void enqueue(struct Queue *q, char *name) {
  pthread_mutex_lock(&q->lock);
    struct Node *newNode = (struct Node *) malloc(sizeof(struct Node));
    // allocates new node
    newNode->next = NULL;
    newNode->fileName = name;

    if(q->end != NULL) q->end->next = newNode; // if last item exists then append newNode to it
    q->end = newNode; // else the newNode becomes last

    if(q->start == NULL) q->start = newNode; // nothing in queue, appends newNode in the queue
    pthread_cond_signal(&q->dequeue);
  pthread_mutex_unlock(&q->lock);
  return;
}
void enqueue2(struct Queue2 *q, char *name) {
  pthread_mutex_lock(&q->lock2);
    struct Node *newNode = (struct Node *) malloc(sizeof(struct Node));
    // allocates new node
    newNode->next = NULL;
    newNode->fileName = name;

    if(q->end != NULL) q->end->next = newNode; // if last item exists then append newNode to it
    q->end = newNode; // else the newNode becomes last

    if(q->start == NULL) q->start = newNode; // nothing in queue, appends newNode in the queue
    pthread_cond_signal(&q->dequeue2);
  pthread_mutex_unlock(&q->lock2);
  return;
}

char *dequeue(struct Queue *q) {

    while(q->start == NULL && q->numActiveThreads!=0) {
      pthread_cond_wait(&q->dequeue, &q->lock);
    }
    if(q->start == NULL )
      return NULL;
    struct Node *temp = q->start; // temp is set to first item in queue
    if (q->start == q->end) {
      q->end = temp->next;
    }
    q->start = temp->next; //

    char *dequeuedFile=temp->fileName;
    free(temp);
    q-> numActiveThreads = q -> numActiveThreads+1;
  return dequeuedFile;
}

char *dequeue2(struct Queue2 *q) {

    while(q->start == NULL && traversalDone == false) {
      pthread_cond_wait(&q->dequeue2, &q->lock2);
    }
    if(q->start == NULL )
      return NULL;
    struct Node *temp = q->start; // temp is set to first item in queue
    if (q->start == q->end) {
      q->end = temp->next;
    }
    q->start = temp->next; //

    char *dequeuedFile=temp->fileName;
    free(temp);
  return dequeuedFile;
}
void* readDir(void *arg){

  struct Args *args = (struct Args *)arg;
  struct Queue *dirQueue = args->dirQ;
  struct Queue2 *fileQueue = args->fileQ;
  char* path = NULL;
  pthread_mutex_lock(&dirQueue->lock);
  if(dirQueue -> start != NULL || dirQueue->numActiveThreads!=0){
    path = dequeue(dirQueue); // needs to be in lock
  }
  if(path == NULL){
    traversalDone = true;
    pthread_mutex_unlock(&dirQueue->lock);
    return NULL;
  }
  pthread_mutex_unlock(&dirQueue->lock);
  DIR* dir = opendir(path); // path seems to be root cause

  if(!dir){
    return NULL;
  }
  struct dirent *file;
  while((file = readdir(dir))!= NULL) {
    if(strcmp(file->d_name,".")!=0 && strcmp(file->d_name,"..")!=0){ // we check .wrap in the file threads, this was for another case(there are default directories in linux)
      char *fileName = file->d_name;
      int plen = strlen(path);
      int flen = strlen(fileName);
      char* newpath = malloc(plen + flen +2);
      memcpy(newpath, path, plen);
      newpath[plen] = '/';
      memcpy(newpath + plen + 1, fileName, flen + 1);
      if(isFileOrDir(newpath) == 1){
        enqueue2(fileQueue,newpath);
      }
      if(isFileOrDir(newpath) == 2){
        enqueue(dirQueue,newpath);
      }

    }
  }
  free(path);
  closedir(dir);
  pthread_mutex_lock(&dirQueue->lock);
  dirQueue-> numActiveThreads = dirQueue -> numActiveThreads-1;
  if(dirQueue->start == NULL && dirQueue -> numActiveThreads == 0){
    traversalDone = true;
    pthread_cond_broadcast(&dirQueue->dequeue);
  }
  pthread_mutex_unlock(&dirQueue->lock);
  return NULL;
}

void* wrapFiles(void *arg){
  struct Args *args = (struct Args *)arg;
  int width = args->width;
  struct Queue2 *fileQueue = args->fileQ;
  while(fileQueue->start != NULL || traversalDone == false ){
    // checks if wrapping is allowed
    int closed;
    char* fileName = NULL;
    pthread_mutex_lock(&fileQueue->lock2);
      fileName = dequeue2(fileQueue); // needs to be in lock
    pthread_mutex_unlock(&fileQueue->lock2);
    if(fileName == NULL){
      return NULL;
    }

    int nameLength = strlen(fileName);
    int counter = 0;
    for(int x = nameLength - 1; x>=0; x--){
      if(fileName[x] == '/')
        break;
      counter++;
    }
    char*ptr = &fileName[nameLength-counter];
    int num = nameLength - counter;
    //int ptrLength = strlen(ptr);
    if ( !((strncmp(".", ptr, 1) == 0) || (strncmp("wrap.", ptr, 5) == 0)) ) {
      int fd = open(fileName, O_RDONLY);
      char *wrapped = malloc( nameLength+6);
      for(int x = 0; x < num; x++){
        wrapped[x]=fileName[x];
      }
      wrapped[num]='w';
      wrapped[num+1]='r';
      wrapped[num+2]='a';
      wrapped[num+3]='p';
      wrapped[num+4]='.';
      int counter2 = 0;
      for(int x=num+5; x <nameLength+5; x++){
        wrapped[x]=ptr[counter2];
        counter2++;
      }
      wrapped[nameLength+5]='\0';
      // opens new destination file
      int newfd = open(wrapped, O_WRONLY | O_TRUNC | O_CREAT, 0666);
      wrap(width, fd, newfd);
      free(wrapped);
      closed = close(fd);
      if (closed != 0) perror("File not closed"); // return EXIT_FAILURE; (removed)
      closed = close(newfd);
      if (closed != 0) perror("Destination file not closed"); // return EXIT_FAILURE; (removed)
    }
      free(fileName);
  }
  return NULL;
}
bool parseArgument(char* argument, int *dirT, int *wrapT){
  int argumentLength = strlen(argument);
  int counter1 = 0;
  int counter2 = 0;
  if(argument[0]!='-'&& argument[1]!='r')
    return false;
  if(argumentLength == 2){
    *dirT=1;
    *wrapT=1;
  }
  for(int x = 2; x< argumentLength; x++){
    if(argument[x]== ',')
      break;
    counter1++;
  }
  for(int x = counter1+3; x<argumentLength; x++){
    counter2++;
  }
  if(counter1==0)
    return false;
  char* first = malloc(counter1+1);
  int counter4=0;
  for(int x =2; x < counter1+2; x++){
    if(isdigit(argument[x])){
      first[counter4]=argument[x];
      counter4++;
    }
    else{
      free(first);
      return false;
    }
  }
  first[counter1]='\0';
  if(counter2 == 0){
    *dirT = atoi(first);
    *wrapT = 1;
    free(first);
    return true;
  }
  else{
    char* second = malloc(counter2+1);
    int counter3 =0;
    for(int x=counter1+3; x< argumentLength; x++){
      if(isdigit(argument[x])){
        second[counter3]=argument[x];
        counter3++;
      }
      else{
        free(first);
        free(second);
        return false;
      }
    }
    second[counter2]='\0';
    *dirT = atoi(first);
    *wrapT = atoi(second);
    free(first);
    free(second);
  }
  return true;
}
void wrap(int width, int fd_input, int fd_output){
  char buffer[BUFFER_SIZE];
   int bytes_read= read(fd_input,buffer, BUFFER_SIZE);
   int spaces = 0;
   int newlines =0;
   int index = 0;
   int wordLength =0;
   int currentPosition = 0;
   int arrayLength = 10;
   int numWords = 0;
   char * word = (char*) malloc(arrayLength * sizeof(char));
   char space = ' ';
   char nextline ='\n';
   int bytes_written = 0;
   bool failure = false;
  while(bytes_read != 0){
    if(bytes_read < 0){
      perror("Couldn't read ");
      exit(EXIT_FAILURE);
    }
    else{
      for(int x= 0; x< bytes_read; x++){
          if(buffer[x] == '\n'){
            if(wordLength!=0){
              if(wordLength > width){
                bytes_written = write(fd_output,word, wordLength);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                bytes_written = write(fd_output,&nextline, 1);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                failure = true;
                arrayLength = 10;
                numWords = 0;
                free(word);
                word = (char*) malloc(arrayLength * sizeof(char));
              }
              else if(numWords == 0){
                bytes_written = write(fd_output,word, wordLength);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                newlines = 0;
                spaces = 0;
                currentPosition = wordLength;
                wordLength = 0;
                arrayLength = 10;
                numWords ++;
                index = 0;
                free(word);
                word = (char*) malloc(arrayLength * sizeof(char));
              }
              else if(newlines >=2){
                bytes_written = write(fd_output,&nextline, 1);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                bytes_written = write(fd_output,&nextline, 1);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                bytes_written = write(fd_output,word, wordLength);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                newlines = 0;
                spaces = 0;
                currentPosition = wordLength;
                wordLength = 0;
                arrayLength = 10;
                numWords ++;
                index = 0;
                free(word);
               word = (char*) malloc(arrayLength * sizeof(char));
              }

              else if( currentPosition + wordLength + 1 <= width){
                bytes_written = write(fd_output,&space, 1);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                bytes_written = write(fd_output,word, wordLength);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                newlines = 0;
                spaces = 0;
                currentPosition = currentPosition + wordLength +1;
                wordLength = 0;
                arrayLength = 10;
                numWords ++;
                index = 0;
                free(word);
                word = (char*) malloc(arrayLength * sizeof(char));
              }
              else if( currentPosition + wordLength + 1 > width){
                bytes_written = write(fd_output,&nextline, 1);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                bytes_written = write(fd_output,word, wordLength);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                newlines = 0;
                spaces = 0;
                currentPosition = wordLength;
                wordLength = 0;
                arrayLength = 10;
                numWords ++;
                index = 0;
                free(word);
                word = (char*) malloc(arrayLength * sizeof(char));
              }
            }
            newlines++;
          }
          else if(isspace(buffer[x]) > 0 ){
            if(wordLength!=0){
              if(wordLength > width){
                bytes_written = write(fd_output,word, wordLength);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                bytes_written = write(fd_output,&nextline, 1);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                newlines = 0;
                spaces = 0;
                currentPosition = 0;
                wordLength = 0;
                index = 0;
                arrayLength = 10;
                numWords = 0;;
                failure = true;
                free(word);
                word = (char*) malloc(arrayLength * sizeof(char));
              }
              else if(numWords == 0){
                bytes_written = write(fd_output,word, wordLength);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                newlines = 0;
                spaces = 0;
                currentPosition = wordLength;
                wordLength = 0;
                arrayLength = 10;
                numWords ++;
                index = 0;
                free(word);
                word = (char*) malloc(arrayLength * sizeof(char));
              }
              else if(newlines >=2){
                bytes_written = write(fd_output,&nextline, 1);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                bytes_written = write(fd_output,&nextline, 1);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                bytes_written = write(fd_output,word, wordLength);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                newlines = 0;
                spaces = 0;
                currentPosition = wordLength;
                wordLength = 0;
                arrayLength = 10;
                numWords ++;
                index = 0;
                free(word);
                word = (char*) malloc(arrayLength * sizeof(char));
              }

              else if( currentPosition + wordLength + 1 <= width){
                bytes_written = write(fd_output,&space, 1);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                bytes_written = write(fd_output,word, wordLength);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                newlines = 0;
                spaces = 0;
                currentPosition = currentPosition +wordLength +1;
                arrayLength = 10;
                numWords ++;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*) malloc(arrayLength * sizeof(char));
              }
              else if( currentPosition + wordLength + 1 > width){
                bytes_written = write(fd_output,&nextline, 1);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                bytes_written = write(fd_output,word, wordLength);
                if(bytes_written < 0){
                  perror("Couldn't write");
                  exit(EXIT_FAILURE);
                }
                newlines = 0;
                spaces = 0;
                currentPosition = wordLength;
                arrayLength = 10;
                numWords ++;
                wordLength = 0;
                index = 0;
                free(word);
                word = (char*) malloc(arrayLength * sizeof(char));
              }
            }
            spaces++;

        }
          else{
            if(index > arrayLength-1){
               word = (char*)realloc(word,(10+arrayLength)*sizeof(char));
               arrayLength +=10;
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
   bytes_written = write(fd_output,&nextline, 1);
  free(word);
  if(failure == true)
    exit(EXIT_FAILURE);

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
int main(int argc, char*argv[]) {
  struct Queue *dQueue = (struct Queue*) malloc(sizeof(struct Queue));
  struct Queue2 *fQueue = (struct Queue2*) malloc(sizeof(struct Queue2));

  int numDirThreads=0;
  int numWrapThreads=0;

  queue_init(dQueue);
  queue_init2(fQueue);

    if (argc == 1 || argc > 4) {
        free(dQueue);
        free(fQueue);
        return EXIT_FAILURE;
     } // change last condition to argc > 4
  int width = 0;

  //printf("%s",argumentOne);
  if (argc == 4) {
    char* argumentOne =argv[1];
    width = atoi(argv[2]);
    assert(width > 0);
    bool success = parseArgument(argumentOne,&numDirThreads,&numWrapThreads);
    if(success == false || numWrapThreads == 0 || numDirThreads == 0){
      free(dQueue);
      free(fQueue);
      return EXIT_FAILURE;
    }

    pthread_t *wrapThreads = malloc(numWrapThreads * sizeof(pthread_t));
    pthread_t *dirThreads = malloc(numDirThreads * sizeof(pthread_t));
    //pthread_t wrapThreads[30];
    //pthread_t dirThreads[30];
    struct Args *args = (struct Args *)malloc(sizeof(struct Args));
    char *dirName = argv[3];
    int size = strlen(dirName)+1;
    char *name = malloc(size * sizeof(char));
    for(int x=0; x< size; x++){
      name[x]=dirName[x];
    }
    enqueue(dQueue,name);
    //int count = 0;
    for(int x = 0; x < numWrapThreads; x++){
      args->dirQ = dQueue;
      args->fileQ = fQueue;
      args->width = width;
      pthread_create(&wrapThreads[x], NULL, wrapFiles,args);
      //count++;
    }
    //printf("num of wrap threads are: %d", count);
    while(traversalDone == false || dQueue->start != NULL){
      for(int x = 0; x < numDirThreads; x++){
        args->dirQ = dQueue;
        args->fileQ = fQueue;
        pthread_create(&dirThreads[x], NULL, readDir,args);
      }
      for(int x = 0; x < numDirThreads; x++){
          pthread_join(dirThreads[x], NULL);
      }
    }
   traversalDone = true;
    pthread_cond_broadcast(&fQueue->dequeue2);
    for(int x = 0; x < numWrapThreads; x++){
      pthread_join(wrapThreads[x], NULL);
    }

    free(wrapThreads);
    free(dirThreads);
    free(args);
  }
  //Standard input
  else if(argc == 2) {
    width = atoi(argv[1]);
    assert(width > 0);
     wrap(width,0,1);
   }
  else{
    width = atoi(argv[1]);
    assert(width > 0);
  int f = isFileOrDir(argv[2]);
     // file
     if (f == 1) {
     int fd = open(argv[2], O_RDONLY);
      if(fd < 0){
     // there was some error in opening the file
       perror("couldn't open file");
       exit(EXIT_FAILURE);
     }
     wrap(width,fd,1);
      close(fd);
   }
     // direc
     if (f == 2) {
       char *dirName = argv[2];
        int closed; // var to check if files are closed or not
        // makes struct for directory
        struct dirent *file;
        DIR *dir = opendir(dirName); // opens directory

         // switch directories
         chdir(dirName);
         int nameLength;
         if (dir) {

           while ( (file = readdir(dir)) != NULL) {
              nameLength = strlen(file->d_name);
             char *fileName = malloc(sizeof(char) * nameLength + 1);
             strcpy(fileName, file->d_name);
              if (isFileOrDir(fileName) == 1) { // only focuses on files, ignores any other subdirectories within the parent directory

           // checks if wrapping is allowed
            if ( !((strncmp(".", fileName, 1) == 0) || (strncmp("wrap.", fileName, 5) == 0)) ) {
               int fd = open(fileName, O_RDONLY);
               char *wrapped = malloc(sizeof(char) * 6 + nameLength);
               strcpy(wrapped, "wrap.");
              strcat(wrapped, fileName); // concatenates "wrap." with given file name

                // opens new destination file
                 int newfd = open(wrapped, O_WRONLY | O_TRUNC | O_CREAT, 0666);
                  wrap(width, fd, newfd);
                   free(wrapped);
                   closed = close(fd);
                 if (closed != 0) perror("File not closed"); // return EXIT_FAILURE; (removed)
                 closed = close(newfd);
                 if (closed != 0) perror("Destination file not closed"); // return EXIT_FAILURE; (removed)
                }
               }
           free(fileName);
           }
        closed = closedir(dir); // close directory when finished
        //if(closed!=0) perror("Directory not closed"); // return EXIT_FAILURE; (removed)
      }
     }
   }
   free(fQueue);
   free(dQueue);
    return EXIT_SUCCESS;
  }
