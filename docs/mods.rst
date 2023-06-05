Mods
####

Mods is a mechanism for optional features. Some
mod can't be disable upon enabled so
`fh_disable_mod`_ return -EBUSY on such situation

Functions
#########
+--------------+-------------------------------------------+-------------------+
| Return Value | Function                                  | Link              |
+==============+===========================================+===================+
| int          | fh_enable_mod(enum fh_mod mod, int flags) | `fh_enable_mod`_  |
+--------------+-------------------------------------------+-------------------+
| bool         | fh_check_mod(enum fh_mod mod, int flags)  | `fh_check_mod`_   |
+--------------+-------------------------------------------+-------------------+

enum fh_mod
###########
Enum of defined standard mods. Range 0x0000 to 0x10000
reserved for standard the rest is implementation detail

+--------------------+--------+-----------------------------+-----------------+
| Enum               | Value  | Description                 | Link            |
+====================+========+=============================+=================+
| ``FH_MOD_UNKNOWN`` | 0x0000 | Placeholder for unknown mod |                 |
+--------------------+--------+-----------------------------+-----------------+
| ``FH_MOD_DMA``     | 0x0001 | Direct Memory Access        | :doc:`mods/dma` |
+--------------------+--------+-----------------------------+-----------------+
|                    |        |                             |                 |
+--------------------+--------+-----------------------------+-----------------+


Function details
################

fh_enable_mod
*************
.. code-block:: c

   int fh_enable_mod(enum fh_mod mod, int flags)

Enable the corresponding ``mod``

Parameters
==========
  ``mod`` - Corresponding mod to be enabled. (See `enum fh_mod`_)
  ``flags`` - Flags passed to the mod to enable/disable some mod features

