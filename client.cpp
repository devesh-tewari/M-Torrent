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
#include <math.h>

#include "makeMtorrent.h"
#include "download.h"
#include "seed.h"

using namespace std;

map < string, string > hashPath;  //maps shared hash string to local file path

map < string, string > hashPieces;  //pieces i have (bitmap)

#define Tracker1port 8002
#define Tracker1IP "10.1.37.71"

#define listenPort 8020

#define bufSize 1024
char paths[]="file_paths.txt";

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
      string localFilePath(tokens[1]);

      shareCommand = true;
      //for(int i=0;i<size;i++)
        //cout<<tokens[i]<<endl;
      commandName = "share";
      char* ch=tokens[1];
      //cout<<ch<<"  fd";

      SHA=get_hash(ch);
      int file_size = getFilesize( tokens[1] );
      //cout<<s;
      ofstream Mtor (tokens[2], ios_base::ate);
      Mtor << "<tracker_1_ip>:<port>" <<endl;
      Mtor << "<tracker_2_ip>:<port>" <<endl;
      Mtor << "<filename>" <<endl;
      Mtor << to_string(file_size) <<endl;
      Mtor << SHA <<endl;
      Mtor.close();

      filename = get_file_name_from_path(tokens[1]);

      SHA = SHAofSHAstr( SHA );   //SHA1 of SHA1 string
//cout<<to_string(listenPort);

      int pieces = ceil( (float)file_size / 524288.0 );
      string bitmap="";
      for(int i=0; i<pieces; i++)
        bitmap = bitmap + "1";

      hashPath[SHA] = localFilePath;
      ofstream map_file( paths , ios::app );
      map_file << SHA <<endl;
      map_file << localFilePath <<endl;
      map_file << bitmap << endl; //bitmap
      map_file.close();

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
    string line;
    ifstream path_file(paths);
    while ( getline (path_file,line) )
    {
      SHA = line;
      getline (path_file,line);  //path of that SHA
      hashPath[SHA] = line;
      getline (path_file,line);  //pieces bitmap
      hashPieces[SHA] = line;
    }
    path_file.close();

    int sockfd;
    char buffer[bufSize];
    //char hello[] = "Hello from client";

    while(1)
{

    getCommand = false;
    shareCommand = false;
    string command;           //
    cout<<"Enter command: ";
    getline (cin, command);
    command = processCommand(command);
    /*if( command == "exit" )
    {

    }*/

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
      pollPieces( buffer, &SHA[0] );
    }

    int pid;
    if( shareCommand )   //kill(pid, SIGKILL);/* or */kill(pid, SIGTERM); to stop seeding
    {
      pid = fork();
      if(pid==0)
      {
        seed( listenPort );
        exit(1);
      }
    }
    close(sockfd);

}
    return 0;
}
