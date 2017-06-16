//
// Created by cristi on 19.04.2017.
//

#include "client.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <string>
#include <dirent.h>
#include <fstream>
#include "user.h"

#define BUFLEN 100

using namespace std;

bool isValidCommand(char buffer[]) {
    return ((strcmp(buffer, "listsold") == 0) || (strcmp(buffer, "getmoney") == 0) ||
            (strcmp(buffer, "putmoney") == 0));
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];
    char tempRecv[BUFLEN];
    if (argc < 3) {
        cout << "Numar invalid de parametri!\n";
        return -1;
    }
    int checkRecv; // varibiala prin care verific daca datele de la server au fost primite corect
    int pid = getpid();
    char fileName[100];
    sprintf(fileName, "client-%d.log", pid); // numele fisierului de iesire
    ofstream myfile;
    myfile.open(fileName);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cout << "ERROR opening socket\n";
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        cout << "ERROR connecting";
        return -1;
    }
    bool isLogged = false; // variabila in care memorez daca un user este logat in procesul client

    while (1) {
        // citesc de la tastatura comenzile
        memset(buffer, 0, BUFLEN);
        cin.getline(buffer, BUFLEN);
        myfile << buffer << endl;
        char command[BUFLEN]; // variabila in care memorez prima parte a comenzii (in cazul in care este compusa din mai multe cuvinte)
        sscanf(buffer, "%s", command);
        if (strcmp(command, "login") == 0) {
            if (isLogged == false) {
                send(sockfd, buffer, strlen(buffer), 0);
                checkRecv = recv(sockfd, tempRecv, sizeof(tempRecv), 0);
                // verific daca datele de la server au fost primite corect, altfel inseamna ca acesta a fost deconectat,
                // caz in care voi inchide socketul si procesul client
                if (checkRecv <= 0) {
                    cout << "-10 : Eroare la apel> serverul a fost deconectat\n";
                    myfile << "-10 : Eroare la apel> serverul a fost deconectat\n";
                    myfile.close();
                    return 0;
                }
                // daca s-a introdus un pin gresit, afisez mesajul specific
                if (strncmp(tempRecv, "ATM> Welcome", 11) != 0) {
                    cout << tempRecv;
                    myfile << tempRecv;
                }
                if (strncmp(tempRecv, "ATM> Welcome", 11) == 0) {
                    // daca am introdus un cod pin corect, astept primirea urmatorului mesaj (de tipul Welcome) si actualizez variabila isLogged
                    isLogged = true;
                    cout << tempRecv;
                    myfile << tempRecv;
                }
            }
            else { //daca sunt deja autentificat
                cout << "-2 : Sesiune deja deschisa\n";
                myfile << "-2 : Sesiune deja deschisa\n";
            }
        }
        else if (strcmp(command, "quit") == 0) {
            // in cazul comenzii quit voi anunta serverul pentru a ma delogat mai intai
            send(sockfd, buffer, strlen(buffer), 0);
            checkRecv = recv(sockfd, tempRecv, sizeof(tempRecv), 0);
            if (checkRecv <= 0) {
                cout << "-10 : Eroare la apel> serverul a fost deconectat\n";
                myfile << "-10 : Eroare la apel> serverul a fost deconectat\n";
                myfile.close();
                return 0;
            }
            cout << tempRecv;
            myfile << tempRecv;
            isLogged = false;
            close(sockfd);
            myfile.close();
            return 0;
        }
        else if (isLogged == false) {
            // ma asigur ca urmatoarele comenzi vor fi efectuate doar daca un user este logat
            cout << "-1 Clientul nu e autentificat\n";
            myfile << "-1 Clientul nu e autentificat\n";
        }
            // urmatoarele comenzi vor fi trimise catre server pentru a fi procesate, ulterior afisand si scriind raspunsul primit
        else if (strcmp(command, "logout") == 0) {
            send(sockfd, buffer, strlen(buffer), 0);
            isLogged = false;
            checkRecv = recv(sockfd, tempRecv, sizeof(tempRecv), 0);
            if (checkRecv <= 0) {
                cout << "-10 : Eroare la apel> serverul a fost deconectat\n";
                myfile << "-10 : Eroare la apel> serverul a fost deconectat\n";
                myfile.close();
                return 0;
            }
            cout << tempRecv;
            myfile << tempRecv;
        }
        else if (isValidCommand(command)) {
            send(sockfd, buffer, strlen(buffer), 0);
            checkRecv = recv(sockfd, tempRecv, sizeof(tempRecv), 0);
            if (checkRecv <= 0) {
                cout << "-10 : Eroare la apel> serverul a fost deconectat\n";
                myfile << "-10 : Eroare la apel> serverul a fost deconectat\n";
                myfile.close();
                return 0;
            }
            cout << tempRecv;
            myfile << tempRecv;
        }
        else {
            cout << "-10 : Eroare la apel> Comanda necunoscuta! Va rugam tastati din nou!\n";
            myfile << "-10 : Eroare la apel> Comanda necunoscuta! Va rugam tastati din nou!\n";
        }
    }
    myfile.close();


    return 0;
}