//Siri Sajja
//main.c
//CSC345 Project 1
//AI (ChatGPT) used for help with enable_raw_mode() & read_line()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define hsize 5

//Advised by AI
#include <termios.h>
//Terminal controls for arrow keys (advised by AI)
struct termios orig_termios;
//Not looking at previous commands
int navigation = -1; 

//History
//Stores last history size
char *history[hsize];
//Counter for hsize
int counter = 0; 
//Where next command will go
int indexer = 0; 
int maxlines = 1000;
int maxargs = 100;

//Last executed command which would be !!
char *lastcommand = NULL;

//Use of AI for arrow keys (AI for function enable_raw_mode())
int enable_raw_mode(void) {
    struct termios raw;
    //Using tcgetattr() to retrieve the current mode information
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        return -1;
    }
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    //Using use tcsetattr() function to set the input processing mode
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        return -1;
    }
    return 0;
}

//Comand history
void cmdhistory(char *command) {
    history[indexer] = command;
    indexer = (indexer+1) % hsize;
    if (counter < hsize) {
        counter++;
    }
}

//Gets history entry by index by which is newest
char *gethistory(int idx) {
    if (idx < 0){
        return NULL;
    }
    if(idx >= counter){
        return NULL;
    }
    int newest = (indexer-1+hsize) % hsize;
    int position = (newest-idx+hsize) % hsize;
    return history[position];
}

//Printing for osc
void printing(void) {
    char *cwd = getcwd(NULL, 0);
    //If there is no cwd then it would just be "osc> " 
    if (!cwd) {
        printf("osc> ");
        //In order to appear immediately
        fflush(stdout);
        return;
    }
    //Find the last "/"
    char *b = cwd;
    for (char *p = cwd; *p; p++) {
        if (*p == '/') {
            //move past one
            b = p + 1;
        }
    }
    //Otherwise "osc:%s> "
    printf("osc:%s> ", b);
    //In order to appear immediately
    fflush(stdout);
}

//Input made to be read
// Tokenize input into characs array, splitting on < > | &
int characters(char *line, char **characs) {
    int t = 0;          
    char *p = line; 

    while (*p && t < maxargs - 1) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t' || *p == '\n') {
            p++;
        }
        if (*p == '\0') break;

        // < > | &
        if (*p == '<' || *p == '>' || *p == '|' || *p == '&') {
            char tmp[2] = { *p, '\0' };
            //Using strdup for easier coding
            characs[t++] = strdup(tmp);
            p++;
            continue;
        }

        char *start = p;
        while (*p && *p != ' ' && *p != '\n' && *p != '<' && *p != '>' && *p != '|' && *p != '&') {
            p++;
        }
        int length = p - start;
        char *cmdd = malloc(length + 1);
        if (cmdd == NULL) {
            break;
        }
        memcpy(cmdd, start, length);
        cmdd[length] = '\0';
        characs[t++] = cmdd;
    }

    characs[t] = NULL;
    return t;
}

//The commands
int commands(char **characs) {
    //Checks is characs array has anyting to process
    if (characs == NULL) {
        return 0;
    }
    //Checks is characs array has anyting to process
    if(characs[0]==NULL){
        return 0;
    }
    //Num of strings in characs
    int i = 0;
    while (characs[i]) {
        i++;
    }

    int f = 0;
    while (characs[f] != NULL) {
        f++;
    }

    //See if last command is &
    if (i > 0) {
        //Pointer to last command
        char *prev = characs[i-1];  
        if(prev != NULL){
            //If prev was & 
            if (*prev == '&') {
                //Remove &
                characs[i-1] = NULL;  
                i--;
            }
        }
    }

    //See if command was cd
    char *commandcd = characs[0];
    while (*commandcd && *commandcd != ' ') {
        commandcd++;
    }
    //Check if the command starts with "cd" or "cd "
    if (strcmp(characs[0], "cd") == 0) {
        if (characs[1] == NULL) {
            printf("No path\n");
        } 
        else {
            if (chdir(characs[1]) != 0) {
                fprintf(stderr, "cd failed\n");
            }
        }
        return 0;
    }

    char *infile = NULL;
    char *outfile = NULL;
    int j = 0;
    while (characs[j]) {
        if (strcmp(characs[j], "<") == 0) {
            if (characs[j+1] == NULL) {
                fprintf(stderr, "No input file specified\n");
                return 1;
            }
            infile = characs[j+1];
            //remove "<" and filename
            for (int k = j; characs[k+2]; k++) {
                characs[k] = characs[k+2];
            }
            //terminate properly
            characs[j] = NULL; 
            continue;
        }
        else if (strcmp(characs[j], ">") == 0) {
            if (characs[j+1] == NULL) {
                fprintf(stderr, "No output file specified\n");
                return 1;
            }
            outfile = characs[j+1];
            //remove ">" and filename
            for (int k = j; characs[k+2]; k++) {
                characs[k] = characs[k+2];
            }
            //terminate properly
            characs[j] = NULL; 
            continue;
        }
        j++;
    }

    char *max[maxargs];
    int ii = 0; 
    int x = 0;
    while (characs[x]) {
        if(ii < maxargs-1){
            max[ii++] = characs[x++];
        }
    }
    max[ii] = NULL;
    if (ii == 0) {
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
        return 1;
    } 
    else if (pid == 0) {
        if (infile) {
            //Open input file
            int fd = open(infile, O_RDONLY);
            if (fd < 0) { 
                fprintf(stderr, "Can't open\n");
                exit(1); 
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (outfile) {
            //Open output file
            int out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out < 0) { 
                fprintf(stderr, "Can't open\n");
                exit(1); 
            }
            if (dup2(out, STDOUT_FILENO) < 0) { 
                fprintf(stderr, "dup2 failed\n");
                exit(1); 
            }
            close(out);
        }
        //Binary at path using execve()
        execvp(characs[0], characs);
        fprintf(stderr, "Execution failed\n");
        exit(1);
    } 
    else {
        int status;
        waitpid(pid, &status, 0);
        printf("\n");   // ensure prompt goes to next line
        fflush(stdout);
    } 

    printf("\n");
    fflush(stdout);
    return 0;
}

//Implementing pipelines for left & right
void pipes(char *left, char *right) {
    //To store for left commands
    char *lchars[maxargs];
    int lcount = characters(left, lchars);
    //To store for right commands
    char *rchars[maxargs];
    int rcount = characters(right, rchars);

    //If there is nothing for lcount
    if (lcount == 0) {
        fprintf(stderr, "No pipe cmd\n");
        return;
    }
    //If there is nothing for rcount
    if(rcount == 0){
        fprintf(stderr, "No pipe cmd\n");
        return;
    }
    int fd[2];
    if (pipe(fd) == -1) {
        fprintf(stderr,"pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        fprintf(stderr,"fork");
        return;
    }
    if (pid1 == 0) {
        //left commands
        //close unused write end
        close(fd[0]);
        //redirect stdin from pipe 
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        execvp(lchars[0], lchars);
        fprintf(stderr,"execvp left");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        fprintf(stderr,"fork");
        return;
    }

    if (pid2 == 0) {
        //right commands
        //close unused write end
        close(fd[1]);        
        //redirect stdin from pipe         
        dup2(fd[0], STDIN_FILENO);    
        close(fd[0]);
        execvp(rchars[0], rchars);
        fprintf(stderr,"execvp right");
        exit(1);
    }
    //Close parents
    close(fd[0]);
    close(fd[1]);

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);
}

//Reads lines with arrow keys & backspace
//Use of AI for read_line()
char *read_line(void) {
    char *buf = malloc(maxlines);
    if (buf ==NULL){
        return NULL;
    } 
    int pos = 0;

    //As long as it returns 1
    while (1) {
        char c;
        //Read file
        int r = read(0, &c, 1);
        if (r <= 0) {
            buf[0] = '\0';
            return buf;
        }
        if (c == '\n') {
            //String end
            buf[pos] = '\0';
            return buf;
        } 
        else if (c == 127 || c == '\b') {
            if (pos > 0) { 
                pos--; 
            }
        } 
        //Store
        else if(pos < maxlines-1 && c >= 32){
            buf[pos++] = c;
            printf("%c", c);
            fflush(stdout);
        }
        //use of AI
        else if (c == 27) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) {
                continue;
            }
            if (read(STDIN_FILENO, &seq[1], 1) <= 0){
                continue;
            } 
            if (seq[0] == '[') {
                //up
                if (seq[1] == 'A') { 
                    if (counter == 0) { 
                        write(STDOUT_FILENO, "\a", 1); 
                        continue; 
                    }
                    if (navigation < counter - 1) {
                        navigation++;
                    }
                    char *ent = gethistory(navigation);
                    if (ent == NULL) {
                        continue;
                    }
                    while (pos > 0) { 
                        write(STDOUT_FILENO, "\b \b", 3); 
                        pos--; 
                    }
                    strncpy(buf, ent, maxlines-1);
                    pos = strlen(buf);
                    write(STDOUT_FILENO, buf, pos);
                } 
                //down
                else if (seq[1] == 'B') { 
                    if (counter == 0) { 
                        write(STDOUT_FILENO, "\a", 1); 
                        continue; 
                    }
                    if (navigation <= 0) {
                        navigation = -1;
                        while (pos > 0) { 
                            write(STDOUT_FILENO, "\b \b", 3); 
                            pos--; 
                        }
                        buf[0] = '\0';
                    } 
                    else {
                        navigation--;
                        char *ent = gethistory(navigation);
                        if (ent == NULL) {
                            continue;
                        }
                        while (pos > 0) { 
                            write(STDOUT_FILENO, "\b \b", 3); 
                            pos--; 
                        }
                        strncpy(buf, ent, maxlines-1);
                        pos = strlen(buf);
                        write(STDOUT_FILENO, buf, pos);
                    }
                }
            }
        } 
        else if (c >= 32) {
            if (pos < maxlines - 1) {
                buf[pos++] = c;
                printf("%c", c);
                //Print immediately
                fflush(stdout);
            }
        }
    }
}

int main() {
    for (int i = 0; i < hsize; ++i){
        history[i] = NULL;
    } 

    //Calls back AI function
    if (enable_raw_mode() == -1) {
        //If something is wrong
        fprintf(stderr, "enable_raw_mode not working\n");
        exit(1);
    }

    //While returning 1
    while (1) {
        printing();
        char *line = read_line();
        //If empty then next line
        if (line == NULL) {
            line = "\n";
            continue;
        }
        //When "exit"
        if (line[0] == 'e' && line[1] == 'x' && line[2] == 'i' && line[3] == 't' && line[4] == '\0') {
            break;
        }
        //When command "!!"" for last command
        if (line[0] == '!' && line[1] == '!' && line[2] == '\0') {
            if (lastcommand == NULL) {
                printf("No commands\n");
                continue;
            } 
            line = lastcommand;
            printf("%s\n", line);
        }

        char *position = NULL;
        int i = 0;
        while (line[i] != '\0') {
            if (line[i] == '|') {
                position = &line[i];
                break;
            }
            i++;
        }
        if (position) {
            *position = '\0';
            pipes(line, position + 1);
            continue;
        }

        char copy[maxlines];
        int k = 0;
        while (line[k] != '\0' && k < maxlines - 1) {
            copy[k] = line[k];
            k++;
        }
        copy[k] = '\0';

        char *characs[maxargs];
        int n = characters(copy, characs);
        if (n > 0) {
            commands(characs);
            if (lastcommand) {
                free(lastcommand);
            }
            lastcommand = strdup(line);
            cmdhistory(strdup(line));
        }
    }
    //Free mem
    for (int i = 0; i < hsize; ++i){
        if (history[i]) {
            free(history[i]);
        }
    }
    if (lastcommand) {
        free(lastcommand);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    return 0;
}
