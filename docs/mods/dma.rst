Direct Access Memory (``FH_MOD_DMA``)
#####################################

Allow users to directly access the underlying data storage
for efficient object access instead through reader and writer

Since
*****
Version 0.1

Flags for this mod
##################
+------------------------+--------+------------------------------------------------------------------+
| Flags                  | Value  | Description                                                      |
+========================+========+==================================================================+
| FH_MOD_DMA_NONBLOCKING | 0x0001 | Enable nonblocking DMA (Needed or nonblocking mapping will fail) |
+------------------------+--------+------------------------------------------------------------------+
| FH_MOD_DMA_ATOMIC      | 0x0002 | Enable atomic operation on DMA mapping                           |
+------------------------+--------+------------------------------------------------------------------+
| FH_MOD_DMA_REFERENCES  | 0x0004 | Allow access to references                                       |
+------------------------+--------+------------------------------------------------------------------+

Types
#####
.. code-block:: c

   #define FH_MOD_DMA_MAP_ATOMIC        0x0001     // The DMA memory is atomic-capable for between C11 atomic operation between
                                                   // OS threads which same object via mapping without this atomic operation
                                                   // is undefined
   #define FH_MOD_DMA_MAP_NONBLOCKING   0x0002     // Allow as long mapping exist it won't block GC (fails if FH_MOD_DMA_NONBLOCKING not
                                                   // enabled at fh_enable time)
   #define FH_MOD_DMA_NO_COPY           0x0004     // The data must not copied
   
   // This must not be initialized or created by program
   // and must be statisfy `alignof(fh_dma_ptr) >= sizeof(void*)`
   struct fh_dma_ptr {
     void* ptr;
     
     // Implementation private data
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
+--------------+-----------------------------------------------------------------------------------------------------------+------------------------+
| Return Value | Method                                                                                                    | Link                   |
+==============+===========================================================================================================+========================+
| fh_dma_ptr*  | fh_object_map_dma(fh_object* self, size_t offset, size_t size, unsigned int mapFlags, unsigned int usage) | `fh_object_map_dma`_   |
+--------------+-----------------------------------------------------------------------------------------------------------+------------------------+
| void         | fh_object_unmap_dma(fh_object* self, fh_dma_ptr* dma)                                                     | `fh_object_unmap_dma`_ |
+--------------+-----------------------------------------------------------------------------------------------------------+------------------------+
| void         | fh_object_sync_dma(fh_object* self, fh_dma_ptr* dma)                                                      | `fh_object_sync_dma`_  |
+--------------+-----------------------------------------------------------------------------------------------------------+------------------------+

Function details
################

fh_object_map_dma
*****************
.. code-block:: c

   @Nullable
   fh_dma_ptr* fh_object_map_dma(fh_object* self, size_t offset, size_t size, unsigned int mapFlags, unsigned int usage)

Maps a part of the object so it can be DMA accessed for the 
whole program's address space. Pointer returned may be unique 
i.e. consecutive calls return different pointers. Result of
writing into DMA region may not immediately visible. Alignment
for the pointer must follow what the descriptor says. Concurrent
writes to same object via DMA behaviour is undefined and that mean
concurrent usage of DMA pointer is undefined too as with concurrent
read and write (both reference access and data access).

The data in DMA eventually synchronized back

Usage is OR'ed depends what the mapping going to be used for any
usage outside of the flags its regarded is undefined

.. tip::
   If this whole thing confusing. Try think the pointer returned
   is pointer to imaginary intermediary buffer which your writes happen
   and eventually sync-ed with backing storage and good luck :3
   
   Or just thought of it as read/write cache (where it might get
   unsynchronized from main memory and maybe the data within mapping 
   asynchronously resync-ed) and need explicit syncing to guarantee
   its available to other thread

.. tip::
   The pointer given may be used by external library or anything like
   ordinary pointer as long as proper synchronization measures taken
   properly (e.g. properly sync back if expect the data must be written
   back to backing storage)

.. note::
   Implementation may take snapshot and copy into special location which
   may be out of date from the backing memory ``fh_object_sync_dma``
   is needed on both writer and reader to pass data between OS' threads but
   write by one thread and then re-read back by same thread always gives
   same data unless there other thread which writes to same location  and
   read on other thread is undefined

Atomic operations within the mapped region is undefined as pointer returned
may be different region (or even CoW protected) hence the atomic may
not work as intended unless ``FH_MOD_DMA_MAP_ATOMIC`` given

If ``FH_MOD_DMA_MAP_NO_COPY`` set, the pointer given is points to the storage used by object and 
every writes and reads immediately appear to ``fh_object_(read|write)_(ref|data)`` without possibility
of stale reads due writes not immediately visible as with ``FH_MOD_DMA_MAP_ATOMIC``. If this used, ``FH_MOD_DMA_SYNC`` 
is not needed to ensure write immediately visible

If ``FH_MOD_DMA_MAP_ATOMIC`` set the backing storage for every mapping
is logically the same but may return different pointer. Allows atomic operation
inside the DMA region to be valid and writes and read visible to other concurrent
context without unmap first and ``fh_object_sync_dma`` still needed to ensure synchronization
with the backing storage if DMA sync disabled unmap is only guarantee way making it visible
to ``fh_object_(read|write)_(ref|data)``

.. tip::
   ``FH_MOD_DMA_MAP_NO_COPY | FH_MOD_DMA_MAP_NONBLOCKING`` useful combination if
   program does large copy and dont want to block GC

And last note the pointer must not be used in another context (any OS thread
is ok but must preserve the context)
.. tip::
   Implementations may return unique pointer for debug feature
   or may return same pointer for optimization or convenience
   due internal memory layout

.. note::
   GC may be blocked until the pointer unmapped.
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
After the call the data written must be flushed back to
main memory

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

Synchronizes the ``ptr`` and ``self``. The operation must
be appear as an atomic operation in respect to other read
and writes to the mapping

Synchronization must happen as if byte by byte comparison

.. tip::
   implementation may choose faster method as long it
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
