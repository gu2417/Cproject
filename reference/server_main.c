////출처: http://remocon33.tistory.com/465
//개발목표 : 컴퓨터공학부 재학생의 원활한 의사소통을 위한 콘솔 기반 채팅 어플리케이션
// 개발자 : 최재성
// 개발기간 : 2026년 4월 15일 ~ 20일 


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <Windows.h>

#include <process.h>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define MAX_ROOM 100
#define MAX_ROOM_SIZE 10
#define PORT 55555

unsigned WINAPI HandleClient(void* arg);//쓰레드 함수
void SendMsg(char* msg,int len);//메시지 보내는 함수
void ErrorHandling(char* msg);
void ReadUserAuthenticationData();
void UploadUserAuthenticationData(char *str);
void MsgChecker(char *str, SOCKET sock); 
void LoginVerification(char *_id, char *_pw , SOCKET sock);
void leftClient(SOCKET clientSock);
void AddUser(char *_id, char *_pw , SOCKET sock);
int CheckDuplicationID(char *_id);
void CreateRoom(char *title, int _num, SOCKET sock);
void RoomList(SOCKET sock);




typedef struct {
	char uID[20];
	char uPW[20];
}USERINFO;

typedef struct {
	char uID[20];
	SOCKET sock;
	//chat chatName[20];
	//int status; // 0: 활성화상태 1: 방해금지상태 
}CONNECTEDUSER;

typedef struct {
	char rTitle[201];  //방 제목 최대 200자 
	int rSize;
	int numJoinedUser;
	CONNECTEDUSER joinedUsers[MAX_ROOM_SIZE];  
	//	int rType;  // 0 공개  1 비공개
	//	char rEnterCode[11];  //비공개 방의 경우 입장 암호 최대 10자 
}ROOM;


int clientCount=0;
int numUsers = 0;  // 유저 파일로 부터 업로드된 사용자 아이디/패스워드 개수 
int numRooms = 0;

SOCKET clientSocks[MAX_CLNT];//클라이언트 소켓 보관용 배열
HANDLE hMutex;//뮤텍스
USERINFO users[MAX_CLNT];
CONNECTEDUSER conUsers[MAX_CLNT];
ROOM rooms[MAX_ROOM];


int main(int argc, char *argv[]) {
	WSADATA wsaData;
	SOCKET serverSock,clientSock;
	SOCKADDR_IN serverAddr,clientAddr;
	int clientAddrSize;
	HANDLE hThread;
	int portNumber = PORT;
	//char port[100];

// 필요 데이터 셋업
	ReadUserAuthenticationData();   //로그인 인증용 데이터 업로드 


//  서버 소켓 준비
	if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0) //윈도우 소켓을 사용하겠다는 사실을 운영체제에 전달
		ErrorHandling("WSAStartup() error!");
	hMutex=CreateMutex(NULL,FALSE,NULL);//하나의 뮤텍스를 생성한다.
	serverSock=socket(PF_INET,SOCK_STREAM,0); //하나의 소켓을 생성한다.
	memset(&serverAddr,0,sizeof(serverAddr));
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	serverAddr.sin_port=portNumber;
// 서버 소켓 준비 끝 

// 서버 소캣 활성화  및 리슨 상태  
	if(bind(serverSock,(SOCKADDR*)&serverAddr,sizeof(serverAddr))==SOCKET_ERROR) //생성한 소켓을 배치한다.
		ErrorHandling("bind() error");
	if(listen(serverSock,10)==SOCKET_ERROR)//소켓을 준비상태에 둔다.  100은 큐에 대기 가능 수 
		ErrorHandling("listen() error");

	printf("listening...\n");

// 클라이언트 접속 대기 및 허용	및 클라이언트 별 스레드 생성 
	while(1){
		clientAddrSize=sizeof(clientAddr);
		clientSock=accept(serverSock,(SOCKADDR*)&clientAddr,&clientAddrSize);//서버에게 전달된 클라이언트 소켓을 clientSock에 전달
		WaitForSingleObject(hMutex,INFINITE);//뮤텍스 실행
		clientSocks[clientCount++]=clientSock;//클라이언트 소켓배열에 방금 가져온 소켓 주소를 전달
		ReleaseMutex(hMutex);//뮤텍스 중지
		hThread=(HANDLE)_beginthreadex(NULL,0,HandleClient,(void*)&clientSock,0,NULL);//HandleClient 쓰레드 실행, clientSock을 매개변수로 전달
		printf("Connected Client IP : %s\n",inet_ntoa(clientAddr.sin_addr));
		printf("listening...\n");
	}
	
	printf("\n\n>>Server Closed..... : \n");
	closesocket(serverSock);//생성한 소켓을 끈다.
	WSACleanup();//윈도우 소켓을 종료하겠다는 사실을 운영체제에 전달
	return 0;
}

unsigned WINAPI HandleClient(void* arg){
	SOCKET clientSock=*((SOCKET*)arg); //매개변수로받은 클라이언트 소켓을 전달
	int strLen=0,i;
	char msg[BUF_SIZE];

	while(1){ //클라이언트로부터 메시지를 받을때까지 기다린다.
	    strLen=recv(clientSock,msg,sizeof(msg),0);
	    if(strLen > 0)	MsgChecker(msg, clientSock);
	    else if(strLen==0){
	    	leftClient(clientSock);
			break;	
		} 
	    else if(strLen<0) {
	    	leftClient(clientSock);	
	    	break;
		}
	}

	return 0;
}

void leftClient(SOCKET clientSock){
	int i;
	printf("client left the chat\n");
	//이 줄을 실행한다는 것은 해당 클라이언트가 나갔다는 사실임 따라서 해당 클라이언트를 배열에서 제거해줘야함
	WaitForSingleObject(hMutex,INFINITE);//뮤텍스 실행
	for(i=0;i<clientCount;i++){//배열의 갯수만큼
		if(clientSock==clientSocks[i]){//만약 현재 clientSock값이 배열의 값과 같다면
			while(i++<clientCount-1)//클라이언트 개수 만큼
				clientSocks[i]=clientSocks[i+1];//앞으로 땡긴다.
			break;
		}
	}
	clientCount--;//클라이언트 개수 하나 감소
	ReleaseMutex(hMutex);//뮤텍스 중지
	closesocket(clientSock);//소켓을 종료한다.
	
}

void SendMsg(char *msg, int len){ //메시지를 모든 클라이언트에게 보낸다.
	int i;
	WaitForSingleObject(hMutex,INFINITE);//뮤텍스 실행
	for(i=0;i<clientCount;i++)//클라이언트 개수만큼
		send(clientSocks[i],msg,len,0);//클라이언트들에게 메시지를 전달한다.
	ReleaseMutex(hMutex);//뮤텍스 중지
}



void MsgChecker(char *str, SOCKET sock){    // 클라이언트로부터 수신된 메시지의 종류 확인 기능, 메시지 확인 후 연동 기능 함수 호출 
	char *header = strtok(str, "::");
	
    // 로그인 요청 메시지 
    if (strcmp(header, "C_LOGIN") == 0) {
        char *id = strtok(NULL, "::");
        char *pw = strtok(NULL, "::");
		//로그인 인증 작업용 함수 호출        
		LoginVerification(id, pw, sock);  
    }
    
    //회원가입 요청 메시지 
    else if (strcmp(header, "C_ADDUSER") == 0) {
        char *id = strtok(NULL, "::");
        char *pw = strtok(NULL, "::");
        //사용자 최초가입 작업용 함수 호출        
		AddUser(id, pw, sock);  
    }
    
    // 새로운 채팅룸 만들기 
	else if (strcmp(header, "C_CREATEROOM") == 0) {
		char *title = strtok(NULL, "::");
        char *temp = strtok(NULL, "::");
        int num = atoi(temp);
        CreateRoom(title, num, sock);  
    }

    // 채팅룸 리스트  
	else if (strcmp(header, "C_ROOMLIST") == 0) {
        RoomList(sock);  
    }
    
    

}


void ErrorHandling(char* msg){
	fputs(msg,stderr);
	fputc('\n',stderr);
	exit(1);
}

void ReadUserAuthenticationData(){  //유저 로그인 인증용 데이터 파일 읽기 
	FILE *fp;
	char str[41];
	fp = fopen("users.txt", "r");
	while(!feof(fp)){
		fgets( str, sizeof(str)-1, fp); 
		UploadUserAuthenticationData(str);
		numUsers++;
	}
}

void UploadUserAuthenticationData(char *str){   //파일에서 읽은 데이터를 user 구조체 배열 내 업로드 
	char* data = strtok(str, "//");
	strcpy( users[numUsers].uID, data); 
	printf("%s \n", users[numUsers].uID);
	data = strtok(NULL, "//");
	strcpy(users[numUsers].uPW, data );
	printf("%s \n", users[numUsers].uPW);
}

void LoginVerification(char *_id, char *_pw , SOCKET sock){
	int i;
	int flg=0;
	for(i=0; i<numUsers; i++) {
        if (strcmp(users[i].uID, _id) == 0 && strcmp(users[i].uPW, _pw) == 0) {
        	printf(">>server sent : S_LOGIN_RES::0::\n"); 
            send(sock, "S_LOGIN_RES::0::", strlen("S_LOGIN_RES::0::"), 0);    // 추후 메세지 생성 함수를 따로 만들어 사용할 것을 추천함 
            flg=1;
            break;
        }
        else if (strcmp(users[i].uID, _id) == 0 && strcmp(users[i].uPW, _pw) != 0) {
 //       	printf("SERVER : %s, %d\n",users[i].uPW, strlen(users[i].uPW) );
 //       	printf("Client : %s, %d\n",_pw, strlen(_pw) );
        	printf(">>server sent : S_LOGIN_RES::2::\n"); 
            send(sock, "S_LOGIN_RES::2::", strlen("S_LOGIN_RES::2::"), 0);
            flg=1;
            break;
        }
    }
	if(flg==0){
		printf(">>server sent : S_LOGIN_RES::1::\n"); 
		send(sock, "S_LOGIN_RES::1::", strlen("S_LOGIN_RES::1::"), 0);
	}
} 

void AddUser(char *_id, char *_pw , SOCKET sock){
	if(CheckDuplicationID(_id)){
		WaitForSingleObject(hMutex,INFINITE);//뮤텍스 실행		
		strcpy(users[numUsers].uID, _id);
		strcpy(users[numUsers].uPW, _pw);
		printf(">>server added new member : %s %s\n", users[numUsers].uID, users[numUsers].uPW); 
		ReleaseMutex(hMutex);//뮤텍스 중지
		numUsers++;
		printf(">>server sent : S_ADDUSER_RES::1::\n"); 
        send(sock, "S_ADDUSER_RES::1::", strlen("S_ADDUSER_RES::1::"), 0);
	}
	else{
		//아이디가 존재함을 통보 
		printf(">>server sent : S_ADDUSER_RES::2::\n"); 
        send(sock, "S_ADDUSER_RES::2::", strlen("S_ADDUSER_RES::2::"), 0);
	}
	
} 

int CheckDuplicationID(char *_id){
	int result=1;
	int i, flg=0;
	for(i=0; i<numUsers; i++){
		if(strcmp(users[i].uID, _id)==0){ //동일한 아이디가 존재하는  경우 
			result = 0;
			return result;
		}
	} 
	return result;
}

void CreateRoom(char *title, int _num, SOCKET sock){
	strcpy(rooms[numRooms].rTitle, title);
	rooms[numRooms].rSize = _num;
	numRooms++;
	printf(">>server sent : S_CREATEROOM_RES::1::\n");
	send(sock, "S_CREATEROOM_RES::1::", strlen("S_CREATEROOM_RES::1::"), 0);
}


void RoomList(SOCKET sock){
	char msg[10000];
	char roomInfo[200];
	int i;
	
	sprintf(msg, "S_ROOMLIST_RES::%d::", numRooms);
	for(i=0; i<numRooms; i++){
		sprintf(roomInfo, "%s::%d::%d::", rooms[i].rTitle, rooms[i].numJoinedUser, rooms[i].rSize);
		strcat(msg, roomInfo);
	}
	printf(">>server sent : %s\n", msg); 
    send(sock, msg, strlen(msg), 0);
}














