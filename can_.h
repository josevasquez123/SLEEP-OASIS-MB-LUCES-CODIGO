#ifndef can_
#define can_

#include "Arduino.h"
#include <mcp_can.h>
#include <LinkedList.h>

/*
 * PORFAVOR LEER
 * SE HA CREADO UNA ESTRUCTURA DE DATOS PARA PODER ALMACENAR LOS VALORES DE TODO LOS CUARTOS QUE SE QUIERA REGISTRAR.
 * LA ESTRUCTURA ES UNA LISTA<STRUCT>, LA CUAL EN SIMPLES PALABRAS ES UNA LISTA DE ARRAYS DE 4 BYTES ... 
 * ... CADA VEZ QUE AGREGO UN STRUCT A LA LISTA SIGNIFICA QUE AGREGO LAS VARIABLES DE UN CUARTO.
 * LA FUNCION ROOMS_REGISTER REGISTRA TODO LOS CUARTOS QUE QUIERAS, POR ENDE, LA VARIABLE NUMBER_ROOMS ES EL VALOR DE CUARTOS.
 * LO QUE HACE ES CREAR STRUCTS Y AGREGARLOS A LA LISTA CON LA DIFERENCIA QUE EL PRIMER BYTE DEL ARRAY DE TU STRUCT ES LA DIRECCION DEL CUARTO...
 * ... EJ: CUARTO 1 -> struct canStructure s1; s1.arr[0] = 1; ESTO LO HACE AUTOMATICO EN UN LOOP Y CON EL PRIMER BYTE YO IDENTIFICO LAS VARIABLES...
 * ... DE LA LISTA
 */

struct canStructure{
  byte arr[4];
};

class CanMCP
{
  public:
    CanMCP(int pinCS, int pinInt);
    void init(void);
    void rooms_register(int number_rooms);
    void send_data(int room_number, struct canStructure data);
    void filter_ids(uint32_t id);
    void send_rooms_data(void);
    void set_CAN_msg(struct canStructure data, int room_number, String objeto, String msg);
    void readMsg(long unsigned int *id, unsigned char *len, unsigned char *buf);

  public:
    LinkedList<canStructure> rooms;
    int intPin;                                       //pin unido al can_int del mcp2515

  private:
    MCP_CAN _can;                                     //Constructor de la libreria mcp_can
    int _nRooms;
};

#endif
