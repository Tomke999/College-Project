#include "tistdtypes.h"
#include "gen_sinus.h"
#include "sine_table.h"
#include "math.h"

#define PI 3.14159265


void gen_sinus_table(Int16 n, float a, float f, Int16 ph, float buffer[])
{
    Int16 i = 0;
    Int16 delta = f * SINE_TABLE_SIZE;//Proizvod normalizovane frekvencije i velicine look-up tabele
    Int16 mask = (SINE_TABLE_SIZE-1);//Maskiranje za ogranicavanje indeksa unutar opsega look-up tabele
    Int16 k = (Int16)(((Int32)ph*delta) & mask); //Izracunavanje pocetnog indeksa na osnovu faznog pomeraja ph

    for (i = 0; i < n; i++)
    {
        k = k & mask;
		if (i <= SINE_TABLE_SIZE/4) //Prvi kvadrant 0 do pi/2
		{
			buffer[i] = a*p_sine_table[k];
			k+=delta;
		}
		else if(k <= SINE_TABLE_SIZE/2) //Drugi kvadrant pi/2 do pi
		{
			buffer[i] = a*p_sine_table[SINE_TABLE_SIZE/2 - k];//konverzija u prvi kvadrant
			k+=delta;
		}
		else if (k <= 3*SINE_TABLE_SIZE/4) //Treći kvadrant pi do 3*pi/4
		{
			buffer[i] = -a*p_sine_table[k - SINE_TABLE_SIZE/2];//konverzija u prvi kvadrant
			k+=delta;
		}
		else //Četvrti kvadrant 3*pi/4 do 2*pi
		{
			buffer[i] = -a*p_sine_table[SINE_TABLE_SIZE - k];//konverzija u prvi kvadrant
			k+=delta;
		}
    }
}
