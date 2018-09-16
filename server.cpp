#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <cstring>
#include <set>
#include <vector>
#include <netinet/in.h>
#include <fstream>

#define port 8001
#define bufSize 1024

using namespace std;

set<string> seedersHash;

char* processRequest(char* buf, char* addr)
{
  char buffer[bufSize];
  strcpy(buffer,buf);
  char *token;
  token = strtok (buffer," ");
  vector<char*> tokens;
  tokens.push_back(token);
  while (token != NULL)
  {
    //cout<<tokens[i++];
    token = strtok(NULL," ");
    tokens.push_back( token );
  }
  int size = tokens.size();

  if( strcmp(tokens[0],"share") == 0 )
	{
    string SHA(tokens[2]);
    if( seedersHash.find(SHA) == seedersHash.end() )
    {
      seedersHash.insert(SHA);
      char logfile[] = "logfile.txt";
      ofstream log ( logfile, ios::ate | ios::app);
      log << tokens[1] <<endl;
      log << SHA <<endl;
      log << addr;
      log << ":" << port <<endl;
      log.close();
    }
	}
  return buf;
}

int main()
{
    int sockfd;
    char buffer[bufSize];
    char* reply;
    struct sockaddr_in servAdd, cliAdd;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket failed to create");
        exit(EXIT_FAILURE);
    }

    memset(&servAdd, 0, sizeof(servAdd));
    memset(&cliAdd, 0, sizeof(cliAdd));

    servAdd.sin_family    = AF_INET; // internet address
    servAdd.sin_addr.s_addr = INADDR_ANY;
    servAdd.sin_port = htons(port);

    if ( bind(sockfd, (const struct sockaddr *)&servAdd, sizeof(servAdd)) < 0 )
    {
        perror("bind function failed");
        exit(EXIT_FAILURE);
    }

    int n,pid;
    unsigned int len;
    while(1)
    {
      n = recvfrom(sockfd, (char *)buffer, bufSize, MSG_WAITALL, ( struct sockaddr *) &cliAdd, &len);
      buffer[n] = '\0';

      char* CliSockAdd = inet_ntoa(cliAdd.sin_addr);
      char cliIP[40];
      strcpy(cliIP,CliSockAdd);
      cout<<cliIP<<endl;
      reply = processRequest( buffer, cliIP );
      cout<<buffer<<endl;
      printf("Client : %s\n", buffer);
      sendto(sockfd, (const char *)reply, strlen(reply), MSG_CONFIRM, (const struct sockaddr *) &cliAdd, len);
      printf("Reply sent.\n");
    }

    return 0;
}
