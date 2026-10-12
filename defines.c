/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <sys/types.h>

#include "defines.h"

/*char seachPath[250];
size_t append2Path(char *directory) {
    size_t lastPathEnd = strlen(seachPath);
    // extends current seachPath: seachPath + / + directory
    strcat(strcat(&seachPath[lastPathEnd], "/"), directory);
    return lastPathEnd;
}
int checkFileSize(char *pathname, off_t size) {
  int check = 0;
  struct stat statbuf;
  if (stat(pathname, &statbuf) == -1)
    return 0;
  if(statbuf.st_size <= size)
    check = 1;

  return check;
}
void search(char cdir[]){
  DIR *dirp = opendir(cdir);
  if(dirp == NULL) return;

  struct dirent *dentry;
  while ((dentry = readdir(dirp)) != NULL){
    if (strcmp(dentry->d_name, ".") == 0 ||
      strcmp(dentry->d_name, "..") == 0)
    {  continue;  }

    if(dentry->d_type == DT_REG){
      size_t lastPath = append2Path(dentry->d_name);

      int match = 0;
      char filename[] = "sendme_";
      if(strncmp(filename, dentry->d_name, 7) == 0){
        if(checkFileSize(seachPath, 4000) == 1){
          match = 1;
        }
      }
      if(match == 1){
        
      }
      seachPath[lastPath] = '\0';
    }else if(dentry->d_type == DT_DIR){
      size_t lastPath = append2Path(dentry->d_name);
      search(seachPath);
    }
  }
  
}*/