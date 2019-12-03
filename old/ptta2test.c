#include "ptta2.hpp"
#include <ctime>
#include <iostream>
#include <cmath>
using namespace std;

int main()
{
    srand(time(NULL));

    Ptta2 teste(rand()%8+1);

    for (int i = 0; i < 24; i++) {
        cout<<" "<<teste.syncpattern[i];
    }
    
    cout<<"\nusermsglength: "<<teste.get_usermsglength()<<endl;
    return 0;
}
