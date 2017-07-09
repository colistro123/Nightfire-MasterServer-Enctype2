#include "sendudp.h"
#include "log.h"
#include <iostream>
#include <tlhelp32.h>
#include <vector>

//Winsock
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

//Other libs
#include "Ws2tcpip.h"

//UINT
#include <stdint.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library
#pragma comment(lib, "winmm.lib") //Pragma for the time unresolved external symbols

bool NFSendUDP::sentServers = false;

typedef struct _ServerList {
	unsigned int	binaryaddress;
	char			ip[64];
	short			queryport;
	char			serverdata[512];
	int				LastAnswerTime;
	short			hostport;
	int				checkAttempts;
} ServerList;
std::vector<_ServerList> servers;

typedef struct {
	char*	ip;
	short	queryport;
} ServerBuffer;

//To manually add some servers that may not be listed...
ServerBuffer serverData[] = {
	{"127.0.0.1", 6550}
};

SOCKET s;

DWORD WINAPI ThreadFunc(LPVOID args);
DWORD WINAPI ThreadFuncTCP(LPVOID args);

int main() {
	#if debugmode
    cout<<"int main called\n"<<endl;
	#endif
	NFSendUDP *cPointer = new NFSendUDP();
	CreateThreads();
    cPointer->initSocket();
	delete cPointer;
}
void CreateThreads( void ) {
	DWORD dwid, dwid2;
	DWORD  dwThreadID;
	HANDLE h = CreateThread(NULL,NULL,LPTHREAD_START_ROUTINE(ThreadFunc),NULL,NULL,&dwid);
	HANDLE s = CreateThread(NULL,NULL,LPTHREAD_START_ROUTINE(ThreadFuncTCP),NULL,NULL,&dwid2);
	CloseHandle(h);
	CloseHandle(s);
}
DWORD WINAPI ThreadFunc(void* data) {
	NFSendUDP *cPointer = new NFSendUDP();
	cPointer->sendServerRequests();
	cPointer->periodicCheck(); //Do our server list periodic check
	delete cPointer;
	return 1;
}
DWORD WINAPI ThreadFuncTCP(void* data) {
	Players *tcp = new Players();
	tcp->ListenOnPort(CLIENT_PORT);
	delete tcp;
	return 1;
}
void NFSendUDP::sendServerRequests( void ) {
	while(!sentInitialServers()) { //If the initial servers weren't sent
		if(s) {
			for(int i=0;i<sizeof(serverData)/sizeof(ServerBuffer);i++) {
				AddServerToListing(serverData[i].ip, serverData[i].queryport);
			}
			setSentInitialServers(true);
		}
	}
}
void NFSendUDP::periodicCheck() {
	for(;;) {
		if(NumServersOnTable() > 0) {
			int time = floor((timeGetTime() - LastCheck)/1000);
			if(time >= MAX_SERVER_CHECKTIME) {
				LoopServerList(s);
				LastCheck = timeGetTime();
			}
		}
	}
}
SOCKET NFSendUDP::createSocket() {
	//Create a socket
	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
		printf("Could not create socket : %d\n" , WSAGetLastError());
	}
	printf("Socket created.\n");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( SERVER_PORT );
	//Bind
	if( bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR) {
		printf("Bind failed with error code : %d\n" , WSAGetLastError());
		//exit(EXIT_FAILURE);
	}
	puts("Bind done");
	return (SOCKET)s;
}
void NFSendUDP::initSocket( void ) {
	slen = sizeof(si_other);
	//Initialise winsock
	printf("\nInitialising Winsock...\n");
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
		printf("Failed. Error Code : %d\n",WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");
	
	s = createSocket();
	//keep listening for data
	while(1) {
		fflush(stdout);
		memset(buf,'\0', BUFLEN); //clear the buffer
		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR) {
			printf("recvfrom() failed with error code : %d\n" , WSAGetLastError());
			continue;
		}
		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		//Check if the server is past max attempts, if it is, delete it
		tryCatchGameData(buf, inet_addr(inet_ntoa(si_other.sin_addr)), ntohs(si_other.sin_port)); //Try to catch the game data (if any) and if it's not past max attempts
		//Send status to the server to get the details and add it to the table
		sendReply(inet_addr(inet_ntoa(si_other.sin_addr)), ntohs(si_other.sin_port), buf, recv_len, s);
	}
}
void NFSendUDP::AddServerToListing(char* serverip, int port) {
	unsigned long binaddr = inet_addr(serverip);
	addServerToList(binaddr, port, serverip, "");
	LogMSG("Masterlist: Added server %s:%d to the list\n", serverip, port);
}
void NFSendUDP::sendReply(unsigned int binaryAddress, unsigned short port, char* data, int length, SOCKET s) {
	LogMSG("Masterlist: sendReply(%d, %d, data, %d)\n", binaryAddress, port, length);
	if (length >= 11) {			
		switch(port) {
			case CLIENT_PORT: {
				LogMSG("Masterlist: Received the following data: %s", data);
				break;
			}
			default: {
				if(strstr(data, "\\heartbeat\\")) {
					if(strstr(data,"\\gamename\\jbnightfire\\")) {
						if(!IsServerOnTable( binaryAddress, port )) { //Only send the packet if the server is not on the table, otherwise we will get into an infinite loop, resulting in spam
							sendStatusPacket(binaryAddress, port);
						}
					}
				}
				break;
			}
		}
	}
}
void NFSendUDP::sendStatusPacket(unsigned int binaryAddress, unsigned short port) {
	in_addr in;
	in.s_addr = binaryAddress;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = binaryAddress;
	sendto(s, "\\status\\", sizeof("\\status\\"), 0, (sockaddr*)&server, sizeof(struct sockaddr));
}
bool NFSendUDP::tryCatchGameData(char* data, int binaryAddress, unsigned short port) {
	if(strstr(data,"\\gamename\\jbnightfire\\gamever\\")) {
		updateServerTable(binaryAddress, port, data);
		return true;
	}
	return false;
}
void NFSendUDP::updateServerTable(unsigned int binaryAddress, unsigned short port, char data[]) {
	in_addr in;
	in.s_addr = binaryAddress;
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	si_other.sin_addr.s_addr = binaryAddress;
	if(!IsServerOnTable(binaryAddress, port)) {
		addServerToList(binaryAddress, port, inet_ntoa(si_other.sin_addr), data);
	} else { //If it is
		if(updateServerCVars(binaryAddress, port, data)) { //Update the data string, so the lists are up to date
			LogMSG("Masterlist: Succesfully updated the server CVars for server %s:%d!\n", inet_ntoa(si_other.sin_addr), port);
		} else {
			LogMSG("Masterlist: Failed to update the server CVars for server %s:%d\n", inet_ntoa(si_other.sin_addr), port);
		}
	}
}
bool NFSendUDP::updateServerCVars(unsigned int binaryAddress, unsigned short port, char data[]) {
	int i = 0;
	for (std::vector<_ServerList>::iterator it = servers.begin(); it != servers.end(); it++) { //Loop through the servers..
		if(binaryAddress == it->binaryaddress) {
			if(port == it->queryport) { //Update the data if it matches this specific server..
				strcpy(it->serverdata, data);
				it->LastAnswerTime = timeGetTime();
				it->checkAttempts = 0; //Back to normal
				return true;
			}
		}
	}
	return false;
}
int NFSendUDP::IsServerOnTable( unsigned int binaryAddress, unsigned short port ) {
	std::vector<_ServerList>::iterator it = servers.begin();
	_ServerList server;
	while(it != servers.end()) {
		server = *(it++);
		if(server.binaryaddress == binaryAddress) {
			if(server.queryport == port) {
				return 1;
			}
		}
	}
	return 0;
}
void NFSendUDP::addServerToList(int binaryAddress, int port, char* address, char data[]) {
	_ServerList new_server;
	memset(&new_server, 0, sizeof(ServerList));

	if(strlen(data) > MAX_DATA_LEN) {
		LogMSG("Masterlist: The server %s:%d exceeds the max data length of %d, couldn't add it!\n", address, port, MAX_DATA_LEN);
		return;
	}
	strcpy(new_server.ip, address);
	new_server.queryport = port;
	new_server.binaryaddress = binaryAddress;
	new_server.LastAnswerTime = timeGetTime();
	new_server.hostport = (short)atoi(findParamByName("hostport", data).c_str());
	strcpy(new_server.serverdata, data);
	servers.push_back(new_server);
	LogMSG("Masterlist: Server: Address: %s:%d added!\n", address, port);
}
void NFSendUDP::RemoveServerFromTable(unsigned int binaryAddress, unsigned short port) {
	in_addr in;
	in.s_addr = binaryAddress;
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	si_other.sin_addr.s_addr = binaryAddress;
	for (std::vector<_ServerList>::iterator it = servers.begin(); it != servers.end(); it++) {
		if(it->binaryaddress == binaryAddress) {
			if(it->queryport == port) {
				it = servers.erase(it);
				LogMSG("Masterlist: Server: Address: %s:%d removed!\n", inet_ntoa(si_other.sin_addr), port);
				break;
			}
		}
		break;
	}
}
bool NFSendUDP::PastMaxAttempts(unsigned int binaryAddress, unsigned short port, SOCKET s) {
	double time;
	for (std::vector<_ServerList>::iterator it = servers.begin(); it != servers.end(); it++) {
		if(it->binaryaddress == binaryAddress) {
			if(it->queryport == port) {
				time = floor((timeGetTime() - it->LastAnswerTime)/1000);
				if(time > MAX_RESPONSE_TIME) {
					if(it->checkAttempts > MAX_CHECK_ATTEMPTS) {
						return true;
					}
				}
			}
		}
	}
	return false;
}
bool NFSendUDP::LoopServerList( SOCKET s ) {
	for (std::vector<_ServerList>::iterator it = servers.begin(); it != servers.end(); it++) {
		sendStatusPacket(it->binaryaddress, it->queryport); //Send the status packet first
		it->checkAttempts++;
	}
	CheckPastMaxAttempts(); //Now check if all the servers aren't past the max attempts...
	return false;
}
void NFSendUDP::CheckPastMaxAttempts( void ) {
	for (std::vector<_ServerList>::iterator it = servers.begin(); it != servers.end(); it++) {
		LogMSG("Checking if server %s:%d isn't past max attempts\n", it->ip, it->queryport);
		if(PastMaxAttempts(it->binaryaddress, it->queryport, s)) {
			RemoveServerFromTable(it->binaryaddress, it->queryport); //Delete the server
			break;
		}
	}
}
string NFSendUDP::KickDataBack( int index, int data_type ) {
	auto it = std::next(servers.begin(), index);
	_ServerList server;
	string qbuf;
	while(it != servers.end()) {
		server = *(it++); //Make a copy of the iterator
		switch(data_type) {
			case SERVER_IP: {
				struct sockaddr_in antelope;
				uint32_t ip = inet_addr(server.ip); 
				sprintf((char *)qbuf.c_str(), "%s", server.ip);
				return qbuf.c_str();
			}
			case SERVER_QPORT: {
				sprintf((char *)qbuf.c_str(), "%d", server.queryport);
				return qbuf.c_str();
			}
			case SERVER_HOSTPORT: {
				sprintf((char *)qbuf.c_str(), "%d", server.hostport);
				return qbuf.c_str();
			}
		}
		break;
	}
	return 0;
}
string NFSendUDP::findParamByName(char* name, string data) {
	Players *playerClass = new Players();
	string buffstr;
	int offset = 0;
	string retstr;
	int found = 0;
	offset = (int)count(data.begin(), data.end(), '\\');
	for(int i=0; i<offset; i++) {
		strcpy((char *)buffstr.c_str(), (char *)playerClass->ExplodeAndReturn(data, i, "\\").c_str());
		if(strcmp(name, buffstr.c_str()) == 0) {
			sprintf((char *)retstr.c_str(), "%s", playerClass->ExplodeAndReturn(data, i+1, "\\").c_str());
			found = 1;
			break;
		}
	}
	delete playerClass;
	return found == 1 ? (char *)retstr.c_str() : (char *)"Parameter not found";
}
int NFSendUDP::NumServersOnTable() {
	std::vector<_ServerList>::iterator it = servers.begin();
	_ServerList server;
	int i = 0;
	while(it != servers.end()) {
		server = *(it++);
		i++;
	}
	return i;
}