#ifndef header_1659181300_fe366ec9_de66_4a69_bd4e_3589227c072c_builder_h
#define header_1659181300_fe366ec9_de66_4a69_bd4e_3589227c072c_builder_h

// Useful macros assisting builder patterns

#define BUILDER_SETTER(builderInstance, type, name, funcName) \
  .funcName = ^typeof(builderInstance)* (type name) { \
                builderInstance.data.name = name; \
                return &builderInstance; \
              }
#define BUILDER_DECLARE(builderType, type, name) \
  builderType (^name)(type)

#endif

