/* 
  Simple client for the inetd servers/services
  
  History 
   v2    980423 YB - rewrite.
   v0.1  980108 ST - modelled after 
                http://www.ecst.csuchico.edu/~beej/guide/net/
 */


#ifdef VMS
extern int errno;
#define MAXHOSTNAMELEN 128
#include <descrip.h>        /* VMS descriptor stuff */
#include <in.h>             /* internet system Constants and structures. */
#include <inet.h>           /* Network address info. */
#include <iodef.h>          /* I/O FUNCTION CODE DEFS */
#include <lib$routines.h>   /* LIB$ RTL-routine signatures. */
#include <netdb.h>          /* Network database library info. */
#include <signal.h>         /* UNIX style Signal Value Definitions */
#include <socket.h>         /* TCP/IP socket definitions. */
#include <ssdef.h>          /* SS$_<xyz> sys ser return stati <8-) */
#include <starlet.h>        /* Sys ser calls */
#include <stdio.h>          /* UNIX 'Standard I/O' Definitions   */
#include <stdlib.h>         /* General Utilities */
#include <string.h>         /* String handling function definitions */
#include <ucx$inetdef.h>    /* UCX network definitions */
#include <unixio.h>         /* Prototypes for UNIX emulation functions */
#else
#include <errno.h> 
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/param.h>
#endif

#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>

/* 
   The port client will be connecting to
 simap          993/tcp                         # IMAP over SSL
 pdaemon       	994
 spop3          995/tcp                         # POP-3 over SSL
 hostdb_fc     	996
 adb        	997
 hostdb       	998
 maildb	        999 
#define SERVER "130.237.208.17" 
#define SERVER "mail1.physto.se" 
 */

/*
#define PORT    998                  
#define SERVER "syshostdb.physto.se" 
*/
#define PORT    999
#define SERVER "maildb.physto.se" 


#define MAXDATASIZE 51200        /* max number of bytes we can get at once */
#define MAXNAME 32

char buf[MAXDATASIZE];
char dbg[MAXDATASIZE];
int numbytes,sockfd;
int uid, gid;
char iamname[MAXNAME], myhostname[MAXHOSTNAMELEN];
void getAnswer();
void connectServer();


void svdebug() { // printf ("Client: %s\n",dbg); 
}


/*********************************************/

void encode(char *str){
  int i=0;
  while(str[i]!='\000'){
    if(str[i]==' ') str[i]='#';
    i++;
  }
}

/*****************************************
 *
 *****************************************/
int main(int argc, char *argv[]){
  int i,  numbytes, advance, nopt, nuser;  
  char *buf2;
  struct passwd *passwd_entry;

  gid = getgid();
  uid = getuid();
  gethostname(myhostname,MAXHOSTNAMELEN);

#ifdef VMS
  strcpy(iamname,getenv("USER"));
#else
  passwd_entry=getpwuid(uid);
  strcpy(iamname,passwd_entry->pw_name);
//strcpy(iamname,getlogin());
#endif

  connectServer();

  buf2 += sprintf(buf2=buf," ");

  i = 1;
  while(i<argc) {
    encode(argv[i]);
    buf2 += sprintf (buf2,"%s ",argv[i]);
    i++;
  }

  sprintf(buf2,
	  "-HLO iam=%s host=%s cmd=%s \n",
	  iamname,myhostname,argv[0]);

  sprintf(dbg,"%s",buf); svdebug();

  if((numbytes=send(sockfd, buf, strlen(buf), 0)) == -1){
    perror("send"); exit(1); }

  getAnswer();

  close(sockfd);

  return 0;
}

void connectServer() {
  struct hostent *he;
  struct sockaddr_in their_addr; /* connector's address information */
  
  if ((he=gethostbyname(SERVER)) == NULL) { 
    perror("gethostbyname");    exit(1); }
  
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");    exit(1); }
  
  their_addr.sin_family = AF_INET;      /* host byte order */
  their_addr.sin_port   = htons(PORT);  /* short, network byte order */
  their_addr.sin_addr   = *((struct in_addr *)he->h_addr);
  
#ifdef VMS
  memset((their_addr.sin_zero), 0,8);
#else
  bzero((their_addr.sin_zero), 8);      /* zero the rest of the struct */
#endif
  
  if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
    perror(buf); exit(1); }

  sprintf(dbg,"connect %s",SERVER);  svdebug(); 
}

void getAnswer(){
  int recvbytes, totbytes, endOfAnswer, nbuff;

  sprintf(dbg,"getAnswer entered...");  svdebug();
  totbytes = 0;
  
  // fcntl(sockfd, F_SETFL, 0);
  endOfAnswer = 0;
  nbuff = 0;
  while ((recvbytes=recv(sockfd, buf, MAXDATASIZE, 0)) > 0) {
    // fcntl(sockfd, F_SETFL, O_NONBLOCK);
    totbytes += recvbytes;
    if (recvbytes>2){
      if ((  buf[recvbytes-3] == 'E') 
	  &&(buf[recvbytes-2] == 'O') 
	  &&(buf[recvbytes-1] == 'L')) {
	endOfAnswer++;
       	recvbytes -= 3;
      }
    }
    buf[recvbytes] = '\0';
    printf ("%s",buf);
    if (endOfAnswer>0) break;
    nbuff++;
    //    sleep(1);
  }
  
  sprintf(dbg,"getAnswer got %d buffers  %d bytes",nbuff,totbytes); svdebug();
  if (totbytes == 0){  // ******** && errno != EWOULDBLOCK) {
    perror("recv");
    exit(1);
  }
}
