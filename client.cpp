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
#include <thread>

#include "makeMtorrent.h"
#include "download.h"
#include "seed.h"

using namespace std;

map < string, string > hashPath;  //maps shared hash string to local file path

map < string, string > hashPieces;  //pieces i have (bitmap)

int listenPort;

#define bufSize 1024
char paths[]="file_paths.txt";

bool getCommand;
bool shareCommand;
bool removeCommand;
map < string, bool > Remove;  // which mtorrent files are seeding or removed
map < string, string > downloads;  //maps an SHA to download status
bool firstShare = true ;
string dest;

bool tracker_alive=false;

string tracker1Sock;
string tracker2Sock;
string mySock;

char TRACKER_1_IP[50];
char TRACKER_1_PORT[20];

char TRACKER_2_IP[50];
char TRACKER_2_PORT[20];

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
      if( tokens[2] == NULL )
      {
          cout<<"FAILURE:INVALID_ARGUMENTS"<<endl;
          return "DummyCommand";
      }
      if( fileExists( (const char*)tokens[1]) == false )
      {
          cout<<"FAILURE:FILE_NOT_FOUND"<<endl;
          return "DummyCommand";
      }
      string localFilePath(tokens[1]);

      //for(int i=0;i<size;i++)
        //cout<<tokens[i]<<endl;
      commandName = "share";
      char* ch=tokens[1];
      //cout<<ch<<"  fd";

      SHA=get_hash(ch);
      int file_size = getFilesize( tokens[1] );
      //cout<<s;
      string MtorName(tokens[2]);
      if(MtorName.substr(MtorName.find_last_of(".") + 1) != "mtorrent")  //delete the mtorrent file
      {
          cout<<"FAILURE:INVALID_ARGUMENTS"<<endl;
          return "DummyCommand";
      }

      filename = get_file_name_from_path(tokens[1]);

      ofstream Mtor (tokens[2], ios_base::ate);
      Mtor << tracker1Sock <<endl;
      Mtor << tracker2Sock <<endl;
      Mtor << filename <<endl;
      Mtor << to_string(file_size) <<endl;
      Mtor << SHA <<endl;
      Mtor.close();

      SHA = SHAofSHAstr( SHA );   //SHA1 of SHA1 string
      Remove[SHA] = false;

      int pieces = ceil( (float)file_size / 524288.0 );
      string bitmap="";
      for(int i=0; i<pieces; i++)
        bitmap = bitmap + "1";

      hashPieces[SHA] = bitmap;

      hashPath[SHA] = localFilePath;

      ofstream map_file( paths , ios::app );
      map_file << SHA <<endl;
      map_file << localFilePath <<endl;
      map_file << bitmap << endl; //bitmap
      map_file.close();

      cout<<"SUCCESS:"<<filename<<".mtorrent"<<endl;

      shareCommand = true;

      return commandName + " " + filename + " " + SHA + " " + mySock;

	}

  else if( strcmp(tokens[0],"get") == 0 )
	{
    commandName = "seederlist";

    if( tokens[2] == NULL || tokens[1] == NULL )
    {
        cout<<"FAILURE:INVALID_ARGUMENTS"<<endl;
        return "DummyCommand";
    }
    string MtorName(tokens[1]);
    if(MtorName.substr(MtorName.find_last_of(".") + 1) != "mtorrent")  //delete the mtorrent file
    {
        cout<<"FAILURE:INVALID_ARGUMENTS"<<endl;
        return "DummyCommand";
    }
    if( fileExists( (const char*)tokens[1]) == false )
    {
        cout<<"FAILURE:MTORRENT_FILE_NOT_FOUND"<<endl;
        return "DummyCommand";
    }
    if( fileExists( (const char*)tokens[2]) == true )
    {
        cout<<"FAILURE:ALREADY_EXISTS"<<endl;
        return "DummyCommand";
    }
    string tem(tokens[2]);
    dest = tem;
    ifstream Mtor ( tokens[1] );

    getline ( Mtor,SHA );
    getline ( Mtor,SHA );
    getline ( Mtor,SHA );
    string filename = SHA;
    getline ( Mtor,SHA );
    getline ( Mtor,SHA );  //this is the SHA concatenated string
    //cout<<"sad: "<<SHA<<endl;
    Mtor.close();

    SHA = SHAofSHAstr( SHA );  //SHA of SHA concatenated string

    downloads[SHA] = "[D] " + filename;

    hashPath[SHA] = tem;

    getCommand = true;
    cout<<"SUCCESS:FILE_PATH_IN_CURRENT_CLIENT"<<endl;

    return commandName + " " + SHA;

  }

  else if( strcmp(tokens[0],"remove") == 0 && tokens[1]!=NULL )
	{
    if( tokens[1] == NULL )
    {
        cout<<"FAILURE:INVALID_ARGUMENTS"<<endl;
        return "DummyCommand";
    }
    string MtorName(tokens[1]);
    if(MtorName.substr(MtorName.find_last_of(".") + 1) != "mtorrent")  //delete the mtorrent file
    {
        cout<<"FAILURE:INVALID_ARGUMENTS"<<endl;
        return "DummyCommand";
    }

    if( fileExists( (const char*)tokens[1]) == false )
    {
        cout<<"FAILURE:FILE_NOT_FOUND"<<endl;
        return "DummyCommand";
    }

    commandName = "remove";

    ifstream Mtor ( tokens[1] );

    getline ( Mtor,SHA );
    getline ( Mtor,SHA );
    getline ( Mtor,SHA );
    getline ( Mtor,SHA );
    getline ( Mtor,SHA );  //this is the SHA concatenated string
    Mtor.close();

    SHA = SHAofSHAstr( SHA );  //SHA of SHA concatenated string

    Remove[SHA] = true;

    string mtorr(tokens[1]);
    if(mtorr.substr(mtorr.find_last_of(".") + 1) == "mtorrent")  //delete the mtorrent file
      remove(tokens[1]);

    cout<<"SUCCESS:FILE_REMOVED"<<endl;

    return commandName + " " + SHA + " " + mySock;

  }


}

void showDownloads()
{
    if( downloads.empty() )
    {
        cout<<"No current downloads"<<endl;
    }
    for( auto i = downloads.begin(); i != downloads.end(); i++ )
      cout<< i->second <<endl;
}

void checkAlive(int sockfd)
{
    char buffer [1024];
    struct sockaddr_in otherServAdd;
    memset(&otherServAdd, 0, sizeof(otherServAdd));

    otherServAdd.sin_family = AF_INET;  // internet address
    otherServAdd.sin_port = htons( atoi(TRACKER_1_PORT) );
    otherServAdd.sin_addr.s_addr = inet_addr(TRACKER_1_IP);

    char reqTracker[] = "alive?";
    unsigned int len;

    struct timeval tv;

    sendto(sockfd, (const char*)reqTracker, strlen(reqTracker), MSG_CONFIRM, (const struct sockaddr*)&otherServAdd, sizeof(otherServAdd));

    tv.tv_sec = 0;
    tv.tv_usec = 800000;  //0.8 seconds
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
    {
        perror("setsockopt Error");
    }

    recvfrom(sockfd, (char *)buffer, bufSize, MSG_WAITALL, (struct sockaddr *) &otherServAdd, &len);
    if( strcmp(buffer,"alive") == 0 )
    {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
        {
            perror("setsockopt Error");
        }
        tracker_alive = true;
        return;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
    {
        perror("setsockopt Error");
    }

    cout<<"Tend"<<tracker_alive<<endl;
    return;
}

int main( int argc, char** argv )
{
    if( argc < 4 )
    {
        cout<<"Insuffecient Arguments"<<endl;
        exit(1);
    }

    string myIP( argv[1] );
    mySock = myIP;
    int colon = myIP.find(':');
    myIP.erase( colon, strlen(argv[1]) - colon );

    string myListenPORT( argv[1] );
    myListenPORT.erase( 0, colon+1 );

    char* MY_IP = &myIP[0];
    char* MY_LISTEN_PORT = &myListenPORT[0];


    string tracker1IP( argv[2] );
    tracker1Sock = tracker1IP;
    colon = tracker1IP.find(':');
    tracker1IP.erase( colon, strlen(argv[2]) - colon );

    string tracker1PORT( argv[2] );
    tracker1PORT.erase( 0, colon+1 );

    strcpy( TRACKER_1_IP , &tracker1IP[0] );
    strcpy( TRACKER_1_PORT , &tracker1PORT[0] );


    string tracker2IP( argv[3] );
    tracker2Sock = tracker2IP;
    colon = tracker2IP.find(':');
    tracker2IP.erase( colon, strlen(argv[3]) - colon );

    string tracker2PORT( argv[3] );
    tracker2PORT.erase( 0, colon+1 );

    strcpy( TRACKER_2_IP , &tracker2IP[0] );
    strcpy( TRACKER_2_PORT , &tracker2PORT[0] );


    listenPort = atoi( MY_LISTEN_PORT );

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

    struct sockaddr_in servAdd;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    int n;
    unsigned int len;

    while(1)
    {
        memset(&servAdd, 0, sizeof(servAdd));

        servAdd.sin_family = AF_INET;
        servAdd.sin_port = htons( atoi(TRACKER_1_PORT) );
        servAdd.sin_addr.s_addr = inet_addr( TRACKER_1_IP );

        getCommand = false;
        shareCommand = false;
        string command;           //
        cout<<"Enter command: ";
        getline (cin, command);
        if( command == "exit" )
          break;

        else if( command == "show downloads" )
        {
          showDownloads();
          continue;
        }

        tracker_alive = false;
        thread alive( checkAlive, sockfd );
        alive.detach();
        sleep(1);
    //cout<<tracker_alive<<endl;
        if( !tracker_alive )   //if tracker 1 is not alive
        {
            memset(&servAdd, 0, sizeof(servAdd));

            servAdd.sin_family = AF_INET;
            servAdd.sin_port = htons( atoi(TRACKER_2_PORT) );
            servAdd.sin_addr.s_addr = inet_addr( TRACKER_2_IP );
        }

        command = processCommand(command);

        sendto(sockfd, (const char *)&command[0], strlen(&command[0]), MSG_CONFIRM, (const struct sockaddr *) &servAdd, sizeof(servAdd));
        //printf("Command sent.\n");

        n = recvfrom(sockfd, (char *)buffer, bufSize, MSG_WAITALL, (struct sockaddr *) &servAdd, &len);
        buffer[n] = '\0';
        //printf("Server : %s\n", buffer);

        if ( getCommand )
        {
          if( strcmp(buffer, "NONE" ) == 0 )  //if there are no seeders available
            continue;
          thread t( pollPieces, buffer, &SHA[0] );  //this will poll pieces for each client and download the file
          t.detach();
        }

        else if( shareCommand && firstShare )
        {
          firstShare = false;
          thread t( seed, listenPort, SHA );
          t.detach();
        }

    }


    for( auto i = Remove.begin(); i != Remove.end(); i++ )
    {
        string rem = "remove";
        unsigned int len;
        if( i->second == false )
        {
            string command = rem + " " + i->first + " " + to_string(listenPort);
            sendto(sockfd, (const char *)&command[0], strlen(&command[0]), MSG_CONFIRM, (const struct sockaddr *) &servAdd, sizeof(servAdd));
            //printf("Command sent.\n");

            recvfrom(sockfd, (char *)buffer, bufSize, MSG_WAITALL, (struct sockaddr *) &servAdd, &len);

            //printf("Server : %s\n", buffer);
        }
    }

    close(sockfd);

    return 0;
}
