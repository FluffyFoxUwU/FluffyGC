#ifndef header_1656770545_7827f843_bd3f_4236_9834_00cadd2220c9_marker_h
#define header_1656770545_7827f843_bd3f_4236_9834_00cadd2220c9_marker_h

struct object_info;
struct region;
struct gc_state;

void gc_marker_mark(struct gc_state* self, struct region* onlyIn, struct object_info* objectInfo);

#endif

