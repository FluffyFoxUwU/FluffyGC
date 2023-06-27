Direct Access Memory (``FH_MOD_DMA``)
#####################################

Allow users to directly access the underlying data storage
for efficient object access instead through reader and writer

Since
*****
Version 0.1

Flags for this mod
##################
+------------------------+--------+------------------------------------------------------------------------------------------+
| Flags                  | Value  | Description                                                                              |
+========================+========+==========================================================================================+
| FH_MOD_DMA_NONBLOCKING | 0x0001 | Enable nonblocking DMA                                                                    |
+------------------------+--------+------------------------------------------------------------------------------------------+
| FH_MOD_DMA_REFERENCES  | 0x0002 | Allow direct access to references                                                        |
+------------------------+--------+------------------------------------------------------------------------------------------+

Types
#####
.. code-block:: c

   #define FH_MOD_DMA_MAP_SAME_MEM 0x0001     // The DMA memory is shared with other mapping instance (e.g. to allow atomic used in them)
   #define FH_MOD_DMA_MAP_NO_SYNC  0x0002     // The pointer given will not be able to sync (essentially taking
                                              // taking snapshot and dont write back)
   #define FH_MOD_DMA_MAP_NONBLOCKING 0x0004  // Allow as long mapping exist it won't block GC (fails if FH_MOD_DMA_NONBLOCKING not
                                              // enabled at fh_enable time)

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
| void         | fh_object_sync_dma(fh_object* self, void* ptr)                                   | `fh_object_sync_dma`_  |
+--------------+----------------------------------------------------------------------------------+------------------------+

Function details
################

fh_object_map_dma
*****************
.. code-block:: c

   @Nullable
   void* fh_object_map_dma(fh_object* self, size_t offset, size_t size, unsigned int mapFlags)

Maps a part of the object so it can be DMA accessed for the 
whole program's address space. Pointer returned maybe unique 
i.e. consecutive calls return different pointers. Result of
writing into DMA region may not immediately visible. Alignment
for the pointer must follow what the descriptor says. Concurrent
writes to same object via DMA behaviour is undefined and that mean
concurrent usage of DMA pointer is undefine too as with concurrent
read and write (both reference access and data access).

Implementation may take snapshot and copy into special location which
maybe out of date from the backing memory ``fh_object_(write|read)_sync_dma``
is needed on both writer and reader

Atomic operations within the mapped region is undefined as pointer returned
may be different region (or even CoW protected) hence the atomic may
not work as intended unless

If ``FH_MOD_DMA_MAP_NO_SYNC`` and ``FH_MOD_DMA_MAP_SAME_MEM`` set, the pointer
given is points to the storage used by object and every writes and read
immediately appear to ``fh_object_(read|write)_(ref|data)`` without possibility
of stale reads due writes not immediately visible as with ``FH_MOD_DMA_MAP_SAME_MEM``.
If this used, ``FH_MOD_DMA_SYNC`` is not needed to ensure write immediately visible

If ``FH_MOD_DMA_MAP_SAME_MEM`` set the backing storage for every mapping
is logically the same but may return different pointer. Allows atomic operation
inside the DMA region to be valid and writes and read visible to other concurrent
context without unmap first and ``fh_object_sync_dma`` still needed
to ensure synchronization with the backing storage if DMA sync disabled
unmap is only guarantee way making it visible to ``fh_object_(read|write)_(ref|data)``
After last DMA pointer unmapped.

If ``FH_MOD_DMA_MAP_NO_SYNC`` set, the DMA pointer is read/write but wont be
able to synchronize (useful if the caller dont care whether its out of date or
not like write back)

If ``FH_MOD_DMA_MAP_NONBLOCKING`` set, as long the mapping exist it wont
block GC

.. tip::
   ``FH_MOD_DMA_MAP_NO_SYNC | FH_MOD_DMA_MAP_NONBLOCKING`` useful combination
    if program does large copy which doesnt matter whether its out of date or not

And last note the pointer must not be used in another context (any OS thread
is ok but must preserve the context) and also this atomically may create copy
if ``FH_MOD_DMA_MAP_NO_SYNC`` unset to support synchronization

.. tip::
   Implementations may return unique pointer for debug feature
   or may return same pointer for optimization or convenience
   due internal memory layout

.. note::
   GC maybe blocked until the pointer unmapped.
   Unless ``FH_MOD_DMA_NONBLOCKING`` flag is set and ``FH_MOD_DMA_MAP_NONBLOCKING`` given

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
If ``FH_MOD_DMA_SYNC`` enabled, implicit write synchronization
occur (which synchronizes writes back before unmapping) else
its only writes what changed

If ``FH_MOD_DMA_MAP_NO_SYNC`` set no write back occurs

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
