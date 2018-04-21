#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <libgen.h>
#include <sys/stat.h>
#include <fcntl.h>

extern char **environ;
void eatblank(char **buf)
{
  while(**buf==' ')
  {
    (*buf)++;
  }
}

void Pipe(char ** args,char *buf);

void Redirect_test(char **cmd)
{
  pid_t id=fork();
  if(id==0)  //child
  {
      int j=0;
      int fd;
      for(j=0;cmd[j];++j){
        if(strcmp(cmd[j],">")==0){ 
          char* file=cmd[j+1];     
          cmd[j]=NULL;  
          close(1);   
          fd=open(file,O_WRONLY|O_CREAT|O_TRUNC,0666);   
          break;  
        }  
      } 
      execvp(cmd[0],cmd);     
      exit(1);  
    }  
    else {  
      waitpid(id,NULL,0);  
    } 
}

void Command(char **args,char *buf)
{
  eatblank(&buf);
  args[0]=buf;
  int index=1;
  char *p=buf; // 遍历每一个字符的指针
  while(*p!='\0') // 逐个遍历整个命令串直到结束
  {
    if(*p==' '){ // 对普通指令(不包括管道和重定向)进行分割
      *p='\0';
      p++;
      eatblank(&p);
      if(*p != '|' && *p != '>') {
          args[index++]=p;
      }
      continue;
    }
    else if (*p == '|') {// 管道
      args[index]=NULL;
      Pipe(args,++p); // 那就执行前面的那一段指令
    }
    else { // 普通字符
      p++;
    }
  }
  args[index]=NULL;
}

void Pipe(char ** args,char *buf)
{
  int fd[2];
  pipe(fd);
  pid_t id2=fork();
  if(id2==0) { // 子进程
    close(1);
    dup(fd[1]);
    close(fd[1]);
    close(fd[0]);
    execvp(args[0],args);
  }
  else {
    waitpid(id2,NULL,0);
    close(0);
    dup(fd[0]);
    close(fd[0]);
    close(fd[1]);
    Command(args,buf);
    execvp(args[0],args);
  }
}


int main()
{
  while(1)
  {
    char *args[64];
    char *args2[64];
    char cmd[1024];
    char cmd2[1024];
    cmd[0] = '\0';
    cmd2[0] = '\0';

    /*清空标准输出区*/
    printf("# ");
    fflush(stdout);
    int i;

    int count=read(0,cmd,sizeof(cmd));
    strcpy(cmd2,cmd);
    cmd[count-1]='\0'; // 读取命令
    cmd2[count-1]='\0';

    args[0] = cmd;
    for (i = 0; *args[i]; i++)
      for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++){
        if (*args[i+1] == ' ') {
          *args[i+1] = '\0';
          args[i+1]++;
          break;
        }
    }
    args[i] = NULL;

    if (!args[0])
      continue;

    if (strncmp(args[0],"exit",4)==0)
      return 0;

    // 设置环境变量
    if (strncmp(args[0], "export", 6)==0){
      char *ptr,*name,*value;
      int index=0;
      ptr = args[1];
      name = ptr;
      for (int i = 0; ptr[i]; i++){
	if (ptr[i]=='='){
	      ptr[i] = '\0';
	      value =&ptr[i+1];
	      break;
       }
	}
      setenv(name, value, 1);
      continue;
    }
    // cd
	
        /* 内建命令 */
        if (strcmp(args[0], "cd") == 0) {
            if (args[1])
                chdir(args[1]);
            continue;
        }
        if (strcmp(args[0], "pwd") == 0) {
            char wd[4096];
            puts(getcwd(wd, 4096));
            continue;
        }

    // 重定向
    int j = 0;
    for (i = 0; args[i]; i++){
      if (strcmp(args[i], ">") == 0){
        j = 1;
        break;
      }
    }
    if (j){
      Redirect_test(args);
      continue;
    }

    // 处理管道，N重都可以
    pid_t id=fork();
    if(id==0) {
      Command(args2,cmd2);
      // printf("ssfs%s\n", args2[0]);
      execvp(args2[0],args2);
      exit(0);
    }
    else {
      wait(NULL);
    }
  }
  return 0;
}
