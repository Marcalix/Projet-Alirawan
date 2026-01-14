#ifndef MY_LORA_H_
#define MY_LORA_H_

// Appel unique des fichiers
#pragma once

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque standard :::::::::::::::::::::::::::::::::::::::::::: */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/_intsup.h>

/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque specifiques :::::::::::::::::::::::::::::::::::::::::::: */
#include "esp_timer.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "soc/gpio_num.h"
#include "hal/uart_types.h"
#include "freertos/semphr.h"
#include "esp_log.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Macros :::::::::::::::::::::::::::::::::::::::::::::: */                 


/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
              /* :::::::::::::::::::::::::::::::::::::::::::: Structure::::::::::::::::::::::::::::::::::::::::::::::::::::: */
              
struct MyLoRa_t;

// --------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Chaîne modifiable adossée à un buffer externe.
 *
 * Représente une vue sur un tampon de caractères (potentiellement '\0'-terminé),
 * avec sa longueur utile et sa capacité totale.
 * Invariants recommandés : data != NULL, size < max_size,
 * et si utilisé comme C-string, data[size] == '\0'.
 */
typedef struct str512_t{
    char   sentence[512]; // buffer (nul-terminé si utilisé comme C-string)
} str512_t;

// --------------------------------------------------------------------------------------------------------------------------------------

/**
 * @brief Configuration matérielle & driver pour le module GNSS sur UART.
 *
 * Contient le port UART, la config série, le mapping des GPIO et
 * la taille des buffers RX/TX du driver.
 */
typedef struct MyLoRa_dev_config_t{
    /* ---- UART ---- */
    uart_port_t   port_num;   /**< Port UART utilisé : UART_NUM_0 / _1 / _2. */
    uart_config_t uart_cfg;   /**< Paramètres UART (baudrate, data bits, parité, stop, source clk). */

    /* ---- Broches ---- */
    gpio_num_t rx_io;         /**< GPIO RX (entrée UART du MCU). */
    gpio_num_t tx_io;         /**< GPIO TX (sortie UART du MCU). */
    gpio_num_t reset_io;      /**< GPIO RESET du GNSS (actif bas si câblé, sinon GPIO_NUM_NC). */
    gpio_num_t boot_io;       /**< GPIO BOOT/CFG du GNSS (optionnel, sinon GPIO_NUM_NC). */

    /* ---- Buffers driver ---- */
    uint16_t rx_buf_size;     /**< Taille buffer RX du driver (ex: 2048–4096). */
    uint16_t tx_buf_size;     /**< Taille buffer TX du driver (ex: 256–1024).  */
   
} MyLoRa_dev_config_t;

// --------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Espace de fonctions liées au mode LoRaWAN du modem.
 *
 * Cette structure regroupe des pointeurs de fonctions pour:
 *  - basculer le modem en LoRaWAN (vs. point-à-point), 
 *  - rejoindre le réseau (OTAA/ABP), 
 *  - activer/désactiver des options (ADR, messages confirmés, mode privé),
 *  - lire des métriques radio (RSSI, SNR) et paramètres réseau.
 *
 * Conventions générales
 * ---------------------
 * - Tous les accesseurs prennent en premier paramètre `self` (contexte MyLoRa_t).
 * - Les fonctions booléennes renvoient `true` en cas de succès/état activé,
 *   `false` si échec/état désactivé (ou non disponible).
 * - Unités :
 *      * Puissance TX/RX en dBm (int8_t).
 *      * RSSI en dBm (int8_t), SNR en dB (int8_t).
 * - Timing : privilégier des lectures non bloquantes/courtes (<100 ms) ; les
 *   opérations réseau (JOIN, changement de classe/mode) peuvent prendre
 *   plusieurs secondes selon le réseau.
 */
typedef struct LoRaWan_mode_t{

    /**
     * @brief Indique si le modem est actuellement configuré en mode LoRaWAN.
     * @return true si le profil LoRaWAN est actif, false sinon.
     */
    bool (*is_LoRaWAN_mode)(struct MyLoRa_t *self);

    /**
     * @brief Bascule le modem en mode LoRaWAN (profil protocolaire).
     * @return true si la bascule a réussi, false sinon.
     * @post Reconfigurer les paramètres LoRaWAN (région, clés) si nécessaire.
     */
    bool (*Enable_LoRaWAN_mode)(struct MyLoRa_t *self);

    /**
     * @brief Démarre la procédure d’association au réseau (JOIN).
     * @details Utilise OTAA si activé, sinon ABP (selon configuration).
     * @return true si la commande est acceptée/envoyée ; ne garantit pas le succès du join.
     * @post Vérifier l’état avec `is_Joined()`.
     */
    bool (*Join)(struct MyLoRa_t *self);

    /**
     * @brief Indique si le modem est joint (attaché) au réseau LoRaWAN.
     * @return true si l’état “joined” est confirmé, false sinon.
     */
    bool (*is_Joined)(struct MyLoRa_t *self);

    /**
     * @brief Active/Désactive le mode de messages confirmés (ACK) côté uplink.
     * @param Mode true pour activer (confirmé), false pour désactiver (non confirmé).
     * @return true si pris en compte, false sinon.
     */
    bool (*Enable_Confirm_mode)(struct MyLoRa_t *self, bool Mode);

    /**
     * @brief Active/Désactive le “mode privé” (réseau privé / NetID 0).
     * @param mode true pour activer le mode privé, false pour le désactiver.
     * @return true si succès, false sinon.
     * @note La signification exacte dépend du firmware modem (ex. NetID/CFList).
     */
    bool (*Enable_private_mode)(struct MyLoRa_t *self, bool Mode);

	/**
     * @brief Indique si le mode privé est actif.
     * @return true si actif, false sinon.
     */
    bool (*is_private_mode)(struct MyLoRa_t *self);
    
    /**
     * @brief Active/Désactive le mode OTAA (sinon ABP).
     * @param mode true pour OTAA, false pour ABP.
     * @return true si pris en compte, false sinon.
     */
    bool (*Enable_OTAA)(struct MyLoRa_t *self, bool Mode);

    /**
     * @brief Indique si l’activation OTAA est sélectionnée (vs ABP).
     * @return true si OTAA, false si ABP.
     */
    bool (*is_OTAA)(struct MyLoRa_t *self);

    /**
     * @brief Indique si les messages confirmés (uplink) sont activés.
     * @return true si confirmé, false sinon.
     */
    bool (*is_Confirm_mode)(struct MyLoRa_t *self);

    /**
     * @brief Active/Désactive l’ADR (Adaptive Data Rate).
     * @param Mode true pour activer ADR, false pour désactiver.
     * @return true si succès, false sinon.
     */
    bool (*Enable_ADR)(struct MyLoRa_t *self, bool Mode);

    /**
     * @brief Indique si l’ADR est activé.
     * @return true si ADR actif, false sinon.
     */
    bool (*is_ADR)(struct MyLoRa_t *self);

    /**
     * @brief Lit la puissance de réception (si exposée par le modem).
     * @param mode true: lecture modem; false: lecture cache si dispo.
     * @return puissance RX en dBm (int8_t).

     */
    int8_t (*get_power_receive)(struct MyLoRa_t *self);

    /**
     * @brief Définit la puissance de réception (si supportée).
     * @param power dBm attendu par le firmware (ex. plage [-128..127] ou contrainte modem).
     * @return true si accepté, false sinon.
     */
    bool (*set_power_receive)(struct MyLoRa_t *self, int8_t Power);

    /**
     * @brief Lit la puissance d’émission (TX power) configurée.
     * @param mode true: lecture modem; false: lecture cache si dispo.
     * @return TX power en dBm (int8_t).
     * @note La plage valide dépend de la région et du module (ex. 2..22 dBm).
     */
    int8_t (*get_power_transmit)(struct MyLoRa_t *self);

    /**
     * @brief Définit la puissance d’émission (TX power).
     * @param power dBm souhaités (respecter les limites régionales/module).
     * @return true si la valeur est acceptée, false sinon.
     * @post Peut modifier la portée/consommation ; ADR peut ensuite ajuster le DR.
     */
    bool   (*set_power_transmit)(struct MyLoRa_t *self, int8_t Power);

    /**
     * @brief Retourne le NetID courant (si applicable).
     * @return NetID (codé sur int8_t ici) ou -1 en cas d’erreur/indisponible.
     * @note Selon le module, NetID peut dépasser 7 bits ; adapter le type si besoin.
     */
    int8_t (*get_netid)(struct MyLoRa_t *self);

    /**
     * @brief Dernier RSSI mesuré (downlink) rapporté par le modem.
     * @return RSSI en dBm (int8_t). Exemple: -120..-30 dBm. INT8_MIN si erreur.
     */
    int8_t (*get_rssi)(struct MyLoRa_t *self);

    /**
     * @brief Dernier SNR mesuré (downlink) rapporté par le modem.
     * @return SNR en dB (int8_t). Exemple: -20..+10 dB. INT8_MIN si erreur.
     */
    int8_t (*get_snr)(struct MyLoRa_t *self);
    
    /**
	 * @brief Envoie un payload uplink sur le FPort donné.
	 * @param self    Contexte modem.
	 * @param payload Chaîne (HEX conseillé, sans espaces).
	 * @param port    FPort (1..223).
	 * @return true si émission OK (TX_DONE / SEND_CONFIRMED_OK), sinon false.
 	*/
	bool (*Send_Payload)(struct MyLoRa_t *self, char *Payload, uint8_t Port);

	/**
	 * @brief Lit le dernier payload downlink disponible.
	 * @param self Contexte modem.
	 * @return str512_t: HEX du payload si dispo, sinon chaîne vide.
	 */
	str512_t (*Receive_Payload)(struct MyLoRa_t *self);
	
} LoRaWan_mode_t;
	
// --------------------------------------------------------------------------------------------------------------------------------------
typedef struct P2P_mode_t{
	
    // A voir
    
} P2P_mode_t;

// --------------------------------------------------------------------------------------------------------------------------------------
typedef struct RF_mode_t{
	
    // A voir
      
} RF_mode_t;

// --------------------------------------------------------------------------------------------------------------------------------------
/* 
 * @brief Info_mode_t — Interface de LECTURE (non destructive).
 *
 * Rôle
 *  - Fournir un accès **idempotent** aux informations du modem : états (boot/sleep) et identifiants
 *    (nom, modèle, version, tension, SN), sans modifier la configuration.
 *
 * Contrat
 *  - Ne doit pas réveiller le modem ni provoquer de side-effects.
 *  - Peut s’appuyer sur des caches gérés par `MyLoRa_t` pour éviter des requêtes lentes.
 *  - Tous les getters renvoient une chaîne valide ; pas de pointeurs NULL.
 *
 * Intégration
 *  - Généralement stocké comme champ `info` dans `MyLoRa_t` et rempli à l’initialisation du driver.
 */
typedef struct Info_mode_t{

    /**
     * @brief Indique si un mode sommeil (light/deep) est actif.
     * @return true si sommeil actif, sinon false.
     * @note Recommandé : tenir un flag/callback interne mis à jour par les fonctions de `Config_mode_t`.
     */
    bool     (*is_sleep)(struct MyLoRa_t *self);

    /**
     * @brief Nom logique/alias du modem.
     * @return Chaîne max 512 ; "N/A" si non supporté.
     * @post  Invalider/rafraîchir après `set_name()`.
     */
    str512_t (*get_name)(struct MyLoRa_t *self);

    /**
     * @brief Baudrate UART configuré côté modem.
     * @return Chaîne représentant le débit (ex. "115200") ; "N/A" si non supporté.
     * @note  Peut être récupéré d’un cache tenu à jour par `set_baud()`.
     */
    str512_t (*get_baud)(struct MyLoRa_t *self);

    /**
     * @brief Modèle matériel (ex. "RAK3172-SiP").
     * @return "N/A" si non supporté.
     */
    str512_t (*get_model)(struct MyLoRa_t *self);

    /**
     * @brief Tension mesurée par le modem (si capabilité présente).
     * @return Format stable (ex."V:x.xx") ; "N/A" sinon.
     * @warning Documenter la convention choisie pour éviter les ambiguïtés côté UI/parseur.
     */
    str512_t (*get_voltage)(struct MyLoRa_t *self);

    /**
     * @brief Version firmware/stack (ex. "RUI4 vX.Y.Z").
     * @return "N/A" si non supporté.
     */
    str512_t (*get_version)(struct MyLoRa_t *self);

    /**
     * @brief Identifiant unique (SN/DevEUI/ChipID).
     * @return "N/A" si non supporté.
     */
    str512_t (*get_serial_number)(struct MyLoRa_t *self);

} Info_mode_t;

// --------------------------------------------------------------------------------------------------------------------------------------

/*
 * @brief Config_mode_t — Interface d’ÉCRITURE/COMMANDE.
 *
 * Rôle
 *  - Centraliser toutes les opérations **mutatrices** : reset/factory, mot de passe, sommeil,
 *    changement de nom/baud, entrée/sortie bootloader.
 *
 * Contrat
 *  - Retour `true` = succès ; `false` = TIMEOUT/NACK/unsupported/erreur format.
 *  - Doit **invalider/rafraîchir** les caches exposés par `Info_mode_t` (nom/baud/états, etc.).
 *  - Peut impacter la connectivité UART (reset/boot/baud) : prévoir re-sync côté hôte.
 *
 * Intégration
 *  - Généralement stocké comme champ dans `MyLoRa_t`, initialisé avec les implémentations AT.
 */
typedef struct Config_mode_t{

    /**
     * @brief Vérifie la santé de la ligne UART (ex. "AT" → "OK", lecture version courte).
     * @return true si la ligne répond ; sinon false.
     */
    bool (*AT_check)(struct MyLoRa_t *self);

    /**
     * @brief Redémarrage logiciel du modem.
     * @return true si l’ordre est accepté et que le modem ré-apparaît, sinon false.
     * @post  Invalider tous les caches ; appliquer un délai avant la première requête suivante.
     */
    bool (*soft_reset)(struct MyLoRa_t *self);

    /**
     * @brief Restaure la configuration d’usine.
     * @return true si succès, sinon false.
     * @warning Peut effacer clés/joins/config locales selon plateforme — à documenter clairement !
     * @post    Re-provisionner si nécessaire (réseau, clés, etc.).
     */
    bool (*set_factory)(struct MyLoRa_t *self);

    /**
     * @brief Active la protection par mot de passe (si supportée).
     * @return true si activée, sinon false.
     * @note  Alias conseillé : `enable_password` (snake_case).
     */
    bool (*Enable_password)(struct MyLoRa_t *self);

    /**
     * @brief Bascule sommeil/réveil (toggle booléen).
     * @param mode true=dormir, false=réveiller.
     * @return true si accepté, sinon false.
     * @post  Mettre à jour l’état interne consulté par `info.is_sleep()`.
     */
    bool (*sleep)(struct MyLoRa_t *self, bool mode);

    /**
     * @brief Sélectionne un niveau de sommeil précis (plateforme-dépendant).
     * @param mode Convention suggérée : 0=awake, 1=light, 2=deep (documenter le mapping exact).
     * @return true si accepté, sinon false.
     * @post  Mettre à jour l’état interne consulté par `info.is_sleep()`.
     */
    bool (*sleep_level)(struct MyLoRa_t *self, uint8_t mode);

    /**
     * @brief Définit le nom logique/alias persistant.
     * @param name Chaîne C (UTF-8 conseillé) ; valider longueur/charset.
     * @return true si enregistré, sinon false.
     * @post  Invalider le cache utilisé par `info.get_name()`.
     */
    bool (*set_name)(struct MyLoRa_t *self, char *name);

    /**
     * @brief Change le mot de passe.
     * @param psw  Chaîne C ; respecter les contraintes de longueur/caractères.
     * @return true si changé, sinon false.
     * @security Effacer le buffer `psw` en mémoire après usage si possible.
     */
    bool (*change_password)(struct MyLoRa_t *self, char *psw);

    /**
     * @brief Modifie le baudrate côté modem.
     * @param new_baud ex. 9600, 115200, 921600…
     * @return true si le modem confirme, sinon false.
     * @post  **Reconfigurer immédiatement l’UART hôte** ; invalider `info.get_baud()` ; re-sync parser.
     */
    bool (*set_baud)(struct MyLoRa_t *self, uint32_t new_baud);

} Config_mode_t;
// --------------------------------------------------------------------------------------------------------------------------------------	
/**
 * @brief Structure "objet C" thread-safe pour la gestion d’un module LoRa via UART.
 *
 * @details
 * Représente une instance logicielle du module LoRa, intégrant :
 * - une configuration matérielle (UART, GPIO),
 * - des pointeurs de méthodes (Begin, Reset, Boot, Send_command...),
 * - un mécanisme de synchronisation (mutex FreeRTOS).
 *
 * L'objet est conçu pour un environnement embarqué multitâche (ex : ESP32-S3 sous FreeRTOS),
 * et permet un accès concurrent sécurisé aux ressources du module via son API.
 *
 * L’initialisation s’effectue en deux temps :
 * 1. `LoRa_build()` : configure les pointeurs et l’objet (aucun accès matériel).
 * 2. `Begin()` : initialise le matériel (UART, GPIO, état interne).
 *
 * @note Aucun buffer n’est alloué dynamiquement ici. L’objet est réutilisable et extensible.
 * @warning La configuration passée à `LoRa_build()` doit rester valide pendant toute la durée de vie de l’objet.
 */
typedef struct MyLoRa_t {
	
	/* =============================== État / handles / variables =============================== */
	
	/** Pointeur vers la configuration matérielle/driver (UART, GPIO, buffers). */
	MyLoRa_dev_config_t *MyLoRa_config;

	/** Mutex pour synchronisation des accès à l'objet LoRa (thread-safe). */
	SemaphoreHandle_t mutex;
	
	/* ================================== Interfaces ================================= */
	
	// Interface LoRaWAN attachée à cet objet
    struct LoRaWan_mode_t LoRaWAN;
    
    // Interface LoRaWAN attachée à cet objet
    struct P2P_mode_t  P2P;
    
    // Interface LoRaWAN attachée à cet objet
    struct RF_mode_t  RF;
    
    // Interface d'information attachée à cet objet
    struct Info_mode_t    Info;
    
    // Interface de configuration attachée à cet objet
    struct Config_mode_t  Config;

	/* ================================== Méthodes ================================= */

	/**
	 * @brief Initialise l’UART, les broches GPIO et l’état interne du module LoRa.
	 *
	 * @param self Instance cible.
	 * @return true si l’initialisation a réussi, false sinon.
	 *
	 * @post UART prêt, broches configurées, état initialisé.
	 */
	bool (*Begin)(struct MyLoRa_t *self);

	/**
	 * @brief Effectue un reset matériel ou logiciel du module LoRa.
	 *
	 * @param self Instance cible.
	 * @return true si le reset a réussi, false sinon.
	 *
	 * @post Le module redémarre, les états précédents sont réinitialisés.
	 */
	bool (*Reset)(struct MyLoRa_t *self);

	/**
	 * @brief Force le passage en mode BOOT via la broche dédiée (si câblée).
	 *
	 * @param self Instance cible.
	 * @return true si l’opération a réussi, false sinon.
	 *
	 * @note Requis pour certaines mises à jour firmware via UART.
	 */
	bool (*Boot)(struct MyLoRa_t *self);

	/**
	 * @brief Modifie dynamiquement le débit (baudrate) du lien UART avec le module LoRa.
	 *
	 * @param self     Instance cible.
	 * @param new_baud Nouveau baudrate à appliquer (ex: 9600, 115200).
	 * @return true si le changement est effectif, false sinon.
	 *
	 * @post L’UART est reconfiguré ; les composants en amont doivent se resynchroniser.
	 */
	bool (*Set_baud)(struct MyLoRa_t *self, uint32_t new_baud);

	/**
	 * @brief Envoie une commande AT et lit la réponse jusqu’à un motif (pattern) donné.
	 *
	 * @param self     Contexte de l’objet LoRa.
	 * @param cmd      Commande AT à envoyer (terminée automatiquement par CRLF).
	 * @param pattern  Caractère de fin attendu dans la réponse (ex: '+').
	 * @param timeout_ms Délai maximal d’attente de la réponse (en millisecondes).
	 * @return         Réponse reçue, encapsulée dans un objet `str512_t`.
	 */
	str512_t (*Send_command)(struct MyLoRa_t *self, const char *cmd, bool skip_first_line, uint32_t timeout_ms);

	/**
	 * @brief Libère les ressources associées (UART, GPIO, buffers, etc.).
	 *
	 * @param self Instance cible.
	 * @return true si la destruction est complète, false sinon.
	 *
	 * @note Attention à l'ownership : ne pas libérer des ressources partagées ailleurs.
	 */
	bool (*Destroy)(struct MyLoRa_t *self);

} MyLoRa_t;

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */
/**
 * @brief Initialise une instance MyLoRa_t sans accéder au matériel.
 *
 * @details
 * Configure les pointeurs de méthode (Begin, Reset, etc.) et associe la configuration matérielle fournie.
 * Ne réalise aucune initialisation UART ou GPIO — cela doit être fait via MyLoRa->Begin().
 *
 * @param[out] MyLoRa       Instance à initialiser (mémoire valide requise).
 * @param[in]  MyLoRa_confg Configuration matérielle, ou NULL pour utiliser les valeurs par défaut.
 *
 * @pre Appeler avant toute utilisation de l’instance.
 * @post L’objet est fonctionnel mais pas encore connecté au matériel.
 * @note La configuration doit rester valide pendant toute la durée de vie de l’objet.
 */
void LoRa_build(MyLoRa_t *MyLoRa, MyLoRa_dev_config_t *MyLoRa_confg);

void Gloabal_request_build(MyLoRa_t *MyLoRa);

void LoRaWAN_request_build(MyLoRa_t *MyLoRa);
#ifdef __cplusplus
}
#endif
#endif /* MY_LORA_H_ */