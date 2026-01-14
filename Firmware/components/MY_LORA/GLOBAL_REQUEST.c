#include "MY_LORA.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables interne :::::::::::::::::::::::::::::::::::::::::::::: */	    

  
/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions Info:::::::::::::::::::::::::::::::::::::::::::::: */
	   
bool Info_sleep(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  return (strcasecmp(MyLoRa->Send_command(MyLoRa,"AT+LPM=?\r\n", false, 10).sentence, "AT+LPM=1") == 0);
}

// --------------------------------------------------------------------------------------------------------------------------------------
str512_t Info_name(MyLoRa_t *MyLoRa)
{
  str512_t reply = MyLoRa->Send_command(MyLoRa,"AT+ALIAS=?\r\n", false, 15);
  char *pattern  = strchr(reply.sentence, '=');
  
  if(!pattern) {*reply.sentence = '\0'; return reply;}
  strcpy(reply.sentence, pattern+1);
  return reply;
}
 
// -------------------------------------------------------------------------------------------------------------------------------------- 
str512_t Info_baud(MyLoRa_t *MyLoRa)
{
  str512_t reply = MyLoRa->Send_command(MyLoRa,"AT+BAUD=?\r\n", false, 15);
  char *pattern  = strchr(reply.sentence, '=');
  
  if(!pattern) {*reply.sentence = '\0'; return reply;}
  strcpy(reply.sentence, pattern+1);
  return reply;
}

// --------------------------------------------------------------------------------------------------------------------------------
str512_t Info_model(MyLoRa_t *MyLoRa)
{
  str512_t reply = MyLoRa->Send_command(MyLoRa,"AT+HWMODEL=?\r\n", false, 15);
  char *pattern  = strchr(reply.sentence, '=');
  
  if(!pattern) {*reply.sentence = '\0'; return reply;}
  strcpy(reply.sentence, pattern+1);
  return reply;
}

// --------------------------------------------------------------------------------------------------------------------------------
str512_t Info_voltage(MyLoRa_t *MyLoRa)
{
  str512_t reply = MyLoRa->Send_command(MyLoRa,"AT+SYSV=?\r\n", false, 15);
  char *pattern  = strchr(reply.sentence, '=');
  
  if(!pattern) {*reply.sentence = '\0'; return reply;}
  strcpy(reply.sentence, pattern+1);
  return reply;
}

// ---------------------------------------------------------------------------------------------------------------------------------
str512_t Info_version(MyLoRa_t *MyLoRa)
{
  str512_t reply = MyLoRa->Send_command(MyLoRa,"AT+VER=?\r\n", false, 15);
  char *pattern  = strchr(reply.sentence, '=');
  
  if(!pattern) {*reply.sentence = '\0'; return reply;}
  strcpy(reply.sentence, pattern+1);
  return reply;
}
				
// ---------------------------------------------------------------------------------------------------------------------------------
str512_t Info_SN(MyLoRa_t *MyLoRa)
{
  str512_t reply = MyLoRa->Send_command(MyLoRa,"AT+SN=?\r\n", false, 	15);
  char *pattern  = strchr(reply.sentence, '=');
  
  if(!pattern) {*reply.sentence = '\0'; return reply;}
  strcpy(reply.sentence, pattern+1);
  return reply;
}
	
/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions Config:::::::::::::::::::::::::::::::::::::::::::::: */
	   
bool check_at_line(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  return (strcasecmp(MyLoRa->Send_command(MyLoRa,"AT\r\n", false, 10).sentence, "OK") == 0);
}
  
// --------------------------------------------------------------------------------------------------------------------------------
bool Soft_reset(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  MyLoRa->Send_command(MyLoRa,"ATZ\r\n", false, 15);
  return true;
}

// --------------------------------------------------------------------------------------------------------------------------------

bool Set_factory(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  MyLoRa->Send_command(MyLoRa,"ATR\r\n", false, 10);
  return true;
}

// -------------------------------------------------------------------------------------------------------------------------------- 
bool enable_pwd(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  MyLoRa->Send_command(MyLoRa,"AT+LOCK\r\n", false, 10);
  return true;	
}

// --------------------------------------------------------------------------------------------------------------------------------
bool Change_password(MyLoRa_t *MyLoRa, char *pwd)
{
  if(!MyLoRa) return false;
  char cmd[128];
  
  snprintf(cmd, sizeof(cmd), "AT+PWORD=%s\r\n", pwd);
  return (strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "OK") == 0);	
}
    
// --------------------------------------------------------------------------------------------------------------------------------
bool Sleep(MyLoRa_t *MyLoRa, bool mode)
{
  if(!MyLoRa) return false;
  char cmd[128];
  
  snprintf(cmd, sizeof(cmd), "AT+LPM=%d\r\n", mode);
  return (strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "OK") == 0);	
}

// --------------------------------------------------------------------------------------------------------------------------------
bool Sleep_level(MyLoRa_t *MyLoRa, uint8_t mode)
{
  if(!MyLoRa) return false;
  char cmd[128];
  
  snprintf(cmd, sizeof(cmd), "AT+LPMLVL=%d\r\n", mode);
  return (strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "OK") == 0);	
}

// --------------------------------------------------------------------------------------------------------------------------------
bool Set_name(MyLoRa_t *MyLoRa, char *name)
{
  if(!MyLoRa) return false;
  char cmd[128];
  
  snprintf(cmd, sizeof(cmd), "AT+ALIAS=%s\r\n", name);
  return (strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "OK") == 0);	
}

// --------------------------------------------------------------------------------------------------------------------------------
bool Set_baud(MyLoRa_t *MyLoRa, uint32_t new_baud)
{
  if(!MyLoRa) return false;
  char cmd[128];
  
  snprintf(cmd, sizeof(cmd), "AT+BAUD=%ld\r\n", new_baud);
  return (strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "OK") == 0);	
}

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */	    
void Gloabal_request_build(MyLoRa_t *MyLoRa)
{
	   if (!MyLoRa) return;
	   
	 /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions Info:::::::::::::::::::::::::::::::::::::::::::::: */
	 	MyLoRa->Info.get_model         = Info_model;
	 	MyLoRa->Info.get_name          = Info_name;
   		MyLoRa->Info.get_baud          = Info_baud;
   		MyLoRa->Info.get_serial_number = Info_SN;
   		MyLoRa->Info.get_version       = Info_version;
   		MyLoRa->Info.is_sleep          = Info_sleep;
   		MyLoRa->Info.get_voltage       = Info_voltage;
   		
   	/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions Config:::::::::::::::::::::::::::::::::::::::::::::: */
   	    MyLoRa->Config.AT_check = check_at_line;
   	    MyLoRa->Config.sleep    = Sleep;
   	    MyLoRa->Config.sleep_level = Sleep_level;  
   	    MyLoRa->Config.soft_reset  =  Soft_reset;
   	    MyLoRa->Config.set_factory =  Set_factory;
   	    MyLoRa->Config.set_baud    =  Set_baud;
   	    MyLoRa->Config.set_name    =  Set_name;
   	    MyLoRa->Config.change_password = Change_password;
   	    MyLoRa->Config.Enable_password = enable_pwd;
}

// -------------------------------------------------------------------------------------------------------------------------------------

