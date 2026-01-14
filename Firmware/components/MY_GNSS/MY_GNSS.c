#include "MY_GNSS.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables interne :::::::::::::::::::::::::::::::::::::::::::::: */	    
Mygps_dev_config_t Mygps_default = {
    /* ---- UART ---- */
    .port_num = UART_NUM_1,                 /**< Port UART utilisé (ex: UART_NUM_1). */
    .uart_cfg = {
        .baud_rate  = 38400,                /**< Débit GNSS par défaut (adapte si besoin: 9600/38400/921600…). */
        .data_bits  = UART_DATA_8_BITS,     /**< 8 bits de données. */
        .parity     = UART_PARITY_DISABLE,  /**< Pas de parité. */
        .stop_bits  = UART_STOP_BITS_1,     /**< 1 bit de stop. */
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE, /**< Pas de RTS/CTS. */
        .source_clk = UART_SCLK_APB,        /**< Horloge APB (par défaut sous ESP-IDF). */
        .rx_flow_ctrl_thresh = 0,           /* Seuil FIFO RX pour RTS */
    },
	
    /* ---- Broches ---- */
    .rx_io    = GPIO_NUM_18,                /**< RX du MCU relié au TX du GNSS. */
    .tx_io    = GPIO_NUM_8,                 /**< TX du MCU relié au RX du GNSS. */
    .reset_io = GPIO_NUM_9,                 /**< Pas câblé -> GPIO_NUM_NC (ou mets la broche si dispo). */
    .boot_io  = GPIO_NUM_46,                /**< Pas câblé -> GPIO_NUM_NC (option de config GNSS). */

    /* ---- Buffers driver ---- */
    .rx_buf_size = 4096,                    /**< Buffer RX driver (gros pour NMEA en rafale). */
    .tx_buf_size = 0,                       /**< Buffer TX driver (petit suffit pour AT/NMEA). */

    /* ---- Tâche FreeRTOS ---- */
    .task_priority = 6,                     /**< Priorité de la tâche parser (ajuste selon charge). */
    .task_name     = "GPS_TASK"             /**< Nom lisible pour debug (<= configMAX_TASK_NAME_LEN). */
};

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions static :::::::::::::::::::::::::::::::::::::::::::::: */
static bool Mygps_uart_init(Mygps_t *Mygps)
{
	if(!Mygps) return false;
	
	/* ---- Verification du port ---- */
    if (Mygps->Mygps_config->port_num >= UART_NUM_MAX) return false; 
    
    /* ---- Applique les paramètres (bits de données, parité, stop, etc.) ---- */
    if (uart_param_config(Mygps->Mygps_config->port_num,
        &Mygps->Mygps_config->uart_cfg) != ESP_OK) return false;
    
    /* ---- Associe les broches TX/RX (RTS/CTS non utilisés) ---- */
    if (uart_set_pin(Mygps->Mygps_config->port_num, // PORT UART (0...2)
                     Mygps->Mygps_config->tx_io,   // TX GPIO
                     Mygps->Mygps_config->rx_io,   // RX GPIO
                     UART_PIN_NO_CHANGE,  // RTS
                     UART_PIN_NO_CHANGE)  // CTS
        != ESP_OK) {
        return false;
    }
    
    // Installe le driver UART avec buffers logiciels.
    // pas de file d’événements, pas d’IRQ personnalisé.
    if (uart_driver_install(Mygps->Mygps_config->port_num,  // Port matériel à utiliser (UART_NUM_x)
                                Mygps->Mygps_config->rx_buf_size, // Taille du ring buffer RX (octets) alloué par le driver.
                                Mygps->Mygps_config->tx_buf_size, // Taille du ring buffer TX (octets).
                                32,   // taille de la file d’événements
                                &Mygps->Mygps_uart_queue, // pointeur vers la file
                                0)   //  Flags d’allocation d’IRQ (0 = défaut)
            != ESP_OK) return false;
    // Active la détection du motif '\n' (1x), délai inter-caractères 21 bits (~0,55 ms @38400), sans pre/post idle ; retourne true si reussi.       
   	return (uart_enable_pattern_det_baud_intr(Mygps->Mygps_config->port_num, '\n', 
	                     1, 25, 0, 0) == ESP_OK); 
	uart_pattern_queue_reset(UART_NUM_1, 128);
	                     
   //uart_set_rx_full_threshold(Mygps->Mygps_config->port_num, 64); 
   //uart_set_rx_timeout(Mygps->Mygps_config->port_num, 80);                                          
   //return true;  
}	
	   
// --------------------------------------------------------------------------------------------------------------------------------------
bool Mygps_set_baud(Mygps_t *Mygps, uint32_t new_baud)
{
	if(!Mygps) return false;
	
	// Validation du port 
    if (Mygps->Mygps_config->port_num >= UART_NUM_MAX) return false;

    // changer le baud rate
    return (uart_set_baudrate(Mygps->Mygps_config->port_num, new_baud) == ESP_OK);
}

// --------------------------------------------------------------------------------------------------------------------------------------
bool Mygps_reset(Mygps_t *Mygps)
{
	if(!Mygps) return false;
	
	if (gpio_set_level(Mygps->Mygps_config->reset_io, 0)!= ESP_OK) return false;
    vTaskDelay(pdMS_TO_TICKS(10));
    if (gpio_set_level(Mygps->Mygps_config->reset_io, 1)!= ESP_OK)return false;
    return true;
}
 
// --------------------------------------------------------------------------------------------------------------------------------------
bool Mygps_boot(Mygps_t *Mygps)
{
	if(!Mygps) return false;
	
	if (gpio_set_level(Mygps->Mygps_config->boot_io, 0)!= ESP_OK) return false;
	vTaskDelay(pdMS_TO_TICKS(1));
	if (!Mygps_reset(Mygps))return false;
	if (gpio_set_level(Mygps->Mygps_config->boot_io, 1)!= ESP_OK) return false;
	return true;
} 

// -------------------------------------------------------------------------------------------------------------------------------
bool Mygps_set_gps_data(Mygps_t *Mygps, gps_data_t *new_data)
{
	if(!Mygps) return false;
	atomic_store_explicit(&Mygps->gps_data, *new_data, memory_order_release); // memory_order_seq_cst
	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------
gps_data_t Mygps_get_gps_data(Mygps_t *Mygps)
{
	return atomic_load_explicit(&Mygps->gps_data, memory_order_acquire); // memory_order_seq_cst
}

// --------------------------------------------------------------------------------------------------------------------------------
static uint8_t is_min(int a, int b) {
    int m = (a < b) ? a : b;   // minimum des deux
    return (m < 0) ? 1 : m;    // si min < 0 => 1, sinon min
}

// --------------------------------------------------------------------------------------------------------------------------------
static void strip_cr_from_sentence(char *sentence) {
	char *cursor = strchr(sentence, '\r');
	if (cursor) {*cursor = '\0';}
}

// --------------------------------------------------------------------------------------------------------------------------------
static void on_minmea_gga(gps_data_t *gps_data, char *sentence)
{
    if (!sentence || !gps_data) return;
    struct minmea_sentence_gga g;
    if (!minmea_parse_gga(&g, sentence)) return;
    
    gps_data->fix_quality   = g.fix_quality;   // 0=no fix, 1=GPS, 2=DGPS...
    gps_data->is_valid      = g.fix_quality > 0;
    gps_data->lat_deg       = minmea_tocoord(&g.latitude);
    gps_data->lon_deg       = minmea_tocoord(&g.longitude);
    gps_data->altitude      = minmea_tofloat(&g.altitude); // métres MSL
    gps_data->nb_sat_tracked = g.satellites_tracked;
}

// --------------------------------------------------------------------------------------------------------------------------------
static void on_minmea_rmc(gps_data_t *gps_data, char *sentence)
{
    if (!sentence || !gps_data) return;
    struct minmea_sentence_rmc r;
    if (!minmea_parse_rmc(&r, sentence)) return;

    gps_data->is_valid   = r.valid;
    gps_data->lat_deg    = minmea_tocoord(&r.latitude);
    gps_data->lon_deg    = minmea_tocoord(&r.longitude);
    gps_data->speed_kmh  = minmea_tofloat(&r.speed)* 1.852f;
    gps_data->date       = r.date;
    gps_data->time       = r.time;
}

// --------------------------------------------------------------------------------------------------------------------------------
static void on_minmea_vtg(gps_data_t *gps_data, char *sentence)
{
    if (!sentence || !gps_data) return;
    struct minmea_sentence_vtg v;
    if (!minmea_parse_vtg(&v, sentence)) return;
    gps_data->speed_kmh  = minmea_tofloat(&v.speed_kph);
}

// --------------------------------------------------------------------------------------------------------------------------------
static void on_minmea_gsa(gps_data_t *gps_data, char *sentence)
{
    if (!sentence || !gps_data) return;
    struct minmea_sentence_gsa s;
    if (!minmea_parse_gsa(&s, sentence)) return;
    gps_data->fix_type = s.fix_type;
}

// --------------------------------------------------------------------------------------------------------------------------------
static void on_minmea_gll(gps_data_t *gps_data, char *sentence)
{
    if (!sentence || !gps_data) return;
    struct minmea_sentence_gll g;
    if (!minmea_parse_gll(&g, sentence)) return;
    gps_data->lat_deg = minmea_tocoord(&g.latitude);
    gps_data->lon_deg = minmea_tocoord(&g.longitude);
      
}

// --------------------------------------------------------------------------------------------------------------------------------
static void on_minmea_zda(gps_data_t *gps_data, char *sentence)
{
    if (!sentence || !gps_data) return;
    struct minmea_sentence_zda z;
    if (!minmea_parse_zda(&z, sentence)) return;
	gps_data->date = z.date;
    gps_data->time = z.time;
}

// --------------------------------------------------------------------------------------------------------------------------------

static void gnss_route_minmea(gps_data_t *gps_data, char *sentence)
{
    if (!sentence) return;
    
    switch (minmea_sentence_id(sentence, true)) {
        case MINMEA_SENTENCE_RMC: on_minmea_rmc(gps_data, sentence); break;
        case MINMEA_SENTENCE_GGA: on_minmea_gga(gps_data, sentence); break;
        case MINMEA_SENTENCE_VTG: on_minmea_vtg(gps_data, sentence); break;
        case MINMEA_SENTENCE_GSA: on_minmea_gsa(gps_data, sentence); break;
        case MINMEA_SENTENCE_GLL: on_minmea_gll(gps_data, sentence); break;
        case MINMEA_SENTENCE_ZDA: on_minmea_zda(gps_data, sentence); break;
        //case MINMEA_SENTENCE_GSV: on_minmea_gsv(line, sat); break;
        case MINMEA_SENTENCE_GST: /* optionnel: précision */ break;
        case MINMEA_SENTENCE_GBS: /* optionnel: SBAS integrity */ break;
        case MINMEA_INVALID: break;
        case MINMEA_UNKNOWN: break; 
        default: break;
    }
}

// --------------------------------------------------------------------------------------------------------------------------------
void Mygps_parser_task(void *arguments) {
	
    //Récupération de l'instance passée à xTaskCreate()
    Mygps_t *Mygps = (Mygps_t *)arguments;
    configASSERT(Mygps != NULL); //Sécurité : arrêt en debug si l'instance est NULL

    // Variables locales
    char sentence[MINMEA_MAX_SENTENCE_LENGTH];     // Buffer pour ligne NMEA + 1 octet pour le '\0'
    uart_event_t event ;
    gps_data_t gps_data = (gps_data_t){0};   // Structure pour stocker les données GPS
   	
    while (1) {
 
			if (xQueueReceive(Mygps->Mygps_uart_queue, &event, portMAX_DELAY)) {
				if (event.type == UART_PATTERN_DET) {

					uint8_t pattern_index = is_min(uart_pattern_pop_pos(Mygps->Mygps_config->port_num)+1, MINMEA_MAX_SENTENCE_LENGTH);
					uart_read_bytes(Mygps->Mygps_config->port_num, (uint8_t *)sentence, pattern_index, 1);
					sentence[pattern_index-1] = '\0';
					strip_cr_from_sentence(sentence);	
					
					char *cursor = strchr(sentence, '$');
					if (cursor) {gnss_route_minmea(&gps_data, cursor); } 
					    }  
					}
					
        		 	Mygps->Set_gps_data(Mygps, &gps_data); //Mise à jour des données GPS
        		 	//xQueueReset(Mygps->Mygps_uart_queue);  
					//uart_flush_input(Mygps->Mygps_config->port_num);	 
					vTaskDelay(pdMS_TO_TICKS(1)); // Pause pour éviter la surcharge CPU	
					
			}
}
     
// --------------------------------------------------------------------------------------------------------------------------------
bool Mygps_start_task(Mygps_t *Mygps)
{
	if(!Mygps) return false;
	if (Mygps->Mygps_task_handle) return false; 
	
    BaseType_t ok = xTaskCreate( Mygps->Parser_task, // fonction
                 Mygps->Mygps_config->task_name, // nom
                 6144,  // stack
                 Mygps, // param
                 7,       // priorité
                 &Mygps->Mygps_task_handle ); // handle de sortie         
   return (ok == pdPASS);   
}

// --------------------------------------------------------------------------------------------------------------------------------
bool Mygps_kill_task(Mygps_t *Mygps)
{
	if(!Mygps) return false;
	if (!Mygps->Mygps_task_handle) return false;
    vTaskDelete(Mygps->Mygps_task_handle); // supprime la tâche ciblée
    Mygps->Mygps_task_handle = NULL;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------------------
bool Mygps_stop_task(Mygps_t *Mygps)
{
	if(!Mygps) return false;
	vTaskSuspend(Mygps->Mygps_task_handle);
	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------
bool Mygps_resume_task(Mygps_t *Mygps)
{
	if(!Mygps) return false;
	vTaskResume(Mygps->Mygps_task_handle);
	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------
bool Mygps_destroy(Mygps_t *Mygps)
{
	if(!Mygps) return false;
	if(!Mygps_kill_task(Mygps)) return false;
	return ( uart_driver_delete(Mygps->Mygps_config->port_num) == ESP_OK );
}

// --------------------------------------------------------------------------------------------------------------------------------
 bool Mygps_begin(Mygps_t *Mygps)
 {
	if(!Mygps) return false;
	if (!Mygps_uart_init(Mygps)) return false;
	gpio_set_direction(Mygps->Mygps_config->reset_io, GPIO_MODE_OUTPUT);
	gpio_set_direction(Mygps->Mygps_config->boot_io, GPIO_MODE_OUTPUT);
	if (gpio_set_level(Mygps->Mygps_config->boot_io, 1)!= ESP_OK)return false;
	vTaskDelay(pdMS_TO_TICKS(10));
	if (gpio_set_level(Mygps->Mygps_config->reset_io, 1)!= ESP_OK)return false;
	return true;
}

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */	    
void gps_build(Mygps_t *Mygps, Mygps_dev_config_t *Mygps_confg)
{
    // Remet tout l’objet à zéro (membres/handles) pour partir d’un état propre.
    memset(Mygps, 0, sizeof(Mygps_t));

    // Renseigne les pointeurs de fonctions (méthodes “virtuelles” de l’objet).
    Mygps->Begin        = Mygps_begin;        // initialise le module (UART, etc.)
    Mygps->Boot         = Mygps_boot;         // séquence boot/boot-pin si dispo
    Mygps->Reset        = Mygps_reset;        // reset matériel si câblé
    Mygps->Set_baud     = Mygps_set_baud;     // Modification, vitesse du port UART
    Mygps->Get_gps_data = Mygps_get_gps_data; // lecteur du snapshot GNSS
    Mygps->Set_gps_data = Mygps_set_gps_data; // écrivain du snapshot GNSS
    Mygps->Parser_task  = Mygps_parser_task;  // tâche FreeRTOS de parsing NMEA
    Mygps->Start_task   = Mygps_start_task;   // crée/lance la tâche parser
    Mygps->Kill_task    = Mygps_kill_task;    // tue la tâche parser (hard stop)
    Mygps->Stop_task    = Mygps_stop_task;    // suspend/arrête proprement
    Mygps->Resume_task  = Mygps_resume_task;  // reprend après stop/suspend
    Mygps->Destroy      = Mygps_destroy;      // libère/ferme ce qui doit l’être

    // Choisit la configuration : si NULL, utiliser la config par défaut statique.
    Mygps->Mygps_config = (!Mygps_confg) ? &Mygps_default : Mygps_confg;
}

// -------------------------------------------------------------------------------------------------------------------------------------

