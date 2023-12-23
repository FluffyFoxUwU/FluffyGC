Direct Access Memory (``FH_MOD_DMA``)
#####################################

Allow users to directly access the underlying data storage
for efficient object access instead through reader and writer.
Access to GC refereneces sstill not allowed

Since
*****
Version 0.1

Types
#####
.. code-block:: c
 
   // This must not be initialized or created by program
   struct fh_dma_ptr {
     void* ptr;
     
     // Maybe a implementation private data
   };

Access
######
+-------------------------+--------+--------------+
| Bitfield name           | Number | Purpose      |
+=========================+========+==============+
| FH_MOD_DMA_ACCESS_READ  | 0x01   | Read access  |
+-------------------------+--------+--------------+
| FH_MOD_DMA_ACCESS_WRITE | 0x02   | Write access |
+-------------------------+--------+--------------+
| FH_MOD_DMA_ACCESS_RW    | 0x03   | R/W access   |
+-------------------------+--------+--------------+

Methods (Extends ``fh_object*``)
################################
+--------------+-------------------------------------------------------------------------------------------------------------+------------------------+
| Return Value | Method                                                                                                      | Link                   |
+==============+=============================================================================================================+========================+
| fh_dma_ptr*  | fh_object_map_dma(fh_object* self, size_t offset, size_t size, unsigned long mapFlags, unsigned long usage) | `fh_object_map_dma`_   |
+--------------+-------------------------------------------------------------------------------------------------------------+------------------------+
| void         | fh_object_unmap_dma(fh_object* self, fh_dma_ptr* dma)                                                       | `fh_object_unmap_dma`_ |
+--------------+-------------------------------------------------------------------------------------------------------------+------------------------+
| void         | fh_object_sync_dma(fh_object* self, fh_dma_ptr* dma)                                                        | `fh_object_sync_dma`_  |
+--------------+-------------------------------------------------------------------------------------------------------------+------------------------+

Function details
################

fh_object_map_dma
*****************
.. code-block:: c
 
   @Nullable
   fh_dma_ptr* fh_object_map_dma(fh_object* self, size_t offset, size_t size, unsigned long mapFlags, unsigned long usage)

Returns ``fh_dma_ptr*`` offseted by ``offset`` sized ``size`` to the backing
storage. Data races still exists even if only mapped read-only.

Pointer returned can be only be used in current context.

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Object to get pointer from
  ``offset`` - Offset where the mapping start
  ``size`` - Size of region to be mapped
  ``mapFlags`` - Currently no flags defined, pass 0
  ``usage`` - Bit flags of how the mapping will be used

Return value
============
  Return ``fh_dma_ptr`` pointer which can be be used

Tags
====
GC-Safepoint May-Block-GC Need-Valid-Context

fh_object_unmap_dma
*****************
.. code-block:: c

   int fh_unmap_dma(fh_object* self, fh_dma_ptr* dma)

Invalidate the ``dma`` pointer. ``dma`` pointer after
this call considered to be free'd and must not be reused.
After the call the changes must be available

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Object where pointer initially came from
  ``dma`` - DMA pointer to be invalidated

Return Value
============
Zero indicate success
 * -EINVAL: Invalid ``dma`` for ``self``

Tags
====
GC-Safepoint May-Unblock-GC Need-Valid-Context

fh_object_sync_dma
******************
.. code-block:: c

   void fh_object_sync_dma(fh_object* self, fh_dma_ptr* dma);

Acts as barrier where writes performed by current context before
this call available to other and other writes by other threads
visible to current context.

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



