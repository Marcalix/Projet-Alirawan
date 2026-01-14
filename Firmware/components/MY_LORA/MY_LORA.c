#include "MY_LORA.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables interne :::::::::::::::::::::::::::::::::::::::::::::: */	    
MyLoRa_dev_config_t MyLoRa_default = 
{
    /* ---- UART ---- */
    .port_num = UART_NUM_2,                 /**< Port UART utilisé (ex: UART_NUM_2). */
    .uart_cfg = {
        .baud_rate  = 115200,               /**< Débit GNSS par défaut (adapte si besoin: 9600/38400/921600…). */
        .data_bits  = UART_DATA_8_BITS,     /**< 8 bits de données. */
        .parity     = UART_PARITY_DISABLE,  /**< Pas de parité. */
        .stop_bits  = UART_STOP_BITS_1,     /**< 1 bit de stop. */
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE, /**< Pas de RTS/CTS. */
        .source_clk = UART_SCLK_APB,        /**< Horloge APB (par défaut sous ESP-IDF). */
        .rx_flow_ctrl_thresh = 0,           /* Seuil FIFO RX pour RTS */
    },
	
    /* ---- Broches ---- */
    .rx_io    = GPIO_NUM_21,                /**< RX du MCU relié au TX du GNSS. */
    .tx_io    = GPIO_NUM_14,                /**< TX du MCU relié au RX du GNSS. */
    .reset_io = GPIO_NUM_48,                /**< Pas câblé -> GPIO_NUM_NC (ou mets la broche si dispo). */
    .boot_io  = GPIO_NUM_47,                /**< Pas câblé -> GPIO_NUM_NC (option de config GNSS). */

    /* ---- Buffers driver ---- */
    .rx_buf_size = 1024,                    /**< Buffer RX driver (gros pour NMEA en rafale). */
    .tx_buf_size = 1024,                    /**< Buffer TX driver (petit suffit pour AT/NMEA). */

};
  
/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions static :::::::::::::::::::::::::::::::::::::::::::::: */
static bool MyLoRa_uart_init(MyLoRa_t *MyLoRa)
{
	if(!MyLoRa) return false;
	
	/* ---- Verification du port ---- */
    if (MyLoRa->MyLoRa_config->port_num >= UART_NUM_MAX) return false; 
    
    /* ---- Applique les paramètres (bits de données, parité, stop, etc.) ---- */
    if (uart_param_config(MyLoRa->MyLoRa_config->port_num,
        &MyLoRa->MyLoRa_config->uart_cfg) != ESP_OK) return false;
    
    /* ---- Associe les broches TX/RX (RTS/CTS non utilisés) ---- */
    if (uart_set_pin(MyLoRa->MyLoRa_config->port_num, // PORT UART (0...2)
                     MyLoRa->MyLoRa_config->tx_io,   // TX GPIO
                     MyLoRa->MyLoRa_config->rx_io,   // RX GPIO
                     UART_PIN_NO_CHANGE,  // RTS
                     UART_PIN_NO_CHANGE)  // CTS
        != ESP_OK) {
        return false;
    }
    
    // Installe le driver UART avec buffers logiciels.
    // pas de file d’événements, pas d’IRQ personnalisé.
    return (uart_driver_install(MyLoRa->MyLoRa_config->port_num,  // Port matériel à utiliser (UART_NUM_x)
                                MyLoRa->MyLoRa_config->rx_buf_size, // Taille du ring buffer RX (octets) alloué par le driver.
                                MyLoRa->MyLoRa_config->tx_buf_size, // Taille du ring buffer TX (octets).
                                0,   // taille de la file d’événements 16
                                NULL, //&MyLoRa->MyLoRa_uart_queue, // pointeur vers la file
                                0)   //  Flags d’allocation d’IRQ (0 = défaut)
            == ESP_OK);  
}	
	   
// --------------------------------------------------------------------------------------------------------------------------------------
bool MyLoRa_set_baud(MyLoRa_t *MyLoRa, uint32_t new_baud)
{
	if(!MyLoRa) return false;
	
	// Validation du port 
    if (MyLoRa->MyLoRa_config->port_num >= UART_NUM_MAX) return false;

    // changer le baud rate
    return (uart_set_baudrate(MyLoRa->MyLoRa_config->port_num, new_baud) == ESP_OK);
}

// --------------------------------------------------------------------------------------------------------------------------------------
bool MyLoRa_reset(MyLoRa_t *MyLoRa)
{
	if(!MyLoRa) return false;
	
	if (gpio_set_level(MyLoRa->MyLoRa_config->reset_io, 0)!= ESP_OK) return false;
    vTaskDelay(pdMS_TO_TICKS(20));
    if (gpio_set_level(MyLoRa->MyLoRa_config->reset_io, 1)!= ESP_OK)return false;
    return true;
}
 
// --------------------------------------------------------------------------------------------------------------------------------------
bool MyLoRa_boot(MyLoRa_t *MyLoRa)
{
	if(!MyLoRa) return false;
	
	if (gpio_set_level(MyLoRa->MyLoRa_config->boot_io, 1)!= ESP_OK) return false;
	vTaskDelay(pdMS_TO_TICKS(1));
	if (!MyLoRa_reset(MyLoRa))return false;
	if (gpio_set_level(MyLoRa->MyLoRa_config->boot_io, 0)!= ESP_OK) return false;
	return true;
} 

// --------------------------------------------------------------------------------------------------------------------------------
static void strip_cr_from_sentence(char *sentence) 
{
	char *cursor = strchr(sentence, '\r');
	if (cursor) {*cursor = '\0';}
}

// --------------------------------------------------------------------------------------------------------------------------------
static bool uart_write(MyLoRa_t *MyLoRa, const char *cmd)
{
	// Verification de la saisie
    if (!MyLoRa)  return false;
    
    // Vider le buffer de lecture
    uart_flush_input(MyLoRa->MyLoRa_config->port_num);
    
    // uart_write_bytes renvoie le nombre d’octets copiés dans le buffer TX ou <0 en erreur
    return ( uart_write_bytes(MyLoRa->MyLoRa_config->port_num, cmd, strlen(cmd)) == strlen(cmd) );
}

// ---------------------------------------------------------------------------------------------------------------------------------
static str512_t uart_read(MyLoRa_t *MyLoRa, bool skip_first_line)
{ 
    // Declaration variable local
    str512_t read = {0};
    int len = uart_read_bytes(MyLoRa->MyLoRa_config->port_num, (uint8_t *)read.sentence, sizeof(read.sentence), pdMS_TO_TICKS(10));
    if (len > 0)
        {
            read.sentence[len] = '\0'; // Assure la terminaison
            char *pattern  = strchr(read.sentence, '\n');   // verifie la presence d'un retour a la ligne
  			if(pattern && skip_first_line) {strcpy(read.sentence, pattern+1);} // recupere les elements apres le pattern
  			
            strip_cr_from_sentence(read.sentence); // Nettoyage éventuel
        }
    return read; // Timeout
}				
// ---------------------------------------------------------------------------------------------------------------------------------
str512_t MyLoRa_send_cmd(MyLoRa_t *MyLoRa, const char *cmd, bool skip_first_line, uint32_t timeout_ms)
{
	uart_write(MyLoRa, cmd);
	vTaskDelay(pdMS_TO_TICKS(20+timeout_ms));
	return uart_read(MyLoRa, skip_first_line);
}
	
// --------------------------------------------------------------------------------------------------------------------------------
bool MyLoRa_destroy(MyLoRa_t *MyLoRa)
{
	if(!MyLoRa) return false;
	return ( uart_driver_delete(MyLoRa->MyLoRa_config->port_num) == ESP_OK );
}

// --------------------------------------------------------------------------------------------------------------------------------
 bool MyLoRa_begin(MyLoRa_t *MyLoRa)
 {
	if(!MyLoRa) return false;
	if (!MyLoRa_uart_init(MyLoRa)) return false;
	gpio_set_direction(MyLoRa->MyLoRa_config->reset_io, GPIO_MODE_OUTPUT);
	gpio_set_direction(MyLoRa->MyLoRa_config->boot_io, GPIO_MODE_OUTPUT);
	if (gpio_set_level(MyLoRa->MyLoRa_config->boot_io, 0)!= ESP_OK)return false;
	vTaskDelay(pdMS_TO_TICKS(20));
	if (gpio_set_level(MyLoRa->MyLoRa_config->reset_io, 1)!= ESP_OK)return false;
	return true;
}

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */	    
void LoRa_build(MyLoRa_t *MyLoRa, MyLoRa_dev_config_t *MyLoRa_confg)
{
	if (!MyLoRa) return;
	
    // Remet tout l’objet à zéro (membres/handles) pour partir d’un état propre.
    memset(MyLoRa, 0, sizeof(MyLoRa_t));

    // Renseigne les pointeurs de fonctions (méthodes “virtuelles” de l’objet).
    MyLoRa->Begin        = MyLoRa_begin;        // initialise le module (UART, etc.)
    MyLoRa->Boot         = MyLoRa_boot;         // séquence boot/boot-pin si dispo
    MyLoRa->Reset        = MyLoRa_reset;        // reset matériel si câblé
    MyLoRa->Set_baud     = MyLoRa_set_baud;     // Modification, vitesse du port UART
    MyLoRa->Send_command = MyLoRa_send_cmd;     // Envoie une commande puis recupere les informations
    MyLoRa->Destroy      = MyLoRa_destroy;      // libère/ferme ce qui doit l’être

    // Choisit la configuration : si NULL, utiliser la config par défaut statique.
    MyLoRa->MyLoRa_config = (!MyLoRa_confg) ? &MyLoRa_default : MyLoRa_confg;
    
    // Renseigne les pointeurs de fonctions (méthodes “virtuelles” de l’objet).
    Gloabal_request_build(MyLoRa);
    
    // Renseigne les pointeurs de fonctions (méthodes “virtuelles” de l’objet).
    LoRaWAN_request_build(MyLoRa);
}

// -------------------------------------------------------------------------------------------------------------------------------------

