#include "I2C.h"
#include <strings.h>

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables globale :::::::::::::::::::::::::::::::::::::::::::::: */
i2c_master_bus_handle_t bus_handle_0;  // Handle pour le bus I2C “numéro 0”
i2c_master_bus_handle_t bus_handle_1;  // Handle pour le bus I2C “numéro 1”

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables interne :::::::::::::::::::::::::::::::::::::::::::::: */	    
	    
	    
/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions :::::::::::::::::::::::::::::::::::::::::::::: */
bool i2c_master_init(bool port, gpio_num_t sda_io, gpio_num_t scl_io)
{ 
	//Construction de la structure de configuration du bus I²C maître
    i2c_master_bus_config_t bus_cfg = {
		.clk_source = I2C_CLK_SRC_DEFAULT,                // Source d’horloge I²C
    	.i2c_port = (port == 0) ? I2C_NUM_0 : I2C_NUM_1,  // numéro de port, si port == 0 → I2C_NUM_0 ; si port == 1 → I2C_NUM_1 
    	.scl_io_num = scl_io,							  // Broche GPIO à utiliser en tant que SCL
    	.sda_io_num = sda_io,							  // Broche GPIO à utiliser en tant que SDA
    	.glitch_ignore_cnt = 7,							  // Nombre de cycles “ignorés” en cas de glitch (parasites courts)
    	.flags.enable_internal_pullup = true, 			  // enable_internal_pullup → active les pull-up internes sur SDA/SCL
    	.flags.allow_pd = false,               			  // autorise la mise en veille profonde (power-down) du bloc I²C
    };
    
    //Création/réservation du bus I²C maître
    //Si i2c_new_master_bus renvoie une erreur,on considère que l’initialisation a échoué → on renvoie false.
   	if ((esp_err_t *)i2c_new_master_bus(&bus_cfg, (port == 0)? &bus_handle_0 : &bus_handle_1) != ESP_OK) return false;
	
	// Si on arrive ici, c’est que ESP_OK a été retourné → initialisation réussie
	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------
bool i2c_conf_device(bool port, uint8_t address, uint32_t frequence, i2c_master_dev_handle_t *dev_handle)
{
	//Préparation de la configuration du périphérique I²C 
	i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7, // longueur de l’adresse (valeurs possibles : 7 ou 10)
    .device_address  = address,			   // adresse 7 bits de l’esclave 
    .scl_speed_hz    = frequence,		   // vitesse de bus (par ex. 100000 pour 100 kHz, ou 400000 pour 400 kHz)
    .scl_wait_us     = 0                   // délai d’attente en µs avant de forcer un NACK 
	};	
	
	//Ajout du périphérique sur le bus sélectionné, retourne un esp_err_t (ESP_OK en cas de succès)
	if ((esp_err_t *)i2c_master_bus_add_device((port == 0)? bus_handle_0 : bus_handle_1, &dev_cfg, dev_handle)!= ESP_OK) return false;
	
	// Si on arrive ici, c’est que ESP_OK a été retourné → initialisation réussie
	return true;
}

// -------------------------------------------------------------------------------------------------------------------------------
uint8_t *i2c_scan(bool port)
{
	
	// Tableau statique pour stocker les adresses trouvées + un compteur
	static uint8_t find_addr[I2C_SCAN_NUM_MAX + 1] = {0};
	  
	// On balaie la plage d'adresses I²C "utilisables" (3 à 118 inclus)
    for (uint8_t addr = 3; addr < 119; addr++) {
		
		// On lance une transmission "vide" (aucune donnée, NULL/0) vers cette adresse.
        // Si un esclave existe à cette adresse, il répondra par un ACK, et err == ESP_OK.
		esp_err_t err = i2c_master_probe((port == 0)? bus_handle_0 : bus_handle_1, addr, -1);
        
        // Si le dernier élément (find_addr[I2C_SCAN_NUM_MAX]) a déjà atteint la
        // capacité maximale (I2C_SCAN_NUM_MAX), on arrête la boucle pour éviter un débordement.
        if (find_addr[I2C_SCAN_NUM_MAX] >= I2C_SCAN_NUM_MAX) break;

		// Si la transmission a réussi (ESP_OK), on stocke l'adresse dans le tableau
        // et on incrémente le compteur (stocké dans find_addr[I2C_SCAN_NUM_MAX]).
        if (err == ESP_OK) find_addr [ find_addr[I2C_SCAN_NUM_MAX]++ ] = addr;  
                
    }
    
    // On renvoie l'adresse du tableau statique. Le tableau contient :
    return find_addr;
}

// ------------------------------------------------------------------------------------------------------------------------------
bool i2c_master_deinit(bool port)
{	 
	// Supprime la configuration du bus et retourne true si l'action est effectuée
    return ( (esp_err_t *) i2c_del_master_bus((port == 0)? bus_handle_0 : bus_handle_1) == ESP_OK ) ; 
}

// ----------------------------------------------------------------------------------------------------------------------------------
bool i2c_writeString(i2c_master_dev_handle_t dev_handle, char *data, size_t data_s, uint32_t timeout_ms)
{
	esp_err_t err = i2c_master_transmit(dev_handle, (uint8_t*)data, data_s, timeout_ms);
    return (err == ESP_OK);
}

// ----------------------------------------------------------------------------------------------------------------------------------
bool i2c_writeBytes(i2c_master_dev_handle_t dev_handle, uint8_t *data, size_t data_s, uint32_t timeout_ms)
{
	esp_err_t err = i2c_master_transmit(dev_handle, data, data_s, timeout_ms);
    return (err == ESP_OK);
}

// ----------------------------------------------------------------------------------------------------------------------------------
bool i2c_readString(i2c_master_dev_handle_t dev_handle, char *data, size_t data_s, uint32_t timeout_ms)
{
	esp_err_t err = i2c_master_receive(dev_handle, (uint8_t*)data, data_s, timeout_ms);
    return (err == ESP_OK);
}

// ----------------------------------------------------------------------------------------------------------------------------------
bool i2c_readBytes(i2c_master_dev_handle_t dev_handle, uint8_t *data, size_t data_s, uint32_t timeout_ms)
{
    esp_err_t err = i2c_master_receive(dev_handle, data, data_s, timeout_ms);
    return (err == ESP_OK);
}

// ----------------------------------------------------------------------------------------------------------------------------------
bool i2c_writeCommand(i2c_master_dev_handle_t dev_handle, char *cmd, size_t cmd_s, char *data, size_t data_s, uint32_t timeout_ms)
{
  esp_err_t err = i2c_master_transmit_receive(dev_handle, (uint8_t*)cmd, cmd_s, (uint8_t*)data, data_s, timeout_ms);
  return (err == ESP_OK);
}
// ----------------------------------------------------------------------------------------------------------------------------------

