#ifndef header_1657023238_6a9f5864_55b8_439f_8a48_c926481f020a_gc_enums_h
#define header_1657023238_6a9f5864_55b8_439f_8a48_c926481f020a_gc_enums_h

enum gc_request_type {
  GC_REQUEST_UNKNOWN,

  // Automaticly trigger old
  // then full if either cant
  // complete a cycle
  GC_REQUEST_COLLECT_YOUNG,

  GC_REQUEST_COLLECT_OLD,
  GC_REQUEST_COLLECT_FULL,
  GC_REQUEST_START_CONCURRENT,
  
  GC_REQUEST_SHUTDOWN
};

#endif

