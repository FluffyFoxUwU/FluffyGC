Mods
####

Mods is a mechanism for optional features.

Enabled mod will apply on next heap creation and
cannot be disable on the heap after creation. The
mod states is thread local only. Functions symbols
associated with a mod should be resolved with the
dynamic lookup function if program does care if
the linked in version don't support it or define
``FH_EXPORT_MOD_SYMBOLS`` at compile-time to add
explicit declaration in mods header files.

The ``FH_EXPORT_MOD_WEAK_SYMBOLS`` is same as
``FH_EXPORT_MOD_SYMBOLS`` but uses ``__attribute__((weak))``
to defined weak symbols.

.. tip::
   Program shouldn't use ``FH_EXPORT_MOD_SYMBOLS`` nor ``FH_EXPORT_MOD_WEAK_SYMBOLS``
   if program care about what version its compiled or linked against

Example using DMA mod:

.. code-block:: c

   #include <FluffyHeap/FluffyHeap.h>
   
   #if FH_HAS_MOD(FH_MOD_DMA)
   // DMA mod headers are available
   // the program can use DMA mod
   // if available at runtime
   # include <FluffyHeap/mods/DMA.h>
   
   fh_object_get_dma_func fh_object_get_dma = NULL;
   fh_object_put_dma_func fh_object_put_dma = NULL;
   #else
   # define FH_MOD_DMA FH_MOD_UNKNOWN
   // Else, headers unavailable
   void* (*fh_object_get_dma)(fh_object*) = NULL;
   void (*fh_object_put_dma)(fh_object*,void*) = NULL;
   #endif
   
   ...
   // The linked library supports enable it
   if (fh_check_mod(FH_MOD_DMA) == true) {
     fh_enable_mod(FH_MOD_DMA, 0);
     
     // Program may check dlsym just to be sure but
     // it should be present on conforming library
     fh_object_get_dma = dlsym(RTLD_DEFAULT, "fh_object_get_dma");
     fh_object_put_dma = dlsym(RTLD_DEFAULT, "fh_object_put_dma");
   } else {
     // DMA not available with linked library or program not compiled
     // with DMA support use alternative method (either emulating
     // it with fh_obj_read_data and fh_obj_write_data)
   }
   
   // Create heap with enabled or disabled DMA mod
   ...

Or another example using ``FH_EXPORT_MOD_WEAK_SYMBOLS`` for usecase
where you have control over header version and don't want boilerplate
code to care about header version. 

.. code-block:: c

   #define FH_EXPORT_MOD_WEAK_SYMBOLS 1
   #include <FluffyHeap/FluffyHeap.h>
   
   #if FH_HAS_MOD(FH_MOD_DMA)
   // DMA mod headers are available
   // the program can use DMA mod
   // if available at runtime
   #include <FluffyHeap/mods/DMA.h>
   #else
   # error "Development headers doesn't have DMA mod"
   #endif
   
   ...
   if (fh_check_mod(FH_MOD_DMA) == true)
     fh_enable_mod(FH_MOD_DMA, 0);
   
   // Create heap with enabled or disabled DMA mod
   ...

Other types
***********
.. code-block:: c

   #define FH_MOD_WAS_ENABLED (1 << 31)

Since
*****
Version 0.1

Functions
#########
+--------------+------------------------------------------------+-------------------+
| Return Value | Function                                       | Link              |
+==============+================================================+===================+
| int          | fh_enable_mod(enum fh_mod mod, uint32_t flags) | `fh_enable_mod`_  |
+--------------+------------------------------------------------+-------------------+
| void         | fh_disable_mod(enum fh_mod mod)                | `fh_disable_mod`_ |
+--------------+------------------------------------------------+-------------------+
| bool         | fh_check_mod(enum fh_mod mod, uint32_t flags)  | `fh_check_mod`_   |
+--------------+------------------------------------------------+-------------------+
| uint32_t     | fh_get_flags(enum fh_mod mod)                  | `fh_get_flags`_   |
+--------------+------------------------------------------------+-------------------+

enum fh_mod
###########
Enum of defined standard mods. Range 0x0000 to 0x10000
reserved for standard the rest is implementation detail

+--------------------+--------+------------------------------------------------------------------------------------------------------------------+-----------------+
| Enum               | Value  | Description                                                                                                      | Link            |
+====================+========+==================================================================================================================+=================+
| ``FH_MOD_UNKNOWN`` | 0x0000 | Always fail on enable and check mod                                                                              |                 |
+--------------------+--------+------------------------------------------------------------------------------------------------------------------+-----------------+
| ``FH_MOD_DMA``     | 0x0001 | Direct Memory Access                                                                                             | :doc:`mods/dma` |
+--------------------+--------+------------------------------------------------------------------------------------------------------------------+-----------------+
| ``FH_MOD_DEBUG``   | 0x0002 | Enables more checks on various stuffs as many as fox could think of (implementation defined for what is checked) |                 |
+--------------------+--------+------------------------------------------------------------------------------------------------------------------+-----------------+

A corresponding macro function ``FH_HAS_MOD(mod)`` expands to 1 if available.

.. warning::
   If ``FH_HAS_MOD(mod)`` say it available doesn't mean the library which eventually
   linked will has the mod usable (check `fh_check_mod`_ if the linked library
   supports the mod)

Flags
*****
Topmost bit of the flags is reserved for ``fh_get_flags`` to indicate
the mod is loaded and other bits is valid. If top bit is unset all other
bits must unset so program can do to check without hassle of
remembering it somewhere. Also allow reduction of if statements
check into one bitwise AND operation and one comparison

.. code-block:: c

   if (fh_get_flags(FH_MOD_WHATEVER) & (FH_MOD_WAS_ENABLED | FH_WHATEVER_INTERESTING_FLAG)) {
     // The mod was enabled and the interesting flag was set
     // do something
   } else {
     // Either mod was disabled or the interesting flag was not set
     // do something else entirely
   }

Since
=====
Version 0.1

Function details
################

fh_enable_mod
*************
.. code-block:: c

   int fh_enable_mod(enum fh_mod mod, uint32_t flags)

Enable the corresponding ``mod``

Since
=====
Version 0.1

Parameters
==========
  ``mod`` - Corresponding mod to be enabled. (See `enum fh_mod`_)
  ``flags`` - Flags passed to the mod to enable/disable some mod
              features (See individual mod's documentation)

Return value
============
Zero indicate success
 * -EINVAL: Invalid flags and mod combination
 * -ENODEV: Mod not found
 * -EBUSY: Mod already enabled (``flags`` not checked in this scenario)

fh_disable_mod
**************
.. code-block:: c

   void fh_disable_mod(enum fh_mod mod)

Disable the corresponding ``mod`` to prevent being used on new heap creation

Since
=====
Version 0.1

Parameters
==========
  ``mod`` - Corresponding mod to be disabled. (See `enum fh_mod`_)

fh_check_mod
************
.. code-block:: c

   bool fh_check_mod(enum fh_mod mod, int flags)

Check if ``mod`` and ``flags`` combination available.

Since
=====
Version 0.1

Parameters
==========
  ``mod`` - Corresponding mod to be checked. (See `enum fh_mod`_)
  ``flags`` - Flags passed to the mod to be check for usability
              features (See individual mod's documentation)

Return value
============
True if a ``mod`` and ``flags`` combination is available and usable.

fh_get_flags
************
.. code-block:: c

   uint32_t fh_get_flags(enum fh_mod mod)

Gets flag which was set during fh_enable_mod call

Since
=====
Version 0.1

Parameters
==========
  ``mod`` - Corresponding mod to be checked. (See `enum fh_mod`_)

Return value
============
The requested flags (check top most bit to see the flags is valid)
