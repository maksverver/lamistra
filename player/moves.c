/* Generates the move table for the default 7x7 board. */

#include <stdio.h>

int main()
{
    int r, c, f;
    for(r = 0; r < 7; ++r)
    {
        for(c = 0; c < 7; ++c)
        {
            f = 7*r + c;
            printf("{");
            if(f != 22 && f != 26)
            {
                if(r > 0 && f - 7 != 22 && f - 7 != 26)
                    printf("%2d,", f - 7);
                if(r < 6 && f + 7 != 22 && f + 7 != 26)
                    printf("%2d,", f + 7);
                if(c > 0 && f - 1 != 22 && f - 1 != 26)
                    printf("%2d,", f - 1);
                if(c < 6 && f + 1 != 22 && f + 1 != 26)
                    printf("%2d,", f + 1);
            }
            
            printf("-1},\n");
        }
    }
}
