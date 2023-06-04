*****************
``struct cuteheap*``
*****************

``struct cuteheap*`` is the primary state where all
operations operated on it serves these purpose

1. Managing contexts
2. Allocating
3. GC controls

Since
*****
Version 0.1

Functions
****************

Static functions
################
+------------------+--------------------------------------------+------------------+
| Return value     | Function description                       | Link             |
+==================+============================================+==================+
| struct cuteheap* | cuteheap_new(struct cuteheap_param* param) | `cuteheap_new`_  |
+------------------+--------------------------------------------+------------------+
| void             | cuteheap_free(struct cuteheap* self)       | `cuteheap_free`_ |
+------------------+--------------------------------------------+------------------+

cuteheap_new `(Back to listing) <Functions>`
************
.. code-block:: c

   struct cuteheap* cuteheap_new(struct cuteheap_param* param)



