#include "MY_LORA.h"

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Variables interne :::::::::::::::::::::::::::::::::::::::::::::: */	    

  
/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Fonctions Static:::::::::::::::::::::::::::::::::::::::::::::: */

bool Enable_LoRaWAN_mode(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa,"AT+NWM=1\r\n", false, 10).sentence, "ok") == 0 );	
}

// --------------------------------------------------------------------------------------------------------------------------------------
bool Is_LoRaWAN_mode(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa,"AT+NWM=?\r\n", false, 10).sentence, "AT+NWM=1") == 0 );
}

// ---------------------------------------------------------------------------------------------------------------------------------------
bool join(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  str512_t reply = MyLoRa->Send_command(MyLoRa, "AT+JOIN\r\n", true, 18000);
  return (strcasecmp(reply.sentence, "+EVT:JOINED") == 0 );
}

// ---------------------------------------------------------------------------------------------------------------------------------------
bool Is_Joined(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa,"AT+NJS=?\r\n", false, 10).sentence, "AT+NJS=1") == 0 );	
}

// ---------------------------------------------------------------------------------------------------------------------------------------
bool Enable_Confirm_mode(MyLoRa_t *MyLoRa, bool Mode)
{
  if(!MyLoRa) return false;
  char cmd[128];
  
  snprintf(cmd, sizeof(cmd), "AT+CFM=%d\r\n", Mode);
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "ok") == 0 );
}

// ---------------------------------------------------------------------------------------------------------------------------------------
bool Enable_private_mode(MyLoRa_t *MyLoRa, bool Mode)
{
  if(!MyLoRa) return false;
  char cmd[128];
  
  snprintf(cmd, sizeof(cmd), "AT+PNM=%d\r\n", !Mode);
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "ok") == 0 );
}
 
// ---------------------------------------------------------------------------------------------------------------------------------------
bool Is_private_mode(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa,"AT+PNM=?\r\n", false, 10).sentence, "AT+PNM=0") == 0 );
}
 
// ---------------------------------------------------------------------------------------------------------------------------------------
bool Is_Confirm_mode(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa,"AT+CFM=?\r\n", false, 10).sentence, "AT+CFM=1") == 0 );
}

// ---------------------------------------------------------------------------------------------------------------------------------------
bool Enable_OTAA(MyLoRa_t *MyLoRa, bool Mode)
{
  if(!MyLoRa) return false;
  char cmd[128];
  
  snprintf(cmd, sizeof(cmd), "AT+NJM=%d\r\n", Mode);
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "ok") == 0 );	
}

// ---------------------------------------------------------------------------------------------------------------------------------------
bool Is_OTAA(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa,"AT+NJM=?\r\n", false, 10).sentence, "AT+NJM=1") == 0 );	
}

// ---------------------------------------------------------------------------------------------------------------------------------------
bool Enable_ADR(MyLoRa_t *MyLoRa, bool Mode)
{
  if(!MyLoRa) return false;
  char cmd[128];
  
  snprintf(cmd, sizeof(cmd), "AT+ADR=%d\r\n", Mode);
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "ok") == 0 );	
}
   
// ---------------------------------------------------------------------------------------------------------------------------------------
bool Is_ADR(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return false;
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa,"AT+ADR=?\r\n", false, 10).sentence, "AT+ADR=1") == 0 );	
}

// ---------------------------------------------------------------------------------------------------------------------------------------
int8_t get_power_receive(MyLoRa_t *MyLoRa)
{
 return atoi(MyLoRa->Send_command(MyLoRa,"AT+DR=?\r\n", false, 10).sentence);
}   
 
// ---------------------------------------------------------------------------------------------------------------------------------------
bool set_power_receive(MyLoRa_t *MyLoRa, int8_t power)
{
  if(!MyLoRa) return false;
  char cmd[128];
  
  snprintf(cmd, sizeof(cmd), "AT+DR=%d\r\n", power);
  return ( strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "ok") == 0 );
}

// ---------------------------------------------------------------------------------------------------------------------------------------
int8_t get_power_transmit(MyLoRa_t *MyLoRa)
{
 	return atoi(MyLoRa->Send_command(MyLoRa,"AT+TXP=?\r\n", false, 10).sentence);
} 
 
// ---------------------------------------------------------------------------------------------------------------------------------------
 bool set_power_transmit(MyLoRa_t *MyLoRa, int8_t power)
{
  	if(!MyLoRa) return false;
  	char cmd[128];
  	
  	snprintf(cmd, sizeof(cmd), "AT+TXP=?=%d\r\n", power);
  	return ( strcasecmp(MyLoRa->Send_command(MyLoRa, cmd, false, 10).sentence, "ok") == 0 );
}

// ---------------------------------------------------------------------------------------------------------------------------------------
int8_t get_netid(MyLoRa_t *MyLoRa)
{
  if(!MyLoRa) return 0;
  str512_t reply = MyLoRa->Send_command(MyLoRa,"AT+NETID=?\r\n", false, 10);
  char *pattern  = strchr(reply.sentence, '=');
  
  if(!pattern) {return 0;}
  strcpy(reply.sentence, pattern+1);
  return atoi(reply.sentence);
	
}

// ---------------------------------------------------------------------------------------------------------------------------------------
int8_t get_rssi(MyLoRa_t *MyLoRa)
{
  str512_t reply = MyLoRa->Send_command(MyLoRa,"AT+RSSI=?\r\n", false, 10);
  char *pattern  = strchr(reply.sentence, '=');
  
  if(!pattern) {return 0;}
  strcpy(reply.sentence, pattern+1);
  return atoi(reply.sentence);	
}

// ---------------------------------------------------------------------------------------------------------------------------------------
int8_t get_snr(MyLoRa_t *MyLoRa)
{
  str512_t reply = MyLoRa->Send_command(MyLoRa,"AT+SNR=?\r\n", false, 10);
  char *pattern  = strchr(reply.sentence, '=');
  
  if(!pattern) {return 0;}
  strcpy(reply.sentence, pattern+1);
  return atoi(reply.sentence);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
bool Send_payload(MyLoRa_t *MyLoRa, char *Payload, uint8_t Port)
{
  if(!MyLoRa) return false;
  char cmd[128];
  	
  snprintf(cmd, sizeof(cmd), "AT+SEND=%d:%s\r\n", Port, Payload);
  str512_t reply = MyLoRa->Send_command(MyLoRa, cmd, true, 17000);
  return (strcasecmp(reply.sentence, "+EVT:SEND_CONFIRMED_OK") == 0 || strcasecmp(reply.sentence, "+EVT:TX_DONE") == 0 );
}   
    
// ---------------------------------------------------------------------------------------------------------------------------------------
str512_t Receive_payload(MyLoRa_t *MyLoRa)
{
	return MyLoRa->Send_command(MyLoRa, "AT+RECV=?\r\n", false, 18000);
}

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */
	   /* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Constructeur :::::::::::::::::::::::::::::::::::::::::::::: */	    
void LoRaWAN_request_build(MyLoRa_t *MyLoRa)
{
	if (!MyLoRa) return;

    MyLoRa->LoRaWAN.is_LoRaWAN_mode      = Is_LoRaWAN_mode;
    MyLoRa->LoRaWAN.Enable_LoRaWAN_mode  = Enable_LoRaWAN_mode;

    MyLoRa->LoRaWAN.Join                 = join;
    MyLoRa->LoRaWAN.is_Joined            = Is_Joined;

    MyLoRa->LoRaWAN.Enable_Confirm_mode  = Enable_Confirm_mode;
    MyLoRa->LoRaWAN.is_Confirm_mode      = Is_Confirm_mode;

    MyLoRa->LoRaWAN.Enable_private_mode  = Enable_private_mode;
    MyLoRa->LoRaWAN.is_private_mode      = Is_private_mode;

    MyLoRa->LoRaWAN.Enable_OTAA          = Enable_OTAA;
    MyLoRa->LoRaWAN.is_OTAA              = Is_OTAA;

    MyLoRa->LoRaWAN.Enable_ADR           = Enable_ADR;
    MyLoRa->LoRaWAN.is_ADR               = Is_ADR;

    MyLoRa->LoRaWAN.get_power_receive    = get_power_receive;
    MyLoRa->LoRaWAN.set_power_receive    = set_power_receive;

    MyLoRa->LoRaWAN.get_power_transmit   = get_power_transmit;
    MyLoRa->LoRaWAN.set_power_transmit   = set_power_transmit;

    MyLoRa->LoRaWAN.get_netid            = get_netid;
    MyLoRa->LoRaWAN.get_rssi             = get_rssi;
    MyLoRa->LoRaWAN.get_snr              = get_snr;

    MyLoRa->LoRaWAN.Send_Payload         = Send_payload;
    MyLoRa->LoRaWAN.Receive_Payload      = Receive_payload;
}