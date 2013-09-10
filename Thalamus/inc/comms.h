#ifndef __COMMS_H__
#define __COMMS_H__

#include "thal.h"

///////////////////////////////////////////// ILINK ///////////////////////////////////////
extern ilink_identify_t ilink_identify;
extern ilink_thalstat_t ilink_thalstat;
extern ilink_thalctrl_t ilink_thalctrl_rx;
extern ilink_thalctrl_t ilink_thalctrl_tx;
extern ilink_imu_t ilink_rawimu;
extern ilink_imu_t ilink_scaledimu;
extern ilink_altitude_t ilink_altitude;
extern ilink_attitude_t ilink_attitude;
extern ilink_thalparam_t ilink_thalparam_tx;
extern ilink_thalparam_t ilink_thalparam_rx;
extern ilink_thalpareq_t ilink_thalpareq;
extern ilink_iochan_t ilink_inputs0;
extern ilink_iochan_t ilink_outputs0;
extern ilink_gpsfly_t ilink_gpsfly;
extern ilink_gpsreq_t ilink_gpsreq;
extern ilink_debug_t ilink_debug;


// ****************************************************************************
// *** Communications Functions
// ****************************************************************************

// *** Deal with ilink requets between Hypo and Thalamus
void ILinkMessageRequest(unsigned short id);
// *** iLink interrupt handler between Hypo and Thalamus
void ILinkMessage(unsigned short id, unsigned short * buffer, unsigned short length);
#endif