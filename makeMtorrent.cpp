#include <stdio.h>
#include <iostream>
#include <openssl/sha.h>
#include <string.h>
#include <cstring>
#include <math.h>
#include <fstream>
#include <set>
#include <sys/stat.h>

//#include "makeMtorrent.h"

#define pieceChars 524288   // 512*1024 (512KB)

using namespace std;

size_t getFilesize(const char* filename) {
    struct stat st;
    if(stat(filename, &st) != 0) {
        return 0;
    }
    return st.st_size;
}

bool fileExists( const char* filepath )
{
    struct stat buf;
    if (stat(filepath, &buf) != -1)
    {
        return true;
    }
    return false;
}

string get_hash( char* path )
{
  string fileHash = "";
  int i;
  int size = getFilesize( path )-1;
  int pieces = ceil( (float)size / (float)pieceChars );
	ifstream ifile( path , ios::binary);
  int readLen;
	if (ifile.good())
	{
    while( size > 0 )
    {
      if( size < pieceChars )
        readLen = size;
      else
        readLen = pieceChars;

      char buffer[readLen];
      ifile.read( buffer, readLen);

      unsigned char hash[SHA_DIGEST_LENGTH];

      SHA1( (unsigned char*)buffer, readLen, hash);

      char hashstr[41];
      for(i = 0; i<20; ++i)
        sprintf(&hashstr[i*2], "%02x", hash[i]);

      hashstr[20]='\0';
      string temp(hashstr);
      fileHash = fileHash + temp;

      size -= pieceChars;
    }
      /*FILE *fptr;

      fptr = fopen("hash.txt", "w");

      fprintf(fptr,"%s", hashstr);
      fclose(fptr);
      ifile.close();*/
	}
  return fileHash;

}

string SHAofSHAstr( string sha )
{
  int i;
  int size = strlen(&sha[0]);

  unsigned char hash[SHA_DIGEST_LENGTH];

  SHA1( (unsigned char*)&sha[0], size, hash);

  char hashstr[41];
  for(i = 0; i<20; ++i)
    sprintf(&hashstr[i*2], "%02x", hash[i]);
  hashstr[20]='\0';
  string SHA(hashstr);

  return SHA;
}

/*int main()
{
  char path[]="hash.txt";
  string s=get_hash(path);
  //cout<<s;
  ofstream Mtor ("test.mtorrent", ios_base::ate);
  Mtor << "<tracker_1_ip>:<port>" <<endl;
  Mtor << "<tracker_2_ip>:<port>" <<endl;
  Mtor << "<filename>" <<endl;
  Mtor << "<filesize in bytes>" <<endl;
  Mtor << s <<endl;
  Mtor.close();
}*/
