``fh_array*`` (extends ``fh_object*``)
######################################

Object but extended for arrays

Since
*****
Version 0.1

Methods
#######

+----------------------+----------------------------------------------------------------------------------+---------------------------+
| Return value         | Method name                                                                      | Link                      |
+======================+==================================================================================+===========================+
| ssize_t              | fh_array_calc_offset(fh_array* self, size_t index)                              | `fh_array_calc_offset`_   |
+----------------------+----------------------------------------------------------------------------------+---------------------------+
| @Nullable fh_object* | fh_array_get_element(fh_array* self, size_t index)                              | ``fh_array_get_element`_` |
+----------------------+----------------------------------------------------------------------------------+---------------------------+
| void                 | fh_array_set_element(fh_array* self, size_t index, @Nullable fh_object* object) | ``fh_array_set_element`_` |
+----------------------+----------------------------------------------------------------------------------+---------------------------+

Constructors
############

+-------------------------------------------------------------------+------------------------+
| Name                                                              | Link                   |
+===================================================================+========================+
| fh_alloc_array(fh_descriptor* elementType, size_t length)         | `fh_alloc_array`_      |
+-------------------------------------------------------------------+------------------------+
| fh_alloc_data_array(size_t alignment, size_t length, size_t size) | `fh_alloc_data_array`_ |
+-------------------------------------------------------------------+------------------------+

Constructor #1 detail
#####################
.. code-block:: c

   @Nullable
   fh_array* fh_alloc_array(fh_descriptor* elementType, size_t length)

Allocates array of ``length`` containing references to ``elementType`` type.
Layout is same as `fh_alloc_data_array`_'s but ``size`` is ``sizeof(fh_object*)``
and ``alignment`` is ``alignof(fh_object*)``

Since
=====
Version 0.1

Parameters
==========
  ``elementType`` - Type of array element
  ``length`` - Array length in items

Return value
============
The allocated object or NULL

Tags
====
GC-May-Invoke Need-Valid-Context

.. _fh_alloc_data_array:
Constructor #2 detail
#####################
.. code-block:: c

   @Nullable
   fh_array* fh_alloc_data_array(size_t alignment, size_t length, size_t size)

Allocates array of ``length`` sized ``size`` each. Array layout
is like what a C array would be:
1. Array and its elements is aligned by ``alignment``
2. May have padding to statisfy ``alignment``, thus length * size may not return real size

Since
=====
Version 0.1

Parameters
==========
  ``alignment`` - Alignment of each member
  ``length`` - Array length
  ``size`` - Member size

Return value
============
The allocated array or NULL

Tags
====
GC-May-Invoke Need-Valid-Context

Method detail
#############

fh_array_calc_offset
********************
.. code-block:: c

   ssize_t fh_array_calc_offset(fh_array* self, size_t index)

Calculate offset for the data operation and
can be used as other way than using array's
``fh_array_get_element`` and ``fh_array_set_element``

Since
=====
Version 0.1

Parameters
==========
  ``self`` -  Array to do operation on
  ``index`` - The index the offset generated from

Tags
====
GC-Safepoint

fh_array_get_element
********************
.. code-block:: c

   fh_object* fh_array_get_element(fh_array* self, size_t index)

Gets element as if by
.. code-block:: c

   ssize_t offset = fh_array_calc_offset(self, index);
   if (offset >= 0)
     return fh_object_read_ref(self, offset);
   else
     return NULL;

Since
=====
Version 0.1

Parameters
==========
  ``self`` -  Array to do operation on
  ``index`` - The index to read from

Reurn value
===========
The requested object at the index

Tags
====
GC-Safepoint Need-Valid-Context

fh_array_set_element
********************
.. code-block:: c

   void fh_array_set_element(fh_array* self, size_t index, @Nullable fh_object* data)

Sets element as if by
.. code-block:: c

   ssize_t offset = fh_array_calc_offset(self, index);
   if (offset >= 0)
     fh_object_write_ref(self, offset, data);

Since
=====
Version 0.1

Parameters
==========
  ``self`` -  Array to do operation on
  ``index`` - The index to write to

Tags
====
GC-Safepoint Need-Valid-Context
