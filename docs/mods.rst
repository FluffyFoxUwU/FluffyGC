Mods
####

Mods is a mechanism for optional features.

Enabled mod will apply on next heap creation and
cannot be disable on the heap after creation. The
mod states is thread local only

Since
*****
Version 0.1

Functions
#########
+--------------+---------------------------------------------------+-------------------+
| Return Value | Function                                          | Link              |
+==============+===================================================+===================+
| int          | fh_enable_mod(enum fh_mod mod, unsigned int flags) | `fh_enable_mod`_  |
+--------------+---------------------------------------------------+-------------------+
| void         | fh_disable_mod(enum fh_mod mod)                   | `fh_disable_mod`_ |
+--------------+---------------------------------------------------+-------------------+
| bool         | fh_check_mod(enum fh_mod mod, unsigned int flags)  | `fh_check_mod`_   |
+--------------+---------------------------------------------------+-------------------+

enum fh_mod
###########
Enum of defined standard mods. Range 0x0000 to 0x10000
reserved for standard the rest is implementation detail

+--------------------+--------+-----------------------------+--------------------+
| Enum               | Value  | Description                 | Link               |
+====================+========+=============================+====================+
| ``FH_MOD_UNKNOWN`` | 0x0000 | Placeholder for unknown mod |                    |
+--------------------+--------+-----------------------------+--------------------+
| ``FH_MOD_DMA``     | 0x0001 | Direct Memory Access        | :doc:`mods/dma`    |
+--------------------+--------+-----------------------------+--------------------+
| ``FH_MOD_ROBUST``  | 0x0002 | Robust checking             | :doc:`mods/robust` |
+--------------------+--------+-----------------------------+--------------------+

Since
=====
Version 0.1

Function details
################

fh_enable_mod
*************
.. code-block:: c

   int fh_enable_mod(enum fh_mod mod, int flags)

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
 * -ENOSYS: Mod not found
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

