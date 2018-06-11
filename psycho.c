#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#define ROOT 0
#define MSG_TAG 100
#define REQ_WYCIECZKA 101
#define REQ_ANSWER_WYCIECZKA 102
#define STATUS_CREATED 1000
#define STATUS_WANT 1001
#define STATUS_INSIDE 1002

MPI_Datatype MPI_PAKIET;
MPI_Status status; 

typedef struct journey
{
    int id;
    int clock;
    int size;
    int status;
} journey;

void createJourneyType() {
	const int nitems=4;
    	int blocklengths[4] = {1,1,1,1};
    	MPI_Datatype typy[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
  		MPI_Aint offsets[4];
		offsets[0] = offsetof(journey, id);
		offsets[1] = offsetof(journey, clock);
		offsets[2] = offsetof(journey, size);
		offsets[3] = offsetof(journey, status);
		MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PAKIET);
		MPI_Type_commit(&MPI_PAKIET);
}

void *parallelFunction(journey *journeys) {
	createJourneyType();
	int localSize;

	// journey * tmpMyJourney = myJourney;

 	MPI_Comm_size( MPI_COMM_WORLD, &localSize );

	//  printf("ID: %d	Zegar: %d	Odebralem chec wyslania wycieczki do tunelu.Nadawca:	%d	Zegar: %d	Rozmiar:%d\n", tmpMyJourney->id, tmpMyJourney->clock, tmpMyJourney->size);
	
	struct journey journey;

    journey.id = 0;
    journey.clock = 0;
    journey.size = 0;
    journey.status = STATUS_CREATED;
	// TODO: nie wiem jak wyłuskać liczbę elementów struktury hehe
	// size_t n = sizeof(journeys)/sizeof(&journeys[0]);
		// printf("po n: %d", n);

	// for (int i; i<n; i++) {
		// printf("odebranaz");
		// journeys[i] = journey;
	// }
	// printf("ID: %d	Zegar: %d	zapisałem wycieczke w polu o id: Nadawca: %d	 Zegar: %d  Rozmiar:%d status: %d\n", journeys[0].id, journeys[0].clock, journeys[0].id, journeys[0].clock, journeys[0].size, journeys[0].status);



	while(1){
		 int waittime = 1;
		 usleep(waittime);

		MPI_Recv( &journey, 1, MPI_PAKIET, MPI_ANY_SOURCE, REQ_WYCIECZKA, MPI_COMM_WORLD, &status);
		printf("Odebralem chec wyslania wycieczki do tunelu. Nadawca: %d	 Zegar: %d  Rozmiar:%d status: %d\n", journey.id, journey.clock, journey.size, journey.status);

	// 	//TODO: mutex na tablice wycieczek
	// 	//addOrUpdate() - zaktualizuj albo dodaj wartości do lokalnej tablicy wycieczek
	}
}



// argv[1] - maksymalna liczba os. w wycieczce
// argv[2] - liczba psychokinetyków
// argv[3] - pojemność podprzestrzeni
int main(int argc,char **argv) {
    int size, tid;
    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int P = atoi(argv[3]);
    int provided;
 
    MPI_Init_thread( &argc, &argv, MPI_THREAD_MULTIPLE, &provided );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &tid );
    printf("podane argumenty: M = %d N = %d P = %d  size = %d\n", M, N, P, size);

    srand(time(NULL));

    int clock = tid;
    int W = 0;
    struct journey journeys[size];

    struct journey myJourney;
    myJourney.id = tid;
    myJourney.clock = clock;
    myJourney.size = W;
    myJourney.status = STATUS_CREATED;

    pthread_t threadRec;
    pthread_create( &threadRec, NULL, parallelFunction, (void *)&journeys);

     while(1) { 
		int waittime = (int)(rand()%100000);
		W = (int)(rand()%10000);

		createJourneyType();

		usleep(waittime);

		// printf("ID: %d	Zegar: %d	wycieczka w main id: Nadawca: %d	 Zegar: %d  Rozmiar:%d status: %d\n", journeys[0].id, journeys[0].clock, journeys[0].id, journeys[0].clock, journeys[0].size, journeys[0].status);

		printf("ID: %d	Zegar: %d spałem: %d \n",myJourney.id,myJourney.clock, waittime);
		clock += 1;
    	myJourney.size = W;
		myJourney.clock = clock;
		myJourney.status = STATUS_WANT;
		printf("ID: %d	Zegar: %d zmieniam na want Rozmiar: %d\n", myJourney.id, myJourney.clock, myJourney.size);

		for (int i=0; i<size; i++) {
			if (myJourney.id != i) {
				MPI_Send( &myJourney, 1, MPI_PAKIET, i, REQ_WYCIECZKA, MPI_COMM_WORLD );
			}
		}

		

// 			printf("ID: %d	Zegar: %d	Wyslalem wszystkim chec wyslania wycieczki do tunelu. Rozmiar:%d\n", myJourney.id, myJourney.clock, myJourney.size);

// 		for (int i=0; i<size-1; i++) {
// 			struct journey tmpReciveJourney; 
// 			MPI_Recv( &tmpReciveJourney, 1, MPI_PAKIET, MPI_ANY_SOURCE, REQ_ANSWER_WYCIECZKA, MPI_COMM_WORLD, &status);
// 			if(tmpReciveJourney.needToEnter){ 
// 				journeys[busy] = tmpReciveJourney;
// 				busy ++;
// 				sort(journeys, busy);
// 			}

// 		}
		
// 	//}
	
//     //}
    
	 }
    //srand( tid );
	usleep(1000000);
    MPI_Finalize();
}
