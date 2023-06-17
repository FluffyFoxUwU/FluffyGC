``fh_object*``
##############

The type which holds reference and allow access to the object

Since
*****
Version 0.1

Methods
#######

+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| Return value         | Method name                                                                           | Link                                     |
+======================+=======================================================================================+==========================================+
| void                 | fh_object_read_data(fh_object* self, void* buffer, size_t offset, size_t size)        | `fh_object_read_data`_                   |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| void                 | fh_object_write_data(fh_object* self, const void* buffer, size_t offset, size_t size) | `fh_object_write_data`_                  |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| @Nullable fh_object* | fh_object_read_ref(fh_object* self, size_t offset)                                    | `fh_object_read_ref`_                    |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| void                 | fh_object_write_ref(fh_object* self, size_t offset, @Nullable fh_object* data)        | `fh_object_write_ref`_                   |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| int                  | fh_init_synchronization_structs(fh_object* self)                                      | `fh_init_synchronization_structs`_       |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| void                 | fh_object_wait(fh_object* self, @Nullable const struct timespec* timeout)             | `fh_object_wait`_                        |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| void                 | fh_object_wake(fh_object* self)                                                       | `fh_object_wake-and-fh_object_wake_all`_ |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| void                 | fh_object_wake_all(fh_object* self)                                                   | `fh_object_wake-and-fh_object_wake_all`_ |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| void                 | fh_object_lock(fh_object* self)                                                       | `fh_object_lock-and-fh_object_unlock`_   |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| void                 | fh_object_unlock(fh_object* self)                                                     | `fh_object_lock-and-fh_object_unlock`_   |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| fh_descriptor*       | fh_object_get_descriptor(fh_object* self)                                             | `fh_object_get_descriptor`_              |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| bool                 | fh_object_is_alias(@Nullable fh_object* a, @Nullable fh_object* b)                    | `fh_object_equals`_                      |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+
| enum fh_object_type  | fh_object_get_type(fh_object* self)                                                   | `fh_object_get_type`_                    |
+----------------------+---------------------------------------------------------------------------------------+------------------------------------------+

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
is meaningless or useless on reference array

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
Must init with ``fh_init_synchronization_structs`` before calling this

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
Must init with ``fh_init_synchronization_structs`` before calling this

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
``fh_init_synchronization_structs`` before calling this

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Current object to do operation on

Tags
====
GC-Safepoint


fh_object_get_descriptor
************************
.. code-block:: c

   fh_descriptor* fh_object_get_descriptor(fh_object* self)

Get descriptor for current object. The address returned is same
for exactly same object type so only need pointer comparison.
Descriptor return can be reused to create new instance (also
works for arrays or anything describe-able by descriptor)

Acquires the descriptor and need ``fh_release_descriptor``
to release the descriptor.

Since
=====
Version 0.1

Parameters
==========
  ``self`` - Current object to do operation on

Tags
====
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

fh_init_synchronization_structs
*******************************
.. code-block:: c

   int fh_init_synchronization_structs(fh_object* self)

Init synchronization related structures for use.

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
