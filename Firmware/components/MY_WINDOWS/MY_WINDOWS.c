#include <stdio.h>
#include "MY_WINDOWS.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables interne :::::::::::::::::::::::::::::::::::::::::::::: */	    


/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions static :::::::::::::::::::::::::::::::::::::::::::::: */
	
	   
// --------------------------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------------------

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */	    

//void gauge_build(Mygauge_t *Mygauge, Mygauge_dev_config_t *Mygauge_confg)
//{
//    // Sécurités basiques : en debug, on stoppe si pointeur invalide.
//    configASSERT(Mygauge != NULL);
//
//    // Remet tout l’objet à zéro (handles, états, buffers…)
//    //    => évite de réutiliser des handles I2C/RTOS périmés.
//    memset(Mygauge, 0, sizeof(Mygauge_t));
//
//    //Renseigne les "méthodes" (pointeurs de fonctions).
//    //    Chaque fonction est responsable de ses vérifs d'arguments.
//    Mygauge->Begin                  = Mygauge_begin;               // Init matériel (I2C, device, etc.)
//    Mygauge->Reset                  = Mygauge_reset;               // Reset matériel si la broche est câblée
//    Mygauge->get_version            = Mygauge_get_version;         // Lit la version/ID du composant
//    Mygauge->get_percent            = Mygauge_get_percent;         // Pourcentage d'état de charge (SOC)
//    Mygauge->get_voltage            = Mygauge_get_voltage;         // Tension batterie (V)
//    Mygauge->get_charge_rate        = Mygauge_get_charge_rate;     // Taux de charge / courant estimé
//    Mygauge->set_temp_compensation  = Mygauge_set_temp_compensation; // Ajuste la compensation en température
//
//    // Choisit la configuration matérielle.
//    //    - Si l'appelant ne fournit rien, on bascule sur la config statique par défaut.
//    //    - Sinon on référence la config fournie (non copiée).
//    //      => responsabilité de l'appelant de maintenir sa durée de vie.
//    Mygauge->Mygauge_config = (Mygauge_confg == NULL) ? &Mygauge_default : Mygauge_confg;
//
//    // (Optionnel) 4) Pré-initialiser des champs d'état/logiques si nécessaire.
//    // ex: Mygauge->initialized = false; Mygauge->timestamp_ms = 0; etc.
//}

// -------------------------------------------------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------------------------------------//
//#include "driver/sdspi_host.h"
//#include "esp_lcd_touch.h"
//#include "esp_lcd_types.h"
//#include "hal/ledc_types.h"
//#include "lvgl.h"
//#include "esp_lvgl_port.h"
//#include "esp_lcd_panel_ops.h"
//#include "esp_lcd_panel_vendor.h"
//#include "esp_lcd_st7796.h"
//#include "driver/spi_master.h"
//#include "driver/gpio.h"
//#include "misc/lv_area.h"
//#include "misc/lv_color.h"
//#include "esp_lcd_touch_ft5x06.h"
//#include <math.h>
//#include <stdint.h>
//#include <string.h>
//
//#include "MY_GNSS.h"
//#include "MY_LCD.h"
//#include "MY_LORA.h"
//#include "PAYLOAD.h"
//#include "themes/lv_theme.h"
//#include "widgets/textarea/lv_textarea.h"
//
//
// //-----------------------------------------------------------------------------------------------------------------------------
// //   ----------------------------------------------------------------------------------------------------------   
///* ====== Handle de la task ====== */
//static TaskHandle_t sApplyTaskHandle = NULL;
//
///* ====== Paramètres ====== */
//#define APPLY_TASK_NAME      "ApplyTask"
//#define APPLY_TASK_STACK     (8*1024)          // adapte si besoin
//#define APPLY_TASK_PRIO      3
//#define APPLY_TASK_PERIOD_MS 500               // rafraîchissement UI
//
//// ----------------------------------------------------------
//Mylcd_t Mylcd;
//Mygps_t Mygps;
//MyLoRa_t Mylora;
//Payload_t payload;
//
//char cmd[128];
//char back[512];
//bool on_1 = false;
//bool on_2 = false;
///* --- Pointeurs UI (statiques) ------------------------------------------------ */
//static lv_obj_t *lbl_lat;
//static lv_obj_t *lbl_lon;
//static lv_obj_t *lbl_rssi;
//static lv_obj_t *lbl_snr;
//static lv_obj_t *btn_start;
//static lv_obj_t *btn_stop;
//static lv_obj_t *sw_1;
//static lv_obj_t *sw_2;
//static lv_obj_t *ta_out;
//
///* --- Callbacks externes (à implémenter côté appli si besoin) ----------------- */
///* Ex.: démarrer/arrêter acquisition, activer/desactiver options */
//__attribute__((weak)) void app_on_start(void) {}
//__attribute__((weak)) void app_on_stop(void) {}
//__attribute__((weak)) void app_on_switch_1(bool on) { (void)on; }
//__attribute__((weak)) void app_on_switch_2(bool on) { (void)on; }
//
//
///* --- API d’update (à appeler depuis tes tâches) ------------------------------ */
//void ui_set_gps(double lat_deg, double lon_deg){
//    /* Thread-safety LVGL : appeler depuis le contexte LVGL (ou protégé par mutex/timer) */
//    char buf[64];
//	if (lvgl_port_lock(pdMS_TO_TICKS(1))) 
//	{ 
//    snprintf(buf, sizeof(buf), "Lat : %.6f°", lat_deg);
//    lv_label_set_text(lbl_lat, buf);
//
//    snprintf(buf, sizeof(buf), "Long: %.6f°", lon_deg);
//    lv_label_set_text(lbl_lon, buf);
//    lvgl_port_unlock(); 
//    }
//}
//
//void ui_set_lora_metrics(int rssi_dbm, float snr_db){
//    char buf[48];
//    if (lvgl_port_lock(pdMS_TO_TICKS(1))) 
//	{ 
//    snprintf(buf, sizeof(buf), "RSSI: %d dBm", rssi_dbm);
//    lv_label_set_text(lbl_rssi, buf);
//
//    snprintf(buf, sizeof(buf), "SNR : %.1f dB", snr_db);
//    lv_label_set_text(lbl_snr, buf);
//    lvgl_port_unlock(); 
//    }
//}
//
//void ui_add_text(char *text){
//	if (lvgl_port_lock(pdMS_TO_TICKS(1))) 
//	{
//	 lv_textarea_add_text(ta_out, text);
//	 lv_textarea_add_text(ta_out, "\n");	
//	 lvgl_port_unlock(); 
//    }
//}
//
///* Optionnel: lire l’état des switches depuis l’appli */
//bool ui_get_switch1(void){ return lv_obj_has_state(sw_1, LV_STATE_CHECKED); }
//bool ui_get_switch2(void){ return lv_obj_has_state(sw_2, LV_STATE_CHECKED); }
//
///* --- Fonction de Test -------------------------------------------------------- */
///* ====== Task fonction ====== */
//static void ApplyTask(void *arg)
//{
//    ui_add_text("===========START================");
//    gps_data_t gps = {};
//    vTaskDelay(pdMS_TO_TICKS(200));
//    
//    
//    while(!Mylora.Config.AT_check(&Mylora)){ ui_add_text(" AT LINK TEST"); } ;
//    ui_add_text(" AT LINK OK");
//    Mylora.LoRaWAN.Enable_Confirm_mode(&Mylora, on_1);
//    Mylora.LoRaWAN.Enable_ADR(&Mylora, on_2);
//    
//    Mylora.LoRaWAN.is_ADR(&Mylora)?ui_add_text("ADR ENABLE") : ui_add_text("NOT ENABLE");
//	
//	Mylora.LoRaWAN.is_Confirm_mode(&Mylora)?ui_add_text("CFM ENABLE") : ui_add_text("NOT ENABLE");
//	
//    while(!Mylora.LoRaWAN.Join(&Mylora)) { ui_add_text(" JOIN LINK TEST");};
//    ui_add_text(" JOIN LINK OK");
//    
//		
//    strcpy(back, Mylora.Send_command(&Mylora, "AT+SEND=2:000000\r\n", true, 18000).sentence);
//    ui_add_text(back);
//    while((strcasecmp(back, "+EVT:SEND_CONFIRMED_OK") != 0) && (strcasecmp(back, "+EVT:TX_DONE") != 0)) 
//    	{ 
//		strcpy(back, Mylora.Send_command(&Mylora, "AT+SEND=2:000000\r\n", true, 18000).sentence); 
//		ui_add_text(back);
//		}
//		
//    for (;;) {
//		
//        /* 1) Sortie propre si notification reçue */
//        if (ulTaskNotifyTake(pdTRUE, 0) > 0) { break;}
//
//        /* 2) Lire GPS une seule fois par tour */
//        gps = Mygps.Get_gps_data(&Mygps);
//        ui_set_gps( gps.lat_deg, gps.lon_deg);
//        while (!gps.is_valid) 
//        {
//			gps = Mygps.Get_gps_data(&Mygps); ui_set_gps( gps.lat_deg, gps.lon_deg);
//			//ui_add_text("GNSS LOSS");
//			vTaskDelay(pdMS_TO_TICKS(50));
//		}
//		
//        int rssi = Mylora.LoRaWAN.get_rssi(&Mylora);
//        int snr = Mylora.LoRaWAN.get_snr(&Mylora);
//        ui_set_lora_metrics(rssi, snr);
//        
//      	Mylora.LoRaWAN.Send_Payload(&Mylora, payload.custom_encode(gps.lat_deg, gps.lon_deg, rssi, snr).sentence, 2) ?ui_add_text("ENVOIE OK") : ui_add_text("ENVOIE ECHEC ");
//      	 
//        /* 3) Délai : laisse respirer le CPU/LVGL */
//        vTaskDelay(pdMS_TO_TICKS(APPLY_TASK_PERIOD_MS));
//    }
//
//    /* Nettoyage si besoin… */
//    /* Fin de task */
//    TaskHandle_t me = sApplyTaskHandle;
//    sApplyTaskHandle = NULL;                 // indique que la task est morte
//    vTaskDelete(NULL);
//}
//
///* ====== Create (xTaskCreate wrapper) ====== */
//bool ApplyTask_Start(void)
//{
//    if (sApplyTaskHandle) return true;       // déjà lancée
//    BaseType_t ok = xTaskCreate(
//        ApplyTask,
//        APPLY_TASK_NAME,
//        APPLY_TASK_STACK,
//        NULL,
//        APPLY_TASK_PRIO,
//        &sApplyTaskHandle
//    );
//    return ok == pdPASS;
//}
//
///* ====== Delete / stop propre ====== */
//void ApplyTask_Stop(void)
//{
//    TaskHandle_t h = sApplyTaskHandle;
//    if (!h) return;                          // pas lancée
//    /* Réveille la task et demande l’arrêt */
//    xTaskNotifyGive(h);
//
//    /* Attendre qu’elle se termine et remette le handle à NULL */
//    /* (non bloquant, boucle courte) */
//    for (int i = 0; i < 100 && sApplyTaskHandle; ++i) {
//        vTaskDelay(pdMS_TO_TICKS(10));
//    }
//
//    /* Si elle n’a pas fini, on force (rare) */
//    if (sApplyTaskHandle) {
//        vTaskDelete(sApplyTaskHandle);
//        sApplyTaskHandle = NULL;
//    }
//    if (lvgl_port_lock(pdMS_TO_TICKS(1))) {
//    lv_label_set_text(lbl_lat, "Lat : --.------°");
//    lv_label_set_text(lbl_lon, "Long: --.------°");
//    lv_label_set_text(lbl_rssi, "RSSI: -- dBm");
//    lv_label_set_text(lbl_snr,  "SNR : --.- dB");
//    lvgl_port_unlock(); 
//    }
//}
///* --- Event handlers ---------------------------------------------------------- */
//
//static void on_btn_start(lv_event_t *e){
//    (void)e;
//    //app_on_start();
//    ApplyTask_Start();
//}
//
//static void on_btn_stop(lv_event_t *e){
//    (void)e;
//    //app_on_stop();
//    ApplyTask_Stop();
//}
//
//static void on_sw_1(lv_event_t *e){
//    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
//    on_1 = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
//    //app_on_switch_1(on);
//}
//
//static void on_sw_2(lv_event_t *e){
//    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
//    on_2 = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
//    //app_on_switch_2(on);
//}
//
///* --- Création de l’interface ------------------------------------------------- */
//
//void ui_create_lora_gps_screen(void)
//{
//    /* ===== Screen root ===== */
//    lv_obj_t *scr = lv_obj_create(NULL);
//    lv_scr_load(scr);
//
//    /* Layout compact 320x480, colonne */
//    const int FOOTER_H = 56;                /* hauteur du footer overlay */
//    lv_obj_set_size(scr, 320, 480);
//    lv_obj_set_style_pad_all(scr, 6, 0);
//    lv_obj_set_style_pad_bottom(scr, FOOTER_H + 6, 0); /* évite recouvrement par footer */
//    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
//    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
//
//    /* ===== Header ===== */
//    lv_obj_t *hdr = lv_obj_create(scr);
//    lv_obj_set_size(hdr, 308, LV_SIZE_CONTENT);
//    lv_obj_set_style_pad_all(hdr, 6, 0);
//    lv_obj_set_style_radius(hdr, 8, 0);
//    lv_obj_set_style_bg_opa(hdr, LV_OPA_20, 0);
//    lv_obj_set_style_bg_color(hdr, lv_palette_main(LV_PALETTE_BLUE), 0);
//    lv_obj_set_style_border_width(hdr, 0, 0);
//    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
//    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
//    lv_label_set_text(lv_label_create(hdr), "Altirawan: GPS & LoRa");
//
//    /* ===== Card GPS (compact) ===== */
//    lv_obj_t *card_gps = lv_obj_create(scr);
//    lv_obj_set_size(card_gps, 308, LV_SIZE_CONTENT);
//    lv_obj_set_style_pad_all(card_gps, 4, 0);
//    lv_obj_set_style_radius(card_gps, 8, 0);
//    lv_obj_set_style_bg_opa(card_gps, LV_OPA_10, 0);
//    lv_obj_set_style_bg_color(card_gps, lv_palette_main(LV_PALETTE_GREEN), 0);
//    lv_obj_set_style_border_width(card_gps, 0, 0);
//    lv_obj_set_style_pad_row(card_gps, 2, 0);
//    lv_obj_set_flex_flow(card_gps, LV_FLEX_FLOW_COLUMN);
//    lv_label_set_text(lv_label_create(card_gps), "GPS");
//    lbl_lat = lv_label_create(card_gps);  lv_label_set_text(lbl_lat, "Lat : --.------°");
//    lbl_lon = lv_label_create(card_gps);  lv_label_set_text(lbl_lon, "Long: --.------°");
//
//    /* ===== Card LoRa (compact) ===== */
//    lv_obj_t *card_lora = lv_obj_create(scr);
//    lv_obj_set_size(card_lora, 308, LV_SIZE_CONTENT);
//    lv_obj_set_style_pad_all(card_lora, 4, 0);
//    lv_obj_set_style_radius(card_lora, 8, 0);
//    lv_obj_set_style_bg_opa(card_lora, LV_OPA_10, 0);
//    lv_obj_set_style_bg_color(card_lora, lv_palette_main(LV_PALETTE_TEAL), 0);
//    lv_obj_set_style_border_width(card_lora, 0, 0);
//    lv_obj_set_style_pad_row(card_lora, 2, 0);
//    lv_obj_set_flex_flow(card_lora, LV_FLEX_FLOW_COLUMN);
//    lv_label_set_text(lv_label_create(card_lora), "LoRa");
//    lbl_rssi = lv_label_create(card_lora); lv_label_set_text(lbl_rssi, "RSSI: -- dBm");
//    lbl_snr  = lv_label_create(card_lora); lv_label_set_text(lbl_snr,  "SNR : --.- dB");
//
//    /* ===== Label + TextArea (grande, pas dans une card) ===== */
//    lv_obj_t *ta_caption = lv_label_create(scr);
//    lv_label_set_text(ta_caption, "Messages");
//
//    ta_out = lv_textarea_create(scr);
//    lv_obj_set_size(ta_out, 308, LV_SIZE_CONTENT);
//    lv_obj_set_style_min_height(ta_out, 160, 0); /* taille mini (grande) */
//    lv_obj_set_style_max_height(ta_out, 260, 0); /* évite de dépasser l’écran */
//    lv_obj_set_flex_grow(ta_out, 1);             /* prend tout l’espace restant */
//    lv_textarea_set_one_line(ta_out, false);
//    lv_textarea_set_max_length(ta_out, 4096);
//    lv_textarea_set_cursor_click_pos(ta_out, false);
//    lv_textarea_set_text_selection(ta_out, false);
//    lv_obj_clear_flag(ta_out, LV_OBJ_FLAG_CLICK_FOCUSABLE); /* non modifiable */
//    /* Style simple */
//    lv_obj_set_style_bg_opa(ta_out, LV_OPA_10, 0);
//    lv_obj_set_style_bg_color(ta_out, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);
//    lv_obj_set_style_border_width(ta_out, 1, 0);
//    lv_obj_set_style_border_color(ta_out, lv_palette_main(LV_PALETTE_GREY), 0);
//    lv_textarea_set_text(ta_out, "=====Pret====.\n");
//   
//    /* ===== Switches (slide boxes), après la text area ===== */
//    lv_obj_t *row_sw = lv_obj_create(scr);
//    lv_obj_set_size(row_sw, 308, LV_SIZE_CONTENT);
//    lv_obj_set_style_bg_opa(row_sw, LV_OPA_TRANSP, 0);
//    lv_obj_set_style_border_width(row_sw, 0, 0);
//    lv_obj_set_style_pad_all(row_sw, 0, 0);
//    lv_obj_set_flex_flow(row_sw, LV_FLEX_FLOW_ROW);
//    lv_obj_set_flex_align(row_sw, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
//
//    lv_obj_t *col1 = lv_obj_create(row_sw);
//    lv_obj_set_size(col1, 148, LV_SIZE_CONTENT);
//    lv_obj_set_style_bg_opa(col1, LV_OPA_TRANSP, 0);
//    lv_obj_set_style_border_width(col1, 0, 0);
//    lv_obj_set_flex_flow(col1, LV_FLEX_FLOW_COLUMN);
//    lv_obj_set_style_pad_row(col1, 2, 0);
//    lv_label_set_text(lv_label_create(col1), "  CFM");
//    sw_1 = lv_switch_create(col1);
//    lv_obj_set_width(sw_1, 64);
//    lv_obj_add_event_cb(sw_1, on_sw_1, LV_EVENT_VALUE_CHANGED, NULL);
//
//    lv_obj_t *col2 = lv_obj_create(row_sw);
//    lv_obj_set_size(col2, 148, LV_SIZE_CONTENT);
//    lv_obj_set_style_bg_opa(col2, LV_OPA_TRANSP, 0);
//    lv_obj_set_style_border_width(col2, 0, 0);
//    lv_obj_set_flex_flow(col2, LV_FLEX_FLOW_COLUMN);
//    lv_obj_set_style_pad_row(col2, 2, 0);
//    lv_label_set_text(lv_label_create(col2), "  ADR");
//    sw_2 = lv_switch_create(col2);
//    lv_obj_set_width(sw_2, 64);
//    lv_obj_add_event_cb(sw_2, on_sw_2, LV_EVENT_VALUE_CHANGED, NULL);
//
//    /* ===== Footer overlay (Start / End hors frame, en bas) ===== */
//    lv_obj_t *footer = lv_obj_create(lv_layer_top());
//    lv_obj_set_size(footer, 320, FOOTER_H);
//    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
//    lv_obj_set_style_bg_opa(footer, LV_OPA_90, 0);
//    lv_obj_set_style_bg_color(footer, lv_palette_lighten(LV_PALETTE_GREY, 3), 0);
//    lv_obj_set_style_radius(footer, 0, 0);
//    lv_obj_set_style_border_width(footer, 0, 0);
//    lv_obj_set_style_pad_all(footer, 8, 0);
//    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
//    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
//
//    btn_start = lv_btn_create(footer);
//    lv_obj_set_size(btn_start, 120, 40);
//    lv_obj_add_event_cb(btn_start, on_btn_start, LV_EVENT_CLICKED, NULL);
//    lv_label_set_text(lv_label_create(btn_start), "Start");
//
//    btn_stop = lv_btn_create(footer);
//    lv_obj_set_size(btn_stop, 120, 40);
//    lv_obj_add_event_cb(btn_stop, on_btn_stop, LV_EVENT_CLICKED, NULL);
//    lv_label_set_text(lv_label_create(btn_stop), "End");
//
//    /* Valeurs par défaut */
//    lv_label_set_text(lbl_lat, "Lat : --.------°");
//    lv_label_set_text(lbl_lon, "Long: --.------°");
//    lv_label_set_text(lbl_rssi, "RSSI: -- dBm");
//    lv_label_set_text(lbl_snr,  "SNR : --.- dB");
//}
//
//
//void app_main(void)
//{
//	Payload_build(&payload);
//	
//	LoRa_build(&Mylora, NULL);
//    Mylora.Begin(&Mylora);
//    
//	gps_build(&Mygps, NULL);
//    Mygps.Begin(&Mygps);
//    Mygps.Start_task(&Mygps);
//    
//	lcd_build(&Mylcd, NULL);
//	Mylcd.Begin(&Mylcd);
//			
//    // 7. Création d’un objet de test
//    lvgl_port_lock(0);
//    ui_create_lora_gps_screen();
//    lvgl_port_unlock();
//    
//while(1) {
////        //uart_get_buffered_data_len(Mygps.Mygps_config->port_num, &rx_len);
////        //ESP_LOGI("UART", "RX buffered (pending) = %u", (unsigned)rx_len);
////        
////		//ESP_LOGI("fix_type", "sentence='%d'", Mygps.Get_gps_data(&Mygps).fix_type);
//		ESP_LOGI("fix_quality", "sentence='%d'", Mygps.Get_gps_data(&Mygps).fix_quality);
//		ESP_LOGI("valid", "sentence='%d'", Mygps.Get_gps_data(&Mygps).is_valid);
////		//ESP_LOGI("speed", "sentence='%f'", Mygps.Get_gps_data(&Mygps).speed_kmh);
//		ESP_LOGI("day", "sentence='%d'", Mygps.Get_gps_data(&Mygps).date.day);
//		ESP_LOGI("sat", "sentence='%d'", Mygps.Get_gps_data(&Mygps).nb_sat_tracked);
//		//ESP_LOGI("alt", "sentence='%f'", Mygps.Get_gps_data(&Mygps).altitude);
//		ESP_LOGI("latt", "sentence='%f'", Mygps.Get_gps_data(&Mygps).lat_deg);
//		ESP_LOGI("long", "sentence='%f'", Mygps.Get_gps_data(&Mygps).lon_deg);	
//		//ESP_LOGI("BOOT", "reset reason = %d", esp_reset_reason()); // doit afficher ESP_RST_SW
//		vTaskDelay(pdMS_TO_TICKS(1000));
//	}
//} 