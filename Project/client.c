#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;
int main (int argc, char *argv[])
{
    int sd,done=1;
    struct sockaddr_in server;
    char mesaj[256];
    if (argc != 3)
    {
        printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }
    port = atoi (argv[2]);
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("Eroare la socket().\n");
        return errno;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons (port);
    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
        perror ("Eroare la connect().\n");
        return errno;
    }
    if (read (sd, &mesaj,256) < 0)
        {
          perror ("[client]Eroare la read() de la server.\n");
          return errno;
        }
        /* afisam mesajul primit */
    printf ("[client  ]%s\n", mesaj);
    while(done)
    {
        
        bzero (mesaj, 256);
        fflush (stdout);
        read (0, mesaj,256);
        //printf("[%s]\n",msg);
        //if(!strcmp(mesaj,"quit\n")) ok=0;
        printf("[client] Am citit: %s\n",mesaj);
        /* trimiterea mesajului la server */
        if (write (sd,mesaj,256) <= 0)
         {
          perror ("[client]Eroare la write() spre server.\n");
          return errno;
        }
        /*if(strstr(mesaj,"login")!=NULL) 
        {
            printf("Ati apelat functia login. Introduceti username ul si parorla\n");
        }
        */
        /* citirea raspunsului dat de server 
      (apel blocant pina cind serverul raspunde) */
        fflush (stdout);
        if (read (sd, &mesaj,256) < 0)
        {
          perror ("[client]Eroare la read() de la server.\n");
          return errno;
        }
        /* afisam mesajul primit */
        printf ("%s\n", mesaj);
  }
   close (sd);
} 