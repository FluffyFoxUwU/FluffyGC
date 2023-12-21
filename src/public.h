#ifndef header_1702952869_b2e75068_78b8_489f_9aad_830b783d1f14_export_h
#define header_1702952869_b2e75068_78b8_489f_9aad_830b783d1f14_export_h

#ifndef PUBLIC
# ifdef __GNUC__
#   define PUBLIC __attribute__((visibility("default"))) extern
# else
#   define PUBLIC extern
# endif
#endif

#endif

