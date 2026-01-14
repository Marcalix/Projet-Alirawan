#ifndef MY_GAUGE_H_
#define MY_GAUGE_H_

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
#include "esp_log.h"
#include "i2c_bus.h"
#include "max17048.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "soc/gpio_num.h"
#include "hal/i2c_types.h"
#include "hal/uart_types.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Macros :::::::::::::::::::::::::::::::::::::::::::::: */                 


/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
              /* :::::::::::::::::::::::::::::::::::::::::::: Structure::::::::::::::::::::::::::::::::::::::::::::::::::::: */

/**
 * @brief Configuration matérielle & driver pour un périphérique I²C.
 *
 * Contient le port I²C, la fréquence d'horloge, l'adresse esclave,
 * le mapping des GPIO (SDA/SCL) et l'activation des pull-ups internes.
 * @note Les pull-ups internes de l'ESP32 sont faibles (~45–100 kΩ) :
 *       OK pour tests, pas recommandés en production. Mettre des 2.2–10 kΩ externes.
 */
typedef struct Mygauge_dev_config_t {
    /* ---- Bus I²C ---- */
    uint8_t   port_num;       /**< Numéro de contrôleur I²C : I2C_NUM_0 / I2C_NUM_1. */
    uint32_t  clk_speed_hz;   /**< Fréquence SCL en Hz (ex. 100000, 400000). */
    uint8_t  dev_addr;       /**< Adresse esclave 7 bits (sur 7) ; mettre l’adresse << 0. */
 
    /* ---- Broches ---- */
    gpio_num_t SDA;           /**< GPIO pour SDA. */
    gpio_num_t SCL;           /**< GPIO pour SCL. */

    /* ---- Pull-ups internes (esp-idf : GPIO_PULLUP_ENABLE / DISABLE) ---- */
    bool      sda_pullup_en;  /**< true = activer pull-up interne sur SDA. */
    bool      scl_pullup_en;  /**< true = activer pull-up interne sur SCL. */

    /* ---- Optionnel périphérique ---- */
    bool      mode;    /**< Mode d'exploitation du device (I2C_MODE_MASTER / I2C_MODE_SLAVE). */
} Mygauge_dev_config_t;

// --------------------------------------------------------------------------------------------------------------------------------------	
/**
 * @brief Objet C représentant un fuel gauge I²C (configuration + handles + méthodes).
 *
 * @details
 * - Initialise une instance « objet C » en regroupant la configuration matérielle,
 *   des handles opaques et des pointeurs de méthodes (Begin/Reset/get_*…).
 * - La configuration matérielle utilisée est @p Mygauge_config, ou une
 *   configuration par défaut (ex. @ref Mygauge_default) si NULL au moment de Begin().
 * - Cette structure **ne réalise pas d’accès matériel** lors de son simple remplissage :
 *   il faut appeler `Begin()` pour initialiser le bus I²C et le périphérique.
 *
 * @pre L’instance doit être allouée par l’appelant (pile ou tas) avant usage.
 * @post Après `Begin()`, les handles I²C/périphérique sont valides si `true` est renvoyé.
 *
 * @warning La mémoire pointée par @ref Mygauge_config doit rester valide pendant
 *          toute la durée de vie de l’objet (propriété non transférée).
 */
typedef struct Mygauge_t {

    /* ============================ État / handles / variables ============================ */

    Mygauge_dev_config_t *Mygauge_config;   /**<
        Pointeur vers la configuration matérielle I²C (port, broches, adresse, pull-ups…).
        @warning L’appelant reste propriétaire de cette structure (pas de copie interne).
        @note Si NULL, `Begin()` pourra charger une config par défaut (si prévue). */

    void *i2c_bus_handle;                  /**<
        Handle (opaque) vers le bus I²C sous-jacent (ex. handle de « i2c_bus » d’Espressif).
        pointeur pour permettre au driver d’écrire le handle alloué.*/

    void *Mygauge_handle;                  /**<
        Handle (opaque) de l’IC fuel gauge (contexte driver).
        Double pointeur si l’allocation est faite dans `Begin()`. */

    /* ====================================== Méthodes ===================================== */

    /**
     * @brief Initialise le bus I²C et le périphérique fuel gauge.
     *
     * @details Configure le contrôleur I²C (SDA/SCL/pull-ups/fréquence), crée les handles,
     *          vérifie la présence du device à @ref Mygauge_dev_config_t::dev_addr,
     *          lit des registres d’identification si nécessaire et met le device en état nominal.
     *
     * @param self Instance cible.
     * @return `true` si l’initialisation complète réussit, sinon `false`.
     *
     * @pre @ref Mygauge_config non NULL (ou disponibilité d’une config par défaut).
     * @post @ref i2c_bus_handle et @ref Mygauge_handle valides si `true`.
     * @warning Non thread-safe sans protection externe (mutex) si partagé.
     */
    bool (*Begin)(struct Mygauge_t *self);

    /**
     * @brief Effectue un reset logiciel du fuel gauge.
     *
     * @details Déclenche un soft reset (si supporté) et restaure les registres par défaut
     *          ou une configuration minimale requise.
     *
     * @param self Instance cible.
     * @return `true` si l’opération a abouti, sinon `false`.
     *
     * @post Le device est revenu dans un état propre (peut invalider des mesures transitoires).
     */
    bool (*Reset)(struct Mygauge_t *self);

    /**
     * @brief Retourne la version/silicon ID/ROM code du fuel gauge.
     *
     * @param self Instance cible.
     * @return Version codée sur 16 bits (interprétation propre au driver) ;
     *         0 ou une valeur sentinelle en cas d’échec.
     */
    uint16_t (*get_version)(struct Mygauge_t *self);

    /**
     * @brief Donne l’état de charge (State of Charge, SoC) en pourcentage.
     *
     * @param self Instance cible.
     * @return SoC en %, typiquement dans [0.0 ; 100.0]. `NAN` en cas d’erreur de lecture.
     *
     * @note Selon l’IC, c’est une estimation coulomb/OCV (ex. MAX17048).
     */
    float (*get_percent)(struct Mygauge_t *self);

    /**
     * @brief Mesure la tension batterie.
     *
     * @param self Instance cible.
     * @return Tension en volts (ex. 3.73f). `NAN` si la lecture échoue.
     *
     * @note L’échelle/LSB dépend de l’IC (convertie ici en volts SI).
     */
    float (*get_voltage)(struct Mygauge_t *self);

    /**
     * @brief Taux de charge/décharge estimé.
     *
     * @param self Instance cible.
     * @return Taux en %/h (positif = charge, négatif = décharge), ou `NAN` si non supporté.
     *
     * @note Sur certains IC, cette info n’est pas disponible nativement ; le driver
     *       peut la dériver (dV/dt, ΔSoC/Δt) → dans ce cas, précision limitée.
     */
    float (*get_charge_rate)(struct Mygauge_t *self);

    /**
     * @brief Applique une compensation de température.
     *
     * @param self        Instance cible.
     * @param temperature Température batterie en °C (ex. 25.0f).
     * @return `true` si la compensation est acceptée par le device/driver, sinon `false`.
     *
     * @note Utile pour corriger l’algorithme OCV/SoC selon la température réelle.
     * @warning Vérifier le domaine valide (p. ex. [-20 ; +60] °C selon l’IC).
     */
    bool (*set_temp_compensation)(struct Mygauge_t *self, float temperature);

} Mygauge_t;

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */
/**
 * @brief Construit / initialise une instance @ref Mygauge_t (style « objet C »).
 *
 * @details
 * - Remet l’instance dans un état propre (mise à zéro des champs/handles).
 * - Renseigne les pointeurs de méthodes : Begin / Reset / get_version /
 *   get_percent / get_voltage / get_charge_rate / set_temp_compensation.
 * - N’effectue **aucune** allocation dynamique ni initialisation matérielle :
 *   il faudra appeler ensuite @ref Mygauge_t::Begin() pour ouvrir le bus I²C
 *   et initialiser le circuit de mesure.
 * - La configuration matérielle est lue via @c Mygps->Mygauge_config.
 *   Si ce pointeur est NULL, @ref Mygauge_t::Begin() pourra utiliser une
 *   configuration par défaut (ex. @ref Mygauge_default), si elle est prévue par le driver.
 *
 * @param[out] Mygps  Instance à initialiser (mémoire valide requise ; pile ou tas).
 *
 * @return void
 *
 * @pre Appeler exactement une fois avant tout autre appel de méthodes sur l’instance.
 * @note Pas thread-safe : si l’instance est partagée, protéger l’accès (mutex).
 * @warning La structure pointée par @c Mygps->Mygauge_config (ou la config par défaut)
 *          doit rester valide pendant toute la durée de vie de l’objet (propriété non transférée).
 */
void gauge_build(Mygauge_t *Mygauge, Mygauge_dev_config_t *Mygauge_confg);

#ifdef __cplusplus
}
#endif
#endif /* MY_GAUGE_H_ */
