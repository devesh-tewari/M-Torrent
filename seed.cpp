#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>

#include "makeMtorrent.h"

using namespace std;

extern map < string, string > hashPath;
extern map < string, string > hashPieces;

void upload( char*, char*, int);

void handleReq( int new_socket, char* buffer )
{
  int valread = read( new_socket , buffer, 1024);
  printf("Requester: %s\n",buffer );

  char reply[1024];
  if( buffer!=NULL )
  {
    char* token;
    token = strtok(buffer," ");
    vector<char*> tokens;
    tokens.push_back(token);
    while (token != NULL)
    {
        //cout<<tokens[i++]<<endl;
        token = strtok(NULL," ");
        tokens.push_back( token );
    }

    if( strcmp(tokens[0], "givePieces") == 0 )
    {
        strcpy(reply,"give hmm");
        cout<<"received piece req"<<endl;
        send(new_socket , reply , strlen(reply) , 0 );
        cout<<"giving piece"<<endl;

        cout<<"Tokens[0] = "<<tokens[0]<<endl;
        cout<<"Tokens[1] = "<<tokens[1]<<endl;
        if( tokens[1]!=NULL && tokens[2]!=NULL )
        {
            string s(tokens[1]);   //SHA
            string p(tokens[2]);   //piece number to be read
            cout<<"s: "<<s<<endl;
            string path = hashPath[s];
            cout<<"Path : "<<path<<endl;
            char* cpath = &path[0];
            cout<<"read from: "<<cpath<<endl;

            upload( cpath, tokens[2], new_socket );
        }
    }

    //{}  //sendPiece
    else
    {
      string SHA(buffer);
      strcpy( reply, &hashPieces[SHA][0] );
      send(new_socket , reply , strlen(reply) , 0 );
    }
    //cout<<hashPath[SHA];
  }


  printf("Pieces/Bitmap sent\n");

  return;
}

void seed(int listenPort)
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    //char *hello = "I have this this pieces";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( listenPort );

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int j;
    while(1)
    {
        //if(remove) return;
        cout<<"accepting.."<<endl;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("Connection accept error");
        }

        thread t( handleReq, new_socket, buffer );

        t.detach();

    }
}

void upload( char* cpath, char* pieces, int new_socket )
{
  int size = getFilesize(cpath);
  FILE* upFile;
  upFile =  fopen( cpath, "rb" );
  //upFile.seekg( i*524288 , ios::beg);
  int pos;
  cout<<"inside upload func"<<endl;
  char buffer[1024];
  int valread;
  cout<<"Requested pieces: "<<pieces;
  cout<<"no of pieces: "<<strlen(pieces)<<endl;
  for( int i=0; i<strlen(pieces); i++ )
  {
      if( pieces[i] == '1' )
      {
          if( i == strlen(pieces)-1 )
          {
                pos = i*524288;
                fseek( upFile, pos, SEEK_SET ); //point to beggining of ith piece

                int j=0;
                while( j<= ((size/1024)%512) )
                {
                    //valread = read( new_socket , buffer, 20);
                    if( j == ((size/1024)%512) )
                    {
                        //memset(&buffer,'\0',1024);
                        //upFile.read( buffer, 1024 );
                        fread(buffer, 1024, 1, upFile);
                        buffer[size%512] = '\0';
                        //cout<<buffer<<" "<<i<<endl;
                        send(new_socket , buffer , size%512 , 0 );
                        break;
                    }
                    //memset(&buffer,'\0',1024);
                    //upFile.read( buffer, 1024 );
                    fread(buffer, 1024, 1, upFile);
                    buffer[1024] = '\0';
                    //cout<<buffer<<" "<<i<<endl;
                    send(new_socket , buffer , 1024 , 0 );

                    j++;
                }
                cout<<"mod: "<<size%512;
                break;
          }
          pos = i*524288;
          fseek( upFile, pos, SEEK_SET );  //point to beggining of ith piece

          int j=0;
          while( j<512 )
          {
              //valread = read( new_socket , buffer, 20);
              memset(&buffer,'\0',1024);
              fread(buffer, 1024, 1, upFile);
              buffer[1024] = '\0';
              //cout<<buffer<<" "<<i<<endl;
              send(new_socket , buffer , 1024 , 0 );

              j++;
          }
          //cout<<"i: "<<i<<endl;
      }
      //cout<<"j :"<<j<<endl;

  }
  fclose(upFile);
  return;
}
