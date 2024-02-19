#include "eth.h"

/*
 * CONSTRUCTOR QUE GUARDA EL PIN RESET Y PIN CS DEL WIZ5500, EL LED INDICADOR QUE HAY CONEXION CON EL SERVIDOR MQTT, EL CONSTRUCTOR CLIENT Y ...
 * ... LA CANTIDAD DE CUARTOS QUE SE HA REGISTRADO.
 */

Eth::Eth(int pinRST, int pinCS, int mqttConnLed, PubSubClient* Client, int nRooms):
_pinRST(pinRST),
_pinCS(pinCS),
_mqttConnLed(mqttConnLed),
client(*Client),
_nRooms(nRooms){}

/*
 * REALIZA LA CONEXION CON EL SERVIDOR MQTT DE LA EMPRESA, ADEMAS SE SUSCRIBE A TODO LOS TOPICOS MQTT
 */

void Eth::reconnect(void){
  while (!client.connected()) {
    Serial.println("Intentando conexion MQTT");
    String clientId = "WNR_";
    clientId = clientId + String(random(0xffff), HEX);

    char *str[] = {"INGRESO", "APAGAR", "LUZ-TECHO",
                    "LUZ-CABECERA", "LUZ-MESA", "LUZ-NCABINA"};                     //SE ESCRIBE TODO LOS TOPICOS MQTT QUE SE QUIERAN SUSCRIBIR
    
    if (client.connect(clientId.c_str(),"billeteronv11","billeterownr123")){
      Serial.println("Conexion a MQTT exitosa!!");
      digitalWrite(_mqttConnLed, HIGH);

      client.subscribe("/SEDE2/MB/LUCES/ALIVE/");
      client.subscribe("/SEDE2/MB/LUCES/PUERTA/");

      for (int i=0; i<_nRooms; i++){                                                
        for (int j=0; j<6; j++){                                                    //J<# -> EL # DEBE SER EL TAMAÑO DEL ARRAY DE STR[]
          char buf[30];
          if (i<9){
            snprintf(buf, 30, "/SEDE2/CABINA0%d/%s",i+1, str[j]);
          }
          else{
            snprintf(buf, 30, "/SEDE2/CABINA%d/%s",i+1, str[j]);
          }
          Serial.println(buf);
          client.subscribe(buf);
        }
      }   
    } 
    else{
      digitalWrite(_mqttConnLed, LOW);
      Serial.print("Fallo la conexion, rc=");
      Serial.print(client.state());
      Serial.println(" Se intentara denuevo en 2 segundos");
      delay(2000);
    }
  }
}

/*
 * REALIZA LA CONEXION DHCP CON INTERNET, POR ENDE, SE UTILIZA IP DINAMICA PARA MAYOR FACILIDAD
 * IMPORTANTE!!! PARA QUE FUNCIONE EL WIZ5500 SE NECESITA QUE SE LE RESETEE POR HARDWARE, COMO SE OBSERVA EN EL CODIGO DE ABAJO ...
 * ... LE ENVIO UN PULSO ALTO POR SU PIN RESET, DESPUES DE ESTO RECIEN PUEDO HACER LA CONEXION DHCP A INTERNET
 */

void Eth::initConn(byte mac[]){
  pinMode(_mqttConnLed, OUTPUT);
  
  pinMode(_pinRST, OUTPUT);
  
  digitalWrite(_pinRST, LOW);

  delay(200);

  digitalWrite(_pinRST, HIGH);

  Ethernet.init(_pinCS);                                                  //ETHERNET CS

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
  }
  else{
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
  }

  delay(1000);
}

/*
 * ENVIA EL NUMERO DEL CUARTO QUE PRESIONO EL BOTON DE PANICO (EL ENVIO ES POR MQTT Y EL TOPICO ESTA AHI)
 */

void Eth::publish_panic(int room_id){
  char buf[24];
  if(room_id<10)
    snprintf(buf, 24, "/SEDE2/CABINA0%d/PANICO", room_id);
  else
    snprintf(buf, 24, "/SEDE2/CABINA%d/PANICO", room_id);
  client.publish(buf, "true");
}
