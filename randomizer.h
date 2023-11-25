#ifndef RANDOMIZER_H
#define RANDOMIZER_H

#include <string>
#include <utility>

class Randomizer {
public:
    Randomizer(const std::string& rfile);
    ~Randomizer();

    int myrandom(int burst);

private:
    int* randvals;
    int maxofs;
    int ofs;
};

#endif // RANDOMIZER_H

