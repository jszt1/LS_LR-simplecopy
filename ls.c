#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

void permsToString(char perms[], int perm) {
  perms[0] = !S_ISREG(perm) ? 'd' : '-';
  perms[1] = (perm & S_IRUSR) ? 'r' : '-';
  perms[2] = (perm & S_IWUSR) ? 'w' : '-';
  perms[3] = (perm & S_IXUSR) ? 'x' : '-';
  perms[4] = (perm & S_IRGRP) ? 'r' : '-';
  perms[5] = (perm & S_IWGRP) ? 'w' : '-';
  perms[6] = (perm & S_IXGRP) ? 'x' : '-';
  perms[7] = (perm & S_IROTH) ? 'r' : '-';
  perms[8] = (perm & S_IWOTH) ? 'w' : '-';
  perms[9] = (perm & S_IXOTH) ? 'x' : '-';
  perms[10] = '\0';
}

void printDir(struct dirent *pDIREnt, struct stat st) {
  char strTime[13];
  struct tm tmTime;
  localtime_r(&st.st_mtime, &tmTime);
  strftime(strTime, 13, "%b %d %H:%M", &tmTime);

  char perms[11];
  permsToString(perms, st.st_mode);

  struct passwd *pwd;
  struct group *gwd;

  pwd = getpwuid(st.st_uid);
  gwd = getgrgid(st.st_gid);

  printf("%s %ld %s %s %-10ld %s ", perms, st.st_nlink, pwd->pw_name,
         gwd->gr_name, st.st_size, strTime);

  if (perms[0] == 'd') {
    printf("\033[1;34m%s\033[0m\n", pDIREnt->d_name);
  } else if (st.st_mode > 0 && (st.st_mode & S_IEXEC)) {
    printf("\033[1;32m%s\033[0m\n", pDIREnt->d_name);
  } else {
    printf("%s\n", pDIREnt->d_name);
  }
}

void openDirectoriesRecursively(const char *dirName) {
  DIR *pDIR;
  struct dirent *pDIREnt;
  int directoriesCount = 0;
  int directoriesCapacity = 5;
  printf("%s:\n", dirName);
  char **directories = malloc(sizeof(char *) * directoriesCapacity);
  if (!directories) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  pDIR = opendir(dirName);
  if (!pDIR) {
    fprintf(stderr, "%s %d: opendir() failed (%s)\n", __FILE__, __LINE__,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  while ((pDIREnt = readdir(pDIR)) != NULL) {
    if (strcmp(pDIREnt->d_name, ".") == 0 ||
        strcmp(pDIREnt->d_name, "..") == 0) {
      continue;
    }

    char fullPath[PATH_MAX];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", dirName, pDIREnt->d_name);

    struct stat st;
    if (stat(fullPath, &st) == -1) {
      perror("stat");
      continue;
    }

    printDir(pDIREnt, st);

    if (S_ISDIR(st.st_mode)) {
      if (directoriesCount >= directoriesCapacity) {
        directoriesCapacity += 5;
        char **temp =
            realloc(directories, directoriesCapacity * sizeof(char *));
        if (!temp) {
          perror("realloc");
          for (int i = 0; i < directoriesCount; i++) {
            free(directories[i]);
          }
          free(directories);
          exit(EXIT_FAILURE);
        }
        directories = temp;
      }
      directories[directoriesCount] = strdup(fullPath);
      if (!directories[directoriesCount]) {
        perror("strdup");
        for (int i = 0; i < directoriesCount; i++) {
          free(directories[i]);
        }
        free(directories);
        exit(EXIT_FAILURE);
      }
      directoriesCount++;
    }
  }
  printf("\n");
  closedir(pDIR);

  for (int i = 0; i < directoriesCount; i++) {
    openDirectoriesRecursively(directories[i]);
    free(directories[i]);
  }

  free(directories);
}

int main(int argc, char *argv[]) {
  openDirectoriesRecursively(".");
  return 0;
}
