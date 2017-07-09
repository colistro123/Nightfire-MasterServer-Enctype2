#ifndef PLAYERS_H
#define PLAYERS_H

//General
#include <stdio.h>
#include <cstdio>
#include <cmath>
#include <math.h>
#include <conio.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <string>
#include <vector>

//Winsock
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

using namespace std;

#define RETURN_CHAR 1
#define RETURN_INT 2

#define MAX_FIELD_LIST_LEN 256
#define MAX_FILTER_LEN 511
#define NF_SECRET_KEY "S9j3L2"
#define	HANDLE_VALIDATE 1
#define HANDLE_CMP 2
#define MAX_VALIDATE_RETRIES 20

#define PBUFLEN 512
#define MAX_LENGTH 24
#define debugmode 1
#define CHALLENGE_LEN 20
#define CRYPTCHAL_LEN 10
#define SERVCHAL_LEN 25
#define MAX_DATA_SIZE 1400

class Players
{
public:
        //Constructor
        Players() { };
        //Destructor
        ~Players() { };
		int ListenOnPort(int portno);
		void processConnection(SOCKET sd);
		void SendSecureChallenge(SOCKET sd);
		bool HandleValidate(SOCKET sd, int type);
		void initVars( void );
		void setupHeader(SOCKET sd);
		void challengeKey(SOCKET sd, char* token);
		string ExplodeAndReturn(string stringToExplode, int field, string delimiter);
		vector<string> explode( const string &delimiter, const string &str);
private: //Private variables so they can only be accessed within the class
        struct sockaddr_in server, si_other;
		int slen, recv_len;
		char buf[PBUFLEN];
		WSADATA wsa;
		char challenge[CHALLENGE_LEN + 1];
		int fromip;
		int sbuffalloclen;
		unsigned char *keyptr;
		unsigned int cryptkey_enctype2[326];
		char *secretkey;
		int headerLen;
};
#endif
