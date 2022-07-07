#ifndef header_1656149251_api_h
#define header_1656149251_api_h

typedef struct {
  int (*GetVersionMajor)();
  int (*GetVersionMinor)();
  int (*GetVersionPatch)();
}* bettergc_api_t;

/*
static void bus() {
  bettergc_api_t* api;
  (*api)->GetVersionMajor();
}
*/

#endif

