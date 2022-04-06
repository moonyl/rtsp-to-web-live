//
// Created by moony on 2022-03-30.
//

extern "C" {
#include "remuxing.h"
}

#include <cstdio>
#include "Remux.h"
#include <iostream>

int main(int argc, char* argv[])    {
    if (argc < 3)
    {
        printf("usage: %s input output\n"
               "API example program to remux a media file with libavformat and libavcodec.\n"
               "The output format is guessed according to the file extension.\n"
               "\n",
               argv[0]);
        return 1;
    }

    try {
        Remux remux{argv[1], argv[2]};
        remux.run();
    } catch(std::exception& e) {
        std::cerr << e.what();
        return 1;
    }

    return 0;
}