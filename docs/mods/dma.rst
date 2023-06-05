Direct Access Memory (``FH_MOD_DMA``)
#####################################

Allow users to directly access the underlying data storage
for efficient object access instead through reader and writer

Flags for this mod
##################
+------------------------+--------+-----------------------------------+
| Flags                  | Value  | Description                       |
+========================+========+===================================+
| FH_MOD_DMA_REFERENCES  | 0x0001 | Enable DMA access to references   |
+------------------------+--------+-----------------------------------+
| FH_MOD_DMA_NONBLOCKING | 0x0002 | Enable non GC blocking DMA access |
+------------------------+--------+-----------------------------------+

Methods (Extends `fh_object*`)
##############################
+--------------+-----------------------------------------------+----------------------+
| Return Value | Method                                        | Link                 |
+==============+===============================================+======================+
| void*        | fh_object_get_dma(fh_object* self)            | `fh_object_get_dma`_ |
+--------------+-----------------------------------------------+----------------------+
| void         | fh_object_put_dma(fh_object* self, void* ptr) | `fh_object_put_dma`_ |
+--------------+-----------------------------------------------+----------------------+

Function details
################

fh_object_get_dma
*****************
.. code-block:: c

   void* fh_object_get_dma(fh_object* self)

Gets direct access pointer to the object which is accessible
for the whole program's address space.

.. note::
   GC is blocked until the pointer returned.
   Unless ``FH_MOD_DMA_NONBLOCKING`` flag is set

.. warning::
   This does **NOT** allow any access to references inside
   an object. So, read/write method still needed to do it.
   Unless ``FH_MOD_DMA_REFERENCES`` flag is set

Parameters
==========
  ``self`` - Object to get pointer from

Return value
============
  Return untyped pointer which can be casted to corresponding
  typed data and used directly


fh_object_put_dma
*****************
.. code-block:: c

   void fh_put_dma(fh_object* self, const void* dma)

Invalidate the ``dma`` pointer.

Parameters
==========
  ``self`` - Object where pointer initially came from
  ``dma`` - DMA pointer to be invalidated

Thread Safety
=============
Thread unsafe if called by multiple thread on same ``self`` or same ``dma``
