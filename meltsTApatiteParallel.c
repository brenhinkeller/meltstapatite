/******************************************************************************
 * FILE: meltsTApatiteParallel.c
 * DESCRIPTION:  
 *   Calculates apatite saturation temperature and mass of apatite saturated 
 *   using pMELTS batch crystallization and the apate saturation equations of 
 *   Watson and Harrison, 1984. Calculates saturation of different whole-rock
 *   compositions in parallel with MPI.
 * AUTHOR: C. Brenhin Keller
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <mpi.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include "arrays.h"
#include "runmelts.h"

double meltsM(double * const array){
	// Format: SiO2 TiO2 Al2O3 Fe2O3 Cr2O3 FeO MnO MgO NiO CoO CaO Na2O K2O P2O5 
	double Si=array[0]/(28.0844+15.9994*2);
	double Ti=array[1]/(47.867+15.9994*2);
	double Al=array[2]/(26.9815+15.9994*1.5);
	double Fe=array[3]/(55.845+15.9994*1.5) + array[5]/(55.845+15.9994);
	double Cr=array[4]/(51.9961+15.9994*1.5);
	double Mn=array[6]/(54.9380+15.9994);
	double Mg=array[7]/(24.3050+15.9994);
	double Ni=array[8]/(58.6934+15.9994);
	double Co=array[9]/(58.9332+15.9994);
	double Ca=array[10]/(40.078+15.9994);
	double Na=array[11]/(22.9898+15.9994/2);
	double K=array[12]/(39.0983+15.9994/2);
	double P=array[13]/(30.9738+15.9994*2.5);
	double TotalMoles = Si+Ti+Al+Fe+Cr+Mn+Mg+Ni+Co+Ca+Na+K+P;
	double M = (Na + K + 2*Ca) / (Al * Si) * TotalMoles;
	return M;
}

double tzirc(const double M, const double Zr){
	// Boehnke et al.
	double Tsat = 10108.0 / (log(496000.0/Zr) + 1.16*(M-1) + 1.48) - 273.15; // Temperature in Celcius
	if (Zr<=0){
		Tsat = NAN;
	}
	return Tsat;
}

double tzircZr(const double M, const double T){
	double Zrsat = 496000.0 / exp(10108.0/(T+273.15) - 1.16*(M-1) - 1.48);
	return Zrsat;
}


double tapatite(const double SiO2, const double P2O5){
	// Harrison and Watson, 1984 GCA 48 pp1467-1477
	double Tsat = (8400 + (SiO2/100 - 0.5) * 2.65E4) / (log(41.82/P2O5) + 3.1 + 12.4*(SiO2/100 - 0.5)) - 273.15; // Temperature in Celcius
	if (P2O5<=0){
		Tsat = NAN;
	}
	return Tsat;
}

double tapatiteP2O5(const double SiO2, const double T){
	// Harrison and Watson, 1984 GCA 48 pp1467-1477
	double P2O5sat = 41.82 / exp((8400 + (SiO2/100 - 0.5)*2.64E4)/(T+273.15) - (3.1 + 12.4*(SiO2/100 - 0.5)));
	return P2O5sat;
}




int main(int argc, char **argv){
	uint32_t datarows, datacolumns;
	uint32_t i, j, k;
	int world_size, world_rank, rc;


	//Check input arguments
	if (argc != 2) {
		fprintf(stderr,"USAGE: %s <input_filename>\n", argv[0]);
		exit(1);
	}

	// Start MPI
	rc = MPI_Init(&argc,&argv); 
	if (rc != MPI_SUCCESS) {
		printf ("Error starting MPI program. Terminating.\n"); MPI_Abort(MPI_COMM_WORLD, rc);
	}

	// Get world size (number of MPI processes) and world rank (# of this process)
	MPI_Comm_size(MPI_COMM_WORLD,&world_size);
	MPI_Comm_rank(MPI_COMM_WORLD,&world_rank);


	if (world_rank==0){
		// Declare variables used only on the root node
		int buf[world_size-1], nextReady;
		MPI_Request reqs[world_size-1];
		MPI_Status stats[world_size-1];

		// Print format of output 
		printf("Kv\tMbulk\tTliq\tTsatbulk\tTf\tTsat\tP2O5sat\tP2O5f\tFf\tSiO2\tP2O5bulk\tMAp\n");

		// Import 2-d source data array as a flat double array. Format:
		// SiO2, TiO2, Al2O3, Fe2O3, Cr2O3, FeO, MnO, MgO, NiO, CoO, CaO, Na2O, K2O, P2O5, CO2, H2O, Zr, Kv;
		double** const data = csvparse(argv[1],',', &datarows, &datacolumns);

		// Listen for task requests from the worker nodes
		for (i=1; i<world_size; i++){
			//        *buf, count, datatype, dest, tag, comm, *request
			MPI_Irecv(&buf[i-1], 1, MPI_INT, i, 0, MPI_COMM_WORLD, &reqs[i-1]);
		}

		// Once any worker asks for a new task, send next task to that worker and keep listening
		for (i=0; i<datarows; i++){
			MPI_Waitany(world_size-1, reqs, &nextReady, stats);
			//       *buf, count, datatype, dest, tag, comm
			MPI_Send(data[i], 18, MPI_DOUBLE, nextReady+1, 1, MPI_COMM_WORLD);
			//        *buf, count, datatype, source, tag, comm, *request
			MPI_Irecv(&buf[nextReady], 1, MPI_INT, nextReady+1, 0, MPI_COMM_WORLD, &reqs[nextReady]);
		}
		for (i=0; i<18; i++){
			printf("%g\t",data[0][i]);
		}

		// Wait for all workers to complete, then send the stop signal
		MPI_Waitall(world_size-1, reqs, stats);	
		double stop[18] = {-1};
		for (i=1; i<world_size; i++){
			MPI_Send(&stop, 18, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);	
		}
	}

	else {
		// Declare variables used only on the worker nodes
		MPI_Request sReq;
		MPI_Status sStat;
		double ic[18];
		FILE *fp;
//		char prefix[200], cmd_string[500];
		char* prefix = malloc(500*sizeof(char));
		char* cmd_string = malloc(1000*sizeof(char));

		// Simulation parameters
		/**********************************************************/
		// Version to run MELTS in (MELTS or pMELTS)
		const char version[]="pMELTS";
		// Melts mode (isobaric, ptpath, etc)
		const char mode[]="isobaric";

		// fO2 buffer to use (None, FMQ, etc.)
		const char fo2Buffer[]="FMQ";
		// fO2 offset from buffer
		double fo2Delta=1;

		// Initial temperature (Celcius)
		double Ti=1700;
		//Initial Pressure (bar)
		double Pi=600;
		//Temperature step size in each simulation
		const int deltaT=-10;
		// Pressure step size;
		const int deltaP=0;

		// Stop simulations at a given percent melt
		const double minPercentMelt=10;

		// Assumed bulk mineral/melt Zr partition coefficient
		const double Kd=0.01;

		// Variables that control size and location of the simulation
		/***********************************************************/	
		// Location of scratch directory (ideally local scratch for each node)
		// This location may vary on your system - contact your sysadmin if unsure
//		const char scratchdir[]="/scratch/gpfs/cbkeller/";
		const char scratchdir[]="/scratch/";

		// Variables that determine how much memory to allocate to imported results
		const int maxMinerals=100, maxSteps=1700/abs(deltaT), maxColumns=50;
		/***********************************************************/


		// Malloc space for the imported melts array
		double **rawMatrix=mallocDoubleArray(maxMinerals*maxSteps,maxColumns);
		double ***melts=malloc(maxMinerals*sizeof(double**));
		char **names=malloc(maxMinerals*sizeof(char*));
		char ***elements=malloc(maxMinerals*sizeof(char**));
		int *meltsrows=malloc(maxMinerals*sizeof(int)), *meltscolumns=malloc(maxMinerals*sizeof(int));
		char **suppressPhase = malloc(maxMinerals*sizeof(char*));
		for (i=0; i<maxMinerals; i++){
			names[i]=malloc(30*sizeof(char));
			elements[i]=malloc(maxColumns*sizeof(char*));
			suppressPhase[i] = malloc(30*sizeof(char));
			for (k=0; k<maxColumns; k++){
				elements[i][k]=malloc(30*sizeof(char));
			}
		}
		int minerals, suppress=0;

		// Suppress phases (optional)
		suppress = 2; // Number of phases you want to suppress
		strcpy(suppressPhase[0],"apatite"); // First phases
		strcpy(suppressPhase[1],"whitlockite"); // Second phase


		//  Variables for finding saturation temperature
		int row, P, T, mass, SiO2, TiO2, Al2O3, Fe2O3, Cr2O3, FeO, MnO, MgO, NiO, CoO, CaO, Na2O, K2O, P2O5, CO2, H2O;
		double M, Tf, Tsat, Tsatbulk, Ts, Tsmax, P2O5f, P2O5sat, MAp;
		
		int apatite;
		
		while (1) {
			// Ask root node for new task
			//       *buf, count, datatype, dest, tag, comm, *request
			MPI_Isend(&world_rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &sReq);
			//       *buf, count, datatype, source, tag, comm, *status
			MPI_Recv(&ic, 18, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &sStat);

			// Exit loop if stop signal recieved
			if (ic[0]<0) break;

			//Configure working directory
			sprintf(prefix,"%sout%i_%.0f/", scratchdir, world_rank, ic[17]);
			sprintf(cmd_string,"mkdir -p %s", prefix);
			system(cmd_string);

//			//Set water
//			ic[15]=3.0;
//			//Set CO2
//			ic[14]=0.1;
			
			//Run MELTS
			runmelts(prefix,ic,version,"isobaric",fo2Buffer,fo2Delta,"1\nsc.melts\n10\n1\n3\n1\nliquid\n1\n0.99\n1\n10\n0\n4\n0\n","","!",Ti,Pi,deltaT,deltaP,0.005,suppress,suppressPhase);

			// If simulation failed, clean up scratch directory and move on to next simulation
			sprintf(cmd_string,"%sPhase_main_tbl.txt", prefix);
			if ((fp = fopen(cmd_string, "r")) == NULL) {
				fprintf(stderr, "%s : MELTS equilibration failed to produce output.\n", prefix);
				sprintf(cmd_string,"rm -r %s", prefix);
				system(cmd_string);
				continue;
			}

			// Import results, if they exist. Format:
			// Pressure Temperature mass S H V Cp viscosity SiO2 TiO2 Al2O3 Fe2O3 Cr2O3 FeO MnO MgO NiO CoO CaO Na2O K2O P2O5 H2O
			minerals=maxMinerals;
			importmelts(maxSteps, maxColumns, prefix, melts, rawMatrix, meltsrows, meltscolumns, names, elements, &minerals);
			if (minerals<1 | strcmp(names[0],"liquid_0")!=0) {
				fprintf(stderr, "%s : MELTS equilibration failed to calculate liquid composition.\n", prefix);
				sprintf(cmd_string,"rm -r %s", prefix);
				system(cmd_string);
				continue;
			}
			// Can delete temp files after we've read them
			sprintf(cmd_string,"rm -r %s", prefix);
			system(cmd_string);

//			// Find built-in MELTS apatite saturation results
//			apatite=-1;
//			for (int i=0; i<minerals; i++){
//				printf("%s\t", names[i]);
//				if (strcmp(names[i],"apatite")==0){
//					apatite=i;
//				}
//			}
//			printf("\n");
//			if (apatite>0){
//				for (int col=0; col<meltscolumns[apatite]; col++) {
//					printf("%s\t", elements[apatite][col]);
//				}
//				printf("\n");
//				for (row=1; row<meltsrows[apatite]; row++){
//					printf("%g\t%g\n", melts[apatite][row][1], melts[apatite][row][2]);
//				}
//			}
			

			// Find the columns containing useful elements
			for(int col=0; col<meltscolumns[0]; col++){
				if (strcmp(elements[0][col], "Pressure")==0) P=col;
				else if (strcmp(elements[0][col], "Temperature")==0) T=col;
				else if (strcmp(elements[0][col], "mass")==0) mass=col;
				else if (strcmp(elements[0][col], "SiO2")==0) SiO2=col;
				else if (strcmp(elements[0][col], "TiO2")==0) TiO2=col;
				else if (strcmp(elements[0][col], "Al2O3")==0) Al2O3=col;
				else if (strcmp(elements[0][col], "Fe2O3")==0) Fe2O3=col;
				else if (strcmp(elements[0][col], "Cr2O3")==0) Cr2O3=col;
				else if (strcmp(elements[0][col], "FeO")==0) FeO=col;
				else if (strcmp(elements[0][col], "MnO")==0) MnO=col;
				else if (strcmp(elements[0][col], "MgO")==0) MgO=col;
				else if (strcmp(elements[0][col], "NiO")==0) NiO=col;
				else if (strcmp(elements[0][col], "CoO")==0) CoO=col;
				else if (strcmp(elements[0][col], "CaO")==0) CaO=col;
				else if (strcmp(elements[0][col], "Na2O")==0) Na2O=col;
				else if (strcmp(elements[0][col], "K2O")==0) K2O=col;
				else if (strcmp(elements[0][col], "P2O5")==0) P2O5=col;
				else if (strcmp(elements[0][col], "CO2")==0) CO2=col;
				else if (strcmp(elements[0][col], "H2O")==0) H2O=col;
			}

			// Calculate saturation temperature and minimum necessary zirconium content	
			Tsat=0;
			Tsatbulk = tapatite(melts[0][0][SiO2], melts[0][0][P2O5]);
			Tsmax = Tsatbulk;
			for(row=1; row<(meltsrows[0]-1); row++){
				//Calculate melt saturation for a given [SiO2] and [P2O5]
				Ts = tapatite(melts[0][row][SiO2], melts[0][row][P2O5]);

				// Keep track of maximum saturation temperature
				if (Ts > Tsmax){
					Tsmax = Ts;
				}

				// Check if we've cooled below the saturation temperature yet
				if (Tsat==0 && Ts > melts[0][row][T]){
					Tsat = Ts;
				}
				// Stop when we get to maximum SiO2
				if (melts[0][row-1][SiO2]>(melts[0][row][SiO2])+0.01){
					row--;
					break;
				}

				// Or when remaining melt falls below minimum percent
				if (melts[0][row][mass]<minPercentMelt){
					row--;
					break;
				}
			}
			// If apatite never saturated, check what the best (highest) saturation temperature was
			if (Tsat==0){
				Tsat = Tsmax;
			}

			//Check out the final saturation state
			P2O5f = melts[0][row][P2O5];
			Tf = melts[0][row][T];
			P2O5sat = tapatiteP2O5(melts[0][row][SiO2], Tf);

			// Determine how much apatite is saturated
			if (P2O5f>P2O5sat){
				MAp=melts[0][row][mass]/100*(P2O5f-P2O5sat);
			} else {
				MAp=0;
			}

			M = meltsM(&melts[0][0][SiO2]);
			// Print results. Format:
			// Kv, Mbulk, Tliquidus, Tsatbulk, Tf, Tsat, P2O5sat, P2O5f, Ff, SiO2, Zrbulk, MAp,
			printf("%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\t%g\n", ic[17], M, melts[0][0][T], Tsatbulk, Tf, Tsat, P2O5sat, P2O5f, melts[0][row][mass], melts[0][0][SiO2], melts[0][0][P2O5], MAp);
		}
	}
	MPI_Finalize();
	return 0;
}

