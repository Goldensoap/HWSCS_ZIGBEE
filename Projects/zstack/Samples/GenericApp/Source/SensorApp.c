/******************************************************************************
  Filename:       SensorApp.c
  Revised:        $Date: 2012-03-07 01:04:58 -0800 (Wed, 07 Mar 2012) $
  Revision:       $Revision: 29656 $

  Description:    Generic Application (no Profile).


  Copyright 2004-2012 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License"). You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED ï¿½AS ISï¿½ WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
******************************************************************************/

/*********************************************************************
  This application isn't intended to do anything useful, it is
  intended to be a simple example of an application's structure.

  This application sends "Hello World" to another "Generic"
  application every 5 seconds.  The application will also
  receives "Hello World" packets.

  The "Hello World" messages are sent/received as MSG type message.

  This applications doesn't have a profile, so it handles everything
  directly - itself.

  Key control:
    SW1:
    SW2:  initiates end device binding
    SW3:
    SW4:  initiates a match description request
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "NLMEDE.h"

#include "SensorApp.h"
#include "DebugTrace.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"

/* RTOS */
#if defined( IAR_ARMCM3_LM )
#include "RTOS_App.h"
#endif  

#include "DHT11.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// This list should be filled with Application specific Cluster IDs.
const cId_t SensorApp_ClusterList[SENSORAPP_MAX_CLUSTERS] =
{
  SENSORAPP_CLUSTERID
};

const SimpleDescriptionFormat_t SensorApp_SimpleDesc =
{
  SENSORAPP_ENDPOINT,              //  int Endpoint;
  SENSORAPP_PROFID,                //  uint16 AppProfId[2];
  SENSORAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SENSORAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SENSORAPP_FLAGS,                 //  int   AppFlags:4;
  SENSORAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)SensorApp_ClusterList,  //  byte *pAppInClusterList;
  SENSORAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)SensorApp_ClusterList   //  byte *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SensorApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SensorApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
byte SensorApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SensorApp_Init() is called.
devStates_t SensorApp_NwkState;


byte SensorApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SensorApp_DstAddr;

static uint8 Room=0x01;

static struct 
{

    uint16  NWK_ADDR;
    uint8   room;
    uint8   sensorNum;
    uint8   sensorType;
    uint16  data;

}SensorData;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void SensorApp_HandleKeys( byte shift, byte keys );
static void SensorApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
static void SensorApp_SendTheMessage( void );

#if defined( IAR_ARMCM3_LM )
static void SensorApp_ProcessRtosMessage( void );
#endif

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SensorApp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void SensorApp_Init( uint8 task_id )
{
  SensorApp_TaskID = task_id;
  SensorApp_NwkState = DEV_INIT;
  SensorApp_TransID = 0;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

  SensorApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  SensorApp_DstAddr.endPoint = 10;
  SensorApp_DstAddr.addr.shortAddr = 0x0000;

  // Fill out the endpoint description.
  SensorApp_epDesc.endPoint = SENSORAPP_ENDPOINT;
  SensorApp_epDesc.task_id = &SensorApp_TaskID;
  SensorApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SensorApp_SimpleDesc;
  SensorApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &SensorApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( SensorApp_TaskID );

    /*传感器硬件初始化*/
    DHT11_init();

  // Update the display
#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "SensorApp", HAL_LCD_LINE_1 );
#endif

#if defined( IAR_ARMCM3_LM )
  // Register this task with RTOS task initiator
  RTOS_RegisterApp( task_id, SENSORAPP_RTOS_MSG_EVT );
#endif
}

/*********************************************************************
 * @fn      SensorApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
uint16 SensorApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  afDataConfirm_t *afDataConfirm;

  // Data Confirmation message fields
  byte sentEP;
  ZStatus_t sentStatus;
  byte sentTransID;       // This should match the value sent
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SensorApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {

        case KEY_CHANGE:
          SensorApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case AF_DATA_CONFIRM_CMD:
          // This message is received as a confirmation of a data packet sent.
          // The status is of ZStatus_t type [defined in ZComDef.h]
          // The message fields are defined in AF.h
          afDataConfirm = (afDataConfirm_t *)MSGpkt;
          sentEP = afDataConfirm->endpoint;
          sentStatus = afDataConfirm->hdr.status;
          sentTransID = afDataConfirm->transID;
          (void)sentEP;
          (void)sentTransID;

          // Action taken when confirmation is received.
          if ( sentStatus != ZSuccess )
          {
            // The data wasn't delivered -- Do something
          }
          break;

        case ZDO_STATE_CHANGE:
          SensorApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if ( (SensorApp_NwkState == DEV_ZB_COORD)
              || (SensorApp_NwkState == DEV_ROUTER)
              || (SensorApp_NwkState == DEV_END_DEVICE) )
          {
            // Start sending "the" message in a regular interval.
            osal_start_timerEx( SensorApp_TaskID,
                                SENSORAPP_SEND_MSG_EVT,
                                SENSORAPP_SEND_MSG_TIMEOUT );
          }
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SensorApp_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  // Send a message out - This event is generated by a timer
  //  (setup in SensorApp_Init()).
  if ( events & SENSORAPP_SEND_MSG_EVT )
  {
    // Send "the" message
    SensorApp_SendTheMessage();

    // Setup to send message again
    osal_start_timerEx( SensorApp_TaskID,
                        SENSORAPP_SEND_MSG_EVT,
                        SENSORAPP_SEND_MSG_TIMEOUT );

    // return unprocessed events
    return (events ^ SENSORAPP_SEND_MSG_EVT);
  }

  
#if defined( IAR_ARMCM3_LM )
  // Receive a message from the RTOS queue
  if ( events & SENSORAPP_RTOS_MSG_EVT )
  {
    // Process message from RTOS queue
    SensorApp_ProcessRtosMessage();

    // return unprocessed events
    return (events ^ SENSORAPP_RTOS_MSG_EVT);
  }
#endif

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * Event Generation Functions
 */

/*********************************************************************
 * @fn      SensorApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_4
 *                 HAL_KEY_SW_3
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void SensorApp_HandleKeys( uint8 shift, uint8 keys )
{

}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      SensorApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
static void SensorApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  switch ( pkt->clusterId )
  {
    case SENSORAPP_CLUSTERID:
      // "the" message
#if defined( LCD_SUPPORTED )
      HalLcdWriteScreen( (char*)pkt->cmd.Data, "rcvd" );
#elif defined( WIN32 )
      WPRINTSTR( pkt->cmd.Data );
#endif
      break;
  }
}

/*********************************************************************
 * @fn      SensorApp_SendTheMessage
 *
 * @brief   Send "the" message.
 *
 * @param   none
 *
 * @return  none
 */
static void SensorApp_SendTheMessage( void )
{
    unsigned char msg[7];
    DHT11();
    SensorData.NWK_ADDR = NLME_GetShortAddr();
    SensorData.room = Room;

    SensorData.sensorNum = 0x01;
    SensorData.sensorType = 0x01;
    SensorData.data = (uint16)ucharRH_data_H;

    msg[0] = SensorData.NWK_ADDR >> 8;
    msg[1] = SensorData.NWK_ADDR & 0x00ff;
    msg[2] = SensorData.room;
    msg[3] = SensorData.sensorNum;
    msg[4] = SensorData.sensorType;
    msg[5] = SensorData.data >> 8;
    msg[6] = SensorData.data & 0x00ff;

    if ( AF_DataRequest( &SensorApp_DstAddr, &SensorApp_epDesc,
                        SENSORAPP_CLUSTERID,
                        (byte)7,
                        (byte *)&msg,
                        &SensorApp_TransID,
                        AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
    {
        // Successfully requested to be sent.
    }
    else
    {
        // Error occurred in request to send.
    }

    SensorData.sensorNum = 0x02;
    SensorData.sensorType = 0x02;
    SensorData.data = (uint16)ucharT_data_H;
    msg[3] = SensorData.sensorNum;
    msg[4] = SensorData.sensorType;
    msg[5] = SensorData.data >> 8;
    msg[6] = SensorData.data & 0x00ff;

    if ( AF_DataRequest( &SensorApp_DstAddr, &SensorApp_epDesc,
                        SENSORAPP_CLUSTERID,
                        (byte)7,
                        (byte *)&msg,
                        &SensorApp_TransID,
                        AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
    {
        // Successfully requested to be sent.
    }
    else
    {
        // Error occurred in request to send.
    }
}

#if defined( IAR_ARMCM3_LM )
/*********************************************************************
 * @fn      SensorApp_ProcessRtosMessage
 *
 * @brief   Receive message from RTOS queue, send response back.
 *
 * @param   none
 *
 * @return  none
 */
static void SensorApp_ProcessRtosMessage( void )
{
  osalQueue_t inMsg;

  if ( osal_queue_receive( OsalQueue, &inMsg, 0 ) == pdPASS )
  {
    uint8 cmndId = inMsg.cmnd;
    uint32 counter = osal_build_uint32( inMsg.cbuf, 4 );

    switch ( cmndId )
    {
      case CMD_INCR:
        counter += 1;  /* Increment the incoming counter */
                       /* Intentionally fall through next case */

      case CMD_ECHO:
      {
        userQueue_t outMsg;

        outMsg.resp = RSP_CODE | cmndId;  /* Response ID */
        osal_buffer_uint32( outMsg.rbuf, counter );    /* Increment counter */
        osal_queue_send( UserQueue1, &outMsg, 0 );  /* Send back to UserTask */
        break;
      }
      
      default:
        break;  /* Ignore unknown command */    
    }
  }
}
#endif

/*********************************************************************
 */
