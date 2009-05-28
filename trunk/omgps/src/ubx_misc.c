#if 0
/************************ utilities: help know details *******************************
*						  unused by runtime system.
**************************************************************************************/

#define UBX_ID_CFG_RXM		0x11
#define UBX_ID_CFG_FXN		0x0E


/* FixNow mode, on time, >=10 */
#define FXN_ON_SECONDS				10
/* FixNow mode, off/reacq-off time */
#define FXN_OFF_SECONDS				60

static const ubx_msg_type_t type_cfg_fxn = 		{UBX_CLASS_CFG, UBX_ID_CFG_FXN};

static const ubx_msg_type_t type_mon_schd = 	{UBX_CLASS_MON, UBX_ID_MON_SCHD};
static const ubx_msg_type_t type_mon_msgpp = 	{UBX_CLASS_MON, UBX_ID_MON_MSGPP};
static const ubx_msg_type_t type_inf_error = 	{UBX_CLASS_INF, UBX_ID_INF_ERROR};
static const ubx_msg_type_t type_inf_warning = 	{UBX_CLASS_INF, UBX_ID_INF_WARNING};
static const ubx_msg_type_t type_cfg_nav2 = 	{UBX_CLASS_CFG, UBX_ID_CFG_NAV2};

/**
 * http://www.u-blox.com/customersupport/gps.g3/ANTARIS_EvalKit_User_Guide(GPS.G3-EK-02003).pdf
 *
 * When waking up TIM-Lx with RxD1, RxD2 from sleep- or backup state send a number (at least 8) of
 * 0xFF characters to wake up the serial communication module otherwise the first bytes may get lost. To
 * request a position fix a position request message must be sent via serial port after waking up the
 * ANTARIS GPS Receiver.
 *
 * http://www.mikrocontroller.net/attachment/36822/NL_60415_-_Datenblatt_u-blox_GPS_Module_24092007_481.pdf
 */
inline void wakeup_fxn_mode()
{
	static const U1 wakeup[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xB5, 0x62, 0x02, 0x40, 0x00, 0x00, 0x42, 0xC8 };

	if (write(gps_dev_fd, wakeup, sizeof(wakeup)) <= 0)
		log_debug("wakeup sleep mode failed...");
	tcdrain(gps_dev_fd);
	sleep(1);
}

/**
 * On-off mode.
 * If T_reacq is longer than T_on a reacquisition timeout will never happen,
 * because T_on overrules T_reacq.
 */
gboolean ubx_cfg_fxn (gboolean readack)
{
	#define MIN_MS	60 * 1000

	U1 flags = 0x02 | 0x10; /* (1) Sleep Mode, and (2) Use on/off time */
	U1 acq_minutes = 2;
	U1 reacq_minutes = 1;
	/* If T_on is set to 0s, the receiver will enter Off State as soon as the Position Accuracy
	 * is below the Accuracy Mask. For an improved accuracy, increase T_on by a few seconds. */
	U4 on_ms = FXN_ON_SECONDS * 1000;
	U4 acq_ms = acq_minutes * MIN_MS;
	U4 reacq_ms = reacq_minutes * MIN_MS;
	U4 off_ms = FXN_OFF_SECONDS * 1000;

	U1 acq[4], reacq[4], on[4], off[4];

	int i, j;
	for (i=0; i<4; i++) {
		j = i << 3;
		acq[i] = (acq_ms >> j) & 0xFF;
		reacq[i] = (reacq_ms >> j) & 0xFF;
		on[i] = (on_ms >> j) & 0xFF;
		off[i] = (off_ms >> j) & 0xFF;
	}

	U1 packet[] = {
		0xB5, 0x62,
		type_cfg_fxn.class, type_cfg_fxn.id,
		36, 0x00, 				/* length */
		flags, 0x00, 0x00, 0x00, /* flags */
		reacq[0], reacq[1], reacq[2], reacq[3], /* t_reacq */
		acq[0], acq[1], acq[2], acq[3], /* t_acq */
		off[0], off[1], off[2], off[3], /* t_reacq_off */
		off[0], off[1], off[2], off[3], /* t_acq_off */
		on[0], on[1], on[2], on[3], /* t_on */
		off[0], off[1], off[2], off[3], /* t_off */
		0x00, 0x00, 0x00, 0x00, /* res */
		0x00, 0x00, 0x00, 0x00, /* base_tow */
		0x00, 0x00				/* checksum */
	};

	if (! ubx_issue_cmd(packet, sizeof(packet)))
		return FALSE;
	if(readack)
		return ubx_read_ack(&type_cfg_fxn);
	return TRUE;
}


static void ubx_parse_mon_schd(ubx_msg_t *msg)
{
	float idle_ms = (float)(msg->payload_addr[20] + (msg->payload_addr[21] << 8));
	log_debug("GPS receiver CPU load=%.2f%%", 100.0 - idle_ms / 10.0);
}

void ubx_mon_schd_poll()
{
	U1 packet[] = {
		0xB5, 0x62,
		type_mon_schd.class, type_mon_schd.id,
		0x00, 0x00,
		0x00, 0x00
	};

	ubx_msg_t msg;

	if (! (ubx_issue_cmd(packet, sizeof(packet)) &&
		ubx_read_next_msg(&msg, &type_mon_schd))) {
		printf("ubx_mon_schd_poll: failed.\n");
		return;
	}

	ubx_parse_mon_schd(&msg);
}

void ubx_mon_msgpp_poll()
{
	U1 packet[] = {
		0xB5, 0x62,
		type_mon_msgpp.class, type_mon_msgpp.id,
		0x00, 0x00,
		0x00, 0x00
	};

	ubx_msg_t msg;

	if (! (ubx_issue_cmd(packet, sizeof(packet)) &&
		ubx_read_next_msg(&msg, &type_mon_msgpp))) {
		printf("ubx_mon_msgpp_poll: failed.\n");
		return;
	}

	printf("msgapp----------------------:\n");
	int i, j = 32;
	for (i=0; i<16; i++) {
		j += i << 1;
		printf("%d:%d, ", i, msg.payload_addr[j] + (msg.payload_addr[j+1] << 8));
	}
	printf("\n");

	for (i=0; i<16; i++) {
		j += i << 1;
		printf("%d:%d, ", i, msg.payload_addr[j] + (msg.payload_addr[j+1] << 8));
	}
	printf("\n");
}


/* for debugging, see whether an individual NMEA message is enabled or not. */
void ubx_cfg_msg_poll(gboolean nmea_std, U1 msg_id)
{
	U1 packet[] = {
		0xB5, 0x62,
		type_cfg_msg.class, type_cfg_msg.id,
		0x02, 0x00,
		(nmea_std? 0xF0:0xF1), msg_id,
		0x00, 0x00
	};

	ubx_msg_t msg;

	printf("ubx_cfg_msg_poll: NMEA type=%s, id=%d\n", (nmea_std? "NMEA STD" : "NMEA UBX"), msg_id);

	if (! (ubx_issue_cmd(packet, sizeof(packet)) &&
		ubx_read_next_msg(&msg, &type_cfg_msg) &&
		ubx_read_ack(&type_cfg_msg))) {
		printf("ubx_cfg_msg_poll: failed.\n");
		return;
	}

	int i;
	for (i=0; i<msg.payload_len; i++)
		printf("0x%02x ", msg.payload_addr[i]);
	printf("\n");
}

void ubx_cfg_inf_poll(U1 protocol_id)
{
	U1 packet[] = {
		0xB5, 0x62,
		type_cfg_inf.class, type_cfg_inf.id,
		0x01, 0x00,
		protocol_id,
		0x00, 0x00
	};

	ubx_msg_t msg;

	if (! (ubx_issue_cmd(packet, sizeof(packet)) &&
		ubx_read_next_msg(&msg, &type_cfg_inf) &&
		ubx_read_ack(&type_cfg_inf))) {
		printf("ubx_cfg_inf_poll: failed.\n");
		return;
	}

	log_debug("0x%02x 0x%02x 0x%02x 0x%02x",
		msg.payload_addr[4], msg.payload_addr[5], msg.payload_addr[6], msg.payload_addr[7]);
}

/* for debugging, see a port configuration.
 * NOTE: two ports are enabled by default! OM kernel bug? */
void ubx_cfg_prt_poll(U1 prt_id)
{
	U1 packet[] = {
		0xB5, 0x62,
		type_cfg_prt.class, type_cfg_prt.id,
		0x01, 0x00,
		prt_id,
		0x00, 0x00
	};

	ubx_msg_t msg;

	printf("ubx_cfg_prt_poll: port id=%d\n", prt_id);

	if (! (ubx_issue_cmd(packet, sizeof(packet)) &&
		ubx_read_next_msg(&msg, &type_cfg_prt) &&
		ubx_read_ack(&type_cfg_prt))) {
		printf("ubx_cfg_prt_poll: failed.\n");
		return;
	}

	int i;
	for (i=0; i<msg.payload_len;i++)
		printf("0x%02x ", msg.payload_addr[i]);
	printf("\n");
}

void ubx_cfg_rxm_poll()
{
	U1 packet[] = {
		0xB5, 0x62,
		type_cfg_rxm.class, type_cfg_rxm.id,
		0x00, 0x00,
		0x00, 0x00
	};
	ubx_msg_t msg;

	if (! (ubx_issue_cmd(packet, sizeof(packet)) &&
		ubx_read_next_msg(&msg, &type_cfg_rxm) &&
		ubx_read_ack(&type_cfg_rxm))) {
		printf("ubx_cfg_rxm_poll: failed.\n");
		return;
	}

	log_debug("ubx_cfg_rxm_poll: gps mode=0x%02X, low power mode=0x%02X",
		msg.payload_addr[0], msg.payload_addr[1]);
}

void ubx_cfg_fxn_poll()
{
	U1 packet[] = {
		0xB5, 0x62,
		type_cfg_fxn.class, type_cfg_fxn.id,
		0x00, 0x00,
		0x00, 0x00
	};
	ubx_msg_t msg;

	if (! (ubx_issue_cmd(packet, sizeof(packet)) &&
		ubx_read_next_msg(&msg, &type_cfg_fxn) &&
		ubx_read_ack(&type_cfg_fxn))) {
		printf("ubx_cfg_fxn_poll: failed.\n");
		return;
	}

	int i;
	for (i=0; i<msg.payload_len; i++)
		printf("%02X ", msg.payload_addr[i]);
	printf("\n");
}

void ubx_cfg_nav2_poll()
{
	U1 packet[] = {
		0xB5, 0x62,
		type_cfg_nav2.class, type_cfg_nav2.id,
		0x00, 0x00,
		0x00, 0x00
	};
	ubx_msg_t msg;

	if (! (ubx_issue_cmd(packet, sizeof(packet)) &&
		ubx_read_next_msg(&msg, &type_cfg_nav2) &&
		ubx_read_ack(&type_cfg_nav2))) {
		printf("ubx_cfg_nav2_poll: failed.\n");
		return;
	}

	int i;
	for (i=0; i<msg.payload_len; i++)
		printf("%02X ", msg.payload_addr[i]);
	printf("\n");
}

#endif
