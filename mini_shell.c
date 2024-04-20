#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_ARGS 10
#define MAX_ARG_LENGTH 30

int get_argv(char *line, char *command, char **argv, char storage[][MAX_ARG_LENGTH]);
void check_command(char *line, int *is_redirection, int *is_pipe);
void delete_space(char *s);
void split(char *line, char *first, char *second);

int main(int argc, char **argv) {
    char line[100];
    char first[30], second[30];
    char command[30];
    char *my_argv[MAX_ARGS], argv_storage[MAX_ARGS][MAX_ARG_LENGTH];
    char pwd[256];
    int is_pipe;
    int is_redirection;

    while (1) {
        getcwd(pwd, 256);
        printf("%s$ ", pwd);
        fgets(line, 100, stdin);
        if (strlen(line) == 1)
            continue;

            check_command(line, &is_redirection, &is_pipe); // pipe명령인지 redir명령인지 check

        if (is_redirection || is_pipe )  { //  파이프 or 리다이렉션
                  split(line, first, second);
                  delete_space(first); delete_space(second);
                  printf("first: %s\n", first);
                  printf("second: %s\n", second);
                  printf("is_redirection: %d\n", is_redirection);
                  printf("is_pipe: %d\n", is_pipe); // 디버깅용도

               
                  if(is_pipe) // 파이프 명령처리
                  {
                      int pipe_fd[2];
                      pipe(pipe_fd);
      
                      if (fork() == 0) {
                          get_argv(first, command, my_argv, argv_storage);
                          close(pipe_fd[0]);
                          close(1);
                          dup2(pipe_fd[1], 1);
                          close(pipe_fd[1]);
                          execvp(command, my_argv); 
                          perror("exec error");
                          exit(-1);
                      }
      
                      if (fork() == 0) {
                          get_argv(second, command, my_argv, argv_storage);
                          close(pipe_fd[1]);
                          close(0);
                          dup2(pipe_fd[0], 0);
                          close(pipe_fd[0]);
                          execvp(command, my_argv); 
                          perror("exec error");
                          exit(-1);
                      }
                      close(pipe_fd[0]);
					  close(pipe_fd[1]);
                      wait(NULL);
                      wait(NULL);
               }
                  else if(is_redirection) // 리다이렉션 명령처리
                  {
                        get_argv(first, command, my_argv, argv_storage);
                        int fd = open(second, O_CREAT | O_WRONLY | O_TRUNC, 0666);
                        if (fd < 0) {
                            perror("open error");
                            exit(-1);
                        }

                  // 리다이렉션된 파일 디스크립터로 표준 출력 설정
                   if(fork()==0){
                           close(1);
                           dup2(fd, 1);
                           close(fd);
                           execvp(command, my_argv);      // 리다이렉션 이전의 명령 처리
                           perror("exec error");
                           exit(-1);
                   }
                        wait(NULL);
                 }
        }
      else {
            // No pipe, No redirection
                  int len = strlen(line);
                  line[len-1] = '\0';
			            get_argv(line, command, my_argv, argv_storage);

            if (strcmp(command, "cd") == 0) {
                if (my_argv[1] != NULL) {
                    chdir(my_argv[1]);
                } else {
                    chdir(getenv("HOME"));
                }
            } else if (strcmp(command, "exit") == 0) {
                exit(0);
            } else if (fork() == 0) {
                execvp(command, my_argv);
                perror("exec error");
                exit(-1);
            }

            wait(NULL);
        }
   }
    return 0;
}


void delete_space(char *s) {
    if (s == NULL)
        return;
		
    int len = strlen(s);

    int start = 0;
    while (isspace(s[start]))
        start++;

    int end = len - 1;
    while (end > start && isspace(s[end]))
        end--;
		int i;
    for (i = 0; start + i <= end; i++)
        s[i] = s[start + i];

    s[end - start + 1] = '\0';
}

void split(char *line, char *first, char *second) { //2개입력시 문자 분할
      char *ptr = strtok(line, "|>");

    if (ptr != NULL) {
        // 첫 번째 명령어
        strcpy(first, ptr);

        // 두 번째 명령어
        ptr = strtok(NULL, "|>");
        if (ptr != NULL) {
            strcpy(second, ptr);
        }
		}
}

void check_command(char *line, int *is_redirection, int *is_pipe) {
    // 초기화
    *is_redirection = 0;
    *is_pipe = 0;

    // '>' 및 '|' 검사
    int i;
    for (i = 0; line[i] != '\0'; i++) {
        if (line[i] == '>') {
            *is_redirection = 1;
        }

        if (line[i] == '|') {
            *is_pipe = 1;
        }
    }
}

int get_argv(char *line, char *command, char **argv, char storage[][30])
{
        int arg_count = 0;
        char *ptr;
        int i;
        ptr = strtok(line, " ");
        while(ptr != NULL)
        {
                strcpy(storage[arg_count], ptr);
                arg_count++;
                ptr = strtok(NULL, " ");
        }

        strcpy(command, storage[0]);

        for(i = 0; i<arg_count; i++)
        {
                argv[i] = storage[i];
        }
        argv[arg_count] = NULL;

        return arg_count;
}
