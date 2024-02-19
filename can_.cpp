#include "can_.h"

/*
 * CONSTRUCTOR QUE ASIGNA EL PIN CS DEL MCP215 Y EL PIN DE INTERRUPCION EXTERNA
 */

CanMCP::CanMCP(int pinCS, int pinInt):_can(pinCS), intPin(pinInt){}

/*
 * INICIALIZA EL MCP2515 A UNA TASA DE ENVIO DE 500KBPS Y ESPECIFICA QUE SE UTILIZA UN CRISTAL DE 8MHZ
 */

void CanMCP::init(void){
  pinMode(intPin, INPUT);

  if(_can.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) Serial.print("MCP2515 Init Okay!!\r\n");     
  else Serial.print("MCP2515 Init Failed!!\r\n");

  _can.setMode(MCP_NORMAL);
}

/*
 * ENVIA DATOS POR CAN, COMO ESTAN EN POOL SE NECESITA UNA BANDERA QUE AVISE CUANDO LLEGO UN NUEVO DATO (BANDERA ES "arr[0]")
 * EL PROTOCOLO DE ENVIO ES [<PRENDER/APAGAR>,<CODIGO DEL OBJETO>]
 * AL FINAL SE LIMPIA LA BANDERA Y SE GUARDA EN LA LISTA ROOMS (REESCRIBE EL MISMO STRUCT EN LA LISTA)
 */

void CanMCP::send_data(int room_number,struct canStructure data){  
  if(data.arr[3]){                                                                     //BANDERA QUE DETERMINA SI LLEGO UN NUEVO MENSAJE O NO
      byte msg[2] = {data.arr[2], data.arr[1]};                                        //msg = {PRENDER/APAGAR, CODIGO DEL OBJETO}
      byte sndStat = _can.sendMsgBuf(data.arr[0], 0, 2, msg);                          //sndStat = {DIR CABINA CAN, CAN_MODE, MSG_LENGTH, DATA}    
      if(sndStat == CAN_OK){
        //send_status_MQTT(arr[2], arr[1], arr[3]);
        Serial.println("Message Sent Successfully!");
      } else {
        //send_error_status_MQTT(arr[2], arr[3]);
        Serial.println("Error Sending Message...");
      }
      data.arr[3] = 0x00;                                                               //LIMPIA BANDERA
      rooms.set(room_number, data);                                                     //REESCRIBE EL ROOM ANTERIOR CON LA BANDERA LIMPIA
    }
}

/*
 * ENVIO DATOS A TODOS LOS CUARTOS 
 */

void CanMCP::send_rooms_data(void){
  for (int i=0; i<_nRooms; i++){
    send_data(i, rooms.get(i));
  }
}

/*
 * FILTRA EL ID DEL MENSAJE PARA QUE SOLO RECIBA MENSAJES CON EL ID QUE SE PONE
 */

void CanMCP::filter_ids(uint32_t id){
  const uint32_t myMask = 0x07FF0000;                         // where to look at:                
  const uint32_t myFilter = 0x0000000 | id;                   // what to find:                    
  _can.init_Mask(0, 0, myMask);                               // Init first mask
  _can.init_Mask(1, 0, myMask);                               // Init first filter
  _can.init_Filt(0, 0, myFilter);                             // Init second mask - must be set,
}

/*
 * CREA LAS VARIABLES PARA CADA UNA DE LOS CUARTOS, ADEMAS REGISTRA LA DIRECCION DE CUARTO EN CADA VARIABLE
 * LA ESTRUCTURA S1 ES UN ARRAY DE 4 BYTES = {<DIRECCION DEL CUARTO>, <CODIGO DEL OBJETO>, <PRENDER O APAGAR EL OBJETO>, <BANDERA DE ENVIO DE DATO>}
 * DIRECCION DEL CUARTO = NUMERO DEL CUARTO (1, 2, 3, 4, ETC.)
 * CODIGO DEL OBJETO = NUMERO QUE ABREVIA EL OBJETO A CONTROLAR. EJ: LUZ DE LA MESA = 0X03
 * PRENDER O APAGAR OBJETO = SI ES 1 SIGNIFICA QUE PRENDE OBJETO, SI ES 0 APAGA EL OBJETO
 * BANDERA = BANDERA QUE SE PONE EN 1 CUANDO LE LLEGO UN MENSAJE MQTT A CIERTO CUARTO. EJ: LE LLEGA UN MENSAJE POR MQTT DE ACTIVAR CUARTO 5 LA LUZ...
 * ... ENTONCES EN LA FUNCION CALLBACK DE LA LIBRERIA ETH, SE RECORRERA POR LA LISTA ROOMS Y BUSCARA LA DIRECCION 5, CUANDO LO ENCUENTRE ESCRIBIRA 1 ...
 * ... EN EL CUARTO BYTE DEL ARRAY.
 */

 /* CODIGO DE OBJETOS
 * 
 * INGRESO: LA CABINA PASA A UN ESTADO OCUPADO Y SE DEBE PRENDER LA LUZ TECHO Y NUMERO DE CABINA -> 0x01
 * APAGAR: APAGAR TODA LAS LUCES CUANDO LA CABINA ESTE EN UN ESTADO DIFERENTE A OCUPADO -> 0x02
 * PUERTA: ABRE O CIERRA CANTONERA -> 0x03
 * LUZ-TECHO: PRENDE O APAGA LUZ TECHO -> 0x04
 * LUZ-CABECERA: PRENDE O APAGA LUZ CABECERA -> 0x05
 * LUZ-MESA: PRENDE O APAGA LUZ MESA -> 0x06
 * LUZ-NCABINA: PRENDE O APAGA LA LUZ DEL NUMERO DE CABINA -> 0x07
 * BOTON PANICO: LA CABINA ENVIA UN MENSAJE PIDIENDO AYUDA DEL PERSONAL -> 0X08
 * ACK: CADA CABINA LE ENVIA UN ACK AL MB CADA VEZ QUE LE LLEGA UN COMANDO, ESTA ES LA FORMA DE SABER SI ESA PLACA SIGUE FUNCIONANDO -> 0X09
 */

void CanMCP::rooms_register(int number_rooms){
  _nRooms = number_rooms;
  for(int i=1; i<number_rooms+1; i++){
    struct canStructure s1;
    s1.arr[0] = i;                                              //ASIGNANDO DIRECCIONES AL PRIMER BYTE DEL ARRAY
    rooms.add(s1);
  }
}

/*
 * SETEA LOS VALORES DEL STRUCT DE UN ROOM
 * PROTOCOLO -> ARR[4] = {<DIR DEL ROOM>, <CODIGO OBJETO>, <PRENDER/APGAR>, <BANDERA>}
 */

void CanMCP::set_CAN_msg(struct canStructure data, int room_number, String objeto, String msg){
  data.arr[3] = 0x01;
  
  if(objeto == "INGRESO"){
    data.arr[1] = 0x01;
  }
  else if(objeto == "APAGAR"){
    data.arr[1] = 0x02;
  }
  //else if(objeto == "PUERTA"){
    //data.arr[1] = 0x03;
  //}
  else if(objeto == "LUZ-TECHO"){
    data.arr[1] = 0x04;
  }
  else if(objeto == "LUZ-CABECERA"){
    data.arr[1] = 0x05;
  }
  else if(objeto == "LUZ-MESA"){
    data.arr[1] = 0x06;
  }
  else if(objeto == "LUZ-NCABINA"){
    data.arr[1] = 0x07;
  }
  if(msg == "true"){
    data.arr[2] = 0x01;
  }
  else if(msg == "false"){
    data.arr[2] = 0x00;
  }
  rooms.set(room_number, data);
}

/*
 * LEE MENSAJE DEL BUFFER CAN
 */

void CanMCP::readMsg(long unsigned int *id, unsigned char *len, unsigned char *buf){
  _can.readMsgBuf(id, len, buf);
}
