//
// Created by cristi on 19.04.2017.
//
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include "server.h"

#define BUFLEN 100
#define MAX_CLIENTS 20

using namespace std;

vector<user> usersList; // vector in care voi tine toti userii din fisierul de intrare
vector<user> loggedUsers; // vector in care voi tine userii logati pe clienti

/*
 * metoda ce cauta un user logat pe unul dintre clientii deschisi in functie de numarul socketului
 */
int searchLogUser(int socketN) {
    int i;
    int nr = loggedUsers.size();
    for (i = 0; i < nr; i++) {
        if (loggedUsers[i].nrSocket == socketN) {
            return i;
        }
    }
    return -1;
}

/*
 * metoda ce cauta un card in lista de useri citita din fisierul de intrare
 */
int searchUsersListByCard(int nrCard) {
    int i;
    int nr = usersList.size();
    for (i = 0; i < nr; i++) {
        if (usersList[i].nrCard == nrCard) {
            return i;
        }
    }
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "Numar de argumente incorect\n";
        return 0;
    }
    int i, checkRecv;
    int count = 0; // contorul ce va numara de cate ori consecutiv a fost intrudus un PIN gresit
    int sockfd, newsockfd;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr;
    user auxUser;
    string auxString;
    int nrUsers;
    int auxInteger;
    double auxDouble;
    ifstream myfile("test.txt");
    myfile >> nrUsers;
    // dupa ce am citit numarul de useri din fisier incep sa citesc datele specifice fiecaruia si ii adaug in lista de useri
    for (i = 0; i < nrUsers; i++) {
        myfile >> auxString;
        auxUser.firstName = auxString;
        myfile >> auxString;
        auxUser.secondName = auxString;
        myfile >> auxInteger;
        auxUser.nrCard = auxInteger;
        myfile >> auxInteger;
        auxUser.pin = auxInteger;
        myfile >> auxString;
        auxUser.password = auxString;
        myfile >> auxDouble;
        auxUser.sold = auxDouble;
        usersList.push_back(auxUser);
    }
    myfile.close();

    fd_set read_fds; //multimea de citire folosita in select
    fd_set temp_fds; //multime folosita temporar
    int fdmax; //valoare maxima file descriptor din multimea read_fds

    //golim cele doua multimi
    FD_ZERO(&read_fds);
    FD_ZERO(&temp_fds);

    //deschid socket pentru a putea comunica
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cout << "Error opening socket\n";
        return -1;
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;// foloseste adresa IP a masinii
    serv_addr.sin_port = htons(atoi(argv[1]));
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
        cout << "Error binding\n";
        return -1;
    }
    listen(sockfd, MAX_CLIENTS);
    //adaugam socketul pe care se asculta conexiuni in multimea read_fds
    FD_SET(sockfd, &read_fds);
    FD_SET(0, &read_fds);
    fdmax = sockfd;
    //incep sa ascult comenzi de la clienti/adaug noi clienti in multimea descriptorilor
    int oldNrCard = 0; // variabila folosita pentru a memora numarul de card si a sti daca se incearca logarea de pe acelasi card

    while (1) {
        temp_fds = read_fds;
        if (select(fdmax + 1, &temp_fds, NULL, NULL, NULL) == -1)
            cout << "Eroare select \n";
        for (i = 0; i <= fdmax; i++) {
            if (!FD_ISSET(i, &temp_fds))
                continue;
            if (i == 0) {
                //se primeste o instructiune de la tastura, verific daca este comanda quit
                memset(buffer, 0, BUFLEN);
                cin.getline(buffer, BUFLEN);
                if (strcmp(buffer, "quit") == 0) {
                    // in cazul in care se primeste quit voi inchide socketul
                    close(sockfd);
                    return 0;
                }
            }
            else if (i == sockfd) {
                socklen_t clilen = sizeof(cli_addr);
                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if (newsockfd == -1) {
                    cout << "Error la accept \n";
                }
                else {
                    //adaug noul socket la multimea descriptorilor de citire
                    FD_SET(newsockfd, &read_fds);
                    if (newsockfd > fdmax)
                        fdmax = newsockfd;
                }
                printf("Noua conexiune de la %s, port %d, socket_client %d\n", inet_ntoa(cli_addr.sin_addr),
                       ntohs(cli_addr.sin_port), newsockfd);
            }
            else {
                // am primit date pe unul din socketii cu care vorbesc cu clientii
                //actiunea serverului: recv()
                memset(buffer, 0, BUFLEN);
                checkRecv = recv(i, buffer, sizeof(buffer), 0);
                if (checkRecv <= 0) {
                    close(i);
                    FD_CLR(i, &read_fds); //scot socketul din multimea de citire
                    cout << "Eroare receive\n";
                }

                else {
                    cout << "Cerere de " << buffer << " venita de la clientul de pe socketul " << i << endl;
                    int cardIndex = 0;
                    char backMessage[50];// variabila folosita pentru trimiterea de mesaje de la server
                    char command[BUFLEN];
                    char restOfCommand[BUFLEN];// parametrii primiti de comanda
                    sscanf(buffer, "%s", command); //primul cuvant din comanda (comanda propriu zisa, fara parametri)
                    user auxLoggedUser;
                    char *bufferSplit[5]; // vector in care memorez cuvintele din buffer
                    char *aux = strtok(buffer, " ");
                    int poz = 0;
                    while (aux) {
                        bufferSplit[poz] = aux;
                        poz++;
                        aux = strtok(NULL, " ");
                    }
                    if (strcmp(command, "login") == 0) {
                        int nrCard = atoi(bufferSplit[1]);
                        //bufferSplit[1] va contine numarul cardului pe care vreau sa fac login
                        cardIndex = searchUsersListByCard(atoi(bufferSplit[1]));
                        //verific existenta cardului(si a userului ce il detine) in lista de useri
                        if (cardIndex == -1) {
                            sprintf(backMessage, "-4 : Numar card inexistent\n");
                            send(i, backMessage, BUFLEN, 0);
                        }
                            //daca numarul cardului exista dar exista deja un client de pe care este logat
                        else if (cardIndex != -1 && usersList[cardIndex].logged == true) {
                            sprintf(backMessage, "ATM> -2 : Sesiune deja deschisa\n");
                            send(i, backMessage, BUFLEN, 0);
                        }
                        else {
                            //daca nu am client de pe care este logat acel card, verific PIN-ul
                            //PIN-ul este reprezentat de bufferSplit[2]
                            if (usersList[cardIndex].pin == atoi(bufferSplit[2])) {
                                strcpy(restOfCommand, buffer + strlen("login") + 1);
                                string name;
                                usersList[cardIndex].logged = true;
                                name = usersList[cardIndex].firstName + ' ' + usersList[cardIndex].secondName;
                                auxLoggedUser.firstName = usersList[cardIndex].firstName;
                                auxLoggedUser.secondName = usersList[cardIndex].secondName;
                                auxLoggedUser.nrCard = usersList[cardIndex].nrCard;
                                auxLoggedUser.password = usersList[cardIndex].password;
                                auxLoggedUser.pin = usersList[cardIndex].pin;
                                auxLoggedUser.logged = true;
                                auxLoggedUser.block = false;
                                auxLoggedUser.nrSocket = i;
                                auxLoggedUser.sold = usersList[cardIndex].sold;
                                loggedUsers.push_back(auxLoggedUser); //adaug noul user in lista de useri logati
                                count = 0;
                                sprintf(backMessage, "ATM> Welcome %s\n", name.c_str());
                                send(i, backMessage, BUFLEN, 0);
                            }
                            if (cardIndex != -1 && usersList[cardIndex].pin != atoi(bufferSplit[2])) {
                                //in cazul in care este introdus un pin gresit, se incrementeaza contorul count
                                if ((oldNrCard == nrCard) || (oldNrCard == 0)) {
                                    count++;
                                    oldNrCard = nrCard;
                                }
                                else if (nrCard != oldNrCard) {
                                    count = 1;
                                    oldNrCard = nrCard;
                                }
                                //cardul va fi blocat dupa 3 introduceri gresite consecutive
                                if (count == 3) {
                                    count = 0;
                                    sprintf(backMessage, "ATM> -5 : Card blocat\n");
                                    usersList[cardIndex].block = true;
                                    send(i, backMessage, BUFLEN, 0);
                                }
                                else {
                                    sprintf(backMessage, "ATM> -3 : Pin gresit\n");
                                    send(i, backMessage, BUFLEN, 0);
                                }
                            }
                        }
                    }
                    else if ((strcmp(command, "logout") == 0)) {
                        //pentru comanda de logout, caut userul ce era logat si ii modific variabila logged,
                        //stergandu-l in acelasi timp din lista de useri logati
                        int userPoz = searchLogUser(i);
                        loggedUsers[userPoz].logged = false;
                        int index = searchUsersListByCard(loggedUsers[userPoz].nrCard);
                        usersList[index].logged = false;
                        //sterg userul din lista de useri logati
                        loggedUsers.erase(loggedUsers.begin() + userPoz);
                        sprintf(backMessage, "ATM> Deconectare de la bancomat\n");
                        send(i, backMessage, BUFLEN, 0);
                    }
                    else if ((strcmp(command, "listsold") == 0)) {
                        //pentru comanda listsold, caut userul ce o initiaza dupa numarul socketului si
                        //trimit inapoi catre clientul soldul curent
                        unsigned int j;
                        for (j = 0; j < loggedUsers.size(); j++) {
                            if (loggedUsers[j].nrSocket == i) {
                                int poz = searchUsersListByCard(loggedUsers[j].nrCard);
                                double sold = usersList[poz].sold;
                                sprintf(backMessage, "ATM> %.2f\n", sold);
                                send(i, backMessage, BUFLEN, 0);
                                break;
                            }
                        }
                    }
                    else if ((strcmp(command, "getmoney") == 0)) {
                        //pentru comanda getmoney, caut userul la fel cum am procedat pentru listsold si dupa
                        //ce fac verificarile necesare scad suma de bani ceruta de client (daca este posibil)
                        strcpy(restOfCommand, buffer + strlen("getmoney") + 1);
                        int val = atoi(restOfCommand);
                        unsigned int j;
                        for (j = 0; j < loggedUsers.size(); j++) {
                            if (loggedUsers[j].nrSocket == i) {
                                int poz = searchUsersListByCard(loggedUsers[j].nrCard);
                                double sold = usersList[poz].sold;
                                if (val % 10 != 0) {
                                    sprintf(backMessage, "ATM> −9 : Suma nu este multiplu de 10\n");
                                    send(i, backMessage, BUFLEN, 0);
                                    break;
                                }
                                if (val > sold) {
                                    sprintf(backMessage, "ATM> −8 : Fonduri insuficiente\n");
                                    send(i, backMessage, BUFLEN, 0);
                                    break;
                                }
                                usersList[poz].sold = usersList[poz].sold - val;
                                sprintf(backMessage, "ATM> Suma %s retrasa cu succes\n", restOfCommand);
                                send(i, backMessage, BUFLEN, 0);
                                break;
                            }
                        }
                    }
                    else if ((strcmp(command, "putmoney") == 0)) {
                        // dupa ce gasesc userul ce a adaugat bani in contul propriu, adaug suma in contul acestuia
                        strcpy(restOfCommand, buffer + strlen("putmoney") + 1);
                        double val = atof(restOfCommand);
                        unsigned int j;
                        for (j = 0; j < loggedUsers.size(); j++) {
                            if (loggedUsers[j].nrSocket == i) {
                                int poz = searchUsersListByCard(loggedUsers[j].nrCard);
                                usersList[poz].sold = usersList[poz].sold + val;
                                sprintf(backMessage, "ATM> Suma depusa cu succes\n");
                                send(i, backMessage, BUFLEN, 0);
                                break;
                            }
                        }
                    }
                    else if (strcmp(command, "quit") == 0) {
                        // in cazul in care unul dintre clienti trimite comanda quit, userul logat pe el in acel
                        //moment va fi delogat si va fi sters din lista de useri logati
                        sprintf(backMessage, "ATM> Clientul va fi deconectat de la server\n");
                        send(i, backMessage, BUFLEN, 0);
                        int userPoz = searchLogUser(i);
                        if (userPoz != -1) {
                            loggedUsers[userPoz].logged = false;
                            int index = searchUsersListByCard(loggedUsers[userPoz].nrCard);
                            usersList[index].logged = false;
                            //sterg userul din lista de useri logati
                            loggedUsers.erase(loggedUsers.begin() + userPoz);
                        }
                    }
                }
            }
        }

    }
    return 0;
}   
