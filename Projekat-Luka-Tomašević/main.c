//////////////////////////////////////////////////////////////////////////////
// *
// *
// *    Projektni zadatak iz predmeta: Projektovanje namenskih računarskih struktura u obradi signala
// *
// *
// *    Tema projekta: "REALIZACIJA SISTEMA ZA SINTEZU ZVUČNOG SIGNALA"
// *
// *
// *    Student: Luka Tomašević EE 189/2018
// *
// *
// *    Mentor: Miloš Pilipović
// *
// *
//////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include "fur_elise.h"
#include "ezdsp5535.h"
#include "ezdsp5535_aic3204_dma.h"
#include "ezdsp5535_i2s.h"
#include "ezdsp5535_i2c.h"
#include "gen_sinus.h"
#include "sine_table.h"
#include "aic3204.h"
#include "math.h"


#define BLOCK_SIZE 128
#define SAMPLE_RATE 48000 //frekvencija odabiranja
#define NUM_SAMPLES (SAMPLE_RATE*furEliseLength)
#define GENERATOR_NUM 5

static WAV_HEADER outputWAVhdr;
static WAV_HEADER inputWAVhdr;


#pragma DATA_ALIGN(clip_buffer, 4)
Int16 clip_buffer[BLOCK_SIZE];
Int16 clip_buffer2[BLOCK_SIZE];

float buffer[BLOCK_SIZE];
Int16 quant_buffer[BLOCK_SIZE];
float tone[BLOCK_SIZE];
float noise[BLOCK_SIZE];

Int16 quantB(float input, Uint16 B)
{
	Int16 Q = (1L << (B - 1));
	float output_float = floor(input * Q + 0.5);
		if(output_float == Q)
			output_float = Q-1;
		Int16 output_int = output_float;
	return output_int;
}

Int16 clipB(Int16 input, Uint16 B)
{
	Int16 max = (1L << (B-1)) - 1;
	Int16 min = - (1L << (B-1));
	Int16 output;

	if(input > max)
		output = max;
	else if (input < min)
		output = min;
	else
		output = input;

	return output;
}

float calculate_snr(float* signal, float* noise, Uint16 n)
{
	int i = 0;
	float Ps = 0, Pe = 0;
	for (i = 0; i < n; i++)
	{
		Ps += (signal[i] * signal[i]) / n;
		Pe += (noise[i] * noise[i]) / n;
	}
	return 10 * log10(Ps/Pe);
}

void DAHDSR(float buffer[], Int16 n, Int16 ph, Int16 tone_duration)
{
	Int16 i;
	Int16 DELAY_DURATION = 20; // Dužina delay faze u odbircima
	Int16 ATTACK_DURATION = 120; // Dužina attack faze u odbircima
	Int16 HOLD_DURATION = 70; // Dužina hold faze u odbircima
	Int16 DECAY_DURATION = 175; // Dužina decay faze u odbircima
	Int16 SUSTAIN_LEVEL = 0.5; // Nivo amplitude u sustain fazi
	Int16 RELEASE_DURATION = 300; // Dužina release faze u odbircima

	for (i = ph; i < ph + tone_duration && i < n; i++)
	{
		if (i < ph + DELAY_DURATION)
		{
			// Delay faza
			buffer[i] = 0.0;
		}
        else if (i < ph + DELAY_DURATION + ATTACK_DURATION)
        {
        	// Attack faza
        	float progress = (i - ph - DELAY_DURATION) / (float)ATTACK_DURATION;
        	buffer[i] = progress;
        }
        else if (i < ph + DELAY_DURATION + ATTACK_DURATION + HOLD_DURATION)
        {
        	// Hold faza
        	buffer[i] = 1.0;
        }
        else if (i < ph + DELAY_DURATION + ATTACK_DURATION + HOLD_DURATION + DECAY_DURATION)
        {
        	// Decay faza
        	float progress = (i - ph - DELAY_DURATION - ATTACK_DURATION - HOLD_DURATION) / (float)DECAY_DURATION;
        	float amplitude = 1.0 - (1.0 - SUSTAIN_LEVEL) * progress;
        	buffer[i] = (Int16)amplitude;
        }
        else if (i < ph + DELAY_DURATION + ATTACK_DURATION + HOLD_DURATION + DECAY_DURATION + RELEASE_DURATION)
        {
        	// Sustain faza
        	buffer[i] = SUSTAIN_LEVEL;
        }
        else
        {
        	// Release faza
        	float progress = (i - ph - DELAY_DURATION - ATTACK_DURATION - HOLD_DURATION - DECAY_DURATION - RELEASE_DURATION) / (float)RELEASE_DURATION;
        	buffer[i] = SUSTAIN_LEVEL * (1.0 - progress);
        }
	}
}


/*
 *
 *  main( )
 *
 */
void main( void )
{
	Int16 i, j;

	/* Inicijalizaija razvojne ploce */
	EZDSP5535_init( );


	 /* Initialise hardware interface and I2C for code */
	aic3204_hardware_init();

	aic3204_set_input_filename("../signal1.wav");
	aic3204_set_output_filename("../fur_eliseOutput1.wav");

	/* Initialise the AIC3204 codec */
	aic3204_init();

	aic3204_dma_init();

	/* Citanje zaglavlja ulazne datoteke */
	aic3204_read_wav_header(&inputWAVhdr);

	/* Popunjavanje zaglavlja izlazne datoteke */
	outputWAVhdr = inputWAVhdr;


	/* Zapis zaglavlja izlazne datoteke */
	aic3204_write_wav_header(&outputWAVhdr);

    tone_t* trenutni_ton[GENERATOR_NUM] = {furEliseNotes0, furEliseNotes1, furEliseNotes2,furEliseNotes3, furEliseNotes4 }; // generisanje kanala

		for(i=0; i<furEliseLength; i++) // petlja koja iterira od 0 do duzine notnog zapisa
		{
			int k;
			for(k = 0; k < BLOCK_SIZE; k++)
				{
						clip_buffer2[k] = 0;
				}

			for(j = 0; j < GENERATOR_NUM; j++) // za svaki od 5 kanala
			{
				// provera da li da odsvira ton, ulazi u blok ako treba
				if( trenutni_ton[j]->time <= i)
				{
					float frequency = note_to_freq(trenutni_ton[j]->note)/SAMPLE_RATE; // normalizovana frekv tj f/fs
					Int16 phase = (i-trenutni_ton[j]->time) * BLOCK_SIZE; // fazni pomeraj
					gen_sinus_table(BLOCK_SIZE, 1.0, frequency, phase, buffer);
					DAHDSR(buffer, BLOCK_SIZE, phase, trenutni_ton[j]->duration);//propuštanje kroz dahdsr
					for(k = 0; k < BLOCK_SIZE; k++)
					{
						tone[k] = buffer[k];
					}
					for(k = 0; k < BLOCK_SIZE; k++)
					{
						quant_buffer[k] = quantB(buffer[k], 15); // kvantizovati sa 15 bita
						clip_buffer[k] = clipB(quant_buffer[k], 14); // pa klipovanje sa 14
						clip_buffer2[k] += clip_buffer[k]; // hocu u 2 da sacuvam
						noise[k] = (clip_buffer[k] - tone[k]);
					}
					float SNR = calculate_snr(tone, noise, BLOCK_SIZE);
					printf("***Mera za kvalitet kvantizacije SNR iznosi: %f za ton %s cija je frekvencija %f Hz.*** \n", SNR, note_to_string(trenutni_ton[j]->note), frequency*SAMPLE_RATE);

					// ako je ton zavrsen predji na sledeci
					if((i - trenutni_ton[j]->time) >= (trenutni_ton[j]->duration - 1))
					{
						trenutni_ton[j]++;
					}
				}
			}

		// zapis
		aic3204_write_block(clip_buffer2, clip_buffer2);
		}


	/* Disable I2S and put codec into reset */
    aic3204_disable();

    printf( "\n***Program has Terminated***\n" );
	SW_BREAKPOINT;
}




