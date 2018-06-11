mpicc -o psycho psycho.c 

mpirun -np 2 psycho 1 2 3

# Opis działania algorytmu:
parametry:
M - maksymalna liczba osób w wycieczce
N - liczba psychokinetyków
P - pojemność podprzestrzeni

zasoby lokalne:
ID - id procesu
ZEGAR - aktualna wartość zegara Lamporta
W - rozmiar wycieczki
wycieczka - lokalna struktura wycieczki psychokinetyka
wycieczki - wektor przechowujący listy (ZEGAR,ID,WYCIECZKA,STATUS). posortowany.
wycieczka w tablice wycieczki przyjmuje następujące statusy: 'created', 'need', 'inside'

algorytm(1watek):
	zeruj zegar
	pętla nieskończona
	{
		losuj rozmiar wycieczki(W) z przedziału(1,M).

        ZEGAR+=1
        wyślij do innych psychokinetyków informacje o uworzeniu wycieczki o rozmiarze W,z tagiem REQ_WYCIECZKA - status 'created'
        
		czekaj lokalnie losowy czas

        ZEGAR+=1
		wyślij do innych psychokinetyków chęć wysłania wycieczki o rozmiarze W, z tagiem REQ_WYCIECZKA - status 'need'
        
        do() {
            jeśli aktywna blokada na lokalną tablicę wycieczek to czekaj
            blokada na lokalną tablicę wycieczek {
                zlicz ile miejsc zajmują wycieczki ze statusem 'inside'. i przypisz do zmiennej sumInside
                zsumuj rozmiary wycieczek ze statusem 'need' i większym zegarem Lamporta przypisz do zmiennej sumNeed
            }
        } while(nie mieści do podprzestrzeni - 
                sumNeed + sumInside + W =< P)

        zmień status wycieczki na 'inside' i wyśli informacje do innych psychokinetyków tagiem REQ_WYCIECZKA - status 'inside'
        czekaj losowy czas
	}
	
(2watek)
	odebranie wiadomości z tagiem REQ_WYCIECZKA
    jeśli aktywna blokada na lokalną tablicę wycieczek to czekaj
	blokada na lokalną tablicę wycieczek {
	    dodaj lub zaktualizuj tablicy wycieczek, wartosci które przyszły (ZEGAR,ID,WYCIECZKA,STATUS)
    }
	ZEGAR+=1
	