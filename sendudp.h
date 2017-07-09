#ifndef SENDUDP_H
#define SENDUDP_H

#include <stdio.h>
#include <cstdio>
#include <cmath>
#include <math.h>
#include <conio.h>
#include <iostream>
#include <time.h>
#include "players.h"

using namespace std;

#define BUFLEN					512		//Max length of buffer
#define MAX_DATA_LEN			512		//Max length of server data
#define MAX_RESPONSE_TIME		5		//5 seconds
#define MAX_CHECK_ATTEMPTS		3		//Maximum amount of times we will send packets to a server before we drop them from the table
#define MAX_SERVER_CHECKTIME	1		//20 seconds (Time interval between server checks)

#define SERVER_PORT				27900	//The port on which to listen for incoming data
#define CLIENT_PORT				28900	//The port on which to listen for incoming data

#define SERVER_IP				1
#define SERVER_QPORT			2
#define SERVER_HOSTPORT			3

extern SOCKET s;
void CreateThreads( void );

class NFSendUDP {
public:
        //Constructor
        NFSendUDP() { };
        //Destructor
        ~NFSendUDP() { };
		void initSocket( void );
		//void sendReply(char buf[512], SOCKET s, int recv_len, int slen);
		void sendReply(unsigned int binaryAddress, unsigned short port, char* data, int length, SOCKET s);
		void sendStatusPacket(unsigned int binaryAddress, unsigned short port);
		void updateServerTable(unsigned int binaryAddress, unsigned short port, char data[]);
		int IsServerOnTable( unsigned int binaryAddress, unsigned short port );
		void addServerToList(int binaryAddress, int port, char* address, char data[]);
		bool tryCatchGameData(char* data, int binaryAddress, unsigned short port);
		bool updateServerCVars(unsigned int binaryAddress, unsigned short port, char data[]);
		bool PastMaxAttempts(unsigned int binaryAddress, unsigned short port, SOCKET s);
		void RemoveServerFromTable(unsigned int binaryAddress, unsigned short port);
		bool LoopServerList( SOCKET s );
		void periodicCheck();
		SOCKET createSocket();
		int NumServersOnTable();
		string KickDataBack(int index, int data_type);
		string findParamByName(char* name, string data);
		void AddServerToListing(char* serverip, int port);
		virtual bool sentInitialServers() { return sentServers; }
		virtual void setSentInitialServers(bool value) { sentServers = value; }
		void sendServerRequests( void );
		void CheckPastMaxAttempts( void );
private: //Private variables so they can only be accessed within the class
        struct sockaddr_in server, si_other;
		int slen , recv_len;
		char buf[BUFLEN];
		WSADATA wsa;
		struct savedaddress;
		int LastCheck;
		static bool sentServers;
};
#endif
