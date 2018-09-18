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

using namespace std;

#define max_pieces 1200

vector<string> decidePieces(vector<string>, int, int);

void download( char*, string );

string SHAofSHA;

void pollPieces( char* clientList, char* SHA )
{
  string temp(SHA);
  SHAofSHA = temp;
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

  int i;
  vector<string> availablePieces;

  for(i = 0; i < no_of_clients; i++ )  //actually here the clients are servevrs from which we want to download
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
      cout<<"Bit map for? "<<SHA<<endl;
      send(sock , SHA , strlen(SHA) , 0 );
      //printf("SHA sent\n");
      valread = read( sock , buffer, 1024);
      printf("Bitmap :%s\n",buffer );

      if( buffer != NULL )
      {
        string bitmap(buffer);
        availablePieces.push_back(bitmap);
      }

    }
  }

  int totalPieces = strlen(&availablePieces[0][0]);
cout<<"Input to piece slector: "<<availablePieces[0]<<endl;
  vector<string> selectedPieces ( decidePieces( availablePieces, no_of_clients, totalPieces ) );

cout<<"download"<<selectedPieces.size()<<" times"<<endl;
  for(int j=0; j<selectedPieces.size(); j++)
  {
    download( clients[j], selectedPieces[j] );
  }

}

vector<string> decidePieces(vector<string> av, int c, int len)
{
  if( av.size() == 1 )
    return av;

  int i,j,k;

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

  int n = len / c;  //atmost pieces to take from each client(initially)

  for(i=0; i<c; i++)
  {
      int min = no_of_pieces[i].second;
      int count = 0;
      for(j=0; j<len; j++)
      {
        if( av[min][j] == '1' && Sel[j] == false )
        {
            Sel[j] = true;
            selected[min][j] = '1';
            count++;
            if( count >= n )  //select atmost n to divide equally
                break;
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

  if(notDone)
  {
      int max;
      for(i=c-1; i>=0; i--)
      {
        max = no_of_pieces[i].second;  //start to select the remaining pieces from maximum pieces
        for(j=0; j<len; j++)
        {
          if( Sel[j] == false && av[max][j] == '1' )
          {
            Sel[j] = true;
            selected[max][j] = '1';
          }
        }
      }
  }

  return selected;

}

void download( char* cliAdd, string bitmap )
{
  cout<<"bitmap: "<<bitmap<<endl;
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

  char buffer[1024] = {0};

  int j=1;
  for(int i=0; i<strlen( &bitmap[0] ); i++)
  {
    if( bitmap[i] == '1' )
    {
        cout<<"about to req piece "<<i<<" : ";
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("\nConnection Failed \n");
            return;
        }

        string req = "givePiece " + SHAofSHA + " " + to_string(i);
        cout<<req<<endl;

        send(sock , &req[0] , strlen(&req[0]) , 0 );
        printf("Piece req Sent sent\n");
        valread = read( sock , buffer, 1024);
        printf("%s\n",buffer );

        char sendAny[] = "keepSending";
        while( j-- )
        {
            send(sock , sendAny , strlen(sendAny) , 0 );
            printf("piece received\n");
            valread = read( sock , buffer, 1024);
            printf("seeder :%s\n",buffer );
        }
    }
  }
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
