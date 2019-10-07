#include "mraa.hpp"

#include <iostream>
using namespace std;

#define OBTENER_TEMP 11
#define OBTENER_MAX 21
#define OBTENER_MIN 31
#define OBTENER_PROM 41
#define OBTENER_TODO 51

#define PKG_START 0
#define PKG_DATA 1
#define PKG_SIZE 2

#define RESPONDER_TEMP 12
#define RESPONDER_MAX 22
#define RESPONDER_MIN 32
#define RESPONDER_PROM 42
#define RESPONDER_TODO 52


#define START_BYTE 0x01
#define END_BYTE 0x02

// Variables Globales
uint8_t pkg_send[4];
uint8_t pkg_receive[256];

// Inicializar led conectado a GPIO y controlador de I2C
// Comunicacion
mraa::Gpio* d_pin = NULL;
mraa::I2c* i2c;

int tieneRespuesta;

// Funciones
int obtener_operacion();
void armar_mensaje(int op);
void enviar_mensaje();
int validar_respuesta();
void imprimir_menu();
int opValido(uint8_t tipo);
void procesar_respuesta();

int main() {

	int operacion = 0;


	d_pin = new mraa::Gpio(3, true, true);

    i2c = new mraa::I2c(0);
    i2c->address(8);

	for(int i=0; i<256; i++) pkg_receive[i] = 0x00;

	imprimir_menu();
	printf("\nOperacion (h for help) > ");
	operacion = obtener_operacion();

    // Indefinidamente
    for (;;) {
    	if (operacion == 6 || operacion == 'h')
    		imprimir_menu();
    	else
    	{
			if (operacion > 0){		// if operacion valida

				armar_mensaje(operacion);

				enviar_mensaje();

				if(tieneRespuesta){
					if(validar_respuesta() < 0)
						printf("Invalid pkg receive \n");
					else
					{
						procesar_respuesta(); // Imprime por pantalla la respuesta
						// Luego de un segundo, encender led e imprimir por stdout
						sleep(1);
						d_pin->write(1);
					}
				}
			}
			else printf("Ingrese una operacion valida\n");
    	}
    	// Forzar la salida de stdout
    	printf("\nOperacion (h for help) > ");
    	fflush(stdout);
    	fflush(stdin);
    	operacion = obtener_operacion();
    }
    return 0;
}

int obtener_operacion(){
	char op;
	int res = scanf("%s",&op);
	if(res > 0){
		//Si esta entre 0 y 7
		if ((op > 48) && (op < 55)){
			tieneRespuesta = 1;
			return (int)(op - 48);
		}
		else{
			printf("Invalid Input \n");
		}
	}
	return (-1);
}

void armar_mensaje(int op){
	pkg_send[PKG_START] = START_BYTE; // Simbolo inicio
    switch (op){
    case 1:
        pkg_send[PKG_DATA] = OBTENER_TEMP;
    	break;
    case 2:
    	pkg_send[PKG_DATA] = OBTENER_MAX;
        break;
    case 3:
    	pkg_send[PKG_DATA] = OBTENER_MIN;
        break;
    case 4:
    	pkg_send[PKG_DATA] = OBTENER_PROM;
        break;
    case 5:
    	pkg_send[PKG_DATA] = OBTENER_TODO;
        break;

    case '?':
    	printf("Invalid Operation \n");
    	break;
    }
    pkg_send[PKG_SIZE] = 0x04;	// Size = 4by
    pkg_send[3] = END_BYTE; // Simbolo Fin
}

void enviar_mensaje(){
	// Apagar led y recibir por I2C
	sleep(1);
	d_pin->write(0);
	// Enviar mensaje
	i2c->write(pkg_send, pkg_send[2]);
}

int validar_respuesta(){
	//Chequea que se reciba una trama como se menciona a continuacion.
	// start|codigoOperacion|tama�o|dato|end
	pkg_receive[0] = i2c->readByte();//Lee el primer byte
	if(pkg_receive[0] == START_BYTE){
		pkg_receive[1] = i2c->readByte(); // codigo op
		if(pkg_receive[1] != END_BYTE && opValido(pkg_receive[1])){
			pkg_receive[2] = i2c->readByte(); // SIZE
			if(pkg_receive[2] > 4 && pkg_receive[2] < 256 && pkg_receive[2] != END_BYTE){
				int i = 2;
				while(pkg_receive[i] != END_BYTE && i < 256){
					i += 1;
					pkg_receive[i] = i2c->readByte();
				}
				if(i >= 256){
					printf("Error reading pkg: No END_BYTE found\n");
					return (-1);
				}
				return (1);
			} else printf("Error reading pkg: invalid SIZE\n");
		} else printf("Error reading pkg: invalid TYPE\n");
	} else printf("Error reading pkg: No START_BYTE found\n");
	return (-1);
}

int opValido(uint8_t tipo){
	return RESPONDER_TEMP == tipo || RESPONDER_MAX == tipo || RESPONDER_MIN == tipo || RESPONDER_PROM == tipo || RESPONDER_TODO == tipo;
}

void procesar_respuesta(){
	// La respuesta ya fue validad.
	// Longitud payload 3 to pkg_receive[2] - 4
	union{
	  float valorf;
	  int valorI;
	  uint8_t valorByte[4];
	} valorRespuesta;
	valorRespuesta.valorI = 0;
	int payloadSize = pkg_receive[2] - 4;
	switch(pkg_receive[1]){
		case RESPONDER_TEMP:
			for(int i=0; i < payloadSize; i++){
				valorRespuesta.valorByte[i] = pkg_receive[pkg_receive[2] - i - 2];
			}
			printf("Valor Temperatura actual: %d\n", valorRespuesta.valorI);
			break;
		case RESPONDER_MAX:
			for(int i=0; i < payloadSize; i++){
				valorRespuesta.valorByte[i] = pkg_receive[pkg_receive[2] - i - 2];
			}
			printf("Valor Temperatura maxima: %d\n", valorRespuesta.valorI);
			break;
		case RESPONDER_MIN:
			for(int i=0; i < payloadSize; i++){
				valorRespuesta.valorByte[i] = pkg_receive[pkg_receive[2] - i - 2];
			}
			printf("Valor Temperatura minima: %d\n", valorRespuesta.valorI);
			break;
		case RESPONDER_PROM:
			for(int i=0; i < payloadSize; i++){
				valorRespuesta.valorByte[i] = pkg_receive[pkg_receive[2] - i - 2];
			}
			printf("Valor Temperatura promedio: %f\n", valorRespuesta.valorf);
			break;
		case RESPONDER_TODO:
			for(int i=0; i < 4; i++){
				valorRespuesta.valorByte[i] = pkg_receive[pkg_receive[2] - i - 2];
			}
			printf("Valor Temperatura promedio: %f\n", valorRespuesta.valorf);
			valorRespuesta.valorI = 0;
			for(int i=0; i < 2; i++){
				valorRespuesta.valorByte[i] = pkg_receive[pkg_receive[2] - i - 6];
			}
			printf("Valor Temperatura minima: %d\n", valorRespuesta.valorI);
			for(int i=0; i < 2; i++){
				valorRespuesta.valorByte[i] = pkg_receive[pkg_receive[2] - i - 8];
			}
			printf("Valor Temperatura maxima: %d\n", valorRespuesta.valorI);
			for(int i=0; i < 2; i++){
				valorRespuesta.valorByte[i] = pkg_receive[pkg_receive[2] - i - 10];
			}
			printf("Valor Temperatura actual: %d\n", valorRespuesta.valorI);
			break;
	}
}

void imprimir_menu(){
	printf("______Operaciones de Temperatura_____\n");
	printf("1) Obtener Temperatura Actual \n");
	printf("2) Obtener Temperatura Maxima \n");
	printf("3) Obtener Temperatura Minima \n");
	printf("4) Obtener Temperatura Promedio \n");
	printf("5) Obtener Todo \n");
}

