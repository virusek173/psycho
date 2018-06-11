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

pthread_mutex_t clockMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t journeysMutex = PTHREAD_MUTEX_INITIALIZER;

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
	int size;
 	MPI_Comm_size( MPI_COMM_WORLD, &size );
	struct journey journey;

	while(1){
		int waittime = 1;
		usleep(waittime);
		MPI_Recv( &journey, 1, MPI_PAKIET, MPI_ANY_SOURCE, REQ_WYCIECZKA, MPI_COMM_WORLD, &status);
		printf("ID: %d	Zegar: %d W2 Odebralem wiadomość wycieczkę do tunelu. Nadawca: %d	 Zegar: %d  Rozmiar:%d status: %d\n", journeys[size].id, journeys[size].clock, journeys[journey.id].id, journeys[journey.id].clock, journeys[journey.id].size, journeys[journey.id].status);

		// Inkrementuje zegar zgodnie z Lamportem
		int tmpClock = journeys[size].clock;
		int oldClock = tmpClock;
		if (journey.clock > tmpClock){
			tmpClock = journey.clock;
		}
		tmpClock +=1;
		printf("ID: %d	Zegar: %d W2 Zwiekszam zegar. Nowy zegar: %d\n",journeys[size].id, oldClock, journeys[size].clock);

		pthread_mutex_lock(&journeysMutex);
		journeys[size].clock = tmpClock;
		journeys[journey.id] = journey;
		pthread_mutex_unlock(&journeysMutex);
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

    srand(tid);

    int clock = tid;
    int W = 0;
    struct journey journeys[size + 1];

    struct journey myJourney;

	myJourney.id = tid;
	myJourney.clock = clock;
	myJourney.size = W;
	myJourney.status = STATUS_CREATED;

	//inicujuje przed stworzeniem 2 watku tablice, zeby mogl odpowiadac innym
	// for (int i=0; i<size; i++) {
		// journeys[i] = myJourney;
	// }

    pthread_t threadRec;
    pthread_create( &threadRec, NULL, parallelFunction, (void *)&journeys);

     while(1) { 
		int waittime = (int)(rand()%100000);
		W = (int)(rand()%10000);
		createJourneyType();

		myJourney.id = tid;
		myJourney.clock = clock;
		myJourney.size = W;
		myJourney.status = STATUS_CREATED;
		printf("ID: %d	Zegar: %d W1 ustawiam status na created Rozmiar: %d\n", myJourney.id, myJourney.clock, myJourney.size);

		journeys[size] = myJourney; // ostatni index to nasza aktualna struktura

		for (int i=0; i<size; i++) {
			if (myJourney.id != i) {
				MPI_Send( &myJourney, 1, MPI_PAKIET, i, REQ_WYCIECZKA, MPI_COMM_WORLD );
			}
		}
		printf("ID: %d	Zegar: %d W1 Wysłałem wszystkim stats created\n", myJourney.id, myJourney.clock);


		usleep(waittime);
		printf("ID: %d	Zegar: %d W1 spałem: %d \n",myJourney.id,myJourney.clock, waittime);
		
		clock += 1;
    	myJourney.size = W;
		myJourney.clock = clock;
		myJourney.status = STATUS_WANT;
		journeys[size] = myJourney;
		printf("ID: %d	Zegar: %d W1 zmieniam status na want Rozmiar: %d\n", myJourney.id, myJourney.clock, myJourney.size);

		for (int i=0; i<size; i++) {
			if (myJourney.id != i) {
				MPI_Send( &myJourney, 1, MPI_PAKIET, i, REQ_WYCIECZKA, MPI_COMM_WORLD );
			}
		}
		printf("ID: %d	Zegar: %d W1 Wysłałem wszystkim status want\n", myJourney.id, myJourney.clock);
	 }

	usleep(1000000);
    MPI_Finalize();
}
