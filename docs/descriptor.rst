``fh_descriptor*``
##################

Descriptor about given object

The definition is

.. code-block:: c

   enum fh_object_type {
     FH_TYPE_NORMAL, /* Just ordinary object */
     FH_TYPE_ARRAY,  /* Array type */
   };
   
   enum fh_reference_strength {
     FH_REF_STRONG
   };
   
   typedef struct {
     const char* name;
     size_t offset;
     const char* dataType;
     enum fh_reference_strength strength;
   } fh_descriptor_field;
   
   typedef struct {
     enum fh_object_type type;
     size_t size;
     size_t alignment;
     
     // For arrays this only 1 long
     fh_descriptor_field fields*;
   } fh_descriptor_param;

For convenience following macro must present
.. code-block:: c

   #define FH_FIELD(type, member, _dataType, refStrength) \
    { \
      .name = #member, \
      .offset = offsetof(type, member), \
      .dataType = (_dataType), \
      .strength = (refStrength) \
    }
   #define FH_FIELD_END() {.name = NULL}

Functions
#########

+--------------------------+------------------------------------------------------------------------+--------------------------+
| Return value             | Function name                                                          | Link                     |
+==========================+========================================================================+==========================+
| @Nullable fh_descriptor* | fh_define_descriptor(const char* name, fh_descriptor_param* parameter) | `fh_define_descriptor`_  |
+--------------------------+------------------------------------------------------------------------+--------------------------+
| @Nullable fh_descriptor* | fh_get_descriptor(const char* name, bool dontInvokeLoader)             | `fh_get_descriptor`_     |
+--------------------------+------------------------------------------------------------------------+--------------------------+
| void                     | fh_release_descriptor(@Nullable fh_descriptor* desc)                             | `fh_release_descriptor`_ |
+--------------------------+------------------------------------------------------------------------+--------------------------+

``fh_define_descriptor`` and ``fh_get_descriptor`` only valid for object
descriptor not array as array differ.

Creation of array using descriptors is illegal please use
appropriate array constructors.

Methods
#######

+--------------------------------+----------------------------------------------+----------------------------+
| Return value                   | Method name                                  | Link                       |
+================================+==============================================+============================+
| @ReadOnly fh_descriptor_param* | fh_descriptor_get_param(fh_descriptor* self) | `fh_descriptor_get_param`_ |
+--------------------------------+----------------------------------------------+----------------------------+

Function details
################

fh_define_descriptor
********************
.. code-block:: c

   @Nullable
   fh_descriptor* fh_define_descriptor(const char* name, fh_descriptor_param* parameter)

Define a descriptor named "name" and acquire it (to prevent being GC-ed). Must be
able handle circular references

Since
=====
Version 0.1

Parameters
==========
  ``name`` - Name for the descriptor (follows Java convention like ``lua.lang.Table`` for example)
  ``parameter`` - Other parameters describing the layout and requirements

Return
======
The descriptor or NULL if failed or duplicate one exist

Tags
=====
GC-Safepoint GC-May-Invoke Need-Valid-Context

fh_define_descriptor
********************
.. code-block:: c

   @Nullable
   fh_descriptor* fh_get_descriptor(const char* name, bool dontInvokeLoader)

Get a descriptor named "name" or call application
defined hook to load if not present and acquire it
(to prevent being GC-ed). Calling application hook
can recurse forever and its valid so application
must ensure there no recursing

Since
=====
Version 0.1

Parameters
==========
  ``name`` - Name for the descriptor (follows Java convention like ``lua.lang.Table`` for example)
  ``dontInvokeLoader`` - Whether to invoke or not invoke app's loader possibly for avoiding recursion

Return
======
The descriptor

Tags
=====
GC-Safepoint GC-May-Invoke Need-Valid-Context May-Block

fh_release_descriptor
*********************
.. code-block:: c

   void fh_release_descriptor(@Nullable fh_descriptor* self)

Release the descriptor so it can be GC-ed. After this
call usage of ``self`` considering undefined beahaviour.
or do nothing if ``self`` is NULL. 

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Descriptor to release

Tags
=====
GC-Safepoint Need-Valid-Context

Method details
##############

fh_descriptor_get_param
***********************
.. code-block:: c

   const fh_descriptor_param* fh_descriptor_get_param(fh_descriptor* self)

Gets read only parameter for the ``self`` descriptor

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Descriptor to retrieve parameter from

Return value
============
The requested parameters read only

Tags
=====
GC-Safepoint Need-Valid-Context
