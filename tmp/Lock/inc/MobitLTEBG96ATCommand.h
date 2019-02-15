/*
 * A library for Quectel BG96 Module
 * This file is about the BG96 AT Command list
 *
 * Copyright (c) 2019 Mobit technology inc.
 * @Author       : Damon
 * @Create time  : 02/13/2019
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERMOBIT_E, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _MOBIT_LTEBG96ATCOMMAND_H_
#define _MOBIT_LTEBG96ATCOMMAND_H_

// AT commands response
 static const char RESPONSE_READY[] = "RDY";
 static const char RESPONSE_OK[] = "OK";
 static const char RESPONSE_CRLF_OK[] = "\r\n\r\nOK";
 static const char RESPONSE_ERROR[] = "ERROR";
 static const char RESPONSE_POWER_DOWN[] = "POWERED DOWN";
 static const char RESPONSE_CONNECT[] = "CONNECT";
 static const char RESPONSE_SEND_OK[] = "SEND OK";
 static const char RESPONSE_SEND_FAIL[] = "SEND FAIL";

// common AT commands
 static const char DEV_AT[] = "";
 static const char DEV_INFORMATION[] = "I";
 static const char DEV_VERSION[] = "+CGMR";
 static const char DEV_IMEI[] = "+CGSN";
 static const char DEV_FUN_LEVEL[] = "+CFUN";
 static const char DEV_LOCAL_RATE[] = "+IPR";
 static const char DEV_SIM_IMSI[] = "+CIMI";
 static const char DEV_SIM_PIN[] = "+CPIN";
 static const char DEV_SIM_ICCID[] = "+QCCID";
 static const char DEV_NET_STATUS[] = "+CREG";
 static const char DEV_NET_STATUS_G[] = "+CGREG";
 static const char DEV_EPS_NET_STATUS[] = "+CEREG";
 static const char DEV_NET_RSSI[] = "+CSQ";
 static const char DEV_NET_OPERATOR[] = "+COPS";
 static const char DEV_NET_INFORMATION[] = "+QNWINFO";
 static const char DEV_NET_PACKET_COUNTER[] = "+QGDCNT";
 static const char DEV_POWER_DOWN[] = "+QPOWD";
 static const char DEV_CLOCK[] = "+CCLK";

// TCPIP AT Commands
 static const char APN_PARAMETERS[] = "+QICSGP";
 static const char ACTIVATE_APN[] = "+QIACT";
 static const char DEACTIVATE_APN[] = "+QIDEACT";
 static const char GET_APN_IP_ADDRESS[] = "+CGPADDR";
 static const char OPEN_SOCKET[] = "+QIOPEN";
 static const char CLOSE_SOCKET[] = "+QICLOSE";
 static const char SOCKET_STATUS[] = "+QISTATE";
 static const char SOCKET_SEND_DATA[] = "+QISEND";
 static const char SOCKET_READ_DATA[] = "+QIRD";
 static const char SOCKET_SEND_HEX_DATA[] = "+QISENDEX";
 static const char DATA_ACCESS_MODES[] = "+QISWTMD";
 static const char PING_FUNCTION[] = "+QPING";
 static const char NTP_FUNCTION[] = "+QNTP";
 static const char CONFIGURE_DNS_SERVER[] = "+QIDNSCFG";
 static const char DNS_FUNCTION[] = "+QIDNSGIP";
 static const char QUERY_ERROR_CODE[] = "+QIGETERROR";
 static const char RECV_SOCKET_EVENT[] = "+QIURC";

// FILE AT Commands
 static const char FILE_SPACE_INFORMATION[] = "+QFLDS";
 static const char FILE_LIST_FILES[] = "+QFLST";
 static const char FILE_DELETE_FILES[] = "+QFDEL";
 static const char FILE_UPLOAD_FILES[] = "+QFUPL";
 static const char FILE_DOWNLOAD_FILE[] = "+QFDWL";
 static const char FILE_OPEN_FILE[] = "+QFOPEN";
 static const char FILE_READ_FILE[] = "+QFREAD";
 static const char FILE_WRITE_FILE[] = "+QFWRITE";
 static const char FILE_SET_POSITION_FILE[] = "+QFSEEK";
 static const char FILE_GET_POSITION_FILE[] = "+QFPOSITION";
 static const char FILE_TRUNCATE_FILE[] = "+QFTUCAT";
 static const char FILE_CLOSE_FILE[] = "+QFCLOSE";

// SSL AT Commands
 static const char SSL_CONFIG_PARAMETER[] = "+QSSLCFG";
 static const char SSL_OPEN_SOCKET[] = "+QSSLOPEN";
 static const char SSL_SEND_DATA[] = "+QSSLSEND";
 static const char SSL_READ_DATA[] = "+QSSLRECV";
 static const char SSL_CLOSE_SOCKET[] = "+QSSLCLOSE";
 static const char SSL_QUERY_STATUS[] = "+QSSLSTATE";
 static const char SSL_SOCKET_EVENT[] = "+QSSLURC";

// HTTP AT Commands
 static const char HTTP_CONFIG_PARAMETER[] = "+QHTTPCFG";
 static const char HTTP_SET_URL[] = "+QHTTPURL";
 static const char HTTP_GET_REQUEST[] = "+QHTTPGET";
 static const char HTTP_POST_REQUEST[] = "+QHTTPPOST";
 static const char HTTP_FILE_POST_REQUEST[] = "+QHTTPPOSTFILE";
 static const char HTTP_READ_RESPONSE[] = "+QHTTPREAD";
 static const char HTTP_FILE_READ_RESPONSE[] = "+QHTTPREADFILE";

// MQTT AT Commands
 static const char MQTT_CONFIG_PARAMETER[] = "+QMTCFG";
 static const char MQTT_OPEN_NETWORK[] = "+QMTOPEN";
 static const char MQTT_CLOSE_NETWORK[] = "+QMTCLOSE";
 static const char MQTT_CREATE_CLIENT[] = "+QMTCONN";
 static const char MQTT_CLOSE_CLIENT[] = "+QMTDISC";
 static const char MQTT_SUBSCRIBE_TOPICS[] = "+QMTSUB";
 static const char MQTT_UNSUBSCRIBE_TOPICS[] = "+QMTUNS";
 static const char MQTT_PUBLISH_MESSAGES[] = "+QMTPUB";
 static const char MQTT_STATUS[] = "+QMTSTAT";
 static const char MQTT_RECV_DATA[] = "+QMTRECV";

// GNSS AT Commands
 static const char GNSS_CONFIGURATION[] = "+QGPSCFG";
 static const char GNSS_TURN_ON[] = "+QGPS";
 static const char GNSS_TURN_OFF[] = "+QGPSEND";
 static const char GNSS_GET_POSITION[] = "+QGPSLOC";
 static const char GNSS_ACQUIRE_NMEA[] = "+QGPSGNMEA";

#endif
