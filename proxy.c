#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include "./simpleSocketAPI.h"


#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port
#define PORTFTP "21"                  // Port du serveur FTP


int main(){
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockRDV;                 // Descripteur de socket de rendez-vous
    int descSockCOM;                 // Descripteur de socket de communication
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    socklen_t len;                   // Variable utilisée pour stocker les 
                                     // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur
    
    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par 
                                      // la fonction getaddrinfo

     // Récupération des informations du serveur
     ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
     if (ecode) {
         fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
         exit(1);
     }
     // Publication de la socket
     ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
     if (ecode == -1) {
         perror("Erreur liaison de la socket de RDV");
         exit(3);
     }
     // Nous n'avons plus besoin de cette liste chainée addrinfo
     freeaddrinfo(res);

     // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
     len=sizeof(struct sockaddr_storage);
     ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
     if (ecode == -1)
     {
         perror("SERVEUR: getsockname");
         exit(4);
     }
     ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN, 
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
     if (ecode != 0) {
             fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
             exit(4);
     }
     printf("L'adresse d'ecoute est: %s\n", serverAddr);
     printf("Le port d'ecoute est: %s\n", serverPort);

     // Definition de la taille du tampon contenant les demandes de connexion
     ecode = listen(descSockRDV, LISTENLEN);
     if (ecode == -1) {
         perror("Erreur initialisation buffer d'écoute");
         exit(5);
     }

    len = sizeof(struct sockaddr_storage);
     // Attente connexion du client
     // Lorsque demande de connexion, creation d'une socket de communication avec le client
     descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
     if (descSockCOM == -1){
         perror("Erreur accept\n");
         exit(6);
     }
    // Echange de données avec le client connecté

    /*****
     * Testez de mettre 220 devant BLABLABLA ...
     * **/
    strcpy(buffer, "220 BLABLABLA Proxy working...\n");
    write(descSockCOM, buffer, strlen(buffer));

    /*******
     * 
     * A vous de continuer !
     * 
     * *****/

    // Première étape : lire le message envoyé par le client //? (login@serveur)
    ecode=read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1){
        perror("Erreur lecture dans socket\n");
        exit(7);
    }
    buffer[ecode]='\0';
    printf("Message recu: %s\n", buffer);

    // Deuxième étape : Découper la chaîne de caractère reçue 
    char login[50];
    sscanf(buffer,"%[^@]@%s",login,serverAddr);
    printf("Login : %s\nServeur : %s\n",login,serverAddr);

    // Troisième étape : Se connecter au serveur FTP //? (ftp.fau.de) + création du socket
    int sockServeurCMD;
    ecode = connect2Server(serverAddr, PORTFTP, &sockServeurCMD);
    if (ecode == -1){
        perror("Erreur connexion au serveur\n");
        exit(8);
    }
    printf("Connexion au serveur réussie\n");

    // Quatrième étape : lire le message envoyé par le serveur 
    ecode=read(sockServeurCMD, buffer, MAXBUFFERLEN-1);
    if (ecode == -1){
        perror("Erreur lecture dans socket\n");
        exit(9);
    }
    buffer[ecode]='\0';
    printf("Message recu du serveur : %s\n", buffer);

    // Cinquième étape : envoyer les informations de connexion au serveur //? (login)
    sprintf(buffer,"%s\r\n",login);
    ecode = write(sockServeurCMD, buffer, strlen(buffer));
    if (ecode == -1){
        perror("Erreur écriture dans socket\n");
        exit(10);
    }
    printf("Message envoyé au serveur : %s\n", buffer);

    // Sixième étape : lire le message envoyé par le serveur 
    ecode=read(sockServeurCMD, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){
        perror("Erreur lecture dans socket\n");
        exit(9);
    }
    buffer[ecode]='\0';
    printf("Message recu du serveur : %s\n", buffer);

    // Septième étape : envoyer le message du serveur au client
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if(ecode == -1){
        perror("Erreur écriture dans socket\n");
        exit(10);
    }
    printf("Message envoyé au client : %s\n", buffer);

    // Huitème étape : lire le mot de passe envoyé par le client //? (email)
    ecode=read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1){
        perror("Erreur lecture dans socket\n");
        exit(7);
    }
    buffer[ecode]='\0';
    printf("Message recu : %s\n", buffer);

    // Neuvième étape : envoyer le mot de passe au serveur //? (Password: email)
    write(sockServeurCMD, buffer, strlen(buffer));
    printf("Message envoyé au serveur : %s\n", buffer);

    // Dixième étape : lire le message envoyé par le serveur
    ecode=read(sockServeurCMD, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){
        perror("Erreur lecture dans socket\n");
        exit(9);
    }
    buffer[ecode]='\0';
    printf("Message recu du serveur : %s\n", buffer);

    //  Onzième étape : envoyer le message du serveur au client //? (230 Anonymous access granted, restrictions apply)
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if(ecode == -1){
        perror("Erreur écriture dans socket\n");
        exit(10);
    }
    printf("Message envoyé au client : %s\n", buffer);
    

    // Douzième étape : lire le message envoyé par le client //? auto SYST (système d'exploitation)
    ecode=read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1){
        perror("Erreur lecture dans socket\n");
        exit(7);
    }
    buffer[ecode]='\0';
    printf("Message recu : %s\n", buffer);

    // Treizième étape : envoyer la requête SYST au serveur 
    write(sockServeurCMD, buffer, strlen(buffer));
    printf("Message envoyé au serveur : %s\n", buffer);

    // Quatorzième étape : lire le message envoyé par le serveur
    ecode=read(sockServeurCMD, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){
        perror("Erreur lecture dans socket\n");
        exit(9);
    }
    buffer[ecode]='\0';
    printf("Message recu du serveur : %s\n", buffer);

    // Quinzième étape : envoyer le message du serveur au client
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if(ecode == -1){
        perror("Erreur écriture dans socket\n");
        exit(10);
    }
    printf("Message envoyé au client : %s\n", buffer);

    // Seizième étape : lire le message envoyé par le client //? read PORT
    ecode=read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1){
        perror("Erreur lecture dans socket\n");
        exit(7);
    }
    buffer[ecode]='\0';
    printf("Message recu : %s\n", buffer);

    // Dix-septième étape : découper le message pour récupérer les informations de connexion
    char ip[20];
    int port;
    int port1, port2;

    sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &port1, &port2);
    sprintf(ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    port = (port1 << 8) + port2;
    char portStr[10];
    sprintf(portStr, "%d", port);

    // Dix-huitième étape : connecter le client au serveur de données
    int dataSock;
    ecode = connect2Server(ip, portStr, &dataSock);
    if (ecode == -1){
        perror("Erreur connexion au serveur\n");
        exit(8);
    }
    printf("Connexion au serveur réussie\n");

    // Dix-neuvième étape : envoyer la requête PASV au serveur
    sprintf(buffer, "PASV\r\n", strlen("PASV\r\n"));
    ecode = write(sockServeurCMD, buffer, strlen(buffer));
    if(ecode == -1){
        perror("Erreur écriture dans socket\n");
        exit(10);
    }
    printf("Message envoyé au serveur : %s\n", buffer);

    // Vingtième étape : lire le message envoyé par le serveur
    ecode=read(sockServeurCMD, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){
        perror("Erreur lecture dans socket\n");
        exit(9);
    }
    buffer[ecode]='\0';
    printf("Message recu du serveur : %s\n", buffer);

    // Récupérer les informations de connexion du serveur de données
    int ipSrv1, ipSrv2, ipSrv3, ipSrv4;
    int portSrv;
    int portS1, portS2;
    sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ipSrv1, &ipSrv2, &ipSrv3, &ipSrv4, &portS1, &portS2);
    portSrv = (portS1 << 8) + portS2;
    char portStrSrv[10];
    char ipSrvStr[20];
    sprintf(portStrSrv, "%d", portSrv);
    sprintf(ipSrvStr, "%d.%d.%d.%d", ipSrv1, ipSrv2, ipSrv3, ipSrv4);
    printf("ipSrvStr : %s\n", ipSrvStr);
    printf("portSrtSrv : %s\n", portStrSrv);


    // Vingt-et-unième étape : connecter le serveur au client de données
    int serverDataSock;
    ecode = connect2Server(ipSrvStr, portStrSrv, &serverDataSock);
    if (ecode == -1){
        perror("Erreur connexion au serveur\n");
        exit(8);
    }
    printf("Connexion au serveur réussie\n");

    


    //Fermeture de la connexion
    printf("Fermeture de la connexion\n");
    close(descSockCOM);
    close(descSockRDV);
    close(sockServeurCMD); // Fermeture du socket de connexion au serveur
}
