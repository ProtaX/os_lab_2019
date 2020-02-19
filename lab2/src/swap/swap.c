#include "swap.h"

void Swap(char *left, char *right)
{
	char buf = *left;
	*left = *right;
	*right = buf;
}
