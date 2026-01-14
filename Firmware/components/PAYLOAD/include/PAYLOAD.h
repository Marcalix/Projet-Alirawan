#ifndef PAYLOAD_H_
#define PAYLOAD_H_

// Appel unique des fichiers
#pragma once

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque standard :::::::::::::::::::::::::::::::::::::::::::: */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "arpa/inet.h" 
#include <sys/_intsup.h>

/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque specifiques :::::::::::::::::::::::::::::::::::::::::::: */


/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Macros :::::::::::::::::::::::::::::::::::::::::::::: */                 


/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
              /* :::::::::::::::::::::::::::::::::::::::::::: Structure::::::::::::::::::::::::::::::::::::::::::::::::::::: */
 /**
 * @brief Chaîne modifiable adossée à un buffer externe.
 *
 * Représente une vue sur un tampon de caractères (potentiellement '\0'-terminé),
 * avec sa longueur utile et sa capacité totale.
 * Invariants recommandés : data != NULL, size < max_size,
 * et si utilisé comme C-string, data[size] == '\0'.
 */
typedef struct str16_t{
    char sentence[16]; // buffer (nul-terminé si utilisé comme C-string)
} str16_t;

// --------------------------------------------------------------------------------------------------------------------------------------
 /**
 * @brief Chaîne modifiable adossée à un buffer externe.
 *
 * Représente une vue sur un tampon de caractères (potentiellement '\0'-terminé),
 * avec sa longueur utile et sa capacité totale.
 * Invariants recommandés : data != NULL, size < max_size,
 * et si utilisé comme C-string, data[size] == '\0'.
 */
typedef struct str256_t{
    char sentence[256]; // buffer (nul-terminé si utilisé comme C-string)
}str256_t;

// --------------------------------------------------------------------------------------------------------------------------------------            
typedef struct Payload_t{
   
   str256_t (*custom_encode)(double latitude, double longitude,  int8_t rssi, int8_t snr);
   
   str256_t (*custom_decode)(char *payload);  
    
} Payload_t;

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */

void Payload_build(Payload_t *payload);

#ifdef __cplusplus
}
#endif
#endif /* PAYLOAD_H_ */
