#include "MY_WINDOWS.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Macro :::::::::::::::::::::::::::::::::::::::::::::: */
#define SPLASH_MS   3000
#define ANIM_TIME   400
#define ANIM_TYPE   LV_SCR_LOAD_ANIM_FADE_ON

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions static :::::::::::::::::::::::::::::::::::::::::::::: */
static void background_img(load_ui_t *ui)	
{
	ui->main_ui = lv_obj_create(NULL);
	lv_obj_clear_flag(ui->main_ui, LV_OBJ_FLAG_SCROLLABLE);
	
	lv_obj_set_style_bg_opa(ui->main_ui, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(ui->main_ui, lv_color_make(0, 0, 0), 0); 
    
	lv_obj_set_style_bg_img_src(ui->main_ui, &Load_img, LV_PART_MAIN);
	
	lv_obj_set_style_bg_image_tiled(ui->main_ui, false, LV_PART_MAIN);

	lv_obj_set_style_bg_image_opa(ui->main_ui, LV_OPA_COVER, LV_PART_MAIN);
}

// -------------------------------------------------------------------------------------------------------------------------------------
static void title_text(load_ui_t *ui)
{
	ui->footer_label = lv_label_create(ui->main_ui);
	
    lv_label_set_text(ui->footer_label, "ALTIRAWAN ");
    lv_obj_set_style_text_color(ui->footer_label, lv_color_white(), LV_STATE_DEFAULT);
    
    lv_obj_set_style_text_color(ui->footer_label, lv_color_make(230, 150, 0), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->footer_label, &lv_font_montserrat_16 , LV_STATE_DEFAULT);  
}
	   
// --------------------------------------------------------------------------------------------------------------------------------------
static void spinner(load_ui_t *ui)
{
    ui->load_spinner = lv_spinner_create(ui->main_ui);
    lv_obj_set_style_arc_color(ui->load_spinner, lv_color_make(230, 150, 0),LV_PART_INDICATOR);
    lv_obj_set_size(ui->load_spinner, 40, 40); 
}


// --------------------------------------------------------------------------------------------------------------------------------
static void auto_skip(load_ui_t *ui) {
    if (ui->next_timer) { lv_timer_del(ui->next_timer); ui->next_timer = NULL; }
    
    // Charge une nouvelle page avec animation; 'del'=true -> libère le splash après l’anim
    if (ui->next_page){ lv_screen_load_anim(ui->next_page, ANIM_TYPE, ANIM_TIME, 0, true); }
}

// --------------------------------------------------------------------------------------------------------------------------------
static void timer_timeout(lv_timer_t *t) 
{ 
	load_ui_t *ui = (load_ui_t *)lv_timer_get_user_data(t);
    auto_skip(ui); 
}

// --------------------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------------------
void load_ui(load_ui_t *ui)
{
	background_img(ui);
	
	title_text(ui);
	lv_obj_align(ui->footer_label, LV_ALIGN_CENTER, 105, 225); 
	
	spinner(ui);
	lv_obj_align(ui->load_spinner, LV_ALIGN_CENTER, 0, 58);
 
	lv_screen_load(ui->main_ui);
	
	// Timer auto-skip
    ui->next_timer = lv_timer_create(timer_timeout, SPLASH_MS, ui);
    lv_timer_set_repeat_count(ui->next_timer, 1);
}

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */	    

void build_load_ui(load_ui_t *ui)
{
  	// Initialisation propre
	memset(ui, 0, sizeof(load_ui_t));
	
	// affectation des methodes
	ui->Begin = load_ui;
}

// -------------------------------------------------------------------------------------------------------------------------------------