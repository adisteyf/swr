/*
 * See LICENSE file for
 * more details & copyright.
 *
 *
 * sw - self writer
 **/



/* includes */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_STRINGS 400

char ** strbuff;
int     line=0;
int     strbufflen=0;
char  * filename=0;


static int    checkcmds    (char *);
static char * strtokn      (char *);
static void   quitredactor (void);
static int    writechanges (void);
static int    readfile     (void);


void
quitredactor (void)
{
  int i=0;
  for (;;) {
    if (!(i<strbufflen)) { break; }
    free(strbuff[i]);
    ++i;
  }
  free(strbuff);
  free(filename);
  exit(0);
}

int
writechanges (void)
{
  if (!filename) {
    puts("No one file is opened.");
    return 0;
  }

  FILE * file = fopen(filename, "w");

  if (!file) {
    printf("Can't open the file. (%s).", filename);
    return 1;
  }

  int i=0;
  for (;;) {
    if (!(i<strbufflen)) { break; }
    fprintf(file,"%s",strbuff[i]);
    ++i;
  }

  fclose(file);
  return 0;
}

char *
strtokn (char * str)
{
    static char * last=0;
    if (str) {
      last = str;
    }

    if (!strlen(last)) { return 0; }
    char * nextnptr = strchr(last, '\n');
    int nextn = nextnptr-last+1;
    char ret[nextn+1];
    ret[nextn]=0;
    for (int i=0; i<nextn; ++i) {
      ret[i] = last[i];
    }

    last = nextnptr+1;
    return strdup(ret);
}

int
readfile (void)
{
  FILE * file = fopen(filename, "r");
  if (!file) { puts("Can't open the file.");return 1; }
  fseek(file,0,SEEK_END);
  long filesize = ftell(file)+1;
  fseek(file,0,SEEK_SET);
  char filestr[filesize];

  filesize=0;
  int newlineslen=0;
  for (;;) {
    char c = getc(file);
    if (c == EOF) { break; }
    if (c == 0x0A) { newlineslen++; }
    filestr[filesize++]=c;
  }

  filestr[filesize]=0;
  if (filesize >= MAX_STRINGS) {
    strbuff = realloc(strbuff, filesize+1);
  }

  char * tok = strtokn(filestr);
  int col=0;
  for (;tok;) {
    strbuff[col++] = tok;
    tok = strtokn(0);
  }
  strbufflen = col;

  fclose(file);
  return 0;
}

int
checkcmds (char * inputbuff)
{
  if (inputbuff==0) { return 1; }
  char cmd = inputbuff[0];

  switch (cmd) {
    case 'e': { /* classic edit (with counter of free lines) */
      char addbuff[100];

      for (;;) {
        if (strbufflen==0 || line>=strbufflen) {
          puts("No space left.");
          line--;
          break;
        }
        if (fgets(addbuff,99,stdin)[0]=='.') {
          line--;
          break;
        }

        if (strbuff[line] && strbuff[line][0]!=0x0A) { free(strbuff[line]); }
        strbuff[line++] = strdup(addbuff);
      }
      break;
    }

    case 'E': { /* force edit (adds new lines) */
      char addbuff[100];

      for (;;) {
        if (fgets(addbuff,99,stdin)[0]=='.') {
          line--;
          break;
        }
        if (strbufflen==0 || line>=strbufflen) {
          puts("(added new string)");
          strbuff[strbufflen++] = strdup("\n");
          if (strbufflen >= MAX_STRINGS && !(strbufflen % 4)) { strbuff = realloc(strbuff, strbufflen+4); }
        }

        if (strbuff[line] && strbuff[line][0]!=0x0A) { free(strbuff[line]); }
        strbuff[line++] = strdup(addbuff);
      }
      break;
    }

    case 'p': {
      puts(strbuff[line]);

      break;
    }

    case 'P': { /* print all lines */
      int i=0;
      for (;;) {
        if (!(i<strbufflen)) { break; }
        printf("%d %s",i,strbuff[i]);
        ++i;
      }

      break;
    }

    case 'g': { /* go to line */
      int tmp=0;
      if (!isdigit(inputbuff[1])) { break; }
      tmp = atoi(&inputbuff[1]);

      if (tmp<0) { break; }

      if (tmp>=strbufflen) {
        puts("Too big number.");
        break;
      }

      line=tmp;
      break;
    }

    case 'o': { /* add new line */
      strbuff[strbufflen++] = strdup("\n");
      break;
    }

    case 'c': { /* cycle any command */
      int tmp=0;
      int i=1;
      tmp = atoi(&inputbuff[i]);
      for (; inputbuff[i]!=0; ++i) {
        if (inputbuff[i]==';') {
          break;
        }
        if (!isdigit(inputbuff[i])) { tmp=-1;break; }
      }

      if (tmp==-1) { break; }

      const int INPUTBUFF_LEN = strlen(inputbuff)-1;
      i=INPUTBUFF_LEN-i-1;
      char inputcyc[i];
      int k=0;
      for (int j=INPUTBUFF_LEN-i; inputbuff[j]!=0&&inputbuff[j]!=0x0A; ++j) {
        inputcyc[k] = inputbuff[j];
        k++;
      }

      for (i=0; i<tmp; ++i) {
        checkcmds(inputcyc);
      }

      break;
    }

    case 'd': { /* delete current line */
      int tmp = line;
      for (;;) {
        if (tmp==strbufflen-1) { break; }
        strbuff[tmp]=strdup(strbuff[tmp+1]);
        tmp++;
      }
      free(strbuff[tmp]);
      strbufflen--;
      break;
    }

    case 'w': {
      writechanges();
      if (strlen(inputbuff)>1 && inputbuff[1]=='q') { quitredactor(); }
      break;
    }

    case 'r': { /* read file */
      const int STRBUFFLEN = strlen(inputbuff);
      const int FILENAMELEN = STRBUFFLEN-3;
      char fn[FILENAMELEN];

      if (inputbuff[1]!=' '||STRBUFFLEN<2) { puts("Incorrect usage.");break; }

      for (int i=2; i<STRBUFFLEN; ++i) {
        fn[i-2] = inputbuff[i];
      }

      fn[FILENAMELEN]=0;
      filename = strdup(fn);
      readfile();

      break;
    }

    case 'q': {
      quitredactor();
    }

    default: {
      if (strcmp(inputbuff, "\n")==0) { break; }
      puts("Unknown command.");
      break;
    }
  }

  return 0;
}

int
main (int argc, char ** argv) {
  if (argc>1) {
    filename = strdup(argv[1]);
  }

  char inputbuff[100];
  strbuff = malloc(sizeof(char *)*MAX_STRINGS);

  for (;;) {
    write(1,"> ",3);
    fgets(inputbuff,99,stdin);
    checkcmds(inputbuff);
  }
  
  quitredactor();
  return 0;
}
