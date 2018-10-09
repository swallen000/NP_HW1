#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>

#include <ctype.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 9487
#define CAPACITY 18
#define BUFSIZE 2048

using namespace std;

typedef struct client{
	int fd;
	char *ip;
	unsigned short port;
	string name;
}INFO;


void shutdown(fd_set *, int , int );
void sayhello(fd_set *, int , int , int , INFO *);
void command(fd_set * , int , int , int , INFO *);
void command_who(int , int , int , INFO *);
void command_yell(int , int , INFO *, char *);
void command_tell(int , INFO * ,char *);
void command_name(int , INFO * ,char *);

int main(int argc , char *argv[]){
	if(argc > 2){
		cout<<"usage: ./server <SERVER_PORT>";
		exit(1);
	}

	unsigned short port;
	int sockfd=0;
	int maxfd;
	if(argc == 2)
		port = atoi(argv[1]);
	else
		port = PORT;

	sockfd = socket(PF_INET , SOCK_STREAM , 0);
	if(sockfd < 0){
		cout<<"Fail to create a socket";
		exit(1);
	}
	struct sockaddr_in serverinfo;
	serverinfo.sin_family = PF_INET;
	serverinfo.sin_addr.s_addr = INADDR_ANY;
	serverinfo.sin_port = htons(port);

	bind(sockfd , (struct sockaddr *) &serverinfo , sizeof(serverinfo));
	listen(sockfd,10);

	fd_set rfds,mas;
	FD_ZERO(&rfds);
	FD_ZERO(&mas);
	FD_SET(0,&mas);
	FD_SET(sockfd,&mas);
	char buf[BUFSIZE];
	INFO info[CAPACITY];
	for(int i=0;i<CAPACITY;i++){
		info[i].fd=-1;
		info[i].name="anonymous";
	}
	//cout<<info[0].name<<endl;
	maxfd=(sockfd>0)? sockfd : 0;

	while(1){
		rfds=mas;
		if(select(maxfd+1 , &rfds , NULL ,NULL ,NULL)<0){
			cout<<"Fail to select";
			exit(1);
		}
		for(int i=0 ; i<=maxfd ; i++){
			if(FD_ISSET(i , &rfds)){
				memset(buf , 0 , BUFSIZE);
				if(i==0){
					fgets(buf , BUFSIZE , stdin);
					if(strcmp(buf , "exit\n")==0){
						shutdown(&mas , maxfd , sockfd);
						exit(0);
					}
				}
				else if(i==sockfd){
					sockaddr_in clientinfo;
					int clientfd;
					socklen_t len = sizeof(clientinfo);
					if( (clientfd=accept(sockfd , (struct sockaddr *) &clientinfo , &len) ) < 0 ){
						cout<<"Fail to accept";
						exit(1);
					}
					int pos,tmp;
					for(int j=3;j<CAPACITY;j++){
						if(info[j].fd == -1 && j!=sockfd){
							pos=j;
							break;
						}
						tmp=j;
					}
					if(tmp!=CAPACITY-1){
						info[pos].fd=clientfd;
						info[pos].ip=inet_ntoa(clientinfo.sin_addr);
						info[pos].port=ntohs(clientinfo.sin_port);
						maxfd = (clientfd > maxfd)? clientfd : maxfd;
						cout<<"new client from "<<clientfd<<"   client's pos is "<<pos<<endl;
						FD_SET(clientfd , &mas);
						sayhello(&mas , maxfd , clientfd , sockfd ,&info[0]);
					}
				}
				if(i != sockfd && i>2)
					command(&mas , maxfd , i , sockfd , &info[0]);
			}
		}

	}
	close(sockfd);
	return 0;
}

void command(fd_set *mas , int maxfd , int fd ,int sockfd, INFO *info){
	char buf[BUFSIZE];
	memset(buf , 0 ,BUFSIZE);
	char *str;
	int pos;
	if(recv(fd , buf , BUFSIZE , 0) <= 0){
		cout<<"Client leaves , fd : "<<fd<<endl;
		for(int i=0 ; i<CAPACITY ; i++){
			if(info[i].fd == fd){
				pos=i;
				break;
			}
		}
		//cout<<"pos= "<<pos<<endl;
		sprintf(buf , "[Server] %s is offline\n" , info[pos].name.c_str());
		for(int i=0 ; i<CAPACITY ; i++){
			if(i!=pos && info[i].fd>=0)
				send(info[i].fd , buf , strlen(buf) , 0);
		}
		info[pos].fd=-1;
		info[pos].name="anonymous";
		FD_CLR(fd , mas);
		close(fd);
	}
	else{
		if(strncmp(buf , "who" ,3)==0 || strcmp(buf , "who\r\n")==0 ){
			command_who(maxfd , fd , sockfd ,&info[0]);
		}
		else if(strncmp(buf , "yell " ,5)==0){
			str=buf+5;
			command_yell(maxfd , fd ,&info[0] , str);
		}
		else if(strncmp(buf , "tell " , 5) == 0){
			str=buf+5;
			command_tell(fd , &info[0], str);
		}
		else if(strncmp(buf , "name " , 5) == 0){
			str=buf+5;
			command_name(fd , &info[0] , str);
		}
		else if(strcmp(buf , "exit\n") == 0 || strcmp(buf , "exit\r\n") == 0){
			cout<<"over";
			info[fd].fd=-1;
			info[fd].name="anonymous";
			FD_CLR(fd , mas);
			close(fd);
		}
		else if(strcmp(buf, "\n" )==0 || strcmp(buf,"\r\n")==0 ){
			/*do nothing*/
		}
		else{
			char error[]="[Server] ERROR: Error command.\n";
			send(fd , error , strlen(error) , 0);
		}
	}

}

void command_who(int maxfd , int fd , int sockfd , INFO *info){
	char buf[BUFSIZE];
	memset(buf , 0 ,BUFSIZE);
	//cout<<"into who";
	for(int i=0 ; i<CAPACITY;i++){
		if(info[i].fd>=0){
			if(info[i].fd == fd){
				sprintf(buf , "[Server] %s %s/%hu ->me\n",info[i].name.c_str(),info[i].ip,info[i].port);
				send(fd, buf , strlen(buf) , 0);
			}
			else{
				sprintf(buf , "[Server] %s %s/%hu \n" , info[i].name.c_str(),info[i].ip,info[i].port );
				send(fd, buf , strlen(buf) , 0);
			}
		}
	}
}

void command_yell(int maxfd , int fd, INFO *info , char *str){
	char buf[BUFSIZE];
	memset(buf , 0 , BUFSIZE);
	int pos;
	for(int i=0 ; i<CAPACITY ;i++){
		if(info[i].fd == fd){
			pos=i;
			break;
		}
	}
	for(int i=0 ; i<strlen(str) ; i++){
		if(str[i]!='\n' && str[i]!='\r' && str[i]!=' ')
			break;
		if( i==(strlen(str)-1) ){
			char error[]="[Server] ERROR: Error command.\n";
			send(fd , error , strlen(error) , 0);
			return;
		}
	}
	for(int i=0 ; i<CAPACITY ;i++){
		if(info[i].fd>=0){
			sprintf(buf , "[Server] %s yell %s", info[pos].name.c_str() , str);
			send(info[i].fd , buf , strlen(buf) , 0);
		}
	}
}

void command_tell(int fd , INFO *info , char *str){
	char buf1[]="[Server] ERROR: You are anonymous.\n";
	char buf2[]="[Server] ERROR: The client to which you sent is anonymous.\n";
	char buf3[]="[Server] ERROR: The receiver doesn't exist.\n";
	char buf4[]="[Server] SUCCESS: Your message has been sent.\n";
	char buf5[BUFSIZE];
	string message=str;
	string temp;
	int i,pos;
	
	for(i=0 ; i<message.length() ; i++){
		if(message[i] == '\n')
			message.erase(i,1);
	}
	for(i=0 ; i<message.length() ; i++){
		if(message[i] != ' ')
			break;
	}
	message.erase(0,i);
	istringstream ss(message);
	ss>>temp;
	message.erase(0,temp.length());
	for(i=0 ; i<message.length() ; i++){
		if(message[i]!='\n' && message[i]!='\r' && message[i]!=' ')
			break;
		if(i== (message.length() -1) ){
			char error[]="[Server] ERROR: Error command.\n";
			send(fd , error , strlen(error) , 0);
			return;
		}
	}
	for(i=0 ; i<message.length() ; i++){
		if(message[i] != ' ')
			break;
	}
	message.erase(0,i);
	//cout<<"message="<<message<<" temp="<<temp<<endl;
	if(info[fd].name == "anonymous")
		send(fd , buf1 , strlen(buf1) , 0);
	else if(temp == "anonymous")
		send(fd , buf2 , strlen(buf2) , 0);
	else {
		//cout<<temp<<";"<<endl;
		for(i=3 ; i<CAPACITY ; i++){
			//cout<<info[i].name<<";"<<endl;
			if(info[i].name == temp){
				pos=i;
				break;
			}
			if(i == CAPACITY-1){
				send(fd , buf3 , strlen(buf3) , 0);
				return;
			}
		}
		//for(i=3 ; i<CAPACITY ; i++){
			//if(i == pos){
				sprintf(buf5 , "[Server] %s tell you %s\n" , info[fd].name.c_str() , message.c_str() );
				send(pos , buf5 , strlen(buf5) , 0);
			//}
			//else if(i == fd){
				send(fd , buf4 , strlen(buf4) , 0);
			//}
		//}
	}
}

void command_name(int fd , INFO *info , char *str){
	char buf1[]="[Server] ERROR: Username can only consists of 2~12 English letters.\n";
	char buf2[]="[Server] ERROR: Username cannot be anonymous.\n";
	char buf3[BUFSIZE];
	string str2;
	int i,j;
	//cout<<"start"<<endl;
	/*for(int i=0 ; str[i]!='\0' ; i++){
		if(str[i]=='\n')
			str[i]='\0';
	}*/
	str2=str;
	/*cout<<"here str2="<<str2<<endl;
	cout<<"here length = "<<str2.length()<<endl;
	for(int i=0;i<strlen(str);i++){
		cout<<str[i]<<" ;";
	}
	cout<<endl;*/
	for(i=0 ; i<str2.length() ; i++){
		while(str2[i] == '\n' || str2[i] == '\r' ||(str2[i]==' '&&(str2[i+1]==' '||str2[i+1]=='\n'||str2[i+1]=='\r') ) ){
			//cout<<"***i="<<i<<"  str2[i]="<<str2[i]<<";"<<endl;
			str2.erase(i,1);
		}
		//cout<<"i="<<i<<"  str2[i]="<<str2[i]<<";"<<endl;
	}
	/*for(i=j=0 ; str[i]!='\0' ; i++){
		if(str[i]!='\n')
			str2[j++]=str[i];
	}
	str2[j]='\0';*/
	//cout<<"change finish"<<endl;
	//cout<<"str2 = "<<str2<<"       str="<<str<<endl;
	for(i=0 ; i<str2.length() ; i++){
		if(str2[i] != ' ')
			break;
	}
	str2.erase(0,i);
	if(str2.length()<2 || str2.length()>12){
		send(fd , buf1 , strlen(buf1) , 0);
	}
	else if(strcmp(str2.c_str() , "anonymous") == 0){
		send(fd , buf2 ,strlen(buf2) , 0);
	}
	else{
		for(i=0 ; i<str2.length() ; i++){
			if( !isalpha(str2[i]) ){
				//cout<<"dead"<<str2<<";"<<endl;
				send(fd , buf1 , strlen(buf1) , 0);
				return;
			}
		}
		for(i=3 ; i<CAPACITY ;i++){
			if(info[i].name == str2 && i!=fd){
				sprintf(buf3 , "[Server] ERROR: %s has been used by others.\n" , str2.c_str() );
				send(fd , buf3 , strlen(buf3) , 0);
				return;
			}
		}
		for(i=3 ; i<CAPACITY ; i++){
			if(info[i].fd>=0){
				if(i==fd){
					sprintf(buf3 , "[Server] You're now known as %s.\n" , str2.c_str() );
					send(fd , buf3 , strlen(buf3) , 0);
				}
				else{
					sprintf(buf3 , "[Server] %s is now known as %s.\n", info[fd].name.c_str() , str2.c_str() );
					send(i , buf3 , strlen(buf3) , 0);
				}
			}
		}
		cout<<info[fd].name<<" change name to "<<str2<<" successfully   fd="<<fd<<endl;
		info[fd].name=str2;
		return;
	}

}

void shutdown(fd_set *mas,int maxfd , int sockfd){
	char buf[BUFSIZE];
	memset(buf , 0 ,BUFSIZE);
	for(int i=3 ; i<=maxfd ;i++){
		if(FD_ISSET(i , mas) && i!=sockfd){
			sprintf(buf , "Server is offline from %d\n" , sockfd);
			if(send(i , buf , strlen(buf) , 0)<0){
				cout<<"Fail to send";
				exit(1);
			}
			close(i);
			FD_CLR(i , mas);
		}
	}
}

void sayhello(fd_set *mas , int maxfd , int clientfd , int sockfd , INFO *info){
	char buf1[]="[Server] Someone is coming!\n";
	char buf2[BUFSIZE];
	memset(buf2 , 0 ,BUFSIZE);
	for(int i=0 ; i<=CAPACITY ; i++){
		if(info[i].fd == clientfd){
			sprintf(buf2 , "[Server] Hello, anonymous! From: %s/%hu\n", info[i].ip , info[i].port);
			if(send(info[i].fd , buf2 , strlen(buf2) , 0)<0)
				cout<<"Fail to send buf2";
		}
		else if(info[i].fd >=0){
			send(info[i].fd , buf1 , strlen(buf1) , 0);
		}
	}
}