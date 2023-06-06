``fluffyheap*``
######################

``fluffyheap*`` is the primary state where all
operations operated on

Since
*****
Version 0.1

Methods
#######
+--------------+------------------------------------+---------------------+
| Return value | Function description               | Link                |
+==============+====================================+=====================+
| int          | fh_attach_thread(fluffyheap* self) | `fh_attach_thread`_ |
+--------------+------------------------------------+---------------------+
| int          | fh_detach_thread(fluffyheap* self) | `fh_detach_thread`_ |
+--------------+------------------------------------+---------------------+

Constructor Detail
##################
.. code-block:: c

   @Nullable
   fluffyheap* fh_new(fh_param* param)

Since
=====
Version 0.1

Destructor Detail
#################
.. code-block:: c

   void fh_free(fluffyheap* self)

.. warning::

   Do not call this while heap in use

Since
=====
Version 0.1

Method Details
##############

fh_attach_thread
****************
.. code-block:: c

   int fh_attach_thread(fluffyheap* self)

Attach thread to a heap. Initially recently
attached thread has no context associated
therefore not ready for usage yet

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Heap state to attach to

Return Value
============
  0 on success
  * -ENOMEM: Memory unavailable to attach current thread


Safety
******
Callable from any unattached thread globally
which mean this usage is invalid.

.. code-block:: c

   // Thread A
   fluffyheap* heap1 = fh_new(&param1);
   fluffyheap* heap2 = fh_new(&param2);
   
   fh_attach_thread(heap1);
   fh_attach_thread(heap2);

fh_detach_thread
****************
.. code-block:: c

   int fh_detach_thread(fluffyheap* self)

Detaches current thread from heap. After this call
all API calls is invalid unless noted by each function

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Heap state to detach from (Passed to ensure the thread
  detach from correct heap)

Return value
============
  0 on success
  * -EBUSY: There is active context within current thread (See :doc:`fh_context_set_current <context#fh_context_set>`)


