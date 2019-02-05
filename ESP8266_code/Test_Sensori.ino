#include <ESP8266Ping.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
//Programma che si occupa della gestione dei sensori a ultrasuoni per controllare l'attraversamento di una persona.
//variabili che definiscono i pin dei sensori per la lettura e scrittura 
const int trigPin1 = 12;
const int echoPin1 = 5;
const int trigPin2 = 4;
const int echoPin2 = 14;
const int Pin = 13;
const int sPin = 10;
const int niter = 5;      //numero di volte che viene effettuata la lettura dai sensori
const int maxit = 20;     //numero massimo di iterazioni prima di resettare i valori dei sensori
//variabili che indicano se il sensore 1 o il sensore 2 hanno avuto un riscontro positivo
boolean out1;             
boolean out2;
//variabili che indicano se è stata effettuata un'entrata o un'uscita
boolean flag1;
boolean flag2;
int counter;              //contatore di ciclo
int stato;                //stato in cui si trova il programma
//variabili che indicano i valori base dei sensori
int baseD1 = 80; //110
int baseD2 = 80; //110
#define tm 15000          //timeout per l'emissione del pulsein()
#define deltaLow 20          //valore che indica l'errore relativo massimo accettato rispetto al valore base
#define deltaHigh 200     //.....
//variabili che indicano i valori di SSID e password per la connessione wifi
const char* ssid = "SiberNet";
const char* password = "fragoleAndroid";
char hostString[16] = {0};//variabile che serve per la ricezione del multicast DNS
unsigned int tcpPort;     //porta tcp per la connessione
IPAddress ip;             //indirizzo ip per la connessione
char outPacket[6];        //paccheto da inviare
WiFiClient client;        //variabile per instanziare una connessione tcp
//varibili che indicano i valori globali di entrate e uscite registrati dall'ultimo invio
int globalE = 0;          
int globalU = 0;
boolean stF = false;              //valore che indica se il multicast DNS ha avuto riscontro positivo
boolean tcpConn = false;          //valore che indica se la connessione tcp ha avuto successo
boolean retry = false;    //valore che indica la presenza di errori nella connessione, usata per la riconfigurazione

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
//Funzione utilizzata per inviare dati tramite la connessione tcp, a indica se si vuole indicare un'entrata o un'uscita, Restituisce 1 se l'invio ha successo 0 altrimenti. 
int SendData(int a){     
        Serial.println("sending data to server");
        //viene controllato se il dispositivo master è ancora raggiungibile tramite ping e se la connessione e ancora attiva
        if (client.connected() && Ping.ping(ip,2)){
              //in base al valore di a si modificano i valori del pacchetto da inviare
              if (a == 1) {
                  outPacket[4] = 0x01;
                  outPacket[5] = globalE;
                  globalE = 0;
              }else{
                  outPacket[4] = 0x02;
                  outPacket[5] = globalU;
                  globalU = 0;
              }
              //invio del pacchettto
              client.write(outPacket, 6);
              Serial.println("sent");
              return 1;
           }else{
              return 0;
           } 
}
//Funzione che si occcupa di aprire la connessione tcp con un host. Restituisce 1 se la connessione è avvenuta 0 altrimenti.
int ConnForStream(){
    int cc = 0;     //contatore che indica il numero di prove eseguite
    //si prova ad aprire la connessione masssimo 3 volte
    while (!client.connect(ip, tcpPort) && cc <2) {
        Serial.println("connection failed");
        delay(1000);
        cc ++;
      }
    if (cc < 3) return 1;
    else return 0;
}
//Funzione che si occupa di cercare il segnale multicast DNS. Restituisce 1 se si è trovato il segnale dall'host indicato 0 altrimenti.
int FindStation(){
  int cnt = 0;      //contatore che indica il numero di prove effettuate;
  boolean cr = false;   //variabile che indica l'effettivo ritrovamento del segnale giusto
  while (cr == false && cnt<2){
      Serial.println("Sending mDNS query");
      int n = MDNS.queryService("custom", "tcp"); // Send out query for mdns tcp services
      Serial.println("mDNS query done");
      //viene controllato il valore di n che indica il numero di segnali trovati
      if (n == 0) {
        Serial.println("no services found");
        cnt++;
      } else {
        Serial.print(n);
        Serial.println(" service(s) found");
        //per ogni segnale trovato viene controllato se l'hostname è quello che cercavamo e nel caso vengono salvati i valori di IP e porta
        for (int i = 0; i < n; ++i) {
          // Print details for each service found
          if (MDNS.hostname(i)=="UNIBO.local"){
              Serial.print(i + 1);
              Serial.print(": ");
              Serial.print(MDNS.hostname(i));
              Serial.print(" (");
              Serial.print(MDNS.IP(i));
              ip = MDNS.IP(i);
              Serial.print(":");
              Serial.print(MDNS.port(i));
              tcpPort = MDNS.port(i);
              Serial.println(")");
              cr = true;
              break;
          }else if(i == n) cnt++;   //quessto serve nel caso ci siano altri segnali mdns ma di host che non sono quelli che si cercavano
        }
      }
  delay(1000);
  }
  if (cr == true) return 1;
  else return 0;
}

void setup() {
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(9600);
  //istruzioni per l'iniziazione della connessione wifi
  WiFi.mode(WIFI_STA);
  // the below instructions and now u will receive.
  wifi_set_sleep_type(NONE_SLEEP_T);
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
  Serial.println("wifi-connesso");
  Serial.println(WiFi.localIP());
  //valori del pacchetto da inviare
  outPacket[0] = 0xFE;
  outPacket[1] = 0xFF;
  outPacket[2] = 0x02;
  //outPacket[3] = 0x00;
  pinMode(Pin, INPUT); //Sets the Pin as Input
  pinMode(trigPin1, OUTPUT); // Sets the trigPin1 as an Output
  pinMode(echoPin1, INPUT); // Sets the echoPin1 as an Input
  pinMode(trigPin2, OUTPUT); // Sets the trigPin2 as an Output
  pinMode(echoPin2, INPUT); // Sets the echoPin2 as an Input
  pinMode(sPin, INPUT_PULLUP); //Sets the sPin as inpunt with pullup
  if (digitalRead(sPin)==LOW) outPacket[3] = 0x00;
  else outPacket[3] = 0x01;
  Serial.println(digitalRead(sPin), DEC);
  stato = 0;
  counter = 0;
  flag1 = false;
  flag2 = false;
  //istruzioni per poter leggere segnali multicast DNS
  WiFi.hostname(hostString);    
    if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  //Se viene trovato il segnale allora si tenta subito di aprire la connessione tcp
  if (FindStation()==1){
    stF = true;
    tcpConn = ConnForStream();
  }
  else stF = false;
   
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
  boolean nv = true;
  boolean nv2 = true;
  //switch contenente gli stati del programma
  switch(stato){
    //stato 0 è lo stato iniziale in cui la porta è chiusa e in cui si rimane finchè non viene aperta
    case 0:
      //controllo se la porta è aperta
      if (digitalRead(Pin) == 0){
        stato = 1;
        break;
      }
      //tentativo di riconnessione in caso di errori di rete
      if (stF == false || tcpConn == false){
          Serial.println("tentativo di riconnessione");
          stF = false;
          tcpConn = false;
          if  (WiFi.status() == WL_CONNECTED){
              stF = FindStation();
              if (stF) tcpConn = ConnForStream();
              else tcpConn = false;
              if (tcpConn){
                if (globalE>0) nv = SendData(1);
                if (globalU>0) nv2 = SendData(2);
              }
              if (nv == false || nv2 == false) tcpConn = false;
          }
      }
      break;
    //stato 1 è lo stato iniziale in cui viene effettuata la lettura dai sensori ripetutamente finchè non viene trovato un risultato positivo  
    case 1:
      //controllo se la porta è chiusa
      if (digitalRead(Pin) == 1){
        stato = 0;
        break;
      }
      val1 = 0;
      val2 = 0;
      j = 0;
      z = 0;
      while(j < niter){
        distance = getDistance(trigPin1, echoPin1);
        if (distance > baseD1+deltaHigh || distance<baseD1-deltaLow) val1++;   //se il valore della distanza del sensore 1 è cambiato allora val1 viene aumentato
        delay(1);
        j++;
      }
      delay(1);
      while(z < niter){
        distance = getDistance(trigPin2, echoPin2);
        if (distance > baseD2+deltaHigh || distance<baseD2-deltaLow) val2++;   //se il valore della distanza del sensore 2 è cambiato allora val2 viene aumentato
        delay(1);
        z++;
      }
      delay(1);
      //se il numero di valori positivi è sufficiente allora viene settato la variabile out relativa a true indicando un valore positivo per quel sensore
      if (val1>=niter/2)  out1 = true;
      if (val2>=niter/2)  out2 = true;
      //se almeno uno dei sensori a ottenuto un valore positivo allora viene cambiato lo stato in cui si trova il programma
      if(out1 || out2) stato = 2;
      break;
    //stato 2 è lo stato in cui almeno uno dei due sensori ha avuto un valore positivo e si aspetta di ricevere un valore positivo dall'altro oppure si resetta lo stato del programma 
    case 2:
      //se il contatore raggiunge il massimo valore di interazioni definito allora viene resettato il valore delle variabili globali e si torna allo stato 0
      if (digitalRead(Pin) == 1){
        stato = 0;
        break;
      }
      if (counter>=maxit){
        out1=false;
        out2=false;
        Serial.println("reset");
        stato = 1;
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
          //Serial.println(distance);
          if (distance > baseD2+deltaHigh || distance<baseD2-deltaLow) val2++;
          delay(1);
          j++;
        }
        //se anche il secondo sensore da un risultato positivo allora si segnala un avvenuto ingresso e si cambia lo stato del programma
        if (val2>=niter/2){
          flag1=true;
          stato = 3;
        }
      //caso in cui il secondo sensore ha dato un riscontro positivo  
      }else{
        counter++;
        val1 = 0;
        j = 0;
        //lettura del primo sensore alla ricerca di un valore positivo
        while(j < niter){
          distance = getDistance(trigPin1, echoPin1);
          //Serial.println(distance);
          if (distance > baseD1+deltaHigh || distance<baseD1-deltaLow) val1++;
          delay(1);
          j++;
        }
        //se anche il primo sensore da un risultato positivo allora si segnala un'avvenuta uscita e si cambia lo stato del programma
        if (val1>=niter/2){
          flag2=true;
          stato = 3;
        }
      }
      break;
    //stato 3 è lo stato in cui è avvenuta un entrata o uscita e si cerca di avvisare la station principale dell'evento, inoltre si resetta il valore delle variabili globali e lo stato del sistema dopo aver controllato che il valore dei sensori sia tornato a quello base 
    case 3:
      //viene inviato il valore di entrate o uscite in base a quale evento è avvenuto
      if (flag1){
          Serial.println("entrata");
          globalE++;
          if (WiFi.status() == WL_CONNECTED && stF == true && tcpConn == true){
             tcpConn = SendData(1);
             //Serial.println(tcpConn);
          }
      }
      if (flag2){ 
          Serial.println("uscita");
          globalU++;
          if (WiFi.status() == WL_CONNECTED && stF == true && tcpConn == true){
            tcpConn = SendData(2);
            //Serial.println(tcpConn);
          }
      }
      counter=0;
      while(counter < 3){
        //Serial.println("fermo nel reset");
        j = 0;
        z = 0;
        val1 = 0;
        val2 = 0;
        //lettura dal sensore 1
        while(j < niter){
          distance = getDistance(trigPin1, echoPin1);
          if (distance > baseD1+deltaHigh || distance<baseD1-deltaLow) val1++;
          delay(1);
          j++;
        }
        //lettura dal sensore 2
        while(z < niter){
          distance = getDistance(trigPin2, echoPin2);
          if (distance > baseD2+deltaHigh || distance<baseD2-deltaLow) val2++;
          delay(1);
          z++;
        }
        //controllo se il numero di valori diversi da quello base è minore di una certa soglia
        if (val1<niter/4) out1 = false;
        if (val2<niter/4) out2 = false;
        if (!out1 && !out2) counter++;
        out1 = true;
        out2 = true;
      }
      //reset delle variabili globali e cambio dello stato a stato 1
      counter = 0;
      flag1=false;
      flag2=false;
      out1 = false;
      out2 = false;
      stato = 1;
      break; 
  }
}
