/*author: František Horázný
 *year: 2020
 *contact: fhorazny@gmail.com*/
 
 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FREQUENCY 48000

double compCoorZ(double hor_x, double hor_y, double ver_x, double ver_y)
{
	
	double z1 = sqrt(fabs((ver_y*ver_y) - (hor_x*hor_x)));		// teoreticky by stačil jeden výpočet, pro přesnější a hladší výpočet zakomponuji oba výpočty
	double z2 = sqrt(fabs((hor_y*hor_y) - (ver_x*ver_x)));
	
	
	return (z1+z2)/2;
}

int computeCoor( double maxlocation1, double maxlocation2, double maxlocation3, double maxlocation4,double *x, double *y, double *z)
{	
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
	
	ver_y = sqrt(fabs(((((ver_x-0.5)*(ver_x-0.5))/pow_maxlocation3)-1) * (0.25-pow_maxlocation3)));//sqrt(((pow(ver_x-0.5,2.0)/pow_maxlocation3)-1) * (0.25-pow_maxlocation3));   

	if(swaped)
		ver_x=-ver_x;
	

	*z = compCoorZ(hor_x,hor_y,ver_x,ver_y);
	*x = hor_x;
	*y = ver_x;
	// *y = 0;
	
	// *z = hor_y;
	

	// printf("posun: %3.2lf, %3.2lf, %3.2lf, %3.2lf\n", (maxlocation1*2*FREQUENCY)/343, (maxlocation2*2*FREQUENCY)/343, (maxlocation3*2*FREQUENCY)/343, (maxlocation4*2*FREQUENCY)/343);

	return 0;
}

int computeCoor2D(double maxlocation2, double maxlocation4,double *x, double *z)
{	
	double hor_x = 0.0;			// horizontal coordinate
	double hor_y = 0.0;			// distance from horizontal axis
	
	
	int swaped = 0;
	
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
	

	*x = hor_x;	
	*z = hor_y;
	
	return 0;
}

void calculate_points()
{
	FILE *out_test;

	out_test = fopen("./out_test.txt", "w");

	for(int loc2=-139;loc2<140;loc2++){	
		for(int loc4=-139;loc4<140;loc4++){
			double output_x, output_z;
			computeCoor2D(loc2, loc4, &output_x, &output_z);
			fprintf(out_test,"%f %f\n", output_x, output_z);
		} 
		printf("%d \n",loc2);
	}
	
	fclose(out_test);
}

int main(int argc, char* argv[])
{
	// calculate_points();
	

	
	
	
	
	/*  začátek výpočtu průměrné chyby v krychli 1x1 metr
	double maxdist = 0;
	double mindist = 9999999;
	double avgdist = 0;
	double distanc = 0;
	
	double x = atof(argv[1]);
	double y = atof(argv[2]);
	double z = atof(argv[3]);
	
	double output_x = __DBL_MIN__;
	double output_y = __DBL_MIN__;
	double output_z = __DBL_MIN__;
	


	for(int i=-50;i<50;i++)
	{
		for(int j=-50;j<50;j++)	
		{	
			for(int k=-50;k<50;k++)
			{
				double input_x = x+(i/100.0);
				double input_y = y+(j/100.0);
				double input_z = z+(k/100.0);

		
				double input_zx=((input_x*input_x)+(input_z*input_z));
				double input_zy=((input_y*input_y)+(input_z*input_z));
				
				double maxlocation1 = sqrt(((input_y+1)*(input_y+1))+input_zx)-sqrt((input_y*input_y)+input_zx);
				double maxlocation2 = sqrt(((input_x+1)*(input_x+1))+input_zy)-sqrt((input_x*input_x)+input_zy);
				double maxlocation3 = sqrt(((input_y-1)*(input_y-1))+input_zx)-sqrt((input_y*input_y)+input_zx);
				double maxlocation4 = sqrt(((input_x-1)*(input_x-1))+input_zy)-sqrt((input_x*input_x)+input_zy);
				
				maxlocation1 = (maxlocation1*FREQUENCY)/343;
				maxlocation2 = (maxlocation2*FREQUENCY)/343;
				maxlocation3 = (maxlocation3*FREQUENCY)/343;
				maxlocation4 = (maxlocation4*FREQUENCY)/343;
				
				maxlocation1 = (int)round(maxlocation1);
				maxlocation2 = (int)round(maxlocation2);
				maxlocation3 = (int)round(maxlocation3);
				maxlocation4 = (int)round(maxlocation4);
				
				computeCoor( maxlocation1, maxlocation2, maxlocation3, maxlocation4, &output_x, &output_y, &output_z);
				
				// printf("vypočítané hodnoty [%3.2lf,%3.2lf,%3.2lf]\n", output_x, output_y, output_z);
				distanc = sqrt( ((input_x-output_x)*(input_x-output_x)) + ((input_y-output_y)*(input_y-output_y)) + ((input_z-output_z)*(input_z-output_z))  );
				// distanc=fabs(input_z-output_z);
				
				if(maxdist<distanc)
					maxdist=distanc;
				
				if(mindist>distanc)
					mindist=distanc;
				
				avgdist +=distanc;
			}
		}
	}
	avgdist = avgdist/(100.0*100.0*100.0);
	
	printf("minimální rozdíl %3.2lf\n", mindist*100);
	printf("maximální rozdíl %3.2lf\n", maxdist*100);
	printf("průměrný  rozdíl %3.2lf\n", avgdist*100);
	// KONEC */
	
	
	// /* Začátek zpětného výpočtu 
	if(argc!=4){		// print help and end
		printf("prosím zadejte 3 argumenty\n");
		printf("argumenty mohou být pouze desetinná nebo celá čísla, jinak má nedefinované chování\n");
		printf("například \"./coor 1 0 3\"\n");
		return 0;
	}
	
	double input_x = atof(argv[1]);
	double input_y = atof(argv[2]);
	double input_z = atof(argv[3]);
	
	printf("původní zadané hodnoty [%3.2f,%3.2f,%3.2f]\n", input_x, input_y, input_z);
	
	double output_x = __DBL_MIN__;
	double output_y = __DBL_MIN__;
	double output_z = __DBL_MIN__;
	
	double input_zx=((input_x*input_x)+(input_z*input_z));
	double input_zy=((input_y*input_y)+(input_z*input_z));
	
	double maxlocation1 = sqrt(((input_y+1)*(input_y+1))+input_zx)-sqrt((input_y*input_y)+input_zx);
	double maxlocation2 = sqrt(((input_x+1)*(input_x+1))+input_zy)-sqrt((input_x*input_x)+input_zy);
	double maxlocation3 = sqrt(((input_y-1)*(input_y-1))+input_zx)-sqrt((input_y*input_y)+input_zx);
	double maxlocation4 = sqrt(((input_x-1)*(input_x-1))+input_zy)-sqrt((input_x*input_x)+input_zy);
	
	
	
	
	maxlocation1 = (maxlocation1*FREQUENCY)/343;
	maxlocation2 = (maxlocation2*FREQUENCY)/343;
	maxlocation3 = (maxlocation3*FREQUENCY)/343;
	maxlocation4 = (maxlocation4*FREQUENCY)/343;
	
	
	printf("vypočítaný nezaokrouhlený posun %3.2lf, %3.2lf, %3.2lf, %3.2lf\n", maxlocation1, maxlocation2, maxlocation3, maxlocation4);
	
	maxlocation1 = (int)round(maxlocation1);
	maxlocation2 = (int)round(maxlocation2);
	maxlocation3 = (int)round(maxlocation3);
	maxlocation4 = (int)round(maxlocation4);
	
	
	computeCoor( maxlocation1, maxlocation2, maxlocation3, maxlocation4, &output_x, &output_y, &output_z);
	
	printf("vypočítané hodnoty [%3.2lf,%3.2lf,%3.2lf]\n", output_x, output_y, output_z);
	
	printf("meow\n");
	// */
	return 0;
	
}