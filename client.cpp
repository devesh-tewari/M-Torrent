#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <cstring>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <fstream>

#include "makeMtorrent.h"

using namespace std;

#define port 8001
#define bufSize 1024
#define Tracker1 "10.1.37.71"

string get_file_name_from_path(char* path)  //this function returns the last folder from an input path
{
    string str;
    str=path;
    int k=str.find_last_of("/");
    if(k==-1)  //if the input does not have any slashes
	   return path;
    int size =str.size();
    string name=str.substr(k+1,(size-k));
    //printf("\nname: %s\n",name);
    //cout <<"NAME: "<< name;
    return name;
}

string processCommand( string s )
{
  char* command = &s[0];
  //cout<<command;
  string commandName;
  string filename;
  string SHA;
  char* token;
  token = strtok (command," ");
  vector<char*> tokens;
  tokens.push_back(token);
  int i=0;
  while (token != NULL)
  {
    //cout<<tokens[i++];
    token = strtok(NULL," ");
    tokens.push_back( token );
  }
  int size = tokens.size();

  if( strcmp(tokens[0],"share") == 0 )
	{
      //for(int i=0;i<size;i++)
        //cout<<tokens[i]<<endl;
      commandName = "share";
      char* ch=tokens[1];
      //cout<<ch<<"  fd";

      SHA=get_hash(ch);
      //cout<<s;
      ofstream Mtor (tokens[2], ios_base::ate);
      Mtor << "<tracker_1_ip>:<port>" <<endl;
      Mtor << "<tracker_2_ip>:<port>" <<endl;
      Mtor << "<filename>" <<endl;
      Mtor << "<filesize in bytes>" <<endl;
      Mtor << SHA <<endl;
      Mtor.close();

      filename = get_file_name_from_path(tokens[1]);

	}
  return commandName+" "+filename+" "+SHA;


}

int main()
{
    int sockfd;
    char buffer[bufSize];
    //char hello[] = "Hello from client";


    string command;           //
    cout<<"Enter command: ";
    getline (cin, command);
    command=processCommand(command);
    cout<<command<<endl;
             //


    struct sockaddr_in servAdd;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servAdd, 0, sizeof(servAdd));

    servAdd.sin_family = AF_INET;
    servAdd.sin_port = htons(port);
    servAdd.sin_addr.s_addr = inet_addr(Tracker1);

    int n;
    unsigned int len;

    sendto(sockfd, (const char *)&command[0], strlen(&command[0]), MSG_CONFIRM, (const struct sockaddr *) &servAdd, sizeof(servAdd));
    printf("Command sent.\n");

    n = recvfrom(sockfd, (char *)buffer, bufSize, MSG_WAITALL, (struct sockaddr *) &servAdd, &len);
    buffer[n] = '\0';
    printf("Server : %s\n", buffer);

    close(sockfd);
    return 0;
}
