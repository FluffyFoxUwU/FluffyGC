``fh_object*``
##############

The type which holds reference and allow access to the object

.. warning::
   As with pretty much any concurrent write/read in this globe, concurrent 
   access with ``fh_object_(read|write)_(ref|data)`` is undefined

Since
*****
Version 0.1

Types
#####

.. code-block:: c

   typedef enum {
     FH_TYPE_NORMAL, /* Just ordinary object */
     FH_TYPE_ARRAY,  /* Array type */
   } fh_object_type;
   
   typedef struct {
     size_t length;
     fh_descriptor* elementType;
   } fh_ref_array_info;
   
   typedef struct {
     fh_object_type type;
     union {
       fh_descriptor* normal;
       fh_ref_array_info* refArray;
     } info;
   } fh_type_info;

Methods
#######

+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| Return value         | Method name                                                                           | Link                                      |
+======================+=======================================================================================+===========================================+
| void                 | fh_object_read_data(fh_object* self, void* buffer, size_t offset, size_t size)        | `fh_object_read_data`_                    |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| void                 | fh_object_write_data(fh_object* self, const void* buffer, size_t offset, size_t size) | `fh_object_write_data`_                   |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| @Nullable fh_object* | fh_object_read_ref(fh_object* self, size_t offset)                                    | `fh_object_read_ref`_                     |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| void                 | fh_object_write_ref(fh_object* self, size_t offset, @Nullable fh_object* data)        | `fh_object_write_ref`_                    |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| int                  | fh_object_init_synchronization_structs(fh_object* self)                               | `fh_object_init_synchronization_structs`_ |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| void                 | fh_object_wait(fh_object* self, @Nullable const struct timespec* timeout)             | `fh_object_wait`_                         |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| void                 | fh_object_wake(fh_object* self)                                                       | `fh_object_wake-and-fh_object_wake_all`_  |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| void                 | fh_object_wake_all(fh_object* self)                                                   | `fh_object_wake-and-fh_object_wake_all`_  |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| void                 | fh_object_lock(fh_object* self)                                                       | `fh_object_lock-and-fh_object_unlock`_    |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| void                 | fh_object_unlock(fh_object* self)                                                     | `fh_object_lock-and-fh_object_unlock`_    |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| const fh_type_info*  | fh_object_get_type_info(fh_object* self)                                              | `fh_object_get_type_info`_                |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| void                 | fh_object_put_type_info(fh_object* self, const fh_type_info* typeInfo)                | `fh_object_put_type_info`_                |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| bool                 | fh_object_is_alias(@Nullable fh_object* a, @Nullable fh_object* b)                    | `fh_object_equals`_                       |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+
| enum fh_object_type  | fh_object_get_type(fh_object* self)                                                   | `fh_object_get_type`_                     |
+----------------------+---------------------------------------------------------------------------------------+-------------------------------------------+

Constructor detail
#####################
.. code-block:: c

   @Nullable
   fh_object* fh_alloc_object(fh_descriptor* desc)

Allocates the object on current context

Since
=====
Version 0.1

Parameters
==========
  ``desc`` - Metadata about data contained inside

Return value
============
The allocated object or NULL

Tags
====
GC-May-Invoke Need-Valid-Context

Method details
##############

fh_object_read_data
*******************
.. code-block:: c

   void fh_object_read_data(fh_object* self, void* buffer, size_t offset, size_t size)

Reads the content of object ranging ``[offset, offset + size)``. This
is meaningless or useless on reference array

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Object to perform read on
  ``buffer`` - Buffer to read into
  ``offset`` - Offset within the object
  ``size`` - Size to read

Tags
====
GC-Safepoint Need-Valid-Context

fh_object_write_data
********************
.. code-block:: c

   void fh_object_write_data(fh_object* self, const void* buffer, size_t offset, size_t size)

Write ``buffer`` content into ``[offset, offset + size)`` region. This
is undefined on reference array. Data written must be considered that
the data may move (as with moving GC implementation) so don't keep fixed
pointer to it

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Object to perform write on
  ``buffer`` - Buffer to write from
  ``offset`` - Offset within the object
  ``size`` - Size to write

Tags
====
GC-Safepoint Need-Valid-Context

fh_object_wait
**************
.. code-block:: c

   void fh_object_wait(fh_object* self, @Nullable const struct timespec* timeout)

Wait until object is notified. The object must be already
locked for this to be valid call. 
Must init with ``fh_object_init_synchronization_structs`` before calling this

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Object to perform operation on
  ``timeout`` - Maximum time to wait before failing or
                NULL to wait forever

Tags
====
GC-Safepoint

.. _fh_object_wake-and-fh_object_wake_all:
fh_object_wake and fh_object_wake_all
*************************************
.. code-block:: c

   void fh_object_wake(fh_object* self)
   void fh_object_wake_all(fh_object* self)

Wake thread waiting on ``self`` (incase of 
``fh_object_wake_all`` wake all threads)
Must init with ``fh_object_init_synchronization_structs`` before calling this

Tags
====
GC-Safepoint

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Current object to do operation on

.. _fh_object_lock-and-fh_object_unlock:
fh_object_lock and fh_object_unlock
***********************************
.. code-block:: c

   void fh_object_lock(fh_object* self);
   void fh_object_unlock(fh_object* self);

Lock the object and unlock the object. Must init with
``fh_object_init_synchronization_structs`` before calling this

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Current object to do operation on

Tags
====
GC-Safepoint


fh_object_get_type_info
************************
.. code-block:: c

   const fh_type_info* fh_object_get_type_info(fh_object* self)

Get type info for current object. The descriptor
contained in the type info will remain valid as long
the type info hasnt been put. To use
any descriptor after the ``fh_object_put_type_info`` 
program must acquire before put call. The type info must
not be used in another context than the caller. This last
cant last longer than the object itself


Since
=====
Version 0.1

Parameters
==========
  ``self`` - Current object to do operation on

Reurn value
===========
The requested type info (must be returned by ``fh_object_put_type_info``  call)

Tags
====
GC-Safepoint

fh_object_put_type_info
***********************
.. code-block:: c

   void fh_object_put_type_info(fh_object* self, const fh_type_info* typeInfo)

Release the associated resources (descriptor and other) and invalidate the typeinfo
for future uses and must be called on same cotext as the initial get

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Current object to do operation on
  ``typeInfo`` - The type info got from ``fh_object_get_type_info``

Tags
=====
GC-Safepoint

fh_object_read_ref
********************
.. code-block:: c

   fh_object* fh_object_read_ref(fh_array* self, size_t offset)

Read object pointer at ``offset`` and adds to current root 

Since
=====
Version 0.1

Parameters
==========
  ``self`` -  Object to do operation on
  ``offset`` - The offset to read from

Reurn value
===========
The requested object at the offset

Tags
====
GC-Safepoint Need-Valid-Context

fh_object_write_ref
********************
.. code-block:: c

   void fh_object_write_ref(fh_array* self, size_t offset, @Nullable fh_object* data)

Writes ``data`` object reference to offset at ``offset``

Since
=====
Version 0.1

Parameters
==========
  ``self`` -  Array to do operation on
  ``offset`` - The index to write to

Tags
====
GC-Safepoint Need-Valid-Context

fh_object_equals
****************
.. code-block:: c

   bool fh_object_is_alias(@Nullable fh_object* a, @Nullable fh_object* b)

Check if ``a`` and ``b`` refers to same object.

Since
=====
Version 0.1

Parameters
==========
  ``a`` - First object to compare
  ``b`` - Second object to compare

Return value
============
True if both objects refers to same object

Tags
====
GC-Safepoint

fh_object_init_synchronization_structs
*******************************
.. code-block:: c

   int fh_object_init_synchronization_structs(fh_object* self)

Init synchronization related structures for use. Concurrent
init is undefined because there no synchronization primiteves
ready for use to synchronize the initialization

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Object to init on

Return value
============
0 on success

Errors:
  -ENOMEM: Not enough memory to initialize the synchronization structure
  -EAGAIN: System limit for the synchronization structure reached

Tags
====
GC-Safepoint

fh_object_get_type
******************
.. code-block:: c

   enum fh_object_type fh_object_get_type(fh_object* self)

Gets object type of ``self``

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Object to retrieve from

Return value
============
The retrieved result

Tags
====
GC-Safepoint
