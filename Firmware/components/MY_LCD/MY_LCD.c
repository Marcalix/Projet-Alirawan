#include "MY_LCD.h"
#include "soc/clk_tree_defs.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables interne :::::::::::::::::::::::::::::::::::::::::::::: */	    
Mylcd_dev_config_t Mylcd_default = {
    /* ===== Bus & résolution ===== */
    .LCD_HOST   = SPI2_HOST,
    .TOUCH_HOST = I2C_NUM_0,

    .LCD_H_RES  = 320,
    .LCD_V_RES  = 480,

    /* ===== Broches SPI LCD ===== */
    .LCD_MOSI   = GPIO_NUM_41,      
    .LCD_SCLK   = GPIO_NUM_42,
    .LCD_CS     = GPIO_NUM_38,
    .LCD_DC     = GPIO_NUM_40,
    .LCD_RST    = GPIO_NUM_39,
    .LCD_BL     = GPIO_NUM_2,

    .LCD_BL_CHANNEL = LEDC_CHANNEL_1,
    .LCD_SCLK_HZ    = 40 * 1000 * 1000U,  // 40 MHz

    /* ===== Broches I²C tactile ===== */
    .TOUCH_SDA  = GPIO_NUM_35,      
    .TOUCH_SCL  = GPIO_NUM_36,
    .TOUCH_RST  = GPIO_NUM_37,     
    .TOUCH_INT  = GPIO_NUM_45,    
    .TOUCH_SCLK_HZ  = 400 * 1000U,  // 400 kHz

    /* ===== Orientation / transformations ===== */
    .lcd_mirror_x  = false,
    .lcd_mirror_y  = true,
    .lcd_swap_xy   = false,

    .touch_mirror_x = true,
    .touch_mirror_y = true,
    .touch_swap_xy  = false,
    
     /* ===== Couleur affichage ===== */
    .bits_per_pixel = 24,
    .color_format   = LV_COLOR_FORMAT_RGB888  
};   

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions static :::::::::::::::::::::::::::::::::::::::::::::: */
	   
static bool lcd_lvgl_init(void)
{
    lv_init(); // Initialisation LVGL et du port (tâche + timer)
    const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    if (lvgl_port_init(&port_cfg) != ESP_OK) return false;
    
    return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------
static bool lcd_spi_confg(Mylcd_t *Mylcd)
{	
	if (!Mylcd) return false;
	
    // Configuration du bus_handle SPI : broches et taille de transfert
    spi_bus_config_t buscfg = {
        .mosi_io_num     = Mylcd->Mylcd_config->LCD_MOSI, // Broche MOSI
        .miso_io_num     = -1,                            // Broche MISO (pas utilisée)
        .sclk_io_num     = Mylcd->Mylcd_config->LCD_SCLK, // Broche d’horloge
        .quadwp_io_num   = -1,                            // Non utilisé en mode 1- ou 2-fils
        .quadhd_io_num   = -1,                            // Non utilisé
        // Taille max de transfert : largeur LCD × 80 lignes × 2 octets/pixel
        .max_transfer_sz = Mylcd->Mylcd_config->LCD_H_RES * 80 * sizeof(uint16_t)
    };
    // Initialise le bus SPI, retourne false en cas d’erreur
    if (spi_bus_initialize(Mylcd->Mylcd_config->LCD_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) return false;

    // Configuration de l’interface panneau LCD via SPI
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num          = Mylcd->Mylcd_config->LCD_CS,      // Broche Chip-Select
        .dc_gpio_num          = Mylcd->Mylcd_config->LCD_DC,      // Broche Data/Command
        .spi_mode             = 0,                                // Mode SPI 0 (CPOL=0, CPHA=0)
        .pclk_hz              = Mylcd->Mylcd_config->LCD_SCLK_HZ, // Fréquence SPI
        .trans_queue_depth    = 10,                               // Profondeur de file de transactions
        .lcd_cmd_bits         = 8,                                // Largeur en bits des commandes
        .lcd_param_bits       = 8,                                // Largeur en bits des paramètres
        .flags.dc_low_on_data = 0,                                // DC = haut pour les données
        .flags.lsb_first      = false                             // MSB en premier
    };
    // Crée le handle IO du panneau via SPI, retourne false en cas d’erreur
    if (esp_lcd_new_panel_io_spi(Mylcd->Mylcd_config->LCD_HOST, &io_config, &Mylcd->Mylcd_panel_io_handle) != ESP_OK) return false;
    
    // Tout est initialisé : bus SPI + IO panneau
    return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------
static bool lcd_panel_init(Mylcd_t *Mylcd)
 {
	 if (!Mylcd) return false;
	 
	// Configuration des paramètres du panneau ST7796
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = Mylcd->Mylcd_config->LCD_RST,       // Broche de reset du LCD
        .bits_per_pixel = Mylcd->Mylcd_config->bits_per_pixel // Format couleur : RGB888 (24 bits/pixel)                            // Format couleur : RGB888 (24 bits/pixel)
    };
    
   // Création du handle du panneau ST7796
   if (esp_lcd_new_panel_st7796(Mylcd->Mylcd_panel_io_handle, &panel_config, &Mylcd->Mylcd_panel_handle) != ESP_OK) return false; 
   if (esp_lcd_panel_reset(Mylcd->Mylcd_panel_handle) != ESP_OK) return false; // Reset matériel du panneau
   vTaskDelay(pdMS_TO_TICKS(1)); 
   if (esp_lcd_panel_init(Mylcd->Mylcd_panel_handle) != ESP_OK) return false;  //  Initialisation logicielle du panneau
   if (esp_lcd_panel_disp_on_off(Mylcd->Mylcd_panel_handle, true) != ESP_OK) return false; // Activation de l’affichage
    
    return true;
 }
 
// --------------------------------------------------------------------------------------------------------------------------------------
static bool lcd_panel_link(Mylcd_t *Mylcd)
{
	if (!Mylcd) return false;
	
	 // Configuration de la structure de liaison LVGL et Affichage
	 lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = Mylcd->Mylcd_panel_io_handle,         // Handle de l’interface SPI vers le panneau     
        .panel_handle  = Mylcd->Mylcd_panel_handle,            // Handle du pilote de panneau (ST7796)
        .buffer_size   = Mylcd->Mylcd_config->LCD_H_RES * 60,  // Taille d’un tampon : largeur écran × 60 lignes
        .double_buffer = true,			                       // Activation du double buffering
        .hres          = Mylcd->Mylcd_config->LCD_H_RES ,      // Résolution horizontale de l’écran
        .vres          = Mylcd->Mylcd_config->LCD_V_RES,       // Résolution verticale de l’écran
        .color_format  = Mylcd->Mylcd_config->color_format     // Format couleur : RGB888 (24 bits/pixel)
    };  
    
    Mylcd->Mylcd_display_handle = lvgl_port_add_disp(&disp_cfg);
    
    if (!Mylcd->Mylcd_display_handle) return false;
    return true;
}
   
// -------------------------------------------------------------------------------------------------------------------------------
//static bool lcd_i2c_confg(Mylcd_t *Mylcd)
//{
//	if (!Mylcd) return false;
//	
//    //Configuration du bus I2C en mode maître
//    i2c_config_t i2c_conf = {
//        .mode = I2C_MODE_MASTER,                          // Mode maître
//        .sda_io_num = Mylcd->Mylcd_config->TOUCH_SDA,     // Broche SDA (données)
//        .scl_io_num = Mylcd->Mylcd_config->TOUCH_SCL,     // Broche SCL (horloge)
//        .sda_pullup_en = GPIO_PULLUP_ENABLE,              // Activation résistance pull-up interne sur SDA
//        .scl_pullup_en = GPIO_PULLUP_ENABLE,              // Activation résistance pull-up interne sur SCL
//        .master.clk_speed = Mylcd->Mylcd_config->TOUCH_SCLK_HZ   // Fréquence d’horloge I2C
//    };
//    // Initialise la configuration I2C, retourne false en cas d’erreur
//    if (i2c_param_config(Mylcd->Mylcd_config->TOUCH_HOST, &i2c_conf) != ESP_OK) return false;
//    
//    // Installe le driver I2C, aucun buffer en RX/TX (touch seulement)
//    if (i2c_driver_install(Mylcd->Mylcd_config->TOUCH_HOST, i2c_conf.mode, 0, 0, 0) != ESP_OK) return false;
//
//    //Configuration de l’interface panneau tactile sur I2C
//    esp_lcd_panel_io_i2c_config_t touch_config = {
//        .dev_addr = 0x38,                     // Adresse I2C du contrôleur tactile 
//        .control_phase_bytes = 1,             // Nombre d’octets pour la phase de commande
//        .dc_bit_offset = 0,                   // Non utilisé en I2C (toujours 0)
//        .lcd_cmd_bits = 8,                    // Largeur en bits des commandes
//        .lcd_param_bits = 8,                  // Largeur en bits des paramètres
//        .flags.disable_control_phase = true   // Désactive la phase de contrôle supplémentaire
//    };
//    
//    // Crée le handle I2C pour le panneau tactile, retourne false en cas d’erreur
//    if (esp_lcd_new_panel_io_i2c(Mylcd->Mylcd_config->TOUCH_HOST, &touch_config, &Mylcd->Mylcd_touch_io_handle) != ESP_OK) return false;
//    
//    // Tout s’est bien passé
//    return true;
//}

static bool lcd_i2c_confg(Mylcd_t *Mylcd)
{	 
    //Configuration du bus I2C
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port   = Mylcd->Mylcd_config->TOUCH_HOST,   // I2C_NUM_0 / I2C_NUM_1
        .scl_io_num = Mylcd->Mylcd_config->TOUCH_SCL,
        .sda_io_num = Mylcd->Mylcd_config->TOUCH_SDA,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true  // mets false si pull-ups externes déjà présentes
        }
    };
  
  	// Initialise la configuration I2C, retourne false en cas d’erreur
    if (i2c_new_master_bus(&bus_cfg, &Mylcd->bus_master_handle)!= ESP_OK) return false;
    
    //Configuration de l’interface panneau tactile sur I2C 
    esp_lcd_panel_io_i2c_config_t io_cfg = {
			.dev_addr = 0x38,
            .control_phase_bytes = 1,
            .dc_bit_offset       = 0,
            .lcd_cmd_bits        = 8,
            .lcd_param_bits      = 8,
            .scl_speed_hz        = Mylcd->Mylcd_config->TOUCH_SCLK_HZ,
            .flags = { .disable_control_phase = true }
        };
    
    // Crée le handle I2C pour le panneau tactile, retourne false en cas d’erreur        
    return ( esp_lcd_new_panel_io_i2c_v2(Mylcd->bus_master_handle, &io_cfg, &Mylcd->Mylcd_touch_io_handle) == ESP_OK);
} 

// -------------------------------------------------------------------------------------------------------------------------------
static bool lcd_touch_init(Mylcd_t *Mylcd)
 {
	 if (!Mylcd) return false;
	 
	//Configuration des paramètres du contrôleur tactile FT5x06
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = Mylcd->Mylcd_config->LCD_H_RES,          // Largeur max renvoyée en coordonnées tactiles
        .y_max = Mylcd->Mylcd_config->LCD_V_RES,          // Hauteur max renvoyée en coordonnées tactiles
        .rst_gpio_num = Mylcd->Mylcd_config->TOUCH_RST,   // Broche de reset du contrôleur tactile
        .int_gpio_num = Mylcd->Mylcd_config->TOUCH_INT,   // Broche d’interruption (notification de contact)
        .levels = {
            .reset = 0,                                   // Niveau logique pour reset actif (0 = bas)
            .interrupt = 0,                               // Niveau logique pour interruption (0 = bas)
        },
        .flags = {
            .swap_xy  = Mylcd->Mylcd_config->touch_swap_xy,     // Échanger X et Y si l’écran est pivoté
            .mirror_x = Mylcd->Mylcd_config->touch_mirror_x,    // Miroir sur l’axe X si l’écran est inversé
            .mirror_y = Mylcd->Mylcd_config->touch_mirror_y,    // Miroir sur l’axe Y si nécessaire
        },
    };
    if (esp_lcd_touch_new_i2c_ft5x06(Mylcd->Mylcd_touch_io_handle, &tp_cfg, &Mylcd->Mylcd_touch_handle) != ESP_OK) return false;
    
    return true;
 }

// ------------------------------------------------------------------------------------------------------------------------------
static bool lcd_touch_link(Mylcd_t *Mylcd)
{
	if (!Mylcd) return false;
	
    // Construction de la configuration pour LVGL
    lvgl_port_touch_cfg_t touch_cfg = {
        .disp   = Mylcd->Mylcd_display_handle,                       // Pointeur vers l’objet d’affichage LVGL
        .handle = Mylcd->Mylcd_touch_handle,  // Handle du contrôleur tactile (FT5x06, etc.)
    };

    // Enregistrement du périphérique tactile auprès du port LVGL
    //    Cela installe la lecture des coordonnées tactiles dans LVGL
    if (!lvgl_port_add_touch(&touch_cfg)) return false;
    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------------
static bool lcd_init_brightness(Mylcd_t *Mylcd)
{
	if (!Mylcd) return false;
	
    // Configuration du timer PWM (fréquence, résolution)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,   // Mode de vitesse (LOW_SPEED recommandé pour BL)
        .timer_num        = LEDC_TIMER_0,          // Numéro de timer utilisé
        .duty_resolution  = LEDC_TIMER_8_BIT,      // Résolution sur 8 bits (0-255)
        .freq_hz          = 5000,                  // Fréquence PWM (5 kHz)
        .clk_cfg          = LEDC_AUTO_CLK          // Sélection automatique de l’horloge
    };
    if (ledc_timer_config(&ledc_timer) != ESP_OK) return false; // Retourne false en cas d’erreur

    // Configuration du canal PWM (GPIO, timer, canal, valeur initiale)
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = Mylcd->Mylcd_config->LCD_BL,          // GPIO utilisé pour le BL
        .speed_mode     = LEDC_LOW_SPEED_MODE,                  // Même mode que le timer
        .channel        = Mylcd->Mylcd_config->LCD_BL_CHANNEL,  // Canal utilisé
        .timer_sel      = LEDC_TIMER_0,                         // Timer associé
        .duty           = 255,                                  // Duty cycle initial (plein)
        .hpoint         = 0,                                    // Point de départ du signal
        .intr_type      = LEDC_INTR_DISABLE                     // Pas d’interruption
    };
    if (ledc_channel_config(&ledc_channel) != ESP_OK) return false; // Retourne false en cas d’erreur

    return true; // Initialisation réussie
}

// ----------------------------------------------------------------------------------------------------------------------------------
bool lcd_set_brightness(Mylcd_t *Mylcd, uint8_t brightness)
{
	if (!Mylcd) return false;
	
    // Met à jour le duty cycle du canal PWM pour régler la luminosité
    if (ledc_set_duty(LEDC_LOW_SPEED_MODE, Mylcd->Mylcd_config->LCD_BL_CHANNEL, brightness) != ESP_OK) return false;
    if (ledc_update_duty(LEDC_LOW_SPEED_MODE, Mylcd->Mylcd_config->LCD_BL_CHANNEL) != ESP_OK) return false;
    return true; // Modification réussie
}

// ----------------------------------------------------------------------------------------------------------------------------------
bool lcd_begin(Mylcd_t *Mylcd)
{
	if (!Mylcd) return false;
	
	// Initialisation du bus I2C pour le tactile (ft5x06)
	if (!lcd_i2c_confg(Mylcd)) return false;
	  
	// Initialisation du contrôleur tactile
	if (!lcd_touch_init(Mylcd)) return false; 
	
	// Initialisation du rétroéclairage (PWM LEDC)
	if (!lcd_init_brightness(Mylcd)) return false;

    // Initialisation LVGL et du port (tâche + timer)
    if (!lcd_lvgl_init()) return false;
    
    // Initialisation du bus SPI et de l'écran LCD
    if (!lcd_spi_confg(Mylcd)) return false;
    if (!lcd_panel_init(Mylcd)) return false;
	
    // Réglage initial de la luminosité
    if (!lcd_set_brightness(Mylcd, 90)) return false;
    
	// Liaison LVGL ↔ LCD
	if (!lcd_panel_link(Mylcd)) return false;
	
    // Liaison tactile ↔ LVGL ↔ LCD
    if (!lcd_touch_link(Mylcd)) return false;
    
    // iversion des couleurs a l'ecran
	esp_lcd_panel_invert_color(Mylcd->Mylcd_panel_handle, true);
	
    // Orientation miroir
    esp_lcd_panel_swap_xy(Mylcd->Mylcd_panel_handle, Mylcd->Mylcd_config->lcd_swap_xy);
	esp_lcd_panel_mirror(Mylcd->Mylcd_panel_handle, Mylcd->Mylcd_config->lcd_mirror_x, Mylcd->Mylcd_config->lcd_mirror_y);
	
	return true;
}

// ----------------------------------------------------------------------------------------------------------------------------------
bool lcd_stop(Mylcd_t *Mylcd)
{
    if (!Mylcd) return false;
 
    // Couper le rétroéclairage (0%)
    if (Mylcd->Mylcd_config) {
        // duty = 0 sur le canal configuré
        if (ledc_set_duty(LEDC_LOW_SPEED_MODE, Mylcd->Mylcd_config->LCD_BL_CHANNEL, 0) != ESP_OK) return false;
        if (ledc_update_duty(LEDC_LOW_SPEED_MODE, Mylcd->Mylcd_config->LCD_BL_CHANNEL) != ESP_OK) return false;
    }

    // Éteindre l’affichage (display off)
    if (Mylcd->Mylcd_panel_handle) {
        if (esp_lcd_panel_disp_on_off(Mylcd->Mylcd_panel_handle, false) != ESP_OK) return false;
    }
    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------
bool lcd_destroy(Mylcd_t *Mylcd)
{
    if (!Mylcd) return false;
    
    // Stop visuel d’abord (pas de flash blanc, etc.)
    if (!lcd_stop(Mylcd)) return false;

    // Arrête le port LVGL (tâche + tick timer) si utilisé
    if (lvgl_port_deinit()!= ESP_OK) return false;  // fourni par esp_lvgl_port
    
    // Détruire TACTILE
    if (Mylcd->Mylcd_touch_handle) {
        if (esp_lcd_touch_del(Mylcd->Mylcd_touch_handle) != ESP_OK) return false;
        Mylcd->Mylcd_touch_handle = NULL;
    }
    if (Mylcd->Mylcd_touch_io_handle) {
        // Rare en I2C, mais au cas où :
        if (esp_lcd_panel_io_del(Mylcd->Mylcd_touch_io_handle) != ESP_OK) return false;
        Mylcd->Mylcd_touch_io_handle = NULL;
    }

    // Détruire LCD / IO SPI
    if (Mylcd->Mylcd_panel_handle) {
        if (esp_lcd_panel_del(Mylcd->Mylcd_panel_handle) != ESP_OK) return false;
        Mylcd->Mylcd_panel_handle = NULL;
    }
    if (Mylcd->Mylcd_panel_io_handle) {
        if (esp_lcd_panel_io_del(Mylcd->Mylcd_panel_io_handle) != ESP_OK) return false;
        Mylcd->Mylcd_panel_io_handle = NULL;
    }

    // Couper PWM BL (met aussi le GPIO à 0)
    if (Mylcd->Mylcd_config) {
    if (ledc_stop(LEDC_LOW_SPEED_MODE, Mylcd->Mylcd_config->LCD_BL_CHANNEL, 0) != ESP_OK) return false;;
    }

    // Libérer BUS si c’est bien ce module qui les a initialisés
    if (Mylcd->Mylcd_config) {
    	if (spi_bus_free(Mylcd->Mylcd_config->LCD_HOST) != ESP_OK) return false;

    	if ( i2c_del_master_bus(Mylcd->bus_master_handle) != ESP_OK) return false;
    }
    return true;
}

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */	    
void lcd_build(Mylcd_t *Mylcd, Mylcd_dev_config_t *Mylcd_confg) {
	
	// Initialisation propre
	memset(Mylcd, 0, sizeof(Mylcd_t));
	
    // câblage des méthodes
    Mylcd->Begin          = lcd_begin;
    Mylcd->Set_brightness = lcd_set_brightness;
    Mylcd->Stop           = lcd_stop;
    Mylcd->Destroy        = lcd_destroy;

    // si on fournit une cfg dès la création, on l’applique
    Mylcd->Mylcd_config = (!Mylcd_confg) ? &Mylcd_default : Mylcd_confg;
}