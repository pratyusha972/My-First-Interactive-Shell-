#include<stdio.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<sys/ptrace.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<pwd.h>
#include<math.h>
#include<ctype.h>
char home[1024];
void prompt();
int get_input();
char **split(char *commandline,char* delim);
void cd_exec(char* arr);
void pwd_exec();
void echo_exec(char* array);
int execute_command(char **array);
void pinfo_exec();
int back_ground(char** args);
void process(char** args);
typedef struct child {	
  int ppid;
  char *command;
  struct child *next;
  int ground;
}child;
struct child *queue = NULL;
int queue_pointer = 0;
int main()
{
	getcwd(home,sizeof(home));  //path
	prompt();
	return 0;
}
void prompt()
{
	int stop = 0;
	while(1)
	{
		char hostname[1000];
		int i = 0;
		char user[1000],path[1000],dup[1000],new[1000];
		new[0]='~';
		new[1]='\0';
		getcwd(path,sizeof(path));  //path
		path[sizeof(path)] = '\0';
		strcpy(user,getenv("USER"));
		gethostname(hostname,sizeof(hostname));  //host name
		int length = strlen(home);
		if(strlen(path) >= strlen(home))
		{
			if(strlen(path) > strlen(home))
			{
				strcpy(dup,path);
				dup[strlen(home)] = '\0';
				if(strcmp(dup,home)==0)
				{
					int j,i;
					for(j = strlen(home),i=1;j<strlen(path);j++,i++)
					{
						new[i] = path[j];
					}
					new[i] = '\0';
				}
			}
			printf("<%s@%s:%s>",user,hostname,new);
		}
		else
			printf("<%s@%s:%s>",user,hostname,path);
		int res = get_input();
		if(res == -1)
		{
			stop = 1;
			break;
		}
	}
	if(stop == 1)
		return;
}
int get_input()
{
	char c = getchar();
	char command[5000];
	char **semicolon,**space;
	int i = 0,flag = 0;
	while(c!=EOF && c!='\n')
	{
		command[i] = c;
		i++;
		c = getchar();
	}
	command[i]='\0';
	semicolon = split(command,";");
	i=0;
	while(semicolon[i]!='\0')
	{
		space = split(semicolon[i]," ");
		int j=0;
		int result = back_ground(space); 
		space = '\0';		
		if(result == -1)
		{
			flag = 1;
			break;
		}
		i++;
	}
	semicolon = '\0';
	if(flag == 1)
		return -1;
	return 0;
}
char **split(char *commandline,char* delim)
{
	int bfsize = 256,i = 0;
	char **tokens = malloc(sizeof(char*) * bfsize), *token, **temp;
	token = strtok(commandline,delim);
	while(token != NULL) 
	{
		tokens[i] = token;
		i++;
		if (i>= bfsize)
		{
			bfsize = bfsize+256;
			temp = tokens;
			tokens = realloc(tokens, bfsize * sizeof(char*));
			if (tokens!=NULL) 
			{
				free(temp);
				fprintf(stderr,"Error message: allocation error\n");
				exit(-1);
			}
		}
		token = strtok(NULL,delim);
	}
	tokens[i] = NULL;
	return tokens;
}
int execute_command(char **array)
{
	int i;
	if(strcmp(array[0],"cd")==0)
	{
		if(array[1]=='\0')
		{
			chdir(home);
			return 0;
		}
		cd_exec(array[1]);
	}
	else if(strcmp(array[0],"pwd")==0)
		pwd_exec();
	else if(strcmp(array[0],"echo")==0)
	{
		if(array[1]=='\0')
		{
			return 0;
		}
		echo_exec(array[1]);
	}
	else if(strcmp(array[0],"pinfo")==0)
	{
		if(array[1]=='\0')
			pinfo_exec(-1);
		else
		{
			int length = strlen(array[1]);
			int sum = 0;
			for(i=0;i<length;i++)
			{
				int num = array[1][length-i-1] - '0';
				sum = num*pow(10,i) + sum;
			}
			pinfo_exec(sum);
		}
	}
	else
	{
		if(execvp(array[0],array) == -1)
			fprintf(stderr,"Error in command\n");
		exit(EXIT_FAILURE);
	}
	return 0;
}
void cd_exec(char* arr)
{
	char **slash,path[1024],**back;
	int i =0,k=0;
	int fd;
	if(arr[0]=='/')
	{
		getcwd(path,sizeof(path));
		back = split(path,"/");
		for(i=0;back[i]!='\0';i++)
			chdir("..");
		if(arr[1]=='\0')
			return;
		else if(arr[1]!='\0')
		{
			slash = split(arr,"/");
			for(i=0;slash[i]!='\0';i++)
			{
				if(strcmp(slash[i],"~")==0)
					fd = chdir(home);
				else
					fd = chdir(slash[i]);

				if(fd!=0)
				{
					for(k=i-1;k>=0;k--)
						chdir("..");
					for(k=0;back[k]!='\0';k++)
						chdir(back[k]);
					perror("Error:");
					break;
				}
			}
			return;
		}
	}
	else
	{
		if(arr[0]=='~' && arr[1]=='\0') 
		{
			chdir(home);
			return;
		}	
		slash = split(arr,"/");
		for(i=0;slash[i]!='\0';i++)
		{
			if(strcmp(slash[i],"~")==0)
				fd = chdir(home);
			else
				fd = chdir(slash[i]);

			if(fd!=0)
			{
				for(k=i-1;k>=0;k--)
					chdir("..");
				perror("Error:");
				break;
			}
		}
		return;
	}

}
void pwd_exec()
{
	char path[1000];
	getcwd(path,sizeof(path));  //path
	path[sizeof(path)] = '\0';
	printf("%s\n",path);
	return;
}
void echo_exec(char* array)
{
	if (array[0] == '$')   //echo: environment variables
	{
		if(strcmp(array,"$HOME")==0)
			printf("%s\n",home);
		else
		{
			char s[200];
			int length = strlen(array);
			strncpy(s,array + 1,length-1);
			char *x=getenv(s);
			if(x!=NULL)
				printf("%s\n",x);
			else
				fprintf(stderr,"path does not exist\n");
		}
	}
	else
		printf("%s\n",array);
	return;
}
void pinfo_exec(int pid)
{
	if(pid == -1)
		pid = getpid();
	char path[40], line[100], *p, sudo_ls[100];
	FILE* statusf;
	snprintf(path, 40, "/proc/%d/status", pid);
	statusf = fopen(path, "r");
	if(!statusf)
	{
		fprintf(stderr,"Error: pid does not exist\n");
		fclose(statusf);
		return;
	}
	printf("pid -- %d\n",pid);
	while(fgets(line, 100, statusf)) {
		if(strncmp(line, "State:", 6) != 0)
			continue;
		p = line + 7;
		while(isspace(*p)) ++p;
		printf("Process State: %s", p);
		break;
	}

	while(fgets(line, 100, statusf)) {
		if(strncmp(line, "VmSize:", 6) != 0)
			continue;
		// Ignore "Vmsize" and whitespace
		p = line + 7;
		while(isspace(*p)) ++p;
		printf("Memory %s",p);
		break;
	}
	fclose(statusf);
	snprintf(sudo_ls,100,"sudo realpath /proc/%d/exe",pid);
	char **temppp = split(sudo_ls," ");
	if(execvp(temppp[0],temppp) == -1)
		fprintf(stderr,"Error: wrong pid\n");
	return;
}
int back_ground(char **args)
{
	pid_t pid, wpid,wpid2;
	char tempstr[1000];
	char tempstr2[1000];
	int status,i,stat,stat2;
	for(i=0; args[i] != NULL; i++){}				        
	int back = 0;
	if(strcmp(args[i-1],"&")==0)
		back = 1;
	child *temp;
	temp = queue;
	int rettt = 0;	
	int head = 0;
	for(i=0;temp!=NULL;i++)
	{
		int blah = waitpid(temp->ppid,&stat, WNOHANG);
		if(blah==temp->ppid && temp->ground == 1)
		{
			printf("%s with pid %d exited normally\n",temp->command,temp->ppid);
			if(i==0)
			{
				if(temp->next == NULL)
				{
					queue = NULL;					
					head = 1;
					break;
				}
				else
				{
					temp->ppid = temp->next->ppid;
					int zzpe;
					for(zzpe=0;temp->next->command[zzpe]!='\0';zzpe++)
						temp->command[zzpe] = temp->next->command[zzpe]; 
			       		temp->command[zzpe] = '\0';	
					temp->ground = temp->next->ground;
					child *kk;
					kk = temp->next;
					temp->next = temp->next->next;
				        kk = NULL;	
				}
			}
			else
			{
				if(temp->next!=NULL)
				{	
					child *kkg;
					kkg = temp->next;
					temp->ppid = temp->next->ppid;
					int zzp;
					for(zzp=0;temp->next->command[zzp]!='\0';zzp++)
						temp->command[zzp] = temp->next->command[zzp]; 
			       		temp->command[zzp] = '\0';	
					temp->ground = temp->next->ground;
					temp = temp->next;
					kkg = NULL;
				}
				else if(temp->next==NULL)
				{
					temp = NULL;
					head = 1;
					break;
				}
			}
		}
		if(head == 1)
			break;
		temp = temp->next;
	}
	head = 0;
	pid = fork();
	if(pid == 0) 				
	{
		rettt = execute_command(args);
	} 
	else if (pid == -1) 
		fprintf(stderr,"Error in forking\n");			
	else 							
	{
		if(back == 1)
		{
			if(queue == NULL)
			{	
				queue =(struct child *)malloc(1*sizeof(struct child));
				queue->next = NULL;
				queue->ppid = pid;
				int zz;
				for(zz=0;args[0][zz]!='\0';zz++)
					tempstr[zz] = args[0][zz]; 
			       	tempstr[zz] = '\0';	
				queue->command = tempstr;
				queue->ground = 1;
				printf("%d\n",pid);
			}
			else
			{
				child *test,*temp2;
				test = queue;
				while(test->next!=NULL)
				{
					test = test->next;
				}
				temp2 =(struct child *)malloc(1*sizeof(struct child));
				temp2->ppid = pid;
				int zzz;
				for(zzz=0;args[0][zzz]!='\0';zzz++)
					tempstr2[zzz] = args[0][zzz];
			       	tempstr2[zzz] = '\0';	
				temp2->command = tempstr2;
				temp2->next = NULL;
				temp2->ground = 1;
				test->next = temp2;
				printf("%d\n",pid);
			}
			back = 0;
		}
		else if(back == 0)
		{
			do 
			{
				wpid = waitpid(pid, &status, WUNTRACED); 
			} 
			while (!WIFEXITED(status) && !WIFSIGNALED(status));	
		}
	}
	free(args);
	return 1;
}
