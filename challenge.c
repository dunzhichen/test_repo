#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <complex.h>

#define CYCLE 10
#define DATA_LENGTH 20
#define PI 3.14159265358979323846f

typedef float complex cplx;

float abc_to_q(float Va, float Vb, float Vc, float theta) {
    return - (2.0f / 3.0f) * (Va * sinf(theta) + Vb * sinf(theta - 2 * PI / 3) + Vc * sinf(theta + 2 * PI / 3));
}

float Va[] = {
          156.63,  246.59,  294.72,  305.51,  300.66,
          268.03,  204.18,  125.41,  42.954, -48.322,
         -154.08, -243.95, -293.12, -303.09, -297.98,
         -264.13, -202.1,  -122.25, -39.893,  51.818
};

float Vb[] = {
          -308.4, - 280.19, -240.66, -186.6,  -99.744,
          -0.54547, 92.853,  181.46,  262.05,  312.39,
           311.44,  283.76,  245.04,  188.62,  102.16,
           2.9662, -89.395, -176.17, -259.16, -309.96
};

float Vc[] = {
           156.11,  82.694, -21.783, -128.37, -213.06,
          -269.49, -309.58, -313.4,  -273.73, -214.81,
          -154.29, -79.64,   24.679,  132.16,  216.63,
           274.14,  311.11,  315.76,  276.27,  216.22
};

float harmonics[5]; // 1st to 5th harmonics, only calculated for phase A

typedef struct _DDATA{
    float *in_a;
    float *in_b;
    float *in_c;
    float F_est;     // [rad/s]
    float Theta_est; // [rad]
    float *Harmonics;
    float Ts;
    float Kp;        // Kp for PLL
    float Ki;        // Ki for PLL
}DDATA;

DDATA ddata = {
.in_a = Va,   
.in_b = Vb,
.in_c = Vc,
.Ts   = 0.001f,
.Kp   = 1.0f,
.Ki   = 100.0f,
.Harmonics = harmonics,
};

void estimateFrequencyAndTheta(DDATA *d, int dataSize, int idx)
{  
        // The q-axis voltage on the estimated reference frame is the error for the three-phase PLL, use a PI controller to drive the error to zero. 
        // Output of the PI controller is the estimated frequency, and integration of that frequency gives estimated angle for the estimated reference frame.

        // calculate q-axis voltage on estimated dq reference frame. The q-axis voltage is the error for the PLL.
        float error = abc_to_q(d->in_a[idx%DATA_LENGTH], d->in_b[idx%DATA_LENGTH], d->in_c[idx%DATA_LENGTH], d->Theta_est);

        static float integrator = 0.0f;
        integrator      += error * d->Ki * d->Ts;
        d->F_est         = 2 * PI * 50.0f + d->Kp * error + integrator; // 50Hz assumed
        d->Theta_est    += d->F_est * d->Ts;
        
        // angle in [0 2*PI]
        if (d->Theta_est > 2*PI)
        {
            d->Theta_est -= 2*PI;
        }

        if (d->Theta_est < 0)
        {
            d->Theta_est += 2*PI;
        }
}

void getHarmonicAmplitudes(DDATA *d, int dataSize) 
{
	// frequency basis for DFT
	int base_bin_frequency     = (int)(1/d->Ts/(float)(dataSize) + 0.5);
	
	// round the fundamental frequency to integer 
	int fundamental_frequency  = (int)(d->F_est/2/PI + 0.5);
	
	// In a full DFT, we calculate the N frequencies. the output of DFT corresponds to the frequencies range 0 : fs/N : (fs - fs/N)
	// In total, that is N DFT data bin. We can find the data bins for specific frequencies to reduce the amount of calculation
	
    int index[5]; 
	// data bin index for the 1st to 5th harmonics in the DFT result      
	for (int i = 0; i < 5; i++)
	{   
        // requires the fundamental frequency/base frequency = integer!
		index[i] = fundamental_frequency * (i + 1) / base_bin_frequency + 1;
	}

	// partial DFT for specific frequencies, i.e., 1st to 5th harmonics
	for (int k = 0; k < 5; k++)
		{
			// placeholder for result of one frequency
			cplx DFT_result = 0;

			int  i = index[k];
		 		for (int j = 0; j < dataSize; j++)
				{   
					DFT_result += cexp(-I*2*PI*(i-1)*j/(float)dataSize) * d->in_a[j%DATA_LENGTH];
				}	
			// save result
			d->Harmonics[k] = 2.0/(float)dataSize * sqrtf(crealf(DFT_result)*crealf(DFT_result) + cimagf(DFT_result)*cimagf(DFT_result));
		}
}

int main()
{
  
    for (int i = 0; i < DATA_LENGTH * CYCLE; i++)
    {   
        estimateFrequencyAndTheta(&ddata, DATA_LENGTH*CYCLE, i);
        //printf("Estimated frequency: %.2f[Hz], angle: %.2f[deg]\n",ddata.F_est/2/PI,ddata.Theta_est*180/PI);		
    }

    // Fundamental frequency of the / DFT base frequency => integer is required.
    // f_fundamental / (fs / (DATA_LENGTH * CYCLE))   
    getHarmonicAmplitudes(&ddata, DATA_LENGTH * CYCLE);
	/*for (int i=0;i<5;i++)
	{
		printf("Harmonics: %d, magnitude: %.3f[V]\n", i+1, ddata.Harmonics[i]);
	}*/
    
    return 0;
}