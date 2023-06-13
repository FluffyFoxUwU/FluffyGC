``fluffyheap*``
######################

``fluffyheap*`` is the primary state where all
operations operated on

Since
*****
Version 0.1

Types
#####
.. code-block:: c

   @Nullable
   fh_descriptor* (*fh_descriptor_loader)(const char* name, void* udata)

A type for application descriptor loader. Returned descriptor must
be already acquired. Context at entrance and exit must not change
although changing context inside loader and switch back is valid

.. code-block:: c

   // Hints for the implementation to select which
   // implementation specific algorithmn or be 
   // ignored (for one size fit all algorithmn)
   enum fh_gc_hint {
     FH_GC_BALANCED,        // App not preferring any type
     FH_GC_LOW_LATENCY,     // App prefer lower latency GC (e.g. interactive apps)
     FH_GC_HIGH_THROUGHPUT  // App prefer higher throughput GC (e.g. server workloads)
   };

   typedef struct {
     enum fh_gc_hint hint;
     
     // Generation count with GC hint is not specified
     // use ``fh_get_generation_count`` to retrieve count
     int generationCount;
     size_t generationSizes[];
   } fh_param;

Functions
#########
+--------------+-----------------------------------------------+----------------------------+
| Return value | Function name                                 | Link                       |
+==============+===============================================+============================+
| int          | fh_get_generation_count(enum fh_gc_hint hint) | `fh_get_generation_count`_ |
+--------------+-----------------------------------------------+----------------------------+

Methods
#######
+--------------------------------+-------------------------------------------------------------------------+-----------------------------+
| Return value                   | Function description                                                    | Link                        |
+================================+=========================================================================+=============================+
| int                            | fh_attach_thread(fluffyheap* self)                                      | `fh_attach_thread`_         |
+--------------------------------+-------------------------------------------------------------------------+-----------------------------+
| int                            | fh_detach_thread(fluffyheap* self)                                      | `fh_detach_thread`_         |
+--------------------------------+-------------------------------------------------------------------------+-----------------------------+
| void                           | fh_set_descriptor_loader(fluffyheap* self, fh_descriptor_loader loader) | `fh_set_descriptor_loader`_ |
+--------------------------------+-------------------------------------------------------------------------+-----------------------------+
| @Nullable fh_descriptor_loader | fh_get_descriptor_loader(fluffyheap* self)                              | `fh_get_descriptor_loader`_ |
+--------------------------------+-------------------------------------------------------------------------+-----------------------------+

Constructor Detail
##################
.. code-block:: c

   @Nullable
   fluffyheap* fh_new(fh_param* param)

There may be another implementation specific way
to create ``fluffyheap*`` if extra parameters not
exist in this API needed

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

Function details
################

fh_get_generation_count
***********************
.. code-block:: c

   @Nonzero
   @Positive
   @ConstantResult
   int fh_get_generation_count(enum fh_gc_hint hint)

Get amount of generation required for specific hint
and must not fail.

Since
=====
Version 0.1

Parameters
==========
  ``hint`` - Hint for which the generation count be retrieved from

Return value
============
Return number of generations. Constant result for same parameter

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

fh_set_descriptor_loader
************************
.. code-block:: c 

   void fh_set_descriptor_loader(fluffyheap* self, @Nullable fh_descriptor_loader loader);

Set descriptor loader. This blocks caller until there no other thread which still doing
descriptor loading and this must not be called inside loader function.

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Heap state to set loader on
  ``loader`` - The new loader

fh_get_descriptor_loader
************************
.. code-block:: c

   @Nullable
   fh_descriptor_loader fh_get_descriptor_loader(fluffyheap* self);

Get the descriptor loader

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Heap state to get loader from

Return value
============
The loader 

