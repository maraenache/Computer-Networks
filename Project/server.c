#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>
#define PORT 2029


int id_mesaj=3;
char inbox[1000];
int iesire_while[30];
int iesire_chat[30];
char users[30][30];
char parole[30][30];
/* codul de eroare returnat de anumite apeluri */
extern int errno;
char *error_message;
int ret;
static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */

typedef struct {
	pthread_t idThread; //id-ul thread-ului
	int thCount; //nr de conexiuni servite
}Thread;

Thread *threadsPool; //un array de structuri Thread

int sd; //descriptorul de socket de ascultare
int nthreads;//numarul de threaduri
pthread_mutex_t mlock=PTHREAD_MUTEX_INITIALIZER;              // variabila mutex ce va fi partajata de threaduri
char username[30];
char password[30];
char str[256], sql[256];//sql=(char*)malloc(100);

void begin(int cl,int idThread);

int ret;

static int callback(void * str, int argc, char ** argv, char ** azColName) {
  int i;
  char * data = (char * ) str;
  for (i = 0; i < argc; i++) {
    //strcat(data, azColName[i]);
    if (argv[i]) strcat(data, argv[i]);
    else strcat(data, "NULL");
  }
  strcat(data,"\n");
  //printf("data din callback %s %ld",data, strlen(data));
  return 0;
}

sqlite3 * database;

void databaseInitialize()
{
    int database_descriptor = sqlite3_open("database.db", &database);
    //daca nu exista
    sprintf(sql, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='users';");
    
    
    database = sqlite3_exec(database, sql, callback, str, &error_message);
    //printf("\n STR count =====%s===\n",str);
    if(strcmp(str,"0\n")==0)
    {
      int database_descriptor = sqlite3_open("database.db", &database);
      sprintf(sql, "CREATE TABLE users (ID INTEGER,username TEXT,password TEXT,status TEXT);");
      database_descriptor = sqlite3_exec(database, sql, 0, 0, & error_message);
      if(database_descriptor!=SQLITE_OK)
        printf("Eroare la inserarea inregistrarilor");

      sprintf(sql, "INSERT INTO users (ID,username,password,status) VALUES (1,'%s','%s','%s');",
       "maraenache","pass1234","online");
      ret= sqlite3_exec(database, sql, 0, 0, & error_message);
      sprintf(sql, "INSERT INTO users (ID,username,password,status) VALUES (2,'%s','%s','%s');", 
      "andreea","12345678","online");
      ret = sqlite3_exec(database, sql, 0, 0, & error_message);
      sprintf(sql, "INSERT INTO users (ID,username,password,status) VALUES (3,'%s','%s','%s');",
       "maria","maria222","online");
      ret = sqlite3_exec(database, sql, 0, 0, & error_message);
      sprintf(sql, "INSERT INTO users (ID,username,password,status) VALUES (4,'%s','%s','%s');", 
      "bianca","lalala","offline");
      ret = sqlite3_exec(database, sql, 0, 0, & error_message);
      sprintf(sql, "INSERT INTO users (ID,username,password,status) VALUES (5,'%s','%s','%s');",
       "darius","copac787","offline");
      ret = sqlite3_exec(database, sql, 0, 0, & error_message);
      sprintf(sql, "INSERT INTO users (ID,username,password,status) VALUES (6,'%s','%s','%s');", 
      "sebi","09882138","online");
      ret = sqlite3_exec(database, sql, 0, 0, & error_message);
      if(ret!=SQLITE_OK)
          printf("Eroare la inserarea inregistrarilor");
    }
}

void logout_start();

void dbmessagesInitialize()
{
  int database_descriptor = sqlite3_open("database.db", &database);
  //daca nu exista
  sprintf(sql, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='messages';");
  database = sqlite3_exec(database, sql, callback, str, &error_message);
  //printf("\n STR count =====%s===\n",str);
   
  if(strcmp(str,"0\n")==0)
  {
    int database_descriptor = sqlite3_open("database.db", &database);
    sprintf(sql, "CREATE TABLE messages (id_mesaj INTEGER, sender TEXT, receiver TEXT, message TEXT, seen TEXT, infoplus TEXT);");
    database_descriptor = sqlite3_exec(database, sql, 0, 0, & error_message);
    if(database_descriptor!=SQLITE_OK)
        printf("Eroare la creare tabel");    
  }
}


//--------------------------------------------------------------MAINUL
int main (int argc, char *argv[])
{
  //databaseInitialize();
  //logout_start();
  dbmessagesInitialize();
  bzero(str, sizeof(str));
  struct sockaddr_in server;	// structura folosita de server  	
  void threadCreate(int);

  if(argc<2)
  {
    fprintf(stderr,"Eroare: Primul argument este numarul de fire de executie...");
    exit(1);
  }

  nthreads=atoi(argv[1]);
  if(nthreads <=0)
	{
    fprintf(stderr,"Eroare: Numar de fire invalid...");
	  exit(1);
	}
  threadsPool = calloc(sizeof(Thread),nthreads);

   /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror ("[server]Eroare la socket().\n");
    return errno;
  }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));

  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
  {
    perror ("[server]Eroare la bind().\n");
    return errno;
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
  {
    perror ("[server]Eroare la listen().\n");
    return errno;
  }

  printf("Nr threaduri %d \n", nthreads); fflush(stdout);
  int i;
  for(i=0; i<nthreads;i++) threadCreate(i);

  /* servim in mod concurent clientii...folosind thread-uri */
  for ( ; ; ) 
  {
	  printf ("[server]Asteptam la portul %d...\n",PORT);
    pause();				
  }
}
				
void threadCreate(int i)
{
	void *treat(void *);
	pthread_create(&threadsPool[i].idThread,NULL,&treat,(void*)i);
	return; /* threadul principal returneaza */
}

void *treat(void * arg)
{		
		int client;
		struct sockaddr_in from; 
 	  bzero (&from, sizeof (from));
 		printf ("[thread]- %d - pornit...\n", (int) arg);fflush(stdout);
      
		for( ; ; )
		{
			int length = sizeof (from);
			pthread_mutex_lock(&mlock);

			printf("Thread %d pregatit \n",(int)arg);
			if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
			{
	 		  perror ("[thread]Eroare la accept().\n");	  			
			}
			pthread_mutex_unlock(&mlock);
			threadsPool[(int)arg].thCount++;

      begin(client,(int)arg); //procesarea cererii
			/* am terminat cu acest client, inchidem conexiunea */
			close (client);			
		}	
}
//void login(int cl, int idThread, char* username, char * password)
char receiver[30];
char message[200];

void logout_start()
{
  int database_descriptor= sqlite3_open("database.db", &database);
  if(database_descriptor!= SQLITE_OK)
  {
    printf("nu se desch bd");
    sqlite3_free(error_message);
  }
  sprintf(sql, "UPDATE users SET status='offline' where username ='maraenache';");
  database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);  
      
  sprintf(sql, "UPDATE users SET status='offline' where username ='maria';");
  database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);  
  
  sprintf(sql, "UPDATE users SET status='offline' where username ='bianca';");
  database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);  
  
  sprintf(sql, "UPDATE users SET status='offline' where username ='andreea';");
  database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);  
  
  sprintf(sql, "UPDATE users SET status='offline' where username ='darius';");
  database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);  
  
  sprintf(sql, "UPDATE users SET status='offline' where username ='sebi';");
  database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);  
}
void chat(int cl, int idThread, char *sender, char* receiver)
{
  printf("buna din functia chat aici thread %d, cu sender %s si receiver %s \n");
  iesire_chat[idThread]=1;
  while (iesire_chat[idThread])
  {
    char msjback[256];
    msjback[0]=0;
    int n=0;
    char comanda_chat[256];
    bzero(comanda_chat, sizeof(comanda_chat));
    fflush (stdout);
  	if((n=read(cl, &comanda_chat, sizeof(comanda_chat))) < 0)
  	{
  		printf ("[Server] error read [Client].\n");
  	}
    if(n>0)
    {
      bzero(msjback,256);
      int database_descriptor= sqlite3_open("database.db", &database);
      if(database_descriptor!= SQLITE_OK)
      {
        printf("nu se desch bd");
        sqlite3_free(error_message);
      }
    	comanda_chat[strlen(comanda_chat)-1]=0;
      printf("\nCOMANDA CHAT E %s\n",comanda_chat);

      if(strstr(comanda_chat,"send"))
      {
        //LETSGOOOOOOOOOOOOOOOOOOOOOOO
        strcpy(message,comanda_chat+5);
        id_mesaj++;
        printf("\n sender  |%s|,\n receiver |%s|,\n message |%s|\n",sender, receiver,message);
        sql[0]=0;
        str[0]=0;
        sprintf(sql, "INSERT INTO messages (id_mesaj,sender,receiver,message,seen) VALUES (%d,'%s','%s','%s','%s');",id_mesaj,sender,receiver,message,"no");
        ret= sqlite3_exec(database, sql, 0, 0, & error_message);
        strcpy(msjback,"Mesaj trimis cu succes!");
      }
      else if(strstr(comanda_chat,"replyto"))
      {
        char id[10];
        bzero(message,sizeof(message));
        id[0]=0;  
        strcpy (id, comanda_chat + 8);
        strcpy(message, comanda_chat +8);
        printf("comanda chat %s|",comanda_chat);
        int poz;
        for(poz=0;id[poz]!=' ';poz++)
                ;
        poz++; //printf("message 1 %d %s %s %s\n", poz, message, message+2, message+poz);
        strcpy(message, message+poz);
        id[poz]=0;
        str[0]=0; 
        sql[0]=0;
        sprintf(sql, "INSERT INTO messages (id_mesaj,sender,receiver,message,seen,infoplus) VALUES (%d,'%s','%s','%s','%s','(raspuns la mesajul cu id %s)');",id_mesaj,sender,receiver,message,"no",id);
        ret= sqlite3_exec(database, sql, 0, 0, & error_message);      
        strcpy(msjback,"\n am ajuns aici replyto\n");

       }
       else if(strstr(comanda_chat, "refresh"))
       {
          sql[0]=0;
          inbox[0]=0;
          sprintf(sql, "SELECT 'id>', id_mesaj,'|',infoplus,' ',sender, '->', receiver, ':', message from messages WHERE (sender='%s' and receiver='%s') or (sender='%s' and receiver='%s');",
          sender,receiver,receiver,sender);
          ret= sqlite3_exec(database, sql, callback, inbox, & error_message);
          printf("inbox %s",inbox);
          strcpy(msjback,"\nConversatie actualizata!\n");
       }
       else if(strstr(comanda_chat, "backtomenu"))
       {
        strcpy(msjback,"\nAti revenit in meniul principal!\n");
        iesire_chat[idThread]=0;
       }
       else
       {
        strcpy(msjback, "[chat]comanda indisponibila");
       }
      if(inbox[0])
      {
          if (write (cl, inbox,strlen(inbox)) <= 0)
        {
          printf("[Thread %d] ",idThread);
          perror ("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",idThread);
        inbox[0]=0;
        
      }
      else if(msjback[0])
      {
        if (write (cl, msjback,256) <= 0)
        {
          printf("[Thread %d] ",idThread);
          perror ("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",idThread);   
      }
    }
  }
}
void menu (int cl,int idThread)
{
  //printf("\n\nBuna, sunt thread %d si am intrat in menu cu username-ul%s",idThread,username);
  iesire_while[idThread]=1;
  while(iesire_while[idThread])
  {
    char msjback[256];
    msjback[0]=0;
    int n=0;
    char comanda[256];
    bzero(comanda, sizeof(comanda));
    fflush (stdout);
  	if((n=read(cl, &comanda, sizeof(comanda))) < 0)
  	{
  		printf ("[Server] error read [Client].\n");
  	}
    if(n>0)
    {
      bzero(msjback,256);
      int database_descriptor= sqlite3_open("database.db", &database);
      if(database_descriptor!= SQLITE_OK)
      {
        printf("nu se desch bd");
        sqlite3_free(error_message);
      }
    	comanda[strlen(comanda)-1]=0;

      //printf("AM citit %s %ld", comanda, strlen(comanda));
      if(strcmp(comanda,"help")==0)
      {
          strcat(msjback,"Comanda quit permite...\nComanda selectUser...");
          printf("\n%s\n",msjback);
      }
      else if(strcmp(comanda, "usersList")==0)
      {
          //printf("\nuserslist letsgooo\n");
          sql[0]=0;
          str[0]=0;
          //sql[0]="sir maxi de trim arata cam asa de lung se opoate ajusta dar cred ca e suficient momentan avand in vedere ca nu asta e scopul problemei"
          sprintf(sql, "SELECT username FROM users");
          database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);
        //  printf("users list ->> \n%s\n", str);
          strcat(msjback,"LISTA TUTUROR UTILIZATORILOR\n");
          strcat(msjback,str);
          strcat(msjback,"\n");
          printf("\n%s\n",msjback);
      }
      else if(strcmp(comanda, "onlineUsers")==0)
      {
        sql[0]=0;
        str[0]=0;
          
        sprintf(sql, "SELECT username FROM users WHERE status ='online';"); 
        database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);
        //printf("STR E ->> \n%s\n", str);
        strcat(msjback,"LISTA UTILIZATORILOR ACTIVI\n");
        strcat(msjback,str);
        strcat(msjback,"\n");
        printf("\n%s\n",msjback);
      }
      else if(strcmp(comanda, "logout")==0)
      {
        iesire_while[idThread]=0;
        printf("Buna aici thread %d ma ocup sa o deconectez pe %s\n",idThread,users[idThread]);
        sprintf(sql, "UPDATE users SET status='offline' where username ='%s';", users[idThread]);
        database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);  
        strcat(msjback,"Utilizatorul ");
        strcat(msjback,users[idThread]);
        strcat(msjback,"a fost deconectat cu succes!\n Pentru a va conecta folositi \nSINTAXA login <username> <password>\n");
        printf("\n%s\n",msjback);
      }
      else if(strcmp(comanda, "getInbox")==0)
      {
        //sprintf(sql, "UPDATE users SET status='offline' where username ='%s' AND password ='%s';", username, password);
        //database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);  
        strcat(msjback,"Nu aveti mesaje necitite!\n");
        printf("\n%s\n",msjback);
        inbox[0]=0;
        
        sprintf(sql, "SELECT sender,': ', message FROM messages WHERE receiver = '%s' and seen = '%s';", users[idThread],"no");  
        database_descriptor = sqlite3_exec(database, sql, callback, inbox, &error_message);
        printf("%s\n",inbox);     
        if(inbox[0])
        {
          sql[0]=0;
          sprintf(sql,"UPDATE messages SET seen='yes' where receiver = '%s' and seen = 'no';", users[idThread]); 
          database_descriptor = sqlite3_exec(database, sql, 0, 0, &error_message);
          printf("am modificat\n");
        }
      
      
      }
      else if(strstr(comanda,"chat"))
      {
        char user[20];
        strcpy(user,comanda+5);
        strcat(msjback,"Conversatia cu userul ");
        strcat(msjback,user);

        str[0]=0;
        sql[0]=0;
        
        sprintf(sql, "SELECT '1' from users where username='%s'",user);

        ret= sqlite3_exec(database, sql, callback, str, &error_message);
        //printf("\nUtilizatorul e in bd->STR %s\n",str);
        
        if(strstr(str,"1"))
        {
          //utiliz e in sistem
            
            sql[0]=0;
            inbox[0]=0;
            sprintf(sql, "SELECT 'id>', id_mesaj,'|',sender, '->', receiver, ':', message from messages WHERE (sender='%s' and receiver='%s') or (sender='%s' and receiver='%s');",
            users[idThread],user,user,users[idThread]);
            ret= sqlite3_exec(database, sql, callback, inbox, & error_message);
            printf("inbox %s",inbox);
            if(inbox[0])
            {
              if (write (cl,inbox,256) <= 0)
              {
               printf("[Thread %d] ",idThread);
               perror("[Thread]Eroare la write() catre client.\n");
              }
              else
              printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",idThread);   
              inbox[0]=0;
              msjback[0]=0;
              printf("merg in functia chat");
              chat(cl,idThread,users[idThread],user);

            }
            else 
            {
              strcpy(msjback,"Nu aveti mesaje cu acest utilizator.\n Folositi comanda sendto pentru a incepe conversatia\n\n");
            }
        }
        else
        {
            strcpy(msjback,"Utilizatorul introdus nu a putut fi identificat.\nPoti deschide conversatii doar cu utilizatorii din sistem.\nIncearca comanda usersList\n");
        }
      
      }
      else if(strstr(comanda, "sendto"))
      {

        printf("Thread %d am aj aici cu userul %s comanda sendto %s\n",idThread,users[idThread],comanda);
      
        bzero(receiver, sizeof (receiver));
        bzero(message,sizeof(message));
        
        //decupare username si password
        strcpy (receiver, comanda + 7);
        strcpy(message, comanda+7);
        
        int poz=0;
        while(receiver[poz]!=' ')
          poz++;
        
        str[0]=0;
        sql[0]=0;
      
        strcpy(message, message+poz+1);

        receiver[poz]=0;
        sprintf(sql, "SELECT '1' from users where username='%s'",receiver);

        ret= sqlite3_exec(database, sql, callback, str, & error_message);
        //printf("\nUtilizatorul e in bd->STR %s\n",str);
        if(strstr(str,"1"))
        {
          //utiliz e in sistem
            id_mesaj++;
            printf("\n sender  |%s|,\n receiver |%s|,\n message |%s|\n",users[idThread], receiver,message);
            sql[0]=0;
            str[0]=0;
            sprintf(sql, "INSERT INTO messages (id_mesaj,sender,receiver,message,seen) VALUES (%d,'%s','%s','%s','%s');",id_mesaj,users[idThread],receiver,message,"no");
            ret= sqlite3_exec(database, sql, 0, 0, & error_message);
            strcpy(msjback,"Mesaj trimis cu succes!");
        }
        else
        {
            strcpy(msjback,"Utilizatorul introdus nu a putut fi identificat.\nPentru a vedea utilizatorii carora le poti trimite mesaj incearca comanda usersList\n");
        }
      
      }
      else
      {
          strcpy(msjback,"comanda gresita");
      }
      /*else if(strcmp(comanda,"quit"))
      {
        strcpy(msjback,"quit");
      }*/
      
      if(inbox[0])
      {
          if (write (cl, inbox,strlen(inbox)) <= 0)
        {
          printf("[Thread %d] ",idThread);
          perror ("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",idThread);
        inbox[0]=0;
        
      }
      else if(msjback[0])
      {
        if (write (cl, msjback,256) <= 0)
        {
          printf("[Thread %d] ",idThread);
          perror ("[Thread]Eroare la write() catre client.\n");
        }
        else
          printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",idThread);   
      }
    }
     
  }
    
}
char raspuns[256];
void begin (int cl,int idThread)
{
  
    char msj[256];
    bzero(msj,256);
      
    strcpy(msj,"Bine ai venit! Introduceti username si parola pt conectare!\nSINTAXA login <username> <password>");
    if (write (cl, msj, 256) <= 0)
    {
      perror ("[Thread]Eroare la write() catre client.\n");
    }
    else
      printf ("[Thread]Mesajul a fost transmis cu succes.\n");

    char mesaj[256];//mesajul primit de trimis la client  
    //int log=1;
    while(1)
    {
                      
      bzero(mesaj,256);
      if (read (cl, &mesaj,sizeof(mesaj)) <= 0)
      {
        printf("[Thread %d]\n",idThread);
        perror ("Eroare la read() de la client.\n");
      }
      printf ("Thread din begin %d am prim mesajul...%s\n",idThread, mesaj);

      int database_descriptor= sqlite3_open("database.db", &database);
    
      if(database_descriptor!= SQLITE_OK)
      {
        printf("nu se desch bd");
        sqlite3_free(error_message);
      }
    

 //login
      if (strstr (mesaj, "login") != NULL)
      {
       
        bzero(raspuns, sizeof (raspuns));
        bzero(username, sizeof (username));
        bzero(password, sizeof (password));
        bzero (sql, sizeof(sql));
        bzero (str, sizeof(str));

        //decupare username si password
        strcpy (username, mesaj + 6);
        
        int poz=0;
        while(username[poz]!=' ')
          poz++;//printf("j e %d", j);
        strcpy(password, username+poz+1);
        username[poz]=0;
        password[strlen(password)-1]=0;
        strcpy(users[idThread],username);
        strcpy(parole[idThread],password);
        
        //printf("\n username %s cu %ld litere\n parola %s cu %ld litere",username, strlen(username), password, strlen(password));
        //login(cl, idThread, username, password);

        sprintf(sql, "SELECT username FROM users WHERE username = '%s' and password ='%s' ;", username,password);
        
        database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);
  
        /*  if (database_descriptor != SQLITE_OK)
          {
            printf ("1 SQL error: %s \n %s\n", error_message, str);
            sqlite3_free (error_message);
          }                        
           else 
        */
        str[strlen(str)-1]=0;
        //printf("\nstr e %s\n",str);
        if(strcmp(str, username)==0)
        {
          
          strcpy(raspuns,"Te-ai logat cu succes! \n");
          strcat(raspuns,"\nMENIUL PRINCIPAL!\n\nAveti urmatoarele optiuni de comenzi:\n\t1)usersList\n\t2)onlineUsers\n\t3)help\n\t4)quit\n\t5)logout\n");
            
          if (write (cl,raspuns,256) <= 0)
          {
           printf("[Thread %d] ",idThread);
           perror("[Thread]Eroare la write() catre client.\n");
          }
          else
          printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",idThread);   
          
          sql[0]=0;
          str[0]=0;
          printf("\naici thread %d ma ocup de user %s sa o conectez\n",idThread,users[idThread]);
          sprintf(sql, "UPDATE users SET status='online' where username='%s';",users[idThread]);
        
          database_descriptor = sqlite3_exec(database, sql, callback, str, &error_message);
          iesire_while[idThread]=1;
          menu(cl, idThread);
        } 
        else 
        {  
          strcpy (raspuns, "Nume utilizator sau parola au fost introduse gresit! Incercati din nou ");
          //de fct aici sa poate incerca din nou;
          if (write (cl, raspuns, 256) <= 0)
          {
           printf("[Thread %d] ",idThread);
           perror ("[Thread]Eroare la write() catre client.\n");
          }
          else
          printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",idThread);   

        }

      }//printf("RASPUNS %s ",raspuns);
     
    }    
       
}
