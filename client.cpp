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
#include "download.h"

using namespace std;

#define Tracker1port 8002
#define Tracker1IP "10.1.37.71"

#define listenPort 8020

#define bufSize 1024

bool getCommand;
bool shareCommand;

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

string SHA;
string processCommand( string s )
{
  char* command = &s[0];
  //cout<<command;
  string commandName;
  string filename;

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
      shareCommand = true;
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

      SHA = SHAofSHAstr( SHA );   //SHA1 of SHA1 string
cout<<to_string(listenPort);
      return commandName + " " + filename + " " + SHA + " " + to_string(listenPort);

	}

  else if( strcmp(tokens[0],"get") == 0 )
	{
    getCommand = true;
    commandName = "seederlist";

    ifstream Mtor ( tokens[1] );

    getline ( Mtor,SHA );
    getline ( Mtor,SHA );
    getline ( Mtor,SHA );
    getline ( Mtor,SHA );
    getline ( Mtor,SHA );  //this is the SHA concatenated string
    Mtor.close();

    SHA = SHAofSHAstr( SHA );  //SHA of SHA concatenated string

    return commandName + " " + SHA;

  }


}

int main()
{
    int sockfd;
    char buffer[bufSize];
    //char hello[] = "Hello from client";

                              /*make seederlist command*/
    getCommand = false;
    shareCommand = false;
    string command;           //
    cout<<"Enter command: ";
    getline (cin, command);
    command = processCommand(command);
    //cout<<command<<endl;
             //


    struct sockaddr_in servAdd;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servAdd, 0, sizeof(servAdd));

    servAdd.sin_family = AF_INET;
    servAdd.sin_port = htons(Tracker1port);
    servAdd.sin_addr.s_addr = inet_addr(Tracker1IP);

    int n;
    unsigned int len;

    sendto(sockfd, (const char *)&command[0], strlen(&command[0]), MSG_CONFIRM, (const struct sockaddr *) &servAdd, sizeof(servAdd));
    printf("Command sent.\n");

    n = recvfrom(sockfd, (char *)buffer, bufSize, MSG_WAITALL, (struct sockaddr *) &servAdd, &len);
    buffer[n] = '\0';
    printf("Server : %s\n", buffer);

    if ( getCommand )
    {
      download( buffer, &SHA[0] );
    }

    if( shareCommand )
    {
      int pid = fork();
      if(pid==0)
        seed( SHA );
    }
    close(sockfd);
    return 0;
}
