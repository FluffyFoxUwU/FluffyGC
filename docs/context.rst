Context
#######

Logical seperation of different stuff to allow usage of
of ``setcontext`` family of function to implement coroutines
without burden of carefully making sure GC root references
not being leaked.

Since
*****
Version 0.1

Functions
#########

+-----------------------+-----------------------------------------------+-------------------+
| Return value          | Function                                      | Link              |
+=======================+===============================================+===================+
| int                   | fh_set_current(@Nullable fh_context* context) | `fh_set_current`_ |
+-----------------------+-----------------------------------------------+-------------------+
| @Nullable fh_context* | fh_get_current()                              | `fh_get_current`_ |
+-----------------------+-----------------------------------------------+-------------------+

Method
######
+--------------+---------------------------------------+------------------------+
| Return value | Method Name                           | Link                   |
+==============+=======================================+========================+
| fluffyheap*  | fh_context_get_heap(fh_context* self) | `fh_context_get_heap`_ |
+--------------+---------------------------------------+------------------------+

Constructor Detail
##################
.. code-block:: c

   @Nullable
   fh_context* fh_context_new(fh_heap* heap)

Since
*****
Version 0.1

Parameter
*********
  ``heap`` - Heap which context associated with

Return value
************
Returns nonnull ``fh_context*`` ready for use

Destructor Detail
#################
.. code-block:: c

   void fh_context_free(fh_heap* heap, fh_context* self);

Since
*****
Version 0.1

Parameter
*********
  ``heap`` - Heap which context associated with
  ``context`` - The context


Functions Details
#################

fh_context_set
**************
.. code-block:: c

   int fh_set_current(@Nullable fh_context* context)

This changes current thread's context to ``context`` and
mark ``context`` as active and mark old ``context`` inactive

.. note::
   It is not OS's context but logical context where GC roots
   and alike lives

Since
=====
Version 0.1

Parameters
==========
  ``context`` - Context to set into

Return Value
============
0 on sucess
 * -EBUSY: The ``context`` is active and in use


fh_context_get
**************
.. code-block:: c

   @Nullable
   fh_context* fh_get_current()

Since
=====
Version 0.1

Return Value
============
Return currently set context

Method details
##############

fh_context_get_heap
*******************
.. code-block:: c

   fluffyheap* fh_context_get_heap(fh_context* self)

Get heap associated with ``self``

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Context to retrieve heap from

Return value
============
The heap
