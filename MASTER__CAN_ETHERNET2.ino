#include "can_.h"
#include "eth.h"

#define CAN_INT       20                                              //MCP2515 INT EXT PIN
#define CAN_CS        10                                              //MCP2515 CS PIN
#define ETH_RST       42                                              //WIZ5500 RST PIN
#define ETH_CS        40                                              //WIZ5500 CS PIN
#define N_ROOMS       22                                              //NUMERO DE CUARTOS
#define MQTT_CONN_LED 32                                              //LED INDICADOR DE CONEXION EXITOSA A MQTT                                 
#define MB_ID         0x00190000                                      //ID DE ESTE MB

void callback(char* topic, byte* payload, unsigned int length);
void check_income_msg(void);

IPAddress mqtt_server(100,25,168,182);
EthernetClient eth_client;
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
PubSubClient client(mqtt_server,1883,callback,eth_client);  

CanMCP canMCP(CAN_CS, CAN_INT);
Eth ethWIZ(ETH_RST, ETH_CS, MQTT_CONN_LED, &client, N_ROOMS);

void setup() {

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
  
  int cabina = atoi(&topic[13]);                                    //OBTIENE EL NUMERO DE CABINA

  Serial.println(topico);
  Serial.println(msg);

  canMCP.set_CAN_msg(canMCP.rooms.get(cabina-1), cabina-1, String(&topic[16]), msg);
}

void check_income_msg(void){
  if(!digitalRead(canMCP.intPin)){                                    
    long unsigned int rxID;
    unsigned char len = 0;
    unsigned char rxBuf[2];
    
    canMCP.readMsg(&rxID, &len, rxBuf);
    
    if(rxBuf[0] == 0x09){
      ethWIZ.publish_panic(rxBuf[1]);
      rxBuf[0] = 0;
      rxBuf[1] = 0;
    }
  }
}
