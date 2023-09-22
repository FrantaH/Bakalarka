/*author: František Horázný
 *year: 2020
 *contact: fhorazny@gmail.com*/

#include <stdio.h>
#include <signal.h>
#include <alsa/asoundlib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>


/* SHARC HW capabilities */
#define DSP_NUM_CHANNELS	5
#define DSP_SAMPLE_SIZE		4
#define DSP_PCM_FORMAT		SND_PCM_FORMAT_FLOAT_LE
#define DSP_PERIOD_LEN		4096
#define DSP_FRAME_SIZE		(DSP_NUM_CHANNELS * DSP_SAMPLE_SIZE)
#define DSP_RATE		48000
#define DSP_BUFFER_SIZE 8



#define PRINT 0					// 1 print everything, 0 print nothing
#define SAMPLE_LEN 6144			// dont change on card - was set to max, where card is fast enough to compute....
#define SAMPLE_LEN_half 3072	// dont change on card ^
#define KV_E_LIMIT 2.5			
#define FREQUENCY 48000.0



#define MAX_X 3		// meters from middle mic to the left
#define MIN_X -3	// meters from middle mic to the right
#define MAX_Z 6		// how far could source of sound be from middle mic
#define MAX_Y 2		// 2 == 3 meters above ground
#define MIN_Y -1	// -1 is ground in my case


float array_audio1[SAMPLE_LEN];
float array_audio2[SAMPLE_LEN];
float array_audio3[SAMPLE_LEN];
float array_audio4[SAMPLE_LEN];
float array_audio5[SAMPLE_LEN];

int done;
int B_counter;

void sighandler(int signum)
{
	printf("Caught SIGINT, exiting\n");
	printf("Counter: %d \n",B_counter);
	done = 1;
}

void sighandler_reset(int signum)
{
	printf("Caught SIGQUIT, reseting counter\n");
	printf("Counter: %d \n",B_counter);
	B_counter = 0;
}

int energyCompute(double* energie1,double* energie2,double* energie3,double* energie4,double* energy_all)
{
	static double min_energy = 999999.0;
	
	*energie1 = 0;
	*energie2 = 0;
	*energie3 = 0;
	*energie4 = 0;
	
	for(int j = 0;j<SAMPLE_LEN-1300;j+=6)
	{
		*energie1 += array_audio1[j]*array_audio1[j];
		*energie2 += array_audio2[j]*array_audio2[j];
		*energie3 += array_audio3[j]*array_audio3[j];
		*energie4 += array_audio4[j]*array_audio4[j];
	}
	*energy_all = (*energie1) + (*energie2) + (*energie3) + (*energie4);
	// *energy_all = (*energy_all)/(SAMPLE_LEN-1300);
	
	if((*energy_all)<min_energy)
		min_energy=(*energy_all);
	else
		min_energy = min_energy*1.1;
	
	if((*energy_all)>min_energy*KV_E_LIMIT)		// energy is big enough (signal isnt just noise
		return 1;
	
	return 0;
}

int normCrossCorel(double norm_var1, double norm_var2, double norm_var3, double norm_var4, double statistics[5][279])
{
	double temp_result1 = 0.0;
	double temp_result2 = 0.0;
	double temp_result3 = 0.0;
	double temp_result4 = 0.0;
	
	for(int i = 0; i < 279 ; i++)
	{
		double norm_var5 = 0.0;
		temp_result1 = 0.0;
		temp_result2 = 0.0;
		temp_result3 = 0.0;
		temp_result4 = 0.0;

		for(int j = 0;j<SAMPLE_LEN-1300;j+=6)	// provede sumu v posunu o i
		{	
			int jj = j+139;
			int ji = j+i;
			temp_result1 = temp_result1 + (array_audio1[jj] * array_audio5[ji]);
			temp_result2 = temp_result2 + (array_audio2[jj] * array_audio5[ji]);
			temp_result3 = temp_result3 + (array_audio3[jj] * array_audio5[ji]);
			temp_result4 = temp_result4 + (array_audio4[jj] * array_audio5[ji]);
			
			norm_var5 = norm_var5 + (array_audio5[ji] * array_audio5[ji]);	
		}
		
		temp_result1 = (temp_result1) / sqrt(norm_var1*norm_var5); 
		temp_result2 = (temp_result2) / sqrt(norm_var2*norm_var5);
		temp_result3 = (temp_result3) / sqrt(norm_var3*norm_var5);
		temp_result4 = (temp_result4) / sqrt(norm_var4*norm_var5);
		
		double NCC_alpha = 0.7;		// weight of old corelation values. 1-alpha = weight of new corelation values
		
		statistics[1][i]=(statistics[1][i]*NCC_alpha)+(temp_result1*(1-NCC_alpha));
		statistics[2][i]=(statistics[2][i]*NCC_alpha)+(temp_result2*(1-NCC_alpha));
		statistics[3][i]=(statistics[3][i]*NCC_alpha)+(temp_result3*(1-NCC_alpha));
		statistics[4][i]=(statistics[4][i]*NCC_alpha)+(temp_result4*(1-NCC_alpha));
	}
	return 0;
}

void getHighestIndexes(double statistics[5][279], double * maxlocation1, double * maxlocation2, double * maxlocation3, double * maxlocation4)
{
	
	double maxval1 = -100000000;
	double maxval2 = -100000000;
	double maxval3 = -100000000;
	double maxval4 = -100000000;

	static int history1[5];
	static int history2[5];
	static int history3[5];
	static int history4[5];
	static unsigned int h=0;
	
	/*for(int i=0;i<279;i++)		code without position filter
	{
		if(statistics[1][i]>maxval1)
		{
			maxval1=statistics[1][i];
			*maxlocation1 = i-139;
		}
		if(statistics[2][i]>maxval2)
		{
			maxval2=statistics[2][i];
			*maxlocation2 = i-139;
		}
		if(statistics[3][i]>maxval3)
		{
			maxval3=statistics[3][i];
			*maxlocation3 = i-139;
		}
		if(statistics[4][i]>maxval4)
		{
			maxval4=statistics[4][i];
			*maxlocation4 = i-139;
		}
	}*/
	
	for(int i=0;i<279;i++)
	{
		if(statistics[1][i]>maxval1)
		{
			maxval1=statistics[1][i];
			history1[h%5] = i-139;
		}
		if(statistics[2][i]>maxval2)
		{
			maxval2=statistics[2][i];
			history2[h%5] = i-139;
		}
		if(statistics[3][i]>maxval3)
		{
			maxval3=statistics[3][i];
			history3[h%5] = i-139;
		}
		if(statistics[4][i]>maxval4)
		{
			maxval4=statistics[4][i];
			history4[h%5] = i-139;
		}
	}

	double avg_shift1 = (history1[0]+history1[1]+history1[2]+history1[3])/5.0;
	double avg_shift2 = (history2[0]+history2[1]+history2[2]+history2[3])/5.0;
	double avg_shift3 = (history3[0]+history3[1]+history3[2]+history3[3])/5.0;
	double avg_shift4 = (history4[0]+history4[1]+history4[2]+history4[3])/5.0;

	if(h<5)				// delete this branch of "if" (6 lines) if you gonna use this app (this excludes first 5 entries so they wont be super-wrong (worse test results))
	{					// but it doesnt need to be here and consumes process time
		*maxlocation1 = history1[h%5];
		*maxlocation2 = history2[h%5];
		*maxlocation3 = history3[h%5];
		*maxlocation4 = history4[h%5];
	}else{
		double leastdif1 = 140;
		double leastdif2 = 140;
		double leastdif3 = 140;
		double leastdif4 = 140;
		
		for(int i = 0;i<5;i++)
		{
			if(leastdif1>fabs(history1[i]-avg_shift1)){
				leastdif1 = fabs(history1[i]-avg_shift1);
				*maxlocation1 = history1[i];
			}
			if(leastdif2>fabs(history2[i]-avg_shift2)){
				leastdif2 = fabs(history2[i]-avg_shift2);
				*maxlocation2 = history2[i];
			}
			if(leastdif3>fabs(history3[i]-avg_shift3)){
				leastdif3 = fabs(history3[i]-avg_shift3);
				*maxlocation3 = history3[i];
			}
			if(leastdif4>fabs(history4[i]-avg_shift4)){
				leastdif4 = fabs(history4[i]-avg_shift4);
				*maxlocation4 = history4[i];
			}
		}
	}
	h++;
}

double compCoorZ(double hor_x, double hor_y, double ver_x, double ver_y)
{
	
	double z1 = sqrt(fabs((ver_y*ver_y) - (hor_x*hor_x)));		// teoreticky by stačil jeden výpočet, pro přesnější a hladší výpočet zakomponuji oba výpočty
	double z2 = sqrt(fabs((hor_y*hor_y) - (ver_x*ver_x)));
	

	return (z1+z2)/2;
}

int computeCoor(double statistics[5][279],double *x, double *y, double *z)
{	

	double maxlocation1 = 333.0;
	double maxlocation2 = 333.0;
	double maxlocation3 = 333.0;
	double maxlocation4 = 333.0;
	
	// get best computed corelation
	getHighestIndexes(statistics, &maxlocation1, &maxlocation2, &maxlocation3, &maxlocation4);
	
	
	
	double hor_x = 0.0;			// horizontal coordinate
	double hor_y = 0.0;			// distance from horizontal axis
	
	double ver_x = 0.0;			// vertical coordinate
	double ver_y = 0.0;			// distance from vertical axis
	int swaped = 0;				// special case, where coordinates needs to be swaped to be computed (division by 0)
	
	
	// horizontal axis
	if(maxlocation4 == 0.0){
		maxlocation4=maxlocation2;
		maxlocation2=0;
		swaped = 1;
	}
		
	maxlocation2 = (maxlocation2*343)/FREQUENCY;
	maxlocation2 = (maxlocation2/2);
	maxlocation4 = (maxlocation4*343)/FREQUENCY;
	maxlocation4 = (maxlocation4/2);
	double pow_maxlocation2 = maxlocation2*maxlocation2;
	double pow_maxlocation4 = maxlocation4*maxlocation4;

	if(maxlocation2 == -maxlocation4)
		hor_x = 0;
	else
		hor_x = ((4*pow_maxlocation2*maxlocation4)-(4*maxlocation2*pow_maxlocation4) + maxlocation2 - maxlocation4)/(2*(maxlocation2+maxlocation4));
	
	hor_y = sqrt(fabs(((((hor_x-0.5)*(hor_x-0.5))/pow_maxlocation4)-1) * (0.25-pow_maxlocation4)));
	
	if(swaped){
		hor_x=-hor_x;
		swaped = 0;
	}
	
	// verical axis
	if(maxlocation3 == 0.0){
		maxlocation3=maxlocation1;
		maxlocation1=0;
		swaped = 1;
	}
	
	
	maxlocation1 = (maxlocation1*343)/FREQUENCY;
	maxlocation1 = (maxlocation1/2);
	maxlocation3 = (maxlocation3*343)/FREQUENCY;
	maxlocation3 = (maxlocation3/2);
	double pow_maxlocation1 = maxlocation1*maxlocation1;
	double pow_maxlocation3 = maxlocation3*maxlocation3;

	if(maxlocation1 == -maxlocation3)
		ver_x = 0;
	else
		ver_x = ((4*pow_maxlocation1*maxlocation3)-(4*maxlocation1*pow_maxlocation3) + maxlocation1 - maxlocation3)/(2*(maxlocation1+maxlocation3));
	
	ver_y = sqrt(fabs(((((ver_x-0.5)*(ver_x-0.5))/pow_maxlocation3)-1) * (0.25-pow_maxlocation3)));

	if(swaped)
		ver_x=-ver_x;
	

	if(ver_x<MAX_Y && ver_x>-1 && hor_x<MAX_X && hor_x>MIN_X && hor_y<MAX_Z && ver_y<MAX_Z){
		*z = compCoorZ(hor_x,hor_y,ver_x,ver_y);
		*x = hor_x;
		*y = ver_x;
	}
	
	if(PRINT)
		printf("posun: %3.2lf, %3.2lf, %3.2lf, %3.2lf  ", (maxlocation1*2*FREQUENCY)/343, (maxlocation2*2*FREQUENCY)/343, (maxlocation3*2*FREQUENCY)/343, (maxlocation4*2*FREQUENCY)/343);

	return 0;
}

int main(int argc, char *argv[])
{
	
	// *** OPEN OUTPUT FILE ***
	FILE *out_test;
	if ((out_test = fopen("out_test.txt", "wb")) == NULL) {
		fprintf(stderr, "fopen\n");
		return 1;
	}
	
	// *** SETUP PCM ***
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *pcm_hw;
	int err;	

	B_counter=0;
	done = 0;
	signal(SIGINT, sighandler);
	signal(SIGQUIT, sighandler_reset);

	/* Open, blocking */
	/* arecored -L */
	if ((err = snd_pcm_open(&pcm_handle, "hw:SHARC", SND_PCM_STREAM_CAPTURE, 0)) != 0) {
		fprintf(stderr, "open: %s\n", snd_strerror(err));
		return 1;
	}

	/* Set params */
	if ((err = snd_pcm_hw_params_malloc(&pcm_hw)) != 0) {
		fprintf(stderr, "malloc: %s\n", snd_strerror(err));
		return 1;
	}
	if ((err = snd_pcm_hw_params_any(pcm_handle, pcm_hw)) != 0) {
		fprintf(stderr, "any: %s\n", snd_strerror(err));
		return 1;
	}
	if ((err = snd_pcm_hw_params_set_channels(pcm_handle, pcm_hw, DSP_NUM_CHANNELS)) != 0) {
		fprintf(stderr, "channels: %s\n", snd_strerror(err));
		return 1;
	}
	int dir = 0;
	unsigned int rate = DSP_RATE;
	if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle, pcm_hw, &rate, &dir)) != 0) {
		fprintf(stderr, "rate %s\n", snd_strerror(err));
		return 1;
	}
	if ((err = snd_pcm_hw_params_set_access(pcm_handle, pcm_hw, SND_PCM_ACCESS_RW_INTERLEAVED)) != 0) {
		fprintf(stderr, "access: %s\n", snd_strerror(err));
		return 1;
	}
	if ((err = snd_pcm_hw_params_set_format(pcm_handle, pcm_hw, DSP_PCM_FORMAT)) != 0) {
		fprintf(stderr, "format: %s\n", snd_strerror(err));
		return 1;
	}
	snd_pcm_uframes_t frames_count = DSP_PERIOD_LEN;
	if ((err = snd_pcm_hw_params_set_period_size_near(pcm_handle, pcm_hw, &frames_count, &dir)) != 0) {                                                                                                                                                                                    
		fprintf(stderr, "period: %s\n", snd_strerror(err));
		return 1;
	} 	
	printf("period size %d, ", frames_count);
	
	unsigned int periods = DSP_BUFFER_SIZE;
	dir = 0;
	if ((err = snd_pcm_hw_params_set_periods_near(pcm_handle, pcm_hw, &periods, &dir)) < 0) {                                                                                                                                                                                    
		fprintf(stderr, "periods: %s\n", snd_strerror(err));
		return 1;
	}
	frames_count = (DSP_PERIOD_LEN * periods);
	if (snd_pcm_hw_params_set_buffer_size_near(pcm_handle, pcm_hw, &frames_count) < 0) {
      fprintf(stderr, "Error setting buffersize.\n");
      return(-1);
    }
	printf("periods %d, buffer size: %d, rate: %d\n", periods, frames_count, rate);
	
	if ((err = snd_pcm_hw_params(pcm_handle, pcm_hw)) != 0) {
		fprintf(stderr, "hw: %s\n", snd_strerror(err));
		return 1;
	} 
	
	snd_pcm_hw_params_free(pcm_hw);



	char buf[DSP_FRAME_SIZE * frames_count];	

	// *** SETUP ALGORITM VARIABLES ***
	memset(array_audio1, 0, frames_count*sizeof(float));
	memset(array_audio2, 0, frames_count*sizeof(float));
	memset(array_audio3, 0, frames_count*sizeof(float));
	memset(array_audio4, 0, frames_count*sizeof(float));
	memset(array_audio5, 0, frames_count*sizeof(float));

	double statistics[5][279];		// 5 for better orientation -> audio4 statistics[4] array_audio4 | statistics[0] not used

	memset(statistics[1], 0, 279*sizeof(double));
	memset(statistics[2], 0, 279*sizeof(double));
	memset(statistics[3], 0, 279*sizeof(double));
	memset(statistics[4], 0, 279*sizeof(double));

	double energy_all = 0.0;
	double energie1 = 0.0;
	double energie2 = 0.0;
	double energie3 = 0.0;
	double energie4 = 0.0;

	double x,y,z;


	// *** MAIN LOOP ***
	while (!done) {
		B_counter++;
		int ret;
		
		// variables for counting process time
		// clock_t start, end, after_energy, norm;
		// double cpu_time_used_energy, cpu_time_used_end, cpu_time_used_norm;
     
		// start = clock();

		// read buffer
		ret = snd_pcm_readi(pcm_handle, buf, frames_count);
		if (ret == -EPIPE || ret == -EINTR || ret == -ESTRPIPE) {
			printf("recovering %s\n\n",snd_strerror(ret));
			snd_pcm_recover(pcm_handle, ret, 1);
		}
		else if (ret == -EIO || ret<0){
			printf("error %s\n",snd_strerror(ret));
			break;
		}
		
		// fill working arrays
		for(int i=0;i<frames_count;i++)
		{
			array_audio1[i] = *((float *)(&buf[(20*i)]));
			array_audio2[i] = *((float *)(&buf[(20*i)+4]));
			array_audio3[i] = *((float *)(&buf[(20*i)+8]));
			array_audio4[i] = *((float *)(&buf[(20*i)+12]));
			array_audio5[i] = *((float *)(&buf[(20*i)+16]));
		}


		// compute energy (detect speech)
		if(energyCompute(&energie1,&energie2,&energie3,&energie4,&energy_all))
		{
			// clock_t start, end;
			// double cpu_time_used;
     
			// start = clock();
			// after_energy = clock();
			
			// compute corelation
			normCrossCorel(energie1, energie2, energie3, energie4, statistics);
			if(PRINT)
				printf("...");  //just to see in debug, that speech has been detected
			
			// end = clock();
			// cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
			// printf("cpu time norm cross %6.3lf  \n",cpu_time_used);
			computeCoor(statistics,&x,&y,&z);

		}
		// norm = clock();

		if(PRINT)
			printf("%4ld: cas:%5.2f, energie: %6.3lf  ",B_counter,((B_counter)*SAMPLE_LEN)/FREQUENCY,energy_all);
			
		// compute coordinates	
		// computeCoor(statistics,&x,&y,&z);


		if(PRINT == 0)
		{
			printf("\r  [ % 6.3lf  % 6.3lf  % 6.3lf ]",x,y,z);
			fflush(stdout);
		} else
			printf("  [ % 6.3lf  % 6.3lf  % 6.3lf ]\n",x,y,z);
		
		fprintf( out_test,"%lf %lf %lf\n",x,y,z);

		// end = clock();
		// cpu_time_used_energy = ((double) (after_energy - start)) / CLOCKS_PER_SEC;
		// cpu_time_used_norm = ((double) (norm - start)) / CLOCKS_PER_SEC;
		// cpu_time_used_end = ((double) (end - start)) / CLOCKS_PER_SEC;

		// printf("cpu time after energy %6.3lf, after corel %6.3lf, end %6.3lf  \n",cpu_time_used_energy*1000, cpu_time_used_norm*1000, cpu_time_used_end*1000);

	}

	if ((err = snd_pcm_close(pcm_handle)) != 0) {
		fprintf(stderr, "close: %s\n", snd_strerror(err));
		return 1;
	}

	fclose(out_test);

	return 0;
}