#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main(int argc,char *argv[]){
	if(argc!=3){
		cout<<"usage: ./client <SERVER_IP> <SERVER_PORT>";
		exit(1);
	}
	
	int sockfd=0;
	sockfd = socket(PF_INET , SOCK_STREAM , 0);
	if(sockfd < 0){
		cout<<"Fail to create a socket";
		exit(1);
	}

	char ip[1024];
	strcpy(ip,argv[1]);
	struct sockaddr_in info;
	memset(&info , 0 , sizeof(info));
	info.sin_family = PF_INET;
	if( !isalpha(ip[0]) ) 
		info.sin_addr.s_addr = inet_addr(argv[1]);
	else{
		struct hostent* host;
		host=gethostbyname(argv[1]);
		info.sin_addr.s_addr = *(unsigned long *)host->h_addr;
	}
	info.sin_port = htons(atoi(argv[2]));
	//getnameinfo(&info, sizeof(sa), argv[1], sizeof(argv[1]), service, sizeof service, 0);

	if(connect(sockfd , (struct sockaddr *)&info , sizeof(info))<0){
		cout<<"Fail to connect";
		exit(1);
	}	

	fd_set rfds,mas;
	FD_ZERO(&rfds);
	FD_ZERO(&mas);
	FD_SET(0,&mas);
	FD_SET(sockfd,&mas);
	char buf[1024];
	while(1){
		rfds = mas;
		if(select(sockfd+1 , &rfds , NULL , NULL , NULL)<0){
			cout<<"fail to select";
			exit(1);
		}
		for(int i=0 ; i<=sockfd ; i++){
			if(FD_ISSET(i , &rfds)){
				memset(buf , 0 , 1024);
				if(i==0){
					fgets(buf , 1024 , stdin);
					if(strncmp(buf , "exit" , 4) == 0){
						FD_CLR(0 , &rfds);
						exit(0);
					}
					if(send(sockfd , buf , strlen(buf) , 0)<0){
						cout<<"Fail to send";
						exit(1);
					}
				}
				else{
					if(recv(sockfd , buf , 1024 , 0)<0){
						cout<<"Fail to recv";
						exit(1);
					}
					else{
						cout<<buf;
					}
				}
			}
		}
	}
	close(sockfd);
	return 0;
}