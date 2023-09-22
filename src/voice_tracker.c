/*author: František Horázný
 *year: 2020
 *contact: fhorazny@gmail.com*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <complex.h>

/*FFTW was written by Matteo Frigo and Steven G. Johnson.  You can
contact them at fftw@fftw.org.  The latest version of FFTW,
benchmarks, links, and other information can be found at the FFTW home
page (http://www.fftw.org).  You can also sign up to the fftw-announce
Google group to receive (infrequent) updates and information about new
releases.*/
#include <fftw3.h>

#define SSPEED 343				// speed of sound
#define PRINT 1					// 1 print everything, 0 print only coordinates
#define SAMPLE_LEN 6144			// změna délky jednoho kusu (1200 = 25 ms) (4800 = 0.1 sec) (24000 = 0.5 sec) 12000   6144 na zvukovce
#define SAMPLE_LEN_half 3072	// 3072 na zvukovce
#define COMPUTATION_TYPE 1		// 1 == normalized cross corelation, 2 == gccPHATCrossCorel (doesnt work, dont know why), 3 == corelation with subtraction
#define FILTER_TYPE 0			// 0 == no filter, 1 == LPF , 2 == BPF 
#define KV_E_LIMIT 2.5			// 2.5 = lowest to detect quiet speech from more than 2 meters. Louder speech is detected without problem with 3
#define FREQUENCY 48000.0


// for better results cut impossible positions (comment condition in computeCoor function to disable this position filter (cca line 589))
#define MAX_X 3		// meters from middle mic to the left
#define MIN_X -3	// meters from middle mic to the right
#define MAX_Z 10	// MAX distance from middle mic
#define MAX_Y 2		// 2 == 3 meters above ground
#define MIN_Y -1	// -1 is ground if cross is on the floor



// files with audio channels
FILE *audio1;
FILE *audio2;
FILE *audio3;
FILE *audio4;
FILE *audio5;

// files for test outputs and positions outputs
FILE *out_test;
FILE *out_raw;

// local array with window of raw data
float array_audio1[SAMPLE_LEN];
float array_audio2[SAMPLE_LEN];
float array_audio3[SAMPLE_LEN];
float array_audio4[SAMPLE_LEN];
float larray_audio5[SAMPLE_LEN+278];

// local array with window of filtered data
float filt_array_audio1[SAMPLE_LEN];
float filt_array_audio2[SAMPLE_LEN];
float filt_array_audio3[SAMPLE_LEN];
float filt_array_audio4[SAMPLE_LEN];
float filt_larray_audio5[SAMPLE_LEN+278];

//closes all files
void closeFiles()
{
	fclose(out_test);
	fclose(out_raw);
	fclose(audio1);
	fclose(audio2);
	fclose(audio3);
	fclose(audio4);
	fclose(audio5);
	printf("delka okna: %d\n", SAMPLE_LEN);
}

//opens all files, ends if error occurs
int openFiles(char* argv[])
{
	audio5 = fopen(argv[1], "r");
	audio1 = fopen(argv[2], "r");
	audio2 = fopen(argv[3], "r");
	audio3 = fopen(argv[4], "r");
	audio4 = fopen(argv[5], "r");
	
	out_test = fopen("./out_test.txt", "w");
	out_raw = fopen("./out_test.raw", "w");

	if(audio4 == NULL || audio5 == NULL || audio2 == NULL || audio1 == NULL || audio3 == NULL || out_test == NULL || out_raw == NULL )
	{
		perror("nepodařilo se otevřit soubory");
		closeFiles();
		return 1;
	}
	return 0;
}

// load 139 values into longer array for the first time so larray[139] corespodns to array[0]
int firstLoadOfArray()
{
	if (fread(&(larray_audio5[139+SAMPLE_LEN_half]),4,139,audio5)<139)
	{
		printf("chyba\n");
		closeFiles();
		return 1;
	}
	return 0;
}

// loads half of arrays into raw arrays (use twice to load SAMPLE_LEN)
// used to overlap windows
int loadHalfOfArray()
{
	memcpy(&(larray_audio5[0]),&(larray_audio5[SAMPLE_LEN_half]),(278+SAMPLE_LEN_half)*sizeof(float));
	if (fread(&(larray_audio5[278+SAMPLE_LEN_half]),4,SAMPLE_LEN_half,audio5)<SAMPLE_LEN_half)
	{			
		closeFiles();
		return 1;
	}
	
	// tady se nejdřív posune pole o polovinu do leva a pak se načte polovina SAMPLE_LEN
	// if (fread(array_audio1,4,SAMPLE_LEN,audio1)<SAMPLE_LEN)
	memcpy(&(array_audio1[0]),&(array_audio1[SAMPLE_LEN_half]),(SAMPLE_LEN_half)*sizeof(float));
	if (fread(&(array_audio1[SAMPLE_LEN_half]),4,SAMPLE_LEN_half,audio1)<SAMPLE_LEN_half)
	{
		closeFiles();
		return 1;
	}

	// tady se nejdřív posune pole o polovinu do leva a pak se načte polovina SAMPLE_LEN
	// if (fread(array_audio2,4,SAMPLE_LEN,audio2)<SAMPLE_LEN)
	memcpy(&(array_audio2[0]),&(array_audio2[SAMPLE_LEN_half]),(SAMPLE_LEN_half)*sizeof(float));
	if (fread(&(array_audio2[SAMPLE_LEN_half]),4,SAMPLE_LEN_half,audio2)<SAMPLE_LEN_half)
	{
		closeFiles();
		return 1;
	}

	// tady se nejdřív posune pole o polovinu do leva a pak se načte polovina SAMPLE_LEN
	// if (fread(array_audio3,4,SAMPLE_LEN,audio3)<SAMPLE_LEN)
	memcpy(&(array_audio3[0]),&(array_audio3[SAMPLE_LEN_half]),(SAMPLE_LEN_half)*sizeof(float));
	if (fread(&(array_audio3[SAMPLE_LEN_half]),4,SAMPLE_LEN_half,audio3)<SAMPLE_LEN_half)
	{
		closeFiles();
		return 1;
	}
	
	// tady se nejdřív posune pole o polovinu do leva a pak se načte polovina SAMPLE_LEN
	// if (fread(array_audio4,4,SAMPLE_LEN,audio4)<SAMPLE_LEN)
	memcpy(&(array_audio4[0]),&(array_audio4[SAMPLE_LEN_half]),(SAMPLE_LEN_half)*sizeof(float));
	if (fread(&(array_audio4[SAMPLE_LEN_half]),4,SAMPLE_LEN_half,audio4)<SAMPLE_LEN_half)
	{
		closeFiles();
		return 1;
	}
	
	memcpy(&(filt_larray_audio5[0]),&(larray_audio5[0]),(278+SAMPLE_LEN)*sizeof(float));
	memcpy(&(filt_array_audio1[0]),&(array_audio1[0]),(SAMPLE_LEN)*sizeof(float));
	memcpy(&(filt_array_audio2[0]),&(array_audio2[0]),(SAMPLE_LEN)*sizeof(float));
	memcpy(&(filt_array_audio3[0]),&(array_audio3[0]),(SAMPLE_LEN)*sizeof(float));
	memcpy(&(filt_array_audio4[0]),&(array_audio4[0]),(SAMPLE_LEN)*sizeof(float));
	
	return 0;
}

// normalize arrays with avg value, used for subCrossCorel
void normalizeArrays(double avgValue1, double avgValue2, double avgValue3, double avgValue4, double avgValue5)
{
	for(int i = 0;i<SAMPLE_LEN;i++)
	{
		filt_array_audio1[i] = filt_array_audio1[i]*10/avgValue1;
		filt_array_audio2[i] = filt_array_audio2[i]*10/avgValue2;
		filt_array_audio3[i] = filt_array_audio3[i]*10/avgValue3;
		filt_array_audio4[i] = filt_array_audio4[i]*10/avgValue4;
		filt_larray_audio5[i] = filt_larray_audio5[i]*10/avgValue5;
	}
	for(int i = SAMPLE_LEN;i<SAMPLE_LEN+278;i++)
	{
		filt_larray_audio5[i] = filt_larray_audio5[i]*10/avgValue5;
	}
}

// compute energy of every channel and all channels
// stores min_energy and returns 1 if energy exceed threshold
int energyCompute(double* energie1,double* energie2,double* energie3,double* energie4,double* energy_all)
{
	static double min_energy = 999999.0;
	
	*energie1 = 0;
	*energie2 = 0;
	*energie3 = 0;
	*energie4 = 0;
	
	for(int j = 0;j<SAMPLE_LEN;j++)
	{
		*energie1 += filt_array_audio1[j]*filt_array_audio1[j];
		*energie2 += filt_array_audio2[j]*filt_array_audio2[j];
		*energie3 += filt_array_audio3[j]*filt_array_audio3[j];
		*energie4 += filt_array_audio4[j]*filt_array_audio4[j];
	}
	*energy_all = (*energie1) + (*energie2) + (*energie3) + (*energie4);
	*energy_all = (*energy_all)/SAMPLE_LEN;
	
	if((*energy_all)<min_energy)
		min_energy=(*energy_all);
	else 
		min_energy = min_energy*1.1;
	
	if((*energy_all)>min_energy*KV_E_LIMIT)		// energy is big enough (signal isnt just a noise)
		return 1;
	
	return 0;
}

// compute avg value of every channel
void avgValueCompute(double* avgValue1,double* avgValue2,double* avgValue3,double* avgValue4,double* avgValue5)
{
	
	for(int j = 0;j<SAMPLE_LEN;j++)
	{
		*avgValue1 += array_audio1[j];
		*avgValue2 += array_audio2[j];
		*avgValue3 += array_audio3[j];
		*avgValue4 += array_audio4[j];
		*avgValue5 += larray_audio5[j];
	}
	for(int j = 0;j<278;j++)
	{
		*avgValue5 += larray_audio5[j+SAMPLE_LEN];
	}
	
	*avgValue1 = *avgValue1/SAMPLE_LEN;
	*avgValue2 = *avgValue2/SAMPLE_LEN;
	*avgValue3 = *avgValue3/SAMPLE_LEN;
	*avgValue4 = *avgValue4/SAMPLE_LEN;
	*avgValue5 = *avgValue5/(SAMPLE_LEN+278);
}

// filter signals, uses raw (array_audio1) values (doesnt change), filtered signals are stored in local filtered arrays (filt_array_audio1)
void filterSignals()
{
	
	// highpass
	if(FILTER_TYPE==2)
	{
		double VC_alpha = 1/((2*M_PI*100*(1/FREQUENCY))+1);
		// double VC_inv_alpha = 1-VC_alpha;
		
		filt_array_audio1[0]=VC_alpha*array_audio1[0];
		filt_array_audio2[0]=VC_alpha*array_audio2[0];
		filt_array_audio3[0]=VC_alpha*array_audio3[0];
		filt_array_audio4[0]=VC_alpha*array_audio4[0];
		filt_larray_audio5[0]=VC_alpha*larray_audio5[0];
			
		for(int i = 1;i<SAMPLE_LEN;i++){		
			filt_array_audio1[i] = VC_alpha*filt_array_audio1[i-1] +VC_alpha*(array_audio1[i] -array_audio1[i-1]);
			filt_array_audio2[i] = VC_alpha*filt_array_audio2[i-1] +VC_alpha*(array_audio2[i] -array_audio2[i-1]);
			filt_array_audio3[i] = VC_alpha*filt_array_audio3[i-1] +VC_alpha*(array_audio3[i] -array_audio3[i-1]);
			filt_array_audio4[i] = VC_alpha*filt_array_audio4[i-1] +VC_alpha*(array_audio4[i] -array_audio4[i-1]);
			filt_larray_audio5[i]= VC_alpha*filt_larray_audio5[i-1]+VC_alpha*(larray_audio5[i]-larray_audio5[i-1]);
		}
		for(int i = SAMPLE_LEN;i<SAMPLE_LEN+278;i++){
			filt_larray_audio5[i] = VC_alpha*filt_larray_audio5[i-1]+VC_alpha*(larray_audio5[i]-larray_audio5[i-1]);
		}
	}
	
	
	// lowpass
	double VC_alpha = (2*M_PI*2000*(1/FREQUENCY))/((2*M_PI*2000*(1/FREQUENCY))+1);
	// double VC_alpha = 0.57;
	double VC_inv_alpha = 1-VC_alpha;
	
	filt_array_audio1[0]=VC_alpha*filt_array_audio1[0];
	filt_array_audio2[0]=VC_alpha*filt_array_audio2[0];
	filt_array_audio3[0]=VC_alpha*filt_array_audio3[0];
	filt_array_audio4[0]=VC_alpha*filt_array_audio4[0];
	filt_larray_audio5[0]=VC_alpha*filt_larray_audio5[0];
		
	for(int i = 1;i<SAMPLE_LEN;i++){		
		filt_array_audio1[i] = VC_alpha*filt_array_audio1[i]+VC_inv_alpha*filt_array_audio1[i-1];
		filt_array_audio2[i] = VC_alpha*filt_array_audio2[i]+VC_inv_alpha*filt_array_audio2[i-1];
		filt_array_audio3[i] = VC_alpha*filt_array_audio3[i]+VC_inv_alpha*filt_array_audio3[i-1];
		filt_array_audio4[i] = VC_alpha*filt_array_audio4[i]+VC_inv_alpha*filt_array_audio4[i-1];
		filt_larray_audio5[i]= VC_alpha*filt_larray_audio5[i]+VC_inv_alpha*filt_larray_audio5[i-1];
	}
	for(int i = SAMPLE_LEN;i<SAMPLE_LEN+279;i++){
		filt_larray_audio5[i] = VC_alpha*filt_larray_audio5[i]+VC_inv_alpha*filt_larray_audio5[i-1];
	}
	
	

	
	
	printf("FILT  ");
	
	fwrite(filt_array_audio3,4, SAMPLE_LEN, out_raw);
}

// compute cross-correlation, saves all values into array statistics
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
				
		for(int j = 0;j<SAMPLE_LEN;j++)	// provede sumu v posunu o i
		{	
			temp_result1 = temp_result1 + (filt_array_audio1[j] * filt_larray_audio5[j+i]);
			temp_result2 = temp_result2 + (filt_array_audio2[j] * filt_larray_audio5[j+i]);
			temp_result3 = temp_result3 + (filt_array_audio3[j] * filt_larray_audio5[j+i]);
			temp_result4 = temp_result4 + (filt_array_audio4[j] * filt_larray_audio5[j+i]);
			
			norm_var5 = norm_var5 + (filt_larray_audio5[j+i] * filt_larray_audio5[j+i]);	
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

// compute subCrossCorel, saves all corelation values into statistics
// signals doesnt need to be normalized, but not normalized arent tested (isnt recommended)
int subCrossCorel(double statistics[5][279])
{
	double temp_result1 = 0.0;
	double temp_result2 = 0.0;
	double temp_result3 = 0.0;
	double temp_result4 = 0.0;
	
	for(int i = 0; i < 279 ; i++)
	{
		temp_result1 = 0.0;
		temp_result2 = 0.0;
		temp_result3 = 0.0;
		temp_result4 = 0.0;
				
		for(int j = 0;j<SAMPLE_LEN;j++)	// provede sumu v posunu o i
		{	
			temp_result1 = temp_result1 + fabs(filt_array_audio1[j] - filt_larray_audio5[j+i]);
			temp_result2 = temp_result2 + fabs(filt_array_audio2[j] - filt_larray_audio5[j+i]);
			temp_result3 = temp_result3 + fabs(filt_array_audio3[j] - filt_larray_audio5[j+i]);
			temp_result4 = temp_result4 + fabs(filt_array_audio4[j] - filt_larray_audio5[j+i]);	
		}
		
		double NCC_alpha = 0.7;		// weight of old corelation values. 1-alpha = weight of new corelation values
		
		statistics[1][i]=(statistics[1][i]*NCC_alpha)+(temp_result1*(1-NCC_alpha));
		statistics[2][i]=(statistics[2][i]*NCC_alpha)+(temp_result2*(1-NCC_alpha));
		statistics[3][i]=(statistics[3][i]*NCC_alpha)+(temp_result3*(1-NCC_alpha));
		statistics[4][i]=(statistics[4][i]*NCC_alpha)+(temp_result4*(1-NCC_alpha));
	}
	
	return 0;
}

// get highest values of statistics
void getHighestIndexes(double statistics[5][279], double * maxlocation1, double * maxlocation2, double * maxlocation3, double * maxlocation4)
{
	double maxval1 = -100000000;
	double maxval2 = -100000000;
	double maxval3 = -100000000;
	double maxval4 = -100000000;

	static int history1[4];
	static int history2[4];
	static int history3[4];
	static int history4[4];
	static unsigned int h=0;
	
	/*
	for(int i=0;i<279;i++)
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
	}// */
	
	// make smoother position changes, might create defect with samples of more than 3/4 sec.
	for(int i=0;i<279;i++)
	{
		if(statistics[1][i]>maxval1)
		{
			maxval1=statistics[1][i];
			history1[h%4] = i-139;
		}
		if(statistics[2][i]>maxval2)
		{
			maxval2=statistics[2][i];
			history2[h%4] = i-139;
		}
		if(statistics[3][i]>maxval3)
		{
			maxval3=statistics[3][i];
			history3[h%4] = i-139;
		}
		if(statistics[4][i]>maxval4)
		{
			maxval4=statistics[4][i];
			history4[h%4] = i-139;
		}
	}

	double avg_shift1 = (history1[0]+history1[1]+history1[2]+history1[3])/4.0;
	double avg_shift2 = (history2[0]+history2[1]+history2[2]+history2[3])/4.0;
	double avg_shift3 = (history3[0]+history3[1]+history3[2]+history3[3])/4.0;
	double avg_shift4 = (history4[0]+history4[1]+history4[2]+history4[3])/4.0;

	if(h<4)				// delete this branch of "if" if you gonna use this app (this excludes first 4 entries so they wont be super-wrong (worse test results))
	{					// but it doesnt need to be here and consumes process time
		*maxlocation1 = history1[h%4];
		*maxlocation2 = history2[h%4];
		*maxlocation3 = history3[h%4];
		*maxlocation4 = history4[h%4];
	}else{
		double leastdif1 = 140;
		double leastdif2 = 140;
		double leastdif3 = 140;
		double leastdif4 = 140;
		
		for(int i = 0;i<4;i++)
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

// get lowest values of statistics
void getLowestIndexes(double statistics[5][279], double * maxlocation1, double * maxlocation2, double * maxlocation3, double * maxlocation4)
{
	double minval1 = __DBL_MAX__;
	double minval2 = __DBL_MAX__;
	double minval3 = __DBL_MAX__;
	double minval4 = __DBL_MAX__;

	for(int i=0;i<279;i++)
	{
		if(statistics[1][i]<minval1)
		{
			minval1=statistics[1][i];
			*maxlocation1 = i-139;
		}
		if(statistics[2][i]<minval2)
		{
			minval2=statistics[2][i];
			*maxlocation2 = i-139;
		}
		if(statistics[3][i]<minval3)
		{
			minval3=statistics[3][i];
			*maxlocation3 = i-139;
		}
		if(statistics[4][i]<minval4)
		{
			minval4=statistics[4][i];
			*maxlocation4 = i-139;
		}
	}
}

// compute coordinate z out of coordinate x and y (hor_x, ver_x) and distance from axes (ver_y, hor_y)
double compCoorZ(double hor_x, double hor_y, double ver_x, double ver_y)
{
	
	double z1 = sqrt(fabs((ver_y*ver_y) - (hor_x*hor_x)));		// teoreticky by stačil jeden výpočet, pro přesnější a hladší výpočet zakomponuji oba výpočty
	double z2 = sqrt(fabs((hor_y*hor_y) - (ver_x*ver_x)));
	
	return (z1+z2)/2;
}

// compute coordinates from array staticstics
int computeCoor(double statistics[5][279],double *x, double *y, double *z)
{	

	double maxlocation1 = 333.0;
	double maxlocation2 = 333.0;
	double maxlocation3 = 333.0;
	double maxlocation4 = 333.0;
	
	if (COMPUTATION_TYPE == 3)
		getLowestIndexes(statistics, &maxlocation1, &maxlocation2, &maxlocation3, &maxlocation4);
	else
		getHighestIndexes(statistics, &maxlocation1, &maxlocation2, &maxlocation3, &maxlocation4);
	
	
	
	double hor_x = 0.0;			// horizontal coordinate
	double hor_y = 0.0;			// distance from horizontal axis
	
	double ver_x = 0.0;			// vertical coordinate
	double ver_y = 0.0;			// distance from vertical axis
	int swaped = 0;
	
	if(maxlocation4 == 0.0){
		maxlocation4=maxlocation2;
		maxlocation2=0;
		swaped = 1;
	}
		
	maxlocation2 = (maxlocation2*SSPEED)/FREQUENCY;
	maxlocation2 = (maxlocation2/2);
	maxlocation4 = (maxlocation4*SSPEED)/FREQUENCY;
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
		printf("posun: %3.2lf, %3.2lf, %3.2lf, %3.2lf  ", (maxlocation1*2*FREQUENCY)/SSPEED, (maxlocation2*2*FREQUENCY)/SSPEED, (maxlocation3*2*FREQUENCY)/SSPEED, (maxlocation4*2*FREQUENCY)/SSPEED);

	return 0;
}

// FOR TESTING PURPOSE
// generate white noise, add sinus function, compute energies, prints SNR (signal to noise ratio)
void generateSignal()
{
	int delay = 50;
	double signal_w = 0.2;
	double fr = 440;	// frekvence
	
	double energy_all = 0.0;
	double energie1 = 0.0;
	double energie2 = 0.0;
	double energie3 = 0.0;
	double energie4 = 0.0;
	// (((rand()%1000)/1000.0)*0.5)
	// for(int i = 0;i<SAMPLE_LEN;i++){		
		// filt_array_audio1[i] = sin(2*M_PI*i/(48000/fr))*signal_w+(((rand()%1000)/1000.0)-0.5);
		// filt_array_audio2[i] = sin(2*M_PI*i/(48000/fr))*signal_w+(((rand()%1000)/1000.0)-0.5);
		// filt_array_audio3[i] = sin(2*M_PI*i/(48000/fr))*signal_w+(((rand()%1000)/1000.0)-0.5);
		// filt_array_audio4[i] = sin(2*M_PI*i/(48000/fr))*signal_w+(((rand()%1000)/1000.0)-0.5);
		// filt_larray_audio5[i]= sin(2*M_PI*(i+139-delay)/(48000/fr))*signal_w+(((rand()%1000)/1000.0)-0.5);
	// }
	// for(int i = SAMPLE_LEN;i<SAMPLE_LEN+279;i++){
		// filt_larray_audio5[i] = sin(2*M_PI*(i+139-delay)/(48000/fr))*signal_w+(((rand()%1000)/1000.0)-0.5);
	// }
	
	// fill with noise
	for(int i = 0;i<SAMPLE_LEN;i++){		
		filt_array_audio1[i] = (((rand()%1000)/1000.0)-0.5);
		filt_array_audio2[i] = (((rand()%1000)/1000.0)-0.5);
		filt_array_audio3[i] = (((rand()%1000)/1000.0)-0.5);
		filt_array_audio4[i] = (((rand()%1000)/1000.0)-0.5);
		filt_larray_audio5[i]= (((rand()%1000)/1000.0)-0.5);
	}
	for(int i = SAMPLE_LEN;i<SAMPLE_LEN+279;i++){
		filt_larray_audio5[i] = (((rand()%1000)/1000.0)-0.5);
	}
	
	// compute energy of noise
	energyCompute(&energie1,&energie2,&energie3,&energie4,&energy_all);
	energie2=0;
	
	// add sinus signal
	for(int i = 1000;i<SAMPLE_LEN-1000;i++){		
		filt_array_audio1[i] += sin(2*M_PI*i/(48000/fr))*signal_w;
		filt_array_audio2[i] += sin(2*M_PI*i/(48000/fr))*signal_w;
		filt_array_audio3[i] += sin(2*M_PI*i/(48000/fr))*signal_w;
		filt_array_audio4[i] += sin(2*M_PI*i/(48000/fr))*signal_w;
		energie2 += (sin(2*M_PI*i/(48000/fr))*signal_w)*(sin(2*M_PI*i/(48000/fr))*signal_w);
		// filt_larray_audio5[i]= sin(2*M_PI*(i)/(48000/fr))*signal_w+(((rand()%1000)/1000.0)-0.5);
	}
	for(int i = 1000-delay;i<SAMPLE_LEN-1000-delay;i++){
		filt_larray_audio5[i+139] += sin(2*M_PI*(i+delay)/(48000/fr))*signal_w;
	}
	printf("en1 %lf en2 %lf, SNR %lf\n",energie1/SAMPLE_LEN,energie2/SAMPLE_LEN,energie2/energie1);
	
}


int main(int argc, char* argv[])
{
	if(argc!=6)
	{
		printf("chybný počet parametrů, zadejte prosím 5 validních souborů s nahrávkami\n");
		return 1;
	}
	
	memset(array_audio1, 0, SAMPLE_LEN*sizeof(float));
	memset(array_audio2, 0, SAMPLE_LEN*sizeof(float));
	memset(array_audio3, 0, SAMPLE_LEN*sizeof(float));
	memset(array_audio4, 0, SAMPLE_LEN*sizeof(float));
	memset(larray_audio5, 0, (SAMPLE_LEN+279)*sizeof(float));

	long unsigned int B_counter = 1;

	double statistics[5][279];		// 5 for better orientation -> audio4 statistics[4] array_audio4 ... statistics[0] not used

	memset(statistics[1], 0, 279*sizeof(double));
	memset(statistics[2], 0, 279*sizeof(double));
	memset(statistics[3], 0, 279*sizeof(double));
	memset(statistics[4], 0, 279*sizeof(double));
	
	
	double energy_all = 0.0;
	double energie1 = 0.0;
	double energie2 = 0.0;
	double energie3 = 0.0;
	double energie4 = 0.0;

	if(openFiles(argv))
	{
		printf("couldnt open files properly\n");
		return 1;
	}
	
	// TESTING CORELATION
	// double subcoreltest[4*1000];
	// double normcoreltest[4*1000];
	// for(int k=0;k<1000;k+=4){
	// generateSignal();
	
	// memset(statistics[1], 0, 279*sizeof(double));
	// memset(statistics[2], 0, 279*sizeof(double));
	// memset(statistics[3], 0, 279*sizeof(double));
	// memset(statistics[4], 0, 279*sizeof(double));
	
	// energyCompute(&energie1,&energie2,&energie3,&energie4,&energy_all);
	// normCrossCorel(energie1, energie2, energie3, energie4, statistics);
	// getHighestIndexes(statistics, &normcoreltest[k], &normcoreltest[k+1], &normcoreltest[k+2], &normcoreltest[k+3]);
	
	// for(int i = 0;i<279;i++)
		// fprintf(out_test,"%d %lf %lf %lf %lf\n",i-139,statistics[1][i],statistics[2][i],statistics[3][i],statistics[4][i]);
	
	
	// memset(statistics[1], 0, 279*sizeof(double));
	// memset(statistics[2], 0, 279*sizeof(double));
	// memset(statistics[3], 0, 279*sizeof(double));
	// memset(statistics[4], 0, 279*sizeof(double));
	
	// subCrossCorel(statistics);
	// getLowestIndexes(statistics, &subcoreltest[k], &subcoreltest[k+1], &subcoreltest[k+2], &subcoreltest[k+3]);
	
	// for(int i = 0;i<279;i++)
		// fprintf(out_test,"\n%d %lf %lf %lf %lf",i-139,statistics[1][i],statistics[2][i],statistics[3][i],statistics[4][i]);
	
	// }
	// for(int i=0;i<1000;i++)
		// fprintf( out_test,"%d %d\n",(int)subcoreltest[i], (int)normcoreltest[i]);
	// fwrite(filt_larray_audio5,4, SAMPLE_LEN+278, out_raw);
	// fwrite(filt_array_audio3,4, SAMPLE_LEN, out_raw);
	// closeFiles();/*
	// END OF TESTING CORELATION
	
	if(loadHalfOfArray())
	{
		printf("error in reading\n");
		return 1;
	}
	
	if(firstLoadOfArray())
	{
		printf("error in reading\n");
		return 1;
	}
	
	if(loadHalfOfArray())
	{
		printf("error in reading\n");
		return 1;
	}
	
	double x,y,z;
	
	// variables needed for GCC-phat
	double siga_loc[SAMPLE_LEN];     //< local copy of input A 
	double sigb_loc[SAMPLE_LEN];     //< local copy of input B 
	double lags_loc[SAMPLE_LEN];     //< local version of computed lag values 
	fftw_complex ffta[SAMPLE_LEN];  //< fft of input A 
	fftw_complex fftb[SAMPLE_LEN];  //< fft of input B 
	fftw_complex xspec[SAMPLE_LEN];  //< the cross-spectrum of A & B 
	double xcorr[SAMPLE_LEN+1];        //< the cross-correlation of A & B 

	fftw_plan pa = fftw_plan_dft_r2c_1d(SAMPLE_LEN, siga_loc, ffta,  FFTW_ESTIMATE|FFTW_DESTROY_INPUT);
	fftw_plan pb = fftw_plan_dft_r2c_1d(SAMPLE_LEN, sigb_loc, fftb,  FFTW_ESTIMATE|FFTW_DESTROY_INPUT);
	fftw_plan px = fftw_plan_dft_c2r_1d(SAMPLE_LEN, xspec,    xcorr, FFTW_ESTIMATE|FFTW_DESTROY_INPUT);

	
	while(1)
	{
		B_counter++;
		
		// load data (len = SAMPLE_LEN_half) two times
		if(loadHalfOfArray())
		{
			printf("End of file or error occures\n");
			return 0;
		}
		if(loadHalfOfArray())
		{
			printf("End of file or error occures\n");
			return 0;
		}
		
		if(FILTER_TYPE >= 1){
			// fwrite(array_audio3,4, SAMPLE_LEN, out_raw);	//load signal into out_test.raw
			
			// filtering signals... input array_audio, output filt_array_audio
			filterSignals();
		}
		if(COMPUTATION_TYPE == 1)			// normCrossCorel
		{
			// compute energy of all signals
			if(energyCompute(&energie1,&energie2,&energie3,&energie4,&energy_all))
			{
				// clock_t start, end;
				// double cpu_time_used;
		 
				// start = clock();

				normCrossCorel(energie1, energie2, energie3, energie4, statistics);

				// end = clock();
				// cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
				// printf("cpu time norm cross %6.3lf  \n",cpu_time_used);

				computeCoor(statistics,&x,&y,&z);
			}
			
			
			if(PRINT)
				printf("%4ld: cas:%5.2f, energie: %6.3lf  ",B_counter,((B_counter)*SAMPLE_LEN)/FREQUENCY,energy_all*10000);
			

			// Prints one window of computed corelation into out_test.txt
			// if(B_counter==94)
			// {		
				// for(int i = 0;i<279;i++)
					// fprintf(out_test,"%d %lf %lf %lf %lf\n",i-139,statistics[1][i],statistics[2][i],statistics[3][i],statistics[4][i]);
			// }
			
			if(PRINT == 0)
			{
				printf("\r  [ % 6.3lf  % 6.3lf  % 6.3lf ]",x,y,z);
				fflush(stdout);
			} else
				printf("  [ % 6.3lf  % 6.3lf  % 6.3lf ]\n",x,y,z);
			
			fprintf( out_test,"%lf %lf %lf\n",x,y,z);
		} 
		else if (COMPUTATION_TYPE == 2)		// gccPHATCrossCorel
		 // * Inspired by https://github.com/wooters/miniDSP
		 // * Author: Chuck Wooters https://github.com/wooters
		 // * Doesnt work, dont know why 
		{
			if(energyCompute(&energie1,&energie2,&energie3,&energie4,&energy_all))
			{	
				for(int s = 0;s<4;s++){
					switch(s)
					{
						case 0:
						memcpy(siga_loc,filt_array_audio1,SAMPLE_LEN);
						break;
						case 1:
						memcpy(siga_loc,filt_array_audio2,SAMPLE_LEN);
						break;
						case 2:
						memcpy(siga_loc,filt_array_audio3,SAMPLE_LEN);
						break;
						case 3:
						memcpy(siga_loc,filt_array_audio4,SAMPLE_LEN);
						break;
					}
					memcpy(sigb_loc,&(filt_larray_audio5[139]),SAMPLE_LEN);

					// FFT of signals a and middle
					fftw_execute(pa);
					fftw_execute(pb);

					
					// PHAse Transform
					unsigned xspec_bound = SAMPLE_LEN/2+1;
					for (unsigned i = 0; i <= xspec_bound; i++) {
						double complex tmp = fftb[i] * conj(ffta[i]); // cross-spectra 
						xspec[i] = tmp / (cabs(tmp)+__DBL_MIN__); 	//adding DBL_MIN to prevent div by 0 +__DBL_MIN_
					}
					// inverse FFT
					fftw_execute(px);
					
					// shift values so 0 shif is in middle
					unsigned xx = (unsigned) floor((double) SAMPLE_LEN/2.0);
					memcpy(lags_loc,&xcorr[xx],sizeof(double)*(SAMPLE_LEN-xx));
					memcpy(&lags_loc[SAMPLE_LEN-xx],xcorr,sizeof(double)*xx);

					// copy only possible window
					int ij = 0;
					for(int i=(SAMPLE_LEN/2)-1-139;i<(SAMPLE_LEN/2)-1+139;i++)		//(int i=(SAMPLE_LEN/2)-1-139;i<(SAMPLE_LEN/2-1)+139;i++)
					{
						statistics[s+1][ij]=lags_loc[i];
						ij++;
					}
				}
				
				if(PRINT)
					printf("...");
			}
			
			if(PRINT)
				printf("%4ld: cas:%5.2f, energie: %6.3lf  ",B_counter,((B_counter)*SAMPLE_LEN)/FREQUENCY,energy_all*10000);
			
			computeCoor(statistics,&x,&y,&z);
			
			if(PRINT == 0)
			{
				printf("\r  [ % 6.3lf  % 6.3lf  % 6.3lf ]",x,y,z);
				fflush(stdout);
			} else
				printf("  [ % 6.3lf  % 6.3lf  % 6.3lf ]\n",x,y,z);
			
			fprintf( out_test,"%lf %lf %lf\n",x,y,z);
		} 
		else if (COMPUTATION_TYPE == 3)		// subtraction of signals
		{
			double avgValue1 = 0;
			double avgValue2 = 0;
			double avgValue3 = 0;
			double avgValue4 = 0; 
			double avgValue5 = 0;
			
			// compute average value of evesignals
			avgValueCompute(&avgValue1, &avgValue2, &avgValue3, &avgValue4, &avgValue5);
			
			// normalize all singlas
			normalizeArrays(avgValue1, avgValue2, avgValue3, avgValue4, avgValue5);

			
			if(energyCompute(&energie1,&energie2,&energie3,&energie4,&energy_all))
			{
				subCrossCorel(statistics);
			}
			
			if(PRINT)
				printf("%4ld: cas:%5.2f, energie: %6.3lf  ",B_counter,((B_counter)*SAMPLE_LEN)/FREQUENCY,energy_all*10000);
			
			computeCoor(statistics,&x,&y,&z);
			
			if(PRINT == 0)
			{
				printf("\r  [ % 6.3lf  % 6.3lf  % 6.3lf ]",x,y,z);
				fflush(stdout);
			} else
				printf("  [ % 6.3lf  % 6.3lf  % 6.3lf ]\n",x,y,z);
			
			fprintf( out_test,"%lf %lf %lf\n",x,y,z);
		}
	}
	// */
	return 0;
}