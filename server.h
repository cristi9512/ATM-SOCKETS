//
// Created by cristi on 19.04.2017.
//

#ifndef UNTITLED_SERVER_H
#define UNTITLED_SERVER_H


#include <vector>
#include "user.h"

class server {
public:

    static std::vector<user> usersList;
    static std::vector<user> userLogat;


    int main(int argc, char *argv[]);
};


#endif //UNTITLED_SERVER_H
