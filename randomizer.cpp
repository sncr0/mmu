#include "randomizer.h"
#include <fstream>
#include <iostream>

Randomizer::Randomizer(const std::string& rfile) {
    std::ifstream file(rfile);
    std::string line;

    std::getline(file, line);
    maxofs = std::stoi(line);

    randvals = new int[maxofs];

    for (int i = 0; i < maxofs; i++) {
        std::getline(file, line);
        randvals[i] = std::stoi(line);
    }

    ofs = 0;
}

Randomizer::~Randomizer() {
    // Deallocate memory
    //delete[] randvals;
}

int Randomizer::myrandom(int burst) {
    // Ensure ofs wraps around
    int randomNumber = (randvals[ofs] % burst);
    randomNumber = randomNumber++;
    ofs = (ofs + 1) % maxofs;

    // Perform the random calculation
    return randomNumber;
}

