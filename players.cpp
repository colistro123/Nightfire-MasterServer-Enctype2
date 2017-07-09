#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <bitset>
#include "buffwriter.h"
//Libs
#include "sendudp.h"
#include "players.h"
#include "log.h" //Logging / Debugging, etc
//Enctype
#include "enctype2_funcs.h"
#include "gsmalg.h"
//Other libs
#include <process.h>
//Winsock
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library
#pragma comment(lib, "winmm.lib") //Pragma for the time unresolved external symbols

SOCKET tcps;
WSADATA w;

//TCP

unsigned __stdcall ClientSession(void *data) {
    SOCKET client_socket = (SOCKET)data;
	fd_set  rset;
    // Process the client.
	Players *tcp = new Players();
	FD_ZERO(&rset);
	FD_SET(client_socket, &rset);
	tcp->initVars();
	tcp->setupHeader(client_socket); //Setup the header
	tcp->SendSecureChallenge(client_socket); //Send the secure challenge just once to that specific socket
	if(tcp->HandleValidate(client_socket, HANDLE_VALIDATE)) {
		tcp->HandleValidate(client_socket, HANDLE_CMP); //Now validate CMP
	}
	delete tcp;
	return 1;
}
string RandomString(int len) {
   srand(time(0));
   string str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
   int pos;
   while(str.size() != len) {
		pos = ((rand() % (str.size() - 1)));
		str.erase (pos, 1);
   }
   return str;
}
void Players::initVars( void ) {
	keyptr = NULL; //It sends as encshare2 if it's null
	headerLen = 0;
}
void Players::SendSecureChallenge(SOCKET sd) {
	char securestring[128];
	char token[64];
	sprintf(token, "%s", RandomString(6).c_str());
	int slen = sprintf(securestring,"\\basic\\secure\\%s",token);
	//for some reason theres a null byte sent at the end so send it here too
	send(sd,securestring,slen,0);
}
bool Players::HandleValidate(SOCKET sd, int type) { //Returns 1 if it validates, 0 if it does not
	LogMSG("Validating player..\n");
	int len;
	bool validated = false;
	int i = 0;
	char recvkey[60];
	do { //20 retries
		len = recv(sd,buf,sizeof(buf),0);
		if(len != 0) {
			switch(type) {
				case HANDLE_VALIDATE: {
					if(strstr(buf, "\\validate\\")) {
						sprintf(recvkey, "%s", ExplodeAndReturn(buf, 8, "\\").c_str());
						LogMSG("Masterlist -> Client: Validated player, sending crypt headers..\n");
						challengeKey(sd, recvkey);
						validated = true;
						break;
					} 
				}
				case HANDLE_CMP: {
					if(strstr(buf,"\\cmp\\") || strstr(buf,"\\queryid\\")) {
						LogMSG("Masterlist -> Client: Validated cmp or queryid, sending server list..\n");
						processConnection(sd);
						memset(buf, 0, sizeof(buf));
						validated = true;
						break;
					}
					break;
				}
			}
			if(i++ > MAX_VALIDATE_RETRIES) {
				LogMSG("Masterlist -> Client: Couldn't validate %s\n", type == HANDLE_VALIDATE ? "\\validate\\" : "\\CMP\\");
				break;
			}
		}
	} while (!validated);
	return validated;
}
void Players::setupHeader(SOCKET sd) {
	uint32_t cryptlen = CRYPTCHAL_LEN;
	uint8_t cryptchal[CRYPTCHAL_LEN];
	uint32_t servchallen = SERVCHAL_LEN;
	uint8_t servchal[SERVCHAL_LEN];
	headerLen = (servchallen+cryptlen)+(sizeof(uint8_t)*2);	
}
void Players::processConnection(SOCKET sd) {
	char writebuff[256];
	uint32_t len = 0;
	uint8_t *p = (uint8_t *)&writebuff;
	NFSendUDP *udp = new NFSendUDP();
	udp->sendServerRequests();
	for(int i = 0; i < udp->NumServersOnTable(); i++) {
		struct sockaddr_in sa;
		uint32_t ip;
		inet_pton(AF_INET, udp->KickDataBack( i, SERVER_IP ).c_str(), &(ip));
		uint16_t queryport = (uint16_t)htons(atoi(udp->KickDataBack( i, SERVER_QPORT ).c_str()));
		BufferWriteInt((uint8_t **)&p,(uint32_t *)&len,ip);
		BufferWriteShort((uint8_t **)&p,(uint32_t *)&len,queryport);
	}
	delete udp;
	BufferWriteData((uint8_t **)&p, (uint32_t *)&len, (uint8_t *)"\\final\\", 7);
	len = enctype2_encoder((unsigned char *)NF_SECRET_KEY, (unsigned char *)&writebuff, len); //Send the header
    sendto(sd, (char *)&writebuff, len, 0, (struct sockaddr *)&sd, sizeof(struct sockaddr_in));
	closesocket(sd);
}
void Players::challengeKey(SOCKET sd, char* token) {
	gsseckey((unsigned char *)&challenge, (unsigned char *)&token, (unsigned char *)NF_SECRET_KEY, 2);
}
int Players::ListenOnPort(int portno) {
    int error = WSAStartup (0x0202, &w);   // Fill in WSA info

    if (error) {
        return false; //For some reason we couldn't start Winsock
    }

    if (w.wVersion != 0x0202) { //Wrong winsock ver?
        WSACleanup ();
        return false;
    }

    SOCKADDR_IN addr; // The address structure for a TCP socket

    addr.sin_family = AF_INET;      // Address family
    addr.sin_port = htons (portno);   // Assign port to this socket

    addr.sin_addr.s_addr = htonl (INADDR_ANY);  

    tcps = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create socket

    if (tcps == INVALID_SOCKET) {
        return false; //Don't continue if we couldn't create a //socket!!
    }

    if (bind(tcps, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR) {
       //We couldn't bind (this will happen if you try to bind to the same  
       //socket more than once)
        return false;
    }
	int on = 1;
	if(setsockopt(tcps, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))
      < 0) return NULL;

    if ( listen(tcps, SOMAXCONN) == SOCKET_ERROR) {
        LogMSG("listen failed: %d\n", WSAGetLastError());
        closesocket(tcps);
        WSACleanup();
        return 1;
    }

	//listenContinuously(tcps);
	SOCKET ClientSocket;
 
    ClientSocket = INVALID_SOCKET;
    // Accept a client socket
    //ClientSocket = accept(s, NULL, NULL);

	SOCKET client_socket;
    while ((client_socket = accept(tcps, NULL, NULL))) {
        // Create a new thread for the accepted client (also pass the accepted client socket).
		fromip = addr.sin_addr.s_addr;
        unsigned threadID;
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &ClientSession, (void*)client_socket, 0, &threadID);
    }

    if (ClientSocket == INVALID_SOCKET) {
        LogMSG("accept failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }
    getchar();
    return 0;
    //Don't forget to clean up with CloseConnection()!
}
string Players::ExplodeAndReturn(string stringToExplode, int field, string delimiter) {
	Players expPointer;
	vector<string> v = expPointer.explode(delimiter, stringToExplode);
	std::string xcontent = v[field];
	char myArray[MAX_LENGTH+1];
	return strcpy(myArray, xcontent.c_str());
}
vector<string> Players::explode( const string &delimiter, const string &str) {
    vector<string> arr;
    int strleng = str.length();
    int delleng = delimiter.length();
    if (delleng==0)
        return arr;//no change
 
    int i=0;
    int k=0;
    while( i<strleng ) {
        int j=0;
        while (i+j<strleng && j<delleng && str[i+j]==delimiter[j])
            j++;
        if (j==delleng) { //found delimiter
            arr.push_back(  str.substr(k, i-k) );
            i+=delleng;
            k=i;
        } else {
            i++;
        }
    }
    arr.push_back(  str.substr(k, i-k) );
    return arr;
}
