#ifndef MY_WINDOWS_H_
#define MY_WINDOWS_H_

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
#include "esp_timer.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "soc/gpio_num.h"
#include "hal/uart_types.h"
#include "esp_log.h"
#include "MY_IMAGE.h"

/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* :::::::::::::::::::::::::::::::::::::::::::::::: Bibliotèque lvgl :::::::::::::::::::::::::::::::::::::::::::: */
#include "lvgl.h"
#include "font/lv_font.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "esp_lvgl_port.h"
#include "misc/lv_types.h"
#include "themes/lv_theme.h"
#include "lv_conf_internal.h"
#include "widgets/textarea/lv_textarea.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Macros :::::::::::::::::::::::::::::::::::::::::::::: */                 


/* :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
              /* :::::::::::::::::::::::::::::::::::::::::::: Structure::::::::::::::::::::::::::::::::::::::::::::::::::::: */

typedef struct load_ui_t {
    /* =============================== État / handles =============================== */

    /** Écran ‘demarrage’ (root object).
     *  Créé via lv_obj_create(NULL), chargé avec lv_screen_load(_anim).
     *  Propriété : load_ui_t (ne pas libérer ailleurs que dans le destructeur dédié). */
    lv_obj_t *main_ui;

    /** Écran destination à afficher après le splash (ex: HOME).*/
    lv_obj_t *next_page;

    /** Timer LVGL déclenchant la transition vers next_page après un certains delay. */
    lv_timer_t *next_timer;

    /** Label du version affiché sur la page.*/
    lv_obj_t *footer_label;

    /** Spinner de chargement (arc animé). */
    lv_obj_t *load_spinner;

    /* ================================== Méthodes ================================= */

    /** Méthode d’initialisation.
     *  Doit préparer main_ui, créer title_label et load_spinner, armer next_timer, etc.
     *  Pattern ‘objet C’ : self est l’instance courante. */
    void (*Begin)(struct load_ui_t *self);

} load_ui_t;

// --------------------------------------------------------------------------------------------------------------------------------------	
typedef struct home_ui_t {
    /* =============================== État / handles =============================== */

    /** Écran ‘demarrage’ (root object).
     *  Créé via lv_obj_create(NULL), chargé avec lv_screen_load(_anim).
     *  Propriété : load_ui_t (ne pas libérer ailleurs que dans le destructeur dédié). */
    lv_obj_t *main_ui;

    /** Écran destination à afficher après le splash (ex: HOME).*/
    lv_obj_t *next_page;

    /** Timer LVGL déclenchant la transition vers next_page après un certains delay. */
    lv_timer_t *next_timer;

    /** Label du version affiché sur la page.*/
    lv_obj_t *footer_label;

    /** Spinner de chargement (arc animé). */
    lv_obj_t *load_spinner;

    /* ================================== Méthodes ================================= */

    /** Méthode d’initialisation.
     *  Doit préparer main_ui, créer title_label et load_spinner, armer next_timer, etc.
     *  Pattern ‘objet C’ : self est l’instance courante. */
    void (*Begin)(struct load_ui_t *self);

} home_ui_t;
// --------------------------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------------------------
//lv_obj_add_event_cb(scr_splash, on_splash_clicked, LV_EVENT_CLICKED, NULL); 

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */
void build_load_ui(load_ui_t *ui);

#ifdef __cplusplus
}
#endif
#endif /* MY_WINDOWS_H_ */

