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

struct Args{
  struct Queue* dirQ;
  struct Queue *fileQ;
  int width;
};

void queue_init(struct Queue *q) {
  q->start = NULL;
  q->end = NULL;
  q->numActiveThreads = 0;

  pthread_mutex_init(&q->lock, NULL);
  pthread_cond_init(&q->dequeue, NULL);
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

char *dequeue(struct Queue *q) {

    while(q->start == NULL && q-> numActiveThreads!=0) {
      pthread_cond_wait(&q->dequeue, &q->lock);
    }
    if(q->start == NULL && q -> numActiveThreads == 0)
      return NULL; //not exit_failure. Just means the thread doesn't need to do any work
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
  struct Queue *fileQueue = args->fileQ;
  char* path = dequeue(dirQueue);
  pthread_mutex_lock(&dirQueue->lock);
  dirQueue-> numActiveThreads = dirQueue -> numActiveThreads+1;
  
  pthread_mutex_unlock(&dirQueue->lock);
  DIR* dir = opendir(path); // path seems to be root cause
  if(!dir){
    dirQueue-> numActiveThreads = dirQueue -> numActiveThreads-1;
    if(dirQueue->start == NULL && dirQueue -> numActiveThreads == 0)
      pthread_cond_signal(&dirQueue->dequeue);
    return NULL;
  }
  struct dirent *file;
  while((file = readdir(dir))!= NULL) {
    if(strcmp(file->d_name,".")!=0 && strcmp(file->d_name,"wrap.")!=0){ // changed .. to wrap.
      char *fileName = file->d_name;
      int plen = strlen(path);
      int flen = strlen(fileName);
      char* newpath = malloc(plen + flen +2);
      memcpy(newpath, path, plen);
      newpath[plen] = '/';
      memcpy(newpath + plen + 1, fileName, flen + 1);
      printf("%s",newpath);
      if(isFileOrDir(newpath) == 1){
        enqueue(fileQueue,newpath);
      }
      if(isFileOrDir(newpath) == 2){
        enqueue(dirQueue,newpath);
      }
      free(newpath);
    }

  }
  dirQueue-> numActiveThreads = dirQueue -> numActiveThreads-1;
  if(dirQueue->start == NULL && dirQueue -> numActiveThreads == 0)
    pthread_cond_signal(&dirQueue->dequeue);
  return NULL;
}

void* wrapFiles(void *arg){
  struct Args *args = (struct Args *)arg;
  int width = args->width;
  struct Queue *fileQueue = args->fileQ;
    struct Queue *dirQueue = args->dirQ;
  while(fileQueue->start != NULL || dirQueue->numActiveThreads != 0 ){
    // checks if wrapping is allowed
    int closed;
    pthread_mutex_lock(&fileQueue->lock);
    char* fileName = dequeue(fileQueue);
    pthread_mutex_unlock(&fileQueue->lock);
    if(!fileName){
      if(fileQueue->start != NULL && dirQueue->numActiveThreads != 0)
        pthread_cond_signal(&fileQueue->dequeue);
      return NULL;
    }

    int nameLength = strlen(fileName);
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
  if(fileQueue->start != NULL && dirQueue->numActiveThreads != 0)
    pthread_cond_signal(&fileQueue->dequeue);;
  return NULL;
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
  struct Queue *fQueue = (struct Queue*) malloc(sizeof(struct Queue));

  int numDirThreads;
  int numWrapThreads;

  queue_init(dQueue);
  queue_init(fQueue);

//    if (argc == 1 || argc > 3) {
//         return EXIT_FAILURE;
//     } // change last condition to argc > 5
  int width = atoi(argv[2]);
  assert(width > 0);

  // num of threads are not being read correctly
  int thread = strlen(argv[1]);
  if (argc == 4) {
    if(thread == 2) { // if ./ww -r 20 FileOrDir
      numDirThreads = 1;
      numWrapThreads = 1;
    } else if(thread == 3) { // if ./ww -rN 20 FileOrDir
        numDirThreads = 1;
        numWrapThreads = argv[1][2];
    } else if(thread == 5) { // if ./ww -rN,M 20 FileOrDir
        numDirThreads = argv[1][2];
        numWrapThreads = argv[1][4];
    } else {
      return EXIT_FAILURE;
    }

    pthread_t wrapThreads[numWrapThreads];
    pthread_t dirThreads[numDirThreads];
    struct Args *args = (struct Args *)malloc(sizeof(struct Args));
    char *dirName = (char *)malloc(strlen(argv[3]) + 1);
    memcpy(dirName, argv[3], strlen(argv[3]));
    memset(dirName + strlen(argv[3]), '\0', 1);
    enqueue(dQueue,dirName);
    int count = 0;
   
    for(int x = 0; x < numWrapThreads; x++){
      args->dirQ = dQueue;
      args->fileQ = fQueue;
      args->width = width;
      pthread_create(&wrapThreads[x], NULL, wrapFiles,args);
      count++;
    }
    //printf("num of wrap threads are: %d", count);
    while(dQueue->numActiveThreads !=0 || dQueue->start != NULL){
      for(int x = 0; x < numDirThreads; x++){
        args->dirQ = dQueue;
        args->fileQ = fQueue;
        pthread_create(&dirThreads[x], NULL, readDir,args);
      }
      for(int x = 0; x < numDirThreads; x++){
          pthread_join(dirThreads[x], NULL);
      }
    }
    for(int x = 0; x < numWrapThreads; x++){
      pthread_join(wrapThreads[x], NULL);
    }

//     free(wrapThreads);
//     free(dirThreads);
    free(fQueue);
    free(dQueue);
    free(args);

  //Standard input
//   if(argc == 2) {
//     wrap(width,0,1);
//   }
//   else{
//     int f = isFileOrDir(argv[3]);
//       // file
//       if (f == 1) {
//         int fd = open(argv[3], O_RDONLY);
//         if(fd < 0){
//           // there was some error in opening the file
//         perror("couldn't open file");
//         exit(EXIT_FAILURE);
//         }
//      wrap(width,fd,1);
//      close(fd);
//     }
//      // direc
//      if (f == 2) {
//         char *dirName = argv[3];
//         int closed; // var to check if files are closed or not
//         // makes struct for directory
//         struct dirent *file;
//         DIR *dir = opendir(dirName); // opens directory

//         // switch directories
//         chdir(dirName);
//         int nameLength;
//         if (dir) {

//            while ( (file = readdir(dir)) != NULL) {
//               nameLength = strlen(file->d_name);
//               char *fileName = malloc(sizeof(char) * nameLength + 1);
//               strcpy(fileName, file->d_name);
//               if (isFileOrDir(fileName) == 1) { // only focuses on files, ignores any other subdirectories within the parent directory

//                 // checks if wrapping is allowed
//                 if ( !((strncmp(".", fileName, 1) == 0) || (strncmp("wrap.", fileName, 5) == 0)) ) {
//                   int fd = open(fileName, O_RDONLY);
//                   char *wrapped = malloc(sizeof(char) * 6 + nameLength);
//                   strcpy(wrapped, "wrap.");
//                   strcat(wrapped, fileName); // concatenates "wrap." with given file name

//                   // opens new destination file
//                   int newfd = open(wrapped, O_WRONLY | O_TRUNC | O_CREAT, 0666);
//                   wrap(width, fd, newfd);
//                   free(wrapped);
//                   closed = close(fd);
//                   if (closed != 0) perror("File not closed"); // return EXIT_FAILURE; (removed)
//                   closed = close(newfd);
//                   if (closed != 0) perror("Destination file not closed"); // return EXIT_FAILURE; (removed)
//                 }
//               }
//            free(fileName);
//            }
//         closed = closedir(dir); // close directory when finished
//         //if(closed!=0) perror("Directory not closed"); // return EXIT_FAILURE; (removed)
//         }
//      }
//   return EXIT_SUCCESS;
//   }
    return EXIT_SUCCESS;
  }
}
