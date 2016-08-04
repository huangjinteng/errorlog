#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <time.h>

void setnonblocking(int fd){
	int opts;
	opts = fcntl(fd,F_GETFL);
	if(opts < 0){
		perror("fcntl F_GETFL");
		exit(0);
	}
	opts = opts | O_NONBLOCK;
	if(fcntl(fd,F_SETFL,opts)){
		perror("fcntl F_SETFL");
		exit(0);
	}
}
int count = 0;
void do_use_fd(int client_fd){
//	const char str[] = "God bless you!\n";
//	if(send(client_fd,str,sizeof(str),0)){
//		perror("do_use_fd send");
//	}
//	count ++;
//	char revbuf[1024];
//	while(recv(client_fd,revbuf,sizeof(revbuf),0)){
//		printf("do_use_fd revbuf = %s\n",revbuf);
//	}
	time_t now;
	const struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	char date[300];// = (char *)malloc(100);
        char temp[20];
	memset(date,0,300);
	printf("start data = %s\n",date);
        sprintf(temp,"applog_%d\0",((*timenow).tm_year + 1900));
	printf("1 temp = %s\n",temp);
	printf("1 data = %s\n",date);
        strcat(date,temp);
        sprintf(temp,"-%d",(((*timenow).tm_mon) + 1));
	printf("2 temp = %s\n",temp);
	printf("2 data = %s\n",date);
        strcat(date,temp);
        sprintf(temp,"-%d",((*timenow).tm_mday) + 1);
	strcat(date,temp);
	printf("3 temp = %s\n",temp);
	printf("3 data = %s\n",date);
	char cwd[1000];
        getcwd(cwd,1000);
	strcat(cwd,"/");
	strcat(cwd,date);
        strcat(cwd,".txt");
	printf("file name = %s\n",cwd);
	if(1){
	//	return;
	}
        FILE *f = fopen(cwd,"a");
	char buffer[1024];
	if(f < 0){
		perror("open f");
		exit(0);
	}
	//memset(buffer,0,1024);
	char title[100];
	sprintf(title,"%04d/%02d/%02d %02d:%02d:%02d\n",timenow->tm_year
,timenow->tm_mon + 1,timenow->tm_mday,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
	fwrite(title,strlen(title),1,f);
	while(1){
        	int len = read(client_fd,buffer,sizeof(buffer));
        	if(len > 0){
        		fwrite(buffer,len,1,f);
                	fflush(f);
                	//fprintf(f,"%s",buffer);
                	printf("hander on buffer %s\n",buffer);
        	}else{
                	printf("handler err buffer %s\n",buffer);
			break;
        	}
        }
	//free(date);
	fclose(f);
	close(client_fd);
}

int main(){
	int sockfd;
	struct sockaddr_in my_addr;
	struct sockaddr_in remote_addr;

	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < -1){
		perror("socket");
		exit(0);
	}
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(8090);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//memset(&my_addr,0,sizeof(my_addr));
	//memset(&remote_addr,0,sizeof(remote_addr));
	if((bind(sockfd,(struct sockaddr *)&my_addr,sizeof(my_addr))) < 0){
		perror("bind");
		exit(0);
	}
	printf("bind ok\n");
	if(listen(sockfd,5) < 0){
		perror("listen");
		exit(0);
	}
	printf("listen ok\n");
	int sin_size = sizeof(struct sockaddr_in);
#define MAX_EVENTS 10
	struct epoll_event ev,events[MAX_EVENTS];
	int conn_sock,nfds,epollfd;
	epollfd = epoll_create(10);
	if(epollfd < 0){
		perror("epoll_create");
		exit(0);
	}
	printf("epoll_create ok\n");
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	if(epoll_ctl(epollfd,EPOLL_CTL_ADD,sockfd,&ev) < 0){
		perror("epoll_ctl");
		exit(0);
	}
	printf("epoll_ctl ok\n");
	for(;;){
		printf("start epoll_wait \n");
		nfds = epoll_wait(epollfd,events,MAX_EVENTS,-1);
		if(nfds == -1){
			perror("epoll_wait");
			exit(0);
		}
		printf("epoll_wait returns,nfds = %d\n",nfds);
		int n;
		for(n = 0;n< nfds;n++){
			if(events[n].data.fd == sockfd){
				printf("this is mainsock\n");
				conn_sock = accept(sockfd,
				(struct sockaddr *)&remote_addr,&sin_size);
				if(conn_sock < 0){
					perror("accept");
					exit(0);
				}
				setnonblocking(conn_sock);
				ev.events = EPOLLIN ;//EPOLLET
				ev.data.fd = conn_sock;
				if(epoll_ctl(epollfd,EPOLL_CTL_ADD,conn_sock,
				&ev) < 0){
					perror("epoll ctl conn_sock");
					exit(0);
				}
			}else{//表示客户端可读写了
				printf("doing file write\n");
				do_use_fd(events[n].data.fd);
				if(epoll_ctl(epollfd,EPOLL_CTL_DEL,conn_sock,&ev)){
					perror("epoll_ctl del");
					//exit(0);
				}
				printf("doing file write done!!!\n");
			}
		}
	}
	return 0;
}




