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
   int (*fh_descriptor_loader)(const char* name, @Nullable void* udata, @WriteOnly fh_descriptor_param* param)
   void (*fh_finalizer)(const void* objData)

A type for application descriptor loader. Must write to ``param``
so that the descriptor can be created. Context at entrance and 
exit must not change though changing context inside loader and
switch back is valid. Loader may be called again but must return
same data for a single ``name`` but may give different parameter
on different heap instance. Descriptor loader must not trigger
loader recursively! (it may define another descriptor but it must
not trigger another loader)

Finalizer called at somewhere after losing reference and before
reclaiming memory or may never be called for long time. But must
be called on heap destruction

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
     uint32_t flags;
     
     // Generation count with GC hint is not specified
     // use ``fh_get_generation_count`` to retrieve count
     int generationCount;
     size_t generationSizes[];
   } fh_param;

Heap flags
##########

+-------------------+--------+-------------------------------------------------------------------------------+
| Bitfield name     | Number | Purpose                                                                       |
+===================+========+===============================================================================+
| FH_HEAP_LOW_POWER | 0x01   | Hint that the algorithmn should be not power intensive while meeting GC hints |
+-------------------+--------+-------------------------------------------------------------------------------+
| FH_HEAP_LOW_LOAD  | 0x02   | Hint that the algorithmn should be not resource intensive                     |
+-------------------+--------+-------------------------------------------------------------------------------+

Functions
#########
+--------------+-------------------------------------------+----------------------------+
| Return value | Function name                             | Link                       |
+==============+===========================================+============================+
| int          | fh_get_generation_count(fh_param* param1) | `fh_get_generation_count`_ |
+--------------+-------------------------------------------+----------------------------+

Methods
#######
+--------------------------------+-------------------------------------------------------------------------+-----------------------------+
| Return value                   | Function description                                                    | Link                        |
+================================+=========================================================================+=============================+
| int                            | fh_attach_thread(fluffyheap* self)                                      | `fh_attach_thread`_         |
+--------------------------------+-------------------------------------------------------------------------+-----------------------------+
| void                           | fh_detach_thread(fluffyheap* self)                                      | `fh_detach_thread`_         |
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

   @ConstantResult
   int fh_get_generation_count(fh_param* param)

Get amount of generation required for specific param

Since
=====
Version 0.1

Parameters
==========
  ``param`` - Combination of parameter to check

Return value
============
Return number of generations. Must be same result for same parameter
Errors:
  ``-EINVAL`` - Invalid parameter (if this returned its 100% wont be accepted by `fh_new`_)

Method Details
##############

fh_attach_thread
****************
.. code-block:: c

   int fh_attach_thread(fluffyheap* self)

Attach thread to a heap. Initially recently
attached thread already has a context associated

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

   void fh_detach_thread(fluffyheap* self)

Detaches current thread from heap. After this call
all API calls is invalid unless noted by each function

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Heap state to detach from (Passed to ensure the thread
  detach from correct heap)

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

