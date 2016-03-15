#include <iostream>
#include <ctime>
 
using namespace std;
 
int main()
{
  time_t current;
 
  current = time(0);
 
  if (current != static_cast<time_t>(-1))
  {
    cout<< ctime(&current) <<flush;
  }
}
