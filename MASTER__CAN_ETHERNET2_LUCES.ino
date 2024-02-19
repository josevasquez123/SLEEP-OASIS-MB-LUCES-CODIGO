#include "can_.h"
#include "eth.h"

#define CAN_INT                   20                                              //MCP2515 INT EXT PIN
#define CAN_CS                    10                                              //MCP2515 CS PIN
#define ETH_RST                   42                                              //WIZ5500 RST PIN
#define ETH_CS                    40                                              //WIZ5500 CS PIN
#define N_ROOMS                   26                                              //NUMERO DE CUARTOS
#define MQTT_CONN_LED             32                                              //LED INDICADOR DE CONEXION EXITOSA A MQTT  
#define PUERTA_PRINCIPAL_RELAY    46                                              //RELAY QUE ABRE/CIERRA LA PUERTA PRINCIPAL DE LA SEDE                               
#define MB_ID                     0x001B0000                                      //ID DE ESTE MB

void callback(char* topic, byte* payload, unsigned int length);
void check_income_msg(void);

IPAddress mqtt_server(34,207,154,79);
EthernetClient eth_client;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
PubSubClient client(mqtt_server,1883,callback,eth_client);  

CanMCP canMCP(CAN_CS, CAN_INT);
Eth ethWIZ(ETH_RST, ETH_CS, MQTT_CONN_LED, &client, N_ROOMS);

void setup() {

  pinMode(PUERTA_PRINCIPAL_RELAY, OUTPUT);

  Serial.begin(115200);

  canMCP.init();

  canMCP.rooms_register(N_ROOMS);

  canMCP.filter_ids(MB_ID);

  ethWIZ.initConn(mac);
}

void loop() {

  if (ethWIZ.client.connected() == false){
    ethWIZ.reconnect();
  }

  ethWIZ.client.loop();

  canMCP.send_rooms_data();

  check_income_msg();
}

void callback(char* topic, byte* payload, unsigned int length){
  payload[length] = 0;
  String topico=String(topic);
  String msg = String((char *)payload);

  if(topico == "/SEDE2/MB/LUCES/ALIVE/"){
    ethWIZ.client.publish("/SEDE2/MB/LUCES/ALIVE/ACK/", "true");
  }
  
  if(topico == "/SEDE2/MB/LUCES/PUERTA/"){
    digitalWrite(PUERTA_PRINCIPAL_RELAY, HIGH);
    delay(4000);
    digitalWrite(PUERTA_PRINCIPAL_RELAY, LOW);
  }

  int cabina = atoi(&topic[13]);                                    //OBTIENE EL NUMERO DE CABINA

  Serial.println(topico);
  Serial.println(msg);

  canMCP.set_CAN_msg(canMCP.rooms.get(cabina-1), cabina-1, String(&topic[16]), msg);
}

void check_income_msg(void){
  if(!digitalRead(canMCP.intPin)){                                    
    long unsigned int rxID;
    unsigned char len = 0;
    unsigned char rxBuf[4];
    char room_id_temp[1];
    
    canMCP.readMsg(&rxID, &len, rxBuf);

    for(int i = 0; i < len; i++) {
      Serial.print("rxBuf[");
      Serial.print(i);
      Serial.print("]: ");
      Serial.println(rxBuf[i]);
    }

    if(rxBuf[0] == 0x08){
      ethWIZ.publish_panic(rxBuf[1]);
    }

    if(rxBuf[0] == 0x09){
      sprintf(room_id_temp, "%c", rxBuf[1]);
      Serial.println("ACK CAN RECIBIDO");
      ethWIZ.client.publish("/SEDE2/LUCES/CABINA/", room_id_temp);
    }

    rxBuf[0] = 0;
    rxBuf[1] = 0;
  }
}
