#ifndef MY_LCD_H_
#define MY_LCD_H_

// Appel unique des fichiers

#pragma once

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque standard :::::::::::::::::::::::::::::::::::::::::::: */
#include <stdint.h>
#include <string.h>
#include <sys/_intsup.h>

/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque specifiques :::::::::::::::::::::::::::::::::::::::::::: */
#include "lvgl.h"
#include <math.h>
#include "esp_timer.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "soc/gpio_num.h"
#include "misc/lv_area.h"
#include "misc/lv_types.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_types.h"
#include "misc/lv_color.h"
#include "hal/i2c_types.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_st7796.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_lcd_panel_vendor.h"

/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
/* :::::::::::::::::::::::::::::::::::::::::::: Structure::::::::::::::::::::::::::::::::::::::::::::::::::::: */

/**
 * @brief Configuration matérielle de l’afficheur LCD et du tactile.
 *
 * @details
 * Regroupe le câblage, les résolutions, fréquences et l’orientation (miroir X/Y,
 * échange d’axes) pour l’écran et le tactile, afin de garder des coordonnées
 * cohérentes après rotation/mirroring du panneau.
 *
 * @note
 * - Mettre les broches non utilisées à GPIO_NUM_NC.
 * - Appliquer la même transformation (miroir/swap) au tactile si nécessaire.
 */
typedef struct Mylcd_dev_config_t {

    /* ===================== Bus & Résolution ===================== */
    spi_host_device_t LCD_HOST;     /**< SPI host (SPI2_HOST/SPI3_HOST). */
    i2c_port_t      TOUCH_HOST;     /**< I2C port (I2C_NUM_0/I2C_NUM_1). */

    uint16_t LCD_H_RES;             /**< Résolution horizontale (px). */
    uint16_t LCD_V_RES;             /**< Résolution verticale (px).   */

    /* ====================== Broches SPI LCD ===================== */
    gpio_num_t   LCD_MOSI;          /**< MOSI. */
    gpio_num_t   LCD_SCLK;          /**< SCLK. */
    gpio_num_t   LCD_CS;            /**< Chip Select. */
    gpio_num_t   LCD_DC;            /**< Data/Command. */
    gpio_num_t   LCD_RST;           /**< Reset matériel. */
    gpio_num_t   LCD_BL;            /**< GPIO rétroéclairage. */

    ledc_channel_t LCD_BL_CHANNEL;  /**< Canal LEDC pour PWM BL. */
    uint32_t       LCD_SCLK_HZ;     /**< Fréquence SPI (Hz). */

    /* ===================== Broches I²C Tactile ================== */
    gpio_num_t TOUCH_SDA;           /**< SDA. */
    gpio_num_t TOUCH_SCL;           /**< SCL. */
    gpio_num_t TOUCH_RST;           /**< Reset tactile (optionnel). */
    gpio_num_t TOUCH_INT;           /**< IRQ tactile (optionnel).    */
    uint32_t   TOUCH_SCLK_HZ;       /**< Fréquence I²C (Hz). */

    /* ================== Orientation / Transformations =========== */
    // Panneau LCD
    bool lcd_mirror_x;              /**< true: miroir horizontal. */
    bool lcd_mirror_y;              /**< true: miroir vertical.   */
    bool lcd_swap_xy;               /**< true: échange X↔Y (rotation 90°). */

    // Tactile (appliquer la même géométrie que le LCD pour coord. cohérentes)
    bool touch_mirror_x;            /**< true: miroir X sur coords tactiles. */
    bool touch_mirror_y;            /**< true: miroir Y sur coords tactiles. */
    bool touch_swap_xy;   
    
    // Couleur affichage
	uint8_t           bits_per_pixel;  /**< Profondeur pixel (ex. 24 pour RGB888). */
	lv_color_format_t color_format;    /**< LV_COLOR_FORMAT_RGB888. */         
    
} Mylcd_dev_config_t;

// --------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Contexte/driver d’un module LCD (afficheur + tactile) sous ESP-IDF.
 *
 * @details
 * Cette structure encapsule l’état courant de l’écran et expose des méthodes
 * (pointeurs de fonctions) pour gérer son cycle de vie : initialisation,
 * réglage de la luminosité, arrêt et destruction.
 *
 * @note
 * - Les handles `esp_lcd_*` peuvent être créés soit en interne par `Begin()`,
 *   soit fournis en amont selon ta logique (clarifie l’ownership dans ton .c).
 * - Le paramètre `new_baud` de `Set_brightness` représente ici un **niveau
 *   de luminosité** (0..100). Le nom historique n’est pas idéal, mais conservé.
 * - Préserve la cohérence LCD/tactile si tu appliques des miroirs/swap d’axes.
 *
 * @warning
 * - Assure-toi que `Mylcd_config` pointe vers une configuration valide pendant
 *   toute la durée de vie de l’instance.
 * - Définis clairement qui détruit les handles (Begin/Destroy vs code appelant).
 */
typedef struct Mylcd_t {
    /* =============================== État / handles =============================== */
    
	/**< Pointeur sur la configuration matérielle (bus, GPIO, orientation, etc.). */
    Mylcd_dev_config_t *Mylcd_config;  
    
    /** Handle du bus I²C (driver NG) créé via i2c_new_master_bus()*/
    i2c_master_bus_handle_t bus_master_handle;
    
    /**< Handle d’afficheur LVGL (v9) retourné par lvgl_port_add_disp(). */         
    lv_display_t *Mylcd_display_handle; 
    
    /**< Handle du panneau LCD (`esp_lcd_panel_handle_t`). */ 
    esp_lcd_panel_handle_t Mylcd_panel_handle;
    
    /**< Handle IO du panneau (SPI/…); créé via `esp_lcd_panel_io_*_new()`. */     
    esp_lcd_panel_io_handle_t Mylcd_panel_io_handle;
    
    /**< Handle IO du tactile (I²C/…); si tactile utilisé. */  
    esp_lcd_panel_io_handle_t Mylcd_touch_io_handle; 
    
    /**< Handle du contrôleur tactile (`esp_lcd_touch_handle_t`). */ 
    esp_lcd_touch_handle_t Mylcd_touch_handle;     
	
    /* ================================== Méthodes ================================= */

    /**
     * @brief Initialise le panneau (et éventuellement le tactile) selon `Mylcd_config`.
     *
     * @param self Instance cible.
     * @return true si l’initialisation réussit, false sinon.
     *
     * @post Handles prêts à l’emploi, backlight réglé sur la valeur par défaut.
     */
    bool (*Begin)(struct Mylcd_t *self);

    /**
     * @brief Régle la luminosité du rétroéclairage.
     *
     * @param self      Instance cible.
     * @param brightness  Niveau de luminosité (0..255). (Nom conservé pour compatibilité)
     * @return true si l’opération réussit, false sinon.
     */
    bool (*Set_brightness)(struct Mylcd_t *self, uint8_t brightness);

    /**
     * @brief Met le panneau (et le tactile) à l’arrêt propre (sleep, BL off, etc.).
     *
     * @param self Instance cible.
     * @return true si l’arrêt réussit, false sinon.
     *
     * @post Les handles restent valides mais l’afficheur est inactif.
     */
    bool (*Stop)(struct Mylcd_t *self);

    /**
     * @brief Libère les ressources détenues par l’instance (handles, IO, etc.).
     *
     * @param self Instance cible.
     * @return true si la destruction réussit, false sinon.
     *
     * @note Décide dans l’implémentation si `Destroy` supprime aussi les handles
     *       fournis par l’extérieur (documente l’ownership pour éviter les doubles free).
     */
    bool (*Destroy)(struct Mylcd_t *self);

} Mylcd_t;

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */
	   
/**
 * @brief Construit/initialise une instance Mylcd_t (style “objet C”).
 *
 * @details
 * - Met l’instance dans un état propre (pointeurs à NULL si implémenté ainsi).
 * - Câble les méthodes (Begin/Set_brightness/Stop/Destroy) vers leurs implémentations.
 * - Associe la configuration matérielle passée en paramètre ; si @p Mylcd_confg est NULL,
 *   utilise la configuration par défaut @ref Mylcd_default.
 * - Ne réalise aucune allocation dynamique ni initialisation matérielle : appeler ensuite
 *   @ref Mylcd_t::Begin pour créer les bus/handles et démarrer l’afficheur.
 *
 * @param[out] Mylcd        Instance cible à initialiser (doit être une zone mémoire valide).
 * @param[in]  Mylcd_confg  Configuration matérielle à utiliser, ou NULL pour les valeurs par défaut.
 *
 * @note Thread-safe : non (à appeler avant tout accès concurrent à l’instance).
 * @warning @p Mylcd_config doit rester valide pendant toute la durée de vie de l’instance.
 */	 
void lcd_build(Mylcd_t *Mylcd, Mylcd_dev_config_t *Mylcd_confg);

#ifdef __cplusplus
}
#endif

#endif /* MY_LCD_H_ */