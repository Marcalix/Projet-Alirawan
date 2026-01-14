#ifndef MY_GNSS_H_
#define MY_GNSS_H_

// Appel unique des fichiers
#pragma once

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque standard :::::::::::::::::::::::::::::::::::::::::::: */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <sys/_intsup.h>


/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque specifiques :::::::::::::::::::::::::::::::::::::::::::: */
#include "minmea.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "soc/gpio_num.h"
#include "hal/uart_types.h"
#include "esp_log.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Macros :::::::::::::::::::::::::::::::::::::::::::::: */                 


/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
              /* :::::::::::::::::::::::::::::::::::::::::::: Structure::::::::::::::::::::::::::::::::::::::::::::::::::::: */
/**
 * @brief Échantillon GNSS agrégé (position, qualité, vitesse, date/heure).
 */
typedef struct gps_data_t{
    bool     is_valid;       /**< true si une position valable est disponible (RMC ‘A’ ou GGA fix>0). */
    float    speed_kmh;      /**< Vitesse sol en km/h (0–65535). */
    double   lat_deg;        /**< Latitude en degrés décimaux (+N, –S). */
    double   lon_deg;        /**< Longitude en degrés décimaux (+E, –W). */
    float    altitude;       /**< Altitude en mètres (réf. GGA). Peut être < 0. */
    uint8_t  fix_quality;    /**< Qualité du fix (GGA): 0=invalid, 1=GPS, 2=DGPS, etc. */
    uint8_t  fix_type;		 /**< type de fix donné par la phrase GSA*/
    uint8_t  nb_sat_tracked; /**< Nombre de satellites utilisés dans la solution (GGA). */
    struct   minmea_date date; /**< Date (RMC) si disponible. */
    struct   minmea_time time;  /**< heure UTC (RMC) si disponible. */
} gps_data_t;
// --------------------------------------------------------------------------------------------------------------------------------------

/**
 * @brief Configuration matérielle & driver pour le module GNSS sur UART.
 *
 * Contient le port UART, la config série, le mapping des GPIO et
 * la taille des buffers RX/TX du driver.
 */
typedef struct Mygps_dev_config_t{
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
    
    /* ---- Tâche FreeRTOS ---- */
    uint8_t  task_priority;      /**< Priorité FreeRTOS (1..configMAX_PRIORITIES-1), ex : 5. */
    char     task_name[configMAX_TASK_NAME_LEN + 1]; /**< Nom de tâche, null-terminé. */
} Mygps_dev_config_t;
// --------------------------------------------------------------------------------------------------------------------------------------	

 /**
 * @brief Initialise une instance Mygps_t (patron “objet C”).
 *
 * @details
 * - Affecte les pointeurs de méthodes (Begin/Set_brightness/Stop/Destroy).
 * - Sélectionne la configuration matérielle : @p Mygps_confg ou @ref Mygps_default si NULL.
 * - Ne fait **aucun** accès matériel ni allocation : appeler ensuite Mygps->Begin().
 *
 * @param[out] Mygps       Instance à initialiser (mémoire valide requise).
 * @param[in]  Mygps_confg Configuration matérielle, ou NULL pour la config par défaut.
 *
 * @pre Appeler avant tout usage concurrent de l’instance.
 * @post L’API est correctement intégrée ; la phase d’acquisition n’a pas encore été lancée.
 * @note La structure pointée par Mylcd_confg doit rester valide pendant toute la vie de l’objet.
 * @warning thread-safe avec les methode definies.
 */
typedef struct Mygps_t {
	 
	/* =============================== État / handles/ Variables =============================== */
     /** Pointeur vers la configuration matérielle/driver (UART, GPIO, buffers). */
    Mygps_dev_config_t *Mygps_config;
    
    /** File d'événements UART (FreeRTOS) fournie par le driver si queue_size>0.
     *  Utilisée avec xQueueReceive() pour traiter UART_DATA, PATTERN_DET, etc. */
	QueueHandle_t Mygps_uart_queue;
	 
    /** Handle de la tâche FreeRTOS du parseur (créé par xTaskCreate). */
    TaskHandle_t Mygps_task_handle;

    /** Dernier instantané GNSS (position, fix, vitesse…), protégé en atomique. */
    _Atomic gps_data_t gps_data;

    /* ================================== Méthodes ================================= */
    /**
     * @brief Initialise l’UART, les GPIO (RX/TX/RESET/BOOT) et l’état interne.
     *
     * @param self Instance cible.
     * @return true si l’initialisation réussit, false sinon.
     *
     * @post UART prêt, broches configurées, états GNSS réinitialisés.
     */
    bool (*Begin)(struct Mygps_t *self);

    /**
     * @brief Déclenche un reset matériel/logiciel du module GNSS.
     *
     * @param self Instance cible.
     * @return true si l’opération réussit, false sinon.
     *
     * @post Le module redémarre, les données courantes peuvent être invalidées.
     */
    bool (*Reset)(struct Mygps_t *self);

    /**
     * @brief Force le mode BOOT/loader via la broche dédiée (si câblée).
     *
     * @param self Instance cible.
     * @return true si l’opération réussit, false sinon.
     *
     * @note Utile pour mise à jour/maintenance du firmware GNSS.
     */
    bool (*Boot)(struct Mygps_t *self);

    /**
     * @brief Modifie le débit série (baudrate) du lien GNSS.
     *
     * @param self      Instance cible.
     * @param new_baud  Nouveau baudrate (ex: 9600, 115200).
     * @return true si le changement réussit, false sinon.
     *
     * @post L’UART est reconfiguré ; re-synchroniser le parseur si besoin.
     */
    bool (*Set_baud)(struct Mygps_t *self, uint32_t new_baud);

    /**
     * @brief Remplace l’instantané des données GNSS (position, fix, etc.).
     *
     * @param self      Instance cible.
     * @param new_data  Pointeur vers les nouvelles valeurs.
     * @return true si la mise à jour réussit, false sinon.
     *
     * @note Conçu pour être thread-safe (accès depuis la tâche parser).
     */
    bool (*Set_gps_data)(struct Mygps_t *self, gps_data_t *new_data);

    /**
     * @brief Retourne la dernière mesure GNSS (copie par valeur).
     *
     * @param self Instance cible.
     * @return Structure `gps_data_t` courante (peut être invalid si pas de fix).
     */
    gps_data_t (*Get_gps_data)(struct Mygps_t *self);

    /**
     * @brief Tâche de parsing: lit l’UART, valide NMEA, met à jour les états.
     *
     * @param arguments Paramètres FreeRTOS (self ou contexte).
     *
     * @post Alimente en continu `gps_data_t` et `sat_data_t[]`.
     */
    void (*Parser_task)(void *arguments);

    /**
     * @brief Crée et lance la tâche parser (xTaskCreate).
     *
     * @param self Instance cible.
     * @return true si la tâche est créée/démarrée, false sinon.
     *
     * @post `Mygps_task_handle` valide et en exécution.
     */
    bool (*Start_task)(struct Mygps_t *self);

    /**
     * @brief Supprime définitivement la tâche parser (vTaskDelete).
     *
     * @param self Instance cible.
     * @return true si la tâche est supprimée, false sinon.
     *
     * @post Handle nul/invalide ; parsing stoppé.
     */
    bool (*Kill_task)(struct Mygps_t *self);

    /**
     * @brief Suspend la tâche parser (vTaskSuspend).
     *
     * @param self Instance cible.
     * @return true si la suspension réussit, false sinon.
     *
     * @post Tâche stoppée temporairement sans destruction.
     */
    bool (*Stop_task)(struct Mygps_t *self);

    /**
     * @brief Reprend la tâche parser (vTaskResume).
     *
     * @param self Instance cible.
     * @return true si la reprise réussit, false sinon.
     *
     * @post Parsing relancé à partir de l’état suspendu.
     */
    bool (*Resume_task)(struct Mygps_t *self);

    /**
     * @brief Libère les ressources (UART, GPIO, buffers) et la tâche si active.
     *
     * @param self Instance cible.
     * @return true si la destruction réussit, false sinon.
     *
     * @note Définir clairement l’ownership pour éviter double-free.
     */
    bool (*Destroy)(struct Mygps_t *self);

} Mygps_t;

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */
/**
 * @brief Construit/initialise une instance Mygps_t (style « objet C »).
 *
 * @details
 * - Remet l’instance dans un état propre (mise à zéro des champs).
 * - Câble les méthodes (Begin/Boot/Reset/Parser_task/Start/Stop/Resume/Destroy).
 * - Associe la configuration matérielle fournie ; si @p Mygps_confg est NULL,
 *   utilise la configuration par défaut @ref Mygps_default.
 * - Ne fait ni allocation dynamique ni init matériel : appeler ensuite
 *   @ref Mygps_t::Begin puis @ref Mygps_t::Start_task pour démarrer le parsing GNSS.
 *
 * @param[out] Mygps        Instance cible à initialiser (mémoire valide requise).
 * @param[in]  Mygps_confg  Configuration UART/GPIO/buffers ; passer NULL pour les
 *                           valeurs par défaut.
 *
 * @return void
 *
 * @pre  Appeler une seule fois avant tout autre appel sur l’instance.
 * @note Thread-safe : non. À appeler avant tout accès concurrent.
 * @warning Le pointeur @p Mygps_confg (ou @ref Mygps_default) doit rester valide
 *          pendant toute la durée de vie de l’instance.
 */
void gps_build(Mygps_t *Mygps, Mygps_dev_config_t *Mygps_confg);   

#ifdef __cplusplus
}
#endif
#endif /* MY_GNSS_H_ */