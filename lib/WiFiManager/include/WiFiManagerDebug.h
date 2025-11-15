/**
 * WiFiManagerDebug.h
 * 
 * Debug output macros and templates for WiFiManager
 * 
 * @author tablatronix
 * @author Alex Hope-O'Connor
 * @license MIT
 */

#ifndef WiFiManagerDebug_h
#define WiFiManagerDebug_h

#if defined(ESP8266) || defined(ESP32)

// Template implementations for DEBUG_WM - must be in header for template instantiation
// This file is included at the end of WiFiManager.h after the class definition
template <typename Generic>
void WiFiManager::DEBUG_WM(Generic text) {
  DEBUG_WM(WM_DEBUG_NOTIFY,text,"");
}

template <typename Generic>
void WiFiManager::DEBUG_WM(wm_debuglevel_t level,Generic text) {
  if(_debugLevel >= level) DEBUG_WM(level,text,"");
}

template <typename Generic, typename Genericb>
void WiFiManager::DEBUG_WM(Generic text,Genericb textb) {
  DEBUG_WM(WM_DEBUG_NOTIFY,text,textb);
}

template <typename Generic, typename Genericb>
void WiFiManager::DEBUG_WM(wm_debuglevel_t level,Generic text,Genericb textb) {
  if(!_debug || _debugLevel < level) return;

  if(_debugLevel >= WM_DEBUG_MAX){
    #ifdef ESP8266
    // uint32_t free;
    // uint16_t max;
    // uint8_t frag;
    // ESP.getHeapStats(&free, &max, &frag);// @todo Does not exist in 2.3.0
    // _debugPort.printf("[MEM] free: %5d | max: %5d | frag: %3d%% \n", free, max, frag); 
    #elif defined ESP32
    // total_free_bytes;      ///<  Total free bytes in the heap. Equivalent to multi_free_heap_size().
    // total_allocated_bytes; ///<  Total bytes allocated to data in the heap.
    // largest_free_block;    ///<  Size of largest free block in the heap. This is the largest malloc-able size.
    // minimum_free_bytes;    ///<  Lifetime minimum free heap size. Equivalent to multi_minimum_free_heap_size().
    // allocated_blocks;      ///<  Number of (variable size) blocks allocated in the heap.
    // free_blocks;           ///<  Number of (variable size) free blocks in the heap.
    // total_blocks;           ///<  Total number of (variable size) blocks in the heap.
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    uint32_t free = info.total_free_bytes;
    uint16_t max  = info.largest_free_block;
    uint8_t frag = 100 - (max * 100) / free;
    _debugPort.printf("[MEM] free: %5u | max: %5u | frag: %3u%% \n", free, max, frag);
    #endif
  }

  _debugPort.print(_debugPrefix);
  if(_debugLevel >= debugLvlShow) _debugPort.print("["+(String)level+"] ");
  _debugPort.print(text);
  if(textb){
    _debugPort.print(" ");
    _debugPort.print(textb);
  }
  _debugPort.println();
}

#endif // defined(ESP8266) || defined(ESP32)

#endif // WiFiManagerDebug_h

