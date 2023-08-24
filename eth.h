#ifndef eth
#define eth

#include "Arduino.h"
#include <Ethernet.h>
#include <PubSubClient.h>

class Eth{
  public:
    Eth(int pinRST, int pinCS, int mqttConnLed,PubSubClient* Client, int nRooms);
    void initConn(byte mac[]);
    void reconnect(void);
    void publish_panic(int room_id);

  public:
    PubSubClient client;
    
  private:
    int _pinRST;
    int _pinCS;
    int _nRooms;
    int _mqttConnLed;
};

#endif
