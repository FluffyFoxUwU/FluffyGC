Direct Access Memory Synchronization (``FH_MOD_DMA_SYNC``)
#####################################

Adds one function to synchronize DMA 

Since
*****
Version 0.1

Methods (Extends ``fh_object*``)
################################
+--------------+----------------------------------------------------------------------------------+------------------------+
| Return Value | Method                                                                           | Link                   |
+==============+==================================================================================+========================+
| void         | fh_object_sync_dma(fh_object* self, void* ptr)                                   | `fh_object_sync_dma`_  |
+--------------+----------------------------------------------------------------------------------+------------------------+

Function details
################

fh_object_sync_dma
******************
.. code-block:: c

   void fh_object_sync_dma(fh_object* self, void* ptr);

Synchronizes the ``ptr`` and ``self``. Synchronization
may be slow (incase of emulation via byte by byte comparison)

Synchronization must happen as if byte by byte comparison
but implementation may choose faster method as long it
semanticly is same behaviour as byte by byte

This function may block if concurrent sync DMA happens but
may blocks even there nothing due false sharing with unrelated
DMA sync on nearby object

On collision like

+-----------------------------+-----------------------------+---------------------+-------------------------+
| Action                      | Backing Object              | Snapshot for DMA    | DMA Pointer             |
+=============================+=============================+=====================+=========================+
| Create mapping (atomically) | 0xB0 0x00 0x06 0x00         | 0xB0 0x00 0x00 0x00 | 0xB0 0x00 0x06 0x00     |
+-----------------------------+-----------------------------+---------------------+-------------------------+
| Concurrent writes           | 0xB0 **0x12** **0xB6** 0x00 | 0xB0 0x00 0x00 0x00 | 0xB0 0x00 **0xA5** 0x00 |
+-----------------------------+-----------------------------+---------------------+-------------------------+

The synchronization result is undefined

+---------------------------+-------------------------+-----------------------------+-----------------------------+
| Action                    | Backing Object          | Snapshot for DMA            | DMA Pointer                 |
+===========================+=========================+=============================+=============================+
| Synchronizes (atomically) | 0xB0 0x12 **0x??** 0x00 | 0xB0 **0x12** **0x??** 0x00 | 0xB0 **0x12** **0x??** 0x00 |
+---------------------------+-------------------------+-----------------------------+-----------------------------+

Bold marks writes

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Object where pointer initially came from
  ``dma`` - DMA pointer to be snychronized

Tags
====
GC-Safepoint Need-Valid-Context May-Blocks

