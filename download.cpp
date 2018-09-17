#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstring>
#include <vector>
#include <bits/stdc++.h>

using namespace std;

void download( char* clientList, char* SHA )
{
  char* token;
  token = strtok (clientList," ");
  vector<char*> clients;
  clients.push_back(token);
  while (token != NULL)
  {
    //cout<<tokens[i++];
    token = strtok(NULL," ");
    clients.push_back( token );
  }
  int no_of_clients = clients.size();

  struct sockaddr_in address;
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  //char *hello = "Hello from client";
  char buffer[1024] = {0};

  for( int i = 0; i < no_of_clients; i++ )  //actually here the clients are servevrs from which we want to download
  {
    string ip( clients[i] );
    int colon = ip.find(':');
    ip.erase( colon, strlen(&clients[i][0]) - colon );

    string port( clients[i] );
    port.erase( 0, colon+1 );

    char* IP = &ip[0];
    char* PORT = &port[0];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      printf("\n Socket creation error \n");
      return;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons( stoi(port) );

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, IP, &serv_addr.sin_addr)<=0)
    {
      printf("\nInvalid address/ Address not supported \n");
      return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }
    send(sock , SHA , strlen(SHA) , 0 );
    printf("SHA sent\n");
    valread = read( sock , buffer, 1024);
    printf("%s\n",buffer );
  }
}
