//
// Created by cristi on 19.04.2017.
//
#include <string>

#ifndef UNTITLED_USER_H
#define UNTITLED_USER_H


class user {
public:
    std::string firstName;
    std::string secondName;
    std::string password;
    int pin;
    int nrCard;
    bool logged = false;
    double sold;
    bool block = false;
    int nrSocket = -999;

public:
    user() {

    }

    user(std::string numee, std::string prenumee, std::string parolaa, int pinn, int nrCardd, bool loggedd,
         double soldd) {
        firstName = numee;
        secondName = prenumee;
        password = parolaa;
        pin = pinn;
        nrCard = nrCardd;
        logged = loggedd;
        sold = soldd;
    }
};


#endif //UNTITLED_USER_H
