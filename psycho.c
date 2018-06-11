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
	//tworze struktur
	createJourneyType();
	
	//pobieram ilosc procesow MPI
	int localSize, localId;
 	MPI_Comm_size( MPI_COMM_WORLD, &localSize );
 	MPI_Comm_rank( MPI_COMM_WORLD, &localId);
	while(1){
		 int waittime = 1;
		 usleep(waittime);
		 
		//odbieram wiadomosc o stanie innego procesu. chec wejscia do podprzestrzeni, lub wyjscie z niej
		struct journey journeyLocal;
		MPI_Recv( &journeyLocal, 1, MPI_PAKIET, MPI_ANY_SOURCE, REQ_WYCIECZKA, MPI_COMM_WORLD, &status);
		printf("ID: %d	Zegar: %d W2	Odebralem wiadomosć od innego wątku. Nadawca: %d Zegar: %d  Rozmiar:%d status: %d \n",journeys[localSize].id,journeys[localSize].clock,  journeyLocal.id, journeyLocal.clock, journeyLocal.size, journeyLocal.status);
		
		//zwiekszam zegar zgodnie z lamportem 
		//BLOKADA MUTEX ZEGAR - ZAKLADAM BLOKADE NA ZMIANE WARTOSCI ZEGARA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		pthread_mutex_lock(&clockMutex);
			// printf("ID: %d	Zegar: %d W2	 MUTEX ZEGAR PODNIEŚ\n", journeys[localSize].id, journeys[localSize].clock);
			//zwiekszam moj zegar! journeys[localSize] - to wartosc mojego zegara
			int tmpClock = journeys[localSize].clock;
			int oldClock = tmpClock;
			if (journeyLocal.clock > tmpClock){
				tmpClock = journeyLocal.clock;
			}
			tmpClock +=1;
			journeys[localSize].clock=tmpClock;
			printf("ID: %d	Zegar: %d W2	Zwiekszam zegar. Nowy zegar: %d\n",journeys[localSize].id, oldClock, journeys[localSize].clock);
			// printf("ID: %d	Zegar: %d W2	MUTEX ZEGAR OPUŚĆ\n", journeys[localSize].id, journeys[localSize].clock);
		pthread_mutex_unlock(&clockMutex);	
		//BLOKADA MUTEX ZEGAR - ZDEJMUJE BLOKADE NA ZMIANE WARTOSCI ZEGARA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		
		
		
		//wstawiam wiadomosc ktora przyszla, do tablicy
		//BLOKADA MUTEX TABLICA -  ZALKADAM BLOKADE NA WSTAWIANIE DO TABLICY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		pthread_mutex_lock(&journeysMutex);
			// printf("ID: %d	Zegar: %d W2	MUTEX TABLICA PODNIEŚ\n", journeys[localSize].id, journeys[localSize].clock);
			journeys[journeyLocal.id] = journeyLocal;
			printf("ID: %d	Zegar: %d W2	zapisałem wycieczke w polu o id: Nadawca: %d	 Zegar: %d  Rozmiar:%d status: %d \n", journeys[localSize].id, journeys[localSize].clock, journeys[journeyLocal.id].id, journeys[journeyLocal.id].clock, journeys[journeyLocal.id].size, journeys[journeyLocal.id].status);
			// printf("ID: %d	Zegar: %d W2	 MUTEX TABLICA OPUŚĆ\n", journeys[localSize].id, journeys[localSize].clock);
		pthread_mutex_unlock(&journeysMutex);
		//BLOKADA MUTEX TABLICA- ZDEJMUJE BLOKADE NA WSTAWIANIE DO TABLICY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		
		
		
		//kwestia sporna :/ - odsyłamy strukture z czasem gdy chcielismy wejsc, czy strukture z czasem terazniejszym. obstawiam ze z momentu gdy chcemy wejsc
		//odsylam swoj status do nadawcy wiadomosci TAG REQ_ANSWER_WYCIECZKA, jesli była to wiadomosc o checi wejscia. jezeli wiadomosc byla o zwolnieniu sekcji krytycznej, to nei odsylam
		if( journeyLocal.status == STATUS_WANT){
			journey myJourney = journeys[localId];
			MPI_Send( &myJourney, 1, MPI_PAKIET, journeyLocal.id, REQ_ANSWER_WYCIECZKA, MPI_COMM_WORLD );
			printf("ID: %d	Zegar: %d W2	Odsyłam mój status do nadawcy wiadomości o id %d.\n",journeys[localSize].id, journeys[localSize].clock, journeyLocal.id);
		}

	}
}

// argv[1] - maksymalna liczba os. w wycieczce		M
// argv[2] - liczba psychokinetyków					N
// argv[3] - pojemność podprzestrzeni				P
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
    struct journey journeys[size+1];
    
    //wszystkim procesom ustawiam status na created
    /*
    pthread_mutex_lock(&journeysMutex);
	
   
    int z;
    for(z=0; z++;z<size){
		journeys[z].status = STATUS_CREATED;
		journeys[z].size = 0;
	}
	
	pthread_mutex_unlock(&journeysMutex);
	*/
	
	//inicjuje moja zmienna, przekazuje ja do tablicy na ostatnim indeksie. Zawsze ostatnia komorka tablicy, to lokalny stan danego procesu
    struct journey myJourney;
    myJourney.id = tid;
    myJourney.clock = clock;
    myJourney.size = W;
    myJourney.status = STATUS_CREATED;
    
    //wpisuje moja strukture do zegara
	//inicujuje przed stworzeniem 2 watku tablice, zeby mogl odpowiadac innym
	journeys[size] = myJourney;
	journeys[tid] = myJourney;
	
    pthread_t threadRec;
    pthread_create( &threadRec, NULL, parallelFunction, (void *)&journeys);

     while(1) { 
		int waittime = 1+(int) (100000.0*rand()/(RAND_MAX+1.0));
		
		//tworze typ wycieczka
		createJourneyType();
		
		//czekam losowy czas
		usleep(waittime);		
		printf("ID: %d	Zegar: %d W1	spałem: %d \n",journeys[size].id,journeys[size].clock, waittime);
		
		
		
		//stwarzam chec wycieczki, inkrementuje zegar
		//BLOKADA MUTEX ZEGAR - ZAKLADAM BLOKADE NA ZMIANE WARTOSCI ZEGARA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		pthread_mutex_lock(&clockMutex);
			// printf("ID: %d	Zegar: %d W1	MUTEX ZEGAR PODNIEŚ\n", journeys[size].id, journeys[size].clock);
			W = 1+(int) ((float)M*rand()/(RAND_MAX+1.0));
			journeys[size].clock += 1;
			journeys[size].size = W;
			journeys[size].status = STATUS_WANT;
			myJourney = journeys[size];
			printf("ID: %d	Zegar: %d W1	Zwiekszam zegar. Nowy zegar: %d\n",journeys[size].id, journeys[size].clock -1, journeys[size].clock);
			// printf("ID: %d	Zegar: %d W1	MUTEX ZEGAR OPUŚĆ\n", journeys[size].id, journeys[size].clock);
		pthread_mutex_unlock(&clockMutex);
		//BLOKADA MUTEX ZEGAR - ZDEJMUJE BLOKADE NA ZMIANE WARTOSCI ZEGARA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		
		//wkladam moja chec wycieczki do tablicy 
		//BLOKADA MUTEX TABLICA - ZAKLADAM BLOKADE NA WSTAWIANIE DO TABLICY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			pthread_mutex_lock(&journeysMutex);
				// printf("ID: %d	Zegar: %d W1	 MUTEX TABLICA PODNIEŚ\n", journeys[size].id, journeys[size].clock);
				journeys[tid] = myJourney;
		
				// printf("ID: %d	Zegar: %d W1	 MUTEX TABLICA OPUŚĆ\n", journeys[size].id, journeys[size].clock);
			pthread_mutex_unlock(&journeysMutex);
		//BLOKADA MUTEX TABLICA - ZDEJMUJE BLOKADE NA WSTAWIANIE DO TABLICY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		
		
		//wysyłam innym procesom, chec odbycua wycieczki TAG REQ_WYCIECZKA
		printf("ID: %d	Zegar: %d W1	Zmieniam status wycieczki na want rozmiar: %d\n", journeys[size].id, journeys[size].clock, journeys[size].size);
		myJourney = journeys[size];
		for (int i=0; i<size; i++) {
			if (myJourney.id != i) {
				MPI_Send( &myJourney, 1, MPI_PAKIET, i, REQ_WYCIECZKA, MPI_COMM_WORLD );
			}
		}
 		printf("ID: %d	Zegar: %d W1	Wyslalem wszystkim chec wyslania wycieczki do tunelu. Rozmiar:%d\n", myJourney.id, myJourney.clock, myJourney.size);
		
		
		
		//odbierz od wszystkich precosow, wiadomosci o ich stanie
		//size -1 ponieważ jest size procesow, a my odbieramy od wszystkich oprocz nas
 		for (int i=0; i<size-1; i++) {
 			struct journey tmpReciveJourney; 
 			//odbieram wiadomosc zwrotna  o ich stanie od innych procesow. TAG REQ_ANSWER_WYCIECZKA
 			MPI_Recv( &tmpReciveJourney, 1, MPI_PAKIET, MPI_ANY_SOURCE, REQ_ANSWER_WYCIECZKA, MPI_COMM_WORLD, &status);
 			printf("ID: %d	Zegar: %d W1	Odebralem odpowiedz odnosnie statusu. Nadawca: %d	 Zegar: %d  Rozmiar:%d status: %d\n",journeys[size].id,journeys[size].clock,  tmpReciveJourney.id, tmpReciveJourney.clock, tmpReciveJourney.size, tmpReciveJourney.status);
 			
 			//Inkrementuje zegar zgodnie z Lamportem
 			//BLOKADA MUTEX ZEGAR - ZAKLADAM BLOKADE NA ZMIANE WARTOSCI ZEGARA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 			pthread_mutex_lock(&clockMutex);
				// printf("ID: %d	Zegar:	%d W1	MUTEX ZEGAR PODNIEŚ\n", journeys[size].id, journeys[size].clock);
				int tmpClock = journeys[size].clock;
				int oldClock = tmpClock;
				if (tmpReciveJourney.clock > tmpClock){
					tmpClock = tmpReciveJourney.clock;
				}
				tmpClock +=1;
				journeys[size].clock=tmpClock;
				printf("ID: %d	Zegar: %d W1	Zwiekszam zegar. Nowy zegar: %d\n",journeys[size].id, oldClock, journeys[size].clock);
				// printf("ID: %d	Zegar: %d W1	MUTEX ZEGAR OPUŚĆ\n", journeys[size].id, journeys[size].clock);
			pthread_mutex_unlock(&clockMutex);
 			//BLOKADA MUTEX ZEGAR - ZDEJMUJE BLOKADE NA ZMIANE WARTOSCI ZEGARA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 			
 			
 			//wstawiam stan procesu ktory opowiedział, to tablicy
			//BLOKADA MUTEX TABLICA - ZAKLADAM BLOKADE NA WSTAWIANIE DO TABLICY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			pthread_mutex_lock(&journeysMutex);
				// printf("ID: %d	Zegar: %d W1	 MUTEX TABLICA PODNIEŚ\n", journeys[size].id, journeys[size].clock);
				
				
				//nie wiem jak to tu rozwiazac, czy wstawiac tylko tych ktorzy chca wejsc
					// 1 z 2 rozwiazan - uaktualniem kolejke, jeżeli nie dostałem wczesniej od niego wiadomosci ze chce wejsc. Jesli nie bylo takiej wiadomosci, to wstawiam
					
					 /*
					if(tmpReciveJourney.status == STATUS_WANT){
						if(journeys[tmpReciveJourney.id].status == STATUS_CREATED){
							journeys[tmpReciveJourney.id] = tmpReciveJourney;
						}
					}else{
						journeys[tmpReciveJourney.id] = tmpReciveJourney;
					}
					
					 */
					//2 z 2 rozwiazan
					journeys[tmpReciveJourney.id] = tmpReciveJourney;
				
				
				printf("ID: %d	Zegar: %d W1	Odebrałem odpowiedź na moje żądanie wysłania wycieczki do tunelu. Nadawca: %d	 Zegar: %d  Rozmiar:%d status: %d \n",journeys[size].id,journeys[size].clock,  tmpReciveJourney.id, tmpReciveJourney.clock, tmpReciveJourney.size, tmpReciveJourney.status);
				// printf("ID: %d	Zegar: %d W1	 MUTEX TABLICA OPUŚĆ\n", journeys[size].id, journeys[size].clock);
			pthread_mutex_unlock(&journeysMutex);
			//BLOKADA MUTEX TABLICA - ZDEJMUJE BLOKADE NA WSTAWIANIE DO TABLICY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		}
		
		
		//czekam aż zwolni się miejsce w przestrzeni. zakladam mutex, a nastepnie sprawdzam czy suma wycieczek od procesow w podprzestrzeni + od tych oczekujących o mniejszym zegarze + moich, jest mniejsza równa pojemnosci podprzestrzeni
		printf("ID: %d	Zegar: %d W1	Zaczynam oczekiwanie na wolne miejsce w podprzestrzeni\n", journeys[size].id, journeys[size].clock);
		int enter = 0;
		do{
			waittime = 10;
			usleep(waittime);
			//BLOKADA MUTEX TABLICA - ZAKLADAM BLOKADE NA WSTAWIANIE DO TABLICY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				pthread_mutex_lock(&journeysMutex);
					//printf("ID: %d	Zegar: %d W1	 MUTEX TABLICA PODNIEŚ\n", journeys[size].id, journeys[size].clock);
					
					
					//iteruje po każdym procesie. jeżeli ma mniejszy zegar lub id, i chce wejsc do podprzestrzeni to sumuje go do zmiennej 'insideSubspaceAndBeforeMe'
					int insideSubspaceAndBeforeMe = 0;
					int p;
					for(p = 0; p < size; p++){
						if (p != tid){
							if((journeys[p].status = STATUS_WANT )||( journeys[p].status = STATUS_INSIDE)){
								if( (journeys[p].clock < journeys[tid].clock) || ( (journeys[p].clock == journeys[tid].clock) && ( journeys[p].id < journeys[tid].id) ) ){
									insideSubspaceAndBeforeMe += journeys[p].size;
								}
							}
						}
					}
					printf("ID: %d	Zegar: %d W1	Przede mną w tunelu i oczekujące: %d osob. Rozmiar mojej wycieczki: %d, Pojemność tunelu: %d\n", journeys[size].id, journeys[size].clock, insideSubspaceAndBeforeMe, journeys[tid].size, P);
					
					//jeżeli zmienna 'insideSubspaceAndBeforeMe' jest mniejsza rowna pojemnosci podprzestrzeni, wchodze!
					if (insideSubspaceAndBeforeMe + journeys[tid].size <= P){
						journeys[tid].status = STATUS_INSIDE;
						printf("ID: %d	Zegar: %d W1	++++++++++++++++++++++++++++++++Wchodzę do podprzestrzeni :)+++++++++++++++++++++++++++++++++ Aktualnie w podprzestrzeni: %d + mój rozmiar: %d, razem: %d\n", journeys[size].id, journeys[size].clock,insideSubspaceAndBeforeMe ,journeys[tid].size, journeys[tid].size+insideSubspaceAndBeforeMe);
						enter = 1;
					}
					

					//printf("ID: %d	Zegar: %d W1	 MUTEX TABLICA OPUŚĆ\n", journeys[size].id, journeys[size].clock);
				pthread_mutex_unlock(&journeysMutex);
			//BLOKADA MUTEX TABLICA - ZDEJMUJE BLOKADE NA WSTAWIANIE DO TABLICY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		} while(enter == 0 );
		
		
		
		//czekaj randomowy czas
		 waittime = 1+(int) (100000.0*rand()/(RAND_MAX +1.0));
		usleep(waittime);
		
		//wychodze z podprzestrzeni
		//BLOKADA MUTEX ZEGAR - ZAKLADAM BLOKADE NA ZMIANE WARTOSCI ZEGARA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 			pthread_mutex_lock(&clockMutex);
				// printf("ID: %d	Zegar:	%d W1	MUTEX ZEGAR PODNIEŚ\n", journeys[size].id, journeys[size].clock);
				printf("ID: %d	Zegar: %d W1	Zwiekszam zegar.	Nowy zegar: %d\n",journeys[size].id, journeys[size].clock, journeys[size].clock+1);
				journeys[size].clock += 1;
				journeys[size].status = STATUS_CREATED;
				// printf("ID: %d	Zegar: %d W1	MUTEX ZEGAR OPUŚĆ\n", journeys[size].id, journeys[size].clock);
			pthread_mutex_unlock(&clockMutex);
 		//BLOKADA MUTEX ZEGAR - ZDEJMUJE BLOKADE NA ZMIANE WARTOSCI ZEGARA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 		
 		//BLOKADA MUTEX TABLICA - ZAKLADAM BLOKADE NA WSTAWIANIE DO TABLICY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			pthread_mutex_lock(&journeysMutex);
				// printf("ID: %d	Zegar: %d W1	 MUTEX TABLICA PODNIEŚ\n", journeys[size].id, journeys[size].clock);
				int count = 0;
				int p;
				for(p = 0; p < size; p++){
						
							if(journeys[p].status = STATUS_INSIDE){
								count += journeys[p].size;
							}
						
				}
			
				printf("ID: %d	Zegar: %d W1	-----------------------------------Opuszczam podprzestrzeń!----------------------------------------- aktualnie w podprzestrzeni: %d - mój rozmiar: %d, beze mnie :%d\n", journeys[size].id, journeys[size].clock,count, journeys[tid].size,count - journeys[tid].size);
				journeys[tid].status = STATUS_CREATED;
			// printf("ID: %d	Zegar: %d W1	 MUTEX TABLICA OPUŚĆ\n", journeys[size].id, journeys[size].clock);
			pthread_mutex_unlock(&journeysMutex);
		//BLOKADA MUTEX TABLICA - ZDEJMUJE BLOKADE NA WSTAWIANIE DO TABLICY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		
		//wysyłam innym procesom, zmiane statusu wycieczki, TAG REQ_ANSWER_WYCIECZKA. (WIADOMOSC IDZIE DO 2 WĄTKU PROCESU)
		printf("ID: %d	Zegar: %d W1	zmieniam na create Rozmiar: %d\n", journeys[size].id, journeys[size].clock, journeys[size].size);
		myJourney = journeys[size];
		for (int i=0; i<size; i++) {
			if (myJourney.id != i) {
				MPI_Send( &myJourney, 1, MPI_PAKIET, i, REQ_WYCIECZKA, MPI_COMM_WORLD );
			}
		}
	 }
    //srand( tid );
	usleep(100000);
    MPI_Finalize();
    
}
