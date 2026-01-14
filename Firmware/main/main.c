//----------------------------------------------------------------------------------------------------------------------#include "driver/sdspi_host.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_types.h"
#include "hal/ledc_types.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7796.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "misc/lv_anim.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "esp_lcd_touch_ft5x06.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "MY_GNSS.h"
#include "MY_LCD.h"
#include "MY_LORA.h"
#include "PAYLOAD.h"
#include "MY_GAUGE.h"
#include "themes/lv_theme.h"
#include "widgets/textarea/lv_textarea.h"

/* ====== Handle de la task ====== */
static TaskHandle_t sApplyTaskHandle = NULL;

/* ====== Paramètres ====== */
#define APPLY_TASK_NAME      "ApplyTask"
#define APPLY_TASK_STACK     (8*1024)          // adapte si besoin
#define APPLY_TASK_PRIO      3
#define APPLY_TASK_PERIOD_MS 500               // rafraîchissement UI

// ----------------------------------------------------------
Mylcd_t Mylcd;
Mygps_t Mygps;
MyLoRa_t Mylora;
Payload_t payload;

char cmd[128];
char back[512];
bool on_1 = false;
bool on_2 = false;
/* --- Pointeurs UI (statiques) ------------------------------------------------ */
static lv_obj_t *lbl_lat;
static lv_obj_t *lbl_lon;
static lv_obj_t *lbl_rssi;
static lv_obj_t *lbl_snr;
static lv_obj_t *btn_start;
static lv_obj_t *btn_stop;
static lv_obj_t *sw_1;
static lv_obj_t *sw_2;
static lv_obj_t *ta_out;

/* --- Callbacks externes (à implémenter côté appli si besoin) ----------------- */
/* Ex.: démarrer/arrêter acquisition, activer/desactiver options */
__attribute__((weak)) void app_on_start(void) {}
__attribute__((weak)) void app_on_stop(void) {}
__attribute__((weak)) void app_on_switch_1(bool on) { (void)on; }
__attribute__((weak)) void app_on_switch_2(bool on) { (void)on; }


/* --- API d’update (à appeler depuis tes tâches) ------------------------------ */
void ui_set_gps(double lat_deg, double lon_deg){
    /* Thread-safety LVGL : appeler depuis le contexte LVGL (ou protégé par mutex/timer) */
    char buf[64];
	if (lvgl_port_lock(pdMS_TO_TICKS(1))) 
	{ 
    snprintf(buf, sizeof(buf), "Lat : %.6f°", lat_deg);
    lv_label_set_text(lbl_lat, buf);

    snprintf(buf, sizeof(buf), "Long: %.6f°", lon_deg);
    lv_label_set_text(lbl_lon, buf);
    lvgl_port_unlock(); 
    }
}

void ui_set_lora_metrics(int rssi_dbm, float snr_db){
    char buf[48];
    if (lvgl_port_lock(pdMS_TO_TICKS(1))) 
	{ 
    snprintf(buf, sizeof(buf), "RSSI: %d dBm", rssi_dbm);
    lv_label_set_text(lbl_rssi, buf);

    snprintf(buf, sizeof(buf), "SNR : %.1f dB", snr_db);
    lv_label_set_text(lbl_snr, buf);
    lvgl_port_unlock(); 
    }
}

void ui_add_text(char *text){
	if (lvgl_port_lock(pdMS_TO_TICKS(1))) 
	{
	 lv_textarea_add_text(ta_out, text);
	 lv_textarea_add_text(ta_out, "\n");	
	 lvgl_port_unlock(); 
    }
}

/* Optionnel: lire l’état des switches depuis l’appli */
bool ui_get_switch1(void){ return lv_obj_has_state(sw_1, LV_STATE_CHECKED); }
bool ui_get_switch2(void){ return lv_obj_has_state(sw_2, LV_STATE_CHECKED); }

/* --- Fonction de Test -------------------------------------------------------- */
/* ====== Task fonction ====== */
static void ApplyTask(void *arg)
{
    ui_add_text("===========START================");
    gps_data_t gps = {};
    vTaskDelay(pdMS_TO_TICKS(200));
    
    
    while(!Mylora.Config.AT_check(&Mylora)){ ui_add_text(" AT LINK TEST"); } ;
    ui_add_text(" AT LINK OK");
    Mylora.LoRaWAN.Enable_Confirm_mode(&Mylora, on_1);
    Mylora.LoRaWAN.Enable_ADR(&Mylora, on_2);
    
    Mylora.LoRaWAN.is_ADR(&Mylora)?ui_add_text("ADR ENABLE") : ui_add_text("NOT ENABLE");
	
	Mylora.LoRaWAN.is_Confirm_mode(&Mylora)?ui_add_text("CFM ENABLE") : ui_add_text("NOT ENABLE");
	
    while(!Mylora.LoRaWAN.Join(&Mylora)) { ui_add_text(" JOIN LINK TEST");};
    ui_add_text(" JOIN LINK OK");
    
		
    strcpy(back, Mylora.Send_command(&Mylora, "AT+SEND=2:000000\r\n", true, 18000).sentence);
    ui_add_text(back);
    while((strcasecmp(back, "+EVT:SEND_CONFIRMED_OK") != 0) && (strcasecmp(back, "+EVT:TX_DONE") != 0)) 
    	{ 
		strcpy(back, Mylora.Send_command(&Mylora, "AT+SEND=2:000000\r\n", true, 18000).sentence); 
		ui_add_text(back);
		}
		
    for (;;) {
		
        /* 1) Sortie propre si notification reçue */
        if (ulTaskNotifyTake(pdTRUE, 0) > 0) { break;}

        /* 2) Lire GPS une seule fois par tour */
        gps = Mygps.Get_gps_data(&Mygps);
        ui_set_gps( gps.lat_deg, gps.lon_deg);
        while (!gps.is_valid) 
        {
			gps = Mygps.Get_gps_data(&Mygps); ui_set_gps( gps.lat_deg, gps.lon_deg);
			//ui_add_text("GNSS LOSS");
			vTaskDelay(pdMS_TO_TICKS(50));
		}
		
        int rssi = Mylora.LoRaWAN.get_rssi(&Mylora);
        int snr = Mylora.LoRaWAN.get_snr(&Mylora);
        ui_set_lora_metrics(rssi, snr);
        
      	Mylora.LoRaWAN.Send_Payload(&Mylora, payload.custom_encode(gps.lat_deg, gps.lon_deg, rssi, snr).sentence, 2) ?ui_add_text("ENVOIE OK") : ui_add_text("ENVOIE ECHEC ");
      	 
        /* 3) Délai : laisse respirer le CPU/LVGL */
        vTaskDelay(pdMS_TO_TICKS(APPLY_TASK_PERIOD_MS));
    }

    /* Nettoyage si besoin… */
    /* Fin de task */
    TaskHandle_t me = sApplyTaskHandle;
    sApplyTaskHandle = NULL;                 // indique que la task est morte
    vTaskDelete(NULL);
}

/* ====== Create (xTaskCreate wrapper) ====== */
bool ApplyTask_Start(void)
{
    if (sApplyTaskHandle) return true;       // déjà lancée
    BaseType_t ok = xTaskCreate(
        ApplyTask,
        APPLY_TASK_NAME,
        APPLY_TASK_STACK,
        NULL,
        APPLY_TASK_PRIO,
        &sApplyTaskHandle
    );
    return ok == pdPASS;
}

/* ====== Delete / stop propre ====== */
void ApplyTask_Stop(void)
{
    TaskHandle_t h = sApplyTaskHandle;
    if (!h) return;                          // pas lancée
    /* Réveille la task et demande l’arrêt */
    xTaskNotifyGive(h);

    /* Attendre qu’elle se termine et remette le handle à NULL */
    /* (non bloquant, boucle courte) */
    for (int i = 0; i < 100 && sApplyTaskHandle; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* Si elle n’a pas fini, on force (rare) */
    if (sApplyTaskHandle) {
        vTaskDelete(sApplyTaskHandle);
        sApplyTaskHandle = NULL;
    }
    if (lvgl_port_lock(pdMS_TO_TICKS(1))) {
    lv_label_set_text(lbl_lat, "Lat : --.------°");
    lv_label_set_text(lbl_lon, "Long: --.------°");
    lv_label_set_text(lbl_rssi, "RSSI: -- dBm");
    lv_label_set_text(lbl_snr,  "SNR : --.- dB");
    lvgl_port_unlock(); 
    }
}
/* --- Event handlers ---------------------------------------------------------- */

static void on_btn_start(lv_event_t *e){
    (void)e;
    //app_on_start();
    ApplyTask_Start();
}

static void on_btn_stop(lv_event_t *e){
    (void)e;
    //app_on_stop();
    ApplyTask_Stop();
}

static void on_sw_1(lv_event_t *e){
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    on_1 = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    //app_on_switch_1(on);
}

static void on_sw_2(lv_event_t *e){
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    on_2 = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    //app_on_switch_2(on);
}

/* --- Création de l’interface ------------------------------------------------- */

void ui_create_lora_gps_screen(void)
{
    /* ===== Screen root ===== */
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);

    /* Layout compact 320x480, colonne */
    const int FOOTER_H = 56;                /* hauteur du footer overlay */
    lv_obj_set_size(scr, 320, 480);
    lv_obj_set_style_pad_all(scr, 6, 0);
    lv_obj_set_style_pad_bottom(scr, FOOTER_H + 6, 0); /* évite recouvrement par footer */
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* ===== Header ===== */
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, 308, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(hdr, 6, 0);
    lv_obj_set_style_radius(hdr, 8, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(hdr, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_label_set_text(lv_label_create(hdr), "Altirawan: GPS & LoRa");

    /* ===== Card GPS (compact) ===== */
    lv_obj_t *card_gps = lv_obj_create(scr);
    lv_obj_set_size(card_gps, 308, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(card_gps, 4, 0);
    lv_obj_set_style_radius(card_gps, 8, 0);
    lv_obj_set_style_bg_opa(card_gps, LV_OPA_10, 0);
    lv_obj_set_style_bg_color(card_gps, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_border_width(card_gps, 0, 0);
    lv_obj_set_style_pad_row(card_gps, 2, 0);
    lv_obj_set_flex_flow(card_gps, LV_FLEX_FLOW_COLUMN);
    lv_label_set_text(lv_label_create(card_gps), "GPS");
    lbl_lat = lv_label_create(card_gps);  lv_label_set_text(lbl_lat, "Lat : --.------°");
    lbl_lon = lv_label_create(card_gps);  lv_label_set_text(lbl_lon, "Long: --.------°");

    /* ===== Card LoRa (compact) ===== */
    lv_obj_t *card_lora = lv_obj_create(scr);
    lv_obj_set_size(card_lora, 308, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(card_lora, 4, 0);
    lv_obj_set_style_radius(card_lora, 8, 0);
    lv_obj_set_style_bg_opa(card_lora, LV_OPA_10, 0);
    lv_obj_set_style_bg_color(card_lora, lv_palette_main(LV_PALETTE_TEAL), 0);
    lv_obj_set_style_border_width(card_lora, 0, 0);
    lv_obj_set_style_pad_row(card_lora, 2, 0);
    lv_obj_set_flex_flow(card_lora, LV_FLEX_FLOW_COLUMN);
    lv_label_set_text(lv_label_create(card_lora), "LoRa");
    lbl_rssi = lv_label_create(card_lora); lv_label_set_text(lbl_rssi, "RSSI: -- dBm");
    lbl_snr  = lv_label_create(card_lora); lv_label_set_text(lbl_snr,  "SNR : --.- dB");

    /* ===== Label + TextArea (grande, pas dans une card) ===== */
    lv_obj_t *ta_caption = lv_label_create(scr);
    lv_label_set_text(ta_caption, "Messages");

    ta_out = lv_textarea_create(scr);
    lv_obj_set_size(ta_out, 308, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(ta_out, 160, 0); /* taille mini (grande) */
    lv_obj_set_style_max_height(ta_out, 260, 0); /* évite de dépasser l’écran */
    lv_obj_set_flex_grow(ta_out, 1);             /* prend tout l’espace restant */
    lv_textarea_set_one_line(ta_out, false);
    lv_textarea_set_max_length(ta_out, 4096);
    lv_textarea_set_cursor_click_pos(ta_out, false);
    lv_textarea_set_text_selection(ta_out, false);
    lv_obj_clear_flag(ta_out, LV_OBJ_FLAG_CLICK_FOCUSABLE); /* non modifiable */
    /* Style simple */
    lv_obj_set_style_bg_opa(ta_out, LV_OPA_10, 0);
    lv_obj_set_style_bg_color(ta_out, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_border_width(ta_out, 1, 0);
    lv_obj_set_style_border_color(ta_out, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_textarea_set_text(ta_out, "=====Pret====.\n");
   
    /* ===== Switches (slide boxes), après la text area ===== */
    lv_obj_t *row_sw = lv_obj_create(scr);
    lv_obj_set_size(row_sw, 308, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_sw, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_sw, 0, 0);
    lv_obj_set_style_pad_all(row_sw, 0, 0);
    lv_obj_set_flex_flow(row_sw, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_sw, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *col1 = lv_obj_create(row_sw);
    lv_obj_set_size(col1, 148, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(col1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col1, 0, 0);
    lv_obj_set_flex_flow(col1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col1, 2, 0);
    lv_label_set_text(lv_label_create(col1), "  CFM");
    sw_1 = lv_switch_create(col1);
    lv_obj_set_width(sw_1, 64);
    lv_obj_add_event_cb(sw_1, on_sw_1, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *col2 = lv_obj_create(row_sw);
    lv_obj_set_size(col2, 148, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(col2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col2, 0, 0);
    lv_obj_set_flex_flow(col2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col2, 2, 0);
    lv_label_set_text(lv_label_create(col2), "  ADR");
    sw_2 = lv_switch_create(col2);
    lv_obj_set_width(sw_2, 64);
    lv_obj_add_event_cb(sw_2, on_sw_2, LV_EVENT_VALUE_CHANGED, NULL);

    /* ===== Footer overlay (Start / End hors frame, en bas) ===== */
    lv_obj_t *footer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(footer, 320, FOOTER_H);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_90, 0);
    lv_obj_set_style_bg_color(footer, lv_palette_lighten(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 8, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    btn_start = lv_btn_create(footer);
    lv_obj_set_size(btn_start, 120, 40);
    lv_obj_add_event_cb(btn_start, on_btn_start, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn_start), "Start");

    btn_stop = lv_btn_create(footer);
    lv_obj_set_size(btn_stop, 120, 40);
    lv_obj_add_event_cb(btn_stop, on_btn_stop, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn_stop), "End");

    /* Valeurs par défaut */
    lv_label_set_text(lbl_lat, "Lat : --.------°");
    lv_label_set_text(lbl_lon, "Long: --.------°");
    lv_label_set_text(lbl_rssi, "RSSI: -- dBm");
    lv_label_set_text(lbl_snr,  "SNR : --.- dB");
}


void app_main(void)
{
	Payload_build(&payload);
	
	LoRa_build(&Mylora, NULL);
    Mylora.Begin(&Mylora);
    
	gps_build(&Mygps, NULL);
    Mygps.Begin(&Mygps);
    Mygps.Start_task(&Mygps);
    
	lcd_build(&Mylcd, NULL);
	Mylcd.Begin(&Mylcd);
			
    // 7. Création d’un objet de test
    lvgl_port_lock(0);
    ui_create_lora_gps_screen();
    lvgl_port_unlock();
    
while(1) {
//        //uart_get_buffered_data_len(Mygps.Mygps_config->port_num, &rx_len);
//        //ESP_LOGI("UART", "RX buffered (pending) = %u", (unsigned)rx_len);
//        
//		//ESP_LOGI("fix_type", "sentence='%d'", Mygps.Get_gps_data(&Mygps).fix_type);
		ESP_LOGI("fix_quality", "sentence='%d'", Mygps.Get_gps_data(&Mygps).fix_quality);
		ESP_LOGI("valid", "sentence='%d'", Mygps.Get_gps_data(&Mygps).is_valid);
//		//ESP_LOGI("speed", "sentence='%f'", Mygps.Get_gps_data(&Mygps).speed_kmh);
		ESP_LOGI("day", "sentence='%d'", Mygps.Get_gps_data(&Mygps).date.day);
		ESP_LOGI("sat", "sentence='%d'", Mygps.Get_gps_data(&Mygps).nb_sat_tracked);
		//ESP_LOGI("alt", "sentence='%f'", Mygps.Get_gps_data(&Mygps).altitude);
		ESP_LOGI("latt", "sentence='%f'", Mygps.Get_gps_data(&Mygps).lat_deg);
		ESP_LOGI("long", "sentence='%f'", Mygps.Get_gps_data(&Mygps).lon_deg);	
		//ESP_LOGI("BOOT", "reset reason = %d", esp_reset_reason()); // doit afficher ESP_RST_SW
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
 
// //-----------------------------------------------------------------------------------------------------------------------------
// //   ----------------------------------------------------------------------------------------------------------   
///* ====== Handle de la task ====== */

//#include "MY_WINDOWS.h"
//
//#define MAX_VALUE 100
//#define MIN_VALUE 0
//
//Mygauge_t gauge;
//
////static void set_value(void * bar, int32_t v)
////{
////    lv_bar_set_value((lv_obj_t *)bar, v, LV_ANIM_OFF);
////}
////
////static void event_cb(lv_event_t * e)
////{
////    lv_obj_t * obj = lv_event_get_target_obj(e);
////
////    lv_draw_label_dsc_t label_dsc;
////    lv_draw_label_dsc_init(&label_dsc);
////    label_dsc.font = LV_FONT_DEFAULT;
////
////    char buf[8];
////    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));
////
////    lv_point_t txt_size;
////    lv_text_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX,
////                     label_dsc.flag);
////
////    lv_area_t txt_area;
////    txt_area.x1 = 0;
////    txt_area.x2 = txt_size.x - 1;
////    txt_area.y1 = 0;
////    txt_area.y2 = txt_size.y - 1;
////
////    lv_area_t indic_area;
////    lv_obj_get_coords(obj, &indic_area);
////    lv_area_set_width(&indic_area, lv_area_get_width(&indic_area) * lv_bar_get_value(obj) / MAX_VALUE);
////
////    /*If the indicator is long enough put the text inside on the right*/
////    if(lv_area_get_width(&indic_area) > txt_size.x + 20) {
////        lv_area_align(&indic_area, &txt_area, LV_ALIGN_RIGHT_MID, -10, 0);
////        label_dsc.color = lv_color_white();
////    }
////    /*If the indicator is still short put the text out of it on the right*/
////    else {
////        lv_area_align(&indic_area, &txt_area, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
////        label_dsc.color = lv_color_black();
////    }
////    label_dsc.text = buf;
////    label_dsc.text_local = true;
////    lv_layer_t * layer = lv_event_get_layer(e);
////    lv_draw_label(layer, &label_dsc, &txt_area);
////}
////
/////**
//// * Custom drawer on the bar to display the current value
//// */
////void lv_example_bar_6(void)
////{
////    lv_obj_t * bar = lv_bar_create(lv_screen_active());
////    lv_bar_set_range(bar, MIN_VALUE, MAX_VALUE);
////    lv_obj_set_size(bar, 200, 20);
////    lv_obj_center(bar);
////    lv_obj_add_event_cb(bar, event_cb, LV_EVENT_DRAW_MAIN_END, NULL);
////
////    lv_anim_t a;
////    lv_anim_init(&a);
////    lv_anim_set_var(&a, bar);
////    lv_anim_set_values(&a, 0, 100);
////    lv_anim_set_exec_cb(&a, set_value);
////    lv_anim_set_duration(&a, 4000);
////    lv_anim_set_reverse_duration(&a, 4000);
////    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
////    lv_anim_start(&a);
////
////}
//
//#define MAX_VALUE 100
//#define MIN_VALUE 0
//
//Mygauge_t gauge;
//static lv_obj_t *battery_bar = NULL;
//
///* Fallback sûr si le gauge n'est pas initialisé */
//static int gauge_percent_safe(void)
//{
//    if (gauge.get_percent) {
//        int p = gauge.get_percent(&gauge);
//        ESP_LOGI("", "SoC=%d%%", p);
//        return (p < 0) ? 0 : (p > 100) ? 100 : p;
//    }
//    return 0; // défaut
//}
//
///* --- Dessin du % sur la barre --- */
//static void event_cb(lv_event_t * e)
//{
//    lv_obj_t * obj = lv_event_get_target(e);
//
//    lv_draw_label_dsc_t label_dsc;
//    lv_draw_label_dsc_init(&label_dsc);
//    label_dsc.font = LV_FONT_DEFAULT;
//
//    char buf[8];
//    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));
//
//    lv_point_t txt_size;
//    lv_text_get_size(&txt_size, buf, label_dsc.font,
//                     label_dsc.letter_space, label_dsc.line_space,
//                     LV_COORD_MAX, label_dsc.flag);
//
//    lv_area_t txt_area = { .x1=0, .y1=0, .x2=txt_size.x-1, .y2=txt_size.y-1 };
//
//    lv_area_t indic_area;
//    lv_obj_get_coords(obj, &indic_area);
//    lv_area_set_width(&indic_area,
//        lv_area_get_width(&indic_area) * lv_bar_get_value(obj) / MAX_VALUE);
//
//    if (lv_area_get_width(&indic_area) > txt_size.x + 20) {
//        lv_area_align(&indic_area, &txt_area, LV_ALIGN_RIGHT_MID, -10, 0);
//        label_dsc.color = lv_color_white();
//    } else {
//        lv_area_align(&indic_area, &txt_area, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
//        label_dsc.color = lv_color_black();
//    }
//
//    label_dsc.text = buf;
//    label_dsc.text_local = true;
//    lv_draw_label(lv_event_get_layer(e), &label_dsc, &txt_area);
//}
//
///* --- Timer LVGL: lit le SoC et met à jour la barre --- */
//static void battery_timer_cb(lv_timer_t *t)
//{
//    LV_UNUSED(t);
//    int p = gauge_percent_safe();  // 0..100
//    lv_bar_set_value(battery_bar, p, LV_ANIM_OFF);
//
//    // Couleur selon le niveau (facultatif)
//    lv_color_t c =
//        (p < 20) ? lv_palette_main(LV_PALETTE_RED) :
//        (p < 50) ? lv_palette_main(LV_PALETTE_AMBER) :
//                   lv_palette_main(LV_PALETTE_GREEN);
//    lv_obj_set_style_bg_color(battery_bar, c, LV_PART_INDICATOR);
//}
//
///* --- Création UI de la barre batterie (à appeler sous lock) --- */
//static void ui_create_battery_bar(void)
//{
//    battery_bar = lv_bar_create(lv_screen_active());
//    lv_bar_set_range(battery_bar, MIN_VALUE, MAX_VALUE);
//    lv_obj_set_size(battery_bar, 220, 20);
//    lv_obj_align(battery_bar, LV_ALIGN_TOP_MID, 0, 10);
//    lv_obj_add_event_cb(battery_bar, event_cb, LV_EVENT_DRAW_MAIN_END, NULL);
//
//    // Valeur initiale (sans risque)
//    lv_bar_set_value(battery_bar, gauge_percent_safe(), LV_ANIM_REPEAT_INFINITE);
//
//    // MAJ toutes les 1 s (callback exécuté côté LVGL)
//    lv_timer_create(battery_timer_cb, 1000, NULL);
//}
//
//void lv_example_bar_battery(void)
//{
//    ui_create_battery_bar();
//}
//
//Mylcd_t Mylcd;
//load_ui_t page;
// 
//void app_main(void)
//{
//	
//	gauge_build(&gauge, NULL);
//	gauge.Begin(&gauge);
//	
//	lcd_build(&Mylcd, NULL);
//	Mylcd.Begin(&Mylcd);
//	
//	build_load_ui(&page);
//	
//    lvgl_port_lock(0);
//    
//    //lv_example_bar_battery();
//    //page.next_page = battery_bar;
//	page.Begin(&page);
//	
//    lvgl_port_unlock();
//   
//}

// -------------------------------------------------------------------------------------------------------------------------------------
//void lv_example(void)
//{
//    /*Create a LED and switch it OFF*/
//    lv_obj_t * led1  = lv_led_create(lv_screen_active());
//    lv_obj_align(led1, LV_ALIGN_CENTER, -80, 0);
//    lv_led_off(led1);
//
//    /*Copy the previous LED and set a brightness*/
//    lv_obj_t * led2  = lv_led_create(lv_screen_active());
//    lv_obj_align(led2, LV_ALIGN_CENTER, 0, 0);
//    lv_led_set_brightness(led2, 150);
//    lv_led_set_color(led2, lv_palette_main(LV_PALETTE_RED));
//
//    /*Copy the previous LED and switch it ON*/
//    lv_obj_t * led3  = lv_led_create(lv_screen_active());
//    lv_obj_align(led3, LV_ALIGN_CENTER, 80, 0);
//    lv_led_on(led3);
//}
//
//void lv_example_canvas_5(void)
//{
//    /*Create a buffer for the canvas*/
//    LV_DRAW_BUF_DEFINE_STATIC(draw_buf, 100, 80, LV_COLOR_FORMAT_ARGB8888);
//    LV_DRAW_BUF_INIT_STATIC(draw_buf);
//
//    /*Create a canvas and initialize its palette*/
//    lv_obj_t * canvas = lv_canvas_create(lv_screen_active());
//    lv_canvas_set_draw_buf(canvas, &draw_buf);
//    lv_canvas_fill_bg(canvas, lv_color_hex3(0xccc), LV_OPA_COVER);
//    lv_obj_center(canvas);
//
//    lv_layer_t layer;
//    lv_canvas_init_layer(canvas, &layer);
//
//    lv_draw_arc_dsc_t dsc;
//    lv_draw_arc_dsc_init(&dsc);
//    dsc.color = lv_palette_main(LV_PALETTE_RED);
//    dsc.width = 5;
//    dsc.center.x = 25;
//    dsc.center.y = 25;
//    dsc.width = 10;
//    dsc.radius = 15;
//    dsc.start_angle = 0;
//    dsc.end_angle = 220;
//
//    lv_draw_arc(&layer, &dsc);
//
//    lv_canvas_finish_layer(canvas, &layer);
//
//}
//
//void lv_example_led(void)
//{
//    /*Create a LED and switch it OFF*/
//    lv_obj_t * led1  = lv_led_create(lv_screen_active());
//    lv_obj_align(led1, LV_ALIGN_CENTER, -80, 0);
//    lv_led_off(led1);
//
//    /*Copy the previous LED and set a brightness*/
//    lv_obj_t * led2  = lv_led_create(lv_screen_active());
//    lv_obj_align(led2, LV_ALIGN_CENTER, 0, 0);
//    lv_led_set_brightness(led2, 150);
//    lv_led_set_color(led2, lv_palette_main(LV_PALETTE_RED));
//
//    /*Copy the previous LED and switch it ON*/
//    lv_obj_t * led3  = lv_led_create(lv_screen_active());
//    lv_obj_align(led3, LV_ALIGN_CENTER, 80, 0);
//    lv_led_on(led3);
//}
//
//void create_keyboard(void)
//{
//    /* 1. Titre */
//    lv_obj_t *label = lv_label_create(lv_scr_act());
//    lv_label_set_text(label, "Clavier virtuel !");
//    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 8);
//    lv_obj_set_style_text_color(label,
//                                lv_color_hex(0xFF0000),              /* rouge franc */
//                                LV_PART_MAIN | LV_STATE_DEFAULT);
//
//    /* 2. Champ de saisie */
//    lv_obj_t *ta = lv_textarea_create(lv_scr_act());
//    lv_obj_set_size(ta, 240, 40);
//    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 40);
//    lv_textarea_set_placeholder_text(ta, "Tapez ici");
//
//    /* 3. Clavier logiciel */
//    lv_obj_t *kb = lv_keyboard_create(lv_scr_act());
//    lv_obj_set_size(kb, 320, 170);                                   /* pleine largeur */
//    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
//    lv_keyboard_set_textarea(kb, ta);                                /* lien clavier ↔ textarea */
//    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);           /* azerty/abc, etc. */
//}
//
//void app_main(void)
//{
//
//	Mylcd_t Mylcd;
//	lcd_build(&Mylcd, NULL);
//	Mylcd.Begin(&Mylcd);
//	
//    // 7. Création d’un objet de test
//    //create_calendar();
//    lvgl_port_lock(0);
//    create_keyboard();
//    //lv_example_led();
//    //v_example_canvas_5();
//    lvgl_port_unlock();
//    
//   
//	//lv_disp_set_rotation(disp, LV_DISP_ROTATION_0);
//	
//    // 8. (Optionnel) si vous désactivez la tâche automatique LVGL :
//     //while (1) {
//      //   lv_timer_handler();
//       //  vTaskDelay(pdMS_TO_TICKS(10));
//     //}
//}


// --------------------------------------------------------------------- Usb Bridge: Ublox -----------------------------------------

//#include "esp_log.h"
//#include "esp_system.h"
//#include "usb/usb_host.h"
//#include "tinyusb.h"
//#include "driver/uart.h"
//#include "tusb.h"
//
//#define UART_PORT UART_NUM_1
//#define TXD_PIN (GPIO_NUM_8)
//#define RXD_PIN (GPIO_NUM_18)
//#define BUF_SIZE 1024
//#define UART_BAUDRATE 38400
//#define GPIO_HIGH_1    9//10  // GPIO à mettre HIGH
//#define GPIO_HIGH_2    46//9   // GPIO à mettre HIGH
//#define GPIO_HIGH_3    4//48   // GPIO à mettre HIGH
//
//
//static const char *TAG = "USB_UART_BRIDGE";
//
//void init_uart() {
//    const uart_config_t uart_config = {
//        .baud_rate = UART_BAUDRATE,
//        .data_bits = UART_DATA_8_BITS,
//        .parity    = UART_PARITY_DISABLE,
//        .stop_bits = UART_STOP_BITS_1,
//        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
//    };
//
//    uart_driver_install(UART_PORT, BUF_SIZE * 2, BUF_SIZE, 0, NULL, 0);
//    uart_param_config(UART_PORT, &uart_config);
//    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//}
//
//void init_gpio() {
//    gpio_set_direction(GPIO_HIGH_1, GPIO_MODE_OUTPUT);
//    gpio_set_direction(GPIO_HIGH_2, GPIO_MODE_OUTPUT);
//    gpio_set_direction(GPIO_HIGH_3, GPIO_MODE_OUTPUT);
//    vTaskDelay(20 / portTICK_PERIOD_MS);
//    gpio_set_level(GPIO_HIGH_3, 1);
//    gpio_set_level(GPIO_HIGH_2, 1);
//    vTaskDelay(25 / portTICK_PERIOD_MS);
//    gpio_set_level(GPIO_HIGH_1, 1);
//    ESP_LOGI(TAG, "GPIO %d et %d mis à HIGH", GPIO_HIGH_1, GPIO_HIGH_2);
//}
//
//void init_boot() {
//    gpio_set_direction(GPIO_HIGH_1, GPIO_MODE_OUTPUT);
//    gpio_set_direction(GPIO_HIGH_2, GPIO_MODE_OUTPUT);
//    gpio_set_direction(GPIO_HIGH_3, GPIO_MODE_OUTPUT);
//    vTaskDelay(20 / portTICK_PERIOD_MS);
//    gpio_set_level(GPIO_HIGH_3, 1);
//    gpio_set_level(GPIO_HIGH_2, 0);
//    gpio_set_level(GPIO_HIGH_1, 0);
//    vTaskDelay(10 / portTICK_PERIOD_MS);
//    gpio_set_level(GPIO_HIGH_1, 1);
//    gpio_set_level(GPIO_HIGH_2, 1);
//    ESP_LOGI(TAG, "GPIO %d et %d mis à HIGH", GPIO_HIGH_1, GPIO_HIGH_2);
//}
//
//
//
//void app_main(void) {
//	init_gpio();
//	//init_boot();
//	vTaskDelay(10 / portTICK_PERIOD_MS);
//    init_uart();
//	
//     // Init USB CDC
//    tinyusb_config_t tusb_cfg = {};
//    tusb_cfg.device_descriptor = NULL;
//    tusb_cfg.string_descriptor = NULL;
//    tusb_cfg.external_phy = false;
//    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
//    ESP_LOGI(TAG, "USB-to-UART bridge started");
//    uint8_t usb_rx_buf[64];
//    uint8_t uart_rx_buf[64];
//	vTaskDelay(10 / portTICK_PERIOD_MS);
//	
//    while (true) {
//        //USB -> UART
//        int usb_len = tud_cdc_read(usb_rx_buf, sizeof(usb_rx_buf));
//        if (usb_len > 0) {
//            uart_write_bytes(UART_PORT, (const char *)usb_rx_buf, usb_len);
//        }
//
//        // UART -> USB
//        int uart_len = uart_read_bytes(UART_PORT, uart_rx_buf, sizeof(uart_rx_buf), 10 / portTICK_PERIOD_MS);
//        if (uart_len > 0) {
//            tud_cdc_write(uart_rx_buf, uart_len);
//            tud_cdc_write_flush();
//        }
//
//        vTaskDelay(10 / portTICK_PERIOD_MS);
//    }
//}

// --------------------------------------------------------------------- Usb Bridge -----------------------------------------
//#include "esp_log.h"
//#include "esp_system.h"
//#include "usb/usb_host.h"
//#include "tinyusb.h"
//#include "driver/uart.h"
//#include "tusb.h"
//
//#define UART_PORT UART_NUM_1
//#define TXD_PIN GPIO_NUM_14   //(GPIO_NUM_4)
//#define RXD_PIN (GPIO_NUM_21) //(GPIO_NUM_5)
//#define BUF_SIZE 1024
//#define UART_BAUDRATE 115200
//#define GPIO_HIGH_1    48   //11  // GPIO à mettre HIGH
//#define GPIO_HIGH_2    47   //3   // GPIO à mettre HIGH
//#define GPIO_HIGH_3    4    //48  // GPIO à mettre HIGH
//
//
//static const char *TAG = "USB_UART_BRIDGE";
//
//void init_uart() {
//    const uart_config_t uart_config = {
//        .baud_rate = UART_BAUDRATE,
//        .data_bits = UART_DATA_8_BITS,
//        .parity    = UART_PARITY_DISABLE,
//        .stop_bits = UART_STOP_BITS_1,
//        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
//    };
//
//    uart_driver_install(UART_PORT, BUF_SIZE * 2, BUF_SIZE, 0, NULL, 0);
//    uart_param_config(UART_PORT, &uart_config);
//    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//}
//
//void init_gpio() {
//    gpio_set_direction(GPIO_HIGH_1, GPIO_MODE_OUTPUT);
//    gpio_set_direction(GPIO_HIGH_2, GPIO_MODE_OUTPUT);
//    gpio_set_direction(GPIO_HIGH_3, GPIO_MODE_OUTPUT);
//    vTaskDelay(20 / portTICK_PERIOD_MS);
//    gpio_set_level(GPIO_HIGH_3, 1);
//    gpio_set_level(GPIO_HIGH_2, 0);
//    vTaskDelay(25 / portTICK_PERIOD_MS);
//    gpio_set_level(GPIO_HIGH_1, 1);
//    ESP_LOGI(TAG, "GPIO %d et %d mis à HIGH", GPIO_HIGH_1, GPIO_HIGH_2);
//}
//
//void app_main(void) {
//	init_gpio();
//	vTaskDelay(10 / portTICK_PERIOD_MS);
//    init_uart();
//	
//     // Init USB CDC
//    tinyusb_config_t tusb_cfg = {};
//    tusb_cfg.device_descriptor = NULL;
//    tusb_cfg.string_descriptor = NULL;
//    tusb_cfg.external_phy = false;
//    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
//    ESP_LOGI(TAG, "USB-to-UART bridge started");
//    uint8_t usb_rx_buf[64];
//    uint8_t uart_rx_buf[64];
//	vTaskDelay(10 / portTICK_PERIOD_MS);
//	
//    while (true) {
//        // USB -> UART
//        int usb_len = tud_cdc_read(usb_rx_buf, sizeof(usb_rx_buf));
//        if (usb_len > 0) {
//            uart_write_bytes(UART_PORT, (const char *)usb_rx_buf, usb_len);
//        }
//
//        // UART -> USB
//        int uart_len = uart_read_bytes(UART_PORT, uart_rx_buf, sizeof(uart_rx_buf), 10 / portTICK_PERIOD_MS);
//        if (uart_len > 0) {
//            tud_cdc_write(uart_rx_buf, uart_len);
//            tud_cdc_write_flush();
//        }
//
//        vTaskDelay(10 / portTICK_PERIOD_MS);
//    }
//}

// -------------------------------------------SD CARD---------------------------------------------------------------
//#include <stdio.h>
//#include <dirent.h>
//#include "esp_log.h"
//#include "driver/spi_master.h"
//#include "sdmmc_cmd.h"
//#include "esp_vfs_fat.h"
//
//#define PIN_MOSI  GPIO_NUM_12 //13 16
//#define PIN_MISO  GPIO_NUM_10 //21 17
//#define PIN_SCK   GPIO_NUM_11 //15
//#define PIN_CS    GPIO_NUM_13 //12   // pull-up 47–100 kΩ conseillé
//
//static const char *TAG = "SD_SPI";
//
//static void list_dir(const char *path) {
//    DIR *d = opendir(path);
//    if (!d) { ESP_LOGW(TAG, "opendir(%s) fail", path); return; }
//    struct dirent *e;
//    while ((e = readdir(d)) != NULL) {
//        printf("%s%s\n", e->d_name, e->d_type == DT_DIR ? "/" : "");
//    }
//    closedir(d);
//}
//
//void app_main(void) {
//    sdmmc_card_t *card;
//
//    // 1) Hôte SDSPI (SPI2) — 10 MHz pour commencer
//    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
//    host.max_freq_khz = 20000; // 10 MHz (si souci, essaie 400 kHz)
//
//    // 2) Bus SPI
//    spi_bus_config_t bus = {
//        .mosi_io_num = PIN_MOSI,
//        .miso_io_num = PIN_MISO,
//        .sclk_io_num = PIN_SCK,
//        .quadwp_io_num = -1,
//        .quadhd_io_num = -1,
//        .max_transfer_sz = 4096,
//    };
//    ESP_ERROR_CHECK(spi_bus_initialize(host.slot, &bus, SPI_DMA_CH_AUTO));
//
//    // 3) Device (CS)
//    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
//    slot.gpio_cs = PIN_CS;
//    slot.host_id = host.slot;
//
//    // 4) Montage FAT (FATFS choisira FAT32 selon la taille)
//    const esp_vfs_fat_mount_config_t mnt = {
//        .format_if_mount_failed = true,      // formate si nécessaire
//        .max_files = 5,
//        .allocation_unit_size = 16 * 1024
//    };
//
//    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot, &mnt, &card);
//    if (ret != ESP_OK) {
//        ESP_LOGE(TAG, "Mount échoué: %s", esp_err_to_name(ret));
//        ESP_LOGE(TAG, "Check câblage: DAT3->CS, CMD->MOSI, DAT0->MISO, CLK->SCK, VDD=3V3, GND commun.");
//        spi_bus_free(host.slot);
//        return;
//    }
//
//    sdmmc_card_print_info(stdout, card);
//    ESP_LOGI(TAG, "Listage racine avant écriture:");
//    list_dir("/sdcard");
//
//    // 5) Test I/O
//    FILE *f = fopen("/sdcard/test.txt", "w");
//    if (!f) { ESP_LOGE(TAG, "open write"); goto out; }
//    fprintf(f, "Hello SD SPI!\n");
//    fclose(f);
//    ESP_LOGI(TAG, "Écrit /sdcard/test.txt");
//
//    f = fopen("/sdcard/test.txt", "r");
//    if (!f) { ESP_LOGE(TAG, "open read"); goto out; }
//    char line[128];
//    printf("--- Contenu ---\n");
//    while (fgets(line, sizeof line, f)) printf("%s", line);
//    fclose(f);
//
//    ESP_LOGI(TAG, "Listage racine après écriture:");
//    list_dir("/sdcard");
//
//out:
//    // 6) Démonter et libérer
//    esp_vfs_fat_sdcard_unmount("/sdcard", card);
//    spi_bus_free(host.slot);
//    ESP_LOGI(TAG, "SD démontée, bus SPI libéré");
//}



