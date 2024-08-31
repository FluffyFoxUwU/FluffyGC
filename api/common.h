#ifndef UWU_FA033D7E_E7E3_4C9B_9416_7F5E53507E54_UWU
#define UWU_FA033D7E_E7E3_4C9B_9416_7F5E53507E54_UWU

#define API_INTERN(x) _Generic((x), \
  struct fluffygc_state*: (struct heap*) (x), \
  struct fluffygc_object*: (struct root_ref*) (x), \
  const struct fluffygc_descriptor*: (const struct descriptor*) (x) \
)

#define API_EXTERN(x) _Generic((x), \
  struct heap*: (struct fluffygc_state*) (x), \
  struct root_ref*: (struct fluffygc_object*) (x), \
  const struct descriptor*: (const struct fluffygc_descriptor*) (x) \
)

#endif
