//轎籀: http://remocon33.tistory.com/465
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <process.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

unsigned WINAPI SendMsg(void* arg);//噙溯萄 瞪歎л熱
unsigned WINAPI RecvMsg(void* arg);//噙溯萄 熱褐л熱
void ErrorHandling(char* msg);


void ShowMainMenu(SOCKET sock);
int InitialMenu(SOCKET sock);
void ShowSystemTitle();
void ShowLobbyTitle();
void exitService(SOCKET sock);
int LoginProcedure(SOCKET sock);
int AddUserProcedure(SOCKET sock);
int CreateRoomProcedure(SOCKET sock);
void ShowRoomListTitle();
int RoomListProcedure(SOCKET sock);


char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
int loggedIn = 0;
	
	
int main(){
	WSADATA wsaData;
	SOCKET sock;
	SOCKADDR_IN serverAddr;
	HANDLE sendThread,recvThread;

	char serverIP[100] = "127.0.0.1";  //localhost
	int port =55555;


	if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){    // 孺紫辦 模鰍擊 餌辨и棻堅 遴艙羹薯縑 憲葡
		ErrorHandling("WSAStartup() error!");
	}
	sock=socket(PF_INET,SOCK_STREAM,0);//模鰍擊 ж釭 儅撩и棻.
	memset(&serverAddr,0,sizeof(serverAddr));
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_addr.s_addr=inet_addr(serverIP);
	serverAddr.sin_port=port;

	if(connect(sock,(SOCKADDR*)&serverAddr,sizeof(serverAddr))==SOCKET_ERROR){    //憮幗縑 蕾樓и棻.
		ErrorHandling("connect() error");
	}
	

	loggedIn = InitialMenu(sock);
	if (loggedIn==1){
		while(1){
			ShowMainMenu(sock);	
		}
	}
	
	
/*
	//蕾樓縑 撩奢ж賊 檜 還 嬴楚陛 褒ч脹棻.
	sendThread=(HANDLE)_beginthreadex(NULL,0,SendMsg,(void*)&sock,0,NULL);//詭衛雖 瞪歎辨 噙溯萄陛 褒ч脹棻.
	recvThread=(HANDLE)_beginthreadex(NULL,0,RecvMsg,(void*)&sock,0,NULL);//詭衛雖 熱褐辨 噙溯萄陛 褒ч脹棻.

	WaitForSingleObject(sendThread,INFINITE);//瞪歎辨 噙溯萄陛 醞雖腆陽梱雖 晦棻萼棻./
	WaitForSingleObject(recvThread,INFINITE);//熱褐辨 噙溯萄陛 醞雖腆陽梱雖 晦棻萼棻.
	//贗塭檜樹お陛 謙猿蒂 衛紫и棻賊 檜還 嬴楚陛 褒ч脹棻.
*/

	

	return 0;
}

unsigned WINAPI SendMsg(void* arg){//瞪歎辨 噙溯萄л熱
	SOCKET sock=*((SOCKET*)arg);//憮幗辨 模鰍擊 瞪殖и棻.
	char nameMsg[NAME_SIZE+BUF_SIZE];
	while(1){//奩犒
		fgets(msg,BUF_SIZE,stdin);//殮溘擊 嫡朝棻.
		if(!strcmp(msg,"q\n")){//q蒂 殮溘ж賊 謙猿и棻.
			send(sock,"q",1,0);//nameMsg蒂 憮幗縑啪 瞪歎и棻.
		}
		sprintf(nameMsg,"%s %s",name,msg);//nameMsg縑 詭衛雖蒂 瞪殖и棻.
		send(sock,nameMsg,strlen(nameMsg),0);//nameMsg蒂 憮幗縑啪 瞪歎и棻.
	}
	return 0;
}

unsigned WINAPI RecvMsg(void* arg){
	SOCKET sock=*((SOCKET*)arg);//憮幗辨 模鰍擊 瞪殖и棻.
	char nameMsg[NAME_SIZE+BUF_SIZE];
	int strLen;
	while(1){//奩犒
		strLen=recv(sock,nameMsg,NAME_SIZE+BUF_SIZE-1,0);//憮幗煎睡攪 詭衛雖蒂 熱褐и棻.
		if(strLen==-1)
			return -1;
		nameMsg[strLen]=0;//僥濠翮曖 部擊 憲葬晦 嬪п 撲薑
		if(!strcmp(nameMsg,"q")){
			printf("left the chat\n");
			closesocket(sock);
			exit(0);
		}
		fputs(nameMsg,stdout);//濠褐曖 夔樂縑 嫡擎 詭衛雖蒂 轎溘и棻.
	}
	return 0;
}

void ErrorHandling(char* msg){
	fputs(msg,stderr);
	fputc('\n',stderr);
	exit(1);
}

void exitService(SOCKET sock) {
//    char msg[50] = "C_DISCONNECT::";
//    send(sock, msg, strlen(msg), 0);   // 憮幗縑啪 謙猿 詭衛雖 瞪歎 (褫暮)
    
    closesocket(sock);  // 模鰍 殘晦
    WSACleanup();       // 孺樓 謙猿
    printf(">End of Service.\n");
    exit(0);            // Щ煎斜極 謙猿
}

// 煎斜檣 瞰離 籀葬  л熱
int LoginProcedure(SOCKET sock) {
    char id[20];
    char pw[20];
    char ch;
    int i;
    char msg[100];
    char response[100];
    int result=0;

     
        i = 0;

        printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式 \n");
        printf("             LOG IN           \n");
        printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式 \n");

        printf("> Enter your ID: ");
        scanf("%s", id);  // 餌辨濠 嬴檜蛤 殮溘 
        getchar(); // 殮溘 幗ぷ 薯剪
        printf("> Enter your Password: ");
        while (1) {
            ch = getch();
            if (ch == 13) {
                pw[i] = '\0';
                printf("\n");
                break;
            } else {
                if (i < sizeof(pw) - 1) {
                    printf("*");
                    pw[i++] = ch;
                }
            }
        }

        sprintf(msg, "C_LOGIN::%s::%s::", id, pw);
        printf("%s\n", msg);
        send(sock, msg, strlen(msg), 0);

        int len = recv(sock, response, sizeof(response) - 1, 0);
        char *header = strtok(response, "::");
	    if (strcmp(header, "S_LOGIN_RES") == 0) {
	        char *res = strtok(NULL, "::");
	        result = atoi(res);  
    	}
		else {
			result = -1;
		}
        
    return result;
}

int AddUserProcedure(SOCKET sock){
    char id[20];
    char pw[20];
    char ch;
    int i;
    char msg[100];
    char response[100];
    int result=0;

     
        i = 0;

        printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式 \n");
        printf("             JOIN MEMBER           \n");
        printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式 \n");

        printf("> Enter new ID: ");
        scanf("%s", id);  // 餌辨濠 嬴檜蛤 殮溘 
        getchar(); // 殮溘 幗ぷ 薯剪
        printf("> Enter new Password: ");
        while (1) {
            ch = getch();
            if (ch == 13) {
                pw[i] = '\0';
                printf("\n");
                break;
            } else {
                if (i < sizeof(pw) - 1) {
                    printf("*");
                    pw[i++] = ch;
                }
            }
        }

        sprintf(msg, "C_ADDUSER::%s::%s::", id, pw);
        //printf("%s\n", msg);
        send(sock, msg, strlen(msg), 0);

        int len = recv(sock, response, sizeof(response) - 1, 0);
        char *header = strtok(response, "::");
	    if (strcmp(header, "S_ADDUSER_RES") == 0) {
	        char *res = strtok(NULL, "::");
	        result = atoi(res);  
    	}
		else {
			result = -1;
		}
        
    return result;	
	
}



void ShowSystemTitle(){
		printf("\n");
	    printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式\n");
	    printf("            SMU Devision of Computer Engineering Chat Service System   \n");
	    printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式\n");	
	    printf("\n");
}
void ShowLobbyTitle(){
	printf("\n");
	printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式\n");
	printf("         [MAIN LOBBY]\n");
	printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式\n");
	printf("\n");
}

void ShowRoomListTitle(){
	printf("\n");
	printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式\n");
	printf("         [CHATTING ROOMS  (Title / Joined User Num  / Max of User Num)]\n");
	printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式\n");
	printf("\n");
}


int InitialMenu(SOCKET sock){
	int cmd;
	int result;
	while(1){
		ShowSystemTitle();
		
		printf("\n");
		printf("1.LOG IN\n");
		printf("2.JOIN MEMBER\n");
		printf("3.QUIT SERVICE\n");
		printf(">Enter Command Number : ");
		scanf("%d", &cmd);
		getchar();
		
		switch (cmd) {
	            case 1:
	                result = LoginProcedure(sock);
	                if(result==0){
	                	printf(">Successful Login\n");
	                	return 1;
					}
					else if(result==1){
						printf(">Invalid ID\n");
					}
					else if(result==2){
						printf(">Invalid Password\n");
					}
					else{
						printf(">FAIL of LOGIN PROCEDURE\n");
					}
	                break;

	            case 2:
	                result = AddUserProcedure(sock);
	                if(result==1){
	                	printf(">Successful add your information\n");
	                	
					}
					else if(result==2){
						printf(">Duplicated ID.\n");
					}
					else{
						printf(">FAIL of ADD USER PROCEDURE\n");
					}
	                break;


	            case 3:
	                exitService(sock);
	                break;
	                
	            default:
	                printf("Invalid Command Number. Enter a valid command.\n");
	    }	
	}
	return -1;
}


void ShowMainMenu(SOCKET sock){
	int cmd;
	int result;

	
		ShowLobbyTitle();
		printf("\n");
		printf("1.RELOAD CHAT ROOM LIST\n");
		printf("2.JOIN CHAT ROOM\n");
		printf("3.CREATE CHAT ROOM\n");
		printf("4.QUIT SERVICE\n");
		printf(">Enter Command Number : ");
		scanf("%d", &cmd);
		getchar();
		
		switch (cmd) {
				case 1:
					result = RoomListProcedure(sock);
					if (result==1) printf(">Successfully Received the list\n");
					else printf(">Fail of Reload Room List\n");
					break;
				
				case 2:
					// Join to the Chat Room 
					break;

				case 3:
					result = CreateRoomProcedure(sock);
					if (result==1) printf(">Created New Room\n");
					else printf(">Fail of New Room Creation\n");
					break;
				
				case 4:
	                exitService(sock);
	                break;
	                
	            default:
	                printf("Invalid Command Number. Enter a valid command.\n");
		}
	
}

int CreateRoomProcedure(SOCKET sock) {
    char roomName[101];
    char msg[100];
    char response[100];
    int num;
    int result;

    printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式 \n");
    printf("          Create New Chat Room          \n");
    printf("式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式 \n");

    printf("> Enter Chat Room Title: ");
    gets(roomName);
    printf("> Enter Number of Available Chat Room Users: ");
    scanf("%d", &num);
    getchar();

    sprintf(msg, "C_CREATEROOM::%s::%d::", roomName, num);
    send(sock, msg, strlen(msg), 0);

	int len = recv(sock, response, sizeof(response) - 1, 0);
    char *header = strtok(response, "::");
	if (strcmp(header, "S_CREATEROOM_RES") == 0) {
	    char *res = strtok(NULL, "::");
	    result = atoi(res);  
	}
	else {
		result = -1;
	}
        
    return result;	
}


int RoomListProcedure(SOCKET sock){
    char msg[100];
    char response[10000];
	int nroom;
    int i;
    int result;
	
	
	//憮幗煎 瓣た瑛 葬蝶お 薑爾 蹂羶
	sprintf(msg, "C_ROOMLIST::");
    send(sock, msg, strlen(msg), 0);
	//憮幗煎睡攪 嫡擎 葬蝶お 薑爾 轎溘
	int len = recv(sock, response, sizeof(response) - 1, 0);
	char *header = strtok(response, "::");
	if (strcmp(header, "S_ROOMLIST_RES") == 0) {
		
	    char *res = strtok(NULL, "::");
	    nroom = atoi(res); 
	    ShowRoomListTitle();
	    
	    for(i=0; i<nroom; i++){
	    	char *tempTitle = strtok(NULL, "::");
	    	char *temp1 = strtok(NULL, "::");
	    	char *temp2 = strtok(NULL, "::");
	    	printf("%d\t%s / %s / %s\n", i+1, tempTitle, temp1, temp2);
		}
		result = 1;
	}
	else {
		result = -1;
	}
        
    return result;	
	
}

