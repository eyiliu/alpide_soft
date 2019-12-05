#include "rbcp.h"

struct rbcp_header{
  unsigned char type;
  unsigned char command;
  unsigned char id;
  unsigned char length;
  unsigned int address;
};


rbcp::rbcp(){
  rbcp(0,(char **)NULL);
}

rbcp::rbcp(int c, char **v){
  argc = c;
  argv = v;
  sitcpPort = 4660;
  sitcpIpAddr= "131.169.133.173";
  if(argc == 2)
    sitcpIpAddr = argv[1];

}

rbcp::rbcp(std::string ip, int udp):argc(0),argv(0)
{
  sitcpPort = udp;
  ipaddr = ip;
  sitcpIpAddr= ipaddr.c_str();
}

rbcp::~rbcp(){
}

int rbcp::run(){

  if((argc != 2) && argc != 1 && argc != 0){
    puts("\nThis application controls bus of a SiTCP chip for debugging.");
    printf("Usage: %s <IP address>\n\n", argv[0]);
    return -1;
  }


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

	if(DispatchCommand(szVerb, szArg1, szArg2, sitcpIpAddr, sitcpPort, 
			   1)<0) 
	  exit(EXIT_FAILURE);
      }

      fclose(fin);
    }else{
      
      if(DispatchCommand(szVerb, szArg1, szArg2, sitcpIpAddr, sitcpPort, 
			 1)<0) 
	break;
    }
  }
  return 0;

}


int rbcp::DispatchCommand(char* pszVerb,
		    char* pszArg1,
		    char* pszArg2,
		    const char* ipAddr,
		    unsigned int rbcpPort,
		    char dispMode
		    ){
  //  char sendData[UDP_BUF_SIZE];
  char recvData[UDP_BUF_SIZE];
  unsigned int tempInt;
  struct rbcp_header sndHeader;
  static unsigned char id = 0;
  id ++;
  sndHeader.type=RBCP_VER;
  sndHeader.id = id;

  if(strcmp(pszVerb, "wrb") == 0){
    tempInt = myAtoi(pszArg2);    
    pszArg2[0]= (char)(0xFF & tempInt);

    sndHeader.command= RBCP_CMD_WR;
    sndHeader.length=1;
    sndHeader.address=htonl(myAtoi(pszArg1));
    
    return rbcp_com(ipAddr, rbcpPort, &sndHeader, pszArg2,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "wrs") == 0){
    tempInt = myAtoi(pszArg2);    
    pszArg2[1]= (char)(0xFF & tempInt);
    pszArg2[0]= (char)((0xFF00 & tempInt)>>8);
 
    sndHeader.command= RBCP_CMD_WR;
    sndHeader.length=2;
    sndHeader.address=htonl(myAtoi(pszArg1));

    return rbcp_com(ipAddr, rbcpPort, &sndHeader, pszArg2,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "wrw") == 0){
    tempInt = myAtoi(pszArg2);

    pszArg2[3]= (char)(0xFF & tempInt);
    pszArg2[2]= (char)((0xFF00 & tempInt)>>8);
    pszArg2[1]= (char)((0xFF0000 & tempInt)>>16);
    pszArg2[0]= (char)((0xFF000000 & tempInt)>>24);

    sndHeader.command= RBCP_CMD_WR;
    sndHeader.length=4;
    sndHeader.address=htonl(myAtoi(pszArg1));

    return rbcp_com(ipAddr, rbcpPort, &sndHeader, pszArg2,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "rd") == 0){
    sndHeader.command= RBCP_CMD_RD;
    sndHeader.length=myAtoi(pszArg2);
    sndHeader.address=htonl(myAtoi(pszArg1));
    
    return rbcp_com(ipAddr, rbcpPort, &sndHeader, pszArg2,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "help") == 0){
    return OnHelp();
  }
  else if(strcmp(pszVerb, "quit") == 0){
    return -1;
  }
  puts("No such command!\n");
  return 0;
  
}

int rbcp::DispatchCommand(const char* pszVerb,
			  unsigned int addrReg,
			  unsigned int dataReg,
			  std::string *recvStr,
			  char dispMode
			  ){
  char sendData[UDP_BUF_SIZE];
  char recvData[UDP_BUF_SIZE];
  unsigned int tempInt;
  struct rbcp_header sndHeader;
  static unsigned char id = 0;
  id ++;
  sndHeader.type=RBCP_VER;
  sndHeader.id = id;

  if(strcmp(pszVerb, "wrb") == 0){
    tempInt = dataReg;    
    sendData[0]= (char)(0xFF & tempInt);

    sndHeader.command= RBCP_CMD_WR;
    sndHeader.length=1;
    sndHeader.address=htonl(addrReg);
    
    return rbcp_com(sitcpIpAddr, sitcpPort, &sndHeader, sendData,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "wrs") == 0){
    tempInt = dataReg;    
    sendData[1]= (char)(0xFF & tempInt);
    sendData[0]= (char)((0xFF00 & tempInt)>>8);
 
    sndHeader.command= RBCP_CMD_WR;
    sndHeader.length=2;
    sndHeader.address=htonl(addrReg);

    return rbcp_com(sitcpIpAddr, sitcpPort, &sndHeader, sendData,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "wrw") == 0){
    tempInt = dataReg;

    sendData[3]= (char)(0xFF & tempInt);
    sendData[2]= (char)((0xFF00 & tempInt)>>8);
    sendData[1]= (char)((0xFF0000 & tempInt)>>16);
    sendData[0]= (char)((0xFF000000 & tempInt)>>24);

    sndHeader.command= RBCP_CMD_WR;
    sndHeader.length=4;
    sndHeader.address=htonl(addrReg);

    return rbcp_com(sitcpIpAddr, sitcpPort, &sndHeader, sendData,recvData,dispMode);
  }
  else if(strcmp(pszVerb, "rd") == 0){
    sndHeader.command= RBCP_CMD_RD;
    sndHeader.length=(char)dataReg;
    sndHeader.address=htonl(addrReg);
    
    int re = rbcp_com(sitcpIpAddr, sitcpPort, &sndHeader, sendData,recvData,dispMode);
    if(recvStr!=NULL){
      recvStr->clear();
      recvStr->append(recvData,dataReg);
    }
    return re;
  }
  else if(strcmp(pszVerb, "help") == 0){
    return OnHelp();
  }
  else if(strcmp(pszVerb, "quit") == 0){
    return -1;
  }
  puts("No such command!\n");
  return 0;
  
}




int rbcp::OnHelp()
{
	puts("\nCommand list:");
	puts("   wrb [address] [byte_data] : Write byte");
	puts("   wrs [address] [short_data]: Write short(16bit)");
	puts("   wrw [address] [word_data] : Write word(32bit)");
	puts("   rd [address] [length]     : Read data");
	puts("   load [file name]          : Execute a script");
	puts("   quit                      : quit from this program\n");

	return 0;
}


unsigned int rbcp::myAtoi(char* str){
  char temp[256];
  int i=0, j=0, value=0, endProc=0;
  strcpy(temp,str);

  if((temp[i]=='0')||isblank(temp[i])) i++;
  if((temp[i]=='x')||(temp[i]=='X')){
    i++;
    j++;
    while((endProc==0)&&(temp[i]!='\0')){
      if(isdigit(temp[i])){
	value=value*16;
	value+=temp[i]-'0';
	i++;
      }else if(isxdigit(temp[i])){
	value=value*16;

	if(isupper(temp[i])){
	  value+=temp[i]-'A'+10;
	}else{
	  value+=temp[i]-'a'+10;
	}
	i++;
      }else{
	endProc=1;
	//	puts("endProc");
      }
    }
    if(j>8){
      puts("Error: too large value is detected.");
      return 0xFFFFFFFF;
    }
  }else{
    while(isdigit(temp[i])){
      value=value*10;
      value+=temp[i]-'0';
      i++;
      j++;
      if(j>10){
	puts("Error: too large value is detected.");
	return 0xFFFFFFFF;
      }
    }
  }

  return value;
}

int rbcp::myScanf(char* inBuf, char* argBuf1, char* argBuf2,  char* argBuf3)
{
  int i=0;

  argBuf1[0]='\0';
  argBuf2[0]='\0';
  argBuf3[0]='\0';

  if((i=myGetArg(inBuf, i, argBuf1))>0){
    if((i=myGetArg(inBuf, i, argBuf2))>0){
      return myGetArg(inBuf, i, argBuf3);
    }else{
      return i;
    }
  }else{
    return i;
  }
  return i;
}

int rbcp::myGetArg(char* inBuf, int i, char* argBuf){
  int j;

  while(i<MAX_LINE_LENGTH){
    if(isblank(inBuf[i])){
      i++;
    }else if(iscntrl(inBuf[i])){
      return 0;
    }else {
      break;
    }
  }

  for(j=0;j<MAX_PARAM_LENGTH;j++){
    if(i<MAX_LINE_LENGTH){
      if(isblank(inBuf[i])){
	argBuf[j]='\0';

      }else if(iscntrl(inBuf[i])){
	argBuf[j]='\0';
	return 0;
      }else{
	argBuf[j]=inBuf[i];
	i++;
      }
    }else{
      return -1;
    }
  }
  return i;
}

int rbcp::rbcp_com(const char* ipAddr, unsigned int port, struct rbcp_header* sendHeader, char* sendData, char* recvData, char dispMode){

  struct sockaddr_in sitcpAddr;
  int sock;

  struct timeval timeout;
  fd_set setSelect;
  
  int sndDataLen;
  int cmdPckLen;

  char sndBuf[1024];
  int i, j = 0;
  int rcvdBytes;
  char rcvdBuf[1024];
  int numReTrans =0;

  /* Create a Socket */
  if(dispMode==RBCP_DISP_MODE_DEBUG) puts("\nCreate socket...\n");

  sock = socket(AF_INET, SOCK_DGRAM, 0);

  sitcpAddr.sin_family      = AF_INET;
  sitcpAddr.sin_port        = htons(port);
  sitcpAddr.sin_addr.s_addr = inet_addr(ipAddr);

  sndDataLen = (int)sendHeader->length;

  if(dispMode==RBCP_DISP_MODE_DEBUG) printf(" Length = %i\n",sndDataLen);

  /* Copy header data */
  memcpy(sndBuf,sendHeader, sizeof(struct rbcp_header));

  if(sendHeader->command==RBCP_CMD_WR){
    memcpy(sndBuf+sizeof(struct rbcp_header),sendData,sndDataLen);
    cmdPckLen=sndDataLen + sizeof(struct rbcp_header);
  }else{
    cmdPckLen=sizeof(struct rbcp_header);
  }


  if(dispMode==RBCP_DISP_MODE_DEBUG){
    for(i=0; i< cmdPckLen;i++){
      if(j==0) {
	printf("\t[%.3x]:%.2x ",i,(unsigned char)sndBuf[i]);
	j++;
      }else if(j==3){
	printf("%.2x\n",(unsigned char)sndBuf[i]);
	j=0;
      }else{
	printf("%.2x ",(unsigned char)sndBuf[i]);
	j++;
      }
    }
    if(j!=3) printf("\n");
  }

  /* send a packet*/

  sendto(sock, sndBuf, cmdPckLen, 0, (struct sockaddr *)&sitcpAddr, sizeof(sitcpAddr));
  if(dispMode==RBCP_DISP_MODE_DEBUG) puts("The packet have been sent!\n");

  /* Receive packets*/
  
  if(dispMode==RBCP_DISP_MODE_DEBUG) puts("\nWait to receive the ACK packet...");


  while(numReTrans<3){

    FD_ZERO(&setSelect);
    FD_SET(sock, &setSelect);

    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;
 
    if(select(sock+1, &setSelect, NULL, NULL,&timeout)==0){
      /* time out */
      puts("\n***** Timeout ! *****");
      sendHeader->id++;
      memcpy(sndBuf,sendHeader, sizeof(struct rbcp_header));
      sendto(sock, sndBuf, cmdPckLen, 0, (struct sockaddr *)&sitcpAddr, sizeof(sitcpAddr));
      numReTrans++;
      FD_ZERO(&setSelect);
      FD_SET(sock, &setSelect);
    } else {
      /* receive packet */
      if(FD_ISSET(sock,&setSelect)){
	rcvdBytes=recvfrom(sock, rcvdBuf, 2048, 0, NULL, NULL);

	if(rcvdBytes<(int)(sizeof(struct rbcp_header))){
	  puts("ERROR: ACK packet is too short");
	  close(sock);
	  return -1;
	}

	if((0x0f & rcvdBuf[1])!=0x8){
	  puts("ERROR: Detected bus error");
	  close(sock);
	  return -1;
	}

	rcvdBuf[rcvdBytes]=0;

	if(RBCP_CMD_RD){
	  memcpy(recvData,rcvdBuf+sizeof(struct rbcp_header),rcvdBytes-sizeof(struct rbcp_header));
	}

	if(dispMode==RBCP_DISP_MODE_DEBUG){
	  puts("\n***** A pacekt is received ! *****.");
	  puts("Received data:");

	  j=0;

	  for(i=0; i<rcvdBytes; i++){
	    if(j==0) {
	      printf("\t[%.3x]:%.2x ",i,(unsigned char)rcvdBuf[i]);
	      j++;
	    }else if(j==3){
	      printf("%.2x\n",(unsigned char)rcvdBuf[i]);
	      j=0;
	    }else{
	      printf("%.2x ",(unsigned char)rcvdBuf[i]);
	      j++;
	    }
	    if(i==7) printf("\n Data:\n");
	  }

	  if(j!=3) puts(" ");
	}else if(dispMode==RBCP_DISP_MODE_INTERACTIVE){
	  if(sendHeader->command==RBCP_CMD_RD){
	    j=0;
	    puts(" ");

	    for(i=8; i<rcvdBytes; i++){
	      if(j==0) {
		printf(" [0x%.8x] %.2x ",ntohl(sendHeader->address)+i-8,
		       (unsigned char)rcvdBuf[i]);
		j++;
	      }else if(j==7){
		printf("%.2x\n",(unsigned char)rcvdBuf[i]);
		j=0;
	      }else if(j==4){
		printf("- %.2x ",(unsigned char)rcvdBuf[i]);
		j++;
	      }else{
		printf("%.2x ",(unsigned char)rcvdBuf[i]);
		j++;
	      }
	    }
	    
	    if(j!=15) puts(" ");
	  }else{
	    printf(" 0x%x: OK\n",ntohl(sendHeader->address));
	  }
	}
	numReTrans = 4;
	close(sock);
	return(rcvdBytes);
      }
    }
  }
  close(sock);

  return -3;
}
