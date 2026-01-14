#include "MY_GAUGE.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables interne :::::::::::::::::::::::::::::::::::::::::::::: */	    
Mygauge_dev_config_t Mygauge_default = {
	/* ---- Bus I²C ---- */
	.port_num     = I2C_NUM_1,
    .clk_speed_hz = 400000,
    .dev_addr     = MAX17048_I2C_ADDR_DEFAULT,
    
    /* ---- Broches ---- */
    .SDA          =  7,       
    .SCL          =	 15,

    /* ---- Pull-ups internes (esp-idf : GPIO_PULLUP_ENABLE / DISABLE) ---- */
   .sda_pullup_en =  GPIO_PULLUP_ENABLE,
   .scl_pullup_en =  GPIO_PULLUP_ENABLE,

    /* ---- Optionnel périphérique ---- */
   .mode          = I2C_MODE_MASTER    
};

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions static :::::::::::::::::::::::::::::::::::::::::::::: */
static bool Mygauge_i2c_init(Mygauge_t *Mygauge)
{
	if(!Mygauge) return false;
	
	/* ---- Verification du port ---- */
    if (Mygauge->Mygauge_config->port_num >= I2C_NUM_MAX) return false; 
    
    i2c_config_t cfg = {
        .mode = Mygauge->Mygauge_config->mode,
        .sda_io_num = Mygauge->Mygauge_config->SDA, .sda_pullup_en = Mygauge->Mygauge_config->sda_pullup_en,
        .scl_io_num = Mygauge->Mygauge_config->SCL, .scl_pullup_en = Mygauge->Mygauge_config->scl_pullup_en, 
        .master.clk_speed = Mygauge->Mygauge_config->clk_speed_hz
    };
 
 	return (Mygauge->i2c_bus_handle) ? true : ((Mygauge->i2c_bus_handle = i2c_bus_create(Mygauge->Mygauge_config->port_num, &cfg)) != NULL);

}	
	   
// --------------------------------------------------------------------------------------------------------------------------------------
bool Mygauge_reset(Mygauge_t *Mygauge)
{
	if (! Mygauge) return false;
	
	return max17048_reset(Mygauge->Mygauge_handle);
}
   
// --------------------------------------------------------------------------------------------------------------------------------
float Mygauge_get_voltage(Mygauge_t *Mygauge)
{
  if (! Mygauge) return -2;
  
  float voltage = -1;
  max17048_get_cell_voltage(Mygauge->Mygauge_handle, &voltage);
  
  return voltage; 
}

// --------------------------------------------------------------------------------------------------------------------------------
float Mygauge_get_percent(Mygauge_t *Mygauge)
{
	if (! Mygauge) return -2;
	
	float percent = -1;
	max17048_get_cell_percent(Mygauge->Mygauge_handle, &percent);
	
	return percent; 
}

// --------------------------------------------------------------------------------------------------------------------------------
float Mygauge_get_charge_rate(Mygauge_t *Mygauge)
{
	if (! Mygauge) return -2;
	
	float rate = -1;
	max17048_get_cell_percent(Mygauge->Mygauge_handle, &rate);
	
	return rate; 
}

// --------------------------------------------------------------------------------------------------------------------------------
uint16_t Mygauge_get_version(Mygauge_t *Mygauge)
{
	if (! Mygauge) return 0;
	
	uint16_t ic_version = 0;
	max17048_get_ic_version(Mygauge->Mygauge_handle, &ic_version);
	
	return ic_version;
}

// --------------------------------------------------------------------------------------------------------------------------------
bool Mygauge_set_temp_compensation(Mygauge_t *Mygauge, float temperature)
{
   if (! Mygauge) return false;
   
   return max17048_temperature_compensation(Mygauge->Mygauge_handle, temperature);
}

// --------------------------------------------------------------------------------------------------------------------------------
 bool Mygauge_begin(Mygauge_t *Mygauge)
{
	if (!Mygauge_i2c_init(Mygauge)) return false;
	
	Mygauge_reset(Mygauge);
	
	Mygauge->Mygauge_handle = max17048_create(Mygauge->i2c_bus_handle, Mygauge->Mygauge_config->dev_addr); // 0x36
	
	return (Mygauge->Mygauge_handle);
}

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */	    

void gauge_build(Mygauge_t *Mygauge, Mygauge_dev_config_t *Mygauge_confg)
{
    // Sécurités basiques : en debug, on stoppe si pointeur invalide.
    configASSERT(Mygauge != NULL);

    // Remet tout l’objet à zéro (handles, états, buffers…)
    //    => évite de réutiliser des handles I2C/RTOS périmés.
    memset(Mygauge, 0, sizeof(Mygauge_t));

    //Renseigne les "méthodes" (pointeurs de fonctions).
    //    Chaque fonction est responsable de ses vérifs d'arguments.
    Mygauge->Begin                  = Mygauge_begin;               // Init matériel (I2C, device, etc.)
    Mygauge->Reset                  = Mygauge_reset;               // Reset matériel si la broche est câblée
    Mygauge->get_version            = Mygauge_get_version;         // Lit la version/ID du composant
    Mygauge->get_percent            = Mygauge_get_percent;         // Pourcentage d'état de charge (SOC)
    Mygauge->get_voltage            = Mygauge_get_voltage;         // Tension batterie (V)
    Mygauge->get_charge_rate        = Mygauge_get_charge_rate;     // Taux de charge / courant estimé
    Mygauge->set_temp_compensation  = Mygauge_set_temp_compensation; // Ajuste la compensation en température

    // Choisit la configuration matérielle.
    //    - Si l'appelant ne fournit rien, on bascule sur la config statique par défaut.
    //    - Sinon on référence la config fournie (non copiée).
    //      => responsabilité de l'appelant de maintenir sa durée de vie.
    Mygauge->Mygauge_config = (Mygauge_confg == NULL) ? &Mygauge_default : Mygauge_confg;

    // (Optionnel) 4) Pré-initialiser des champs d'état/logiques si nécessaire.
    // ex: Mygauge->initialized = false; Mygauge->timestamp_ms = 0; etc.
}

// -------------------------------------------------------------------------------------------------------------------------------------