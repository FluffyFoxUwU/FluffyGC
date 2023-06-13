Direct Access Memory (``FH_MOD_DMA``)
#####################################

Allow users to directly access the underlying data storage
for efficient object access instead through reader and writer

Since
*****
Version 0.1

Flags for this mod
##################
+------------------------+--------+-----------------------------------+
| Flags                  | Value  | Description                       |
+========================+========+===================================+
| FH_MOD_DMA_REFERENCES  | 0x0001 | Enable DMA access to references   |
+------------------------+--------+-----------------------------------+

Access
######
+-------------------------+--------+--------------+
| Bitfield name           | Number | Purpose      |
+=========================+========+==============+
| FH_MOD_DMA_ACCESS_READ  | 0x01   | Read access  |
+-------------------------+--------+--------------+
| FH_MOD_DMA_ACCESS_WRITE | 0x02   | Write access |
+-------------------------+--------+--------------+

Methods (Extends ``fh_object*``)
################################
+--------------+----------------------------------------------------------------------------------+------------------------+
| Return Value | Method                                                                           | Link                   |
+==============+==================================================================================+========================+
| void*        | fh_object_map_dma(fh_object* self, size_t alignment, size_t offset, size_t size) | `fh_object_map_dma`_   |
+--------------+----------------------------------------------------------------------------------+------------------------+
| void         | fh_object_unmap_dma(fh_object* self, void* ptr)                                  | `fh_object_unmap_dma`_ |
+--------------+----------------------------------------------------------------------------------+------------------------+

Function details
################

fh_object_map_dma
*****************
.. code-block:: c

   @Nullable
   void* fh_object_map_dma(fh_object* self, size_t offset, size_t size)

Maps a part of the object so it can be DMA accessed for the 
whole program's address space. Pointer returned is unique 
i.e. consecutive calls return different pointers. Result of
writing into DMA region may not immediately visible. Alignment
for the pointer must follow what the descriptor say if possible

.. tip::
   Implementations may return unique pointer for debug feature
   or may return same pointer for optimization or convenience
   due internal memory layout

.. note::
   GC maybe blocked until the pointer returned.
   Unless ``FH_MOD_DMA_NONBLOCKING`` flag is set

.. warning::
   This does **NOT** allow any access to references inside
   an object. So, read/write method still needed to do it.
   Unless ``FH_MOD_DMA_REFERENCES`` flag is set

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Object to get pointer from
  ``offset`` - Offset where the mapping start
  ``size`` - Size of region to be mapped

Return value
============
  Return untyped pointer which can be casted to corresponding
  typed data and used directly or NULL incase of error

Tags
====
GC-Safepoint May-Block-GC Need-Valid-Context

fh_object_unmap_dma
*****************
.. code-block:: c

   int fh_unmap_dma(fh_object* self, const void* dma)

Invalidate the ``dma`` pointer. ``dma`` pointer after
this call considered to be free'd and must not be reused.
Writes that happen before this will be stored into object

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
