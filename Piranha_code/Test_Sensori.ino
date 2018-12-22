
//Programma che si occupa della gestione dei sensori a ultrasuoni per controllare l'attraversamento di una persona.
//variabili che definiscono i pin dei sensori per la lettura e scrittura 
const int trigPin1 = 0;
const int echoPin1 = 1;
const int trigPin2 = 2;
const int echoPin2 = 3;
const int niter = 5;      //variabile che indiaca il numero di volte che viene effettuata la lettura dai sensori
const int maxit = 20;     //variabile che indica il numero massimo di iterazioni prima di resettare i valori dei sensori
//variabili che indicano se il sensore 1 o il sensore 2 hanno avuto un riscontro positivo
boolean out1;             
boolean out2;
//variabili che indicano se è stata effettuata un'entrata o un'uscita
boolean flag1;
boolean flag2;
int counter;              //variabile usata come contatore di ciclo
int stato;                //variabile di stato che indica lo stato in cui si trova il programma
//variabili che indicano i valori base dei sensori
int baseD1;
int baseD2;
#define campIt 300        //valore di interazioni per il campionamento
#define tm 15000          //timeout per l'emissione del pulsein()
#define delta 20          //valore che indica l'errore relativo massimo accettato rispetto al valore base
/*
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#define TCPPORT 8080;
#define UDPPORT 6060;

void conn(){
  int socket_sensor, new_socket, valread;
  struct sockaddr_in address; 
  int opt = 1; 
  int addrlen = sizeof(address); 
  char buffer[1024] = {0}; 
  
  // Creating socket file descriptor 
  if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    }
    // Forcefully attaching socket to the port 6060 
    if (setsockopt(socket_sensor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( UDPPORT );
    
    if (bind(socket_sensor, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    if (listen(server_fd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  
                       (socklen_t*)&addrlen))<0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 
    valread = read( new_socket , buffer, 1024); 
    printf("%s\n",buffer ); 
    
}

void snd(){
    struct sockaddr_in address; 
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    char *hello = "Hello from client"; 
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    memset(&serv_addr, '0', sizeof(serv_addr)); 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(TCPPORT);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 
    send(sock , hello , strlen(hello) , 0 ); 
    printf("Hello message sent\n"); 
    valread = read( sock , buffer, 1024); 
    printf("%s\n",buffer );
}
*/
//Funzione utilizzata per trovare la distanza a cui si trova un ostacolo, tPin e ePin indicano i due pin del sensori a ultrasuoni da usare. Restituisce la distanza in cm. 
int getDistance(int tPin, int ePin){
     digitalWrite(tPin, LOW);
     delayMicroseconds(2);
     // Sets the trigPin on HIGH state for 10 micro seconds
     digitalWrite(tPin, HIGH);
     delayMicroseconds(10);
     digitalWrite(tPin, LOW);
     // Reads the echoPin, returns the sound wave travel time in microseconds
     long duration = pulseIn(ePin, HIGH, tm);
     // Calculating the distance
     int distance = duration*0.034/2;
     return distance;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(trigPin1, OUTPUT); // Sets the trigPin1 as an Output
  pinMode(echoPin1, INPUT); // Sets the echoPin1 as an Input
  pinMode(trigPin2, OUTPUT); // Sets the trigPin2 as an Output
  pinMode(echoPin2, INPUT); // Sets the echoPin2 as an Input
  stato = 0;
  counter = 0;
  flag1 = false;
  flag2 = false;
  long somma = 0;   //variabile usata per sommare tutti i valori delle distanze
  for (int i=0; i<campIt;i++){
    somma = somma + getDistance(trigPin1, echoPin1);  
    delay(1);      
  }
  baseD1 = somma / campIt;  //il valore base viene ottenuto dalla media dei valori delle distanze ottenuti
  somma = 0;
  for (int j=0; j<campIt;j++){
     somma = somma + getDistance(trigPin2, echoPin2);
     delay(1);
  }
  baseD2 = somma / campIt;  
}

void loop() {
  // put your main code here:
  //variabili usate per contare il numero di valori positivi nella lettura dai sensori
  int val1;
  int val2;
  //variabili usate come contatori
  int j = 0;
  int z = 0;
  //variabile contenente la distanza in cm
  int distance;
  //switch contenente gli stati del programma
  switch(stato){
    //stato 0 è lo stato iniziale in cui viene effettuata la lettura dai sensori ripetutamente finchè non viene trovato un risultato positivo
    case 0:
      val1 = 0;
      val2 = 0;
      j = 0;
      z = 0;
      while(j < niter){
        distance = getDistance(trigPin1, echoPin1);
        if (distance > baseD1+delta || distance<baseD1-delta) val1++;   //se il valore della distanza del sensore 1 è cambiato allora val1 viene aumentato
        delay(1);
        j++;
      }
      delay(1);
      while(z < niter){
        distance = getDistance(trigPin2, echoPin2);
        if (distance > baseD2+delta || distance<baseD2-delta) val2++;   //se il valore della distanza del sensore 2 è cambiato allora val2 viene aumentato
        delay(1);
        z++;
      }
      delay(1);
      //se il numero di valori positivi è sufficiente allora viene settato la variabile out relativa a true indicando un valore positivo per quel sensore
      if (val1>=niter/2)  out1 = true;
      if (val2>=niter/2)  out2 = true;
      //se almeno uno dei sensori a ottenuto un valore positivo allora viene cambiato lo stato in cui si trova il programma
      if(out1 || out2) stato = 1;
      break;
    //stato 1 è lo stato in cui almeno uno dei due sensori ha avuto un valore positivo e si aspetta di ricevere un valore positivo dall'altro oppure si resetta lo stato del programma 
    case 1:
      //se il contatore raggiunge il massimo valore di interazioni definito allora viene resettato il valore delle variabili globali e si torna allo stato 0
      if (counter>=maxit){
        out1=false;
        out2=false;
        //Serial.println("reset");
        stato = 0;
        counter = 0;
      }
      //caso in cui il primo sensore ha dato un riscontro positivo
      if (out1){
        counter++;
        val2 = 0;
        j = 0;
        //lettura del secondo sensore alla ricerca di un valore positivo
        while(j < niter){
          distance = getDistance(trigPin2, echoPin2);
          if (distance > baseD2+delta || distance<baseD2-delta) val2++;
          delay(1);
          j++;
        }
        //se anche il secondo sensore da un risultato positivo allora si segnala un avvenuto ingresso e si cambia lo stato del programma
        if (val2>=niter/2){
          flag1=true;
          stato = 2;
        }
      //caso in cui il secondo sensore ha dato un riscontro positivo  
      }else{
        counter++;
        val1 = 0;
        j = 0;
        //lettura del primo sensore alla ricerca di un valore positivo
        while(j < niter){
          distance = getDistance(trigPin1, echoPin1);
          if (distance > baseD1+delta || distance<baseD1-delta) val1++;
          delay(1);
          j++;
        }
        //se anche il primo sensore da un risultato positivo allora si segnala un'avvenuta uscita e si cambia lo stato del programma
        if (val1>=niter/2){
          flag2=true;
          stato = 2;
        }
      }
      break;
    //stato 2 è lo stato in cui è avvenuta un entrata o uscita e si resetta il valore delle variabili globali e lo stato del sistema dopo aver controllato che il valore dei sensori sia tornato a quello base 
    case 2:
      //viene stampato entrata o uscita in base a quale evento è avvenuto
      if (flag1) Serial.println("entrata");
      if (flag2) Serial.println("uscita");
      counter=0;
      while(counter < 1){
        j = 0;
        z = 0;
        val1 = 0;
        val2 = 0;
        //lettura dal sensore 1
        while(j < niter){
          distance = getDistance(trigPin1, echoPin1);
          if (distance > baseD1+delta || distance<baseD1-delta) val1++;
          delay(1);
          j++;
        }
        //lettura dal sensore 2
        while(z < niter){
          distance = getDistance(trigPin2, echoPin2);
          if (distance > baseD2+delta || distance<baseD2-delta) val2++;
          delay(1);
          z++;
        }
        //controllo se il numero di valori diversi da quello base è minore di una certa soglia
        if (val1<niter/4) out1 = false;
        if (val2<niter/4) out2 = false;
        if (!out1 && !out2) counter++;
        else counter = 0;
      }
      //reset delle variabili globali e cambio dello stato a stato iniziale
      counter = 0;
      //Serial.println("out");
      flag1=false;
      flag2=false;
      stato = 0;
      break;  
  }
}
