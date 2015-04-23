#ifndef REMOTESCREEN_PROTOCOL_H_
#define REMOTESCREEN_PROTOCOL_H_

#include <pebble.h>
#include "Errors.h"

#define API_VERSION 0x0000 // Won't worry compatibility until 0x100. i.e. 1.0

typedef enum RemoteFuncs {
  FUNC_NO_FUNC = 0,
  FUNC_NEW_WINDOW ,
  FUNC_NEW_TEXT_LAYER,
  FUNC_APPLY_ATTRIBUTES,
  FUNC_PUSH_WINDOW,
  FUNC_REQUEST_CLICKS,
  FUNC_GET_DICTIONARY_BY_ID,
  FUNC_GET_TEXT_LAYER_BY_ID,
  FUNC_CLEAR_WINDOW,
  FUNC_RESET_WINDOWS,
} RemoteFuncs;

enum Keys {
  KEY_STATUS = 0,
  KEY_API_VERSION,
  KEY_ERROR_CODE,
  KEY_RETURN_VALUE,
  KEY_TRANSACTION_ID,
  KEY_WINDOW_ID,
  KEY_TEXT_LAYER_ID,
  KEY_METHOD_ID,
  KEY_ATTRIBUTE_FONT,
  KEY_ATTRIBUTE_BG_COLOR,
  KEY_ATTRIBUTE_FG_COLOR,
  KEY_ATTRIBUTE_TEXT,
  KEY_ATTRIBUTE_ALIGNMENT,
  KEY_ATTRIBUTE_RECT,
  KEY_CLICK,
  KEY_BUTTON_0,
  KEY_BUTTON_1,
  KEY_BUTTON_2,
  KEY_BUTTON_3,
  KEY_BUTTON_4,
  KEY_BUTTON_5,
  KEY_BUTTON_6,
  KEY_BUTTON_7, // reserve space for 8 buttons, although there are only 4 right now.
  KEY_ID,
};

#define ROOT_WINDOW_ID 1
#define ROOT_WINDOW_HANDLE 0

enum Status {
  STATUS_OK = 0,
  STATUS_ERR,
  STATUS_STARTED,
  STATUS_STOPPED,
};

#define CLICK_DATA(r, c, b) (((r)?1:0) << 16 | (c << 8) | (b) )
#define LONG_CLICK_DATA(t, r,c,b) ((t) | ((r)?1:0) << 16 | (c << 8) | (b) )
  
#define BUTTON_WANT_SINGLE_CLICK (1 << 0)
#define BUTTON_WANT_REPEATED_MASK (0xFFF << 8)
#define BUTTON_WANT_MULTI_MASK (0xF << 1)
#define BUTTON_LONG_CLICK_MASK (0xFFF << 20)
#define BUTTON_LONG_CLICK_DELAY(t) (((t) >> 20) & 0xFFF)
#define BUTTON_REPEAT_DELAY(t) (((t) >> 8) & 0xFFF)
#define BUTTON_MULTI_MAX(t) (((t) > 1) & 0xF)
#define LONG_CLICK_DOWN (1<<17)
#define LONG_CLICK_UP (1<<18)

DictionaryIterator *begin_message(int status, uint32_t tid);
void send_message(DictionaryIterator *iter);
  
#endif
