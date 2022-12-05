#include <algorithm>
#include <iostream>
#include <cstring>
#include <cstdio>
using namespace std;



template < typename T> 
inline T read()
{
	T x = 0, w = 1; char c = getchar();
	while(c < '0' || c > '9') { if(c == '-') w = -1; c = getchar(); }
	while(c >= '0' && c <= '9') x = x * 10 + c - '0', c = getchar();
	return x * w;
}

int main()
{

	return 0;
}
