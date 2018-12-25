#include "rbcp_com.h"

int main(int argc, char* argv[]){

  char* sitcpIpAddr;
  unsigned int sitcpPort;

  struct rbcp_header sndHeader;

  char tempKeyBuf[MAX_LINE_LENGTH];
  char szVerb[MAX_PARAM_LENGTH];
  char szArg1[MAX_PARAM_LENGTH];
  char szArg2[MAX_PARAM_LENGTH];
  int rtnValue;

  FILE *fin;

  if(argc != 3){
    puts("\nThis application controls bus of a SiTCP chip for debugging.");
    printf("Usage: %s <IP address> <Port #>\n\n", argv[0]);
    return -1;
  }else{
    sitcpIpAddr = argv[1];
    sitcpPort   = atoi(argv[2]);
  }

  sndHeader.type=RBCP_VER;
  sndHeader.id=0;

  while(1){
    printf("SiTCP-RBCP$ ");
    fgets(tempKeyBuf, MAX_LINE_LENGTH, stdin);
    if((rtnValue=myScanf(tempKeyBuf,szVerb, szArg1, szArg2))<0){
      printf("Erro myScanf(): %i\n",rtnValue);
      return -1;
    }

    if(strcmp(szVerb, "load") == 0){
      if((fin = fopen(szArg1,"r"))==NULL){
	printf("ERROR: Cannot open %s\n",szArg1);
	break;
      }
      while(fgets(tempKeyBuf, MAX_LINE_LENGTH, fin)!=NULL){
	if((rtnValue=myScanf(tempKeyBuf,szVerb, szArg1, szArg2))<0){
	  printf("ERROR: myScanf(): %i\n",rtnValue);
	  return -1;
	}

	sndHeader.id++;

	if(DispatchCommand(szVerb, szArg1, szArg2, sitcpIpAddr, sitcpPort, &sndHeader,1)<0) exit(EXIT_FAILURE);
      }

      fclose(fin);
    }else{

      sndHeader.id++;
      
      if(DispatchCommand(szVerb, szArg1, szArg2, sitcpIpAddr, sitcpPort, &sndHeader,1)<0) break;
    }
  }
  return 0;
}
