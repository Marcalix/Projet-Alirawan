#ifndef MAIN_I2C_H_
#define MAIN_I2C_H_

#pragma once

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque generales :::::::::::::::::::::::::::::::::::::::::::: */
#include <stdint.h>
#include <string.h>
#include <sys/_intsup.h>

/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque specifiques :::::::::::::::::::::::::::::::::::::::::::: */
#include "esp_timer.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "driver/i2s_common.h"
#include "soc/gpio_num.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Macros :::::::::::::::::::::::::::::::::::::::::::::: */
#define I2C_SCAN_NUM_MAX 4	// Nombre maximal d’adresses I²C que l’on souhaite détecter lors du scan

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables globale :::::::::::::::::::::::::::::::::::::::::::::: */
extern i2c_master_bus_handle_t bus_handle_0;  // Handle pour le bus I2C “numéro 0”
extern i2c_master_bus_handle_t bus_handle_1;  // Handle pour le bus I2C “numéro 1”

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions :::::::::::::::::::::::::::::::::::::::::::::: */
/**
 * @brief   Initialise un bus I²C en mode “master” (maître).
 *
 * @param   port      Choix du bus : false → I2C_NUM_0, true → I2C_NUM_1
 * @param   sda_io    Numéro de GPIO à utiliser pour SDA
 * @param   scl_io    Numéro de GPIO à utiliser pour SCL
 * @return  true si l’initialisation a réussi, false sinon.
 */
bool i2c_master_init(bool port, gpio_num_t sda_io, gpio_num_t scl_io);


/**
 * @brief   Ajoute et configure un périphérique I²C sur un bus déjà initialisé.
 *
 * @param   port        Choix du bus : false → bus_handle_0 (I2C_NUM_0), true → bus_handle_1 (I2C_NUM_1)
 * @param   address     Adresse 7 bits de l’esclave I²C (0x00–0x7F)
 * @param   frequence   Fréquence (en Hz) de communication souhaitée pour ce périphérique
 * @param   dev_handle  Pointeur vers i2c_master_dev_handle_t où sera stocké le handle du périphérique créé
 * @return  true si l’ajout/configuration du périphérique a réussi (ESP_OK), false sinon
 */
bool i2c_conf_device(bool port, uint8_t address, uint32_t frequence, i2c_master_dev_handle_t *dev_handle);


/**
 * @brief   Scanne un bus I²C pour détecter les adresses d’esclaves actifs.
 *
 * Cette fonction balaie les adresses I²C valides (de 0x03 à 0x76) sur le bus
 * spécifié (I2C_NUM_0 si port==0, sinon I2C_NUM_1) en appelant i2c_master_probe
 * sur chaque adresse. Si un esclave répond (ACK), son adresse est stockée dans
 * un tableau statique. Le dernier élément de ce tableau sert de compteur du nombre
 * d’adresses trouvées. Une fois terminé (ou si le tableau atteint I2C_SCAN_NUM_MAX),
 * le tableau est retourné. Le format du tableau renvoyé est :
 *   - indices [0..n-1] : adresses 7 bits des esclaves détectés,
 *   - index [I2C_SCAN_NUM_MAX] : nombre total d’adresses détectées.
 *
 * @param   port  Choix du bus I²C : 0 pour bus_handle_0 (I2C_NUM_0), 1 pour bus_handle_1 (I2C_NUM_1).
 * @return  Pointeur vers un uint8_t[] statique contenant les adresses trouvées et le compteur.
 *          find_addr[I2C_SCAN_NUM_MAX] contient le nombre d’adresses détectées.
 */
uint8_t *i2c_scan(bool port);


/**
 * @brief   Supprime (déinitialise) un bus I²C maître précédemment créé.
 *
 * Cette fonction appelle i2c_del_master_bus sur le handle du bus spécifié
 * (bus_handle_0 si port==0, sinon bus_handle_1). Si la suppression réussit
 * (retourne ESP_OK), la fonction renvoie true, sinon false.
 *
 * @param   port  Choix du bus : 0 pour bus_handle_0 (I2C_NUM_0), 1 pour bus_handle_1 (I2C_NUM_1).
 * @return  true si le bus a été correctement supprimé (ESP_OK), false sinon.
 */
bool i2c_master_deinit(bool port);

/**
 * @brief   Envoie une chaîne de caractères en I²C vers un esclave.
 *
 * Cette fonction utilise i2c_master_transmit pour transmettre un buffer de
 * données au format C-string (tableau de char se terminant par '\0'). Elle
 * envoie exactement strlen(data) octets (sans compter le caractère nul final),
 * et retourne true si la transmission a réussi (ACK reçu par l’esclave),
 * false sinon.
 *
 * @param   dev_handle  : handle du périphérique I²C obtenu via i2c_master_bus_add_device
 * @param   data        : pointeur vers la chaîne de caractères à envoyer (ASCII, sans '\0')
 * @param   data_s		: taille de la donnée à ecrire sur le bus 
 * @param   timeout_ms  : timeout maximal (en millisecondes) pour la transaction I²C
 * @return  true si la fonction i2c_master_transmit renvoie ESP_OK (succès I²C), false sinon
 */
bool i2c_writeString(i2c_master_dev_handle_t dev_handle, char *data, size_t data_s, uint32_t timeout_ms);

/**
 * @brief Lecture de données depuis un esclave I2C.
 * 
 * Cette fonction lit une séquence de `data_s` octets à partir du périphérique I2C associé à `dev_handle`,
 * et stocke le résultat dans le tableau `data`.
 * 
 * @param dev_handle    Handle du périphérique I2C.
 * @param data          Pointeur vers le tampon de réception.
 * @param data_s        Nombre d’octets à lire.
 * @param timeout_ms    Délai d’attente en millisecondes pour la transaction.
 * 
 * @return true         Si la lecture a réussi (ACK reçu).
 * @return false        En cas d’erreur (ex : pas de périphérique à cette adresse).
 */
bool i2c_writeBytes(i2c_master_dev_handle_t dev_handle, uint8_t *data, size_t data_s, uint32_t timeout_ms);

/**
 * @brief   Lit une séquence d’octets depuis un esclave I²C dans un buffer.
 *
 * Cette fonction utilise i2c_master_receive pour récupérer des données en lecture
 * depuis un esclave I²C, et place ces octets dans le buffer pointé par `data`.
 * Elle appelle i2c_master_receive avec comme longueur le résultat de strlen(data),
 * c’est-à-dire le nombre de caractères avant le premier caractère nul dans `data`.
 *
 * @param   dev_handle   :Handle du périphérique I²C (issu de i2c_master_bus_add_device)
 * @param   data         :Pointeur vers le buffer où seront stockés les octets lus.
 *                       Doit contenir au moins strlen(data) octets d’espace utile.
 * @param   data_s		 :taille de la donnée à lire sur le bus
 * @param   timeout_ms   :Timeout maximal (en millisecondes) pour la transaction I²C.
 * @return  true si i2c_master_receive renvoie ESP_OK (lecture réussie), false sinon.
 */
bool i2c_readString(i2c_master_dev_handle_t dev_handle, char *data, size_t data_s, uint32_t timeout_ms);

/**
 * @brief Écriture de données vers un esclave I2C.
 * 
 * Cette fonction envoie une séquence de `data_s` octets du tableau `data` vers le périphérique I2C 
 * associé à `dev_handle`.
 * 
 * @param dev_handle    Handle du périphérique I2C.
 * @param data          Pointeur vers le tampon de données à envoyer.
 * @param data_s        Nombre d’octets à écrire.
 * @param timeout_ms    Délai d’attente en millisecondes pour la transaction.
 * 
 * @return true         Si l’écriture a réussi (ACK reçu).
 * @return false        En cas d’erreur (ex : périphérique non présent).
 */
bool i2c_readBytes(i2c_master_dev_handle_t dev_handle, uint8_t *data, size_t data_s, uint32_t timeout_ms);

/**
 * @brief   Effectue une transaction I²C combinée "write-then-read" vers un esclave.
 *
 * Cette fonction envoie d’abord une séquence d’octets (cmd) à l’esclave, puis
 * lit immédiatement une séquence d’octets en réponse, le tout dans une même
 * transaction I²C (START → WRITE(cmd) → REPEATED START → READ(data) → STOP).
 *
 * @param   dev_handle   :Handle du périphérique I²C (issu de i2c_master_bus_add_device).
 * @param   cmd          :Chaîne de caractères C (ASCII) à transmettre en écriture.
 *                       Seuls les octets avant '\0' (strlen(cmd) octets) seront envoyés.
 * @param   cmd_s		 :taille de la commande à envoyer sur le bus 
 * @param   data         :Pointeur vers le buffer où seront stockés les octets lus.
 *                       On tentera de lire strlen(data)-1 octets. Le dernier octet de `data`
 *                       reste généralement réservé pour le caractère nul '\0'.
 * @param  data_s		 :taille de la donnée à lire sur le bus
 * @param   timeout_ms   :Timeout maximal (en millisecondes) pour l’ensemble de la transaction
 *                       (WRITE + READ).
 * @return  true si i2c_master_transmit_receive renvoie ESP_OK (transaction réussie), false sinon.
 */
bool i2c_writeCommand(i2c_master_dev_handle_t dev_handle, char *cmd, size_t cmd_s, char *data, size_t data_s, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_I2C_H_ */

