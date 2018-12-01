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
#include <iostream>
#include <thread>

#include "makeMtorrent.h"
#include "seed.h"

using namespace std;

extern int listenPort;

#define max_pieces 1200

extern map < string, string > hashPath;  //maps shared hash string to local file path

extern map < string, string > hashPieces;  //pieces i have (bitmap)

extern map < string, bool > Remove;

extern map < string, string > downloads;

vector<string> decidePieces(vector<string>, int, int);

extern bool firstShare;

void download( char*, string, string );

string SHAofSHA;

void pollPieces( char* clientList, char* SHA, string mySock )
{
  string temp(SHA);
  SHAofSHA = temp;
  //cout<<SHAofSHA<<" yaela";
  char* token;
  token = strtok (clientList," ");
  vector<char*> clients;
  
  if ( token != NULL && strcmp (token, &mySock[0]) != 0)
    clients.push_back( token );

  while (token != NULL)
  {
    cout << clients.size() << endl;
    //cout<<tokens[i++];
    token = strtok(NULL," ");

    if ( token != NULL && strcmp (token, &mySock[0]) != 0)
      clients.push_back( token );

  }
  int no_of_clients = clients.size();

  if (no_of_clients == 0)
  {
    cout << "Cannot download from own socket" << endl;
    return;
  }

  struct sockaddr_in address;
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  //char *hello = "Hello from client";
  char buffer[2048] = {0};

  int i;
  vector<string> availablePieces;

  for(i = 0; i < no_of_clients; i++ )  //actually here the clients are seeders from which we want to download
  {
    if( clients[i] != NULL )
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
      //cout<<"bitMapFor? "<<SHA<<endl;
      char bitMapReq[1024];
      strcpy( bitMapReq, "bitMapFor? " );
      strcat( bitMapReq, SHA );
      send(sock , bitMapReq , strlen(bitMapReq) , 0 );
      //printf("SHA sent\n");
      valread = read( sock , buffer, 2048);
      //printf("Bitmap :%s\n",buffer );

      if( buffer != NULL )
      {
        string bitmap(buffer);
        availablePieces.push_back(bitmap);
      }

    }
  }

  int totalPieces = strlen(&availablePieces[0][0]);
//cout<<"Input to piece slector: "<<availablePieces[0]<<endl;
  vector<string> selectedPieces ( decidePieces( availablePieces, no_of_clients, totalPieces ) );

  string bitmap="";
  for(int i=0; i<totalPieces; i++)
    bitmap = bitmap + "0";

  hashPieces[SHAofSHA] = bitmap;

//cout<<"download "<<selectedPieces.size()<<" times"<<endl;

  thread download_thread[ selectedPieces.size() ];

  for(int j=0; j<selectedPieces.size(); j++)
  {
    download_thread[j] = thread( download, clients[j], selectedPieces[j], hashPath[SHAofSHA] );
  }

  for(int j=0; j<selectedPieces.size(); j++)
  {
    download_thread[j].join();
  }

  downloads[SHAofSHA][1] = 'S';

  return;
}

vector<string> decidePieces(vector<string> av, int c, int len)
{
  if( av.size() == 1 )
    return av;

  int i,j;

  pair<int,int> p;  // <number of pieces, client number>
  vector< pair<int,int> > no_of_pieces;  //stores number of pieces a clientNumber has

  string init = "";
  for(i=0; i<len; i++)
    init = init + "0";

  vector<string> selected(c,init);  //which pieces are decided currently for which client
  bool Sel[len] = {false};
  bool notDone = true;

  int pieces;
  for(i=0; i<c; i++)
  {
    pieces = 0;
    for(j=0; j<len; j++)  //count the number of pieces at client i
    {
      selected[i][j] = '0';
      if( av[i][j] == '1' )
        pieces++;
    }
    p = make_pair(pieces,i);
    no_of_pieces.push_back(p);
  }

  int min;
  sort( no_of_pieces.begin(), no_of_pieces.end() );//the client number will remain intact because of pair

  int count[c] = {0};
  int n;
  n = (float)len / (float)c  ;  //atmost pieces to take from each client(initially)

  while( notDone )
  {
      for(i=0; i<c; i++)
      {
          int min = no_of_pieces[i].second;
          for(j=0; j<len; j++)
          {
              if( av[min][j] == '1' && Sel[j] == false )
              {
                  Sel[j] = true;
                  selected[min][j] = '1';
                  count[min]++;
                  if( count[min] >= n )  //select atmost n to divide equally
                  {
                      break;
                  }
              }
          }
      }

      notDone = false;
      for(i=0; i<len; i++)   //check if all pieces are selected
      {
          if( Sel[i] == false )
          {
            notDone = true;
            break;
          }
      }

      n++;  // if there are some pieces left, increase pieces per client by 1

  }

  return selected;

}

void download( char* cliAdd, string bitmap, string dest )
{
  //cout<<"bitmap: "<<bitmap<<endl;
  string ip( cliAdd );
  int colon = ip.find(':');
  ip.erase( colon, strlen(cliAdd) - colon );

  string port( cliAdd );
  port.erase( 0, colon+1 );

  char* IP = &ip[0];
  char* PORT = &port[0];

  struct sockaddr_in address;
  int sock = 0, valread;
  struct sockaddr_in serv_addr;

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

  int j;

  //cout<<"about to req piece "<<endl;
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
      printf("\nConnection Failed \n");
      return;
  }

  string req = "givePieces " + SHAofSHA + " " + &bitmap[0];
  //cout<<req<<endl;

  char buffer[1024] = {0};

  send(sock , &req[0] , strlen(&req[0]) , 0 );
  //printf("Pieces req Sent\n");
  valread = read( sock , buffer, 1024);
  //printf("%s\n",buffer );
  memset(&buffer,'\0',1024);

  int size;
  size = getFilesize( &hashPath[ SHAofSHA ][0] );


  int pos;
  //cout<<"bitmap len: "<<strlen( &bitmap[0] );
  //ofstream df("down.txt", ios::binary );
  //df.close();
  FILE *downFile ;
  downFile = fopen(&dest[0] , "wb");
char sendAny[] = "keepSending";

  for(int i=0; i<strlen( &bitmap[0] ); i++)
  {
    if( bitmap[i] == '1' )
    {
        if( i == strlen( &bitmap[0] )-1 )
        {
            pos = i*524288;
            //cout<<"pos: "<<pos;
            //downFile.seekp( pos );
            fseek( downFile, pos, SEEK_SET );
            j=0;
            while( j<= (size/1024)%512 )//&& size > 0 )
            {

                //send(sock , sendAny , strlen(sendAny) , 0 );
                //printf("piece received\n");
                valread = recv( sock , buffer, 1024, 0);
                //buffer[1024] = '\0';
                //cout<<buffer<<" "<<i<<endl;
                fwrite( buffer, valread, 1, downFile );
                //downFile.write( buffer, strlen(buffer) );
                j++;
                //size--;
            }
            hashPieces[SHAofSHA][i] = '1';
            //cout<<"i: "<<i<<endl;
            break;
        }

        pos = i*524288;
        //cout<<"pos: "<<pos;
        //downFile.seekp( pos );
        fseek( downFile, pos, SEEK_SET );
        j=0;
        while( j< 512 )//&& size > 0 )
        {
            //send(sock , sendAny , strlen(sendAny) , 0 );
            //printf("piece received\n");
            recv( sock , buffer, 1024, 0);
            //buffer[1024] = '\0';
            //cout<<buffer<<" "<<i<<endl;
            fwrite( buffer, 1024, 1, downFile );
            //downFile.write( buffer, strlen(buffer) );
            j++;
            //size--;
        }
        hashPieces[SHAofSHA][i] = '1';

        if( i == 0 )
        {
            Remove[SHAofSHA] = false ;
            if( firstShare )
            {
                firstShare = false;
                thread t( seed, listenPort, SHAofSHA );
                t.detach();
            }
        }

        //cout<<"i: "<<i<<endl;

    }

  }
  //cout<<"SIZE: "<<hashPath[ SHAofSHA ]<<" "<<size<<endl;
  fclose(downFile);
  return;
}
/*int main()
{
  vector<string> v;
  v.push_back("1111111111");
  v.push_back("1111111111");
  v.push_back("1111111111");
//cout<<v[0];
  vector<string> o( decidePieces(v, 3,10) );
  cout<<o[0]<<endl;
  cout<<o[1]<<endl;
  cout<<o[2]<<endl;

  return 0;
}*/
